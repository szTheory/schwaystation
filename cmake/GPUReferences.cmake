# Shared setup for the pinned GPU reference tests.
#
# Fault prevented: the nightly clipping oracle silently compares against
# synthetic data or an unverified/stale capture instead of the pinned hardware
# executable and PNG.

set(SCHWAY__GPU_REFERENCE_MODULE "${CMAKE_CURRENT_LIST_FILE}")
set(SCHWAY_GPU_REFERENCE_CASES
  clut-cache lines quad rectangles texture-overflow transparency triangle
  uv-interpolation vram-to-vram-overlap)

function(schway__gpu_reference_fetch url destination sha256)
  if(EXISTS "${destination}")
    file(SHA256 "${destination}" current_sha256)
    if("${current_sha256}" STREQUAL "${sha256}")
      return()
    endif()
  endif()
  file(DOWNLOAD "${url}" "${destination}"
    EXPECTED_HASH "SHA256=${sha256}"
    STATUS fetch_status
    INACTIVITY_TIMEOUT 30
    TIMEOUT 120)
  list(GET fetch_status 0 fetch_code)
  if(NOT fetch_code EQUAL 0)
    list(GET fetch_status 1 fetch_message)
    message(FATAL_ERROR "GPU_REFERENCE_FETCH_ERROR: ${fetch_message}")
  endif()
endfunction()

function(schway__gpu_reference_file_url path output)
  file(TO_CMAKE_PATH "${path}" url_path)
  if(url_path MATCHES "^[A-Za-z]:/")
    string(PREPEND url_path "/")
  endif()
  string(REPLACE " " "%20" url_path "${url_path}")
  set(${output} "file://${url_path}" PARENT_SCOPE)
endfunction()

function(schway__gpu_reference_expect_hash_rejection url destination sha256 label)
  execute_process(
    COMMAND "${CMAKE_COMMAND}"
      "-DSCHWAY_GPU_REFERENCE_DOWNLOAD_URL=${url}"
      "-DSCHWAY_GPU_REFERENCE_DOWNLOAD_DESTINATION=${destination}"
      "-DSCHWAY_GPU_REFERENCE_DOWNLOAD_SHA256=${sha256}"
      -P "${SCHWAY__GPU_REFERENCE_MODULE}"
    RESULT_VARIABLE child_result
    OUTPUT_VARIABLE child_out
    ERROR_VARIABLE child_err)
  if(child_result STREQUAL "0")
    message(FATAL_ERROR "GPU_REFERENCE_FETCH_CONTROL_ERROR: ${label} was accepted")
  endif()
  set(child_output "${child_out}${child_err}")
  if(NOT child_output MATCHES "[Hh]ash")
    message(FATAL_ERROR "GPU_REFERENCE_FETCH_CONTROL_ERROR: ${label} failed outside hash validation")
  endif()
endfunction()

function(schway__gpu_reference_fetch_controls directory)
  file(MAKE_DIRECTORY "${directory}")
  set(source "${directory}/source.bin")
  set(corrupt_source "${directory}/corrupt.bin")
  file(WRITE "${source}" "pinned reference fetch control\n")
  file(SHA256 "${source}" source_sha256)
  schway__gpu_reference_file_url("${source}" source_url)

  set(valid_destination "${directory}/valid.bin")
  schway__gpu_reference_fetch("${source_url}" "${valid_destination}" "${source_sha256}")
  file(SHA256 "${valid_destination}" valid_sha256)
  if(NOT valid_sha256 STREQUAL source_sha256)
    message(FATAL_ERROR "GPU_REFERENCE_FETCH_CONTROL_ERROR: valid digest was not preserved")
  endif()

  schway__gpu_reference_expect_hash_rejection(
    "${source_url}" "${directory}/wrong-pin.bin"
    "0000000000000000000000000000000000000000000000000000000000000000"
    "wrong expected digest")

  file(WRITE "${corrupt_source}" "corrupted reference fetch control\n")
  schway__gpu_reference_file_url("${corrupt_source}" corrupt_url)
  schway__gpu_reference_expect_hash_rejection(
    "${corrupt_url}" "${directory}/corrupt-source.bin"
    "${source_sha256}" "mutated source bytes")
endfunction()

if(CMAKE_SCRIPT_MODE_FILE)
  if(DEFINED SCHWAY_GPU_REFERENCE_DOWNLOAD_URL)
    if(NOT DEFINED SCHWAY_GPU_REFERENCE_DOWNLOAD_DESTINATION OR
       NOT DEFINED SCHWAY_GPU_REFERENCE_DOWNLOAD_SHA256)
      message(FATAL_ERROR "GPU_REFERENCE_FETCH_ERROR: download control arguments are incomplete")
    endif()
    schway__gpu_reference_fetch(
      "${SCHWAY_GPU_REFERENCE_DOWNLOAD_URL}"
      "${SCHWAY_GPU_REFERENCE_DOWNLOAD_DESTINATION}"
      "${SCHWAY_GPU_REFERENCE_DOWNLOAD_SHA256}")
    return()
  endif()
  if(DEFINED SCHWAY_GPU_REFERENCE_CONTROL_DIR)
    schway__gpu_reference_fetch_controls("${SCHWAY_GPU_REFERENCE_CONTROL_DIR}")
    return()
  endif()
  message(FATAL_ERROR "GPU_REFERENCE_FETCH_CONTROL_ERROR: script mode needs a control directory")
endif()

function(schway_add_gpu_reference_target)
  if(TARGET schwaystation-test-gpu-reference)
    return()
  endif()
  add_executable(schwaystation-test-gpu-reference
    "${PROJECT_SOURCE_DIR}/tests/gpu/reference.c"
    "${PROJECT_SOURCE_DIR}/runner/png.c"
    "${PROJECT_SOURCE_DIR}/runner/crc32.c")
  target_include_directories(schwaystation-test-gpu-reference PRIVATE
    "${PROJECT_SOURCE_DIR}/src"
    "${PROJECT_SOURCE_DIR}/runner"
    "${PROJECT_SOURCE_DIR}/tests")
  target_link_libraries(schwaystation-test-gpu-reference PRIVATE schwaystation::schwaystation)
  set_target_properties(schwaystation-test-gpu-reference PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/tests")
  schway_warnings(schwaystation-test-gpu-reference)
endfunction()

function(schway_prepare_gpu_reference_log case output_log)
  if(NOT case MATCHES "^(gp0-e1|mask-bit|gpustat)$")
    message(FATAL_ERROR "GPU_REFERENCE_CASE_ERROR: unsupported selected hardware log '${case}'")
  endif()

  set(reference_dir "${PROJECT_BINARY_DIR}/gpu-reference")
  set(log "${reference_dir}/gpu/${case}/psx.log")
  file(MAKE_DIRECTORY "${reference_dir}/gpu/${case}")
  if(case STREQUAL "gpustat")
    file(STRINGS "${PROJECT_SOURCE_DIR}/pins.txt" source_rows
      REGEX "^ps1-tests-gpu-gpustat-log ")
    list(LENGTH source_rows source_count)
    if(NOT source_count EQUAL 1)
      message(FATAL_ERROR "GPU_REFERENCE_FETCH_ERROR: pins.txt must contain one ps1-tests gpustat log row")
    endif()
    list(GET source_rows 0 source_row)
    if(NOT source_row MATCHES "^ps1-tests-gpu-gpustat-log f727802fead1 (https://[^ ]+) ([0-9a-f]+)$")
      message(FATAL_ERROR "GPU_REFERENCE_FETCH_ERROR: malformed pinned gpustat log row")
    endif()
    set(log_url "${CMAKE_MATCH_1}")
    set(log_sha256 "${CMAKE_MATCH_2}")
    string(LENGTH "${log_sha256}" log_sha256_length)
    if(NOT log_sha256_length EQUAL 64)
      message(FATAL_ERROR "GPU_REFERENCE_FETCH_ERROR: invalid pinned gpustat log digest length")
    endif()
    schway__gpu_reference_fetch("${log_url}" "${log}" "${log_sha256}")
  else()
    file(STRINGS "${PROJECT_SOURCE_DIR}/pins.txt" pin_rows
      REGEX "^ps1-tests-gpu-captures ")
    list(LENGTH pin_rows pin_count)
    if(NOT pin_count EQUAL 1)
      message(FATAL_ERROR "GPU_REFERENCE_FETCH_ERROR: pins.txt must contain one ps1-tests GPU archive row")
    endif()
    list(GET pin_rows 0 pin_row)
    if(NOT pin_row MATCHES "^ps1-tests-gpu-captures build-158 (https://[^ ]+) ([0-9a-f]+)$")
      message(FATAL_ERROR "GPU_REFERENCE_FETCH_ERROR: malformed ps1-tests GPU archive pin")
    endif()
    set(archive_url "${CMAKE_MATCH_1}")
    set(archive_sha256 "${CMAKE_MATCH_2}")
    string(LENGTH "${archive_sha256}" archive_sha256_length)
    if(NOT archive_sha256_length EQUAL 64)
      message(FATAL_ERROR "GPU_REFERENCE_FETCH_ERROR: invalid ps1-tests archive digest length")
    endif()
    set(archive "${reference_dir}/ps1-tests-build-158-tests.zip")
    schway__gpu_reference_fetch("${archive_url}" "${archive}" "${archive_sha256}")
    file(ARCHIVE_EXTRACT INPUT "${archive}" DESTINATION "${reference_dir}"
      PATTERNS "gpu/${case}/psx.log")
  endif()

  if(NOT EXISTS "${log}")
    message(FATAL_ERROR "GPU_REFERENCE_ARCHIVE_ERROR: pinned source lacks gpu/${case}/psx.log")
  endif()
  file(SIZE "${log}" log_size)
  if(log_size EQUAL 0 OR log_size GREATER 65536)
    message(FATAL_ERROR "GPU_REFERENCE_ARCHIVE_ERROR: gpu/${case}/psx.log has invalid length")
  endif()
  set(${output_log} "${log}" PARENT_SCOPE)
endfunction()

function(schway_prepare_gpu_reference_capture case output_exe output_png output_golden)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)

  set(allowed_cases clipping ${SCHWAY_GPU_REFERENCE_CASES})
  if(NOT case IN_LIST allowed_cases)
    message(FATAL_ERROR "GPU_REFERENCE_CASE_ERROR: unsupported selected capture '${case}'")
  endif()

  file(STRINGS "${PROJECT_SOURCE_DIR}/pins.txt" pin_rows
    REGEX "^ps1-tests-gpu-captures ")
  list(LENGTH pin_rows pin_count)
  if(NOT pin_count EQUAL 1)
    message(FATAL_ERROR "GPU_REFERENCE_FETCH_ERROR: pins.txt must contain one ps1-tests GPU capture row")
  endif()
  list(GET pin_rows 0 pin_row)
  if(NOT pin_row MATCHES "^ps1-tests-gpu-captures build-158 (https://[^ ]+) ([0-9a-f]+)$")
    message(FATAL_ERROR "GPU_REFERENCE_FETCH_ERROR: malformed ps1-tests GPU capture pin")
  endif()
  set(archive_url "${CMAKE_MATCH_1}")
  set(archive_sha256 "${CMAKE_MATCH_2}")
  string(LENGTH "${archive_sha256}" archive_sha256_length)
  if(NOT archive_sha256_length EQUAL 64)
    message(FATAL_ERROR "GPU_REFERENCE_FETCH_ERROR: invalid ps1-tests archive digest length")
  endif()

  file(STRINGS "${PROJECT_SOURCE_DIR}/tests/gpu/allow.txt" allowance_archive_rows
    REGEX "^# archive-sha256 ")
  list(LENGTH allowance_archive_rows allowance_archive_count)
  if(NOT allowance_archive_count EQUAL 1)
    message(FATAL_ERROR "GPU_REFERENCE_ALLOWANCE_ERROR: allow.txt must name one pinned archive digest")
  endif()
  list(GET allowance_archive_rows 0 allowance_archive_row)
  if(NOT allowance_archive_row MATCHES "^# archive-sha256 ([0-9a-f]+)$")
    message(FATAL_ERROR "GPU_REFERENCE_ALLOWANCE_ERROR: malformed archive digest in allow.txt")
  endif()
  set(allowance_archive_sha256 "${CMAKE_MATCH_1}")
  if(NOT "${allowance_archive_sha256}" STREQUAL "${archive_sha256}")
    message(FATAL_ERROR "GPU_REFERENCE_ALLOWANCE_ERROR: allow.txt is not keyed to the pinned archive")
  endif()

  set(reference_dir "${PROJECT_BINARY_DIR}/gpu-reference")
  set(archive "${reference_dir}/ps1-tests-build-158-tests.zip")
  file(MAKE_DIRECTORY "${reference_dir}")
  schway__gpu_reference_fetch("${archive_url}" "${archive}" "${archive_sha256}")

  file(ARCHIVE_EXTRACT INPUT "${archive}" DESTINATION "${reference_dir}"
    PATTERNS "gpu/${case}/${case}.exe" "gpu/${case}/vram.png")
  set(executable "${reference_dir}/gpu/${case}/${case}.exe")
  set(png "${reference_dir}/gpu/${case}/vram.png")
  if(NOT EXISTS "${executable}" OR NOT EXISTS "${png}")
    message(FATAL_ERROR "GPU_REFERENCE_ARCHIVE_ERROR: pinned archive lacks ${case} inputs")
  endif()

  set(golden "${reference_dir}/${case}-vram.rgb555le")
  execute_process(
    COMMAND "${Python3_EXECUTABLE}"
      "${PROJECT_SOURCE_DIR}/tests/gpu/reference_png.py"
      decode "${png}" "${golden}"
    RESULT_VARIABLE decode_result
    OUTPUT_VARIABLE decode_out
    ERROR_VARIABLE decode_err)
  if(NOT decode_result STREQUAL "0")
    message(FATAL_ERROR "GPU_REFERENCE_PNG_ERROR: ${decode_out}${decode_err}")
  endif()
  file(SIZE "${golden}" golden_size)
  if(NOT golden_size EQUAL 1048576)
    message(FATAL_ERROR "GPU_REFERENCE_PNG_ERROR: canonical ${case} capture has the wrong byte length")
  endif()

  set(${output_exe} "gpu-reference/gpu/${case}/${case}.exe" PARENT_SCOPE)
  set(${output_png} "gpu-reference/gpu/${case}/vram.png" PARENT_SCOPE)
  set(${output_golden} "gpu-reference/${case}-vram.rgb555le" PARENT_SCOPE)
endfunction()
