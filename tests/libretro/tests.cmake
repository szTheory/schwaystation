# This file also drives the runner and libretro host for XA/CD-DA PCM parity.
if(CMAKE_SCRIPT_MODE_FILE AND DEFINED SCHWAY_LIBRETRO_DISC_AUDIO_TEST)
  file(MAKE_DIRECTORY "${TEST_DIR}")
  foreach(region IN ITEMS ntsc-u pal)
    set(pcm_file "${TEST_DIR}/libretro-disc-audio-${region}.pcm")
    file(REMOVE "${pcm_file}")
    execute_process(
      COMMAND ${EMULATOR} "${RUNNER}" --region "${region}" --bios "${BIOS}"
        --disc "${PLAYLIST}" --frames 1200 --stop-on mailbox --hash-audio
        --test-audio-file "${pcm_file}"
      RESULT_VARIABLE runner_status OUTPUT_VARIABLE runner_out ERROR_VARIABLE runner_err)
    if(NOT runner_status STREQUAL "0" OR NOT runner_out MATCHES "mailbox SWMB status=00")
      message(FATAL_ERROR "${region} runner disc audio capture failed (exit ${runner_status})\n${runner_out}\n${runner_err}")
    endif()
    if(NOT EXISTS "${pcm_file}")
      message(FATAL_ERROR "${region} runner reported success without writing disc PCM bytes")
    endif()
    file(SIZE "${pcm_file}" pcm_size)
    math(EXPR pcm_remainder "${pcm_size} % 4")
    if(pcm_size EQUAL 0 OR NOT pcm_remainder EQUAL 0)
      message(FATAL_ERROR "${region} runner disc PCM has invalid byte length ${pcm_size}")
    endif()
    file(SHA256 "${pcm_file}" pcm_sha256)
    if(NOT runner_out MATCHES "audio sha256 ${pcm_sha256}")
      message(FATAL_ERROR "${region} runner disc PCM differs from its audio hash\n${runner_out}")
    endif()

    foreach(callback IN ITEMS batch single)
      execute_process(
        COMMAND ${EMULATOR} "${HOST}" "${MODULE}"
          "disc-audio-equality-${callback}" "${region}" "${PLAYLIST}" "${pcm_file}"
          "${BIOS_DIRECTORY}"
        RESULT_VARIABLE host_status OUTPUT_VARIABLE host_out ERROR_VARIABLE host_err)
      if(NOT host_status STREQUAL "0")
        message(FATAL_ERROR "libretro ${region} ${callback} disc PCM comparison failed (exit ${host_status})\n${host_out}\n${host_err}")
      endif()
      message("${host_out}")
    endforeach()
  endforeach()
  return()
endif()

# This file also drives the real runner from libretro.host. Script mode is
# used by that one test so every guest executable goes through the configured
# cross-compiling emulator and its status is checked before host comparison.
if(CMAKE_SCRIPT_MODE_FILE AND DEFINED SCHWAY_LIBRETRO_AUDIO_TEST)
  file(MAKE_DIRECTORY "${TEST_DIR}")
  foreach(region IN ITEMS ntsc-j ntsc-u pal)
    set(pcm_file "${TEST_DIR}/libretro-audio-${region}.pcm")
    file(REMOVE "${pcm_file}")
    execute_process(
      COMMAND ${EMULATOR} "${RUNNER}" --region "${region}" --exe "${FIXTURE}"
        --frames "${FRAMES}" --stop-on mailbox --hash-audio --test-audio-file "${pcm_file}"
      RESULT_VARIABLE runner_status OUTPUT_VARIABLE runner_out ERROR_VARIABLE runner_err)
    if(NOT runner_status STREQUAL "0")
      message(FATAL_ERROR "${region} runner PCM capture failed (exit ${runner_status})\n${runner_out}\n${runner_err}")
    endif()
    if(NOT EXISTS "${pcm_file}")
      message(FATAL_ERROR "${region} runner reported success without writing PCM bytes")
    endif()
    file(SIZE "${pcm_file}" pcm_size)
    math(EXPR pcm_remainder "${pcm_size} % 4")
    if(pcm_size EQUAL 0 OR NOT pcm_remainder EQUAL 0)
      message(FATAL_ERROR "${region} runner PCM has invalid byte length ${pcm_size}")
    endif()
    file(SHA256 "${pcm_file}" pcm_sha256)
    if(NOT runner_out MATCHES "audio sha256 ${pcm_sha256}")
      message(FATAL_ERROR "${region} runner PCM does not match --hash-audio output\n${runner_out}")
    endif()

    foreach(callback IN ITEMS batch single)
      execute_process(
        COMMAND ${EMULATOR} "${HOST}" "${MODULE}" "audio-equality-${callback}" "${region}"
          "${FIXTURE}" "${pcm_file}"
        RESULT_VARIABLE host_status OUTPUT_VARIABLE host_out ERROR_VARIABLE host_err)
      if(NOT host_status STREQUAL "0")
        message(FATAL_ERROR "libretro ${region} ${callback} PCM comparison failed (exit ${host_status})\n${host_out}\n${host_err}")
      endif()
      message("${host_out}")
    endforeach()
  endforeach()

  execute_process(
    COMMAND ${EMULATOR} "${HOST}" "${MODULE}" "${REGION}"
    RESULT_VARIABLE baseline_status OUTPUT_VARIABLE baseline_out ERROR_VARIABLE baseline_err)
  if(NOT baseline_status STREQUAL "0")
    message(FATAL_ERROR "libretro host baseline failed (exit ${baseline_status})\n${baseline_out}\n${baseline_err}")
  endif()
  message("${baseline_out}")
  return()
endif()

# Tests that inspect card 2 use a separate, uninstalled module.
add_library(schwaystation-libretro-card-test MODULE
  ${PROJECT_SOURCE_DIR}/libretro/libretro.c)
target_link_libraries(schwaystation-libretro-card-test PRIVATE
  schwaystation::schwaystation schwaystation-host schwaystation_disc_image)
target_compile_definitions(schwaystation-libretro-card-test PRIVATE
  SCHWAYSTATION_LIBRETRO_TESTING
  SCHWAYSTATION_LIBRETRO_VERSION="${PROJECT_VERSION}"
  SCHWAYSTATION_CHD=$<BOOL:${SCHWAYSTATION_CHD}>)
set_target_properties(schwaystation-libretro-card-test PROPERTIES C_VISIBILITY_PRESET hidden)
schway_warnings(schwaystation-libretro-card-test)

# The in-process frontend. It links the library and the shared converter
# itself, runs a reference instance beside the module and compares the
# delivered frame with that instance's conversion byte for byte.
add_executable(schwaystation-test-libretro-host ${CMAKE_CURRENT_LIST_DIR}/host.c)
target_include_directories(schwaystation-test-libretro-host PRIVATE ${PROJECT_SOURCE_DIR}/libretro)
target_link_libraries(schwaystation-test-libretro-host PRIVATE schwaystation::schwaystation
  schwaystation-host schwaystation_disc_image ${CMAKE_DL_LIBS})
target_compile_definitions(schwaystation-test-libretro-host PRIVATE
  SCHWAYSTATION_CHD=$<BOOL:${SCHWAYSTATION_CHD}>)
set_target_properties(schwaystation-test-libretro-host PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-libretro-host)
add_dependencies(schwaystation-test-libretro-host schwaystation_libretro schwaystation-libretro-card-test)
add_dependencies(schwaystation-test-libretro-host schwaystation-fixtures)
add_dependencies(schwaystation-test-libretro-host schwaystation-run-test)
if(TARGET schwaystation-disc-fixtures)
  add_dependencies(schwaystation-test-libretro-host schwaystation-disc-fixtures)
endif()

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
# The host integration checks both callback modes against runner PCM for each
# region. The separate PAL baseline retains the legacy no-options check.
foreach(case IN ITEMS "libretro.host;ntsc-u" "libretro.host.pal;pal")
  list(GET case 0 name)
  list(GET case 1 mode)
  if(name STREQUAL "libretro.host")
    string(REPLACE ";" "$<SEMICOLON>" libretro_audio_emulator "${CMAKE_CROSSCOMPILING_EMULATOR}")
    # run 43672826908: macOS x64 hit 240 s; run 43665013436 passed in 219.73 s. Raise to 360 s.
    schway_add_test(NAME ${name}
      COMMAND ${CMAKE_COMMAND} -DSCHWAY_LIBRETRO_AUDIO_TEST=1
        "-DEMULATOR=${libretro_audio_emulator}"
        -DRUNNER=$<TARGET_FILE:schwaystation-run-test>
        -DHOST=$<TARGET_FILE:schwaystation-test-libretro-host>
        -DMODULE=$<TARGET_FILE:schwaystation_libretro>
        -DREGION=${mode} -DFRAMES=610
        -DFIXTURE=${PROJECT_BINARY_DIR}/fixtures/spu_tone.exe
        -DTEST_DIR=${PROJECT_BINARY_DIR}/tests
        -P ${CMAKE_CURRENT_LIST_FILE}
      LABELS libretro portable TIMEOUT 360)
  else()
    schway_add_test(NAME ${name}
      COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
        $<TARGET_FILE:schwaystation_libretro> ${mode}
      LABELS libretro portable TIMEOUT 60)
  endif()
endforeach()

# Fault prevented: disk callbacks report success without changing the live
# guest's CD-ROM interrupt and subsequent TOC responses, or a playlist/member
# swap changes card 2's original frontend content path.
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/tests/libretro-disc-control)
schway_add_test(NAME libretro.host.disc_control
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
    $<TARGET_FILE:schwaystation-libretro-card-test> disc-control
    ${PROJECT_BINARY_DIR}/fixtures/disc_boot_runner.m3u
    ${PROJECT_BINARY_DIR}/fixtures/disc_boot_playlist.cue
    ${PROJECT_BINARY_DIR}/openbios ${PROJECT_BINARY_DIR}/tests/libretro-disc-control
  LABELS libretro portable TIMEOUT 240)

# Fault prevented: the libretro core changes disc XA/CD-DA samples, callback
# batching, or NTSC/PAL frame cadence relative to the runner.
string(REPLACE ";" "$<SEMICOLON>" libretro_disc_audio_emulator
  "${CMAKE_CROSSCOMPILING_EMULATOR}")
schway_add_test(NAME libretro.host.disc_audio
  COMMAND ${CMAKE_COMMAND} -DSCHWAY_LIBRETRO_DISC_AUDIO_TEST=1
    "-DEMULATOR=${libretro_disc_audio_emulator}"
    -DRUNNER=$<TARGET_FILE:schwaystation-run-test>
    -DHOST=$<TARGET_FILE:schwaystation-test-libretro-host>
    -DMODULE=$<TARGET_FILE:schwaystation_libretro>
    -DPLAYLIST=${PROJECT_BINARY_DIR}/fixtures/disc_boot_runner.m3u
    -DBIOS=${PROJECT_BINARY_DIR}/openbios/openbios-fast.bin
    -DBIOS_DIRECTORY=${PROJECT_BINARY_DIR}/openbios
    -DTEST_DIR=${PROJECT_BINARY_DIR}/tests
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS libretro portable TIMEOUT 240)

# These Plan 19 lanes are registered now so the exact inventory is guarded
# before the checker is implemented by its first caller.
schway_add_test(NAME libretro.vendored
  COMMAND ${CMAKE_COMMAND} -DCASE=check -DROOT=${PROJECT_SOURCE_DIR}
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/vendored.cmake
  LABELS libretro portable TIMEOUT 30)
schway_add_test(NAME libretro.vendored.self_test
  COMMAND ${CMAKE_COMMAND} -DCASE=self_test -DROOT=${PROJECT_SOURCE_DIR}
    -DWORK=${PROJECT_BINARY_DIR}/tests/libretro/vendored-self-test
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/vendored.cmake
  LABELS libretro portable TIMEOUT 30)

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

file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/tests/libretro-content-chd)
file(WRITE ${PROJECT_BINARY_DIR}/tests/libretro-content-chd/invalid.chd "not a CHD image\n")
# Fault prevented: CHD is omitted from the frontend path or extension contract,
# its sector callbacks differ from CUE, or a rejected replacement loses media.
schway_add_test(NAME libretro.content.chd
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
    $<TARGET_FILE:schwaystation-libretro-card-test> content-chd
    $<BOOL:${SCHWAYSTATION_CHD}>
    ${PROJECT_BINARY_DIR}/fixtures/disc_boot.chd
    ${PROJECT_BINARY_DIR}/fixtures/disc_boot.cue
    ${PROJECT_BINARY_DIR}/tests/libretro-content-chd/invalid.chd
    ${PROJECT_BINARY_DIR}/openbios ${PROJECT_BINARY_DIR}/tests/libretro-content-chd
  LABELS libretro portable TIMEOUT 180)
set_tests_properties(libretro.content.chd PROPERTIES
  FIXTURES_REQUIRED "disc_media_preflight;compat_features_preflight")

# Fault prevented: libretro bypasses either shared legacy provider, changes bytes or
# TOC, loses the active image after a rejected replacement, or breaks disk swap.
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/tests/libretro-content-legacy)
file(WRITE ${PROJECT_BINARY_DIR}/tests/libretro-content-legacy/invalid.ecm "not an ECM image\n")
file(WRITE ${PROJECT_BINARY_DIR}/tests/libretro-content-legacy/invalid.ccd "not a CCD image\n")
schway_add_test(NAME libretro.content.legacy
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
    $<TARGET_FILE:schwaystation_libretro> content-legacy $<BOOL:${SCHWAYSTATION_CHD}>
    ${PROJECT_BINARY_DIR}/fixtures/disc_boot_legacy_literal.ecm
    ${PROJECT_BINARY_DIR}/fixtures/disc_boot_legacy_source.cue
    ${PROJECT_BINARY_DIR}/fixtures/disc_boot_legacy.ccd
    ${PROJECT_BINARY_DIR}/fixtures/disc_boot_legacy_image.cue
    ${PROJECT_BINARY_DIR}/tests/libretro-content-legacy/invalid.ecm
    ${PROJECT_BINARY_DIR}/tests/libretro-content-legacy/invalid.ccd
    ${PROJECT_BINARY_DIR}/openbios ${PROJECT_BINARY_DIR}/tests/libretro-content-legacy
  LABELS libretro portable TIMEOUT 120)
set_tests_properties(libretro.content.legacy PROPERTIES
  FIXTURES_REQUIRED "disc_legacy_preflight;disc_legacy_parity;compat_features_preflight")

file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/tests/libretro-content-pbp)
file(WRITE ${PROJECT_BINARY_DIR}/tests/libretro-content-pbp/invalid.pbp "not a PBP image\n")
# Fault prevented: libretro bypasses the shared PBP provider, changes media or
# cards after a rejected image, or diverges from the owned single-track CUE.
schway_add_test(NAME libretro.content.pbp
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
    $<TARGET_FILE:schwaystation-libretro-card-test> content-pbp
    $<BOOL:${SCHWAYSTATION_CHD}>
    ${PROJECT_BINARY_DIR}/fixtures/schwaystation-disc_boot.pbp
    ${PROJECT_BINARY_DIR}/fixtures/disc_boot_pbp.cue
    ${PROJECT_BINARY_DIR}/tests/libretro-content-pbp/invalid.pbp
    ${PROJECT_BINARY_DIR}/openbios ${PROJECT_BINARY_DIR}/tests/libretro-content-pbp
  LABELS libretro portable TIMEOUT 60)
set_tests_properties(libretro.content.pbp PROPERTIES
  FIXTURES_REQUIRED "disc_pbp_preflight;compat_features_preflight")

# Fault prevented: libretro hides SAVE_RAM, writes the frontend-owned first
# card, or fails to load and persist the independent second card.
schway_add_test(NAME libretro.cards.persistence
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
    $<TARGET_FILE:schwaystation-libretro-card-test> card-persistence
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
