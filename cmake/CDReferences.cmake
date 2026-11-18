# Fetch and run the pinned ps1-tests CD timing and Getloc guests.
#
# Fault prevented: timing predicates or Getloc ordering being accepted without
# executing the pinned real callers against the public disc callback.

set(SCHWAY__CD_REFERENCE_MODULE "${CMAKE_CURRENT_LIST_FILE}")

function(schway__cd_reference_verify_file path expected_sha256 label)
  if(NOT EXISTS "${path}")
    message(FATAL_ERROR "CD_REFERENCE_ASSET_ERROR: ${label} is missing")
  endif()
  file(SHA256 "${path}" actual_sha256)
  if(NOT actual_sha256 STREQUAL "${expected_sha256}")
    message(FATAL_ERROR
      "CD_REFERENCE_DIGEST_ERROR: ${label}: SHA-256 mismatch (expected ${expected_sha256}, got ${actual_sha256})")
  endif()
endfunction()

function(schway__cd_reference_pin name expected_version output_url output_sha256)
  file(STRINGS "${PROJECT_SOURCE_DIR}/pins.txt" rows REGEX "^${name} ")
  list(LENGTH rows row_count)
  if(NOT row_count EQUAL 1)
    message(FATAL_ERROR "CD_REFERENCE_PIN_ERROR: pins.txt must contain exactly one ${name} row")
  endif()
  list(GET rows 0 row)
  if(NOT row MATCHES "^${name} ([^ ]+) (https://[^ ]+) ([0-9a-f]+)$")
    message(FATAL_ERROR "CD_REFERENCE_PIN_ERROR: malformed ${name} row in pins.txt")
  endif()
  set(version "${CMAKE_MATCH_1}")
  set(url "${CMAKE_MATCH_2}")
  set(sha256 "${CMAKE_MATCH_3}")
  string(LENGTH "${sha256}" sha256_length)
  if(NOT version STREQUAL "${expected_version}" OR NOT sha256_length EQUAL 64)
    message(FATAL_ERROR "CD_REFERENCE_PIN_ERROR: invalid version or SHA-256 for ${name}")
  endif()
  set(${output_url} "${url}" PARENT_SCOPE)
  set(${output_sha256} "${sha256}" PARENT_SCOPE)
endfunction()

function(schway__cd_reference_fetch_pin name expected_version destination output_path output_sha256)
  schway__cd_reference_pin("${name}" "${expected_version}" pin_url pin_sha256)
  string(REGEX REPLACE "^.*/" "" filename "${pin_url}")
  set(path "${destination}/${filename}")
  set(needs_fetch TRUE)
  if(EXISTS "${path}")
    file(SHA256 "${path}" existing_sha256)
    if(existing_sha256 STREQUAL pin_sha256)
      set(needs_fetch FALSE)
    endif()
  endif()
  if(needs_fetch)
    execute_process(
      COMMAND sh "${PROJECT_SOURCE_DIR}/scripts/fetch-pin.sh" "${name}" "${destination}"
      RESULT_VARIABLE fetch_result
      OUTPUT_VARIABLE fetch_output
      ERROR_VARIABLE fetch_error)
    if(NOT fetch_result STREQUAL "0")
      message(FATAL_ERROR "CD_REFERENCE_FETCH_ERROR: ${fetch_output}${fetch_error}")
    endif()
  endif()
  schway__cd_reference_verify_file("${path}" "${pin_sha256}" "${name}")
  set(${output_path} "${path}" PARENT_SCOPE)
  set(${output_sha256} "${pin_sha256}" PARENT_SCOPE)
endfunction()

function(schway_prepare_cd_reference_assets output_dir output_timing_exe output_timing_log
         output_getloc_exe output_getloc_log output_timing_exe_sha output_getloc_exe_sha
         output_timing_log_sha output_getloc_log_sha)
  file(STRINGS "${PROJECT_SOURCE_DIR}/pins.txt" source_rows
    REGEX "^# ps1-tests-source commit=")
  list(LENGTH source_rows source_count)
  if(NOT source_count EQUAL 1 OR NOT source_rows MATCHES
     "^# ps1-tests-source commit=f727802fead11f1daa7549285548392ef87749cb$")
    message(FATAL_ERROR "CD_REFERENCE_PIN_ERROR: source commit identity is absent or changed")
  endif()

  file(STRINGS "${PROJECT_SOURCE_DIR}/pins.txt" executable_rows
    REGEX "^# ps1-tests-release-member path=cdrom/(timing/timing|getloc/getloc)[.]exe ")
  list(LENGTH executable_rows executable_count)
  if(NOT executable_count EQUAL 2)
    message(FATAL_ERROR "CD_REFERENCE_PIN_ERROR: both release executable identities are required")
  endif()
  set(timing_exe_sha "")
  set(getloc_exe_sha "")
  foreach(row IN LISTS executable_rows)
    if(row MATCHES "^# ps1-tests-release-member path=cdrom/timing/timing[.]exe release=build-158 sha256=([0-9a-f]+) size=16384$")
      set(timing_exe_sha "${CMAKE_MATCH_1}")
    elseif(row MATCHES "^# ps1-tests-release-member path=cdrom/getloc/getloc[.]exe release=build-158 sha256=([0-9a-f]+) size=16384$")
      set(getloc_exe_sha "${CMAKE_MATCH_1}")
    else()
      message(FATAL_ERROR "CD_REFERENCE_PIN_ERROR: malformed CD release executable identity")
    endif()
  endforeach()
  if(NOT timing_exe_sha STREQUAL "500cecd3b2afac6f533c980142067838198ac2d45e8e801cfcc8d4dcd08b0f2d" OR
     NOT getloc_exe_sha STREQUAL "ce1e2904caa1569a9d90036c037d50cf63698dbd156cbe6a5d274b8ae3b0c7dc")
    message(FATAL_ERROR "CD_REFERENCE_PIN_ERROR: CD executable digest differs from the reviewed build-158 members")
  endif()

  set(reference_dir "${PROJECT_BINARY_DIR}/cdrom-reference")
  file(MAKE_DIRECTORY "${reference_dir}")
  schway__cd_reference_fetch_pin(ps1-tests-gpu-captures build-158
    "${reference_dir}/archive" archive archive_sha)
  file(SIZE "${archive}" archive_size)
  if(NOT archive_size EQUAL 6613816)
    message(FATAL_ERROR "CD_REFERENCE_ARCHIVE_ERROR: build-158 tests.zip has an unexpected size")
  endif()
  schway__cd_reference_fetch_pin(ps1-tests-cdrom-timing-log
    f727802fead1 "${reference_dir}/source-timing"
    timing_source_log timing_log_sha)
  schway__cd_reference_fetch_pin(ps1-tests-cdrom-getloc-log
    f727802fead1 "${reference_dir}/source-getloc"
    getloc_source_log getloc_log_sha)

  file(ARCHIVE_EXTRACT INPUT "${archive}" DESTINATION "${reference_dir}"
    PATTERNS "cdrom/timing/timing.exe" "cdrom/timing/psx.log"
             "cdrom/getloc/getloc.exe" "cdrom/getloc/psx.log")
  set(timing_exe "${reference_dir}/cdrom/timing/timing.exe")
  set(timing_log "${reference_dir}/cdrom/timing/psx.log")
  set(getloc_exe "${reference_dir}/cdrom/getloc/getloc.exe")
  set(getloc_log "${reference_dir}/cdrom/getloc/psx.log")
  schway__cd_reference_verify_file("${timing_exe}" "${timing_exe_sha}" "timing executable")
  schway__cd_reference_verify_file("${getloc_exe}" "${getloc_exe_sha}" "Getloc executable")
  schway__cd_reference_verify_file("${timing_log}" "${timing_log_sha}" "release timing log")
  schway__cd_reference_verify_file("${getloc_log}" "${getloc_log_sha}" "release Getloc log")
  schway__cd_reference_verify_file("${timing_source_log}" "${timing_log_sha}" "source timing log")
  schway__cd_reference_verify_file("${getloc_source_log}" "${getloc_log_sha}" "source Getloc log")
  file(SIZE "${timing_exe}" timing_exe_size)
  file(SIZE "${getloc_exe}" getloc_exe_size)
  if(NOT timing_exe_size EQUAL 16384 OR NOT getloc_exe_size EQUAL 16384)
    message(FATAL_ERROR "CD_REFERENCE_ARCHIVE_ERROR: CD guest executable has an unexpected size")
  endif()

  set(${output_dir} "${reference_dir}" PARENT_SCOPE)
  set(${output_timing_exe} "${timing_exe}" PARENT_SCOPE)
  set(${output_timing_log} "${timing_log}" PARENT_SCOPE)
  set(${output_getloc_exe} "${getloc_exe}" PARENT_SCOPE)
  set(${output_getloc_log} "${getloc_log}" PARENT_SCOPE)
  set(${output_timing_exe_sha} "${timing_exe_sha}" PARENT_SCOPE)
  set(${output_getloc_exe_sha} "${getloc_exe_sha}" PARENT_SCOPE)
  set(${output_timing_log_sha} "${timing_log_sha}" PARENT_SCOPE)
  set(${output_getloc_log_sha} "${getloc_log_sha}" PARENT_SCOPE)
endfunction()

function(schway_add_cd_reference_target timing_exe_sha getloc_exe_sha timing_log_sha getloc_log_sha)
  if(TARGET schwaystation-test-cdrom-reference)
    return()
  endif()
  add_executable(schwaystation-test-cdrom-reference
    "${PROJECT_SOURCE_DIR}/tests/cdrom/reference.c"
    "${PROJECT_SOURCE_DIR}/runner/sha256.c")
  target_include_directories(schwaystation-test-cdrom-reference PRIVATE
    "${PROJECT_SOURCE_DIR}/src"
    "${PROJECT_SOURCE_DIR}/runner" "${PROJECT_SOURCE_DIR}/tests")
  target_compile_definitions(schwaystation-test-cdrom-reference PRIVATE
    "SCHWAY_CDROM_TIMING_EXE_SHA256=\"${timing_exe_sha}\""
    "SCHWAY_CDROM_GETLOC_EXE_SHA256=\"${getloc_exe_sha}\""
    "SCHWAY_CDROM_TIMING_LOG_SHA256=\"${timing_log_sha}\""
    "SCHWAY_CDROM_GETLOC_LOG_SHA256=\"${getloc_log_sha}\"")
  target_link_libraries(schwaystation-test-cdrom-reference PRIVATE schwaystation::schwaystation)
  set_target_properties(schwaystation-test-cdrom-reference PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/tests")
  schway_warnings(schwaystation-test-cdrom-reference)
endfunction()

function(schway_register_cd_reference_tests timing_exe timing_log getloc_exe getloc_log)
  schway_add_test(NAME cdrom.reference.timing
    COMMAND schwaystation-test-cdrom-reference timing "${timing_exe}" "${timing_log}"
    LABELS disc nightly portable TIMEOUT 120)
  schway_add_test(NAME cdrom.reference.getloc
    COMMAND schwaystation-test-cdrom-reference getloc "${getloc_exe}" "${getloc_log}"
    LABELS disc nightly portable TIMEOUT 120)
  schway_add_test(NAME cdrom.reference.controls
    COMMAND schwaystation-test-cdrom-reference controls "${getloc_log}"
    LABELS disc nightly portable TIMEOUT 30)
endfunction()
