# SIO0 registers, scheduled byte replies and controller transactions.
add_executable(schwaystation-test-sio ${CMAKE_CURRENT_LIST_DIR}/sio_tests.c)
target_include_directories(schwaystation-test-sio PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-sio PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-sio PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-sio)

# Fault prevented: a byte reply or IRQ7 that precedes its baud or pad-ACK deadline,
# a digital pad with the wrong handshake, or an ACK on the final byte.
schway_add_test(NAME sio.digital.poll_irq
  COMMAND schwaystation-test-sio digital_poll_irq
  LABELS sio portable TIMEOUT 10)
# Fault prevented: a restored in-flight byte or ACK moves its exact deadline
# or raises IRQ7 more than once.
schway_add_test(NAME sio.state.pending_byte
  COMMAND schwaystation-test-sio state_pending_byte
  LABELS sio portable TIMEOUT 10)
schway_add_test(NAME sio.state.pending_ack
  COMMAND schwaystation-test-sio state_pending_ack
  LABELS sio portable TIMEOUT 10)
# Fault prevented: reset or deselect leaves stale byte/ACK events in the scheduler.
schway_add_test(NAME sio.transport.abort
  COMMAND schwaystation-test-sio transport_abort
  LABELS sio portable TIMEOUT 10)
# Fault prevented: pad button state leaks between ports or core instances.
schway_add_test(NAME sio.pad.independent_ports_instances
  COMMAND schwaystation-test-sio independent_ports_instances
  LABELS sio portable TIMEOUT 10)
