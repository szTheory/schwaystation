# Fetch and run the pinned ps1-tests MDEC block and status guests.
#
# Fault prevented: unpinned guest output or an unstamped status trace being
# mistaken for exact hardware bytes or a hardware cycle measurement.

set(SCHWAY__MDEC_REFERENCE_MODULE "${CMAKE_CURRENT_LIST_FILE}")

function(schway__mdec_reference_verify_file path expected_sha256 label)
  if(NOT EXISTS "${path}")
    message(FATAL_ERROR "MDEC_REFERENCE_ASSET_ERROR: ${label} is missing")
  endif()
  file(SHA256 "${path}" actual_sha256)
  if(NOT actual_sha256 STREQUAL "${expected_sha256}")
    message(FATAL_ERROR
      "MDEC_REFERENCE_DIGEST_ERROR: ${label}: SHA-256 mismatch (expected ${expected_sha256}, got ${actual_sha256})")
  endif()
endfunction()

function(schway__mdec_reference_pin name expected_version output_url output_sha256)
  file(STRINGS "${PROJECT_SOURCE_DIR}/pins.txt" rows REGEX "^${name} ")
  list(LENGTH rows row_count)
  if(NOT row_count EQUAL 1)
    message(FATAL_ERROR "MDEC_REFERENCE_PIN_ERROR: pins.txt must contain exactly one ${name} row")
  endif()
  list(GET rows 0 row)
  if(NOT row MATCHES "^${name} ([^ ]+) (https://[^ ]+) ([0-9a-f]+)$")
    message(FATAL_ERROR "MDEC_REFERENCE_PIN_ERROR: malformed ${name} row in pins.txt")
  endif()
  set(version "${CMAKE_MATCH_1}")
  set(url "${CMAKE_MATCH_2}")
  set(sha256 "${CMAKE_MATCH_3}")
  string(LENGTH "${sha256}" sha256_length)
  if(NOT version STREQUAL "${expected_version}" OR NOT sha256_length EQUAL 64)
    message(FATAL_ERROR "MDEC_REFERENCE_PIN_ERROR: invalid version or SHA-256 for ${name}")
  endif()
  set(${output_url} "${url}" PARENT_SCOPE)
  set(${output_sha256} "${sha256}" PARENT_SCOPE)
endfunction()

function(schway__mdec_reference_fetch_pin name expected_version destination output_path output_sha256)
  schway__mdec_reference_pin("${name}" "${expected_version}" pin_url pin_sha256)
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
      message(FATAL_ERROR "MDEC_REFERENCE_FETCH_ERROR: ${fetch_output}${fetch_error}")
    endif()
  endif()
  schway__mdec_reference_verify_file("${path}" "${pin_sha256}" "${name}")
  set(${output_path} "${path}" PARENT_SCOPE)
  set(${output_sha256} "${pin_sha256}" PARENT_SCOPE)
endfunction()

function(schway__mdec_reference_member path output_sha256 output_size)
  file(STRINGS "${PROJECT_SOURCE_DIR}/pins.txt" rows
    REGEX "^# ps1-tests-release-member path=${path} ")
  list(LENGTH rows row_count)
  if(NOT row_count EQUAL 1)
    message(FATAL_ERROR "MDEC_REFERENCE_PIN_ERROR: expected one archive member row for ${path}")
  endif()
  list(GET rows 0 row)
  if(NOT row MATCHES "^# ps1-tests-release-member path=${path} release=build-158 sha256=([0-9a-f]+) size=([0-9]+)$")
    message(FATAL_ERROR "MDEC_REFERENCE_PIN_ERROR: malformed archive member row for ${path}")
  endif()
  set(sha256 "${CMAKE_MATCH_1}")
  set(size "${CMAKE_MATCH_2}")
  string(LENGTH "${sha256}" sha256_length)
  if(NOT sha256_length EQUAL 64 OR NOT size MATCHES "^[1-9][0-9]*$")
    message(FATAL_ERROR "MDEC_REFERENCE_PIN_ERROR: invalid archive member digest or size for ${path}")
  endif()
  set(${output_sha256} "${sha256}" PARENT_SCOPE)
  set(${output_size} "${size}" PARENT_SCOPE)
endfunction()

function(schway_prepare_mdec_reference_assets output_dir output_4bit_exe output_4bit_log
         output_8bit_exe output_8bit_log output_status_exe output_status_log
         output_4bit_sha output_8bit_sha output_status_sha output_4bit_log_sha
         output_8bit_log_sha output_status_log_sha)
  file(STRINGS "${PROJECT_SOURCE_DIR}/pins.txt" source_rows
    REGEX "^# ps1-tests-source commit=")
  list(LENGTH source_rows source_count)
  if(NOT source_count EQUAL 1 OR NOT source_rows MATCHES
     "^# ps1-tests-source commit=f727802fead11f1daa7549285548392ef87749cb$")
    message(FATAL_ERROR "MDEC_REFERENCE_PIN_ERROR: source commit identity is absent or changed")
  endif()
  file(STRINGS "${PROJECT_SOURCE_DIR}/pins.txt" source_files
    REGEX "^# ps1-tests-source-file path=mdec/(4bit|8bit|step-by-step-log)/psx[.]log ")
  list(LENGTH source_files source_file_count)
  if(NOT source_file_count EQUAL 3)
    message(FATAL_ERROR "MDEC_REFERENCE_PIN_ERROR: expected three pinned MDEC source logs")
  endif()

  set(reference_dir "${PROJECT_BINARY_DIR}/mdec-reference")
  file(MAKE_DIRECTORY "${reference_dir}")
  schway__mdec_reference_pin(ps1-tests-gpu-captures build-158 archive_url archive_sha256)
  set(shared_archive "${PROJECT_SOURCE_DIR}/build/nightly/gpu-reference/ps1-tests-build-158-tests.zip")
  if(EXISTS "${shared_archive}")
    schway__mdec_reference_verify_file("${shared_archive}" "${archive_sha256}" "build-158 archive")
    set(archive "${shared_archive}")
  else()
    schway__mdec_reference_fetch_pin(ps1-tests-gpu-captures build-158
      "${reference_dir}/archive" archive fetched_archive_sha256)
    set(archive_sha256 "${fetched_archive_sha256}")
  endif()
  file(SIZE "${archive}" archive_size)
  if(NOT archive_size EQUAL 6613816)
    message(FATAL_ERROR "MDEC_REFERENCE_ARCHIVE_ERROR: build-158 tests.zip has an unexpected size")
  endif()

  schway__mdec_reference_member(mdec/4bit/4bit.exe exe_4bit_sha exe_4bit_size)
  schway__mdec_reference_member(mdec/4bit/psx.log log_4bit_sha log_4bit_size)
  schway__mdec_reference_member(mdec/8bit/8bit.exe exe_8bit_sha exe_8bit_size)
  schway__mdec_reference_member(mdec/8bit/psx.log log_8bit_sha log_8bit_size)
  schway__mdec_reference_member(mdec/step-by-step-log/step-by-step-log.exe exe_status_sha exe_status_size)
  schway__mdec_reference_member(mdec/step-by-step-log/psx.log log_status_sha log_status_size)

  schway__mdec_reference_fetch_pin(ps1-tests-mdec-4bit-log f727802fead1
    "${reference_dir}/source-4bit" source_4bit_log source_4bit_sha)
  schway__mdec_reference_fetch_pin(ps1-tests-mdec-8bit-log f727802fead1
    "${reference_dir}/source-8bit" source_8bit_log source_8bit_sha)
  schway__mdec_reference_fetch_pin(ps1-tests-mdec-status-log f727802fead1
    "${reference_dir}/source-status" source_status_log source_status_sha)

  file(ARCHIVE_EXTRACT INPUT "${archive}" DESTINATION "${reference_dir}"
    PATTERNS "mdec/4bit/4bit.exe" "mdec/4bit/psx.log"
             "mdec/8bit/8bit.exe" "mdec/8bit/psx.log"
             "mdec/step-by-step-log/step-by-step-log.exe"
             "mdec/step-by-step-log/psx.log")
  set(exe_4bit "${reference_dir}/mdec/4bit/4bit.exe")
  set(log_4bit "${reference_dir}/mdec/4bit/psx.log")
  set(exe_8bit "${reference_dir}/mdec/8bit/8bit.exe")
  set(log_8bit "${reference_dir}/mdec/8bit/psx.log")
  set(exe_status "${reference_dir}/mdec/step-by-step-log/step-by-step-log.exe")
  set(log_status "${reference_dir}/mdec/step-by-step-log/psx.log")
  schway__mdec_reference_verify_file("${exe_4bit}" "${exe_4bit_sha}" "4-bit guest")
  schway__mdec_reference_verify_file("${log_4bit}" "${log_4bit_sha}" "4-bit hardware output")
  schway__mdec_reference_verify_file("${exe_8bit}" "${exe_8bit_sha}" "8-bit guest")
  schway__mdec_reference_verify_file("${log_8bit}" "${log_8bit_sha}" "8-bit hardware output")
  schway__mdec_reference_verify_file("${exe_status}" "${exe_status_sha}" "status guest")
  schway__mdec_reference_verify_file("${log_status}" "${log_status_sha}" "status hardware log")
  schway__mdec_reference_verify_file("${source_4bit_log}" "${log_4bit_sha}" "pinned 4-bit source log")
  schway__mdec_reference_verify_file("${source_8bit_log}" "${log_8bit_sha}" "pinned 8-bit source log")
  schway__mdec_reference_verify_file("${source_status_log}" "${log_status_sha}" "pinned status source log")
  foreach(member IN ITEMS exe_4bit log_4bit exe_8bit log_8bit exe_status log_status)
    file(SIZE "${${member}}" actual_size)
    if(NOT actual_size EQUAL ${${member}_size})
      message(FATAL_ERROR "MDEC_REFERENCE_ARCHIVE_ERROR: ${member} has an unexpected size")
    endif()
  endforeach()

  set(${output_dir} "${reference_dir}" PARENT_SCOPE)
  set(${output_4bit_exe} "${exe_4bit}" PARENT_SCOPE)
  set(${output_4bit_log} "${log_4bit}" PARENT_SCOPE)
  set(${output_8bit_exe} "${exe_8bit}" PARENT_SCOPE)
  set(${output_8bit_log} "${log_8bit}" PARENT_SCOPE)
  set(${output_status_exe} "${exe_status}" PARENT_SCOPE)
  set(${output_status_log} "${log_status}" PARENT_SCOPE)
  set(${output_4bit_sha} "${exe_4bit_sha}" PARENT_SCOPE)
  set(${output_8bit_sha} "${exe_8bit_sha}" PARENT_SCOPE)
  set(${output_status_sha} "${exe_status_sha}" PARENT_SCOPE)
  set(${output_4bit_log_sha} "${log_4bit_sha}" PARENT_SCOPE)
  set(${output_8bit_log_sha} "${log_8bit_sha}" PARENT_SCOPE)
  set(${output_status_log_sha} "${log_status_sha}" PARENT_SCOPE)
endfunction()

function(schway_add_mdec_reference_target exe_4bit_sha exe_8bit_sha exe_status_sha
         log_4bit_sha log_8bit_sha log_status_sha)
  if(TARGET schwaystation-test-mdec-reference)
    return()
  endif()
  add_executable(schwaystation-test-mdec-reference
    "${PROJECT_SOURCE_DIR}/tests/mdec/reference.c"
    "${PROJECT_SOURCE_DIR}/runner/sha256.c")
  target_include_directories(schwaystation-test-mdec-reference PRIVATE
    "${PROJECT_SOURCE_DIR}/src" "${PROJECT_SOURCE_DIR}/runner" "${PROJECT_SOURCE_DIR}/tests")
  target_compile_definitions(schwaystation-test-mdec-reference PRIVATE
    "SCHWAY_MDEC_4BIT_EXE_SHA256=\"${exe_4bit_sha}\""
    "SCHWAY_MDEC_8BIT_EXE_SHA256=\"${exe_8bit_sha}\""
    "SCHWAY_MDEC_STATUS_EXE_SHA256=\"${exe_status_sha}\""
    "SCHWAY_MDEC_4BIT_LOG_SHA256=\"${log_4bit_sha}\""
    "SCHWAY_MDEC_8BIT_LOG_SHA256=\"${log_8bit_sha}\""
    "SCHWAY_MDEC_STATUS_LOG_SHA256=\"${log_status_sha}\"")
  target_link_libraries(schwaystation-test-mdec-reference PRIVATE schwaystation::schwaystation)
  set_target_properties(schwaystation-test-mdec-reference PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/tests")
  schway_warnings(schwaystation-test-mdec-reference)
endfunction()

function(schway_register_mdec_reference_tests exe_4bit log_4bit exe_8bit log_8bit
         exe_status log_status)
  schway_add_test(NAME mdec.reference.input_preflight
    COMMAND schwaystation-test-mdec-reference input-preflight
      "${PROJECT_BINARY_DIR}/mdec-reference"
    LABELS mdec nightly portable TIMEOUT 10)
  set_tests_properties(mdec.reference.input_preflight PROPERTIES
    FIXTURES_SETUP mdec_reference_input_preflight)

  schway_add_test(NAME mdec.reference.4bit
    COMMAND schwaystation-test-mdec-reference 4bit "${exe_4bit}" "${log_4bit}"
    LABELS mdec nightly portable TIMEOUT 120)
  set_tests_properties(mdec.reference.4bit PROPERTIES
    FIXTURES_REQUIRED mdec_reference_input_preflight)
  schway_add_test(NAME mdec.reference.8bit_regression
    COMMAND schwaystation-test-mdec-reference 8bit-regression "${exe_8bit}" "${log_8bit}"
    LABELS mdec nightly portable TIMEOUT 120)
  set_tests_properties(mdec.reference.8bit_regression PROPERTIES
    FIXTURES_REQUIRED mdec_reference_input_preflight)
  schway_add_test(NAME mdec.reference.status_budget
    COMMAND schwaystation-test-mdec-reference status-budget "${exe_status}" "${log_status}"
    LABELS mdec nightly portable TIMEOUT 10)
  schway_add_test(NAME mdec.reference.coverage_controls
    COMMAND schwaystation-test-mdec-reference coverage-controls
    LABELS mdec nightly portable TIMEOUT 10)
  schway_add_test(NAME mdec.reference.controls
    COMMAND schwaystation-test-mdec-reference controls "${log_4bit}" "${log_8bit}" "${log_status}"
    LABELS mdec nightly portable TIMEOUT 30)
endfunction()

function(schway_prepare_mdec_color_reference_assets output_dir output_exes output_pngs
         output_exe_sha256s output_png_sha256s output_scene_sha256s)
  file(STRINGS "${PROJECT_SOURCE_DIR}/pins.txt" source_rows
    REGEX "^# ps1-tests-source commit=")
  list(LENGTH source_rows source_count)
  if(NOT source_count EQUAL 1 OR NOT source_rows MATCHES
     "^# ps1-tests-source commit=f727802fead11f1daa7549285548392ef87749cb$")
    message(FATAL_ERROR "MDEC_COLOR_REFERENCE_PIN_ERROR: source commit identity is absent or changed")
  endif()

  set(reference_dir "${PROJECT_BINARY_DIR}/mdec-reference/color")
  file(MAKE_DIRECTORY "${reference_dir}/canonical")
  schway__mdec_reference_pin(ps1-tests-gpu-captures build-158 archive_url archive_sha256)
  set(shared_archive "${PROJECT_SOURCE_DIR}/build/nightly/gpu-reference/ps1-tests-build-158-tests.zip")
  if(EXISTS "${shared_archive}")
    schway__mdec_reference_verify_file("${shared_archive}" "${archive_sha256}" "build-158 archive")
    set(archive "${shared_archive}")
  else()
    schway__mdec_reference_fetch_pin(ps1-tests-gpu-captures build-158
      "${reference_dir}/archive" archive ignored_archive_sha256)
  endif()
  if(NOT archive)
    set(archive "${reference_dir}/archive/tests.zip")
  endif()
  file(SIZE "${archive}" archive_size)
  if(NOT archive_size EQUAL 6613816)
    message(FATAL_ERROR "MDEC_COLOR_REFERENCE_ARCHIVE_ERROR: build-158 tests.zip has an unexpected size")
  endif()

  set(members
    mdec/frame/frame-15bit.exe
    mdec/frame/frame-24bit.exe
    mdec/movie/movie-15bit.exe
    mdec/movie/movie-24bit.exe
    mdec/frame/vram-15bit.png
    mdec/frame/vram-24bit.png
    mdec/movie/vram-15bit.png
    mdec/movie/vram-24bit.png)
  set(member_sha256s "")
  set(member_sizes "")
  foreach(member IN LISTS members)
    schway__mdec_reference_member("${member}" member_sha256 member_size)
    list(APPEND member_sha256s "${member_sha256}")
    list(APPEND member_sizes "${member_size}")
  endforeach()
  file(ARCHIVE_EXTRACT INPUT "${archive}" DESTINATION "${reference_dir}"
    PATTERNS ${members})

  set(exe_relpaths
    mdec/frame/frame-15bit.exe
    mdec/frame/frame-24bit.exe
    mdec/movie/movie-15bit.exe
    mdec/movie/movie-24bit.exe)
  set(png_relpaths
    mdec/frame/vram-15bit.png
    mdec/frame/vram-24bit.png
    mdec/movie/vram-15bit.png
    mdec/movie/vram-24bit.png)
  set(exes "")
  set(exe_sha256s "")
  foreach(index RANGE 0 3)
    list(GET exe_relpaths ${index} relpath)
    list(GET member_sha256s ${index} expected_sha256)
    list(GET member_sizes ${index} expected_size)
    set(path "${reference_dir}/${relpath}")
    schway__mdec_reference_verify_file("${path}" "${expected_sha256}" "${relpath}")
    file(SIZE "${path}" actual_size)
    if(NOT actual_size EQUAL "${expected_size}")
      message(FATAL_ERROR "MDEC_COLOR_REFERENCE_ARCHIVE_ERROR: ${relpath} has an unexpected size")
    endif()
    list(APPEND exes "${path}")
    list(APPEND exe_sha256s "${expected_sha256}")
  endforeach()

  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  set(pngs "")
  set(png_sha256s "")
  foreach(index RANGE 0 3)
    list(GET png_relpaths ${index} relpath)
    math(EXPR member_index "${index} + 4")
    list(GET member_sha256s ${member_index} expected_sha256)
    list(GET member_sizes ${member_index} expected_size)
    set(png_path "${reference_dir}/${relpath}")
    schway__mdec_reference_verify_file("${png_path}" "${expected_sha256}" "${relpath}")
    file(SIZE "${png_path}" actual_size)
    if(NOT actual_size EQUAL "${expected_size}")
      message(FATAL_ERROR "MDEC_COLOR_REFERENCE_ARCHIVE_ERROR: ${relpath} has an unexpected size")
    endif()
    string(REPLACE "/" "-" canonical_name "${relpath}")
    string(REPLACE ".png" ".rgb555le" canonical_name "${canonical_name}")
    set(canonical_path "${reference_dir}/canonical/${canonical_name}")
    execute_process(
      COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/gpu/reference_png.py"
        decode "${png_path}" "${canonical_path}"
      RESULT_VARIABLE decode_result
      OUTPUT_VARIABLE decode_output
      ERROR_VARIABLE decode_error)
    if(NOT decode_result STREQUAL "0")
      message(FATAL_ERROR "MDEC_COLOR_REFERENCE_PNG_ERROR: ${relpath}: ${decode_output}${decode_error}")
    endif()
    file(SHA256 "${canonical_path}" canonical_sha256)
    list(APPEND pngs "${canonical_path}")
    list(APPEND png_sha256s "${canonical_sha256}")
  endforeach()
  list(GET png_sha256s 0 frame_scene_sha256)
  list(GET png_sha256s 2 movie_scene_sha256)
  set(scene_sha256s "${frame_scene_sha256};${frame_scene_sha256};${movie_scene_sha256};${movie_scene_sha256}")

  set(${output_dir} "${reference_dir}" PARENT_SCOPE)
  set(${output_exes} "${exes}" PARENT_SCOPE)
  set(${output_pngs} "${pngs}" PARENT_SCOPE)
  set(${output_exe_sha256s} "${exe_sha256s}" PARENT_SCOPE)
  set(${output_png_sha256s} "${png_sha256s}" PARENT_SCOPE)
  set(${output_scene_sha256s} "${scene_sha256s}" PARENT_SCOPE)
endfunction()

function(schway_add_mdec_color_reference_support exe_sha256s png_sha256s scene_sha256s)
  if(NOT TARGET schwaystation-test-mdec-reference)
    message(FATAL_ERROR "MDEC_COLOR_REFERENCE_TARGET_ERROR: create the MDEC reference runner first")
  endif()
  target_sources(schwaystation-test-mdec-reference PRIVATE
    "${PROJECT_SOURCE_DIR}/tests/mdec/color_reference.c")
  set(cases frame15 frame24 movie15 movie24)
  foreach(index RANGE 0 3)
    list(GET exe_sha256s ${index} exe_sha256)
    list(GET png_sha256s ${index} png_sha256)
    list(GET scene_sha256s ${index} scene_sha256)
    list(GET cases ${index} case)
    string(TOUPPER "${case}" case_upper)
    target_compile_definitions(schwaystation-test-mdec-reference PRIVATE
      "SCHWAY_MDEC_${case_upper}_EXE_SHA256=\"${exe_sha256}\"")
    target_compile_definitions(schwaystation-test-mdec-reference PRIVATE
      "SCHWAY_MDEC_${case_upper}_VRAM_SHA256=\"${png_sha256}\"")
    if(case STREQUAL "frame15")
      target_compile_definitions(schwaystation-test-mdec-reference PRIVATE
        "SCHWAY_MDEC_FRAME_SCENE_SHA256=\"${scene_sha256}\"")
    elseif(case STREQUAL "movie15")
      target_compile_definitions(schwaystation-test-mdec-reference PRIVATE
        "SCHWAY_MDEC_MOVIE_SCENE_SHA256=\"${scene_sha256}\"")
    endif()
  endforeach()
endfunction()

function(schway_register_mdec_color_reference_tests reference_dir exes pngs)
  if(NOT TARGET schwaystation-test-mdec-reference)
    message(FATAL_ERROR "MDEC_COLOR_REFERENCE_TARGET_ERROR: create the MDEC reference runner first")
  endif()
  set(asset_args "")
  list(APPEND asset_args ${exes} ${pngs})
  schway_add_test(NAME mdec.reference.color_assets
    COMMAND schwaystation-test-mdec-reference color-assets ${asset_args}
    LABELS mdec nightly portable TIMEOUT 20)
  set_tests_properties(mdec.reference.color_assets PROPERTIES
    FIXTURES_SETUP mdec_color_reference_assets)
  schway_add_test(NAME mdec.reference.color_controls
    COMMAND schwaystation-test-mdec-reference color-controls
    LABELS mdec nightly portable TIMEOUT 10)

  set(cases frame15 frame24 movie15 movie24)
  set(scene_pngs "")
  list(GET pngs 0 frame_scene)
  list(GET pngs 2 movie_scene)
  list(APPEND scene_pngs "${frame_scene}" "${frame_scene}" "${movie_scene}" "${movie_scene}")
  foreach(index RANGE 0 3)
    list(GET cases ${index} case)
    list(GET exes ${index} exe)
    list(GET pngs ${index} vram_png)
    list(GET scene_pngs ${index} scene_png)
    schway_add_test(NAME mdec.reference.coverage_${case}
      COMMAND schwaystation-test-mdec-reference color-coverage ${case} "${exe}"
      LABELS mdec nightly portable TIMEOUT 10)
    set_tests_properties(mdec.reference.coverage_${case} PROPERTIES
      FIXTURES_REQUIRED "mdec_color_reference_assets;mdec_reference_input_preflight")
  endforeach()
endfunction()
