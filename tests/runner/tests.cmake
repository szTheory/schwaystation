# schwaystation-run: frame hashes, PNG output, bench report, usage and exit statuses, build revision.
#
# Run as a script (cmake -DCASE=<case> ... -P tests.cmake), this file is one
# runner check; include()d by tests/tests.cmake, it registers the tests.
if(CMAKE_SCRIPT_MODE_FILE)
  # Runs the runner with ARGN and fails unless it exits with code and its
  # merged output matches regex.
  function(expect_exit code regex)
    execute_process(COMMAND ${EMULATOR} ${RUN} ${ARGN} RESULT_VARIABLE rc
      OUTPUT_VARIABLE out ERROR_VARIABLE out)
    if(NOT rc STREQUAL code OR NOT out MATCHES "${regex}")
      message(FATAL_ERROR "schwaystation-run ${ARGN}: expected exit ${code} and output matching\n"
        "  ${regex}\ngot exit ${rc} and output\n${out}")
    endif()
  endfunction()

  if(CASE STREQUAL "png_file")
    # The decode check runs first, so the pinned digest is only ever
    # compared for a file whose IHDR and every pixel are already known good.
    file(REMOVE "${OUT}")
    expect_exit(0 "^$" --frames 5 --png "5:${OUT}")
    execute_process(COMMAND ${EMULATOR} ${DECODE} png_file "${OUT}" RESULT_VARIABLE rc
      OUTPUT_VARIABLE out ERROR_VARIABLE out)
    if(NOT rc STREQUAL "0")
      message(FATAL_ERROR "the PNG does not decode to the card (exit ${rc})\n${out}")
    endif()
    file(SHA256 "${OUT}" sha256)
    if(NOT sha256 STREQUAL EXPECTED)
      message(FATAL_ERROR "PNG SHA-256 mismatch: expected ${EXPECTED} got ${sha256}")
    endif()
  else()
    message(FATAL_ERROR "unknown CASE '${CASE}'")
  endif()
  return()
endif()

# Every command that starts the runner goes through the build's
# CMAKE_CROSSCOMPILING_EMULATOR, so an emulated lane runs its own guest binary.
set(runner_cmd ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-run>)

# Fault prevented: the runner's printed card hash, the line hosts and CI
# compare, drifts from the pinned card or differs on another platform.
schway_add_test(NAME runner.hash
  COMMAND ${runner_cmd} --frames 60 --hash
  PASS_REGEX "frame 60 sha256 ${SCHWAYSTATION_TESTCARD_SHA256}"
  LABELS portable runner TIMEOUT 30)

# Fault prevented: a card that does not move, so the frame hash cannot tell
# frame 5 from frame 60.
schway_add_test(NAME runner.hash.frame5
  COMMAND ${runner_cmd} --frames 5 --hash
  PASS_REGEX "frame 5 sha256 ${SCHWAYSTATION_TESTCARD_SHA256_FRAME5}"
  LABELS portable runner TIMEOUT 30)

# Fault prevented: --region not reaching the frame the host sees.
schway_add_test(NAME runner.hash.ntsc_j
  COMMAND ${runner_cmd} --frames 60 --hash --region ntsc-j
  PASS_REGEX "frame 60 sha256 ${SCHWAYSTATION_TESTCARD_SHA256_NTSC_J}"
  LABELS portable runner TIMEOUT 30)
schway_add_test(NAME runner.hash.pal
  COMMAND ${runner_cmd} --frames 60 --hash --region pal
  PASS_REGEX "frame 60 sha256 ${SCHWAYSTATION_TESTCARD_SHA256_PAL}"
  LABELS portable runner TIMEOUT 30)

# Fault prevented: a lane whose hashes are never compared with the others.
# Writes build/<preset>/hashes.txt, the file CI collects from every lane and
# compares byte for byte.
string(REPLACE ";" "$<SEMICOLON>" runner_emulator "${CMAKE_CROSSCOMPILING_EMULATOR}")
set(runner_hashes ${PROJECT_SOURCE_DIR}/tests/cmake/hashes.cmake)
schway_add_test(NAME runner.hashes_file
  COMMAND ${CMAKE_COMMAND} -DMODE=write -DRUN=$<TARGET_FILE:schwaystation-run>
    "-DEMULATOR=${runner_emulator}" -DOUT=${PROJECT_BINARY_DIR} -P ${runner_hashes}
  LABELS portable runner TIMEOUT 60)

# Fault prevented: a comparison that passes lanes which differ, or a lane
# that silently drops out. Two files one hex digit apart must fail, and two
# files where three are required must fail.
set(runner_fixture ${PROJECT_BINARY_DIR}/tests/hashes)
file(WRITE ${runner_fixture}/a.txt "frame 60 sha256 00\n")
file(WRITE ${runner_fixture}/b.txt "frame 60 sha256 01\n")
# CMake wraps long messages, so the regexes allow a break between words.
foreach(case IN ITEMS "count;3;a;expected 3 files,[^/]*got 2" "byte;2;b;b.txt[^/]*differs")
  list(GET case 0 name)
  list(GET case 1 count)
  list(GET case 2 second)
  list(GET case 3 regex)
  set(cmd ${CMAKE_COMMAND} "-DFILES=${runner_fixture}/a.txt\;${runner_fixture}/${second}.txt"
    -DCOUNT=${count} -DMODE=compare -P ${runner_hashes})
  string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
  schway_add_test(NAME runner.hashes_compare.${name}.self_test
    COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=${regex}"
      -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
    LABELS runner TIMEOUT 10)
endforeach()

# The PNG encoder and its checksums, checked by an independent parse, and the
# shared converter's known answers. The same program decodes the runner's PNG
# for runner.png_file.
add_executable(schwaystation-test-runner
  ${CMAKE_CURRENT_LIST_DIR}/test_png.c
  ${PROJECT_SOURCE_DIR}/runner/png.c
  ${PROJECT_SOURCE_DIR}/runner/crc32.c)
target_include_directories(schwaystation-test-runner PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-runner PRIVATE schwaystation::schwaystation
  schwaystation-host)
set_target_properties(schwaystation-test-runner PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-runner)

# Fault prevented: an encoder whose chunk CRCs, Adler-32 or stored blocks are
# wrong, so a viewer or the RetroArch compare rejects or misreads the file.
schway_add_test(NAME runner.png
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-runner> png
  LABELS portable runner TIMEOUT 10)

# Fault prevented: a wrong 15-bit to XRGB8888 rule. The runner's PNG and the
# libretro core share the converter, so comparing them cannot catch it.
schway_add_test(NAME runner.xrgb8888
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-runner> xrgb8888
  LABELS portable runner TIMEOUT 10)

set(runner_check ${CMAKE_COMMAND} -DRUN=$<TARGET_FILE:schwaystation-run>
  "-DEMULATOR=${runner_emulator}")

# Fault prevented: the reference PNG the RetroArch screenshot is compared
# with changes bytes (converter, layout or encoder) while the frame hash
# still passes, or no longer shows the card.
schway_add_test(NAME runner.png_file
  COMMAND ${runner_check} -DCASE=png_file -DOUT=${PROJECT_BINARY_DIR}/tests/card.png
    -DDECODE=$<TARGET_FILE:schwaystation-test-runner> -DEXPECTED=${SCHWAYSTATION_TESTCARD_PNG_SHA256}
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)
