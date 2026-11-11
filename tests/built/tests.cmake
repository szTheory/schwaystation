# Fault prevented: a BIOS load can borrow caller memory or report an unrelated
# digest instead of executing a copied exact-size ROM from the reset vector.
add_executable(schwaystation-test-bios ${CMAKE_CURRENT_LIST_DIR}/bios_tests.c)
target_include_directories(schwaystation-test-bios PRIVATE ${PROJECT_SOURCE_DIR}/tests)
target_link_libraries(schwaystation-test-bios PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-bios PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-bios)

schway_add_test(NAME built.bios.synthetic_rom
  COMMAND schwaystation-test-bios synthetic_rom
  LABELS bios portable TIMEOUT 10)

schway_add_test(NAME built.bios.copy_edges
  COMMAND schwaystation-test-bios copy_edges
  LABELS bios portable TIMEOUT 10)
