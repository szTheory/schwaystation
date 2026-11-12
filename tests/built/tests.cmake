# Run selected runner BIOS assertions as a script so tests can inspect both
# captured output and the exact runner exit status without helper scripts.
set(SCHWAY_OPENBIOS_SHELL_REGION "ntsc-u")
set(SCHWAY_OPENBIOS_SHELL_FRAME 240)
set(SCHWAY_OPENBIOS_SHELL_SHA256 "4d1f0ff260e2cf065ae0b7add4ba60353b2b01241dda8325309b5398ba00acec")
if(CMAKE_SCRIPT_MODE_FILE)
  if(CASE STREQUAL "openbios_shell")
    foreach(var RUN BIOS EXPECTED_FILE REGION SHELL_FRAME SHELL_SHA256 TUPLE_FILE)
      if(NOT DEFINED ${var} OR "${${var}}" STREQUAL "")
        message(FATAL_ERROR "OpenBIOS shell test requires ${var}")
      endif()
    endforeach()
    file(REMOVE "${TUPLE_FILE}")
    if(NOT EXISTS "${BIOS}" OR IS_DIRECTORY "${BIOS}")
      message(FATAL_ERROR "OpenBIOS shell test requires the checked handoff")
    endif()
    file(SIZE "${BIOS}" bios_size)
    file(SHA256 "${BIOS}" bios_digest)
    file(READ "${EXPECTED_FILE}" expected_bios_digest)
    string(STRIP "${expected_bios_digest}" expected_bios_digest)
    if(NOT bios_size EQUAL 524288 OR NOT bios_digest STREQUAL expected_bios_digest)
      message(FATAL_ERROR "OpenBIOS shell test refused unchecked input (${bios_size} bytes, ${bios_digest})")
    endif()
    execute_process(COMMAND ${EMULATOR} "${RUN}" --region "${REGION}" --bios "${BIOS}"
      --frames "${SHELL_FRAME}" --tty --hash
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    set(output "${out}${err}")
    if(NOT rc STREQUAL "0" OR
       NOT output MATCHES "bios flags=loaded status=unknown region=unknown sha256=${expected_bios_digest}" OR
       NOT output MATCHES "frame ${SHELL_FRAME} sha256 ${SHELL_SHA256}")
      message(FATAL_ERROR "OpenBIOS shell frame did not match its checked native tuple: ${rc}: ${output}")
    endif()
    file(WRITE "${TUPLE_FILE}" "openbios shell ${REGION} frame ${SHELL_FRAME} sha256 ${SHELL_SHA256}\n")
    message("${output}")
    return()
  elseif(CASE STREQUAL "openbios_shell_selector")
    foreach(var RUN BIOS EXPECTED_FILE REGION)
      if(NOT DEFINED ${var} OR "${${var}}" STREQUAL "")
        message(FATAL_ERROR "OpenBIOS selector test requires ${var}")
      endif()
    endforeach()
    if(NOT EXISTS "${BIOS}" OR IS_DIRECTORY "${BIOS}")
      message(FATAL_ERROR "OpenBIOS selector test requires the checked handoff")
    endif()
    file(SIZE "${BIOS}" bios_size)
    file(SHA256 "${BIOS}" bios_digest)
    file(READ "${EXPECTED_FILE}" expected_bios_digest)
    string(STRIP "${expected_bios_digest}" expected_bios_digest)
    if(NOT bios_size EQUAL 524288 OR NOT bios_digest STREQUAL expected_bios_digest)
      message(FATAL_ERROR "OpenBIOS selector test refused unchecked input (${bios_size} bytes, ${bios_digest})")
    endif()
    execute_process(COMMAND ${EMULATOR} "${RUN}" --region "${REGION}" --bios "${BIOS}" --frames 1 --hash
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    set(output "${out}${err}")
    if(NOT rc STREQUAL "0" OR
       NOT output MATCHES "bios flags=loaded status=unknown region=unknown sha256=${expected_bios_digest}" OR
       NOT output MATCHES "frame 1 sha256 [0-9a-f]+")
      message(FATAL_ERROR "OpenBIOS region selector ${REGION} did not run with the checked image: ${rc}: ${output}")
    endif()
    return()
  elseif(CASE STREQUAL "openbios_shell_not_applicable")
    if(NOT DEFINED TUPLE_FILE OR "${TUPLE_FILE}" STREQUAL "")
      message(FATAL_ERROR "OpenBIOS shell applicability result requires TUPLE_FILE")
    endif()
    file(REMOVE "${TUPLE_FILE}")
    if("$ENV{CI}" STREQUAL "true")
      message(FATAL_ERROR "OpenBIOS shell is required in CI, but the checked handoff is missing")
    endif()
    file(WRITE "${TUPLE_FILE}" "openbios shell ${REGION} frame ${SHELL_FRAME} not-applicable no-checked-handoff\n")
    message("OpenBIOS shell: not-applicable because there is no local checked handoff")
    return()
  elseif(CASE STREQUAL "bios_runner")
    execute_process(COMMAND ${EMULATOR} "${RUN}" --bios "${BIOS}" --frames 1 --tty
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    set(output "${out}${err}")
    if(NOT "${rc}" STREQUAL "6")
      message(FATAL_ERROR "BIOS reset case expected exit 6, got ${rc}: ${output}")
    endif()
    if(NOT output MATCHES "bios flags=loaded status=unknown region=unknown sha256=${BIOS_SHA256}")
      message(FATAL_ERROR "BIOS reset case did not report the copied image identity: ${output}")
    endif()
    if(NOT output MATCHES "exit code=0x42")
      message(FATAL_ERROR "BIOS reset code did not run: ${output}")
    endif()
    return()
  elseif(CASE STREQUAL "bios_runner_edges")
    function(expect_bios_run path digest exit_code)
      execute_process(COMMAND ${EMULATOR} "${RUN}" --bios "${path}" --frames 1 --tty
        RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
      set(output "${out}${err}")
      if(NOT "${rc}" STREQUAL "6" OR
         NOT output MATCHES "bios flags=loaded status=unknown region=unknown sha256=${digest}" OR
         NOT output MATCHES "exit code=0x${exit_code}")
        message(FATAL_ERROR "BIOS run for ${path}: expected unknown ${digest} and exit 0x${exit_code}, got ${rc}: ${output}")
      endif()
    endfunction()
    function(expect_bios_refusal path reason)
      execute_process(COMMAND ${EMULATOR} "${RUN}" --bios "${path}" --frames 1
        RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
      set(output "${out}${err}")
      if(NOT "${rc}" STREQUAL "1" OR NOT output MATCHES "${reason}")
        message(FATAL_ERROR "BIOS refusal for ${path}: expected status 1 and ${reason}, got ${rc}: ${output}")
      endif()
    endfunction()

    if(NOT "${BIOS_SHA256}" STREQUAL "${RENAMED_SHA256}")
      message(FATAL_ERROR "renaming identical BIOS bytes changed the independently computed SHA-256")
    endif()
    expect_bios_run("${RENAMED_BIOS}" "${BIOS_SHA256}" 42)
    expect_bios_run("${UNKNOWN_BIOS}" "${UNKNOWN_SHA256}" 43)
    expect_bios_refusal("${EMPTY_BIOS}" "BIOS must be exactly 524288 bytes")
    expect_bios_refusal("${SHORT_BIOS}" "BIOS must be exactly 524288 bytes")
    expect_bios_refusal("${LONG_BIOS}" "BIOS must be exactly 524288 bytes")
    expect_bios_refusal("${MISSING_BIOS}" "cannot open")

    execute_process(COMMAND ${EMULATOR} "${RUN}" --bios "${BIOS}" --exe "${MISSING_EXE}" --frames 1
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    set(output "${out}${err}")
    if(NOT "${rc}" STREQUAL "2" OR NOT output MATCHES "--bios and --exe cannot be used together")
      message(FATAL_ERROR "conflicting content selectors were not refused before loading: ${rc}: ${output}")
    endif()
    execute_process(COMMAND ${EMULATOR} "${RUN}" --bios "${BIOS}" --bios "${RENAMED_BIOS}" --frames 1
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    set(output "${out}${err}")
    if(NOT "${rc}" STREQUAL "2" OR NOT output MATCHES "give --bios once")
      message(FATAL_ERROR "duplicate --bios was not refused: ${rc}: ${output}")
    endif()
    execute_process(COMMAND ${EMULATOR} "${RUN}" --bios --frames 1
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    set(output "${out}${err}")
    if(NOT "${rc}" STREQUAL "2" OR NOT output MATCHES "--bios needs a file")
      message(FATAL_ERROR "missing --bios path was not refused: ${rc}: ${output}")
    endif()
    return()
  endif()
  message(FATAL_ERROR "unknown built BIOS script case: ${CASE}")
endif()

# This synthetic ROM contains only self-authored MIPS reset instructions. Its
# exit-port write proves that the actual CLI copied and executed the supplied
# image; no firmware bytes are stored in the source tree.
set(bios_runner_fixture_dir "${PROJECT_BINARY_DIR}/tests/built/bios-runner")
file(MAKE_DIRECTORY "${bios_runner_fixture_dir}")
string(ASCII 128 31 8 60 113 15 8 53 66 66 9 60 66 66 41 53 17 17 9 161
  bios_runner_reset_code)
string(REPEAT "S" 524268 bios_runner_tail)
set(bios_runner_fixture "${bios_runner_fixture_dir}/scph5501.bin")
file(WRITE "${bios_runner_fixture}" "${bios_runner_reset_code}${bios_runner_tail}")
file(SHA256 "${bios_runner_fixture}" bios_runner_sha256)
file(SIZE "${bios_runner_fixture}" bios_runner_size)
if(NOT bios_runner_size EQUAL 524288)
  message(FATAL_ERROR "synthetic BIOS fixture is ${bios_runner_size} bytes, expected 524288")
endif()

set(bios_runner_renamed_fixture "${bios_runner_fixture_dir}/other-name.bin")
file(WRITE "${bios_runner_renamed_fixture}" "${bios_runner_reset_code}${bios_runner_tail}")
file(SHA256 "${bios_runner_renamed_fixture}" bios_runner_renamed_sha256)

string(ASCII 128 31 8 60 113 15 8 53 67 67 9 60 67 67 41 53 17 17 9 161
  bios_runner_unknown_reset_code)
string(REPEAT "U" 524268 bios_runner_unknown_tail)
set(bios_runner_unknown_fixture "${bios_runner_fixture_dir}/unknown-image.bin")
file(WRITE "${bios_runner_unknown_fixture}" "${bios_runner_unknown_reset_code}${bios_runner_unknown_tail}")
file(SHA256 "${bios_runner_unknown_fixture}" bios_runner_unknown_sha256)

set(bios_runner_empty_fixture "${bios_runner_fixture_dir}/empty.bin")
file(WRITE "${bios_runner_empty_fixture}" "")
string(REPEAT "S" 524267 bios_runner_short_tail)
set(bios_runner_short_fixture "${bios_runner_fixture_dir}/short.bin")
file(WRITE "${bios_runner_short_fixture}" "${bios_runner_reset_code}${bios_runner_short_tail}")
string(REPEAT "S" 524269 bios_runner_long_tail)
set(bios_runner_long_fixture "${bios_runner_fixture_dir}/long.bin")
file(WRITE "${bios_runner_long_fixture}" "${bios_runner_reset_code}${bios_runner_long_tail}")
file(SIZE "${bios_runner_empty_fixture}" bios_runner_empty_size)
file(SIZE "${bios_runner_short_fixture}" bios_runner_short_size)
file(SIZE "${bios_runner_long_fixture}" bios_runner_long_size)
if(NOT bios_runner_empty_size EQUAL 0 OR NOT bios_runner_short_size EQUAL 524287 OR
   NOT bios_runner_long_size EQUAL 524289)
  message(FATAL_ERROR "synthetic BIOS boundary fixtures have incorrect sizes")
endif()
set(bios_runner_missing_fixture "${bios_runner_fixture_dir}/missing.bin")
set(bios_runner_missing_exe "${bios_runner_fixture_dir}/missing.exe")
string(REPLACE ";" "$<SEMICOLON>" bios_runner_emulator "${CMAKE_CROSSCOMPILING_EMULATOR}")

set(bios_runner_cmd ${CMAKE_COMMAND}
  "-DRUN=$<TARGET_FILE:schwaystation-run>"
  "-DEMULATOR=${bios_runner_emulator}"
  "-DBIOS=${bios_runner_fixture}"
  "-DBIOS_SHA256=${bios_runner_sha256}"
  -DCASE=bios_runner -P ${CMAKE_CURRENT_LIST_FILE})
schway_add_test(NAME built.bios.runner
  COMMAND ${bios_runner_cmd}
  LABELS bios portable runner TIMEOUT 30)

set(bios_runner_edges_cmd ${CMAKE_COMMAND}
  "-DRUN=$<TARGET_FILE:schwaystation-run>"
  "-DEMULATOR=${bios_runner_emulator}"
  "-DBIOS=${bios_runner_fixture}"
  "-DBIOS_SHA256=${bios_runner_sha256}"
  "-DRENAMED_BIOS=${bios_runner_renamed_fixture}"
  "-DRENAMED_SHA256=${bios_runner_renamed_sha256}"
  "-DUNKNOWN_BIOS=${bios_runner_unknown_fixture}"
  "-DUNKNOWN_SHA256=${bios_runner_unknown_sha256}"
  "-DEMPTY_BIOS=${bios_runner_empty_fixture}"
  "-DSHORT_BIOS=${bios_runner_short_fixture}"
  "-DLONG_BIOS=${bios_runner_long_fixture}"
  "-DMISSING_BIOS=${bios_runner_missing_fixture}"
  "-DMISSING_EXE=${bios_runner_missing_exe}"
  -DCASE=bios_runner_edges -P ${CMAKE_CURRENT_LIST_FILE})
schway_add_test(NAME built.bios.runner_edges
  COMMAND ${bios_runner_edges_cmd}
  LABELS bios portable runner TIMEOUT 30)

# Fault prevented: a BIOS load can borrow caller memory or report an unrelated
# digest instead of executing a copied exact-size ROM from the reset vector.
add_executable(schwaystation-test-bios ${CMAKE_CURRENT_LIST_DIR}/bios_tests.c)
target_include_directories(schwaystation-test-bios PRIVATE ${PROJECT_SOURCE_DIR}/tests)
target_link_libraries(schwaystation-test-bios PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-bios PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-bios)

# Fault prevented: synthetic recognition keys leaking into the production
# BIOS table instead of exercising the lookup in a separate test binary.
add_library(schwaystation_bios_synthetic OBJECT ${PROJECT_SOURCE_DIR}/src/bios.c)
schway_core_options(schwaystation_bios_synthetic)
target_include_directories(schwaystation_bios_synthetic PRIVATE ${PROJECT_SOURCE_DIR}/runner)
target_compile_definitions(schwaystation_bios_synthetic PRIVATE SCHWAY_BIOS_TEST_SYNTHETIC=1)
add_executable(schwaystation-test-bios-synthetic
  ${CMAKE_CURRENT_LIST_DIR}/bios_tests.c
  $<TARGET_OBJECTS:schwaystation_bios_synthetic>)
target_include_directories(schwaystation-test-bios-synthetic PRIVATE ${PROJECT_SOURCE_DIR}/tests)
target_link_libraries(schwaystation-test-bios-synthetic PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-bios-synthetic PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-bios-synthetic)

schway_add_test(NAME built.bios.synthetic_rom
  COMMAND schwaystation-test-bios synthetic_rom
  LABELS bios portable TIMEOUT 10)

schway_add_test(NAME built.bios.copy_edges
  COMMAND schwaystation-test-bios copy_edges
  LABELS bios portable TIMEOUT 10)

schway_add_test(NAME built.bios.identity
  COMMAND schwaystation-test-bios-synthetic identity
  LABELS bios portable TIMEOUT 10)

schway_add_test(NAME built.bios.identity_edges
  COMMAND schwaystation-test-bios identity_edges
  LABELS bios portable TIMEOUT 10)

schway_add_test(NAME built.bios.test_key_isolation
  COMMAND schwaystation-test-bios test_key_isolation
  LABELS bios portable TIMEOUT 10)

# Fault prevented: a same-sized OpenBIOS output with changed bytes being accepted.
# Generate both inputs in the build tree so no firmware or source bytes are retained.
set(openbios_fixture_dir "${PROJECT_BINARY_DIR}/tests/built/openbios")
file(MAKE_DIRECTORY "${openbios_fixture_dir}")
string(REPEAT "S" 524288 openbios_fixture)
set(openbios_fixture_path "${openbios_fixture_dir}/openbios.bin")
set(openbios_expected_path "${openbios_fixture_dir}/expected.txt")
set(openbios_corrupt_path "${openbios_fixture_dir}/openbios-corrupt.bin")
file(WRITE "${openbios_fixture_path}" "${openbios_fixture}")
file(SHA256 "${openbios_fixture_path}" openbios_fixture_sha256)
file(WRITE "${openbios_expected_path}" "${openbios_fixture_sha256}\n")
string(SUBSTRING "${openbios_fixture}" 0 1 openbios_first)
string(SUBSTRING "${openbios_fixture}" 1 -1 openbios_tail)
if(openbios_first STREQUAL "S")
  set(openbios_bad_first "T")
else()
  set(openbios_bad_first "S")
endif()
file(WRITE "${openbios_corrupt_path}" "${openbios_bad_first}${openbios_tail}")

schway_add_test(NAME built.openbios.digest
  COMMAND ${CMAKE_COMMAND} "-DOPENBIOS_INPUT=${openbios_fixture_path}"
    "-DEXPECTED_FILE=${openbios_expected_path}" -P ${PROJECT_SOURCE_DIR}/cmake/OpenBIOS.cmake
  LABELS bios portable TIMEOUT 10)

set(openbios_corrupt_cmd ${CMAKE_COMMAND} "-DOPENBIOS_INPUT=${openbios_corrupt_path}"
  "-DEXPECTED_FILE=${openbios_expected_path}" -P ${PROJECT_SOURCE_DIR}/cmake/OpenBIOS.cmake)
string(REPLACE ";" "$<SEMICOLON>" openbios_corrupt_cmd "${openbios_corrupt_cmd}")
schway_add_test(NAME built.openbios.digest_corrupt
  COMMAND ${CMAKE_COMMAND} "-DCMD=${openbios_corrupt_cmd}" -DCODE=1
    "-DREGEX=OpenBIOS SHA256 mismatch" -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
  LABELS bios portable TIMEOUT 10)

# CI must use the artifact downloaded from this run; local builds use the
# existing generated fixture when no hosted handoff is available.
set(openbios_handoff_input "${PROJECT_BINARY_DIR}/openbios/openbios.bin")
set(openbios_handoff_expected "${PROJECT_SOURCE_DIR}/tests/built/expected.txt")
if(NOT EXISTS "${openbios_handoff_input}")
  # Non-consumer workflows may configure ci for a build-only target; the
  # actual consumer jobs verify the downloaded artifact before configuration.
  set(openbios_handoff_input "${openbios_fixture_path}")
  set(openbios_handoff_expected "${openbios_expected_path}")
endif()

set(openbios_shell_input "${PROJECT_BINARY_DIR}/openbios/openbios.bin")
set(openbios_shell_tuple_file "${PROJECT_BINARY_DIR}/openbios-shell-tuple.txt")
file(REMOVE "${openbios_shell_tuple_file}")
# Fault prevented: a checked BIOS that no longer reaches its inspected shell frame.
if(EXISTS "${openbios_shell_input}")
  string(REPLACE ";" "$<SEMICOLON>" openbios_shell_emulator "${CMAKE_CROSSCOMPILING_EMULATOR}")
  set(openbios_shell_cmd ${CMAKE_COMMAND}
    "-DRUN=$<TARGET_FILE:schwaystation-run>"
    "-DEMULATOR=${openbios_shell_emulator}"
    "-DBIOS=${openbios_shell_input}"
    "-DEXPECTED_FILE=${openbios_handoff_expected}"
    "-DREGION=${SCHWAY_OPENBIOS_SHELL_REGION}"
    "-DSHELL_FRAME=${SCHWAY_OPENBIOS_SHELL_FRAME}"
    "-DSHELL_SHA256=${SCHWAY_OPENBIOS_SHELL_SHA256}"
    "-DTUPLE_FILE=${openbios_shell_tuple_file}"
    -DCASE=openbios_shell -P ${CMAKE_CURRENT_LIST_FILE})
  schway_add_test(NAME built.openbios.shell
    COMMAND ${openbios_shell_cmd}
    LABELS bios portable runner TIMEOUT 30)
else()
  schway_add_test(NAME built.openbios.shell
    COMMAND ${CMAKE_COMMAND}
      "-DTUPLE_FILE=${openbios_shell_tuple_file}"
      "-DREGION=${SCHWAY_OPENBIOS_SHELL_REGION}"
      "-DSHELL_FRAME=${SCHWAY_OPENBIOS_SHELL_FRAME}"
      -DCASE=openbios_shell_not_applicable -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS bios portable runner TIMEOUT 10
    PASS_REGEX "not-applicable because there is no local checked handoff")
endif()

set_tests_properties(built.openbios.shell PROPERTIES FIXTURES_SETUP openbios_shell)
cmake_language(DEFER CALL set_tests_properties runner.hashes_file PROPERTIES
  DEPENDS built.openbios.shell FIXTURES_REQUIRED openbios_shell)

# Fault prevented: a consumer using a missing or altered artifact from another
# run instead of this run's digest-checked OpenBIOS handoff.
schway_add_test(NAME built.openbios.handoff
  COMMAND ${CMAKE_COMMAND} "-DOPENBIOS_INPUT=${openbios_handoff_input}"
    "-DEXPECTED_FILE=${openbios_handoff_expected}"
    -P ${PROJECT_SOURCE_DIR}/cmake/OpenBIOS.cmake
  LABELS bios portable TIMEOUT 10)

# Fault prevented: corrupted or missing handoff data being treated as firmware.
set(openbios_handoff_control_dir "${PROJECT_BINARY_DIR}/tests/built/openbios/handoff-controls")
file(MAKE_DIRECTORY "${openbios_handoff_control_dir}")
set(openbios_handoff_corrupt_input "${openbios_handoff_control_dir}/openbios-corrupt.bin")
file(COPY_FILE "${openbios_handoff_input}" "${openbios_handoff_corrupt_input}")
file(APPEND "${openbios_handoff_corrupt_input}" "x")
set(openbios_handoff_corrupt_cmd ${CMAKE_COMMAND}
  "-DOPENBIOS_INPUT=${openbios_handoff_corrupt_input}"
  "-DEXPECTED_FILE=${openbios_handoff_expected}" -P ${PROJECT_SOURCE_DIR}/cmake/OpenBIOS.cmake)
string(REPLACE ";" "$<SEMICOLON>" openbios_handoff_corrupt_cmd "${openbios_handoff_corrupt_cmd}")
schway_add_test(NAME built.openbios.handoff_corruption
  COMMAND ${CMAKE_COMMAND} "-DCMD=${openbios_handoff_corrupt_cmd}" -DCODE=1
    "-DREGEX=OpenBIOS: expected 524288 bytes" -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
  LABELS bios portable TIMEOUT 10)

set(openbios_handoff_missing_input "${openbios_handoff_control_dir}/openbios-missing.bin")
file(REMOVE "${openbios_handoff_missing_input}")
set(openbios_handoff_missing_cmd ${CMAKE_COMMAND}
  "-DOPENBIOS_INPUT=${openbios_handoff_missing_input}"
  "-DEXPECTED_FILE=${openbios_handoff_expected}" -P ${PROJECT_SOURCE_DIR}/cmake/OpenBIOS.cmake)
string(REPLACE ";" "$<SEMICOLON>" openbios_handoff_missing_cmd "${openbios_handoff_missing_cmd}")
schway_add_test(NAME built.openbios.handoff_missing
  COMMAND ${CMAKE_COMMAND} "-DCMD=${openbios_handoff_missing_cmd}" -DCODE=1
    "-DREGEX=OpenBIOS: input is missing or not a file"
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
  LABELS bios portable TIMEOUT 10)
