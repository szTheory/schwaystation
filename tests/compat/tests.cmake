if(CMAKE_SCRIPT_MODE_FILE AND CASE STREQUAL "probe_synthetic_identity")
  foreach(required WORK_ROOT TEST_BINARY RUN)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
      message(FATAL_ERROR "ISO probe test requires ${required}")
    endif()
  endforeach()
  string(REPLACE "|" ";" EMULATOR "${EMULATOR}")
  set(work "${WORK_ROOT}/compat-probe-synthetic")
  file(REMOVE_RECURSE "${work}")
  file(MAKE_DIRECTORY "${work}")
  set(image "${work}/synthetic.iso")
  execute_process(COMMAND ${EMULATOR} "${TEST_BINARY}" synthetic "${image}"
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(NOT rc STREQUAL "0")
    message(FATAL_ERROR "cannot generate ISO fixture: ${rc}: ${out}${err}")
  endif()
  file(SHA1 "${image}" track_sha1)
  if(NOT track_sha1 STREQUAL "b7a1f733d515077a111f6d792d6549fde163f912")
    message(FATAL_ERROR "generated ISO track SHA-1 changed: ${track_sha1}")
  endif()
  execute_process(COMMAND ${EMULATOR} "${RUN}" --probe "${image}"
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(NOT rc STREQUAL "0" OR
     NOT out MATCHES "format=disc" OR
     NOT out MATCHES "serial=SWAY-00001" OR
     NOT out MATCHES "boot_name=SWAY_000[.]01" OR
     NOT out MATCHES "exe_size=4096" OR
     NOT out MATCHES "rcheevos_md5=12481bed29c6c0d54a6f776ef5cec981" OR
     NOT out MATCHES "boot_status=unavailable" OR
     NOT out MATCHES "play_status=unavailable")
    message(FATAL_ERROR "runner did not report the exact synthetic ISO identity: ${rc}: ${out}${err}")
  endif()
  file(WRITE "${WORK_ROOT}/compat-probe-track1.sha1" "${track_sha1}\n")
  message(STATUS "ISO probe reported the exact synthetic metadata identity")
  return()
endif()

if(CMAKE_SCRIPT_MODE_FILE AND CASE STREQUAL "probe_full_identity")
  foreach(required WORK_ROOT TEST_BINARY RUN)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
      message(FATAL_ERROR "full identity probe test requires ${required}")
    endif()
  endforeach()
  string(REPLACE "|" ";" EMULATOR "${EMULATOR}")
  set(work "${WORK_ROOT}/compat-probe-full-identity")
  file(REMOVE_RECURSE "${work}")
  file(MAKE_DIRECTORY "${work}/metadata")
  set(image "${work}/synthetic.iso")
  execute_process(COMMAND ${EMULATOR} "${TEST_BINARY}" synthetic "${image}"
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(NOT rc STREQUAL "0")
    message(FATAL_ERROR "cannot generate ISO fixture: ${rc}: ${out}${err}")
  endif()
  file(SHA1 "${image}" track_sha1)
  set(ledger "${work}/metadata/ledger.csv")
  set(dat "${work}/metadata/title.dat")
  set(config "${work}/metadata/config.txt")
  file(WRITE "${dat}" "clrmamepro (\n name \"Synthetic test DAT\"\n)\ngame (\n name \"Schway Synthetic Metadata Fixture\"\n region \"USA\"\n serial \"SWAY-00001\"\n)\n")
  file(WRITE "${ledger}" "serial,md5,track1_sha1,title,region,media,predicate,status,evidence,date\nSWAY-00001,12481bed29c6c0d54a6f776ef5cec981,${track_sha1},Schway Synthetic Metadata Fixture,ntsc-u,cue/bin,metadata.identity,pass,synthetic:iso9660-known-answer,2026-11-12\nSWAY-00001,12481bed29c6c0d54a6f776ef5cec981,${track_sha1},Schway Synthetic Metadata Fixture,ntsc-u,cue/bin,boots@600,unknown,unavailable,2026-11-12\nSWAY-00001,12481bed29c6c0d54a6f776ef5cec981,${track_sha1},Schway Synthetic Metadata Fixture,ntsc-u,cue/bin,plays@600,unknown,unavailable,2026-11-12\n")
  file(WRITE "${config}" "schwaystation-test-metadata-v1\n")
  execute_process(COMMAND ${EMULATOR} "${RUN}" --probe "${image}"
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(NOT rc STREQUAL "0" OR
     NOT out MATCHES "serial=SWAY-00001" OR
     NOT out MATCHES "rcheevos_md5=12481bed29c6c0d54a6f776ef5cec981" OR
     NOT out MATCHES "track1_sha1=${track_sha1}" OR
     NOT out MATCHES "title=Schway Synthetic Metadata Fixture" OR
     NOT out MATCHES "metadata[.]identity=pass" OR
     NOT out MATCHES "boot_status=unknown" OR
     NOT out MATCHES "play_status=unknown")
    message(FATAL_ERROR "runner did not join exact synthetic track and DAT identity: ${rc}: ${out}${err}")
  endif()
  return()
endif()

if(CMAKE_SCRIPT_MODE_FILE AND CASE STREQUAL "probe_cue_modes")
  foreach(required WORK_ROOT TEST_BINARY RUN)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
      message(FATAL_ERROR "CUE probe test requires ${required}")
    endif()
  endforeach()
  string(REPLACE "|" ";" EMULATOR "${EMULATOR}")
  set(work "${WORK_ROOT}/compat-probe-cue-modes")
  file(REMOVE_RECURSE "${work}")
  file(MAKE_DIRECTORY "${work}")

  foreach(mode IN ITEMS cue_2048 cue_mode1_2352 cue_mode2_2352 cue_mode2_2336 cue_track99)
    set(cue "${work}/${mode}.cue")
    execute_process(COMMAND ${EMULATOR} "${TEST_BINARY}" "${mode}" "${cue}"
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT rc STREQUAL "0")
      message(FATAL_ERROR "cannot generate ${mode} CUE fixture: ${rc}: ${out}${err}")
    endif()
    execute_process(COMMAND ${EMULATOR} "${RUN}" --probe "${cue}"
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT rc STREQUAL "0" OR NOT out MATCHES "format=disc" OR
       NOT out MATCHES "serial=SWAY-00001" OR
       NOT out MATCHES "boot_name=SWAY_000[.]01" OR
       NOT out MATCHES "exe_size=4096" OR
       NOT out MATCHES "rcheevos_md5=12481bed29c6c0d54a6f776ef5cec981" OR
       NOT out MATCHES "boot_status=unavailable" OR
       NOT out MATCHES "play_status=unavailable")
      message(FATAL_ERROR "${mode} CUE did not preserve the synthetic disc identity: ${rc}: ${out}${err}")
    endif()
  endforeach()
  message(STATUS "CUE sector modes and track 99 preserved the exact synthetic identity")
  return()
endif()

if(CMAKE_SCRIPT_MODE_FILE AND CASE STREQUAL "probe_cue_edges")
  foreach(required WORK_ROOT TEST_BINARY RUN)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
      message(FATAL_ERROR "CUE edge probe test requires ${required}")
    endif()
  endforeach()
  string(REPLACE "|" ";" EMULATOR "${EMULATOR}")
  set(work "${WORK_ROOT}/compat-probe-cue-edges")
  file(REMOVE_RECURSE "${work}")
  file(MAKE_DIRECTORY "${work}")

  foreach(mode IN ITEMS cue_endpoint cue_bad_msf cue_bad_mode cue_bad_track cue_bad_index
      cue_escape cue_missing cue_name_255 cue_name_256 cue_max_bytes cue_over_bytes)
    set(case_dir "${work}/${mode}")
    file(MAKE_DIRECTORY "${case_dir}")
    set(cue "${case_dir}/synthetic.cue")
    execute_process(COMMAND ${EMULATOR} "${TEST_BINARY}" "${mode}" "${cue}"
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT rc STREQUAL "0")
      message(FATAL_ERROR "cannot generate ${mode} CUE fixture: ${rc}: ${out}${err}")
    endif()
    execute_process(COMMAND ${EMULATOR} "${RUN}" --probe "${cue}"
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(mode STREQUAL "cue_name_255" OR mode STREQUAL "cue_max_bytes")
      if(NOT rc STREQUAL "0" OR NOT out MATCHES "format=disc" OR
         NOT out MATCHES "serial=SWAY-00001" OR
         NOT out MATCHES "boot_name=SWAY_000[.]01" OR
         NOT out MATCHES "rcheevos_md5=12481bed29c6c0d54a6f776ef5cec981" OR
         NOT out MATCHES "boot_status=unavailable" OR NOT out MATCHES "play_status=unavailable")
        message(FATAL_ERROR "${mode} CUE boundary did not preserve identity: ${rc}: ${out}${err}")
      endif()
    else()
      set(expected "invalid CUE track metadata")
      if(mode STREQUAL "cue_endpoint" OR mode STREQUAL "cue_missing")
        set(expected "cannot read CUE track file")
      elseif(mode STREQUAL "cue_bad_mode")
        set(expected "unsupported CUE track mode")
      elseif(mode STREQUAL "cue_escape")
        set(expected "unsafe or escaping CUE FILE path")
      elseif(mode STREQUAL "cue_over_bytes")
        set(expected "disc image exceeds probe limits")
      endif()
      if(rc STREQUAL "0" OR NOT "${out}" STREQUAL "" OR NOT err MATCHES "${expected}")
        message(FATAL_ERROR "${mode} CUE edge was accepted or refused unexpectedly: ${rc}: ${out}${err}")
      endif()
    endif()
  endforeach()
  message(STATUS "CUE timestamp, metadata, path and size edges passed through --probe")
  return()
endif()

if(CMAKE_SCRIPT_MODE_FILE AND CASE STREQUAL "probe_iso_edges")
  foreach(required WORK_ROOT TEST_BINARY RUN)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
      message(FATAL_ERROR "ISO edge probe test requires ${required}")
    endif()
  endforeach()
  string(REPLACE "|" ";" EMULATOR "${EMULATOR}")
  set(work "${WORK_ROOT}/compat-probe-edges")
  file(REMOVE_RECURSE "${work}")
  file(MAKE_DIRECTORY "${work}")

  set(image "${work}/fallback.iso")
  execute_process(COMMAND ${EMULATOR} "${TEST_BINARY}" fallback "${image}"
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(NOT rc STREQUAL "0")
    message(FATAL_ERROR "cannot generate fallback ISO fixture: ${rc}: ${out}${err}")
  endif()
  execute_process(COMMAND ${EMULATOR} "${RUN}" --probe "${image}"
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(NOT rc STREQUAL "0" OR NOT out MATCHES "format=disc" OR
     NOT out MATCHES "serial=n/a" OR NOT out MATCHES "boot_name=PSX[.]EXE" OR
     NOT out MATCHES "boot_status=unavailable" OR NOT out MATCHES "play_status=unavailable")
    message(FATAL_ERROR "missing SYSTEM.CNF did not fall back to root PSX.EXE: ${rc}: ${out}${err}")
  endif()

  set(image "${work}/missing.iso")
  execute_process(COMMAND ${EMULATOR} "${TEST_BINARY}" missing "${image}"
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(NOT rc STREQUAL "0")
    message(FATAL_ERROR "cannot generate missing-boot ISO fixture: ${rc}: ${out}${err}")
  endif()
  execute_process(COMMAND ${EMULATOR} "${RUN}" --probe "${image}"
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(rc STREQUAL "0" OR NOT "${out}" STREQUAL "" OR
     NOT err MATCHES "disc boot executable not found")
    message(FATAL_ERROR "ISO with no boot inputs was accepted: ${rc}: ${out}${err}")
  endif()

  set(image "${work}/bad-extent.iso")
  execute_process(COMMAND ${EMULATOR} "${TEST_BINARY}" bad_extent "${image}"
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(NOT rc STREQUAL "0")
    message(FATAL_ERROR "cannot generate bad-extent ISO fixture: ${rc}: ${out}${err}")
  endif()
  execute_process(COMMAND ${EMULATOR} "${RUN}" --probe "${image}"
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(rc STREQUAL "0" OR NOT "${out}" STREQUAL "" OR
     NOT err MATCHES "invalid ISO9660 metadata")
    message(FATAL_ERROR "ISO with an out-of-volume boot extent was accepted: ${rc}: ${out}${err}")
  endif()

  set(image "${work}/traversal.iso")
  execute_process(COMMAND ${EMULATOR} "${TEST_BINARY}" traversal "${image}"
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(NOT rc STREQUAL "0")
    message(FATAL_ERROR "cannot generate traversal ISO fixture: ${rc}: ${out}${err}")
  endif()
  execute_process(COMMAND ${EMULATOR} "${RUN}" --probe "${image}"
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(rc STREQUAL "0" OR NOT "${out}" STREQUAL "" OR
     NOT err MATCHES "invalid SYSTEM[.]CNF boot metadata")
    message(FATAL_ERROR "SYSTEM.CNF parent traversal was accepted: ${rc}: ${out}${err}")
  endif()
  message(STATUS "ISO probe fallback and refusal edges passed")
  return()
endif()

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

if(CMAKE_SCRIPT_MODE_FILE AND CASE STREQUAL "compat_protected_controls")
  foreach(required SOURCE_ROOT WORK_ROOT)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
      message(FATAL_ERROR "protected compatibility controls require ${required}")
    endif()
  endforeach()
  string(RANDOM LENGTH 10 ALPHABET 0123456789abcdef suffix)
  set(work "${WORK_ROOT}/compat-controls-${suffix}")
  set(head "${work}/head")
  file(MAKE_DIRECTORY "${head}/scripts" "${head}/tests/cmake")
  configure_file("${SOURCE_ROOT}/scripts/compat-ratchet.sh" "${head}/scripts/compat-ratchet.sh" COPYONLY)
  configure_file("${SOURCE_ROOT}/tests/cmake/compat-ratchet.cmake" "${head}/tests/cmake/compat-ratchet.cmake" COPYONLY)
  set(header "serial,md5,track1_sha1,title,region,media,predicate,status,evidence,date")
  set(pass "fixture/control,00000000000000000000000000000000,-,Control,ntsc-u,exe,frame@60=aaaaaaaa,pass,fixture.control:exit=0,2026-11-11")
  set(fail "fixture/control,00000000000000000000000000000000,-,Control,ntsc-u,exe,frame@60=aaaaaaaa,fail,fixture.control:exit=1,2026-11-11")
  set(candidate "${work}/candidate.csv")
  set(fresh "${work}/fresh.csv")
  set(allow "${work}/allow.txt")
  file(MAKE_DIRECTORY "${work}")
  file(WRITE "${candidate}" "${header}\n${fail}\n")
  file(WRITE "${fresh}" "${header}\n${fail}\n")
  file(WRITE "${allow}" "")

  function(make_base name include_ledger include_judge include_helper)
    set(base "${work}/${name}/base")
    file(MAKE_DIRECTORY "${base}")
    file(WRITE "${base}/seed.txt" "base\n")
    set(stage_files seed.txt)
    execute_process(COMMAND git init -q "${base}" RESULT_VARIABLE result ERROR_VARIABLE error)
    if(NOT "${result}" STREQUAL "0")
      message(FATAL_ERROR "${name}: cannot initialize base: ${error}")
    endif()
    execute_process(COMMAND git -C "${base}" config user.name "Compat Tests" RESULT_VARIABLE result)
    execute_process(COMMAND git -C "${base}" config user.email "12345678+compat-tests@users.noreply.github.com" RESULT_VARIABLE result)
    if(include_ledger)
      file(MAKE_DIRECTORY "${base}/tests/compat")
      file(WRITE "${base}/tests/compat/ledger.csv" "${header}\n${pass}\n")
      list(APPEND stage_files tests/compat/ledger.csv)
    endif()
    if(include_judge)
      file(MAKE_DIRECTORY "${base}/scripts")
      configure_file("${SOURCE_ROOT}/scripts/compat-ratchet.sh" "${base}/scripts/compat-ratchet.sh" COPYONLY)
      list(APPEND stage_files scripts/compat-ratchet.sh)
    endif()
    if(include_helper)
      file(MAKE_DIRECTORY "${base}/tests/cmake")
      configure_file("${SOURCE_ROOT}/tests/cmake/compat-ratchet.cmake" "${base}/tests/cmake/compat-ratchet.cmake" COPYONLY)
      list(APPEND stage_files tests/cmake/compat-ratchet.cmake)
    endif()
    execute_process(COMMAND git -C "${base}" add ${stage_files} RESULT_VARIABLE result)
    execute_process(COMMAND git -C "${base}" commit -qm "protected ${name} base" RESULT_VARIABLE result)
    if(NOT "${result}" STREQUAL "0")
      message(FATAL_ERROR "${name}: cannot commit base fixture")
    endif()
    execute_process(COMMAND git -C "${base}" rev-parse HEAD RESULT_VARIABLE result OUTPUT_VARIABLE base_sha OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT "${result}" STREQUAL "0")
      message(FATAL_ERROR "${name}: cannot identify base fixture commit")
    endif()
    set("BASE_${name}" "${base}" PARENT_SCOPE)
    set("SHA_${name}" "${base_sha}" PARENT_SCOPE)
  endfunction()

  function(check_base name base base_sha expected)
    execute_process(COMMAND "${CMAKE_COMMAND}" -E env "COMPAT_BASE_DIR=${base}" "COMPAT_BASE_SHA=${base_sha}"
        sh "${head}/scripts/compat-ratchet.sh" "${candidate}" "${fresh}" "" "${allow}" 2026-11-12
      RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(expected AND NOT "${result}" STREQUAL "0")
      message(FATAL_ERROR "${name}: expected acceptance, got ${result}: ${output}${error}")
    elseif(NOT expected AND "${result}" STREQUAL "0")
      message(FATAL_ERROR "${name}: invalid baseline was accepted")
    endif()
  endfunction()

  make_base(introduction FALSE FALSE FALSE)
  check_base("absent-both introduction" "${BASE_introduction}" "${SHA_introduction}" TRUE)
  make_base(ledger_only TRUE FALSE TRUE)
  check_base("ledger without judge" "${BASE_ledger_only}" "${SHA_ledger_only}" FALSE)
  make_base(judge_only FALSE TRUE TRUE)
  check_base("judge without ledger" "${BASE_judge_only}" "${SHA_judge_only}" FALSE)
  make_base(missing_helper TRUE TRUE FALSE)
  check_base("judge without CMake dependency" "${BASE_missing_helper}" "${SHA_missing_helper}" FALSE)
  make_base(wrong_sha TRUE TRUE TRUE)
  check_base("mismatched base commit" "${BASE_wrong_sha}" "0000000000000000000000000000000000000000" FALSE)
  file(REMOVE "${head}/tests/cmake/compat-ratchet.cmake")
  check_base("deleted head helper" "${BASE_wrong_sha}" "${SHA_wrong_sha}" FALSE)
  file(REMOVE_RECURSE "${work}")
  message(STATUS "compatibility: introduction, one-missing, dependency, SHA and deleted-head controls passed")
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

add_executable(schwaystation-test-probe ${CMAKE_CURRENT_LIST_DIR}/probe_tests.c)
set_target_properties(schwaystation-test-probe PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-probe)

string(REPLACE ";" "|" schway__probe_emulator "${CMAKE_CROSSCOMPILING_EMULATOR}")

# Fault prevented: a raw ISO probe reporting a different synthetic boot identity.
schway_add_test(NAME compat.probe.synthetic_identity
  COMMAND ${CMAKE_COMMAND} -DWORK_ROOT=${PROJECT_BINARY_DIR}
    -DTEST_BINARY=$<TARGET_FILE:schwaystation-test-probe>
    -DRUN=$<TARGET_FILE:schwaystation-run> "-DEMULATOR=${schway__probe_emulator}"
    -DCASE=probe_synthetic_identity -P ${PROJECT_SOURCE_DIR}/tests/compat/tests.cmake
  LABELS compat portable TIMEOUT 30)

# Fault prevented: a matching serial, boot MD5, track SHA-1 and DAT title failing to join.
schway_add_test(NAME compat.probe.full_identity
  COMMAND ${CMAKE_COMMAND} -DWORK_ROOT=${PROJECT_BINARY_DIR}
    -DTEST_BINARY=$<TARGET_FILE:schwaystation-test-probe>
    -DRUN=$<TARGET_FILE:schwaystation-run> "-DEMULATOR=${schway__probe_emulator}"
    -DCASE=probe_full_identity -P ${PROJECT_SOURCE_DIR}/tests/compat/tests.cmake
  LABELS compat portable TIMEOUT 30)

# Fault prevented: CUE sector representations changing the canonical boot identity.
schway_add_test(NAME compat.probe.cue_modes
  COMMAND ${CMAKE_COMMAND} -DWORK_ROOT=${PROJECT_BINARY_DIR}
    -DTEST_BINARY=$<TARGET_FILE:schwaystation-test-probe>
    -DRUN=$<TARGET_FILE:schwaystation-run> "-DEMULATOR=${schway__probe_emulator}"
    -DCASE=probe_cue_modes -P ${PROJECT_SOURCE_DIR}/tests/compat/tests.cmake
  LABELS compat portable TIMEOUT 30)

# Fault prevented: a CUE timestamp, track, mode, path or input-size boundary being accepted incorrectly.
schway_add_test(NAME compat.probe.cue_edges
  COMMAND ${CMAKE_COMMAND} -DWORK_ROOT=${PROJECT_BINARY_DIR}
    -DTEST_BINARY=$<TARGET_FILE:schwaystation-test-probe>
    -DRUN=$<TARGET_FILE:schwaystation-run> "-DEMULATOR=${schway__probe_emulator}"
    -DCASE=probe_cue_edges -P ${PROJECT_SOURCE_DIR}/tests/compat/tests.cmake
  LABELS compat portable TIMEOUT 30)

# Fault prevented: a missing, unavailable or out-of-volume boot input being reported as a probe success.
schway_add_test(NAME compat.probe.iso_edges
  COMMAND ${CMAKE_COMMAND} -DWORK_ROOT=${PROJECT_BINARY_DIR}
    -DTEST_BINARY=$<TARGET_FILE:schwaystation-test-probe>
    -DRUN=$<TARGET_FILE:schwaystation-run> "-DEMULATOR=${schway__probe_emulator}"
    -DCASE=probe_iso_edges -P ${PROJECT_SOURCE_DIR}/tests/compat/tests.cmake
  LABELS compat portable TIMEOUT 30)

# Fault prevented: a changed head judge accepting a regression against a protected base.
schway_add_test(NAME compat.ratchet.baseline
  COMMAND ${CMAKE_COMMAND} -DSOURCE_ROOT=${PROJECT_SOURCE_DIR} -DWORK_ROOT=${PROJECT_BINARY_DIR}
    -DCASE=compat_baseline -P ${PROJECT_SOURCE_DIR}/tests/compat/tests.cmake
  LABELS compat portable TIMEOUT 30)

# Fault prevented: an incomplete or wrong compatibility baseline selecting introduction.
schway_add_test(NAME compat.ratchet.protected_controls
  COMMAND ${CMAKE_COMMAND} -DSOURCE_ROOT=${PROJECT_SOURCE_DIR} -DWORK_ROOT=${PROJECT_BINARY_DIR}
    -DCASE=compat_protected_controls -P ${PROJECT_SOURCE_DIR}/tests/compat/tests.cmake
  LABELS compat portable TIMEOUT 45)

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
