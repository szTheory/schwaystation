# Repository policy: no skipped tests, documentation budgets, the PASS_REGEX exit-status rule.
set(policy_scripts ${CMAKE_CURRENT_LIST_DIR}/../cmake)

schway_add_test(NAME policy.no_skip
  COMMAND ${CMAKE_COMMAND} -DROOT=${PROJECT_SOURCE_DIR} -P ${policy_scripts}/no_skip.cmake
  LABELS policy TIMEOUT 10)

# The self test scans a tree holding one forbidden line and expects that line
# named. The property name is built in pieces so policy.no_skip passes here.
set(policy_rule SKIP_)
string(APPEND policy_rule RETURN_CODE)
set(policy_fixture ${PROJECT_BINARY_DIR}/policy/no_skip_fixture)
file(WRITE ${policy_fixture}/CMakeLists.txt
  "set_tests_properties(t PROPERTIES ${policy_rule} 77)\n")
set(policy_cmd ${CMAKE_COMMAND} -DROOT=${policy_fixture} -P ${policy_scripts}/no_skip.cmake)
string(REPLACE ";" "$<SEMICOLON>" policy_cmd "${policy_cmd}")
schway_add_test(NAME policy.no_skip.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${policy_cmd}" -DCODE=1
    "-DREGEX=CMakeLists.txt:1: ${policy_rule}" -P ${policy_scripts}/expect_fail.cmake
  LABELS policy TIMEOUT 10)

# Fault prevented: a test that passes on matching text while its command failed.
# exit_one.cmake prints the match and fails; the driver runs it under the same
# wrapper schway_add_test uses for PASS_REGEX (CODE 0), and the outer wrapper
# expects that run to be refused.
set(policy_exit_one ${PROJECT_BINARY_DIR}/policy/exit_one.cmake)
set(policy_driver ${PROJECT_BINARY_DIR}/policy/pass_regex_driver.cmake)
file(WRITE ${policy_exit_one} "message(\"pass-regex-match\")\nmessage(FATAL_ERROR \"exit one\")\n")
file(WRITE ${policy_driver}
  "set(CMD \"${CMAKE_COMMAND}\" -P \"${policy_exit_one}\")\nset(CODE 0)\nset(REGEX pass-regex-match)\n"
  "include(\"${policy_scripts}/expect_fail.cmake\")\n")
set(policy_cmd ${CMAKE_COMMAND} -P ${policy_driver})
string(REPLACE ";" "$<SEMICOLON>" policy_cmd "${policy_cmd}")
schway_add_test(NAME policy.pass_regex_status.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${policy_cmd}" -DCODE=1 "-DREGEX=expected exit 0"
    -P ${policy_scripts}/expect_fail.cmake
  LABELS policy portable TIMEOUT 10)

# Byte budgets: README.md under 16384, include/schwaystation.h under 40960 and
# each docs/*.md and docs/*/*.md page under 61440.
schway_add_test(NAME docs.budgets
  COMMAND ${CMAKE_COMMAND} -DROOT=${PROJECT_SOURCE_DIR} -P ${policy_scripts}/budgets.cmake
  LABELS policy TIMEOUT 10)
