# Public PCM delivery through the library's bounded audio ring.
add_executable(schwaystation-test-spu ${CMAKE_CURRENT_LIST_DIR}/model.c)
target_link_libraries(schwaystation-test-spu PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-spu PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-spu)

# Fault prevented: frame audio count or stereo drain layout diverges from the
# elapsed core cycles or the public 44.1 kHz contract.
schway_add_test(NAME spu.model
  COMMAND schwaystation-test-spu "${SCHWAYSTATION_FIXTURES_DIR}" "$<TARGET_FILE:schwaystation-run>"
  LABELS audio portable TIMEOUT 10)

add_executable(schwaystation-test-spu-reference ${CMAKE_CURRENT_LIST_DIR}/reference.c)
target_include_directories(schwaystation-test-spu-reference PRIVATE ${PROJECT_SOURCE_DIR}/tests)
set_target_properties(schwaystation-test-spu-reference PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-spu-reference)

schway_add_test(NAME spu.reference.controls
  COMMAND schwaystation-test-spu-reference predicate_controls
  LABELS audio portable TIMEOUT 10)
