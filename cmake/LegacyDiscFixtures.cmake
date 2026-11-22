set(schway__legacy_fixture_dir "${PROJECT_BINARY_DIR}/fixtures")
set(schway__legacy_source_bin "${schway__legacy_fixture_dir}/disc_boot_pbp_source.bin")
set(schway__legacy_source_cue "${schway__legacy_fixture_dir}/disc_boot_pbp_source.cue")
set(schway__legacy_disc_cue "${schway__legacy_fixture_dir}/disc_boot.cue")
set(schway__legacy_ecm_generator "${PROJECT_SOURCE_DIR}/tools/gen_ecm.py")
set(schway__legacy_ccd_generator "${PROJECT_SOURCE_DIR}/tools/gen_ccd.py")
set(schway__legacy_ecm_literal "${schway__legacy_fixture_dir}/disc_boot_legacy_literal.ecm")
set(schway__legacy_ecm_modeled "${schway__legacy_fixture_dir}/disc_boot_legacy_modeled.ecm")
set(schway__legacy_ecm_manifest "${schway__legacy_fixture_dir}/disc_boot_legacy_ecm.manifest")
set(schway__legacy_ecm_source_cue "${schway__legacy_fixture_dir}/disc_boot_legacy_source.cue")
set(schway__legacy_ecm_literal_cue "${schway__legacy_fixture_dir}/disc_boot_legacy_literal.cue")
set(schway__legacy_ecm_modeled_cue "${schway__legacy_fixture_dir}/disc_boot_legacy_modeled.cue")
add_custom_command(OUTPUT "${schway__legacy_ecm_literal}" "${schway__legacy_ecm_modeled}"
    "${schway__legacy_ecm_manifest}" "${schway__legacy_ecm_source_cue}"
    "${schway__legacy_ecm_literal_cue}" "${schway__legacy_ecm_modeled_cue}"
  COMMAND "${Python3_EXECUTABLE}" "${schway__legacy_ecm_generator}"
    --source "${schway__legacy_source_bin}" --source-cue "${schway__legacy_source_cue}"
    --output-dir "${schway__legacy_fixture_dir}"
  DEPENDS schwaystation-pbp-source "${schway__legacy_source_bin}" "${schway__legacy_source_cue}"
    "${schway__legacy_ecm_generator}" "${PROJECT_SOURCE_DIR}/tests/fixtures/disc_boot_pbp.xml"
  COMMENT "Build deterministic literal and modeled ECM fixtures"
  VERBATIM)

set(schway__legacy_ccd_img "${schway__legacy_fixture_dir}/disc_boot_legacy.img")
set(schway__legacy_ccd "${schway__legacy_fixture_dir}/disc_boot_legacy.ccd")
set(schway__legacy_ccd_sub "${schway__legacy_fixture_dir}/disc_boot_legacy.sub")
set(schway__legacy_ccd_manifest "${schway__legacy_fixture_dir}/disc_boot_legacy_ccd.manifest")
add_custom_command(OUTPUT "${schway__legacy_ccd_img}" "${schway__legacy_ccd}"
    "${schway__legacy_ccd_sub}" "${schway__legacy_ccd_manifest}"
  COMMAND "${Python3_EXECUTABLE}" "${schway__legacy_ccd_generator}"
    --source-cue "${schway__legacy_disc_cue}" --output-dir "${schway__legacy_fixture_dir}"
  DEPENDS schwaystation-disc-fixtures "${schway__legacy_disc_cue}"
    "${schway__legacy_fixture_dir}/disc_boot.bin"
    "${schway__legacy_fixture_dir}/disc_boot_track02.bin"
    "${schway__legacy_ccd_generator}" "${PROJECT_SOURCE_DIR}/tests/fixtures/disc_boot.xml"
  COMMENT "Build deterministic CloneCD image, descriptor, and SUB fixture"
  VERBATIM)

add_custom_target(schwaystation-legacy-disc-fixtures
  DEPENDS "${schway__legacy_ecm_literal}" "${schway__legacy_ecm_modeled}"
    "${schway__legacy_ecm_manifest}" "${schway__legacy_ecm_source_cue}"
    "${schway__legacy_ecm_literal_cue}" "${schway__legacy_ecm_modeled_cue}"
    "${schway__legacy_ccd_img}" "${schway__legacy_ccd}"
    "${schway__legacy_ccd_sub}" "${schway__legacy_ccd_manifest}")
