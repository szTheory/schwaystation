# The integer-only proof: the core and a fixture built with general registers only (nofp lane).

# A fixture holding one double division must fail to build under
# -mgeneral-regs-only with GCC's own diagnostic (AArch64 names the option;
# x86-64 refuses the SSE register a double travels in). Fault prevented: a
# flag that stopped rejecting floating point, so the lane passes on a core
# that uses it. The fixture is built only inside this test.
set(nofp_cmd ${CMAKE_C_COMPILER} -std=c17 -mgeneral-regs-only
  -c ${CMAKE_CURRENT_LIST_DIR}/nofp_fixture.c -o ${PROJECT_BINARY_DIR}/tests/nofp_fixture.o)
string(REPLACE ";" "$<SEMICOLON>" nofp_cmd "${nofp_cmd}")
schway_add_test(NAME abi.nofp_fixture
  COMMAND ${CMAKE_COMMAND} "-DCMD=${nofp_cmd}" -DCODE=1
    "-DREGEX=general-regs-only[^ ]* is incompatible with the use of floating-point.types|SSE register (return|argument) with SSE disabled"
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
  LABELS abi TIMEOUT 30)
