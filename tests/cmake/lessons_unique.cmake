cmake_minimum_required(VERSION 3.25)
if(DEFINED SELF_TEST)
  function(check fixture expected)
    file(WRITE "${SELF_TEST}.md" "${fixture}")
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DLEDGER=${SELF_TEST}.md" -P "${CMAKE_SCRIPT_MODE_FILE}"
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
    if((expected STREQUAL "" AND NOT rc STREQUAL "0") OR
       (NOT expected STREQUAL "" AND (rc STREQUAL "0" OR NOT out MATCHES "${expected}")))
      message(FATAL_ERROR "lessons_unique: expected '${expected}' from the fixture, got exit ${rc}\n${out}")
    endif()
  endfunction()
  set(imports [=[
## Template
| XFER-001 | Ignored example | Sample | observed |
## Imported lessons, first batch
| XFER-001 | One fact with a reference to XFER-001 | Origin | observed |
]=])
  set(own [=[
## Lessons learned here
Lesson SW-TEST-001 (one revision)
Symptom and reproduction: **Keep** setup
  in [sync](first-reference).
Sources: old evidence
Status: observed
]=])
  check("${imports}${own}" "")
  check("${imports}| XFER-001 | A different fact | Elsewhere | reproduced |\n${own}" "duplicate (lesson )?ID.*XFER-001")
  check("${imports}${own}Lesson SW-TEST-002 (another revision)\nSymptom and reproduction: KEEP setup, in sync!\nSources: new evidence\nStatus: reproduced\n"
    "duplicate (lesson )?text.*SW-TEST-00[12]")
  return()
endif()
