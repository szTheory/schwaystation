add_executable(schwaystation-test-compat ${CMAKE_CURRENT_LIST_DIR}/compat_tests.c)
set_target_properties(schwaystation-test-compat PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-compat)

# Fault prevented: rows or a header that do not use the ten meaningful fields.
schway_add_test(NAME compat.ratchet.rows
  COMMAND schwaystation-test-compat schema ${PROJECT_SOURCE_DIR}/tests/compat/ledger.csv
  LABELS compat portable TIMEOUT 10)
