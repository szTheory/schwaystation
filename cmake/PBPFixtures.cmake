set(schway__pbp_source_bin "${PROJECT_BINARY_DIR}/fixtures/disc_boot.bin")
set(schway__pbp_image "${PROJECT_BINARY_DIR}/fixtures/schwaystation-disc_boot.pbp")
set(schway__pbp_cue "${PROJECT_BINARY_DIR}/fixtures/disc_boot_pbp.cue")
set(schway__pbp_manifest "${PROJECT_BINARY_DIR}/tests/disc_boot_pbp.manifest")
set(schway__pbp_generator "${PROJECT_SOURCE_DIR}/tools/gen_pbp.py")

add_custom_command(OUTPUT "${schway__pbp_image}"
  BYPRODUCTS "${schway__pbp_cue}"
    "${PROJECT_BINARY_DIR}/fixtures/disc_boot_pbp.bin"
    "${schway__pbp_manifest}"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${PROJECT_BINARY_DIR}/tests"
  COMMAND "${Python3_EXECUTABLE}" "${schway__pbp_generator}"
    --source "${schway__pbp_source_bin}"
    --output "${schway__pbp_image}"
    --cue "${schway__pbp_cue}"
    --manifest "${schway__pbp_manifest}"
  DEPENDS schwaystation-disc-fixtures "${schway__pbp_source_bin}" "${schway__pbp_generator}"
  COMMENT "Build the bounded owned PBP fixture"
  VERBATIM)
add_custom_target(schwaystation-pbp-fixtures ALL DEPENDS "${schway__pbp_image}")
