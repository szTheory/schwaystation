#!/bin/sh
# Collect a bounded, read-only triage artifact from the GitHub metadata APIs.
set -eu
if [ "$#" -ne 2 ] || [ "$1" != collect ]; then
  printf '%s\n' 'usage: triage-report.sh collect OUTPUT' >&2
  exit 2
fi
output=$2
repo=${GITHUB_REPOSITORY:-}
if [ -z "${GH_TOKEN:-}" ] || [ -z "$repo" ]; then
  printf '%s\n' 'triage-report: GH_TOKEN and GITHUB_REPOSITORY are required' >&2
  exit 1
fi
owner=${repo%%/*}
name=${repo#*/}
case "$repo" in */*) ;; *) owner=; name= ;; esac
case "$owner/$name" in *[!A-Za-z0-9_./-]*|/*|*/|*/*/*)
  printf '%s\n' 'triage-report: invalid GITHUB_REPOSITORY' >&2
  exit 1 ;;
esac
umask 077
scratch=$(mktemp -d "${TMPDIR:-/tmp}/schwaystation-triage.XXXXXX") || {
  printf '%s\n' 'triage-report: cannot create private scratch directory' >&2
  exit 1
}
report_tmp=
cleanup() { rm -rf "$scratch"; [ -z "$report_tmp" ] || rm -f "$report_tmp"; }
trap cleanup 0
trap 'exit 1' HUP INT TERM
api() {
  if ! gh api "$1" > "$2" 2>/dev/null; then
    printf '%s\n' 'triage-report: GitHub API request failed' >&2
    exit 1
  fi
}
api "repos/$repo/issues?state=open&per_page=100&page=1" "$scratch/issues.json"
for workflow in ci release nightly; do
  api "repos/$repo/actions/workflows/$workflow.yml/runs?per_page=1" "$scratch/$workflow.json"
done
case "$output" in */*) out_dir=${output%/*} ;; *) out_dir=. ;; esac
[ -n "$out_dir" ] || out_dir=/
report_tmp=$(mktemp "$out_dir/.triage-report.XXXXXX") || {
  printf '%s\n' 'triage-report: cannot create output file' >&2
  exit 1
}
if ! jq -n --slurpfile issues "$scratch/issues.json" --slurpfile ci "$scratch/ci.json" \
    --slurpfile release "$scratch/release.json" --slurpfile nightly "$scratch/nightly.json" '
  def object: type == "object";
  def positive_id: type == "number" and . > 0 and . <= 9007199254740991 and floor == .;
  def timestamp: type == "string" and test("^[0-9]{4}-(0[1-9]|1[0-2])-(0[1-9]|[12][0-9]|3[01])T([01][0-9]|2[0-3]):[0-5][0-9]:[0-5][0-9]Z\\z");
  def choice($values): . as $value | $values | index($value) != null;
  def scrub($limit):
    gsub("(?:\u001b[]PX^_]|[\u0090\u0098\u009d-\u009f])[^\u0007\u001b\u009c]*(?:\u0007|\u001b\\\\|\u009c|\\z)"; "")
    | gsub("(?:\u001b\\[|\u009b)[0-?]*[ -/]*[@-~]|\u001b[ -/]*[@-~]"; "")
    | gsub("[\\p{Cc}\\p{Cf}]"; "")
    | gsub("(?i)[a-z][a-z0-9+.-]*://[^\\s<>]+"; "[redacted]")
    | gsub("(?i)(?<before>^|[^[:alnum:]_])(?:[a-z]:[/\\\\]|\\\\\\\\|/[^\\s])[^\"<>|`]*"; "\\(.before)[redacted]")
    | gsub("[^\\s<>@]+@[^\\s<>@]+"; "[redacted]")
    | gsub("(?i)[a-f0-9]{7,}"; "[redacted]") | .[:$limit];
  def issue:
    if (.number | positive_id) and (.title | type == "string") and
       (.labels | type == "array" and all(.[]; object and (.name | type == "string"))) and
       (.created_at | timestamp) and (.updated_at | timestamp)
    then {id:.number,title:(.title | scrub(160)),labels:[.labels[:8][] | .name | scrub(48)],created_at,updated_at}
    else error("invalid issue") end;
  def workflow($name):
    .workflow_runs | if type == "array" then . else error("invalid workflow response") end
    | if length == 0 then {workflow:$name,run_id:null,status:null,conclusion:null,updated_at:null}
      else .[0] | if object and (.id | positive_id) and (.updated_at | timestamp) and
        (.status | choice(["completed","queued","in_progress","requested","waiting","pending"])) and
        has("conclusion") and (.conclusion | choice([null,"action_required","cancelled","failure","neutral","skipped","stale","startup_failure","success","timed_out"]))
      then {workflow:$name,run_id:.id,status,conclusion,updated_at} else error("invalid run") end end;
  if ($issues | length == 1 and (.[0] | type == "array" and all(.[]; object))) and
     ($ci | length == 1 and (.[0] | object)) and ($release | length == 1 and (.[0] | object)) and
     ($nightly | length == 1 and (.[0] | object)) then
    ($issues[0] | map(select(has("pull_request") | not))) as $rows
    | {schema_version:1,issues_truncated:($issues[0] | length >= 100),issues:[$rows[:100][] | issue],
       workflows:[($ci[0] | workflow("ci")),($release[0] | workflow("release")),($nightly[0] | workflow("nightly"))]}
  else error("invalid input") end
' > "$report_tmp" 2>/dev/null; then
  printf '%s\n' 'triage-report: invalid GitHub metadata' >&2
  exit 1
fi
if ! jq -e 'type == "object" and keys == ["issues","issues_truncated","schema_version","workflows"] and
  .schema_version == 1 and (.issues_truncated | type == "boolean") and
  (.workflows | length == 3 and map(.workflow) == ["ci","release","nightly"])' "$report_tmp" >/dev/null 2>&1; then
  printf '%s\n' 'triage-report: invalid GitHub metadata' >&2
  exit 1
fi
mv -f "$report_tmp" "$output"
report_tmp=
