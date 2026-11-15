# Prepare and validate the pinned SPU memory-transfer oracle.
#
# Fault prevented: an unpinned, incomplete, or mismatched guest/log pair being
# treated as hardware transfer evidence.

set(SCHWAY__SPU_REFERENCE_MODULE "${CMAKE_CURRENT_LIST_FILE}")

function(schway__spu_reference_verify_file path expected_sha256 label)
  if(NOT EXISTS "${path}")
    message(FATAL_ERROR "SPU_REFERENCE_ASSET_ERROR: ${label} is missing")
  endif()
  file(SHA256 "${path}" actual_sha256)
  if(NOT actual_sha256 STREQUAL "${expected_sha256}")
    message(FATAL_ERROR
      "SPU_REFERENCE_DIGEST_ERROR: ${label}: SHA-256 mismatch (expected ${expected_sha256}, got ${actual_sha256})")
  endif()
endfunction()

function(schway__spu_reference_pin name expected_version output_url output_sha256)
  file(STRINGS "${PROJECT_SOURCE_DIR}/pins.txt" rows REGEX "^${name} ")
  list(LENGTH rows row_count)
  if(NOT row_count EQUAL 1)
    message(FATAL_ERROR "SPU_REFERENCE_PIN_ERROR: pins.txt must contain exactly one ${name} row")
  endif()
  list(GET rows 0 row)
  if(NOT row MATCHES "^${name} ([^ ]+) (https://[^ ]+) ([0-9a-f]+)$")
    message(FATAL_ERROR "SPU_REFERENCE_PIN_ERROR: malformed ${name} row in pins.txt")
  endif()
  set(version "${CMAKE_MATCH_1}")
  set(url "${CMAKE_MATCH_2}")
  set(sha256 "${CMAKE_MATCH_3}")
  string(LENGTH "${sha256}" sha256_length)
  if(NOT version STREQUAL "${expected_version}" OR NOT sha256_length EQUAL 64)
    message(FATAL_ERROR "SPU_REFERENCE_PIN_ERROR: invalid version or SHA-256 for ${name}")
  endif()
  set(${output_url} "${url}" PARENT_SCOPE)
  set(${output_sha256} "${sha256}" PARENT_SCOPE)
endfunction()

function(schway__spu_reference_fetch_pin name expected_version destination output_path output_sha256)
  schway__spu_reference_pin("${name}" "${expected_version}" pin_url pin_sha256)
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
      message(FATAL_ERROR "SPU_REFERENCE_FETCH_ERROR: ${fetch_output}${fetch_error}")
    endif()
  endif()
  schway__spu_reference_verify_file("${path}" "${pin_sha256}" "${name}")
  set(${output_path} "${path}" PARENT_SCOPE)
  set(${output_sha256} "${pin_sha256}" PARENT_SCOPE)
endfunction()

function(schway__spu_reference_validate_source_identity output_exe_sha256)
  file(STRINGS "${PROJECT_SOURCE_DIR}/pins.txt" source_rows
    REGEX "^# ps1-tests-source commit=")
  list(LENGTH source_rows source_count)
  if(NOT source_count EQUAL 1 OR
     NOT source_rows MATCHES "^# ps1-tests-source commit=f727802fead11f1daa7549285548392ef87749cb$")
    message(FATAL_ERROR "SPU_REFERENCE_PIN_ERROR: source commit identity is absent or changed")
  endif()

  file(STRINGS "${PROJECT_SOURCE_DIR}/pins.txt" source_file_rows
    REGEX "^# ps1-tests-source-file path=spu/memory-transfer/(Makefile|main.cpp) ")
  list(LENGTH source_file_rows source_file_count)
  if(NOT source_file_count EQUAL 2)
    message(FATAL_ERROR "SPU_REFERENCE_PIN_ERROR: source file identities are incomplete")
  endif()

  file(STRINGS "${PROJECT_SOURCE_DIR}/pins.txt" executable_rows
    REGEX "^# ps1-tests-release-member path=spu/memory-transfer/memory-transfer[.]exe ")
  list(LENGTH executable_rows executable_count)
  if(NOT executable_count EQUAL 1)
    message(FATAL_ERROR "SPU_REFERENCE_PIN_ERROR: release executable identity is missing")
  endif()
  list(GET executable_rows 0 executable_row)
  if(NOT executable_row MATCHES
     "^# ps1-tests-release-member path=spu/memory-transfer/memory-transfer[.]exe release=build-158 sha256=([0-9a-f]+)$")
    message(FATAL_ERROR "SPU_REFERENCE_PIN_ERROR: malformed release executable identity")
  endif()
  set(exe_sha256 "${CMAKE_MATCH_1}")
  string(LENGTH "${exe_sha256}" exe_sha256_length)
  if(NOT exe_sha256_length EQUAL 64)
    message(FATAL_ERROR "SPU_REFERENCE_PIN_ERROR: invalid executable SHA-256 length")
  endif()
  set(${output_exe_sha256} "${exe_sha256}" PARENT_SCOPE)
endfunction()

function(schway_prepare_spu_reference_assets output_dir output_exe output_log output_exe_sha256 output_log_sha256)
  schway__spu_reference_validate_source_identity(exe_sha256)
  set(reference_dir "${PROJECT_BINARY_DIR}/spu-reference")
  file(MAKE_DIRECTORY "${reference_dir}")

  # The memory-transfer executable is a member of the already pinned build-158 archive.
  schway__spu_reference_fetch_pin(ps1-tests-gpu-captures build-158
    "${reference_dir}/tests.zip" archive archive_sha256)
  file(SIZE "${archive}" archive_size)
  if(NOT archive_size EQUAL 6613816)
    message(FATAL_ERROR "SPU_REFERENCE_ARCHIVE_ERROR: build-158 tests.zip has an unexpected size")
  endif()

  schway__spu_reference_fetch_pin(ps1-tests-spu-memory-transfer-log
    f727802fead11f1daa7549285548392ef87749cb
    "${reference_dir}/source-log" source_log log_sha256)

  file(ARCHIVE_EXTRACT INPUT "${archive}" DESTINATION "${reference_dir}"
    PATTERNS "spu/memory-transfer/memory-transfer.exe" "spu/memory-transfer/psx.log")
  set(exe "${reference_dir}/spu/memory-transfer/memory-transfer.exe")
  set(log "${reference_dir}/spu/memory-transfer/psx.log")
  set(source_log "${reference_dir}/source-log/psx.log")
  schway__spu_reference_verify_file("${exe}" "${exe_sha256}" "memory-transfer executable")
  schway__spu_reference_verify_file("${log}" "${log_sha256}" "release memory-transfer log")
  schway__spu_reference_verify_file("${source_log}" "${log_sha256}" "pinned source memory-transfer log")
  file(SIZE "${exe}" exe_size)
  if(NOT exe_size EQUAL 24576)
    message(FATAL_ERROR "SPU_REFERENCE_ARCHIVE_ERROR: memory-transfer executable has an unexpected size")
  endif()

  set(${output_dir} "${reference_dir}" PARENT_SCOPE)
  set(${output_exe} "${exe}" PARENT_SCOPE)
  set(${output_log} "${log}" PARENT_SCOPE)
  set(${output_exe_sha256} "${exe_sha256}" PARENT_SCOPE)
  set(${output_log_sha256} "${log_sha256}" PARENT_SCOPE)
endfunction()

function(schway_add_spu_reference_target exe_sha256 log_sha256)
  if(TARGET schwaystation-test-spu-reference)
    return()
  endif()
  add_executable(schwaystation-test-spu-reference
    "${PROJECT_SOURCE_DIR}/tests/spu/reference.c"
    "${PROJECT_SOURCE_DIR}/runner/sha256.c")
  target_include_directories(schwaystation-test-spu-reference PRIVATE
    "${PROJECT_SOURCE_DIR}/runner"
    "${PROJECT_SOURCE_DIR}/tests")
  target_compile_definitions(schwaystation-test-spu-reference PRIVATE
    "SCHWAY_SPU_REFERENCE_EXE_SHA256=\"${exe_sha256}\""
    "SCHWAY_SPU_REFERENCE_LOG_SHA256=\"${log_sha256}\"")
  target_link_libraries(schwaystation-test-spu-reference PRIVATE schwaystation::schwaystation)
  set_target_properties(schwaystation-test-spu-reference PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/tests")
  schway_warnings(schwaystation-test-spu-reference)
endfunction()

function(schway__spu_reference_expect_hash_rejection source expected label)
  execute_process(
    COMMAND "${CMAKE_COMMAND}"
      "-DSCHWAY_SPU_REFERENCE_VERIFY_PATH=${source}"
      "-DSCHWAY_SPU_REFERENCE_VERIFY_SHA256=${expected}"
      "-DSCHWAY_SPU_REFERENCE_VERIFY_LABEL=${label}"
      -P "${SCHWAY__SPU_REFERENCE_MODULE}"
    RESULT_VARIABLE child_result
    OUTPUT_VARIABLE child_out
    ERROR_VARIABLE child_err)
  if(child_result STREQUAL "0")
    message(FATAL_ERROR "SPU_REFERENCE_CONTROL_ERROR: ${label} digest mismatch was accepted")
  endif()
  set(child_output "${child_out}${child_err}")
  if(NOT child_output MATCHES "SPU_REFERENCE_DIGEST_ERROR")
    message(FATAL_ERROR "SPU_REFERENCE_CONTROL_ERROR: ${label} failed outside digest validation")
  endif()
endfunction()

function(schway__spu_reference_fetch_controls directory)
  file(MAKE_DIRECTORY "${directory}")
  set(source "${directory}/source.bin")
  set(corrupt_source "${directory}/corrupt.bin")
  file(WRITE "${source}" "pinned SPU reference control\n")
  file(SHA256 "${source}" source_sha256)
  schway__spu_reference_verify_file("${source}" "${source_sha256}" "valid control source")
  schway__spu_reference_expect_hash_rejection(
    "${source}" "0000000000000000000000000000000000000000000000000000000000000000"
    "wrong expected digest")

  file(WRITE "${corrupt_source}" "mutated SPU reference control\n")
  schway__spu_reference_expect_hash_rejection(
    "${corrupt_source}" "${source_sha256}" "mutated source bytes")
endfunction()

if(CMAKE_SCRIPT_MODE_FILE)
  if(DEFINED SCHWAY_SPU_REFERENCE_VERIFY_PATH)
    if(NOT DEFINED SCHWAY_SPU_REFERENCE_VERIFY_SHA256 OR
       NOT DEFINED SCHWAY_SPU_REFERENCE_VERIFY_LABEL)
      message(FATAL_ERROR "SPU_REFERENCE_CONTROL_ERROR: digest control arguments are incomplete")
    endif()
    schway__spu_reference_verify_file(
      "${SCHWAY_SPU_REFERENCE_VERIFY_PATH}"
      "${SCHWAY_SPU_REFERENCE_VERIFY_SHA256}"
      "${SCHWAY_SPU_REFERENCE_VERIFY_LABEL}")
    return()
  endif()
  if(DEFINED SCHWAY_SPU_REFERENCE_CONTROL_DIR)
    schway__spu_reference_fetch_controls("${SCHWAY_SPU_REFERENCE_CONTROL_DIR}")
    return()
  endif()
  message(FATAL_ERROR "SPU_REFERENCE_CONTROL_ERROR: script mode needs a control directory")
endif()
