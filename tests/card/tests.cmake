# Memory card protocol tests through JOY MMIO and the instance scheduler.
add_executable(schwaystation-test-card ${CMAKE_CURRENT_LIST_DIR}/card_tests.c)
target_include_directories(schwaystation-test-card PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-card PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-card PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-card)

# Host-side card import and persistence use a real temporary filesystem.
add_executable(schwaystation-test-card-files ${CMAKE_CURRENT_LIST_DIR}/files.c)
target_include_directories(schwaystation-test-card-files PRIVATE
  ${PROJECT_SOURCE_DIR}/host
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_BINARY_DIR}/generated)
target_compile_definitions(schwaystation-test-card-files PRIVATE SCHWAY_HOST_CARD_TESTING=1)
target_link_libraries(schwaystation-test-card-files PRIVATE schwaystation-host)
set_target_properties(schwaystation-test-card-files PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-card-files)

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

# Fault prevented: imports or state restores partially apply invalid card data or roll back saves.
schway_add_test(NAME card.state.import_atomic
  COMMAND schwaystation-test-card state_import_atomic
  LABELS card portable TIMEOUT 10)

# Fault prevented: state restores accept impossible in-flight card transport fields.
schway_add_test(NAME card.state.pending_edges
  COMMAND schwaystation-test-card state_pending_edges
  LABELS card portable TIMEOUT 10)

# Fault prevented: raw card imports rewrite their source or atomic saves expose partial data.
schway_add_test(NAME card.files.atomic_roundtrip
  COMMAND schwaystation-test-card-files atomic_roundtrip
  LABELS card portable TIMEOUT 10)

# Fault prevented: malformed imports mutate a card or failed durable writes replace the old image.
schway_add_test(NAME card.files.atomic_failures
  COMMAND schwaystation-test-card-files atomic_failures
  LABELS card portable TIMEOUT 10)
