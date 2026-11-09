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

# Fault prevented: a header field read from the wrong offset or byte order, or
# a probe that disagrees with the header it was given.
schway_add_test(NAME exe.probe_valid
  COMMAND schwaystation-test-exe probe_valid
  LABELS exe portable TIMEOUT 10)

# Fault prevented: a hostile header that passes a rule, or a refused file that
# writes a byte of RAM or marks the instance loaded.
schway_add_test(NAME exe.rejects
  COMMAND schwaystation-test-exe rejects
  LABELS exe portable TIMEOUT 10)

# Fault prevented: a limit off by one: text that overruns the end of RAM by a
# sector, a refused empty text, a largest file that is rejected or one byte
# more that is accepted, or a segment alias that loads elsewhere.
schway_add_test(NAME exe.boundaries
  COMMAND schwaystation-test-exe boundaries
  LABELS exe portable TIMEOUT 10)

# Fault prevented: a low-64-KiB overlap or a trailing byte that is refused
# instead of noted, or one that goes unreported.
schway_add_test(NAME exe.lint
  COMMAND schwaystation-test-exe lint
  LABELS exe portable TIMEOUT 10)

# Fault prevented: a region text compared loosely, so one byte off or one byte
# later still names a region.
schway_add_test(NAME exe.licence
  COMMAND schwaystation-test-exe licence
  LABELS exe portable TIMEOUT 10)

# Fault prevented: a text segment at the wrong physical address, a launch
# block with the wrong words, or any other byte of the instance changed by a
# load.
schway_add_test(NAME exe.load_writes
  COMMAND schwaystation-test-exe load_writes
  LABELS exe portable TIMEOUT 10)

# Fault prevented: a refused load that leaves any byte of the saved state
# changed.
schway_add_test(NAME exe.error_changes_nothing
  COMMAND schwaystation-test-exe error_changes_nothing
  LABELS exe portable TIMEOUT 10)

# Fault prevented: a load after the first frame that changes the running
# instance, or a power reset that does not allow a new one.
schway_add_test(NAME exe.after_first_frame
  COMMAND schwaystation-test-exe after_first_frame
  LABELS exe portable TIMEOUT 10)

# Fault prevented: a debug-port bit that is not stored, an unknown flag bit
# that is accepted, or an older caller's profile that keeps a stale bit.
schway_add_test(NAME exe.profile_flags
  COMMAND schwaystation-test-exe profile_flags
  LABELS exe portable TIMEOUT 10)
