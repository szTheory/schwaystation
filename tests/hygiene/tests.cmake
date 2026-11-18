# Hygiene tests, registered only by the hygiene preset (SCHWAYSTATION_HYGIENE).
set(hygiene_scripts ${CMAKE_CURRENT_LIST_DIR}/../cmake)

# Fault prevented: repository-check code growing past its budget. Every line
# of the shell scripts, the CMake check scripts and the hooks counts, at most
# 2700, the approved absolute ceiling; Phase 6 ratchets to its measured total at ship.
schway_add_test(NAME policy.gate_budget
  COMMAND ${CMAKE_COMMAND} -DROOT=${CMAKE_SOURCE_DIR}
    "-DPATTERNS=scripts/*.sh$<SEMICOLON>tests/cmake/*.cmake$<SEMICOLON>.githooks/*" -DLIMIT=2700
    -P ${hygiene_scripts}/line_budget.cmake
  LABELS hygiene policy TIMEOUT 30)
set(hygiene_budget ${CMAKE_BINARY_DIR}/hygiene/line_budget_fixture)
file(REMOVE_RECURSE ${hygiene_budget})
string(REPEAT "line\n" 10 hygiene_lines)
file(CONFIGURE OUTPUT ${hygiene_budget}/sample.txt CONTENT "${hygiene_lines}" NEWLINE_STYLE UNIX)
set(hygiene_cmd ${CMAKE_COMMAND} -DROOT=${hygiene_budget} -DPATTERNS=* -DLIMIT=9
  -P ${hygiene_scripts}/line_budget.cmake)
string(REPLACE ";" "$<SEMICOLON>" hygiene_cmd "${hygiene_cmd}")
schway_add_test(NAME policy.gate_budget.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${hygiene_cmd}" -DCODE=1
    "-DREGEX=line_budget: 10 lines in .* exceeds the limit of 9"
    -P ${hygiene_scripts}/expect_fail.cmake
  LABELS hygiene policy TIMEOUT 30)

# Fault prevented: a committed third-party binary or table whose notice does
# not name it, or a notice naming a path that does not exist. The self test's
# fixture holds a one-row manifest whose path is on no Applies-to line and a
# notices file that names a missing path; both must be named.
schway_add_test(NAME policy.notices
  COMMAND ${CMAKE_COMMAND} -DROOT=${CMAKE_SOURCE_DIR}
    -DMANIFEST=${CMAKE_SOURCE_DIR}/tests/fixtures/manifest.txt
    -DNOTICES=${CMAKE_SOURCE_DIR}/THIRD-PARTY-NOTICES.md -P ${hygiene_scripts}/notices.cmake
  LABELS hygiene policy TIMEOUT 30)
set(hygiene_notices ${CMAKE_BINARY_DIR}/hygiene/notices_fixture)
file(REMOVE_RECURSE ${hygiene_notices})
file(WRITE ${hygiene_notices}/manifest.txt "# fixture\nstray/unlisted.bin\tsource\tpin\tMIT\tdigest\tNOTICES.md\n")
file(WRITE ${hygiene_notices}/NOTICES.md "## Fixture\n\n- **Applies to:** `gone/missing.txt`\n")
set(hygiene_cmd ${CMAKE_COMMAND} -DROOT=${hygiene_notices} -DMANIFEST=${hygiene_notices}/manifest.txt
  -DNOTICES=${hygiene_notices}/NOTICES.md -P ${hygiene_scripts}/notices.cmake)
string(REPLACE ";" "$<SEMICOLON>" hygiene_cmd "${hygiene_cmd}")
schway_add_test(NAME policy.notices.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${hygiene_cmd}" -DCODE=1
    "-DREGEX=gone/missing\\.txt, which does not exist.*stray/unlisted\\.bin has a notice in the manifest"
    -P ${hygiene_scripts}/expect_fail.cmake
  LABELS hygiene policy TIMEOUT 30)

# Fault prevented: a personal path or address, an unlisted binary, a game
# image or GPL text in any file git would track, ignored build output aside.
schway_add_test(NAME hygiene.tree
  COMMAND ${SCHWAYSTATION_SH} ${CMAKE_SOURCE_DIR}/scripts/hygiene.sh --tree
  LABELS hygiene TIMEOUT 60)

# Fault prevented: a tree scan that passes whatever the tree holds. A
# throwaway repository holds a copy of scripts/hygiene.sh and one untracked,
# not ignored file holding a home directory path and an address, both of
# which must be named. Both are built in pieces, so this file holds neither.
function(schway__hygiene_repo dir)
  file(REMOVE_RECURSE ${dir})
  file(MAKE_DIRECTORY ${dir}/scripts)
  file(COPY ${CMAKE_SOURCE_DIR}/scripts/hygiene.sh DESTINATION ${dir}/scripts)
  execute_process(COMMAND ${GIT_EXECUTABLE} init -q ${dir} RESULT_VARIABLE rc)
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "hygiene: git init ${dir} failed")
  endif()
endfunction()
set(hygiene_tree ${CMAKE_BINARY_DIR}/hygiene/tree_fixture)
schway__hygiene_repo(${hygiene_tree})
set(hygiene_home "/Use")
string(APPEND hygiene_home "rs/someone/projects/schwaystation")
set(hygiene_address "someone")
string(APPEND hygiene_address "@" "mail.test")
file(WRITE ${hygiene_tree}/notes.txt "built in ${hygiene_home}\nsent to ${hygiene_address}\n")
set(hygiene_cmd ${SCHWAYSTATION_SH} -c "cd \"$1\" && sh scripts/hygiene.sh --tree" sh ${hygiene_tree})
string(REPLACE ";" "$<SEMICOLON>" hygiene_cmd "${hygiene_cmd}")
schway_add_test(NAME hygiene.tree.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${hygiene_cmd}" -DCODE=1
    "-DREGEX=hygiene: home directory path: notes\\.txt.*hygiene: email address: notes\\.txt"
    -P ${hygiene_scripts}/expect_fail.cmake
  LABELS hygiene TIMEOUT 60)

# Fault prevented: a BIOS dump or a PlayStation executable entering the tree
# under a neutral name. The tree scan above detects both; its self test's
# throwaway repository holds a 524288-byte file of NUL bytes and a file whose
# first 8 bytes are decoded here from scripts/hygiene.sh's signature, so no
# tracked file holds either.
set(hygiene_bios ${CMAKE_BINARY_DIR}/hygiene/bios_fixture)
schway__hygiene_repo(${hygiene_bios})
execute_process(COMMAND ${SCHWAYSTATION_SH} -c "dd if=/dev/zero of=\"$1/blob.dat\" bs=1024 count=512 2>/dev/null"
  sh ${hygiene_bios} RESULT_VARIABLE hygiene_rc)
file(STRINGS ${CMAKE_SOURCE_DIR}/scripts/hygiene.sh hygiene_magic REGEX "magic=\\$\\(printf '[0-9a-f]+'\\)")
list(GET hygiene_magic 0 hygiene_magic)
string(REGEX MATCH "[0-9a-f][0-9a-f][0-9a-f][0-9a-f]+" hygiene_magic "${hygiene_magic}")
string(LENGTH "${hygiene_magic}" hygiene_len)
if(NOT hygiene_rc EQUAL 0 OR NOT hygiene_len EQUAL 16)
  message(FATAL_ERROR "hygiene: no 524288-byte fixture, or no 8-byte executable signature in scripts/hygiene.sh")
endif()
set(hygiene_bytes "")
foreach(hygiene_i RANGE 0 14 2)
  string(SUBSTRING "${hygiene_magic}" ${hygiene_i} 2 hygiene_pair)
  math(EXPR hygiene_code "0x${hygiene_pair}")
  string(ASCII ${hygiene_code} hygiene_char)
  string(APPEND hygiene_bytes "${hygiene_char}")
endforeach()
file(WRITE ${hygiene_bios}/sample.dat "${hygiene_bytes} and some text\n")
set(hygiene_cmd ${SCHWAYSTATION_SH} -c "cd \"$1\" && sh scripts/hygiene.sh --tree" sh ${hygiene_bios})
string(REPLACE ";" "$<SEMICOLON>" hygiene_cmd "${hygiene_cmd}")
schway_add_test(NAME hygiene.no_bios_bytes.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${hygiene_cmd}" -DCODE=1
    "-DREGEX=BIOS image by size: blob\\.dat.*PlayStation executable by content: sample\\.dat"
    -P ${hygiene_scripts}/expect_fail.cmake
  LABELS hygiene TIMEOUT 60)

# Fault prevented: floating point in the core's text. The self test scans a
# fixture with six float lines beside two lines that only look like them.
schway_add_test(NAME hygiene.no_float
  COMMAND ${CMAKE_COMMAND} "-DROOTS=${CMAKE_SOURCE_DIR}/src$<SEMICOLON>${CMAKE_SOURCE_DIR}/include"
    -P ${hygiene_scripts}/no_float.cmake
  LABELS hygiene TIMEOUT 30)
set(hygiene_fixture ${CMAKE_BINARY_DIR}/hygiene/no_float_fixture)
file(REMOVE_RECURSE ${hygiene_fixture})
file(WRITE ${hygiene_fixture}/sample.c "int y = 0x1E5;\nint double_free = 2;\nx = 1.5;\n"
  "z = 0x1p3;\nw = 1e3;\ndouble d;\n#include <math.h>\n#include <float.h>\n")
set(hygiene_cmd ${CMAKE_COMMAND} -DROOTS=${hygiene_fixture} -P ${hygiene_scripts}/no_float.cmake)
string(REPLACE ";" "$<SEMICOLON>" hygiene_cmd "${hygiene_cmd}")
schway_add_test(NAME hygiene.no_float.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${hygiene_cmd}" -DCODE=1
    "-DREGEX=sample.c:3: x = 1.5.*sample.c:8: #include <float.h>.*no_float: 6 lines"
    -P ${hygiene_scripts}/expect_fail.cmake
  LABELS hygiene TIMEOUT 30)

# Fault prevented: a pull-request title or body that cuts the wrong release
# or none, or starts no CI run on main. Each fixture line is want|title|body,
# the body with \n escapes; scripts/check-title.sh must give every want, and
# the fixtures must hold both kinds, so a check that passes or fails
# everything fails here.
set(hygiene_titles [=[
script=$1
passes=0
fails=0
while IFS='|' read -r want title body
do
  if [ -z "$want" ]
  then continue
  fi
  if printf '%b\n' "$body" | sh "$script" "$title" > /dev/null 2>&1
  then got=pass
  else got=fail
  fi
  if [ "$got" != "$want" ]
  then
    echo "ci.title: want $want, got $got: $title|$body"
    exit 1
  fi
  if [ "$want" = pass ]
  then passes=$((passes + 1))
  else fails=$((fails + 1))
  fi
done < "$2"
echo "ci.title: $passes pass and $fails fail fixtures"
[ "$passes" -gt 0 ] && [ "$fails" -gt 0 ]
]=])
schway_add_test(NAME ci.title
  COMMAND ${SCHWAYSTATION_SH} -c "${hygiene_titles}" sh ${CMAKE_SOURCE_DIR}/scripts/check-title.sh
    ${CMAKE_CURRENT_LIST_DIR}/fixtures/titles.txt
  LABELS hygiene TIMEOUT 30)
# Fault prevented: a title check that accepts a "Phase N: Name" title, or
# fails it for some other reason than the one it names.
set(hygiene_cmd ${SCHWAYSTATION_SH} -c "sh \"$1\" \"$2\" < /dev/null" sh
  ${CMAKE_SOURCE_DIR}/scripts/check-title.sh "Phase 1: A test frame in RetroArch")
string(REPLACE ";" "$<SEMICOLON>" hygiene_cmd "${hygiene_cmd}")
schway_add_test(NAME ci.title.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${hygiene_cmd}" -DCODE=1 "-DREGEX=not a Conventional Commit title"
    -P ${hygiene_scripts}/expect_fail.cmake
  LABELS hygiene TIMEOUT 30)

# Fault prevented: a code change skipping CI as planning only, or a change to
# the release pipeline merging unrehearsed. heavy=false only for a non-empty
# list wholly under .planning/, both names of a rename included, from a pull
# request of at most 3000 files, the most the files API lists;
# release_inputs=true for any of the five release inputs and for any list
# that cannot be trusted. The route controls below execute the marked shell
# block from ci.yml itself, so this test covers event wiring as well as paths.
file(READ ${CMAKE_SOURCE_DIR}/.github/workflows/ci.yml hygiene_ci_workflow)
string(FIND "${hygiene_ci_workflow}" "# CI_CLASSIFIER_TEST_BEGIN\n" hygiene_ci_start)
string(FIND "${hygiene_ci_workflow}" "\n          # CI_CLASSIFIER_TEST_END" hygiene_ci_end)
if(hygiene_ci_start EQUAL -1 OR hygiene_ci_end EQUAL -1 OR hygiene_ci_end LESS hygiene_ci_start)
  message(FATAL_ERROR "ci.classify: missing workflow shell markers")
endif()
string(LENGTH "# CI_CLASSIFIER_TEST_BEGIN\n" hygiene_ci_marker_length)
math(EXPR hygiene_ci_start "${hygiene_ci_start} + ${hygiene_ci_marker_length}")
math(EXPR hygiene_ci_length "${hygiene_ci_end} - ${hygiene_ci_start}")
string(SUBSTRING "${hygiene_ci_workflow}" ${hygiene_ci_start} ${hygiene_ci_length} hygiene_ci_step)
string(REPLACE "\n          " "\n" hygiene_ci_step "${hygiene_ci_step}")
string(SUBSTRING "${hygiene_ci_step}" 10 -1 hygiene_ci_step)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/hygiene)
set(hygiene_ci_script ${CMAKE_BINARY_DIR}/hygiene/classifier-step.sh)
file(WRITE ${hygiene_ci_script} "#!/bin/bash\nset -euo pipefail\n${hygiene_ci_step}\n")
set(hygiene_classify [=[
workflow=$1
source_root=$2
fixture_root=$3
n=0
check_paths() {
  got=$(printf '%b' "$3" | sh "$source_root/scripts/classify-changes.sh" "${4-2}" | tr '\n' ' ')
  if [ "$got" != "heavy=$1 release_inputs=$2 " ]; then
    echo "ci.classify: want heavy=$1 release_inputs=$2, got $got for: $3"
    exit 1
  fi
  n=$((n + 1))
}
check_route() {
  name=$1 want=$2 event=$3 ref=$4 number=$5 call_ref=$6 before=$7 after=$8 checkout=$9
  route_path=${10:-$bin}
  gh_fail=${11:-}
  git -C "$runner" fetch -q --no-tags --depth=1 origin "$checkout"
  git -C "$runner" checkout -q --detach "$checkout"
  : > "$output"
  got=$(cd "$runner" && EVENT="$event" REF="$ref" NUMBER="$number" CALL_REF="$call_ref" \
    BEFORE="$before" AFTER="$after" CHANGED=1 GITHUB_WORKSPACE="$runner" \
    GITHUB_REPOSITORY=fixture/project GITHUB_OUTPUT="$output" GH_TOKEN=test-token GH_LIST_FAIL="$gh_fail" \
    PATH="$route_path:$PATH" bash -e -o pipefail "$workflow")
  output_got=$(tr '\n' ' ' < "$output")
  if [ "$output_got" != "$want" ]; then
    echo "ci.classify: $name wanted $want, got $output_got"
    echo "workflow stdout: $got"
    exit 1
  fi
  n=$((n + 1))
}
check_paths true false '.planning/STATE.md\nsrc/core.c\n'
check_paths true false 'src/core.c\n.planning/core.c\n'
check_paths true true ''
check_paths true true '\n\n'
check_paths true false '(listing failed)\n'
check_paths true false '.planning-notes/a.md\n'
check_paths true false 'docs/.planning/a.md\n'
check_paths false false '.planning/STATE.md\n.planning/phases/01/a.md\n'
check_paths false false '.planning/STATE.md'
check_paths false false '.planning/STATE.md\n' 3000
check_paths true true '.planning/STATE.md\n' 3001
check_paths true true '.planning/STATE.md\n' ''
check_paths true true '.planning/STATE.md\n' 'x'
check_paths true true '.github/workflows/release.yml\n'
check_paths true true '.github/workflows/ci.yml\n'
check_paths true true 'src/core.c\nscripts/publish.sh\n'
check_paths true true 'release-please-config.json\n'
check_paths true true 'cmake/packaging.cmake\n'
check_paths true true 'scripts/publish.sh\nscripts/release.sh\n'
check_paths true false 'scripts/publish.sh.orig\ndocs/release.yml\n'
check_paths false false '.planning/STATE.md\n' 1

rm -rf "$fixture_root"
mkdir -p "$fixture_root/repo/.planning" "$fixture_root/bin"
remote=$fixture_root/origin.git
repo=$fixture_root/repo
runner=$fixture_root/runner
output=$fixture_root/output
git init -q --bare "$remote"
git --git-dir="$remote" symbolic-ref HEAD refs/heads/main
git init -q "$repo"
git -C "$repo" checkout -q -b main
git -C "$repo" remote add origin "$remote"
git -C "$repo" config user.name CI
git -C "$repo" config user.email ci@users.noreply.github.com
mkdir -p "$repo/scripts"
cp "$source_root/scripts/classify-changes.sh" "$repo/scripts/classify-changes.sh"
printf '%s\n' 'rename seed' > "$repo/.planning/rename.txt"
commit() {
  git -C "$repo" add -A
  git -C "$repo" -c user.name=CI -c user.email=ci@users.noreply.github.com commit -qm "$1"
  git -C "$repo" rev-parse HEAD
}
base=$(commit base)
printf '%s\n' 'planning state' > "$repo/.planning/STATE.md"
planning=$(commit planning)
mkdir -p "$repo/src"
printf '%s\n' 'source' > "$repo/src/core.c"
source=$(commit source)
mv "$repo/.planning/rename.txt" "$repo/src/rename.txt"
planning_to_source=$(commit planning-to-source-rename)
printf '%s\n' 'rename seed' > "$repo/src/moved.txt"
source_seed=$(commit source-rename-seed)
mv "$repo/src/moved.txt" "$repo/.planning/moved.txt"
source_to_planning=$(commit source-to-planning-rename)
mkdir -p "$repo/.github/workflows"
printf '%s\n' 'release' > "$repo/.github/workflows/release.yml"
release=$(commit release-input)
weird=$(printf '.planning/innocent\nsrc/escaped.c')
mkdir -p "$(dirname "$repo/$weird")"
printf '%s\n' 'quoted path' > "$repo/$weird"
special=$(commit special-path)
mkdir -p "$repo/.planning/many"
i=1
while [ "$i" -le 3001 ]; do
  : > "$repo/.planning/many/$i"
  i=$((i + 1))
done
oversize=$(commit oversize)
git -C "$repo" push -q origin main
git clone -q --depth 1 "file://$remote" "$runner"

printf '%s\n' '#!/bin/sh' '[ "${GH_LIST_FAIL-}" = 1 ] && exit 1' \
  "printf '%s\\n' '.planning/STATE.md'" > "$fixture_root/bin/gh"
chmod +x "$fixture_root/bin/gh"
bin=$fixture_root/bin
git_path=$fixture_root/diff-failure
mkdir -p "$git_path"
real_git=$(command -v git)
printf '%s\n' '#!/bin/sh' 'for arg do' '  [ "$arg" = diff ] && exit 1' 'done' \
  "exec $real_git \"\$@\"" > "$git_path/git"
chmod +x "$git_path/git"

check_route main-planning 'heavy=false release_inputs=false ' push refs/heads/main '' '' "$base" "$planning" "$planning"
check_route main-source 'heavy=true release_inputs=false ' push refs/heads/main '' '' "$planning" "$source" "$source"
check_route rename-planning-to-source 'heavy=true release_inputs=false ' push refs/heads/main '' '' "$planning" "$planning_to_source" "$planning_to_source"
check_route rename-source-to-planning 'heavy=true release_inputs=false ' push refs/heads/main '' '' "$source_seed" "$source_to_planning" "$source_to_planning"
check_route release-input 'heavy=true release_inputs=true ' push refs/heads/main '' '' "$source_to_planning" "$release" "$release"
check_route quoted-path 'heavy=true release_inputs=false ' push refs/heads/main '' '' "$release" "$special" "$special"
check_route diff-failure 'heavy=true release_inputs=true ' push refs/heads/main '' '' "$base" "$planning" "$planning" "$git_path"
check_route over-limit 'heavy=true release_inputs=true ' push refs/heads/main '' '' "$special" "$oversize" "$oversize"
check_route empty-diff 'heavy=true release_inputs=true ' push refs/heads/main '' '' "$planning" "$planning" "$planning"
check_route missing-before 'heavy=true release_inputs=true ' push refs/heads/main '' '' '' "$planning" "$planning"
check_route malformed-before 'heavy=true release_inputs=true ' push refs/heads/main '' '' x "$planning" "$planning"
check_route zero-before 'heavy=true release_inputs=true ' push refs/heads/main '' '' 0000000000000000000000000000000000000000 "$planning" "$planning"
check_route unavailable-before 'heavy=true release_inputs=true ' push refs/heads/main '' '' 1111111111111111111111111111111111111111 "$planning" "$planning"
check_route missing-after 'heavy=true release_inputs=true ' push refs/heads/main '' '' "$base" '' "$planning"
check_route malformed-after 'heavy=true release_inputs=true ' push refs/heads/main '' '' "$base" x "$planning"
check_route zero-after 'heavy=true release_inputs=true ' push refs/heads/main '' '' "$base" 0000000000000000000000000000000000000000 "$planning"
check_route mismatch 'heavy=true release_inputs=true ' push refs/heads/main '' '' "$base" "$source" "$planning"
check_route release-call 'heavy=true release_inputs=true ' workflow_call '' '' "$base" '' '' "$planning"
check_route manual 'heavy=true release_inputs=true ' workflow_dispatch refs/heads/main '' '' "$planning" "$planning" "$planning"
check_route non-main-push 'heavy=true release_inputs=true ' push refs/heads/feature '' '' "$planning" "$planning" "$planning"
check_route pr-planning 'heavy=false release_inputs=false ' pull_request refs/pull/1/merge 1 '' '' '' "$planning"
check_route pr-listing-failure 'heavy=true release_inputs=false ' pull_request refs/pull/1/merge 1 '' '' '' "$planning" '' 1
echo "ci.classify: $n lists"
]=])
schway_add_test(NAME ci.classify
  COMMAND ${SCHWAYSTATION_SH} -c "${hygiene_classify}" sh ${hygiene_ci_script}
    ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/hygiene/ci-classifier
  LABELS hygiene TIMEOUT 30)

# Fault prevented: a release with a missing or extra archive, or a SHA256SUMS
# whose order differs from host to host. scripts/publish.sh prepare runs on 18
# empty placeholder zips and six build legs' commit.txt, and SHA256SUMS must
# list the 18 in C-locale name order. The self test leaves one zip out and
# adds one, which must each fail naming the zip.
set(hygiene_publish ${CMAKE_BINARY_DIR}/hygiene/publish_set)
file(REMOVE_RECURSE ${hygiene_publish})
foreach(c library runner libretro)
  foreach(o linux macos windows)
    foreach(a x64 arm64)
      file(WRITE ${hygiene_publish}/full/schwaystation-${c}-${o}-${a}.zip "")
      file(WRITE ${hygiene_publish}/long/schwaystation-${c}-${o}-${a}.zip "")
      if(NOT "${c}-${o}-${a}" STREQUAL "runner-windows-x64")
        file(WRITE ${hygiene_publish}/short/schwaystation-${c}-${o}-${a}.zip "")
      endif()
    endforeach()
  endforeach()
endforeach()
file(WRITE ${hygiene_publish}/long/schwaystation-runner-linux-x86.zip "")
foreach(leg RANGE 1 6)
  foreach(set full short long)
    file(WRITE ${hygiene_publish}/${set}/legs/${leg}/commit.txt "fixture\n")
  endforeach()
endforeach()
set(hygiene_publish_check [=[
EXPECTED_SHA=fixture sh "$1" prepare "$2" || exit 1
names=$(awk '{ print $2 }' "$2/SHA256SUMS")
count=$(printf '%s\n' "$names" | grep -c '^schwaystation-[a-z]*-[a-z]*-[a-z0-9]*\.zip$')
if [ "$count" -ne 18 ]
then
  echo "policy.publish_set: $count archives in SHA256SUMS, want 18"
  exit 1
fi
if ! printf '%s\n' "$names" | LC_ALL=C sort -c -u
then
  echo "policy.publish_set: SHA256SUMS is not in C-locale name order"
  exit 1
fi
echo "policy.publish_set: 18 archives in C-locale name order"
]=])
schway_add_test(NAME policy.publish_set
  COMMAND ${SCHWAYSTATION_SH} -c "${hygiene_publish_check}" sh ${CMAKE_SOURCE_DIR}/scripts/publish.sh
    ${hygiene_publish}/full
  LABELS hygiene policy TIMEOUT 30)
set(hygiene_cmd ${CMAKE_COMMAND} -E env EXPECTED_SHA=fixture ${SCHWAYSTATION_SH} -c
  "sh \"$1\" prepare \"$2\"\nsh \"$1\" prepare \"$3\"" sh
  ${CMAKE_SOURCE_DIR}/scripts/publish.sh ${hygiene_publish}/short ${hygiene_publish}/long)
string(REPLACE ";" "$<SEMICOLON>" hygiene_cmd "${hygiene_cmd}")
schway_add_test(NAME policy.publish_set.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${hygiene_cmd}" -DCODE=1
    "-DREGEX=publish: missing schwaystation-runner-windows-x64.zip.*publish: unexpected schwaystation-runner-linux-x86.zip"
    -P ${hygiene_scripts}/expect_fail.cmake
  LABELS hygiene policy TIMEOUT 30)

# Fault prevented: a release that verifies against a pull request's
# attestation. verify on a release tag with a pull request's ref must refuse
# before any network call.
set(hygiene_cmd ${CMAKE_COMMAND} -E env GITHUB_REPOSITORY=fixture/fixture SOURCE_REF=refs/pull/1/merge
  ${SCHWAYSTATION_SH} ${CMAKE_SOURCE_DIR}/scripts/publish.sh verify fixture-release ${hygiene_publish}/full)
string(REPLACE ";" "$<SEMICOLON>" hygiene_cmd "${hygiene_cmd}")
schway_add_test(NAME policy.publish_ref
  COMMAND ${CMAKE_COMMAND} "-DCMD=${hygiene_cmd}" -DCODE=1
    "-DREGEX=publish: fixture-release verifies only from refs/heads/main, not refs/pull/1/merge"
    -P ${hygiene_scripts}/expect_fail.cmake
  LABELS hygiene policy TIMEOUT 30)

# Fault prevented: a release that accepts downloads signed by the rehearsal's
# workflow. verify runs end to end against a gh and a curl on PATH that log
# their arguments and serve a fixture release; with SIGNER_WORKFLOW=ci.yml a
# release tag must still verify as release.yml, and a rehearsal- tag as ci.yml.
set(hygiene_signer ${CMAKE_BINARY_DIR}/hygiene/publish_signer)
file(REMOVE_RECURSE ${hygiene_signer})
foreach(c library runner libretro)
  foreach(o linux macos windows)
    foreach(a x64 arm64)
      file(WRITE ${hygiene_signer}/dist/schwaystation-${c}-${o}-${a}.zip "${c}-${o}-${a}\n")
    endforeach()
  endforeach()
endforeach()
foreach(leg RANGE 1 6)
  file(WRITE ${hygiene_signer}/dist/legs/${leg}/commit.txt "fixture\n")
endforeach()
# The original verifies from its ref; the flipped copy, checked with no ref,
# does not.
file(WRITE ${hygiene_signer}/bin/gh [=[
#!/bin/sh
echo "gh $*" >> "$STUB_LOG"
case "$*" in
  "attestation verify "*--source-ref*) exit 0 ;;
  "attestation verify "*) exit 1 ;;
  *isDraft*) echo false ;;
  *assets*) cd "$STUB_DIR" && ls schwaystation-*.zip SHA256SUMS ;;
esac
]=])
file(WRITE ${hygiene_signer}/bin/curl [=[
#!/bin/sh
while [ $# -gt 1 ]
do
  if [ "$1" = -o ]
  then out=$2
  fi
  shift
done
cp "$STUB_DIR/${1##*/}" "$out"
]=])
file(CHMOD ${hygiene_signer}/bin/gh ${hygiene_signer}/bin/curl
  FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
set(hygiene_signer_check [=[
script=$1
export STUB_DIR=$2/dist STUB_LOG=$2/gh.log RUNNER_TEMP=$2/tmp PATH="$2/bin:$PATH"
export GITHUB_REPOSITORY=fixture/fixture SIGNER_WORKFLOW=ci.yml EXPECTED_SHA=fixture
sh "$script" prepare "$STUB_DIR" > /dev/null || exit 1
for case in fixture-release:refs/heads/main:release.yml rehearsal-1-1:refs/pull/1/merge:ci.yml
do
  tag=${case%%:*} rest=${case#*:}
  : > "$STUB_LOG"
  SOURCE_REF=${rest%%:*} sh "$script" verify "$tag" "$STUB_DIR" > /dev/null || exit 1
  want="--signer-workflow fixture/fixture/.github/workflows/${rest#*:} "
  got=$(grep -c -e "$want" "$STUB_LOG")
  other=$(grep -e '--signer-workflow' "$STUB_LOG" | grep -c -v -e "$want")
  if [ "$got" -ne 18 ] || [ "$other" -ne 0 ]
  then
    echo "policy.publish_signer: $tag verified $got downloads as ${rest#*:} and $other otherwise"
    exit 1
  fi
done
echo "policy.publish_signer: a release tag verifies as release.yml, a rehearsal tag as ci.yml"
]=])
schway_add_test(NAME policy.publish_signer
  COMMAND ${SCHWAYSTATION_SH} -c "${hygiene_signer_check}" sh ${CMAKE_SOURCE_DIR}/scripts/publish.sh ${hygiene_signer}
  LABELS hygiene policy TIMEOUT 30)

# Fault prevented: real-host test code growing without bound. Every line
# under tests/retroarch counts, at most 300; the self test counts a fixture
# directory holding one 301-line file.
schway_add_test(NAME policy.e2e_budget
  COMMAND ${CMAKE_COMMAND} -DROOT=${CMAKE_SOURCE_DIR} -DPATTERNS=tests/retroarch/* -DLIMIT=300
    -P ${hygiene_scripts}/line_budget.cmake
  LABELS hygiene policy TIMEOUT 30)
set(hygiene_e2e ${CMAKE_BINARY_DIR}/hygiene/e2e_budget_fixture)
file(REMOVE_RECURSE ${hygiene_e2e})
string(REPEAT "line\n" 301 hygiene_lines)
file(CONFIGURE OUTPUT ${hygiene_e2e}/sample.cmake CONTENT "${hygiene_lines}" NEWLINE_STYLE UNIX)
set(hygiene_cmd ${CMAKE_COMMAND} -DROOT=${hygiene_e2e} -DPATTERNS=* -DLIMIT=300
  -P ${hygiene_scripts}/line_budget.cmake)
string(REPLACE ";" "$<SEMICOLON>" hygiene_cmd "${hygiene_cmd}")
schway_add_test(NAME policy.e2e_budget.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${hygiene_cmd}" -DCODE=1
    "-DREGEX=line_budget: 301 lines in .* exceeds the limit of 300"
    -P ${hygiene_scripts}/expect_fail.cmake
  LABELS hygiene policy TIMEOUT 30)

# Fault prevented: a job timeout guessed rather than measured. Every
# timeout-minutes line names a run ID or says provisional. The self test
# scans a fixture with two good lines and two bad ones.
schway_add_test(NAME hygiene.timeout_comments
  COMMAND ${CMAKE_COMMAND} -DMODE=timeouts -DDIR=${CMAKE_SOURCE_DIR}/.github/workflows
    -P ${hygiene_scripts}/workflows.cmake
  LABELS hygiene TIMEOUT 30)

# Fault prevented: sound acceptance jobs or their exact test inventories being disconnected.
schway_add_test(NAME policy.media_registration
  COMMAND ${CMAKE_COMMAND} -DROOT=${CMAKE_SOURCE_DIR} -DCASE=disc_registration
    -P ${hygiene_scripts}/workflows.cmake
  LABELS hygiene policy TIMEOUT 30)
set(hygiene_timeouts ${CMAKE_BINARY_DIR}/hygiene/timeouts_fixture)
file(REMOVE_RECURSE ${hygiene_timeouts})
string(REPEAT "1" 11 hygiene_run)
file(WRITE ${hygiene_timeouts}/sample.yml "jobs:\n  a:\n    timeout-minutes: 5  # provisional\n"
  "  b:\n    timeout-minutes: 30\n  c:\n    timeout-minutes: 8  # 2 x 3 min, run ${hygiene_run}\n"
  "  d:\n    timeout-minutes: 15  # a guess\n")
set(hygiene_cmd ${CMAKE_COMMAND} -DMODE=timeouts -DDIR=${hygiene_timeouts} -P ${hygiene_scripts}/workflows.cmake)
string(REPLACE ";" "$<SEMICOLON>" hygiene_cmd "${hygiene_cmd}")
schway_add_test(NAME hygiene.timeout_comments.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${hygiene_cmd}" -DCODE=1
    "-DREGEX=sample.yml: timeout-minutes: 30.*sample.yml: timeout-minutes: 15 .*a guess.*timeout_comments: 2 timeout-minutes: lines without a run ID or provisional"
    -P ${hygiene_scripts}/expect_fail.cmake
  LABELS hygiene TIMEOUT 30)

# Fault prevented: an action referenced by a tag that can be moved under the
# release. The self test scans a fixture that uses a tag.
schway_add_test(NAME policy.action_pins
  COMMAND ${CMAKE_COMMAND} -DMODE=pins -DDIR=${CMAKE_SOURCE_DIR}/.github/workflows
    -P ${hygiene_scripts}/workflows.cmake
  LABELS hygiene policy TIMEOUT 30)
set(hygiene_actions ${CMAKE_BINARY_DIR}/hygiene/action_pins_fixture)
file(REMOVE_RECURSE ${hygiene_actions})
string(REPEAT "0" 40 hygiene_sha)
file(WRITE ${hygiene_actions}/sample.yml "steps:\n  - uses: ./.github/actions/local\n"
  "  - uses: actions/checkout@${hygiene_sha} # v7.0.1\n  - uses: actions/checkout@v4\n")
set(hygiene_cmd ${CMAKE_COMMAND} -DMODE=pins -DDIR=${hygiene_actions} -P ${hygiene_scripts}/workflows.cmake)
string(REPLACE ";" "$<SEMICOLON>" hygiene_cmd "${hygiene_cmd}")
schway_add_test(NAME policy.action_pins.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${hygiene_cmd}" -DCODE=1
    "-DREGEX=action_pins: sample.yml: uses: actions/checkout@v4.*action_pins: 1 uses: lines not pinned"
    -P ${hygiene_scripts}/expect_fail.cmake
  LABELS hygiene policy TIMEOUT 30)
