# Run selected runner BIOS assertions as a script so tests can inspect both
# captured output and the exact runner exit status without helper scripts.
if(CMAKE_SCRIPT_MODE_FILE)
  if(CASE STREQUAL "bios_runner")
    execute_process(COMMAND "${RUN}" --bios "${BIOS}" --frames 1 --tty
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

set(bios_runner_cmd ${CMAKE_COMMAND}
  "-DRUN=$<TARGET_FILE:schwaystation-run>"
  "-DBIOS=${bios_runner_fixture}"
  "-DBIOS_SHA256=${bios_runner_sha256}"
  -DCASE=bios_runner -P ${CMAKE_CURRENT_LIST_FILE})
schway_add_test(NAME built.bios.runner
  COMMAND ${bios_runner_cmd}
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
