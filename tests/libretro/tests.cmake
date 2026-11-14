# This file also drives the real runner from libretro.host. Script mode is
# used by that one test so every guest executable goes through the configured
# cross-compiling emulator and its status is checked before host comparison.
if(CMAKE_SCRIPT_MODE_FILE AND DEFINED SCHWAY_LIBRETRO_AUDIO_TEST)
  file(REMOVE "${PCM_FILE}")
  execute_process(
    COMMAND ${EMULATOR} "${RUNNER}" --region "${REGION}" --exe "${FIXTURE}"
      --frames "${FRAMES}" --stop-on mailbox --hash-audio --test-audio-file "${PCM_FILE}"
    RESULT_VARIABLE runner_status OUTPUT_VARIABLE runner_out ERROR_VARIABLE runner_err)
  if(NOT runner_status STREQUAL "0")
    message(FATAL_ERROR "runner PCM capture failed (exit ${runner_status})\n${runner_out}\n${runner_err}")
  endif()
  if(NOT EXISTS "${PCM_FILE}")
    message(FATAL_ERROR "runner reported success without writing PCM bytes")
  endif()
  file(SIZE "${PCM_FILE}" pcm_size)
  math(EXPR pcm_remainder "${pcm_size} % 4")
  if(pcm_size EQUAL 0 OR NOT pcm_remainder EQUAL 0)
    message(FATAL_ERROR "runner PCM file has invalid byte length ${pcm_size}")
  endif()
  file(SHA256 "${PCM_FILE}" pcm_sha256)
  if(NOT runner_out MATCHES "audio sha256 ${pcm_sha256}")
    message(FATAL_ERROR "runner PCM file does not match --hash-audio output\n${runner_out}")
  endif()

  execute_process(
    COMMAND ${EMULATOR} "${HOST}" "${MODULE}" audio-equality-batch "${REGION}"
      "${FIXTURE}" "${PCM_FILE}"
    RESULT_VARIABLE host_status OUTPUT_VARIABLE host_out ERROR_VARIABLE host_err)
  if(NOT host_status STREQUAL "0")
    message(FATAL_ERROR "libretro batch PCM comparison failed (exit ${host_status})\n${host_out}\n${host_err}")
  endif()
  message("${host_out}")
  execute_process(
    COMMAND ${EMULATOR} "${HOST}" "${MODULE}" "${REGION}"
    RESULT_VARIABLE baseline_status OUTPUT_VARIABLE baseline_out ERROR_VARIABLE baseline_err)
  if(NOT baseline_status STREQUAL "0")
    message(FATAL_ERROR "libretro host baseline failed (exit ${baseline_status})\n${baseline_out}\n${baseline_err}")
  endif()
  message("${baseline_out}")
  return()
endif()

# The libretro core loaded in-process by a test host.

# The in-process frontend. It links the library and the shared converter
# itself, runs a reference instance beside the module and compares the
# delivered frame with that instance's conversion byte for byte.
add_executable(schwaystation-test-libretro-host ${CMAKE_CURRENT_LIST_DIR}/host.c)
target_include_directories(schwaystation-test-libretro-host PRIVATE ${PROJECT_SOURCE_DIR}/libretro)
target_link_libraries(schwaystation-test-libretro-host PRIVATE schwaystation::schwaystation
  schwaystation-host ${CMAKE_DL_LIBS})
set_target_properties(schwaystation-test-libretro-host PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-libretro-host)
add_dependencies(schwaystation-test-libretro-host schwaystation_libretro)
add_dependencies(schwaystation-test-libretro-host schwaystation-fixtures)
add_dependencies(schwaystation-test-libretro-host schwaystation-run-test)

# Fault prevented: a generated .info file advertises a functional claim
# without a gating registered inventory row.
schway_add_test(NAME libretro.info.claims
  COMMAND ${CMAKE_COMMAND}
    -DINFO_FILE=${PROJECT_BINARY_DIR}/libretro/schwaystation_libretro.info
    -DINVENTORY=${CMAKE_CURRENT_LIST_DIR}/inventory.txt
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/info_claims.cmake
  LABELS libretro portable TIMEOUT 30)
schway_add_test(NAME libretro.info.claims.self_test
  COMMAND ${CMAKE_COMMAND}
    -DINFO_FILE=${PROJECT_BINARY_DIR}/libretro/schwaystation_libretro.info
    -DINVENTORY=${CMAKE_CURRENT_LIST_DIR}/inventory.txt
    -DCHECKER=${PROJECT_SOURCE_DIR}/tests/cmake/info_claims.cmake
    -DEXPECT_FAIL=${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
    -DWORK=${PROJECT_BINARY_DIR}/tests/libretro/info-claims-self-test
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/info_claims_self_test.cmake
  LABELS libretro portable TIMEOUT 30)

# Fault prevented: the core RetroArch loads shows a frame, rate, geometry or
# audio length other than the library's, needs content to start, loses its
# region option, or resets or restores a state other than the library does.
# ntsc-u answers core options version 2 and takes batch audio; pal answers no
# options version and takes audio pair by pair.
foreach(case IN ITEMS "libretro.host;ntsc-u" "libretro.host.pal;pal")
  list(GET case 0 name)
  list(GET case 1 mode)
  if(name STREQUAL "libretro.host")
    string(REPLACE ";" "$<SEMICOLON>" libretro_audio_emulator "${CMAKE_CROSSCOMPILING_EMULATOR}")
    schway_add_test(NAME ${name}
      COMMAND ${CMAKE_COMMAND} -DSCHWAY_LIBRETRO_AUDIO_TEST=1
        "-DEMULATOR=${libretro_audio_emulator}"
        -DRUNNER=$<TARGET_FILE:schwaystation-run-test>
        -DHOST=$<TARGET_FILE:schwaystation-test-libretro-host>
        -DMODULE=$<TARGET_FILE:schwaystation_libretro>
        -DREGION=${mode} -DFRAMES=610
        -DFIXTURE=${PROJECT_BINARY_DIR}/fixtures/spu_tone.exe
        -DPCM_FILE=${PROJECT_BINARY_DIR}/tests/libretro-audio-ntsc-u.pcm
        -P ${CMAKE_CURRENT_LIST_FILE}
      LABELS libretro portable TIMEOUT 120)
  else()
    schway_add_test(NAME ${name}
      COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
        $<TARGET_FILE:schwaystation_libretro> ${mode}
      LABELS libretro portable TIMEOUT 60)
  endif()
endforeach()

# Fault prevented: a fullpath PS-EXE that cannot run with the built-in stub
# kernel, or whose SIO input never changes the guest's independently known GPU pixel.
schway_add_test(NAME libretro.content.no_bios
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
    $<TARGET_FILE:schwaystation_libretro> content-no-bios ${PROJECT_BINARY_DIR}/fixtures/pad_counter.exe
  LABELS libretro portable TIMEOUT 120)

# Fault prevented: the advertised case-insensitive .psexe extension does not
# load a real fullpath guest through the same BIOS-free adapter path as .exe.
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/tests/libretro-psexe)
schway_add_test(NAME libretro.content.psexe
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
    $<TARGET_FILE:schwaystation_libretro> content-psexe
    ${PROJECT_BINARY_DIR}/fixtures/pad_counter.exe ${PROJECT_BINARY_DIR}/tests/libretro-psexe
  LABELS libretro portable TIMEOUT 120)

# Fault prevented: selected BIOS files silently fall back or replace active
# state on failure, or PS-EXE content accidentally inherits the BIOS option.
schway_add_test(NAME libretro.content.lifecycle
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
    $<TARGET_FILE:schwaystation_libretro> content-lifecycle
    ${PROJECT_BINARY_DIR}/fixtures/pad_counter.exe ${PROJECT_BINARY_DIR}/tests
  LABELS libretro portable TIMEOUT 120)

# Fault prevented: libretro hides SAVE_RAM, writes the frontend-owned first
# card, or fails to load and persist the independent second card.
schway_add_test(NAME libretro.cards.persistence
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
    $<TARGET_FILE:schwaystation_libretro> card-persistence
    ${PROJECT_BINARY_DIR}/fixtures/memcard_roundtrip.exe ${PROJECT_BINARY_DIR}/tests
  LABELS libretro portable TIMEOUT 120)

file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/tests/libretro-cards-controls")
# Fault prevented: unchanged, invalid or failed card2 persistence rewrites a
# previous good image, or missing host save locations stop guest execution.
schway_add_test(NAME libretro.cards.controls
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
    $<TARGET_FILE:schwaystation_libretro> card-controls
    ${PROJECT_BINARY_DIR}/fixtures/memcard_roundtrip.exe
    ${PROJECT_BINARY_DIR}/tests/libretro-cards-controls
  LABELS libretro portable TIMEOUT 120)

# Fault prevented: address-space descriptors omit valid RAM/scratchpad aliases
# or claim holes and adjacent non-memory regions.
schway_add_test(NAME libretro.memory.aliases
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
    $<TARGET_FILE:schwaystation_libretro> memory-aliases
  LABELS libretro portable TIMEOUT 120)

# Fault prevented: reset, unload, reload, deinit or a declined map callback
# leaves stale descriptors or makes direct SYSTEM_RAM access unavailable.
schway_add_test(NAME libretro.memory.boundaries
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
    $<TARGET_FILE:schwaystation_libretro> memory-boundaries
  LABELS libretro portable TIMEOUT 120)

# Fault prevented: a controller layout, axis endpoint, or once-per-frame input latch diverges from guest play.
schway_add_test(NAME libretro.input.controls
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
    $<TARGET_FILE:schwaystation_libretro> input-controls ${PROJECT_BINARY_DIR}/fixtures/pad_counter.exe
  LABELS libretro portable TIMEOUT 120)

# Fault prevented: published input descriptors have wrong ports, IDs,
# descriptions, or do not remain valid through load, run, unload and deinit.
schway_add_test(NAME libretro.input.descriptors
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
    $<TARGET_FILE:schwaystation_libretro> input-descriptors
  LABELS libretro portable TIMEOUT 120)

schway_add_test(NAME libretro.input.movies
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
    $<TARGET_FILE:schwaystation_libretro> input-movies ${PROJECT_BINARY_DIR}/fixtures/pad_counter.exe
  LABELS libretro portable TIMEOUT 240)
# Runs 43271212007 and 43292417084 hit the old bound; serialize this matrix to capture a full host timing.
set_tests_properties(libretro.input.movies PROPERTIES RUN_SERIAL TRUE)

# Fault prevented: absent interfaces, zero motors, reset or unload leave a stale rumble report.
schway_add_test(NAME libretro.rumble.controls
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
    $<TARGET_FILE:schwaystation_libretro> rumble-controls ${PROJECT_BINARY_DIR}/tests/rumble_controls_guest.exe
  LABELS libretro portable TIMEOUT 120)

# Fault prevented: real guest 43h/4Dh/42h transactions fail to reach both host rumble motors.
schway_add_test(NAME libretro.rumble.guest
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
    $<TARGET_FILE:schwaystation_libretro> rumble-guest ${PROJECT_BINARY_DIR}/tests/rumble_guest.exe
  LABELS libretro portable TIMEOUT 120)

# Fault prevented: a native packed-RGB24 frame is dropped instead of converted,
# dimension changes fail to notify, or a rate notification is repeated.
schway_add_test(NAME libretro.video.geometry
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
    $<TARGET_FILE:schwaystation_libretro> video-geometry ${PROJECT_BINARY_DIR}/tests/native_video.exe
  LABELS libretro portable TIMEOUT 120)

schway_add_test(NAME libretro.video.geometry_edges
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
    $<TARGET_FILE:schwaystation_libretro> video-geometry-edges ${PROJECT_BINARY_DIR}/tests/native_video_edges.exe
  LABELS libretro portable TIMEOUT 120)
