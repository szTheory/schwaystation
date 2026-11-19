#!/bin/sh
set -eu

fail() { printf '%s\n' "ci timing: $*" >&2; exit 1; }
pages=${1:-${CI_JOBS_FILE:-}}
[ -n "$pages" ] && [ -f "$pages" ] || fail "missing paginated Actions jobs JSON"
[ -n "${GITHUB_SHA:-}" ] && [ -n "${GITHUB_RUN_ID:-}" ] && [ -n "${GITHUB_RUN_ATTEMPT:-}" ] || fail "missing current run identity"
[ -n "${GITHUB_PR_HEAD_SHA:-}" ] || fail "missing pull request head SHA"
case "$GITHUB_SHA" in *[!0-9a-f]*|'') fail "invalid current commit SHA";; esac
case "$GITHUB_PR_HEAD_SHA" in *[!0-9a-f]*|'') fail "invalid pull request head SHA";; esac
[ "${#GITHUB_PR_HEAD_SHA}" -eq 40 ] || fail "invalid pull request head SHA"
case "$GITHUB_RUN_ID:$GITHUB_RUN_ATTEMPT" in *[!0-9:]*|:*|*:) fail "invalid run attempt identity";; esac
[ "${GITHUB_EVENT_NAME:-}" = pull_request ] || fail "timing applies only to pull requests"
[ "${CI_HEAVY:-}" = true ] && [ "${CI_CALLED:-}" = false ] || fail "timing applies only to full, uncalled CI"
[ -n "${GITHUB_HEAD_REF:-}" ] || fail "missing pull request branch identity"
release_inputs=${CI_RELEASE_INPUTS:-false}
case "$release_inputs" in true|false) ;; *) fail "invalid release-input route";; esac

root=$(git rev-parse --show-toplevel) || fail "not in a git checkout"
actual_head=$(git -C "$root" rev-parse HEAD) || fail "could not resolve checkout head"
[ "$actual_head" = "$GITHUB_SHA" ] || fail "checkout HEAD does not match GITHUB_SHA"
input_root=$root
if [ "${CI_TESTING:-false}" = true ]; then
  input_root=${CI_INPUT_ROOT:-}
  [ -n "$input_root" ] && [ -d "$input_root" ] || fail "missing test-only input tree"
elif [ -n "${CI_INPUT_ROOT:-}" ]; then
  fail "test-only workload input override is disabled"
fi

jq -e --arg run "$GITHUB_RUN_ID" --arg attempt "$GITHUB_RUN_ATTEMPT" --arg head "$GITHUB_PR_HEAD_SHA" \
  --arg branch "$GITHUB_HEAD_REF" '
  . as $pages | ($pages[0].total_count) as $total |
  ($pages | type == "array" and length > 0) and ($total | type == "number") and
  ($pages | all(.[]; .total_count == $total and (.jobs | type == "array"))) and
  ([$pages[].jobs[]] as $jobs |
    ($jobs | length) == $total and
    ($jobs | map(.id) | unique | length) == ($jobs | length) and
    ($jobs | map(.name) | unique | length) == ($jobs | length) and
    all($jobs[]; ((.run_id | tostring) == $run) and ((.run_attempt | tostring) == $attempt)
      and (.head_sha | type == "string" and length == 40) and .head_sha == $head
      and (.head_branch == $branch)
      and (.labels | type == "array" and length > 0 and all(.[]; type == "string" and length > 0))) and
    ($jobs | map(.head_sha) | unique | length) == 1)
' "$pages" >/dev/null 2>&1 || fail "malformed, stale, or incomplete pagination/jobs data"

required='["changes","hygiene","title","gpu-reference","openbios","build (linux-x64)","build (linux-arm64)","build (macos-arm64)","build (macos-x64)","build (windows-x64)","build (windows-arm64)","asan","nofp (ubuntu-26.04)","nofp (ubuntu-26.04-arm)","be32","wasm32","hash-equality","retroarch","perf","rehearsal","cleanup","CI required"]'
jq -e --argjson required "$required" '
  ([.[].jobs[].name] as $names | ($required - $names) == [] and ($names - $required) == [])
' "$pages" >/dev/null 2>&1 || fail "missing or unexpected required job identity"
for group in 'build (' 'nofp ('; do
  count=$(jq -r --arg prefix "$group" '[.[].jobs[] | select(.name | startswith($prefix)) | .labels | join(",")] | unique | length' "$pages")
  if [ "$group" = 'build (' ]; then expected=6; else expected=2; fi
  [ "$count" -eq "$expected" ] || fail "duplicate or missing $group runner labels"
done

tmp=${TMPDIR:-/tmp}/schway-ci-path.$$
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
mkdir -p "$tmp"
flat="$tmp/jobs.json"
jq '[.[].jobs[]]' "$pages" > "$flat" || fail "could not flatten paginated job records"
jq -r '
  .[] | [.name,.status,(.conclusion // ""),
    (if .started_at == null or .completed_at == null then ""
     elif (.started_at | type == "string" and test("^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z$")) and
          (.completed_at | type == "string" and test("^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z$"))
     then ((.completed_at | fromdateiso8601) - (.started_at | fromdateiso8601)) | tostring
     else error("job timestamp") end)] | map(tostring) | join("|")
' "$flat" > "$tmp/job-times" || fail "invalid UTC timestamps in completed jobs"
ci_started=$(jq -r '[.[] | select(.name == "CI required")][0].started_at // ""' "$flat")
printf '%s\n' "$ci_started" | grep -Eq '^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z$' || fail "invalid current gate start time"
d_changes= d_hygiene= d_title= d_gpu= d_openbios= d_asan=
d_build_linux_x64= d_build_linux_arm64= d_build_macos_arm64= d_build_macos_x64=
d_build_windows_x64= d_build_windows_arm64= d_nofp_linux= d_nofp_linux_arm=
d_be32= d_wasm32= d_hash= d_retroarch= d_perf= d_rehearsal= d_cleanup=
while IFS='|' read -r name status result value; do
  if [ "$name" = "CI required" ]; then
    [ "$status" = in_progress ] && [ -z "$result" ] && [ -z "$value" ] || fail "current CI required job is not the active attempt"
    continue
  fi
  if [ "$result" = skipped ] && { [ "$name" = rehearsal ] || [ "$name" = cleanup ]; } && [ "${CI_RELEASE_INPUTS:-false}" = false ]; then
    value=0
  else
    [ "$status" = completed ] && [ "$result" = success ] || fail "required job did not complete successfully: $name"
  fi
  case "$value" in *[!0-9]*|'') fail "invalid elapsed seconds for job: $name";; esac
  case "$name" in
    changes) d_changes=$value;; hygiene) d_hygiene=$value;; title) d_title=$value;;
    gpu-reference) d_gpu=$value;; openbios) d_openbios=$value;; asan) d_asan=$value;;
    'build (linux-x64)') d_build_linux_x64=$value;; 'build (linux-arm64)') d_build_linux_arm64=$value;;
    'build (macos-arm64)') d_build_macos_arm64=$value;; 'build (macos-x64)') d_build_macos_x64=$value;;
    'build (windows-x64)') d_build_windows_x64=$value;; 'build (windows-arm64)') d_build_windows_arm64=$value;;
    'nofp (ubuntu-26.04)') d_nofp_linux=$value;; 'nofp (ubuntu-26.04-arm)') d_nofp_linux_arm=$value;;
    be32) d_be32=$value;; wasm32) d_wasm32=$value;; hash-equality) d_hash=$value;;
    retroarch) d_retroarch=$value;; perf) d_perf=$value;; rehearsal) d_rehearsal=$value;; cleanup) d_cleanup=$value;;
    *) fail "unexpected job row: $name";;
  esac
done < "$tmp/job-times"

max_build=$d_build_linux_x64
for value in "$d_build_linux_arm64" "$d_build_macos_arm64" "$d_build_macos_x64" "$d_build_windows_x64" "$d_build_windows_arm64"; do
  [ "$value" -le "$max_build" ] || max_build=$value
done
max_nofp=$d_nofp_linux
[ "$d_nofp_linux_arm" -le "$max_nofp" ] || max_nofp=$d_nofp_linux_arm
through_openbios=$((d_changes + d_openbios))
through_build=$((through_openbios + max_build))
through_asan=$((through_openbios + d_asan))
through_nofp=$((d_changes + max_nofp))
through_be32=$((through_openbios + d_be32))
through_wasm32=$((through_openbios + d_wasm32))
through_hash=$(( $(printf '%s\n' "$through_build" "$through_be32" "$through_wasm32" | sort -nr | sed -n '1p') + d_hash ))
through_retroarch=$((through_build + d_retroarch))
through_perf=$((d_changes + d_perf))
through_rehearsal=$((through_build + d_rehearsal))
through_cleanup=$((through_rehearsal + d_cleanup))
critical_path=$(printf '%s\n' "$d_hygiene" "$d_title" "$((d_changes + d_gpu))" "$through_asan" \
  "$through_nofp" "$through_be32" "$through_wasm32" "$through_hash" "$through_retroarch" \
  "$through_perf" "$through_rehearsal" "$through_cleanup" | sort -nr | sed -n '1p')
[ "$critical_path" -gt 0 ] || fail "critical path must be positive"

paths="$tmp/paths"
if [ "${CI_TESTING:-false}" = true ]; then
  find "$input_root" -type f | sed "s|^$input_root/||" | awk '
    $0 == ".github/workflows/ci.yml" || $0 == "CMakePresets.json" || $0 == "pins.txt" ||
    $0 ~ /(^|\/)CMakeLists[.]txt$/ || $0 ~ /^cmake\/.*[.](cmake|txt)$/ ||
    $0 ~ /^tests\/.*\/(tests[.]cmake|inventory[.]txt|manifest[.]txt)$/ ||
    $0 ~ /^tests\/cmake\/.*[.]cmake$/ || (index($0, "scripts/") == 1 && index(substr($0, 9), "/") == 0 && $0 ~ /[.]sh$/) { print }' | LC_ALL=C sort > "$paths"
else
  git -C "$root" ls-tree -r --name-only "$GITHUB_SHA" | awk '
    $0 == ".github/workflows/ci.yml" || $0 == "CMakePresets.json" || $0 == "pins.txt" ||
    $0 ~ /(^|\/)CMakeLists[.]txt$/ || $0 ~ /^cmake\/.*[.](cmake|txt)$/ ||
    $0 ~ /^tests\/.*\/(tests[.]cmake|inventory[.]txt|manifest[.]txt)$/ ||
    $0 ~ /^tests\/cmake\/.*[.]cmake$/ || (index($0, "scripts/") == 1 && index(substr($0, 9), "/") == 0 && $0 ~ /[.]sh$/) { print }' | LC_ALL=C sort > "$paths"
fi
grep -qx '.github/workflows/ci.yml' "$paths" || fail "CI workflow is missing from the workload projection"
stream="$tmp/workload"
: > "$stream"
while IFS= read -r path; do
  [ "$path" = .github/workflows/ci.yml ] || continue
  source="$input_root/$path"
  [ -f "$source" ] && [ ! -L "$source" ] || fail "missing workload input: $path"
  normalized="$tmp/record"
  sed -e '/^[[:space:]]*#/d' -e '/^[[:space:]]*name:[[:space:]]/d' \
    -e '/^[[:space:]]*timeout-minutes:[[:space:]]/d' -e 's/[[:space:]]*$//' -e '/^$/d' "$source" > "$normalized"
  printf '%s\0' "$path" >> "$stream"
  cat "$normalized" >> "$stream"
  printf '\n' >> "$stream"
done < "$paths"
printf '%s\0' '@actual-runner-labels' >> "$stream"
jq -c '[.[].jobs[] | [.name, (.labels | sort)]] | sort_by(.[0])' "$pages" >> "$stream"
printf '\n' >> "$stream"
branch_class=non-main
[ "$GITHUB_HEAD_REF" = main ] && branch_class=main
printf '%s\0%s\0%s\0%s\0%s\n' '@workflow-route' ci.yml pull_request "$branch_class" "$release_inputs" >> "$stream"
while IFS= read -r path; do
  [ "$path" = .github/workflows/ci.yml ] && continue
  source="$input_root/$path"
  [ -f "$source" ] && [ ! -L "$source" ] || fail "missing workload input: $path"
  normalized="$tmp/record"
  case "$path" in
    pins.txt|tests/fixtures/manifest.txt)
      awk 'NF && $0 !~ /^[[:space:]]*#/ { print $0 }' "$source" | LC_ALL=C sort > "$normalized";;
    *.json) jq -cS . "$source" > "$normalized" || fail "invalid JSON workload input: $path";;
    scripts/*.sh)
      awk 'substr($0, 1, 2) == "#!" { print; next } /^[[:space:]]*#/ { next } { sub(/[[:space:]]+$/, ""); if (length) print }' "$source" > "$normalized";;
    *.cmake|*.txt|*/CMakeLists.txt|CMakeLists.txt)
      sed -e '/^[[:space:]]*#/d' -e 's/[[:space:]]*$//' -e '/^$/d' "$source" > "$normalized";;
    *) sed -e 's/\r$//' -e 's/[[:space:]]*$//' -e '/^$/d' "$source" > "$normalized";;
  esac
  printf '%s\0' "$path" >> "$stream"
  cat "$normalized" >> "$stream"
  printf '\n' >> "$stream"
done < "$paths"
if command -v sha256sum >/dev/null 2>&1; then
  fingerprint=$(sha256sum "$stream" | awk '{print $1}')
else
  fingerprint=$(shasum -a 256 "$stream" | awk '{print $1}')
fi

quality=${QUALITY_FILE:-quality.json}
case "$quality" in /*) ;; *) quality="$root/$quality";; esac
cmake "-DQUALITY=$quality" -P "$root/tests/cmake/quality.cmake" || fail "invalid quality registry"
baseline=$(jq -er '.ci.critical_path_s.workload_sha256 | select(test("^[0-9a-f]{64}$"))' "$quality") || fail "missing CI workload baseline"
printf 'ci timing: measured=%s s fingerprint=%s\n' "$critical_path" "$fingerprint"
if [ "$fingerprint" != "$baseline" ]; then
  printf 'ci timing: unbaselined workload (recorded=%s); comparison deferred until the fingerprint is refreshed\n' "$baseline"
  exit 0
fi
cmake "-DQUALITY=$quality" -DMETRIC=ci.critical_path_s "-DMEASURED=$critical_path" \
  -P "$root/tests/cmake/quality.cmake"
printf 'ci timing: comparable workload passed at %s seconds\n' "$critical_path"
