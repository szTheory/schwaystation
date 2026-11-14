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

# Fault prevented: ADSR rate periods and steps diverge from the baseline table.
add_executable(schwaystation-test-spu-envelopes ${CMAKE_CURRENT_LIST_DIR}/envelopes.c)
target_link_libraries(schwaystation-test-spu-envelopes PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-spu-envelopes PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-spu-envelopes)
schway_add_test(NAME spu.envelopes
  COMMAND schwaystation-test-spu-envelopes
  LABELS audio portable TIMEOUT 10)

# Fault prevented: BRR samples fail to decode and reach Gaussian interpolation.
add_executable(schwaystation-test-spu-voices ${CMAKE_CURRENT_LIST_DIR}/voices.c)
target_link_libraries(schwaystation-test-spu-voices PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-spu-voices PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-spu-voices)
schway_add_test(NAME spu.voices
  COMMAND schwaystation-test-spu-voices
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

schway_add_test(NAME spu.reference.first_upload
  COMMAND schwaystation-test-spu-reference memory-transfer --first-upload --assets "${spu_reference_dir}"
  LABELS audio portable TIMEOUT 120)

schway_add_test(NAME spu.reference.log
  COMMAND schwaystation-test-spu-reference log "${spu_reference_log}"
  LABELS audio portable TIMEOUT 10)
