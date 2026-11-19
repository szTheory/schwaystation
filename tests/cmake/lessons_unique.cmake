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

function(record_lesson id prose)
  if(DEFINED lesson_id_${id})
    message(FATAL_ERROR "lessons_unique: duplicate lesson ID ${id}")
  endif()
  string(REGEX REPLACE "\\[([^]]+)\\]\\([^)]*\\)" "\\1" prose "${prose}")
  string(REGEX REPLACE "https?://[^ \t\r\n<>]+" "" prose "${prose}")
  string(REGEX REPLACE "[`*_]" "" prose "${prose}")
  string(TOLOWER "${prose}" prose)
  string(REGEX REPLACE "[^a-z0-9]+" " " prose "${prose}")
  string(STRIP "${prose}" prose)
  if(prose STREQUAL "")
    message(FATAL_ERROR "lessons_unique: missing lesson text for ${id}")
  endif()
  string(SHA256 key "${prose}")
  if(DEFINED lesson_text_${key})
    message(FATAL_ERROR "lessons_unique: duplicate lesson text for ${lesson_text_${key}} and ${id}")
  endif()
  set(lesson_id_${id} TRUE PARENT_SCOPE)
  set(lesson_text_${key} "${id}" PARENT_SCOPE)
  set(found TRUE PARENT_SCOPE)
endfunction()

file(STRINGS "${LEDGER}" lines ENCODING UTF-8)
foreach(line IN LISTS lines)
  if(line MATCHES "^##[ \t]+(.*)$")
    set(section "${CMAKE_MATCH_1}")
    if(own_id)
      record_lesson("${own_id}" "${own_text}")
      unset(own_id)
    endif()
  elseif(section MATCHES "^Imported lessons" AND
         line MATCHES "^\\|[ \t]*(XFER-[0-9]+)[ \t]*\\|((\\\\\\||[^|])*)\\|")
    record_lesson("${CMAKE_MATCH_1}" "${CMAKE_MATCH_2}")
  elseif(section STREQUAL "Lessons learned here")
    if(line MATCHES "^Lesson (SW-[A-Z0-9-]+-[0-9]+)([ \t]|$)")
      if(own_id)
        record_lesson("${own_id}" "${own_text}")
      endif()
      set(own_id "${CMAKE_MATCH_1}")
      set(own_text "")
      set(metadata FALSE)
    elseif(line MATCHES "^(Sources?|Status):")
      set(metadata TRUE)
    elseif(own_id AND NOT metadata)
      string(REGEX REPLACE "^(Symptom and reproduction|Root cause / invariant|Fix and evidence|Applies when|Does not establish|Suggested check)[^:]*:" "" line "${line}")
      string(APPEND own_text " ${line}")
    endif()
  endif()
endforeach()
if(own_id)
  record_lesson("${own_id}" "${own_text}")
endif()
if(NOT found)
  message(FATAL_ERROR "lessons_unique: no primary entries in lesson ledger")
endif()
