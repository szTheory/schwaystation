set(schway__pbp_source_bin "${PROJECT_BINARY_DIR}/fixtures/disc_boot_pbp_source.bin")
set(schway__pbp_source_cue "${PROJECT_BINARY_DIR}/fixtures/disc_boot_pbp_source.cue")
set(schway__pbp_source_xml "${PROJECT_BINARY_DIR}/fixtures/disc_boot_pbp.xml")
set(schway__pbp_source_guest "${PROJECT_BINARY_DIR}/fixtures/disc_boot_pbp.exe")
set(schway__pbp_image "${PROJECT_BINARY_DIR}/fixtures/schwaystation-disc_boot.pbp")
set(schway__pbp_cue "${PROJECT_BINARY_DIR}/fixtures/disc_boot_pbp.cue")
set(schway__pbp_manifest "${PROJECT_BINARY_DIR}/tests/disc_boot_pbp.manifest")
set(schway__pbp_generator "${PROJECT_SOURCE_DIR}/tools/gen_pbp.py")

configure_file("${PROJECT_SOURCE_DIR}/tests/fixtures/disc_boot_pbp.xml"
  "${schway__pbp_source_xml}" COPYONLY)
add_custom_command(OUTPUT "${schway__pbp_source_guest}"
  COMMAND "${SCHWAYSTATION_ARMIPS}" -strequ IMAGE "${schway__pbp_source_guest}"
    -equ PBP_MODE 1 disc_boot.asm
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/tests/fixtures/src"
  DEPENDS schwaystation-armips
    "${PROJECT_SOURCE_DIR}/tests/fixtures/src/disc_boot.asm"
    "${PROJECT_SOURCE_DIR}/tests/fixtures/src/header.inc"
    "${PROJECT_SOURCE_DIR}/tests/fixtures/src/mailbox.inc"
  COMMENT "armips single-track PBP guest"
  VERBATIM)
add_custom_command(OUTPUT "${schway__pbp_source_bin}"
  BYPRODUCTS "${schway__pbp_source_cue}"
  COMMAND "${SCHWAYSTATION_MKPSXISO}" -q -y -o "${schway__pbp_source_bin}"
    disc_boot_pbp.xml
  COMMAND "${Python3_EXECUTABLE}" "${schway__pbp_generator}"
    --normalize-cue "${schway__pbp_source_cue}"
  WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/fixtures"
  DEPENDS schwaystation-mkpsxiso "${schway__pbp_source_guest}"
    "${schway__pbp_source_xml}" "${PROJECT_BINARY_DIR}/fixtures/SYSTEM.CNF"
    "${schway__disc_media_dir}/media.sha256"
    "${schway__disc_media_dir}/xa.xa" "${schway__disc_media_dir}/movie.str"
    "${schway__pbp_generator}"
  COMMENT "Build the one-data-track PBP source image"
  VERBATIM)
add_custom_target(schwaystation-pbp-source DEPENDS "${schway__pbp_source_bin}")

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
