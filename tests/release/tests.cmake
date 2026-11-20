# Release tests, label release: they read the archives the package step
# writes, so the ci workflows run them in a second test step after packaging
# and the first test step excludes them.
cmake_minimum_required(VERSION 3.25)

function(release_info_value path key output)
  file(STRINGS "${path}" info_lines)
  set(found FALSE)
  foreach(line IN LISTS info_lines)
    if(line MATCHES "^${key} = \"([^\"]*)\"$")
      if(found)
        message(FATAL_ERROR "release.notices: duplicate .info key ${key}")
      endif()
      set(found TRUE)
      set(value "${CMAKE_MATCH_1}")
    endif()
  endforeach()
  if(NOT found)
    message(FATAL_ERROR "release.notices: missing .info key ${key} in ${path}")
  endif()
  set(${output} "${value}" PARENT_SCOPE)
endfunction()

if(CMAKE_SCRIPT_MODE_FILE)
  foreach(required ARCHIVE EXPECTED_INFO_FILE)
    if(NOT EXISTS "${${required}}")
      message(FATAL_ERROR "release.notices: ${required} does not exist")
    endif()
  endforeach()
  if(NOT DEFINED OUT_DIR OR "${OUT_DIR}" STREQUAL "")
    message(FATAL_ERROR "release.notices: OUT_DIR is required")
  endif()
  file(REMOVE_RECURSE "${OUT_DIR}")
  file(ARCHIVE_EXTRACT INPUT "${ARCHIVE}" DESTINATION "${OUT_DIR}")
  set(packaged_info "${OUT_DIR}/info/schwaystation_libretro.info")
  if(NOT EXISTS "${packaged_info}")
    message(FATAL_ERROR "release.notices: archive lacks info/schwaystation_libretro.info")
  endif()
  foreach(key supported_extensions license)
    release_info_value("${EXPECTED_INFO_FILE}" ${key} expected)
    release_info_value("${packaged_info}" ${key} actual)
    if(NOT actual STREQUAL expected)
      message(FATAL_ERROR "release.notices: packaged .info ${key} does not match configured provider")
    endif()
  endforeach()
  message(STATUS "release.notices: packaged metadata matches configured provider")
  return()
endif()

set(release_scripts ${CMAKE_CURRENT_LIST_DIR}/../cmake)
set(release_suffix ${SCHWAYSTATION_OS}-${SCHWAYSTATION_ARCH})

schway_add_test(NAME release.archive_contents
  COMMAND ${CMAKE_COMMAND} -DDIR=${PROJECT_BINARY_DIR} -DSUFFIX=${release_suffix}
    -DSOURCE_DIR=${PROJECT_SOURCE_DIR} -DBINARY_DIR=${PROJECT_BINARY_DIR}
    -P ${release_scripts}/archive_contents.cmake
  LABELS release TIMEOUT 60)

# The self test checks a libretro zip built without THIRD-PARTY-NOTICES.md
# and expects that file named.
set(release_fixture ${PROJECT_BINARY_DIR}/release/self_test)
set(release_core schwaystation_libretro.so)
if(SCHWAYSTATION_OS STREQUAL "macos")
  set(release_core schwaystation_libretro.dylib)
elseif(SCHWAYSTATION_OS STREQUAL "windows")
  set(release_core schwaystation_libretro.dll)
endif()
file(REMOVE_RECURSE ${release_fixture})
file(COPY ${PROJECT_SOURCE_DIR}/LICENSE DESTINATION ${release_fixture}/stage)
file(WRITE ${release_fixture}/stage/cores/${release_core} "core\n")
file(WRITE ${release_fixture}/stage/info/schwaystation_libretro.info "info\n")
file(MAKE_DIRECTORY ${release_fixture}/zips)
execute_process(
  COMMAND ${CMAKE_COMMAND} -E tar cf
    ${release_fixture}/zips/schwaystation-libretro-${release_suffix}.zip --format=zip
    LICENSE cores info
  WORKING_DIRECTORY ${release_fixture}/stage
  COMMAND_ERROR_IS_FATAL ANY)
set(release_cmd ${CMAKE_COMMAND} -DDIR=${release_fixture}/zips -DSUFFIX=${release_suffix}
  -DSOURCE_DIR=${PROJECT_SOURCE_DIR} -DBINARY_DIR=${release_fixture} -DCOMPONENTS=libretro
  -P ${release_scripts}/archive_contents.cmake)
string(REPLACE ";" "$<SEMICOLON>" release_cmd "${release_cmd}")
schway_add_test(NAME release.archive_contents.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${release_cmd}" -DCODE=1
    "-DREGEX=lacks: THIRD-PARTY-NOTICES.md" -P ${release_scripts}/expect_fail.cmake
  LABELS release TIMEOUT 10)

# Compare packaged CHD claims with the configured info already checked at configure time.
set(release_notice_archive
  ${PROJECT_BINARY_DIR}/schwaystation-libretro-${release_suffix}.zip)
set(release_notice_info ${PROJECT_BINARY_DIR}/libretro/schwaystation_libretro.info)
set(release_notice_out ${PROJECT_BINARY_DIR}/release/notices/extracted)
schway_add_test(NAME release.notices
  COMMAND ${CMAKE_COMMAND} "-DARCHIVE=${release_notice_archive}"
    "-DEXPECTED_INFO_FILE=${release_notice_info}" "-DOUT_DIR=${release_notice_out}"
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS release TIMEOUT 60)

set(release_notice_fixture ${PROJECT_BINARY_DIR}/release/notices_self_test)
set(release_notice_stage ${release_notice_fixture}/stage)
set(release_notice_zips ${release_notice_fixture}/zips)
file(REMOVE_RECURSE ${release_notice_fixture})
file(MAKE_DIRECTORY ${release_notice_stage}/info ${release_notice_zips})
file(COPY_FILE ${release_notice_info}
  ${release_notice_stage}/info/schwaystation_libretro.info)
file(COPY_FILE ${PROJECT_SOURCE_DIR}/LICENSE ${release_notice_stage}/LICENSE)
set(release_notice_test_archive
  ${release_notice_zips}/schwaystation-libretro-${release_suffix}.zip)
execute_process(COMMAND ${CMAKE_COMMAND} -E tar cf ${release_notice_test_archive}
    --format=zip LICENSE info
  WORKING_DIRECTORY ${release_notice_stage} COMMAND_ERROR_IS_FATAL ANY)
file(READ ${release_notice_info} release_notice_bad_info)
string(REGEX REPLACE "license = \"[^\"]*\"" "license = \"wrong\""
  release_notice_bad_info "${release_notice_bad_info}")
set(release_notice_bad_expected ${release_notice_fixture}/wrong.info)
file(WRITE ${release_notice_bad_expected} "${release_notice_bad_info}")
set(release_notice_self_cmd "${CMAKE_COMMAND};-DARCHIVE=${release_notice_test_archive};-DEXPECTED_INFO_FILE=${release_notice_bad_expected};-DOUT_DIR=${release_notice_fixture}/extract;-P;${CMAKE_CURRENT_LIST_FILE}")
string(REPLACE ";" "$<SEMICOLON>" release_notice_self_cmd "${release_notice_self_cmd}")
schway_add_test(NAME release.notices.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${release_notice_self_cmd}" -DCODE=1
    "-DREGEX=packaged .info license does not match configured provider"
    -P ${release_scripts}/expect_fail.cmake
  LABELS release TIMEOUT 10)

# The scanner fixture contains generated bytes and a separate one-row table.
# Nothing in it is firmware, and no synthetic key reaches the production table.
find_package(Git REQUIRED)
set(release_scan_dir ${PROJECT_BINARY_DIR}/release/firmware_scan_self_test)
set(release_scan_root ${release_scan_dir}/tracked)
set(release_scan_stage ${release_scan_dir}/archive_stage)
set(release_scan_archive ${release_scan_dir}/synthetic.zip)
set(release_scan_table ${release_scan_dir}/synthetic_table.inc)
file(REMOVE_RECURSE "${release_scan_dir}")
file(MAKE_DIRECTORY "${release_scan_root}" "${release_scan_stage}")
set(release_scan_payload "Synthetic scanner fixture; no firmware bytes.\n")
file(WRITE "${release_scan_root}/matching.data" "${release_scan_payload}")
file(WRITE "${release_scan_stage}/matching.member" "${release_scan_payload}")
file(MD5 "${release_scan_root}/matching.data" release_scan_md5)
file(SHA1 "${release_scan_root}/matching.data" release_scan_sha1)

function(release_scan_bytes hex output)
  string(LENGTH "${hex}" hex_length)
  math(EXPR last "${hex_length} - 2")
  set(bytes "")
  foreach(offset RANGE 0 ${last} 2)
    string(SUBSTRING "${hex}" ${offset} 2 byte)
    string(APPEND bytes "0x${byte}u, ")
  endforeach()
  set(${output} "${bytes}" PARENT_SCOPE)
endfunction()
release_scan_bytes("${release_scan_md5}" release_scan_md5_bytes)
release_scan_bytes("${release_scan_sha1}" release_scan_sha1_bytes)
file(WRITE "${release_scan_table}"
  "/* Synthetic test key only. */\n{{${release_scan_md5_bytes}},\n  {${release_scan_sha1_bytes}}, SCHWAY_REGION_NTSC_U},\n")
execute_process(COMMAND "${GIT_EXECUTABLE}" init --quiet "${release_scan_root}"
  COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${release_scan_root}" add -- matching.data
  WORKING_DIRECTORY "${release_scan_root}" COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND "${CMAKE_COMMAND}" -E tar cf "${release_scan_archive}"
  --format=zip matching.member
  WORKING_DIRECTORY "${release_scan_stage}" COMMAND_ERROR_IS_FATAL ANY)

set(release_scan_self_cmd "${CMAKE_COMMAND}" -DTRACKED=1
  "-DROOT=${release_scan_root}" "-DGIT=${GIT_EXECUTABLE}"
  "-DZIPS=${release_scan_archive}" "-DBIOS_TABLE=${release_scan_table}"
  -P "${release_scripts}/firmware_scan.cmake")
string(REPLACE ";" "$<SEMICOLON>" release_scan_self_cmd "${release_scan_self_cmd}")
schway_add_test(NAME release.firmware_scan
  COMMAND ${CMAKE_COMMAND} -DTRACKED=1 "-DROOT=${PROJECT_SOURCE_DIR}"
    "-DZIPS=${PROJECT_BINARY_DIR}/schwaystation-*-${release_suffix}.zip"
    "-DBIOS_TABLE=${PROJECT_SOURCE_DIR}/src/bios_table.inc"
    -P ${release_scripts}/firmware_scan.cmake
  LABELS release TIMEOUT 60)
schway_add_test(NAME release.firmware_scan.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${release_scan_self_cmd}" -DCODE=1
    "-DREGEX=2 prohibited content matches found"
    -P ${release_scripts}/expect_fail.cmake
  LABELS release TIMEOUT 30)

set(release_scan_edge_dir ${release_scan_dir}/archive_edges)
set(release_scan_edge_stage ${release_scan_edge_dir}/nested_stage)
set(release_scan_edge_root ${release_scan_edge_dir}/clean_tracked)
set(release_scan_clean_stage ${release_scan_edge_dir}/clean_stage)
set(release_scan_empty_stage ${release_scan_edge_dir}/empty_stage)
set(release_scan_content_stage ${release_scan_edge_dir}/content_stage)
file(MAKE_DIRECTORY "${release_scan_edge_stage}/deep/nested"
  "${release_scan_edge_root}" "${release_scan_clean_stage}/nested"
  "${release_scan_empty_stage}/empty" "${release_scan_content_stage}")
file(WRITE "${release_scan_edge_stage}/deep/nested/matching.data" "${release_scan_payload}")
set(release_scan_changed_payload "Synthetic scanner fixture; no firmware bytes!\n")
file(WRITE "${release_scan_edge_root}/changed.data" "${release_scan_changed_payload}")
file(WRITE "${release_scan_clean_stage}/nested/changed.member" "${release_scan_changed_payload}")
execute_process(COMMAND "${GIT_EXECUTABLE}" init --quiet "${release_scan_edge_root}"
  COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${release_scan_edge_root}" add -- changed.data
  WORKING_DIRECTORY "${release_scan_edge_root}" COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND "${CMAKE_COMMAND}" -E tar cf "${release_scan_edge_dir}/nested.zip"
  --format=zip deep/nested/matching.data
  WORKING_DIRECTORY "${release_scan_edge_stage}" COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND "${CMAKE_COMMAND}" -E tar cf "${release_scan_edge_dir}/clean.zip"
  --format=zip nested/changed.member
  WORKING_DIRECTORY "${release_scan_clean_stage}" COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND "${CMAKE_COMMAND}" -E tar cf "${release_scan_edge_dir}/empty.zip"
  --format=zip empty
  WORKING_DIRECTORY "${release_scan_empty_stage}" COMMAND_ERROR_IS_FATAL ANY)
file(WRITE "${release_scan_edge_dir}/invalid.zip" "synthetic invalid archive")
string(REPEAT "x" 524288 release_scan_bios_size)
file(WRITE "${release_scan_content_stage}/size.member" "${release_scan_bios_size}")
string(ASCII 80 83 45 88 32 69 88 69 release_scan_exe_signature)
file(WRITE "${release_scan_content_stage}/exe.member" "${release_scan_exe_signature}")
string(REPEAT "x" 32769 release_scan_cd_prefix)
string(ASCII 67 68 48 48 49 release_scan_cd_signature)
file(WRITE "${release_scan_content_stage}/cd_2048.member"
  "${release_scan_cd_prefix}${release_scan_cd_signature}")
string(REPEAT "x" 37657 release_scan_cd_prefix)
file(WRITE "${release_scan_content_stage}/cd_2352.member"
  "${release_scan_cd_prefix}${release_scan_cd_signature}")
string(ASCII 76 105 99 101 110 115 101 100 32 98 121 32 83 111 110 121
  32 67 111 109 112 117 116 101 114 32 69 110 116 101 114 116 release_scan_licence_window)
file(WRITE "${release_scan_content_stage}/licence.member" "${release_scan_licence_window}")
execute_process(COMMAND "${CMAKE_COMMAND}" -E tar cf "${release_scan_edge_dir}/content.zip"
  --format=zip size.member exe.member cd_2048.member cd_2352.member licence.member
  WORKING_DIRECTORY "${release_scan_content_stage}" COMMAND_ERROR_IS_FATAL ANY)
schway_add_test(NAME release.firmware_scan.archive_edges
  COMMAND ${CMAKE_COMMAND} -DARCHIVE_EDGE_TEST=1
    "-DEDGE_DIR=${release_scan_edge_dir}" "-DGIT=${GIT_EXECUTABLE}"
    "-DBIOS_TABLE=${release_scan_table}" -P ${release_scripts}/firmware_scan.cmake
  LABELS release TIMEOUT 30)
