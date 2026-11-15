# CD-ROM register, command and deadline behavior through MMIO and the scheduler.
add_executable(schwaystation-test-cdrom ${CMAKE_CURRENT_LIST_DIR}/model.c)
target_include_directories(schwaystation-test-cdrom PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-cdrom PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-cdrom PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-cdrom)

# Fault prevented: the command response does not raise the enabled CD interrupt,
# or acknowledging it leaves a stale interrupt code visible.
schway_add_test(NAME cdrom.command
  COMMAND schwaystation-test-cdrom command
  LABELS disc portable TIMEOUT 10)

# Fault prevented: the Nop acknowledge arrives before or after the selected
# 50,000-cycle deadline.
schway_add_test(NAME cdrom.timing
  COMMAND schwaystation-test-cdrom timing
  LABELS disc portable TIMEOUT 10)

# Fault prevented: media/profile, regional firmware, lid and malformed-command
# responses drift from the CD command matrix.
schway_add_test(NAME cdrom.region
  COMMAND schwaystation-test-cdrom region
  LABELS disc portable TIMEOUT 10)

# Fault prevented: parameter writes overflow the 16-byte FIFO, clearing it does
# not restore write readiness, or an empty result read returns a stale byte.
schway_add_test(NAME cdrom.bounds
  COMMAND schwaystation-test-cdrom bounds
  LABELS disc portable TIMEOUT 10)
