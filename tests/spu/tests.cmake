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

include(${PROJECT_SOURCE_DIR}/cmake/SPUReferences.cmake)
schway_prepare_spu_reference_assets(
  spu_reference_dir spu_reference_exe spu_reference_log
  spu_reference_exe_sha256 spu_reference_log_sha256)
schway_add_spu_reference_target("${spu_reference_exe_sha256}" "${spu_reference_log_sha256}")

schway_add_test(NAME spu.reference.controls
  COMMAND schwaystation-test-spu-reference predicate_controls
  LABELS audio portable TIMEOUT 10)

schway_add_test(NAME spu.reference.fetch_controls
  COMMAND ${CMAKE_COMMAND}
    "-DSCHWAY_SPU_REFERENCE_CONTROL_DIR=${PROJECT_BINARY_DIR}/tests/spu/reference-fetch-controls"
    -P ${PROJECT_SOURCE_DIR}/cmake/SPUReferences.cmake
  LABELS audio portable TIMEOUT 30)

schway_add_test(NAME spu.reference.log
  COMMAND schwaystation-test-spu-reference log "${spu_reference_log}"
  LABELS audio portable TIMEOUT 10)
