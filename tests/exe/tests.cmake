# The PS-EXE loader and its probe.
# The validator, the probe and the loader through the public header, plus the
# instance's RAM and state. One executable runs the case named on its command
# line. White-box: the test sees src/, so it can read guest RAM directly.
add_executable(schwaystation-test-exe ${CMAKE_CURRENT_LIST_DIR}/exe_tests.c)
target_include_directories(schwaystation-test-exe PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-exe PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-exe PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-exe)

# Fault prevented: a header field read from the wrong offset or byte order, or a
# probe that disagrees with the header it was given.
schway_add_test(NAME exe.probe_valid
  COMMAND schwaystation-test-exe probe_valid
  LABELS exe portable TIMEOUT 10)
