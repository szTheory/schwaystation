cmake_minimum_required(VERSION 3.25)
if(CMAKE_HOST_WIN32)
  find_package(Git QUIET)
  if(Git_FOUND)
    get_filename_component(git_bin "${GIT_EXECUTABLE}" DIRECTORY)
    set(git_usr_bin "${git_bin}/../usr/bin" "${git_bin}/../../usr/bin")
  endif()
endif()
find_program(SH sh PATHS ${git_usr_bin} REQUIRED)
find_program(JQ jq PATHS ${git_usr_bin} REQUIRED)
set(report "${ROOT}/scripts/triage-report.sh")
set(fixture_dir "${BIN}/fixtures")
set(stub_dir "${BIN}/stub")
set(tmp_dir "${BIN}/tmp")
set(output "${BIN}/triage.json")
file(REMOVE_RECURSE "${BIN}")
file(MAKE_DIRECTORY "${fixture_dir}" "${stub_dir}" "${tmp_dir}")

set(date 2026-11-18T00:00:00Z)
set(private_posix "/Use")
string(APPEND private_posix "rs/HOMECANARY/file")
set(private_windows [=[C:\\Use]=])
string(APPEND private_windows [=[rs\\DRIVECANARY\\file]=])
string(REPEAT a1 20 private_hash)
set(private_mail "MAILCANARY@")
string(APPEND private_mail "hidden.invalid")
set(issue [=[{"number":1,"title":"Question \"quoted\"; $(data) \u001b]0;OSCCANARY\u0007\u001b[31m\u001b[0m\u202e\u0085",
  "labels":[{"name":"bug"},{"name":"@private_posix@"},{"name":"@private_windows@ SpaceDRIVECANARY"},
  {"name":"@private_mail@"},{"name":"@private_hash@"},{"name":"\\\\UNCCANARY\\share"},
  {"name":"/tmp/TEMPCANARY"},{"name":"https://hidden.invalid/URLCANARY"}],
  "created_at":"@date@","updated_at":"@date@","body":"BODYCANARY","user":{"login":"ACTORCANARY"}}]=])
string(CONFIGURE "${issue}" issue @ONLY)
file(WRITE "${fixture_dir}/normal-issues.json" "[${issue},{\"number\":2,\"pull_request\":{},\"title\":\"PRCANARY\"}]")
file(WRITE "${fixture_dir}/empty-issues.json" "[]")
file(WRITE "${fixture_dir}/malformed-issues.json" "[{]")
file(WRITE "${fixture_dir}/ci.json"
  "{\"workflow_runs\":[{\"id\":3,\"name\":\"RUNNAMECANARY\",\"head_sha\":\"SHA_CANARY\",\"status\":\"completed\",\"conclusion\":\"success\",\"updated_at\":\"${date}\"},{\"id\":4,\"status\":\"queued\",\"conclusion\":null,\"updated_at\":\"${date}\"}]}")
file(WRITE "${fixture_dir}/release.json" "{\"workflow_runs\":[]}")
file(WRITE "${fixture_dir}/nightly.json" "{\"workflow_runs\":[]}")
file(WRITE "${fixture_dir}/malformed-ci.json" "{\"workflow_runs\":[{\"id\":3,\"status\":\"unexpected\",\"conclusion\":null,\"updated_at\":\"${date}\"}]}")

string(REPEAT z 200 long)
set(row "{\"number\":4,\"title\":\"${long}\",\"labels\":[{\"name\":\"${long}\"}],\"created_at\":\"${date}\",\"updated_at\":\"${date}\"}")
string(REPEAT "${row}," 99 rows)
file(WRITE "${fixture_dir}/cap-issues.json" "[${rows}${row}]")

file(WRITE "${stub_dir}/gh" [=[#!/bin/sh
set -eu
if [ "$#" -ne 2 ] || [ "$1" != api ]; then
  printf '%s\n' 'unexpected gh invocation' >&2
  exit 90
fi
endpoint=$2
printf '%s\n' "$endpoint" >> "$TRIAGE_REQUEST_LOG"
if [ "$endpoint" = "${TRIAGE_FAIL_ENDPOINT:-}" ]; then
  printf '%s\n' 'RAWFAILCANARY' >&2
  exit 1
fi
case "$endpoint" in
  'repos/owner/repo/issues?state=open&per_page=100&page=1')
    if [ "$TRIAGE_SCENARIO" = no-runs ]; then
      cat "$TRIAGE_FIXTURE_DIR/empty-issues.json"
    else
      cat "$TRIAGE_FIXTURE_DIR/$TRIAGE_SCENARIO-issues.json"
    fi ;;
  'repos/owner/repo/actions/workflows/ci.yml/runs?per_page=1')
    if [ "$TRIAGE_SCENARIO" = malformed-run ]; then
      cat "$TRIAGE_FIXTURE_DIR/malformed-ci.json"
    elif [ "$TRIAGE_SCENARIO" = no-runs ]; then
      cat "$TRIAGE_FIXTURE_DIR/release.json"
    else
      cat "$TRIAGE_FIXTURE_DIR/ci.json"
    fi ;;
  'repos/owner/repo/actions/workflows/release.yml/runs?per_page=1')
    cat "$TRIAGE_FIXTURE_DIR/release.json" ;;
  'repos/owner/repo/actions/workflows/nightly.yml/runs?per_page=1')
    cat "$TRIAGE_FIXTURE_DIR/nightly.json" ;;
  *)
    printf '%s\n' 'unexpected endpoint' >&2
    exit 91 ;;
esac
]=])
file(CHMOD "${stub_dir}/gh" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

set(schema [=[keys == ["issues","issues_truncated","schema_version","workflows"] and
  .schema_version == 1 and (.issues_truncated | type == "boolean") and
  (.workflows | map(.workflow) == ["ci","release","nightly"] and
  all(.[]; keys == ["conclusion","run_id","status","updated_at","workflow"]))]=])
set(normal_check [=[.issues_truncated == false and (.issues | length == 1 and
  (.[0] | keys == ["created_at","id","labels","title","updated_at"] and .id == 1 and
  .labels[0] == "bug" and (.labels | length == 8) and
  (.title | startswith("Question \"quoted\"; $(data)") and (test("[\\p{Cc}\\p{Cf}]") | not)))) and
  .workflows[0].run_id == 3 and .workflows[0].status == "completed" and
  .workflows[0].conclusion == "success" and
  all(.workflows[1:][]; .run_id == null and .status == null and .conclusion == null and .updated_at == null)]=])

function(assert_no_temp_files)
  file(GLOB scratch_files "${tmp_dir}/schwaystation-triage.*")
  if(scratch_files)
    message(FATAL_ERROR "ci.triage: collector left private API response files behind")
  endif()
endfunction()

macro(invoke scenario fail_endpoint)
  execute_process(COMMAND "${CMAKE_COMMAND}" -E env
      "PATH=${stub_dir}:$ENV{PATH}" "GH_TOKEN=fixture-token" "GITHUB_REPOSITORY=owner/repo"
      "TRIAGE_FIXTURE_DIR=${fixture_dir}" "TRIAGE_REQUEST_LOG=${BIN}/requests.txt"
      "TRIAGE_SCENARIO=${scenario}" "TRIAGE_FAIL_ENDPOINT=${fail_endpoint}" "TMPDIR=${tmp_dir}"
      "${SH}" "${report}" collect "${output}"
    WORKING_DIRECTORY "${BIN}" RESULT_VARIABLE rc OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr)
endmacro()

function(collect scenario predicate)
  file(REMOVE "${output}" "${BIN}/requests.txt")
  invoke("${scenario}" "")
  if(NOT rc STREQUAL "0" OR NOT EXISTS "${output}")
    message(FATAL_ERROR "ci.triage: valid API metadata did not produce the sanitized artifact")
  endif()
  if("${stdout}${stderr}" MATCHES "CANARY|RAWFAIL|fixture-token")
    message(FATAL_ERROR "ci.triage: API response data or credentials reached command output")
  endif()
  execute_process(COMMAND "${JQ}" -e "${schema} and (${predicate})" "${output}"
    RESULT_VARIABLE rc OUTPUT_QUIET ERROR_QUIET)
  if(NOT rc STREQUAL "0")
    message(FATAL_ERROR "ci.triage: artifact failed its schema, bounds or privacy assertion")
  endif()
  file(READ "${output}" artifact)
  if("${artifact}" MATCHES "CANARY|${private_hash}|fixture-token|RUNNAME|SHA_CANARY|BODYCANARY|ACTORCANARY")
    message(FATAL_ERROR "ci.triage: artifact contains untrusted or excluded API fields")
  endif()
  file(READ "${BIN}/requests.txt" requests)
  set(expected_requests [=[repos/owner/repo/issues?state=open&per_page=100&page=1
repos/owner/repo/actions/workflows/ci.yml/runs?per_page=1
repos/owner/repo/actions/workflows/release.yml/runs?per_page=1
repos/owner/repo/actions/workflows/nightly.yml/runs?per_page=1
]=])
  if(NOT requests STREQUAL expected_requests)
    message(FATAL_ERROR "ci.triage: collector used an unexpected API endpoint or request count")
  endif()
  assert_no_temp_files()
  set(artifact_text "${artifact}" PARENT_SCOPE)
endfunction()

function(reject scenario fail_endpoint)
  file(REMOVE "${output}" "${BIN}/requests.txt")
  invoke("${scenario}" "${fail_endpoint}")
  if(rc STREQUAL "0" OR EXISTS "${output}" OR "${stdout}${stderr}" MATCHES "CANARY|RAWFAIL|fixture-token")
    message(FATAL_ERROR "ci.triage: invalid API response published data or leaked response text")
  endif()
  if(NOT stderr MATCHES "triage-report: invalid GitHub metadata|triage-report: GitHub API request failed")
    message(FATAL_ERROR "ci.triage: API failure did not produce the fixed diagnostic")
  endif()
  assert_no_temp_files()
endfunction()

collect(normal "${normal_check}")
set(first "${artifact_text}")
collect(normal "${normal_check}")
if(NOT artifact_text STREQUAL first)
  message(FATAL_ERROR "ci.triage: identical inputs did not produce a stable artifact")
endif()
collect(empty ".issues == [] and .issues_truncated == false and .workflows[0].run_id == 3 and all(.workflows[1:][]; .run_id == null and .status == null and .conclusion == null and .updated_at == null)")
collect(no-runs ".issues == [] and all(.workflows[]; .run_id == null and .status == null and .conclusion == null and .updated_at == null)")
collect(cap ".issues_truncated == true and (.issues | length == 100 and all(.[]; (.title | length == 160) and ((.labels | length) == 1) and ((.labels[0] | length) == 48)))")
reject(malformed "")
reject(malformed-run "")
reject(normal "repos/owner/repo/actions/workflows/nightly.yml/runs?per_page=1")
