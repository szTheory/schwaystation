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
    video.tuple video.pitch info audio.ring profile.rejected profile.locked reset.power
    api.size_tags api.statuses api.error_changes_nothing create.allocator instances.independent)
  schway_add_test(NAME core.${case}
    COMMAND schwaystation-test-core ${case}
    LABELS core portable TIMEOUT 10)
endforeach()

# audio.pairs runs 8,000 frames, so its time is set by the lane's memory speed:
# 1.2 s on an Apple core, 6.42 s on the Intel macOS runner in run 40942957712
# (79 tests), over 10 s in run 42687433421 (114 tests, run in parallel).
schway_add_test(NAME core.audio.pairs
  COMMAND schwaystation-test-core audio.pairs
  LABELS core portable TIMEOUT 30)

# Save states and the field visitor. Fault prevented: a state that does not
# round-trip byte for byte, a corruption applied before it is refused, or a
# visitor that misses a field a save writes.
add_executable(schwaystation-test-state ${CMAKE_CURRENT_LIST_DIR}/test_state.c)
target_include_directories(schwaystation-test-state PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-state PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-state PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-state)

foreach(case IN ITEMS state.cpu_fields state.round_trip state.stable_bytes state.rejects state.visit
    state.record_sweep state.run_bounds)
  schway_add_test(NAME core.${case}
    COMMAND schwaystation-test-state ${case}
    LABELS core portable TIMEOUT 10)
endforeach()

# The I/O page sweep. Fault prevented: an address of 1F801000h-1F801FFFh that
# is owned, unbuilt and a gap at once, none of them, or on the wrong side of a
# range edge.
add_executable(schwaystation-test-bus-unbuilt ${CMAKE_CURRENT_LIST_DIR}/bus_unbuilt.c)
target_include_directories(schwaystation-test-bus-unbuilt PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-bus-unbuilt PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-bus-unbuilt PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-bus-unbuilt)

schway_add_test(NAME core.bus_unbuilt
  COMMAND schwaystation-test-bus-unbuilt
  LABELS bus portable TIMEOUT 10)

# The bounded run and the split property. Fault prevented: a run that overshoots
# its cycle budget or the frame boundary, a stop and a resume that leave a
# different state than one run, or a frame end a hostile state can push a run
# past.
add_executable(schwaystation-test-run ${CMAKE_CURRENT_LIST_DIR}/test_run.c)
target_include_directories(schwaystation-test-run PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-run PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-run PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-run)

foreach(case IN ITEMS budget_resume split_equivalence peek_no_side_effects peek_rejects peek_values
    fault_none fault_latched)
  schway_add_test(NAME core.run.${case}
    COMMAND schwaystation-test-run ${case}
    LABELS core portable TIMEOUT 10)
endforeach()

# The trace ring. Fault prevented: a traced run that loses or misorders its
# records, a ring that does not keep exactly the last 4,096, a fault that is not
# the last record, an index past the end that answers, or tracing that changes
# the state, video, audio or fault record.
add_executable(schwaystation-test-trace ${CMAKE_CURRENT_LIST_DIR}/test_trace.c)
target_include_directories(schwaystation-test-trace PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-trace PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-trace PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-trace)

foreach(case IN ITEMS exec_records wrap short_run last_is_fault index_past_end irq)
  schway_add_test(NAME core.trace.${case}
    COMMAND schwaystation-test-trace ${case}
    LABELS core portable TIMEOUT 10)
endforeach()

schway_add_test(NAME core.trace_hash_equal
  COMMAND schwaystation-test-trace hash_equal
  LABELS core portable TIMEOUT 10)
