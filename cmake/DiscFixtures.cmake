function(schway__disc_verify_media directory)
  set(manifest "${directory}/media.sha256")
  if(NOT EXISTS "${manifest}")
    message(FATAL_ERROR "disc fixtures: run artifact lacks media.sha256")
  endif()
  file(STRINGS "${manifest}" rows)
  foreach(name IN ITEMS cdda.wav movie.str xa.xa)
    set(found FALSE)
    set(expected "")
    foreach(row IN LISTS rows)
      if(row MATCHES "^([0-9a-f]+)  ${name}$")
        set(expected "${CMAKE_MATCH_1}")
        set(found TRUE)
      endif()
    endforeach()
    string(LENGTH "${expected}" expected_length)
    if(NOT expected_length EQUAL 64)
      set(found FALSE)
    endif()
    if(NOT found OR NOT EXISTS "${directory}/${name}")
      message(FATAL_ERROR "disc fixtures: run artifact lacks pinned ${name}")
    endif()
    file(SHA256 "${directory}/${name}" actual)
    if(NOT actual STREQUAL expected)
      message(FATAL_ERROR "disc fixtures: run artifact digest mismatch for ${name}")
    endif()
  endforeach()
endfunction()

if(CMAKE_SCRIPT_MODE_FILE)
  foreach(var MEDIA_DIR TEST_ROOT)
    if(NOT DEFINED ${var} OR "${${var}}" STREQUAL "")
      message(FATAL_ERROR "disc fixtures: -D${var} is required")
    endif()
  endforeach()
  file(REMOVE_RECURSE "${TEST_ROOT}")
  file(COPY "${MEDIA_DIR}/" DESTINATION "${TEST_ROOT}")
  file(APPEND "${TEST_ROOT}/xa.xa" "x")
  schway__disc_verify_media("${TEST_ROOT}")
  return()
endif()

# Reproducible disc fixture: all platforms consume the same run-local encodes,
# then independently build the image with the pinned native tool.
set(schway__disc_media_dir "${PROJECT_BINARY_DIR}/fixtures/media")
find_package(Python3 REQUIRED COMPONENTS Interpreter)
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/fixtures")
configure_file("${PROJECT_SOURCE_DIR}/tests/fixtures/disc_boot.xml"
  "${PROJECT_BINARY_DIR}/fixtures/disc_boot.xml" COPYONLY)
configure_file("${PROJECT_SOURCE_DIR}/tests/fixtures/src/disc_boot.cnf"
  "${PROJECT_BINARY_DIR}/fixtures/SYSTEM.CNF" COPYONLY)
configure_file("${PROJECT_SOURCE_DIR}/tests/fixtures/src/disc_boot.sbi"
  "${PROJECT_BINARY_DIR}/fixtures/disc_boot.sbi" COPYONLY)

if(NOT "$ENV{SCHWAYSTATION_DISC_MEDIA_DIR}" STREQUAL "")
  set(schway__disc_media_source "$ENV{SCHWAYSTATION_DISC_MEDIA_DIR}")
  schway__disc_verify_media("${schway__disc_media_source}")
  file(COPY "${schway__disc_media_source}/" DESTINATION "${schway__disc_media_dir}")
else()
  include("${PROJECT_SOURCE_DIR}/cmake/Psxavenc.cmake")
  add_custom_command(OUTPUT "${schway__disc_media_dir}/media.sha256"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${schway__disc_media_dir}"
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tools/gen_disc.py"
      --output-dir "${schway__disc_media_dir}" --encoder "$<TARGET_FILE:schwaystation-psxavenc>"
    DEPENDS schwaystation-psxavenc "${PROJECT_SOURCE_DIR}/tools/gen_disc.py"
    COMMENT "Encode deterministic disc media"
    VERBATIM)
  add_custom_target(schwaystation-disc-media DEPENDS "${schway__disc_media_dir}/media.sha256")
endif()

include("${PROJECT_SOURCE_DIR}/cmake/Mkpsxiso.cmake")
set(schway__disc_bin "${PROJECT_BINARY_DIR}/fixtures/disc_boot.bin")
set(schway__disc_cue "${PROJECT_BINARY_DIR}/fixtures/disc_boot.cue")
add_custom_command(OUTPUT "${schway__disc_bin}"
  BYPRODUCTS "${schway__disc_cue}" "${PROJECT_BINARY_DIR}/fixtures/disc_boot_track02.bin"
  COMMAND "${SCHWAYSTATION_MKPSXISO}" -q -y -o "${schway__disc_bin}" disc_boot.xml
  COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tools/gen_disc.py"
    --split-image "${schway__disc_bin}" --cue "${schway__disc_cue}"
    --cd-da "${schway__disc_media_dir}/cdda.wav"
  WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/fixtures"
  DEPENDS schwaystation-mkpsxiso schwaystation-fixtures
    "${PROJECT_BINARY_DIR}/fixtures/disc_boot.xml"
    "${PROJECT_BINARY_DIR}/fixtures/SYSTEM.CNF"
    "${PROJECT_BINARY_DIR}/fixtures/hello.exe"
    "${schway__disc_media_dir}/media.sha256"
    "${PROJECT_SOURCE_DIR}/tools/gen_disc.py"
  COMMENT "Build the owned disc fixture"
  VERBATIM)
add_custom_target(schwaystation-disc-fixtures ALL DEPENDS "${schway__disc_bin}")
