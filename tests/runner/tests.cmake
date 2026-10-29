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

  # Runs the runner with ARGN through expect_fail.cmake, which passes only
  # for exit 2 and a usage message.
  function(refuse)
    set(CMD ${EMULATOR} ${RUN} ${ARGN})
    set(CODE 2)
    set(REGEX "usage")
    include(${CMAKE_CURRENT_LIST_DIR}/../cmake/expect_fail.cmake)
  endfunction()

  # Runs one case of the bench unit test and fails unless it exits 0.
  function(unit case)
    execute_process(COMMAND ${EMULATOR} ${UNIT} ${case} RESULT_VARIABLE rc
      OUTPUT_VARIABLE out ERROR_VARIABLE out)
    if(NOT rc STREQUAL "0")
      message(FATAL_ERROR "schwaystation-test-bench ${case}: exit ${rc}\n${out}")
    endif()
  endfunction()

  if(CASE STREQUAL "bench_format")
    # The report's text from fixed samples, then the runner's own report:
    # every line present, in order, as a whole number. Times are never
    # compared, as they vary from run to run.
    unit(format)
    set(num "[0-9]+")
    set(report "frames 30\np50_us ${num}\np90_us ${num}\np99_us ${num}\np99[.]9_us ${num}\n"
      "p99[.]99_us ${num}\np99[.]999_us ${num}\nmax_us ${num}\nmax_rss_kib ${num}\n")
    string(CONCAT report ${report})
    expect_exit(0 "^${report}$" --bench 30)
    # One frame: every percentile is the one sample, so all seven times are
    # equal.
    execute_process(COMMAND ${EMULATOR} ${RUN} --bench 1 RESULT_VARIABLE rc OUTPUT_VARIABLE out)
    string(REGEX MATCHALL "_us [0-9]+" times "${out}")
    list(REMOVE_DUPLICATES times)
    list(LENGTH times n)
    if(NOT rc STREQUAL "0" OR NOT out MATCHES "^frames 1\n" OR NOT n EQUAL 1)
      message(FATAL_ERROR "--bench 1: expected exit 0 and seven equal times, got exit ${rc}\n${out}")
    endif()
  elseif(CASE STREQUAL "bench_bytes")
    # The sample buffer is sized exactly up to the cap; a count that is zero,
    # negative, missing or above the command line's limit is refused before
    # anything runs, never a wrapped allocation.
    unit(bytes)
    refuse(--bench 0)
    refuse(--bench -1)
    refuse(--bench)
    refuse(--bench 1000001)
    refuse(--bench 1073741824)
    refuse(--bench 4294967295)
  elseif(CASE STREQUAL "stop")
    # The flag, then a runner built to raise SIGTERM inside frame 5: the run
    # ends after that frame, still prints what was asked, and exits 0.
    unit(stop)
    set(RUN ${HOOKED})
    expect_exit(0 "^frame 5 sha256 ${EXPECTED}\n$" --frames 60 --hash)
    expect_exit(0 "^frames 5\n.*max_rss_kib [0-9]+\n$" --bench 60)
  elseif(CASE STREQUAL "no_args")
    refuse()
  elseif(CASE STREQUAL "unknown")
    # An option this release does not have, and --help or --version with
    # anything else.
    refuse(--frames 1 --bios)
    refuse(--frames 1 --disc x.cue)
    refuse(--frames 1 --help)
    refuse(--version --frames 1)
  elseif(CASE STREQUAL "positional")
    # This release loads no disc, so a file name is a usage error.
    refuse(--frames 1 game.cue)
    refuse(game.cue)
  elseif(CASE STREQUAL "frame_range")
    refuse(--frames 0)
    refuse(--frames 1000001)
    refuse(--frames 4294967296)
    refuse(--frames -1)
    refuse(--frames 1x)
    refuse(--frames)
    refuse(--hash --frames)
    refuse(--frames 1 --frames 1)
  elseif(CASE STREQUAL "png_syntax")
    # Each malformed or out-of-range --png is refused, whichever order the
    # options come in, and writes nothing.
    file(REMOVE "${OUT}")
    refuse(--frames 5 --png)
    refuse(--frames 5 --png "${OUT}")
    refuse(--frames 5 --png "5:")
    refuse(--frames 5 --png ":${OUT}")
    refuse(--frames 5 --png "0:${OUT}")
    refuse(--frames 5 --png "x:${OUT}")
    refuse(--frames 5 --png "6:${OUT}")
    refuse(--png "6:${OUT}" --frames 5)
    refuse(--frames 5 --png "5:${OUT}" --png "5:${OUT}")
    if(EXISTS "${OUT}")
      message(FATAL_ERROR "a refused --png wrote ${OUT}")
    endif()
  elseif(CASE STREQUAL "region")
    refuse(--frames 1 --region)
    refuse(--frames 1 --region secam)
    refuse(--frames 1 --region NTSC-U)
    refuse(--frames 1 --region auto)
  elseif(CASE STREQUAL "png_file")
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

# Fault prevented: a command line that runs something it should refuse, so a
# CI step hashes a frame nobody asked for: no arguments, an option this
# release lacks, a file name, a frame count out of range or missing, a
# malformed or out-of-range --png, or an unknown region.
foreach(case no_args unknown positional frame_range png_syntax region)
  schway_add_test(NAME runner.usage.${case}
    COMMAND ${runner_check} -DCASE=${case} -DOUT=${PROJECT_BINARY_DIR}/tests/refused.png
      -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS portable runner TIMEOUT 30)
endforeach()

# The bench statistics and the stop flag, tested directly, and a runner built
# with a hook that raises SIGTERM from inside frame 5.
add_executable(schwaystation-test-bench
  ${CMAKE_CURRENT_LIST_DIR}/test_runner.c
  ${PROJECT_SOURCE_DIR}/runner/bench.c
  ${PROJECT_SOURCE_DIR}/runner/stop.c)
set_target_properties(schwaystation-test-bench PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-bench)

add_executable(schwaystation-test-run-stop ${SCHWAYSTATION_RUNNER_SOURCES})
target_include_directories(schwaystation-test-run-stop PRIVATE ${SCHWAYSTATION_RUNNER_INCLUDE})
target_link_libraries(schwaystation-test-run-stop PRIVATE schwaystation::schwaystation
  schwaystation-host)
target_compile_definitions(schwaystation-test-run-stop PRIVATE _POSIX_C_SOURCE=200809L
  SCHWAY_RUN_RAISE_AT=5)
set_target_properties(schwaystation-test-run-stop PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-run-stop)

set(bench_check ${runner_check} -DUNIT=$<TARGET_FILE:schwaystation-test-bench>)

# Fault prevented: a percentile that interpolates, picks the wrong rank or
# reads unsorted samples, so the report names a time no frame took.
schway_add_test(NAME runner.bench.percentile
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-bench> percentile
  LABELS bench portable runner TIMEOUT 10)

# Fault prevented: a bench report missing a line, out of order or not in
# whole units, so a script reading it misreads the times.
schway_add_test(NAME runner.bench.format
  COMMAND ${bench_check} -DCASE=bench_format -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS bench portable runner TIMEOUT 30)

# Fault prevented: a sample count whose buffer size wraps to a small
# allocation, so the frame loop writes past it.
schway_add_test(NAME runner.bench.bytes
  COMMAND ${bench_check} -DCASE=bench_bytes -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS bench portable runner TIMEOUT 30)

# Fault prevented: SIGINT or SIGTERM killing the run mid-frame or losing the
# output already asked for, so an interrupted CI job reports nothing.
schway_add_test(NAME runner.stop
  COMMAND ${bench_check} -DHOOKED=$<TARGET_FILE:schwaystation-test-run-stop>
    -DEXPECTED=${SCHWAYSTATION_TESTCARD_SHA256_FRAME5} -DCASE=stop -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# Fault prevented: a runner that exits 0 after failing to write a file, or
# calls a failure a usage error.
set(runner_failure_cmd ${runner_cmd} --frames 1 --png "1:${PROJECT_BINARY_DIR}/tests/missing/x.png")
string(REPLACE ";" "$<SEMICOLON>" runner_failure_cmd "${runner_failure_cmd}")
schway_add_test(NAME runner.exit.failure
  COMMAND ${CMAKE_COMMAND} "-DCMD=${runner_failure_cmd}" -DCODE=1 "-DREGEX=cannot write"
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
  LABELS portable runner TIMEOUT 30)

# Fault prevented: --version printing something a script cannot parse.
schway_add_test(NAME runner.version
  COMMAND ${runner_cmd} --version
  PASS_REGEX "schwaystation-run [0-9]+[.][0-9]+[.][0-9]+ ([0-9a-f]+|unknown)"
  LABELS portable runner TIMEOUT 10)

# Fault prevented: testing a stale configure after a commit, so the results
# describe a tree other than HEAD. The self-test forces a mismatch.
set(runner_revision ${runner_check} -DSOURCE=${PROJECT_SOURCE_DIR}
  -P ${PROJECT_SOURCE_DIR}/tests/cmake/revision.cmake)
schway_add_test(NAME build.revision_matches_head
  COMMAND ${runner_revision}
  LABELS runner TIMEOUT 30)
set(runner_revision_cmd ${CMAKE_COMMAND} -DRUN=$<TARGET_FILE:schwaystation-run>
  -DREVISION=0000000000000000000000000000000000000000 -DSOURCE=${PROJECT_SOURCE_DIR}
  -P ${PROJECT_SOURCE_DIR}/tests/cmake/revision.cmake)
string(REPLACE ";" "$<SEMICOLON>" runner_revision_cmd "${runner_revision_cmd}")
schway_add_test(NAME build.revision_matches_head.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${runner_revision_cmd}" -DCODE=1
    "-DREGEX=configured at[^/]*0000000000000000000000000000000000000000[^/]*HEAD[^/]*is[^/]*[0-9a-f]"
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
  LABELS runner TIMEOUT 30)
