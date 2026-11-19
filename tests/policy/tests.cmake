# Repository policy: no skipped tests, documentation budgets, the PASS_REGEX exit-status rule.
set(policy_scripts ${CMAKE_CURRENT_LIST_DIR}/../cmake)

# Fault prevented: a required workflow routing or measured quality check accepting a regression.
schway_add_test(NAME ci.classify_policy
  COMMAND ${CMAKE_COMMAND} -DROOT=${PROJECT_SOURCE_DIR} -DBIN=${PROJECT_BINARY_DIR}/policy/routes
    -P ${policy_scripts}/ci_route_controls.cmake
  LABELS policy TIMEOUT 30)
schway_add_test(NAME policy.quality_metrics
  COMMAND ${CMAKE_COMMAND} -DROOT=${PROJECT_SOURCE_DIR} -DBIN=${PROJECT_BINARY_DIR}/policy/quality
    -P ${policy_scripts}/quality_controls.cmake
  LABELS policy TIMEOUT 180)

# Fault prevented: publishing unbounded or private metadata from the GitHub API.
schway_add_test(NAME ci.triage
  COMMAND ${CMAKE_COMMAND} -DROOT=${PROJECT_SOURCE_DIR} -DBIN=${PROJECT_BINARY_DIR}/policy/triage
    -P ${policy_scripts}/triage_controls.cmake
  LABELS policy TIMEOUT 30)

# Fault prevented: repeated lesson IDs or prose hiding distinct findings in the ledger.
schway_add_test(NAME policy.lessons_unique
  COMMAND ${CMAKE_COMMAND} -DLEDGER=${PROJECT_SOURCE_DIR}/.planning/LESSONS.md
    -P ${policy_scripts}/lessons_unique.cmake
  LABELS policy TIMEOUT 10)
schway_add_test(NAME policy.lessons_unique.self_test
  COMMAND ${CMAKE_COMMAND} -DSELF_TEST=${PROJECT_BINARY_DIR}/policy/lessons
    -P ${policy_scripts}/lessons_unique.cmake
  LABELS policy TIMEOUT 10)

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

# Fault prevented: an oversized generated disc or boot executable entering a fixture.
schway_add_test(NAME policy.fixture_budget
  COMMAND ${CMAKE_COMMAND} -DIMAGE=${PROJECT_BINARY_DIR}/fixtures/disc_boot.bin
    -DAUDIO=${PROJECT_BINARY_DIR}/fixtures/disc_boot_track02.bin
    -DEXE=${PROJECT_BINARY_DIR}/fixtures/hello.exe -P ${PROJECT_SOURCE_DIR}/tests/cmake/fixture_budget.cmake
  LABELS policy portable TIMEOUT 10)

# Fault prevented: a public name hosts cannot find in the embedding guide, or a
# guide naming a removed name. The self test checks a copy of the header with one
# more function and expects that function named as undocumented.
schway_add_test(NAME docs.api_names
  COMMAND ${CMAKE_COMMAND} -DHEADER=${PROJECT_SOURCE_DIR}/include/schwaystation.h
    -DDOC=${PROJECT_SOURCE_DIR}/docs/embedding.md -P ${policy_scripts}/api_names.cmake
  LABELS policy portable TIMEOUT 10)
file(READ ${PROJECT_SOURCE_DIR}/include/schwaystation.h policy_header)
string(REPLACE "void schway_destroy(schway *inst);"
  "void schway_destroy(schway *inst);\nschway_status schway_not_documented(void);"
  policy_header "${policy_header}")
if(NOT policy_header MATCHES "schway_not_documented")
  message(FATAL_ERROR "docs.api_names.self_test: the schway_destroy declaration moved")
endif()
file(WRITE ${PROJECT_BINARY_DIR}/policy/undocumented_name.h "${policy_header}")
set(policy_cmd ${CMAKE_COMMAND} -DHEADER=${PROJECT_BINARY_DIR}/policy/undocumented_name.h
  -DDOC=${PROJECT_SOURCE_DIR}/docs/embedding.md -P ${policy_scripts}/api_names.cmake)
string(REPLACE ";" "$<SEMICOLON>" policy_cmd "${policy_cmd}")
schway_add_test(NAME docs.api_names.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${policy_cmd}" -DCODE=1
    "-DREGEX=api_names:.*undocumented.*schway_not_documented"
    -P ${policy_scripts}/expect_fail.cmake
  LABELS policy portable TIMEOUT 10)
