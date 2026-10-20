# The core's behaviour through the public API: test card, video, audio, profile, reset, allocator and state.
# One executable runs the case named on its command line. It also sees src/,
# so it can compare the frame with the test card's pixel function and reach
# the instance's VRAM.
add_executable(schwaystation-test-core ${CMAKE_CURRENT_LIST_DIR}/test_core.c)
target_include_directories(schwaystation-test-core PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-core PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-core PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-core)

# Fault prevented: a frame, a query or a state call that allocates after
# create, which a host running a frame from an audio callback or a
# fixed-memory target cannot accept.
schway_add_test(NAME core.no_alloc_after_load
  COMMAND schwaystation-test-core no_alloc_after_load
  LABELS perf portable TIMEOUT 10)
