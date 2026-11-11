# Memory card protocol tests through JOY MMIO and the instance scheduler.
add_executable(schwaystation-test-card ${CMAKE_CURRENT_LIST_DIR}/card_tests.c)
target_include_directories(schwaystation-test-card PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-card PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-card PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-card)

# Fault prevented: card address handshakes lose their distinct scheduled ACK deadline.
schway_add_test(NAME card.protocol.read
  COMMAND schwaystation-test-card protocol_read
  LABELS card portable TIMEOUT 10)

# Fault prevented: invalid sectors and restored protocol state corrupt card images or stall reads.
schway_add_test(NAME card.protocol.read_edges
  COMMAND schwaystation-test-card protocol_read_edges
  LABELS card portable TIMEOUT 10)

# Fault prevented: card writes become visible before their successful terminal byte.
schway_add_test(NAME card.protocol.write_commit
  COMMAND schwaystation-test-card protocol_write_commit
  LABELS card portable TIMEOUT 10)

# Fault prevented: edge addresses, slots, failed rewrites or interrupted transactions corrupt card data.
schway_add_test(NAME card.protocol.write_edges
  COMMAND schwaystation-test-card protocol_write_edges
  LABELS card portable TIMEOUT 10)
