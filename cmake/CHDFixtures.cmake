if(CMAKE_SCRIPT_MODE_FILE)
  foreach(schway__chd_fixture_var IN ITEMS
      CHD_TOOL CUE OUTPUT_CHD MANIFEST)
    if(NOT DEFINED ${schway__chd_fixture_var} OR "${${schway__chd_fixture_var}}" STREQUAL "")
      message(FATAL_ERROR "CHD fixture: -D${schway__chd_fixture_var} is required")
    endif()
  endforeach()
  include("${CMAKE_CURRENT_LIST_DIR}/CHDToolPins.cmake")
  foreach(schway__chd_fixture_pin IN ITEMS TOOL_COMMIT TOOL_ARCHIVE_SHA256)
    if(DEFINED ${schway__chd_fixture_pin}
        AND NOT "${${schway__chd_fixture_pin}}" STREQUAL "")
      if(schway__chd_fixture_pin STREQUAL "TOOL_COMMIT")
        set(schway__chd_fixture_expected "${schway__chdtool_commit}")
      else()
        set(schway__chd_fixture_expected "${schway__chdtool_sha256}")
      endif()
      if(NOT "${${schway__chd_fixture_pin}}" STREQUAL "${schway__chd_fixture_expected}")
        message(FATAL_ERROR "CHD fixture: ${schway__chd_fixture_pin} does not match pins.txt")
      endif()
    endif()
  endforeach()
  set(TOOL_COMMIT "${schway__chdtool_commit}")
  set(TOOL_ARCHIVE_SHA256 "${schway__chdtool_sha256}")

  foreach(schway__chd_fixture_path IN ITEMS "${CHD_TOOL}" "${CUE}")
    if(NOT EXISTS "${schway__chd_fixture_path}")
      message(FATAL_ERROR "CHD fixture: required input is missing: ${schway__chd_fixture_path}")
    endif()
  endforeach()

  get_filename_component(schway__chd_fixture_dir "${CUE}" DIRECTORY)
  file(READ "${CUE}" schway__chd_fixture_cue)
  string(REPLACE "\r\n" "\n" schway__chd_fixture_cue "${schway__chd_fixture_cue}")
  set(schway__chd_fixture_expected_cue [=[FILE "disc_boot.bin" BINARY
  TRACK 01 MODE2/2352
    INDEX 01 00:00:00
FILE "disc_boot_track02.bin" BINARY
  TRACK 02 AUDIO
    INDEX 00 00:00:00
    INDEX 01 00:02:00
]=])
  if(NOT schway__chd_fixture_cue STREQUAL schway__chd_fixture_expected_cue)
    message(FATAL_ERROR "CHD fixture: owned CUE track layout changed")
  endif()

  set(schway__chd_fixture_track1 "${schway__chd_fixture_dir}/disc_boot.bin")
  set(schway__chd_fixture_track2 "${schway__chd_fixture_dir}/disc_boot_track02.bin")
  foreach(schway__chd_fixture_track IN ITEMS "${schway__chd_fixture_track1}" "${schway__chd_fixture_track2}")
    if(NOT EXISTS "${schway__chd_fixture_track}")
      message(FATAL_ERROR "CHD fixture: CUE track is missing: ${schway__chd_fixture_track}")
    endif()
  endforeach()
  file(SIZE "${schway__chd_fixture_track1}" schway__chd_fixture_track1_bytes)
  file(SIZE "${schway__chd_fixture_track2}" schway__chd_fixture_track2_bytes)
  foreach(schway__chd_fixture_bytes IN ITEMS
      "${schway__chd_fixture_track1_bytes}" "${schway__chd_fixture_track2_bytes}")
    math(EXPR schway__chd_fixture_remainder "${schway__chd_fixture_bytes} % 2352")
    if(NOT schway__chd_fixture_remainder EQUAL 0)
      message(FATAL_ERROR "CHD fixture: source track is not a whole number of raw sectors")
    endif()
  endforeach()
  math(EXPR schway__chd_fixture_track1_sectors "${schway__chd_fixture_track1_bytes} / 2352")
  math(EXPR schway__chd_fixture_track2_sectors "${schway__chd_fixture_track2_bytes} / 2352")
  math(EXPR schway__chd_fixture_sector_count
    "${schway__chd_fixture_track1_sectors} + ${schway__chd_fixture_track2_sectors}")
  math(EXPR schway__chd_fixture_source_bytes
    "${schway__chd_fixture_track1_bytes} + ${schway__chd_fixture_track2_bytes}")
  if(NOT schway__chd_fixture_track1_sectors EQUAL 64
      OR NOT schway__chd_fixture_track2_sectors EQUAL 158
      OR NOT schway__chd_fixture_sector_count EQUAL 222
      OR schway__chd_fixture_source_bytes GREATER_EQUAL 524288)
    message(FATAL_ERROR "CHD fixture: authored source counts or size cap changed")
  endif()

  execute_process(COMMAND "${CHD_TOOL}" --print-settings
    RESULT_VARIABLE schway__chd_fixture_settings_result
    OUTPUT_VARIABLE schway__chd_fixture_settings
    ERROR_VARIABLE schway__chd_fixture_settings_error
    TIMEOUT 10)
  set(schway__chd_fixture_expected_settings [=[format=schwaystation-chd-tool-v1
codecs=cdlz,cdzl,cdfl,0
hunk_bytes=19584
unit_bytes=2448
workers=1
]=])
  if(NOT schway__chd_fixture_settings_result EQUAL 0
      OR NOT schway__chd_fixture_settings STREQUAL schway__chd_fixture_expected_settings)
    message(FATAL_ERROR "CHD fixture: unexpected generator settings: ${schway__chd_fixture_settings}${schway__chd_fixture_settings_error}")
  endif()

  file(SHA256 "${CUE}" schway__chd_fixture_cue_sha256)
  file(SHA256 "${schway__chd_fixture_track1}" schway__chd_fixture_track1_sha256)
  file(SHA256 "${schway__chd_fixture_track2}" schway__chd_fixture_track2_sha256)
  file(SHA256 "${CHD_TOOL}" schway__chd_fixture_tool_sha256)
  get_filename_component(schway__chd_fixture_output_dir "${OUTPUT_CHD}" DIRECTORY)
  get_filename_component(schway__chd_fixture_manifest_dir "${MANIFEST}" DIRECTORY)
  file(MAKE_DIRECTORY "${schway__chd_fixture_output_dir}" "${schway__chd_fixture_manifest_dir}")
  set(schway__chd_fixture_repeat "${OUTPUT_CHD}.repeat")
  file(REMOVE "${OUTPUT_CHD}" "${MANIFEST}" "${schway__chd_fixture_repeat}")

  execute_process(COMMAND "${CHD_TOOL}" "${CUE}" "${OUTPUT_CHD}"
    RESULT_VARIABLE schway__chd_fixture_first_result
    OUTPUT_VARIABLE schway__chd_fixture_first_output
    ERROR_VARIABLE schway__chd_fixture_first_error
    TIMEOUT 60)
  if(NOT schway__chd_fixture_first_result EQUAL 0 OR NOT EXISTS "${OUTPUT_CHD}")
    file(REMOVE "${OUTPUT_CHD}")
    message(FATAL_ERROR "CHD fixture: first generation failed: ${schway__chd_fixture_first_output}${schway__chd_fixture_first_error}")
  endif()

  execute_process(COMMAND "${CHD_TOOL}" "${CUE}" "${schway__chd_fixture_repeat}"
    RESULT_VARIABLE schway__chd_fixture_repeat_result
    OUTPUT_VARIABLE schway__chd_fixture_repeat_output
    ERROR_VARIABLE schway__chd_fixture_repeat_error
    TIMEOUT 60)
  if(NOT schway__chd_fixture_repeat_result EQUAL 0 OR NOT EXISTS "${schway__chd_fixture_repeat}")
    file(REMOVE "${OUTPUT_CHD}" "${schway__chd_fixture_repeat}")
    message(FATAL_ERROR "CHD fixture: repeat generation failed: ${schway__chd_fixture_repeat_output}${schway__chd_fixture_repeat_error}")
  endif()

  file(SIZE "${OUTPUT_CHD}" schway__chd_fixture_output_bytes)
  file(SIZE "${schway__chd_fixture_repeat}" schway__chd_fixture_repeat_bytes)
  file(SHA256 "${OUTPUT_CHD}" schway__chd_fixture_output_sha256)
  file(SHA256 "${schway__chd_fixture_repeat}" schway__chd_fixture_repeat_sha256)
  file(REMOVE "${schway__chd_fixture_repeat}")
  if(schway__chd_fixture_output_bytes LESS 1
      OR schway__chd_fixture_output_bytes GREATER_EQUAL 524288
      OR NOT schway__chd_fixture_output_bytes EQUAL schway__chd_fixture_repeat_bytes
      OR NOT schway__chd_fixture_output_sha256 STREQUAL schway__chd_fixture_repeat_sha256)
    file(REMOVE "${OUTPUT_CHD}" "${MANIFEST}")
    message(FATAL_ERROR "CHD fixture: output exceeded its cap or repeated bytes differ")
  endif()

  file(WRITE "${MANIFEST}"
    "format=schwaystation-chd-fixture-v1\n"
    "source_cue_sha256=${schway__chd_fixture_cue_sha256}\n"
    "track1_file=disc_boot.bin\n"
    "track1_type=MODE2/2352\n"
    "track1_bytes=${schway__chd_fixture_track1_bytes}\n"
    "track1_sectors=${schway__chd_fixture_track1_sectors}\n"
    "track1_sha256=${schway__chd_fixture_track1_sha256}\n"
    "track2_file=disc_boot_track02.bin\n"
    "track2_type=AUDIO\n"
    "track2_bytes=${schway__chd_fixture_track2_bytes}\n"
    "track2_sectors=${schway__chd_fixture_track2_sectors}\n"
    "track2_sha256=${schway__chd_fixture_track2_sha256}\n"
    "expected_track_count=2\n"
    "logical_sector_count=${schway__chd_fixture_sector_count}\n"
    "tool_commit=${TOOL_COMMIT}\n"
    "tool_archive_sha256=${TOOL_ARCHIVE_SHA256}\n"
    "tool_sha256=${schway__chd_fixture_tool_sha256}\n"
    "codecs=cdlz,cdzl,cdfl,0\n"
    "hunk_bytes=19584\n"
    "unit_bytes=2448\n"
    "workers=1\n"
    "chd_bytes=${schway__chd_fixture_output_bytes}\n"
    "chd_sha256=${schway__chd_fixture_output_sha256}\n")
  message(STATUS "CHD fixture: tracks=${schway__chd_fixture_track1_sectors}+${schway__chd_fixture_track2_sectors} sectors=${schway__chd_fixture_sector_count} bytes=${schway__chd_fixture_output_bytes} sha256=${schway__chd_fixture_output_sha256}")
  return()
endif()

if(NOT SCHWAYSTATION_CHD OR NOT SCHWAYSTATION_BUILD_CHD_TOOL)
  return()
endif()

set(SCHWAYSTATION_CHD_FIXTURE "${PROJECT_BINARY_DIR}/fixtures/disc_boot.chd")
set(SCHWAYSTATION_CHD_MANIFEST "${PROJECT_BINARY_DIR}/tests/disc_boot.chd.manifest")
add_custom_command(OUTPUT "${SCHWAYSTATION_CHD_FIXTURE}" "${SCHWAYSTATION_CHD_MANIFEST}"
  COMMAND "${CMAKE_COMMAND}"
    "-DCHD_TOOL=$<TARGET_FILE:schwaystation-chd-tool>"
    "-DCUE=${PROJECT_BINARY_DIR}/fixtures/disc_boot.cue"
    "-DOUTPUT_CHD=${SCHWAYSTATION_CHD_FIXTURE}"
    "-DMANIFEST=${SCHWAYSTATION_CHD_MANIFEST}"
    "-DTOOL_COMMIT=${schway__chdtool_commit}"
    "-DTOOL_ARCHIVE_SHA256=${schway__chdtool_sha256}"
    -P "${PROJECT_SOURCE_DIR}/cmake/CHDFixtures.cmake"
  DEPENDS schwaystation-disc-fixtures schwaystation-chd-tool
    "${PROJECT_BINARY_DIR}/fixtures/disc_boot.cue"
    "${PROJECT_BINARY_DIR}/fixtures/disc_boot.bin"
    "${PROJECT_BINARY_DIR}/fixtures/disc_boot_track02.bin"
    "${PROJECT_SOURCE_DIR}/cmake/CHDFixtures.cmake"
  COMMENT "Create and verify the owned CHD fixture"
  VERBATIM)
add_custom_target(schwaystation-chd-fixtures
  DEPENDS "${SCHWAYSTATION_CHD_FIXTURE}" "${SCHWAYSTATION_CHD_MANIFEST}")
