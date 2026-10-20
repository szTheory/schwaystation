# Hygiene tests, registered only by the hygiene preset (SCHWAYSTATION_HYGIENE).
set(hygiene_scripts ${CMAKE_CURRENT_LIST_DIR}/../cmake)

# Fault prevented: repository-check code growing past its budget. Every line
# of the shell scripts, the CMake check scripts and the hooks counts, at most
# 1500; the self test counts a 10-line fixture against a limit of 9.
schway_add_test(NAME policy.gate_budget
  COMMAND ${CMAKE_COMMAND} -DROOT=${CMAKE_SOURCE_DIR}
    "-DPATTERNS=scripts/*.sh$<SEMICOLON>tests/cmake/*.cmake$<SEMICOLON>.githooks/*" -DLIMIT=1500
    -P ${hygiene_scripts}/line_budget.cmake
  LABELS hygiene policy TIMEOUT 30)
set(hygiene_budget ${CMAKE_BINARY_DIR}/hygiene/line_budget_fixture)
file(REMOVE_RECURSE ${hygiene_budget})
string(REPEAT "line\n" 10 hygiene_lines)
file(CONFIGURE OUTPUT ${hygiene_budget}/sample.txt CONTENT "${hygiene_lines}" NEWLINE_STYLE UNIX)
set(hygiene_cmd ${CMAKE_COMMAND} -DROOT=${hygiene_budget} -DPATTERNS=* -DLIMIT=9
  -P ${hygiene_scripts}/line_budget.cmake)
string(REPLACE ";" "$<SEMICOLON>" hygiene_cmd "${hygiene_cmd}")
schway_add_test(NAME policy.gate_budget.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${hygiene_cmd}" -DCODE=1
    "-DREGEX=line_budget: 10 lines in .* exceeds the limit of 9"
    -P ${hygiene_scripts}/expect_fail.cmake
  LABELS hygiene policy TIMEOUT 30)
