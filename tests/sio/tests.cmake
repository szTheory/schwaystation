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
