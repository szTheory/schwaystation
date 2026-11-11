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
