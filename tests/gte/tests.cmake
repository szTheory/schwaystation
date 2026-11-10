# The geometry engine: the hardware cases, the reference model and the register tests.
# One executable runs the case named on its command line. It also sees src/,
# so it reaches the engine's register and command entry points directly.
add_executable(schwaystation-test-gte ${CMAKE_CURRENT_LIST_DIR}/gte_tests.c)
target_include_directories(schwaystation-test-gte PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-gte PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-gte PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-gte)

set(gte_vectors_bin ${PROJECT_SOURCE_DIR}/tests/vectors/gte/ps1tests-1150.bin)

# Fault prevented: a register or a command that reads, writes or computes
# differently from the hardware-captured cases (a mirror, a sign extension, the
# FIFO push, LZCR, the FLAG bits, a saturation, the divide, a matrix, a colour
# step). All 1,150 cases run in the upstream order, the histogram proves the
# file holds 50 register-only cases and 50 of each of the 22 commands, and the
# count is the zero-case guard: a run that replayed fewer fails.
schway_add_test(NAME gte.vectors.ps1tests
  COMMAND schwaystation-test-gte vectors ${gte_vectors_bin} --expect 1150 --histogram
  LABELS gte portable TIMEOUT 60)

# One command line for expect_fail.cmake: the emulated lane's own guest binary,
# then the arguments, with the list separators protected.
function(schway__gte_cmd out)
  set(cmd ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-gte> ${ARGN})
  string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
  set(${out} "${cmd}" PARENT_SCOPE)
endfunction()

# The two unreadable inputs, built in the build tree at configure time: an
# empty file and the first 1,000 bytes of the cases, copied with the one CMake
# call that keeps NUL bytes.
set(gte_scratch ${PROJECT_BINARY_DIR}/tests)
file(MAKE_DIRECTORY ${gte_scratch})
file(WRITE ${gte_scratch}/gte_none.bin "")
# A drive-letter path gets the slash a file URL needs.
set(gte_url "${gte_vectors_bin}")
if(NOT gte_url MATCHES "^/")
  string(PREPEND gte_url "/")
endif()
file(DOWNLOAD file://${gte_url} ${gte_scratch}/gte_cut.bin RANGE_END 999
  STATUS gte_status)
list(GET gte_status 0 gte_code)
if(NOT gte_code EQUAL 0)
  message(FATAL_ERROR "gte: copying the first 1000 bytes of the cases failed: ${gte_status}")
endif()

# Fault prevented: a harness that passes when it read nothing, or only part of
# the file.
foreach(kind IN ITEMS empty truncated)
  if(kind STREQUAL "empty")
    set(input ${gte_scratch}/gte_none.bin)
  else()
    set(input ${gte_scratch}/gte_cut.bin)
  endif()
  schway__gte_cmd(cmd vectors ${input} --expect 1150 --histogram)
  schway_add_test(NAME gte.vectors.${kind}.self_test
    COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=gte vectors:.*(unreadable|truncated)"
      -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
    LABELS gte portable TIMEOUT 60)
endforeach()

# Fault prevented: a harness that compares too little of a case, so one wrong
# output bit passes. The expected output of case 600 is altered in memory and
# the run must fail naming that case.
schway__gte_cmd(cmd vectors ${gte_vectors_bin} --expect 1150 --histogram --flip 600)
schway_add_test(NAME gte.vectors.flipped.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=gte vectors: case 600 "
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
  LABELS gte portable TIMEOUT 60)

# Fault prevented: a register index, an edge value or a command number that
# reads or writes outside the engine's struct, reads differently twice, or
# claims more than the longest command's cycles. The asan lane's ASan and
# UBSan do the bounds proof; the program checks the rest.
schway_add_test(NAME gte.regs.sweep
  COMMAND schwaystation-test-gte regs_sweep
  LABELS gte portable TIMEOUT 30)

# Fault prevented: the reciprocal table the divide reads differing from its
# formula in even one of 257 bytes, which would skew projected points only for
# the divisors that index that entry and pass nearly every hardware case.
schway_add_test(NAME gte.unr_table
  COMMAND schwaystation-test-gte unr_table
  LABELS gte portable TIMEOUT 30)

# Fault prevented: the unused command bits (20 to 24, as a game sets them for
# NCLIP, 0140006h) changing the area, the flags or the cycles; the hardware
# cases never set those bits.
schway_add_test(NAME gte.nclip_fake_bits
  COMMAND schwaystation-test-gte nclip_fake_bits
  LABELS gte portable TIMEOUT 30)
