# Test registration and the exact test inventory.
#
# Fault prevented: a test that is silently not registered, or registered on
# one lane only, lets CI pass without running it. Configure fails unless the
# tests this build registers, with their labels, equal the union of the
# tests/<dir>/inventory.txt files that tests/inventory/<SCHWAYSTATION_INVENTORY>.txt
# selects (tests/cmake/inventory.cmake reads them).
#
# Inventory format: one test per line, "name<TAB>labels", labels sorted and
# separated by commas, lines sorted; lines starting with # are comments.

# schway_add_test(NAME n COMMAND cmd... LABELS l... TIMEOUT seconds [COST c] [PASS_REGEX re])
# TIMEOUT is multiplied by SCHWAYSTATION_TIMEOUT_SCALE, which slow lanes raise.
# PASS_REGEX makes the test pass only when its output matches and its command
# exits 0: the command runs through tests/cmake/expect_fail.cmake with CODE 0,
# because CTest's own pass-regex property would override a non-zero exit.
function(schway_add_test)
  cmake_parse_arguments(PARSE_ARGV 0 T "" "NAME;TIMEOUT;COST;PASS_REGEX" "COMMAND;LABELS")
  if(NOT T_NAME OR NOT T_COMMAND OR NOT T_LABELS OR NOT T_TIMEOUT)
    message(FATAL_ERROR "schway_add_test: NAME, COMMAND, LABELS and TIMEOUT are required")
  endif()
  if(NOT DEFINED T_COST)
    set(T_COST 1)
  endif()
  math(EXPR timeout "${T_TIMEOUT} * ${SCHWAYSTATION_TIMEOUT_SCALE}")
  if(DEFINED T_PASS_REGEX)
    set(command "${T_COMMAND}")
    string(REPLACE ";" "$<SEMICOLON>" command "${command}")
    add_test(NAME ${T_NAME}
      COMMAND ${CMAKE_COMMAND} "-DCMD=${command}" -DCODE=0 "-DREGEX=${T_PASS_REGEX}"
        -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake)
  else()
    add_test(NAME ${T_NAME} COMMAND ${T_COMMAND})
  endif()
  set_tests_properties(${T_NAME} PROPERTIES LABELS "${T_LABELS}" TIMEOUT ${timeout} COST ${T_COST})
endfunction()

include(${PROJECT_SOURCE_DIR}/tests/cmake/inventory.cmake)

# Runs once the top-level directory has been processed, so every include()d
# subsystem file has registered its tests.
function(schway__tests_check_inventory)
  set(file "${PROJECT_SOURCE_DIR}/tests/inventory/${SCHWAYSTATION_INVENTORY}.txt")
  schway_inventory_union("${file}" "${PROJECT_SOURCE_DIR}/tests" listed)

  get_property(names DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY TESTS)
  set(registered "")
  foreach(name IN LISTS names)
    get_test_property(${name} LABELS labels)
    list(SORT labels)
    list(JOIN labels "," labels)
    list(APPEND registered "${name}\t${labels}")
  endforeach()

  set(missing ${listed})
  set(unexpected ${registered})
  if(registered)
    list(REMOVE_ITEM missing ${registered})
  endif()
  if(listed)
    list(REMOVE_ITEM unexpected ${listed})
  endif()
  if(missing OR unexpected)
    list(JOIN missing "\n  " missing)
    list(JOIN unexpected "\n  " unexpected)
    message(FATAL_ERROR "registered tests differ from the directories ${file} selects\n"
      "listed but not registered:\n  ${missing}\n"
      "registered but not listed:\n  ${unexpected}")
  endif()
endfunction()

cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL schway__tests_check_inventory)
