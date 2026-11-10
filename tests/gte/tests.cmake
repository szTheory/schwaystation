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
# FIFO push, LZCR, the FLAG bits, a saturation, the divide). The cases run in
# the upstream order and the count is the zero-case guard: a run that
# replayed fewer fails.
schway_add_test(NAME gte.vectors.part1
  COMMAND schwaystation-test-gte vectors ${gte_vectors_bin} --expect 100 --ops reg,01
  LABELS gte portable TIMEOUT 30)

# Fault prevented: a register index, an edge value or a command number that
# reads or writes outside the engine's struct, reads differently twice, or
# claims more than the longest command's cycles. The asan lane's ASan and
# UBSan do the bounds proof; the program checks the rest.
schway_add_test(NAME gte.regs.sweep
  COMMAND schwaystation-test-gte regs_sweep
  LABELS gte portable TIMEOUT 30)
