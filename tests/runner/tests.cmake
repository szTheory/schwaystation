# schwaystation-run: frame hashes, PNG output, bench report, usage and exit statuses, build revision.

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
