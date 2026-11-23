# CD-ROM register, command and deadline behavior through MMIO and the scheduler.
add_executable(schwaystation-test-cdrom
  ${CMAKE_CURRENT_LIST_DIR}/model.c
  ${CMAKE_CURRENT_LIST_DIR}/sectors.c)
target_include_directories(schwaystation-test-cdrom PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-cdrom PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-cdrom PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-cdrom)

add_executable(schwaystation-test-cdrom-sbi ${CMAKE_CURRENT_LIST_DIR}/sbi.c)
target_include_directories(schwaystation-test-cdrom-sbi PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-cdrom-sbi PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-cdrom-sbi PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-cdrom-sbi)

schway_add_test(NAME cdrom.sbi
  COMMAND schwaystation-test-cdrom-sbi
  LABELS disc portable TIMEOUT 10)

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

# Fault prevented: scheduled host callbacks, INT1 ordering or RDDATA contents
# differ from the selected cycle and sector window contract.
schway_add_test(NAME cdrom.sectors
  COMMAND schwaystation-test-cdrom sectors
  LABELS disc portable TIMEOUT 10)

# Fault prevented: pending CD interrupts or a bad-Q marker corrupt the bounded
# one-sector delivery state.
schway_add_test(NAME cdrom.delivery
  COMMAND schwaystation-test-cdrom delivery
  LABELS disc portable TIMEOUT 10)

add_executable(schwaystation-test-cdxa ${CMAKE_CURRENT_LIST_DIR}/xa.c)
target_include_directories(schwaystation-test-cdxa PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-cdxa PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-cdxa PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-cdxa)

# Fault prevented: XA Form2 bytes, ADPCM predictors or output sample count drift.
schway_add_test(NAME disc.xa
  COMMAND schwaystation-test-cdxa
  LABELS disc portable TIMEOUT 10)

add_executable(schwaystation-test-cdrom-audio ${CMAKE_CURRENT_LIST_DIR}/audio.c)
target_include_directories(schwaystation-test-cdrom-audio PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-cdrom-audio PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-cdrom-audio PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-cdrom-audio)

# Fault prevented: queued CD-DA bypasses or drains outside the native SPU tick.
schway_add_test(NAME disc.audio.cdda
  COMMAND schwaystation-test-cdrom-audio cdda
  LABELS disc portable TIMEOUT 10)

# Fault prevented: accepted XA sectors fail to produce native-rate CD audio.
schway_add_test(NAME disc.audio.xa
  COMMAND schwaystation-test-cdrom-audio xa
  LABELS disc portable TIMEOUT 10)

# Fault prevented: XA and CDDA source samples fail to mix on one native tick.
schway_add_test(NAME disc.audio.combined
  COMMAND schwaystation-test-cdrom-audio combined
  LABELS disc portable TIMEOUT 10)
