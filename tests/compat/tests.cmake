if(CMAKE_SCRIPT_MODE_FILE AND CASE STREQUAL "compat_baseline")
  foreach(required SOURCE_ROOT WORK_ROOT)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
      message(FATAL_ERROR "protected compatibility test requires ${required}")
    endif()
  endforeach()
  string(RANDOM LENGTH 10 ALPHABET 0123456789abcdef suffix)
  set(work "${WORK_ROOT}/compat-baseline-${suffix}")
  set(base "${work}/base")
  set(head "${work}/head")
  foreach(root IN ITEMS "${base}" "${head}")
    file(MAKE_DIRECTORY "${root}/scripts" "${root}/tests/cmake" "${root}/tests/compat")
    configure_file("${SOURCE_ROOT}/scripts/compat-ratchet.sh" "${root}/scripts/compat-ratchet.sh" COPYONLY)
  endforeach()
  configure_file("${SOURCE_ROOT}/tests/cmake/compat-ratchet.cmake" "${base}/tests/cmake/compat-ratchet.cmake" COPYONLY)
  file(WRITE "${head}/tests/cmake/compat-ratchet.cmake" "message(STATUS \"head judge accepted all rows\")\n")
  set(header "serial,md5,track1_sha1,title,region,media,predicate,status,evidence,date")
  set(pass "fixture/control,00000000000000000000000000000000,-,Control,ntsc-u,exe,frame@60=aaaaaaaa,pass,fixture.control:exit=0,2026-11-11")
  set(fail "fixture/control,00000000000000000000000000000000,-,Control,ntsc-u,exe,frame@60=aaaaaaaa,fail,fixture.control:exit=1,2026-11-11")
  file(WRITE "${base}/tests/compat/ledger.csv" "${header}\n${pass}\n")
  execute_process(COMMAND git init -q "${base}" RESULT_VARIABLE result ERROR_VARIABLE error)
  if(NOT "${result}" STREQUAL "0")
    message(FATAL_ERROR "cannot initialize protected-base fixture: ${error}")
  endif()
  execute_process(COMMAND git -C "${base}" config user.name "Compat Tests" RESULT_VARIABLE result)
  execute_process(COMMAND git -C "${base}" config user.email "12345678+compat-tests@users.noreply.github.com" RESULT_VARIABLE result)
  execute_process(COMMAND git -C "${base}" add scripts tests RESULT_VARIABLE result)
  execute_process(COMMAND git -C "${base}" commit -qm "protected compatibility base" RESULT_VARIABLE result)
  if(NOT "${result}" STREQUAL "0")
    message(FATAL_ERROR "cannot commit protected-base fixture")
  endif()
  execute_process(COMMAND git -C "${base}" rev-parse HEAD RESULT_VARIABLE result OUTPUT_VARIABLE base_sha OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT "${result}" STREQUAL "0")
    message(FATAL_ERROR "cannot identify protected-base fixture commit")
  endif()
  set(candidate "${work}/candidate.csv")
  set(fresh "${work}/fresh.csv")
  set(allow "${work}/allow.txt")
  file(WRITE "${candidate}" "${header}\n${fail}\n")
  file(WRITE "${fresh}" "${header}\n${fail}\n")
  file(WRITE "${allow}" "")
  execute_process(COMMAND "${CMAKE_COMMAND}" -E env "COMPAT_BASE_DIR=${base}" "COMPAT_BASE_SHA=${base_sha}"
      sh "${head}/scripts/compat-ratchet.sh" "${candidate}" "${fresh}" "" "${allow}" 2026-11-12
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
  if("${result}" STREQUAL "0")
    message(FATAL_ERROR "protected base judge accepted a later regression: ${output}${error}")
  endif()
  file(REMOVE_RECURSE "${work}")
  message(STATUS "compatibility: copied base judge rejected a later regression")
  return()
endif()

if(CMAKE_SCRIPT_MODE_FILE AND CASE STREQUAL "compat_live")
  foreach(required SOURCE_ROOT WORK_ROOT LEDGER TEST_BINARY RUN FIXTURE_DIR)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
      message(FATAL_ERROR "compatibility live check requires ${required}")
    endif()
  endforeach()
  execute_process(COMMAND "${TEST_BINARY}" schema "${LEDGER}" RESULT_VARIABLE rc)
  if(NOT rc STREQUAL "0")
    message(FATAL_ERROR "compatibility ledger schema check failed: ${rc}")
  endif()
  execute_process(COMMAND ${EMULATOR} "${RUN}" --exe "${FIXTURE_DIR}/gpu_motion.exe"
      --frames 60 --hash RESULT_VARIABLE rc OUTPUT_VARIABLE gpu ERROR_VARIABLE err)
  if(NOT rc STREQUAL "0" OR NOT gpu MATCHES "frame 60 sha256 ([0-9a-f]+)")
    message(FATAL_ERROR "fresh gpu_motion run failed: ${rc}: ${gpu}${err}")
  endif()
  set(gpu_hash "${CMAKE_MATCH_1}")
  foreach(pair IN ITEMS "pad_counter;300;${WORK_ROOT}/tests/pad-counter-press60.txt"
      "memcard_roundtrip;120;${WORK_ROOT}/tests/memcard-roundtrip/second.txt")
    list(GET pair 0 fixture)
    list(GET pair 1 frame)
    list(GET pair 2 path)
    if(NOT EXISTS "${path}")
      message(FATAL_ERROR "fresh ${fixture} result is missing: ${path}")
    endif()
    file(READ "${path}" report)
    if(NOT report MATCHES "frame ${frame} sha256 ([0-9a-f]+)")
      message(FATAL_ERROR "fresh ${fixture} result lacks frame ${frame}")
    endif()
    set(${fixture}_hash "${CMAKE_MATCH_1}")
  endforeach()
  if(NOT EXISTS "${WORK_ROOT}/openbios-shell-tuple.txt")
    message(FATAL_ERROR "fresh OpenBIOS shell tuple is missing")
  endif()
  file(READ "${WORK_ROOT}/openbios-shell-tuple.txt" bios_report)
  if(NOT bios_report MATCHES "openbios shell ntsc-u frame 240 sha256 ([0-9a-f]+)")
    message(FATAL_ERROR "fresh OpenBIOS shell test did not produce a checked frame tuple")
  endif()
  set(bios_hash "${CMAKE_MATCH_1}")
  file(READ "${LEDGER}" contents)
  string(REPLACE "\r" "" contents "${contents}")
  set(semi_token "__COMPAT_SEMICOLON_ESCAPE__")
  if(contents MATCHES "${semi_token}")
    message(FATAL_ERROR "compatibility ledger contains the reserved parser token")
  endif()
  string(REPLACE ";" "${semi_token}" contents "${contents}")
  string(REPLACE "\n" ";" rows "${contents}")
  list(POP_FRONT rows header)
  set(fresh "${WORK_ROOT}/tests/compat-fresh.csv")
  file(WRITE "${fresh}" "${header}\n")
  foreach(row IN LISTS rows)
    if(row STREQUAL "")
      continue()
    endif()
    string(REPLACE "," ";" fields "${row}")
    list(GET fields 0 serial)
    list(GET fields 6 predicate)
    list(GET fields 9 today)
    if(serial STREQUAL "fixture/gpu_motion")
      set(value "frame@60=${gpu_hash}")
    elseif(serial STREQUAL "fixture/pad_counter")
      set(value "frame@300=${pad_counter_hash}")
    elseif(serial STREQUAL "fixture/memcard_roundtrip")
      set(value "frame@120=${memcard_roundtrip_hash}")
    elseif(serial STREQUAL "fixture/openbios-shell")
      set(value "frame@240=${bios_hash}")
    else()
      message(FATAL_ERROR "compatibility ledger has no fresh-result producer for ${serial}")
    endif()
    string(REPLACE "${semi_token}" ";" row "${row}")
    string(REPLACE "${predicate}" "${value}" row "${row}")
    file(APPEND "${fresh}" "${row}\n")
  endforeach()
  execute_process(COMMAND sh "${SOURCE_ROOT}/scripts/compat-ratchet.sh"
      "${LEDGER}" "${fresh}" "" "${SOURCE_ROOT}/tests/compat/allow.txt" "${today}"
    RESULT_VARIABLE rc)
  if(NOT rc STREQUAL "0")
    message(FATAL_ERROR "fresh fixture results disagree with compatibility ledger: ${rc}")
  endif()
  return()
endif()

add_executable(schwaystation-test-compat ${CMAKE_CURRENT_LIST_DIR}/compat_tests.c)
set_target_properties(schwaystation-test-compat PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-compat)

# Fault prevented: a changed head judge accepting a regression against a protected base.
schway_add_test(NAME compat.ratchet.baseline
  COMMAND ${CMAKE_COMMAND} -DSOURCE_ROOT=${PROJECT_SOURCE_DIR} -DWORK_ROOT=${PROJECT_BINARY_DIR}
    -DCASE=compat_baseline -P ${PROJECT_SOURCE_DIR}/tests/compat/tests.cmake
  LABELS compat portable TIMEOUT 30)

# Fault prevented: rows or a header that do not use the ten meaningful fields.
schway_add_test(NAME compat.ratchet.rows
  COMMAND ${CMAKE_COMMAND}
    -DSOURCE_ROOT=${PROJECT_SOURCE_DIR} -DWORK_ROOT=${PROJECT_BINARY_DIR}
    -DLEDGER=${PROJECT_SOURCE_DIR}/tests/compat/ledger.csv
    -DTEST_BINARY=$<TARGET_FILE:schwaystation-test-compat>
    -DFIXTURE_DIR=${SCHWAYSTATION_FIXTURES_DIR}
    -DRUN=$<TARGET_FILE:schwaystation-run> "-DEMULATOR=${schway__probe_emulator}"
    -DCASE=compat_live -P ${PROJECT_SOURCE_DIR}/tests/compat/tests.cmake
  LABELS compat portable TIMEOUT 90)

schway_add_test(NAME compat.ratchet.transitions
  COMMAND ${CMAKE_COMMAND} -DSOURCE_ROOT=${PROJECT_SOURCE_DIR} -DWORK_ROOT=${PROJECT_BINARY_DIR} -DCASE=ratchet
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/compat-controls.cmake
  LABELS compat portable TIMEOUT 30)

schway_add_test(NAME policy.compat_doc_fresh
  COMMAND ${CMAKE_COMMAND} -DSOURCE_ROOT=${PROJECT_SOURCE_DIR} -DWORK_ROOT=${PROJECT_BINARY_DIR} -DCASE=docs
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/compat-controls.cmake
  LABELS compat portable TIMEOUT 30)

set_tests_properties(fixtures.gpu_motion.hash.60 PROPERTIES FIXTURES_SETUP compat_gpu_motion)
set_tests_properties(fixtures.pad_counter.press60 PROPERTIES FIXTURES_SETUP compat_pad_counter)
set_tests_properties(fixtures.memcard_roundtrip.persistence PROPERTIES FIXTURES_SETUP compat_memcard)
set_tests_properties(compat.ratchet.rows PROPERTIES
  FIXTURES_REQUIRED "compat_gpu_motion;compat_pad_counter;compat_memcard;openbios_shell")
