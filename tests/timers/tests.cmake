# The three root counters: sources, targets, sync modes and interrupts.

# The counters on the system clock: closed-form counting, targets, wraps, the
# write hold and the four interrupt modes. One executable runs the case named on
# its command line; it sees src/ to drive the registers through the bus and read
# the scheduler's deadline.
add_executable(schwaystation-test-timers ${CMAKE_CURRENT_LIST_DIR}/timer_tests.c)
target_include_directories(schwaystation-test-timers PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-timers PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-timers PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-timers)

# Fault prevented: a counter that does not count the system clock over 8 at
# every eighth tick, a FFFFh wrap that is not 10000h cycles long, an interrupt
# raised at another tick than the event's or by anything but the scheduler.
schway_add_test(NAME timers.div8.wrap_irq
  COMMAND schwaystation-test-timers div8_wrap_irq
  LABELS timers portable TIMEOUT 10)
