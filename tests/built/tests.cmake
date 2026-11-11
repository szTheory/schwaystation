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
