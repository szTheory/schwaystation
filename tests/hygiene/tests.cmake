# Hygiene tests, registered only by the hygiene preset (SCHWAYSTATION_HYGIENE).
set(hygiene_scripts ${CMAKE_CURRENT_LIST_DIR}/../cmake)

# Fault prevented: repository-check code growing past its budget. Every line
# of the shell scripts, the CMake check scripts and the hooks counts, at most
# 1500; the self test counts a 10-line fixture against a limit of 9.
schway_add_test(NAME policy.gate_budget
  COMMAND ${CMAKE_COMMAND} -DROOT=${CMAKE_SOURCE_DIR}
    "-DPATTERNS=scripts/*.sh$<SEMICOLON>tests/cmake/*.cmake$<SEMICOLON>.githooks/*" -DLIMIT=1500
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
# under a neutral name. The self test's throwaway repository holds a
# 524288-byte file of NUL bytes and a file whose first 8 bytes are decoded
# here from the executable signature's hex in scripts/hygiene.sh, so no
# tracked file holds either.
schway_add_test(NAME hygiene.no_bios_bytes
  COMMAND ${SCHWAYSTATION_SH} ${CMAKE_SOURCE_DIR}/scripts/hygiene.sh --tree
  LABELS hygiene TIMEOUT 60)
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
# that cannot be trusted.
set(hygiene_classify [=[
script=$1
n=0
check() {
  got=$(printf '%b' "$3" | sh "$script" "${4-2}" | tr '\n' ' ')
  if [ "$got" != "heavy=$1 release_inputs=$2 " ]
  then
    echo "ci.classify: want heavy=$1 release_inputs=$2, got $got for: $3"
    exit 1
  fi
  n=$((n + 1))
}
check true false '.planning/STATE.md\nsrc/core.c\n'
check true false 'src/core.c\n.planning/core.c\n'
check true true ''
check true true '\n\n'
check true false '(listing failed)\n'
check true false '.planning-notes/a.md\n'
check true false 'docs/.planning/a.md\n'
check false false '.planning/STATE.md\n.planning/phases/01/a.md\n'
check false false '.planning/STATE.md'
check false false '.planning/STATE.md\n' 3000
check true true '.planning/STATE.md\n' 3001
check true true '.planning/STATE.md\n' ''
check true true '.planning/STATE.md\n' 'x'
check true true '.github/workflows/release.yml\n'
check true true '.github/workflows/ci.yml\n'
check true true 'src/core.c\nscripts/publish.sh\n'
check true true 'release-please-config.json\n'
check true true 'cmake/packaging.cmake\n'
check true true 'scripts/publish.sh\nscripts/release.sh\n'
check true false 'scripts/publish.sh.orig\ndocs/release.yml\n'
check false false '.planning/STATE.md\n' 1
echo "ci.classify: $n lists"
]=])
schway_add_test(NAME ci.classify
  COMMAND ${SCHWAYSTATION_SH} -c "${hygiene_classify}" sh ${CMAKE_SOURCE_DIR}/scripts/classify-changes.sh
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
