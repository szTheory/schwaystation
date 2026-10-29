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

# Fault prevented: a card rule, display tuple, cycle or audio count, profile,
# reset or size-tag contract that drifts from the documents without a test
# failing on some lane.
foreach(case IN ITEMS testcard.pixels testcard.counter testcard.frames_differ testcard.full_redraw
    video.tuple video.pitch info audio.pairs audio.ring profile.rejected profile.locked reset.power
    api.size_tags api.statuses api.error_changes_nothing create.allocator instances.independent)
  schway_add_test(NAME core.${case}
    COMMAND schwaystation-test-core ${case}
    LABELS core portable TIMEOUT 10)
endforeach()

# Save states and the field visitor. Fault prevented: a state that does not
# round-trip byte for byte, a corruption applied before it is refused, or a
# visitor that misses a field a save writes.
add_executable(schwaystation-test-state ${CMAKE_CURRENT_LIST_DIR}/test_state.c)
target_include_directories(schwaystation-test-state PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-state PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-state PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-state)

foreach(case IN ITEMS state.round_trip state.stable_bytes state.rejects state.visit
    state.record_sweep state.run_bounds)
  schway_add_test(NAME core.${case}
    COMMAND schwaystation-test-state ${case}
    LABELS core portable TIMEOUT 10)
endforeach()
