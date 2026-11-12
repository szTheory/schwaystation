if(CMAKE_SCRIPT_MODE_FILE AND DEFINED EMULATOR)
  string(REPLACE "|" ";" EMULATOR "${EMULATOR}")
endif()

function(schway__compat_write_runner_launcher target launcher)
  get_target_property(output_dir ${target} RUNTIME_OUTPUT_DIRECTORY)
  get_target_property(output_name ${target} OUTPUT_NAME)
  if(NOT output_name)
    set(output_name ${target})
  endif()
  set(program "${output_dir}/${output_name}${CMAKE_EXECUTABLE_SUFFIX}")
  set(command "")
  foreach(argument IN LISTS CMAKE_CROSSCOMPILING_EMULATOR)
    string(REPLACE "'" "'\\''" quoted "${argument}")
    string(APPEND command " '${quoted}'")
  endforeach()
  string(REPLACE "'" "'\\''" quoted_program "${program}")
  file(WRITE "${launcher}" "#!/bin/sh\nexec${command} '${quoted_program}' \"\$@\"\n")
  file(CHMOD "${launcher}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endfunction()

function(probe_write_metadata root track_sha1)
  set(metadata_dir "${root}/metadata")
  file(MAKE_DIRECTORY "${metadata_dir}")
  set(PROBE_LEDGER "${metadata_dir}/ledger.csv")
  set(PROBE_DAT "${metadata_dir}/title.dat")
  set(PROBE_CONFIG "${metadata_dir}/config.txt")
  file(WRITE "${PROBE_DAT}" "clrmamepro (\n name \"Synthetic test DAT\"\n)\ngame (\n name \"Schway Synthetic Metadata Fixture\"\n region \"USA\"\n serial \"SWAY-00001\"\n)\n")
  file(WRITE "${PROBE_LEDGER}" "serial,md5,track1_sha1,title,region,media,predicate,status,evidence,date\nSWAY-00001,12481bed29c6c0d54a6f776ef5cec981,${track_sha1},Schway Synthetic Metadata Fixture,ntsc-u,cue/bin,metadata.identity,pass,synthetic:iso9660-known-answer,2026-11-12\nSWAY-00001,12481bed29c6c0d54a6f776ef5cec981,${track_sha1},Schway Synthetic Metadata Fixture,ntsc-u,cue/bin,boots@600,unknown,unavailable,2026-11-12\nSWAY-00001,12481bed29c6c0d54a6f776ef5cec981,${track_sha1},Schway Synthetic Metadata Fixture,ntsc-u,cue/bin,plays@600,unknown,unavailable,2026-11-12\n")
  file(WRITE "${PROBE_CONFIG}" "schwaystation-test-metadata-v1\n")
  set(PROBE_LEDGER "${PROBE_LEDGER}" PARENT_SCOPE)
  set(PROBE_DAT "${PROBE_DAT}" PARENT_SCOPE)
  set(PROBE_CONFIG "${PROBE_CONFIG}" PARENT_SCOPE)
endfunction()

function(library_prepare root)
  file(REMOVE_RECURSE "${root}")
  file(MAKE_DIRECTORY "${root}/discs" "${root}/bios" "${root}/metadata")
  execute_process(COMMAND ${EMULATOR} "${TEST_PROBE}" cue_2048 "${root}/discs/synthetic.cue"
    RESULT_VARIABLE rc OUTPUT_QUIET ERROR_QUIET)
  if(NOT rc STREQUAL "0")
    message(FATAL_ERROR "cannot generate bounded CUE/BIN fixture: ${rc}")
  endif()
  file(GLOB tracks "${root}/discs/*.bin")
  list(LENGTH tracks track_count)
  if(NOT track_count EQUAL 1)
    message(FATAL_ERROR "the CUE fixture did not produce one physical track")
  endif()
  list(GET tracks 0 track)
  file(SHA1 "${track}" track_sha1)
  set(config "${root}/config.txt")
  set(ledger "${root}/metadata/ledger.csv")
  set(dat "${root}/metadata/title.dat")
  set(probe_config "${root}/metadata/config.txt")
  file(WRITE "${config}" "SCHWAYSTATION_DISC_DIR=${root}/discs\nSCHWAYSTATION_BIOS_DIR=${root}/bios\n")
  file(WRITE "${probe_config}" "schwaystation-test-metadata-v1\n")
  file(WRITE "${dat}" "clrmamepro (\n name \"Synthetic test DAT\"\n)\ngame (\n name \"Schway Synthetic Metadata Fixture\"\n region \"USA\"\n serial \"SWAY-00001\"\n)\n")
  file(WRITE "${ledger}" "serial,md5,track1_sha1,title,region,media,predicate,status,evidence,date\nSWAY-00001,12481bed29c6c0d54a6f776ef5cec981,${track_sha1},Schway Synthetic Metadata Fixture,ntsc-u,cue/bin,metadata.identity,pass,synthetic:iso9660-known-answer,2026-11-12\nSWAY-00001,12481bed29c6c0d54a6f776ef5cec981,${track_sha1},Schway Synthetic Metadata Fixture,ntsc-u,cue/bin,boots@600,unknown,unavailable,2026-11-12\nSWAY-00001,12481bed29c6c0d54a6f776ef5cec981,${track_sha1},Schway Synthetic Metadata Fixture,ntsc-u,cue/bin,plays@600,unknown,unavailable,2026-11-12\n")
  set(LIBRARY_ROOT "${root}" PARENT_SCOPE)
  set(LIBRARY_TRACK_SHA1 "${track_sha1}" PARENT_SCOPE)
  set(LIBRARY_CONFIG "${config}" PARENT_SCOPE)
  set(LIBRARY_LEDGER "${ledger}" PARENT_SCOPE)
  set(LIBRARY_DAT "${dat}" PARENT_SCOPE)
  set(LIBRARY_PROBE_CONFIG "${probe_config}" PARENT_SCOPE)
endfunction()

function(library_snapshot root snapshot)
  file(GLOB_RECURSE paths LIST_DIRECTORIES FALSE RELATIVE "${root}" "${root}/*")
  set(entries)
  foreach(path IN LISTS paths)
    file(SHA256 "${root}/${path}" digest)
    list(APPEND entries "${path}|${digest}")
  endforeach()
  list(SORT entries)
  set(${snapshot} "${entries}" PARENT_SCOPE)
endfunction()

function(library_assert_snapshot root before)
  file(GLOB_RECURSE paths LIST_DIRECTORIES FALSE RELATIVE "${root}" "${root}/*")
  set(entries)
  foreach(path IN LISTS paths)
    if(NOT path STREQUAL "build/library/results.csv")
      file(SHA256 "${root}/${path}" digest)
      list(APPEND entries "${path}|${digest}")
    endif()
  endforeach()
  list(SORT entries)
  if(NOT "${entries}" STREQUAL "${before}")
    message(FATAL_ERROR "library command created or changed a file outside build/library/results.csv")
  endif()
  if(NOT EXISTS "${root}/build/library/results.csv")
    message(FATAL_ERROR "library command did not create build/library/results.csv")
  endif()
endfunction()

function(library_invoke root config ledger dat probe_config runner)
  if(ARGC GREATER 7)
    execute_process(COMMAND "${CMAKE_COMMAND}" -E env
        "LIBRARY_REAL_RUNNER=${ARGV6}" "LIBRARY_PROBE_LOG=${ARGV7}"
        sh "${SOURCE_ROOT}/scripts/library.sh"
        --test-root "${root}" --test-config "${config}" --test-ledger "${ledger}"
        --test-dat "${dat}" --test-probe-config "${probe_config}" --test-runner "${runner}"
      RESULT_VARIABLE library_rc OUTPUT_VARIABLE library_out ERROR_VARIABLE library_err)
  else()
    execute_process(COMMAND sh "${SOURCE_ROOT}/scripts/library.sh"
        --test-root "${root}" --test-config "${config}" --test-ledger "${ledger}"
        --test-dat "${dat}" --test-probe-config "${probe_config}" --test-runner "${runner}"
      RESULT_VARIABLE library_rc OUTPUT_VARIABLE library_out ERROR_VARIABLE library_err)
  endif()
  set(LIBRARY_RC "${library_rc}" PARENT_SCOPE)
  set(LIBRARY_OUT "${library_out}" PARENT_SCOPE)
  set(LIBRARY_ERR "${library_err}" PARENT_SCOPE)
endfunction()

function(library_validate root mode track_sha1 expected)
  execute_process(COMMAND ${EMULATOR} "${LIBRARY_TEST}" "${mode}" "${root}/build/library/results.csv" "${track_sha1}"
    RESULT_VARIABLE result OUTPUT_QUIET ERROR_QUIET)
  if(expected AND NOT result STREQUAL "0")
    message(FATAL_ERROR "library report failed ${mode} validation: ${result}")
  elseif(NOT expected AND result STREQUAL "0")
    message(FATAL_ERROR "library report unexpectedly passed ${mode} validation")
  endif()
endfunction()

if(CMAKE_SCRIPT_MODE_FILE AND CASE STREQUAL "library_synthetic_result")
  foreach(required WORK_ROOT SOURCE_ROOT TEST_PROBE LIBRARY_TEST RUN)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
      message(FATAL_ERROR "library synthetic test requires ${required}")
    endif()
  endforeach()
  if(DEFINED RUNNER_LAUNCHER AND NOT "${RUNNER_LAUNCHER}" STREQUAL "")
    set(RUN "${RUNNER_LAUNCHER}")
  endif()
  set(root "${WORK_ROOT}/compat-library-synthetic")
  library_prepare("${root}")
  library_snapshot("${root}" before)
  library_invoke("${root}" "${LIBRARY_CONFIG}" "${LIBRARY_LEDGER}" "${LIBRARY_DAT}"
    "${LIBRARY_PROBE_CONFIG}" "${RUN}")
  if(NOT LIBRARY_RC STREQUAL "0")
    message(FATAL_ERROR "temporary library invocation failed: ${LIBRARY_RC} ${LIBRARY_ERR}")
  endif()
  library_assert_snapshot("${root}" "${before}")
  library_validate("${root}" identity "${LIBRARY_TRACK_SHA1}" TRUE)
  message(STATUS "library report joined the exact CUE/BIN identity without changing other inputs")
  return()
endif()

if(CMAKE_SCRIPT_MODE_FILE AND CASE STREQUAL "library_controls")
  foreach(required WORK_ROOT SOURCE_ROOT TEST_PROBE LIBRARY_TEST RUN PRODUCTION_RUN)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
      message(FATAL_ERROR "library control test requires ${required}")
    endif()
  endforeach()
  if(DEFINED RUNNER_LAUNCHER AND NOT "${RUNNER_LAUNCHER}" STREQUAL "")
    set(RUN "${RUNNER_LAUNCHER}")
  endif()
  set(root "${WORK_ROOT}/compat-library-controls")
  library_prepare("${root}")

  if(NOT DEFINED RUNNER_LAUNCHER OR "${RUNNER_LAUNCHER}" STREQUAL "")
    file(MAKE_DIRECTORY "${WORK_ROOT}/tests/compat")
    set(suffixed_test_runner "${WORK_ROOT}/tests/compat/schwaystation-run-test.exe")
    file(COPY_FILE "${RUN}" "${suffixed_test_runner}")
    set(RUN "${suffixed_test_runner}")
  endif()

  execute_process(COMMAND sh "${SOURCE_ROOT}/scripts/library.sh"
      --test-root relative-library-root
      --test-config relative-library-root/config.txt
      --test-ledger relative-library-root/ledger.csv
      --test-dat relative-library-root/metadata.dat
      --test-probe-config relative-library-root/probe.conf
      --test-runner "${RUN}"
    RESULT_VARIABLE relative_root_rc ERROR_VARIABLE relative_root_err)
  if(relative_root_rc STREQUAL "0" OR NOT relative_root_err MATCHES "temporary root must be absolute")
    message(FATAL_ERROR "library test mode accepted a relative prepared root: ${relative_root_rc}")
  endif()

  execute_process(COMMAND sh "${SOURCE_ROOT}/scripts/library.sh"
      --test-root "${root}"
      --test-config "${root}/../escaped-config.txt"
      --test-ledger "${root}/ledger.csv"
      --test-dat "${root}/metadata.dat"
      --test-probe-config "${root}/probe.conf"
      --test-runner "${RUN}"
    RESULT_VARIABLE escaped_input_rc ERROR_VARIABLE escaped_input_err)
  if(escaped_input_rc STREQUAL "0" OR NOT escaped_input_err MATCHES "temporary input path is invalid")
    message(FATAL_ERROR "library test mode accepted a path escaping its prepared root: ${escaped_input_rc}")
  endif()

  execute_process(COMMAND sh "${SOURCE_ROOT}/scripts/library.sh"
      --test-root "${root}"
      --test-config "${root}/config.txt"
      --test-ledger "${root}/ledger.csv"
      --test-dat "${root}/metadata.dat"
      --test-probe-config "${root}/probe.conf"
      --test-runner schwaystation-run-test.exe
    RESULT_VARIABLE relative_runner_rc ERROR_VARIABLE relative_runner_err)
  if(relative_runner_rc STREQUAL "0" OR NOT relative_runner_err MATCHES "test runner is invalid")
    message(FATAL_ERROR "library test mode accepted a relative runner path: ${relative_runner_rc}")
  endif()

  file(WRITE "${root}/build/library/results.csv" "previous report\n")
  file(SHA256 "${root}/build/library/results.csv" old_report)
  library_invoke("${root}" "${root}/missing-config.txt" "${LIBRARY_LEDGER}" "${LIBRARY_DAT}"
    "${LIBRARY_PROBE_CONFIG}" "${RUN}")
  if(LIBRARY_RC STREQUAL "0")
    message(FATAL_ERROR "absent temporary configuration was accepted")
  endif()
  file(SHA256 "${root}/build/library/results.csv" actual_report)
  if(NOT actual_report STREQUAL old_report)
    message(FATAL_ERROR "absent configuration changed the previous report")
  endif()

  set(malicious "${root}/malicious-config.txt")
  file(WRITE "${malicious}" "SCHWAYSTATION_DISC_DIR=$(touch ${root}/PRIVATE_SENTINEL_EXECUTED)\nSCHWAYSTATION_BIOS_DIR=${root}/bios\n")
  library_invoke("${root}" "${malicious}" "${LIBRARY_LEDGER}" "${LIBRARY_DAT}"
    "${LIBRARY_PROBE_CONFIG}" "${RUN}")
  if(LIBRARY_RC STREQUAL "0" OR EXISTS "${root}/PRIVATE_SENTINEL_EXECUTED")
    message(FATAL_ERROR "configuration was evaluated as shell code")
  endif()
  set(bad_bios_config "${root}/bad-bios-config.txt")
  file(WRITE "${bad_bios_config}" "SCHWAYSTATION_DISC_DIR=${root}/discs\nSCHWAYSTATION_BIOS_DIR=${root}/absent-bios\n")
  library_invoke("${root}" "${bad_bios_config}" "${LIBRARY_LEDGER}" "${LIBRARY_DAT}"
    "${LIBRARY_PROBE_CONFIG}" "${RUN}")
  if(LIBRARY_RC STREQUAL "0")
    message(FATAL_ERROR "missing configured BIOS directory was accepted")
  endif()

  set(empty_root "${WORK_ROOT}/compat-library-empty")
  library_prepare("${empty_root}")
  file(GLOB inputs "${empty_root}/discs/*")
  file(REMOVE ${inputs})
  library_snapshot("${empty_root}" before_empty)
  library_invoke("${empty_root}" "${LIBRARY_CONFIG}" "${LIBRARY_LEDGER}" "${LIBRARY_DAT}"
    "${LIBRARY_PROBE_CONFIG}" "${RUN}")
  if(NOT LIBRARY_RC STREQUAL "0")
    message(FATAL_ERROR "valid empty library input was rejected: ${LIBRARY_ERR}")
  endif()
  library_assert_snapshot("${empty_root}" "${before_empty}")
  library_validate("${empty_root}" empty "${LIBRARY_TRACK_SHA1}" TRUE)

  set(duplicate_root "${WORK_ROOT}/compat-library-duplicates")
  library_prepare("${duplicate_root}")
  set(wrapper_dir "${WORK_ROOT}/library-runner/tests")
  file(MAKE_DIRECTORY "${wrapper_dir}")
  set(wrapper "${wrapper_dir}/schwaystation-run-test")
  set(probe_log "${WORK_ROOT}/library-probe-calls.txt")
  file(REMOVE "${probe_log}")
  file(WRITE "${wrapper}" "#!/bin/sh\nprintf '%s\\n' \"$2\" >> \"$LIBRARY_PROBE_LOG\"\nexec \"$LIBRARY_REAL_RUNNER\" \"$@\"\n")
  file(CHMOD "${wrapper}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
  file(GLOB tracks "${duplicate_root}/discs/*.bin")
  list(GET tracks 0 track)
  get_filename_component(track_name "${track}" NAME)
  file(READ "${duplicate_root}/discs/synthetic.cue" cue_text)
  string(REPLACE "\"${track_name}\"" "\"duplicate.bin\"" duplicate_cue "${cue_text}")
  file(COPY_FILE "${track}" "${duplicate_root}/discs/duplicate.bin")
  file(WRITE "${duplicate_root}/discs/duplicate.cue" "${duplicate_cue}")
  file(COPY_FILE "${track}" "${duplicate_root}/discs/standalone.bin")
  set(cue2352_source "${duplicate_root}/metadata/companion.cue")
  execute_process(COMMAND ${EMULATOR} "${TEST_PROBE}" cue_mode1_2352 "${cue2352_source}"
    RESULT_VARIABLE rc OUTPUT_QUIET ERROR_QUIET)
  if(NOT rc STREQUAL "0")
    message(FATAL_ERROR "cannot generate 2352 CUE companion fixture")
  endif()
  file(READ "${cue2352_source}" cue2352_text)
  string(REPLACE "\"synthetic.bin\"" "\"companion2352.bin\"" cue2352_text "${cue2352_text}")
  file(COPY_FILE "${duplicate_root}/metadata/synthetic.bin"
    "${duplicate_root}/discs/companion2352.bin")
  file(WRITE "${duplicate_root}/discs/companion2352.cue" "${cue2352_text}")
  library_snapshot("${duplicate_root}" before_duplicate)
  library_invoke("${duplicate_root}" "${LIBRARY_CONFIG}" "${LIBRARY_LEDGER}" "${LIBRARY_DAT}"
    "${LIBRARY_PROBE_CONFIG}" "${wrapper}" "${RUN}" "${probe_log}")
  if(NOT LIBRARY_RC STREQUAL "0")
    message(FATAL_ERROR "duplicate CUE/BIN or standalone BIN identity was rejected: ${LIBRARY_ERR}")
  endif()
  library_assert_snapshot("${duplicate_root}" "${before_duplicate}")
  library_validate("${duplicate_root}" identity "${LIBRARY_TRACK_SHA1}" TRUE)
  file(STRINGS "${probe_log}" probed_images)
  list(LENGTH probed_images probe_count)
  list(FIND probed_images "${duplicate_root}/discs/standalone.bin" standalone_index)
  if(NOT probe_count EQUAL 4 OR standalone_index EQUAL -1 OR
     "${probed_images}" MATCHES "synthetic[.]bin|duplicate[.]bin|companion2352[.]bin")
    message(FATAL_ERROR "standalone BIN was not probed exactly once or a CUE companion was probed raw: ${probed_images}")
  endif()

  set(invalid_bin "${duplicate_root}/discs/PRIVATE_SENTINEL_INVALID.bin")
  file(WRITE "${invalid_bin}" "not a raw 2048-sector image")
  file(SHA256 "${duplicate_root}/build/library/results.csv" old_report)
  library_invoke("${duplicate_root}" "${LIBRARY_CONFIG}" "${LIBRARY_LEDGER}" "${LIBRARY_DAT}"
    "${LIBRARY_PROBE_CONFIG}" "${RUN}")
  if(LIBRARY_RC STREQUAL "0" OR LIBRARY_ERR MATCHES "PRIVATE_SENTINEL")
    message(FATAL_ERROR "invalid standalone BIN was accepted or its name leaked")
  endif()
  file(SHA256 "${duplicate_root}/build/library/results.csv" actual_report)
  if(NOT actual_report STREQUAL old_report)
    message(FATAL_ERROR "invalid standalone BIN replaced the previous report")
  endif()
  file(REMOVE "${invalid_bin}")

  set(unlisted_root "${WORK_ROOT}/PRIVATE_SENTINEL_LIBRARY_ROOT")
  library_prepare("${unlisted_root}")
  file(READ "${LIBRARY_LEDGER}" ledger_text)
  string(REPLACE "${LIBRARY_TRACK_SHA1}" "0000000000000000000000000000000000000000"
    wrong_ledger_text "${ledger_text}")
  set(wrong_ledger "${unlisted_root}/metadata/wrong.csv")
  file(WRITE "${wrong_ledger}" "${wrong_ledger_text}")
  library_invoke("${unlisted_root}" "${LIBRARY_CONFIG}" "${wrong_ledger}" "${LIBRARY_DAT}"
    "${LIBRARY_PROBE_CONFIG}" "${RUN}")
  if(NOT LIBRARY_RC STREQUAL "0")
    message(FATAL_ERROR "wrong identity join should remain private: ${LIBRARY_ERR}")
  endif()
  if(LIBRARY_OUT MATCHES "PRIVATE_SENTINEL|SWAY-00001|SWAY_000[.]01|12481bed29c6c0d54a6f776ef5cec981|${LIBRARY_TRACK_SHA1}" OR
     LIBRARY_ERR MATCHES "PRIVATE_SENTINEL")
    message(FATAL_ERROR "unjoined input identity or path leaked to command output")
  endif()
  library_validate("${unlisted_root}" identity "${LIBRARY_TRACK_SHA1}" FALSE)
  library_validate("${unlisted_root}" unlisted "${LIBRARY_TRACK_SHA1}" TRUE)
  file(READ "${unlisted_root}/build/library/results.csv" redacted)
  if(redacted MATCHES "PRIVATE_SENTINEL|SWAY-00001|SWAY_000[.]01|12481bed29c6c0d54a6f776ef5cec981|${LIBRARY_TRACK_SHA1}")
    message(FATAL_ERROR "unjoined identity data was written to the local report")
  endif()

  string(REPLACE "metadata.identity,pass,synthetic:iso9660-known-answer"
    "metadata.identity,unknown,unavailable" unknown_ledger_text "${ledger_text}")
  set(unknown_ledger "${unlisted_root}/metadata/unknown.csv")
  file(WRITE "${unknown_ledger}" "${unknown_ledger_text}")
  library_invoke("${unlisted_root}" "${LIBRARY_CONFIG}" "${unknown_ledger}" "${LIBRARY_DAT}"
    "${LIBRARY_PROBE_CONFIG}" "${RUN}")
  if(NOT LIBRARY_RC STREQUAL "0")
    message(FATAL_ERROR "all-unavailable measurements were rejected: ${LIBRARY_ERR}")
  endif()
  library_validate("${unlisted_root}" identity "${LIBRARY_TRACK_SHA1}" FALSE)
  library_validate("${unlisted_root}" unlisted "${LIBRARY_TRACK_SHA1}" TRUE)

  set(duplicate_ledger "${unlisted_root}/metadata/duplicate.csv")
  string(REGEX MATCH "[^\n]*metadata[.]identity,pass[^\n]*" identity_row "${ledger_text}")
  file(WRITE "${duplicate_ledger}" "${ledger_text}${identity_row}\n")
  file(WRITE "${unlisted_root}/build/library/results.csv" "previous report\n")
  file(SHA256 "${unlisted_root}/build/library/results.csv" old_report)
  library_invoke("${unlisted_root}" "${LIBRARY_CONFIG}" "${duplicate_ledger}" "${LIBRARY_DAT}"
    "${LIBRARY_PROBE_CONFIG}" "${RUN}")
  if(LIBRARY_RC STREQUAL "0")
    message(FATAL_ERROR "duplicate metadata identity keys were accepted")
  endif()
  file(SHA256 "${unlisted_root}/build/library/results.csv" actual_report)
  if(NOT actual_report STREQUAL old_report)
    message(FATAL_ERROR "bad ledger changed the previous report")
  endif()

  set(unsupported_root "${WORK_ROOT}/compat-library-unsupported")
  library_prepare("${unsupported_root}")
  file(GLOB inputs "${unsupported_root}/discs/*")
  file(REMOVE ${inputs})
  file(WRITE "${unsupported_root}/discs/PRIVATE_SENTINEL_MEDIA.chd" "unsupported image")
  library_invoke("${unsupported_root}" "${LIBRARY_CONFIG}" "${LIBRARY_LEDGER}" "${LIBRARY_DAT}"
    "${LIBRARY_PROBE_CONFIG}" "${RUN}")
  if(NOT LIBRARY_RC STREQUAL "0")
    message(FATAL_ERROR "unsupported media should be counted without an identity result")
  endif()
  library_validate("${unsupported_root}" empty "${LIBRARY_TRACK_SHA1}" TRUE)

  set(malformed_root "${WORK_ROOT}/compat-library-malformed")
  library_prepare("${malformed_root}")
  file(GLOB tracks "${malformed_root}/discs/*.bin")
  list(GET tracks 0 track)
  get_filename_component(track_name "${track}" NAME)
  file(READ "${malformed_root}/discs/synthetic.cue" cue_text)
  string(REPLACE "\"${track_name}\"" "\"PRIVATE_SENTINEL_ABSENT.bin\"" malformed_cue "${cue_text}")
  file(WRITE "${malformed_root}/discs/synthetic.cue" "${malformed_cue}")
  file(WRITE "${malformed_root}/build/library/results.csv" "previous report\n")
  file(SHA256 "${malformed_root}/build/library/results.csv" old_report)
  library_invoke("${malformed_root}" "${LIBRARY_CONFIG}" "${LIBRARY_LEDGER}" "${LIBRARY_DAT}"
    "${LIBRARY_PROBE_CONFIG}" "${RUN}")
  if(LIBRARY_RC STREQUAL "0" OR LIBRARY_ERR MATCHES "PRIVATE_SENTINEL")
    message(FATAL_ERROR "malformed CUE was accepted or its path leaked")
  endif()
  file(SHA256 "${malformed_root}/build/library/results.csv" actual_report)
  if(NOT actual_report STREQUAL old_report)
    message(FATAL_ERROR "malformed input changed the previous report")
  endif()

  set(system_root "${WORK_ROOT}/compat-library-bad-system")
  library_prepare("${system_root}")
  set(system_iso "${system_root}/discs/bad-system.iso")
  execute_process(COMMAND ${EMULATOR} "${TEST_PROBE}" traversal "${system_iso}"
    RESULT_VARIABLE rc OUTPUT_QUIET ERROR_QUIET)
  if(NOT rc STREQUAL "0")
    message(FATAL_ERROR "cannot generate invalid SYSTEM.CNF fixture")
  endif()
  file(WRITE "${system_root}/build/library/results.csv" "previous report\n")
  file(SHA256 "${system_root}/build/library/results.csv" old_report)
  library_invoke("${system_root}" "${LIBRARY_CONFIG}" "${LIBRARY_LEDGER}" "${LIBRARY_DAT}"
    "${LIBRARY_PROBE_CONFIG}" "${RUN}")
  if(LIBRARY_RC STREQUAL "0" OR LIBRARY_ERR MATCHES "bad-system.iso")
    message(FATAL_ERROR "invalid SYSTEM.CNF was accepted or its path leaked")
  endif()
  file(SHA256 "${system_root}/build/library/results.csv" actual_report)
  if(NOT actual_report STREQUAL old_report)
    message(FATAL_ERROR "invalid SYSTEM.CNF changed the previous report")
  endif()

  set(ordinary_root "${WORK_ROOT}/compat-library-ordinary")
  set(fake_bin "${ordinary_root}/test-bin")
  file(REMOVE_RECURSE "${ordinary_root}")
  file(MAKE_DIRECTORY "${ordinary_root}/scripts" "${ordinary_root}/build/library"
    "${ordinary_root}/tests/compat" "${fake_bin}")
  file(COPY_FILE "${SOURCE_ROOT}/scripts/library.sh" "${ordinary_root}/scripts/library.sh")
  if(DEFINED PRODUCTION_RUNNER_LAUNCHER AND NOT "${PRODUCTION_RUNNER_LAUNCHER}" STREQUAL "")
    file(COPY_FILE "${PRODUCTION_RUNNER_LAUNCHER}" "${ordinary_root}/build/library/schwaystation-run")
  else()
    get_filename_component(production_runner_name "${PRODUCTION_RUN}" NAME)
    file(COPY_FILE "${PRODUCTION_RUN}" "${ordinary_root}/build/library/${production_runner_name}")
  endif()
  file(COPY_FILE "${LIBRARY_LEDGER}" "${ordinary_root}/tests/compat/ledger.csv")
  file(COPY_FILE "${LIBRARY_DAT}" "${ordinary_root}/build/library/Sony - PlayStation.dat")
  file(WRITE "${ordinary_root}/.env.local"
    "SCHWAYSTATION_DISC_DIR=${duplicate_root}/discs\nSCHWAYSTATION_BIOS_DIR=${duplicate_root}/bios\n")
  set(fake_cmake "${fake_bin}/cmake")
  file(WRITE "${fake_cmake}" "#!/bin/sh\n[ \"$*\" = \"--workflow --preset library\" ] || exit 2\nexit 0\n")
  file(CHMOD "${fake_cmake}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
  if(WIN32)
    set(path_separator ";")
  else()
    set(path_separator ":")
  endif()
  set(mirror_path "${fake_bin}${path_separator}$ENV{PATH}")
  library_snapshot("${ordinary_root}" before_ordinary)
  execute_process(COMMAND "${CMAKE_COMMAND}" -E env "PATH=${mirror_path}" sh
      "${ordinary_root}/scripts/library.sh"
    WORKING_DIRECTORY "${ordinary_root}"
    RESULT_VARIABLE ordinary_rc OUTPUT_VARIABLE ordinary_out ERROR_VARIABLE ordinary_err)
  if(NOT ordinary_rc STREQUAL "0")
    message(FATAL_ERROR "ordinary library mode failed against its temporary provider: ${ordinary_rc}: ${ordinary_out}${ordinary_err}")
  endif()
  library_assert_snapshot("${ordinary_root}" "${before_ordinary}")
  file(READ "${ordinary_root}/build/library/results.csv" ordinary_report)
  if(NOT ordinary_report MATCHES "SWAY-00001,12481bed29c6c0d54a6f776ef5cec981,${LIBRARY_TRACK_SHA1},Schway Synthetic Metadata Fixture,ntsc-u,cue/bin,metadata[.]identity,pass,synthetic:iso9660-known-answer,2026-11-12" OR
     NOT ordinary_report MATCHES "SWAY-00001,12481bed29c6c0d54a6f776ef5cec981,${LIBRARY_TRACK_SHA1},Schway Synthetic Metadata Fixture,ntsc-u,cue/bin,boots@600,unknown,unavailable:boots@600," OR
     NOT ordinary_report MATCHES "SWAY-00001,12481bed29c6c0d54a6f776ef5cec981,${LIBRARY_TRACK_SHA1},Schway Synthetic Metadata Fixture,ntsc-u,cue/bin,plays@600,unknown,unavailable:plays@600," OR
     NOT ordinary_report MATCHES "unlisted,unavailable,unavailable,unavailable,unknown,cue/bin,metadata[.]identity,unknown,unavailable," OR
     ordinary_report MATCHES "companion2352|PRIVATE_SENTINEL")
    message(FATAL_ERROR "ordinary mode did not join known identity or keep an unlisted CUE/BIN private")
  endif()

  file(SHA256 "${ordinary_root}/build/library/results.csv" old_report)
  file(WRITE "${duplicate_root}/discs/PRIVATE_SENTINEL_INVALID.bin" "not a raw 2048-sector image")
  execute_process(COMMAND "${CMAKE_COMMAND}" -E env "PATH=${mirror_path}" sh
      "${ordinary_root}/scripts/library.sh"
    WORKING_DIRECTORY "${ordinary_root}"
    RESULT_VARIABLE invalid_bin_rc OUTPUT_VARIABLE invalid_bin_out ERROR_VARIABLE invalid_bin_err)
  if(invalid_bin_rc STREQUAL "0" OR invalid_bin_err MATCHES "PRIVATE_SENTINEL")
    message(FATAL_ERROR "ordinary mode accepted or exposed an invalid standalone BIN")
  endif()
  file(SHA256 "${ordinary_root}/build/library/results.csv" actual_report)
  if(NOT actual_report STREQUAL old_report)
    message(FATAL_ERROR "ordinary mode replaced the previous report after an invalid BIN")
  endif()
  message(STATUS "library controls rejected unsafe configuration and joins without leaking or replacing results")
  return()
endif()

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
     NOT out MATCHES "serial=redacted" OR
     NOT out MATCHES "boot_name=redacted" OR
     NOT out MATCHES "rcheevos_md5=redacted" OR
     NOT out MATCHES "track1_sha1=redacted" OR
     NOT out MATCHES "metadata[.]identity=unknown" OR
     NOT out MATCHES "boot_status=unknown" OR
     NOT out MATCHES "play_status=unknown" OR
     out MATCHES "SWAY-00001|SWAY_000[.]01|12481bed29c6c0d54a6f776ef5cec981|${track_sha1}")
    message(FATAL_ERROR "unlisted ISO identity was not safely redacted: ${rc}: ${out}${err}")
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
  file(WRITE "${dat}" "clrmamepro (\n name \"Synthetic test DAT\"\n)\ngame (\n name \"Legacy, title with unsupported CSV separator\"\n region \"USA\"\n serial \"SLES-00000\"\n)\ngame (\n name \"Schway Synthetic Metadata Fixture\"\n region \"USA\"\n serial \"SWAY-00001\"\n)\n")
  file(WRITE "${ledger}" "serial,md5,track1_sha1,title,region,media,predicate,status,evidence,date\nSWAY-00001,12481bed29c6c0d54a6f776ef5cec981,${track_sha1},Schway Synthetic Metadata Fixture,ntsc-u,cue/bin,metadata.identity,pass,synthetic:iso9660-known-answer,2026-11-12\nSWAY-00001,12481bed29c6c0d54a6f776ef5cec981,${track_sha1},Schway Synthetic Metadata Fixture,ntsc-u,cue/bin,boots@600,unknown,unavailable,2026-11-12\nSWAY-00001,12481bed29c6c0d54a6f776ef5cec981,${track_sha1},Schway Synthetic Metadata Fixture,ntsc-u,cue/bin,plays@600,unknown,unavailable,2026-11-12\n")
  file(WRITE "${config}" "schwaystation-test-metadata-v1\n")
  execute_process(COMMAND ${EMULATOR} "${RUN}" --probe "${image}"
      --test-ledger "${ledger}" --test-dat "${dat}" --test-root "${work}"
      --test-config "${config}"
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(NOT rc STREQUAL "0" OR
     NOT out MATCHES "serial=SWAY-00001" OR
     NOT out MATCHES "rcheevos_md5=12481bed29c6c0d54a6f776ef5cec981" OR
     NOT out MATCHES "track1_sha1=${track_sha1}" OR
     NOT out MATCHES "title=Schway Synthetic Metadata Fixture" OR
     NOT out MATCHES "metadata[.]identity=pass" OR
     NOT out MATCHES "metadata[.]identity_date=2026-11-12" OR
     NOT out MATCHES "boot_status=unknown" OR
     NOT out MATCHES "play_status=unknown")
    message(FATAL_ERROR "runner did not join exact synthetic track and DAT identity: ${rc}: ${out}${err}")
  endif()
  return()
endif()

if(CMAKE_SCRIPT_MODE_FILE AND CASE STREQUAL "probe_malformed")
  foreach(required WORK_ROOT TEST_BINARY RUN)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
      message(FATAL_ERROR "malformed probe test requires ${required}")
    endif()
  endforeach()
  string(REPLACE "|" ";" EMULATOR "${EMULATOR}")
  set(work "${WORK_ROOT}/compat-probe-malformed")
  file(REMOVE_RECURSE "${work}")
  file(MAKE_DIRECTORY "${work}")

  foreach(mode IN ITEMS truncated overlap loop bad_extent)
    set(root "${work}/${mode}")
    file(MAKE_DIRECTORY "${root}")
    set(image "${root}/input.iso")
    execute_process(COMMAND ${EMULATOR} "${TEST_BINARY}" "${mode}" "${image}"
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT rc STREQUAL "0")
      message(FATAL_ERROR "cannot generate ${mode} ISO: ${rc}: ${out}${err}")
    endif()
    execute_process(COMMAND ${EMULATOR} "${RUN}" --probe "${image}"
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(rc STREQUAL "0" OR NOT "${out}" STREQUAL "" OR
       NOT err MATCHES "schwaystation-run: input:" OR err MATCHES "${root}")
      message(FATAL_ERROR "${mode} ISO was accepted, leaked its path, or escaped bounded refusal: ${rc}: ${out}${err}")
    endif()
  endforeach()

  set(cue "${work}/bad.cue")
  execute_process(COMMAND ${EMULATOR} "${TEST_BINARY}" cue_bad_msf "${cue}"
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(NOT rc STREQUAL "0")
    message(FATAL_ERROR "cannot generate malformed CUE: ${rc}: ${out}${err}")
  endif()
  execute_process(COMMAND ${EMULATOR} "${RUN}" --probe "${cue}"
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(rc STREQUAL "0" OR NOT "${out}" STREQUAL "" OR
     NOT err MATCHES "invalid CUE track metadata" OR err MATCHES "bad[.]cue")
    message(FATAL_ERROR "malformed CUE was accepted or exposed its path: ${rc}: ${out}${err}")
  endif()

  set(absent "${work}/PRIVATE_SENTINEL_ABSENT.iso")
  execute_process(COMMAND ${EMULATOR} "${RUN}" --probe "${absent}"
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(rc STREQUAL "0" OR NOT "${out}" STREQUAL "" OR
     NOT err MATCHES "cannot open the file" OR err MATCHES "PRIVATE_SENTINEL")
    message(FATAL_ERROR "absent input was accepted or its path leaked: ${rc}: ${out}${err}")
  endif()
  set(unsupported "${work}/PRIVATE_SENTINEL_MEDIA.pbp")
  file(WRITE "${unsupported}" "PRIVATE_SENTINEL_CONTENT")
  execute_process(COMMAND ${EMULATOR} "${RUN}" --probe "${unsupported}"
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(rc STREQUAL "0" OR NOT "${out}" STREQUAL "" OR
     NOT err MATCHES "unsupported disc image media" OR err MATCHES "PRIVATE_SENTINEL")
    message(FATAL_ERROR "unsupported media was accepted or exposed private input: ${rc}: ${out}${err}")
  endif()

  set(root "${work}/PRIVATE_SENTINEL_ROOT_04_42")
  file(MAKE_DIRECTORY "${root}")
  set(image "${root}/PRIVATE_SENTINEL_NAME.iso")
  execute_process(COMMAND ${EMULATOR} "${TEST_BINARY}" synthetic "${image}"
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(NOT rc STREQUAL "0")
    message(FATAL_ERROR "cannot generate unlisted ISO: ${rc}: ${out}${err}")
  endif()
  file(SHA1 "${image}" track_sha1)
  execute_process(COMMAND ${EMULATOR} "${RUN}" --probe "${image}"
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(NOT rc STREQUAL "0" OR NOT out MATCHES "metadata[.]identity=unknown" OR
     NOT out MATCHES "serial=redacted" OR NOT out MATCHES "rcheevos_md5=redacted" OR
     NOT out MATCHES "track1_sha1=redacted" OR NOT out MATCHES "boot_name=redacted" OR
     out MATCHES "PRIVATE_SENTINEL|SWAY-00001|SWAY_000[.]01|12481bed29c6c0d54a6f776ef5cec981|${track_sha1}")
    message(FATAL_ERROR "unlisted valid identity was not redacted: ${rc}: ${out}${err}")
  endif()

  foreach(control IN ITEMS wrong_md5 wrong_sha1 wrong_title missing_dat missing_ledger all_unknown)
    set(root "${work}/${control}")
    file(MAKE_DIRECTORY "${root}")
    set(image "${root}/input.iso")
    execute_process(COMMAND ${EMULATOR} "${TEST_BINARY}" synthetic "${image}"
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT rc STREQUAL "0")
      message(FATAL_ERROR "cannot generate ${control} ISO: ${rc}: ${out}${err}")
    endif()
    file(SHA1 "${image}" track_sha1)
    probe_write_metadata("${root}" "${track_sha1}")
    if(control STREQUAL "wrong_md5")
      file(READ "${PROBE_LEDGER}" text)
      string(REPLACE "12481bed29c6c0d54a6f776ef5cec981" "00000000000000000000000000000000" text "${text}")
      file(WRITE "${PROBE_LEDGER}" "${text}")
    elseif(control STREQUAL "wrong_sha1")
      file(READ "${PROBE_LEDGER}" text)
      string(REPLACE "${track_sha1}" "0000000000000000000000000000000000000000" text "${text}")
      file(WRITE "${PROBE_LEDGER}" "${text}")
    elseif(control STREQUAL "wrong_title")
      file(READ "${PROBE_DAT}" text)
      string(REPLACE "Schway Synthetic Metadata Fixture" "Private Wrong Title" text "${text}")
      file(WRITE "${PROBE_DAT}" "${text}")
    elseif(control STREQUAL "missing_dat")
      set(PROBE_DAT "${root}/metadata/absent.dat")
    elseif(control STREQUAL "missing_ledger")
      set(PROBE_LEDGER "${root}/metadata/absent.csv")
    elseif(control STREQUAL "all_unknown")
      file(READ "${PROBE_LEDGER}" text)
      string(REPLACE "metadata.identity,pass" "metadata.identity,unknown" text "${text}")
      file(WRITE "${PROBE_LEDGER}" "${text}")
    endif()
    execute_process(COMMAND ${EMULATOR} "${RUN}" --probe "${image}"
        --test-ledger "${PROBE_LEDGER}" --test-dat "${PROBE_DAT}"
        --test-root "${root}" --test-config "${PROBE_CONFIG}"
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT rc STREQUAL "0" OR NOT out MATCHES "metadata[.]identity=unknown" OR
       NOT out MATCHES "serial=redacted" OR NOT out MATCHES "rcheevos_md5=redacted" OR
       NOT out MATCHES "track1_sha1=redacted" OR
       out MATCHES "SWAY-00001|SWAY_000[.]01|12481bed29c6c0d54a6f776ef5cec981|${track_sha1}|Private Wrong Title")
      message(FATAL_ERROR "${control} metadata join was accepted or leaked private identity: ${rc}: ${out}${err}")
    endif()
  endforeach()
  return()
endif()

if(CMAKE_SCRIPT_MODE_FILE AND CASE STREQUAL "probe_sha1_vectors")
  if(NOT DEFINED TEST_BINARY OR "${TEST_BINARY}" STREQUAL "")
    message(FATAL_ERROR "SHA-1 vectors require TEST_BINARY")
  endif()
  string(REPLACE "|" ";" EMULATOR "${EMULATOR}")
  execute_process(COMMAND ${EMULATOR} "${TEST_BINARY}" sha1_vectors
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(NOT rc STREQUAL "0")
    message(FATAL_ERROR "streaming SHA-1 known-answer vector failed: ${rc}: ${out}${err}")
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
    set(mode_dir "${work}/${mode}")
    file(MAKE_DIRECTORY "${mode_dir}")
    set(cue "${mode_dir}/${mode}.cue")
    execute_process(COMMAND ${EMULATOR} "${TEST_BINARY}" "${mode}" "${cue}"
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT rc STREQUAL "0")
      message(FATAL_ERROR "cannot generate ${mode} CUE fixture: ${rc}: ${out}${err}")
    endif()
    set(track_file "${mode_dir}/synthetic.bin")
    if(mode STREQUAL "cue_track99")
      set(track_file "${cue}.track1")
    endif()
    file(SHA1 "${track_file}" track_sha1)
    probe_write_metadata("${mode_dir}" "${track_sha1}")
    execute_process(COMMAND ${EMULATOR} "${RUN}" --probe "${cue}"
        --test-ledger "${PROBE_LEDGER}" --test-dat "${PROBE_DAT}"
        --test-root "${mode_dir}" --test-config "${PROBE_CONFIG}"
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT rc STREQUAL "0" OR NOT out MATCHES "format=disc" OR
       NOT out MATCHES "serial=SWAY-00001" OR
       NOT out MATCHES "rcheevos_md5=12481bed29c6c0d54a6f776ef5cec981" OR
       NOT out MATCHES "track1_sha1=${track_sha1}" OR
       NOT out MATCHES "title=Schway Synthetic Metadata Fixture" OR
       NOT out MATCHES "metadata[.]identity=pass" OR
       NOT out MATCHES "boot_status=unknown" OR
       NOT out MATCHES "play_status=unknown")
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
    set(fixture_mode "${mode}")
    if(WIN32 AND mode STREQUAL "cue_name_255")
      set(fixture_mode cue_name_255_no_companion)
    endif()
    execute_process(COMMAND ${EMULATOR} "${TEST_BINARY}" "${fixture_mode}" "${cue}"
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT rc STREQUAL "0")
      message(FATAL_ERROR "cannot generate ${mode} CUE fixture: ${rc}: ${out}${err}")
    endif()
    set(probe_args)
    if(WIN32 AND mode STREQUAL "cue_name_255")
      execute_process(COMMAND ${EMULATOR} "${RUN}" --probe "${cue}"
        RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
      if(rc STREQUAL "0" OR NOT err MATCHES "cannot read CUE track file")
        message(FATAL_ERROR "255-byte CUE name was rejected before the Windows path limit: ${rc}: ${out}${err}")
      endif()
      continue()
    endif()
    if(mode STREQUAL "cue_name_255")
      string(REPEAT "a" 251 name_prefix)
      set(track_file "${case_dir}/${name_prefix}.bin")
    elseif(mode STREQUAL "cue_max_bytes")
      set(track_file "${case_dir}/synthetic.bin")
    endif()
    if(mode STREQUAL "cue_name_255" OR mode STREQUAL "cue_max_bytes")
      file(SHA1 "${track_file}" track_sha1)
      probe_write_metadata("${case_dir}" "${track_sha1}")
      list(APPEND probe_args --test-ledger "${PROBE_LEDGER}" --test-dat "${PROBE_DAT}"
        --test-root "${case_dir}" --test-config "${PROBE_CONFIG}")
    endif()
    execute_process(COMMAND ${EMULATOR} "${RUN}" --probe "${cue}" ${probe_args}
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(mode STREQUAL "cue_name_255" OR mode STREQUAL "cue_max_bytes")
      if(NOT rc STREQUAL "0" OR NOT out MATCHES "format=disc" OR
         NOT out MATCHES "serial=SWAY-00001" OR
         NOT out MATCHES "rcheevos_md5=12481bed29c6c0d54a6f776ef5cec981" OR
         NOT out MATCHES "track1_sha1=${track_sha1}" OR
         NOT out MATCHES "metadata[.]identity=pass" OR
         NOT out MATCHES "boot_status=unknown" OR NOT out MATCHES "play_status=unknown")
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
     NOT out MATCHES "metadata[.]identity=unknown" OR
     NOT out MATCHES "boot_status=unknown" OR NOT out MATCHES "play_status=unknown")
    message(FATAL_ERROR "missing SYSTEM.CNF fallback input was refused or exposed private metadata: ${rc}: ${out}${err}")
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
  execute_process(COMMAND ${EMULATOR} "${TEST_BINARY}" schema "${LEDGER}" RESULT_VARIABLE rc)
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
target_link_libraries(schwaystation-test-probe PRIVATE schwaystation-host)
add_dependencies(schwaystation-test-probe schwaystation-run-test)

add_executable(schwaystation-test-library ${CMAKE_CURRENT_LIST_DIR}/library_tests.c)
set_target_properties(schwaystation-test-library PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-library)
add_dependencies(schwaystation-test-library schwaystation-test-probe schwaystation-run-test)
add_dependencies(schwaystation-test-library schwaystation-run)

string(REPLACE ";" "|" schway__probe_emulator "${CMAKE_CROSSCOMPILING_EMULATOR}")
set(schway__library_test_runner_launcher "")
set(schway__library_production_runner_launcher "")
if(CMAKE_CROSSCOMPILING_EMULATOR)
  set(schway__compat_launcher_dir "${PROJECT_BINARY_DIR}/tests/compat")
  file(MAKE_DIRECTORY "${schway__compat_launcher_dir}")
  set(schway__library_test_runner_launcher "${schway__compat_launcher_dir}/schwaystation-run-test")
  set(schway__library_production_runner_launcher "${schway__compat_launcher_dir}/schwaystation-run")
  schway__compat_write_runner_launcher(schwaystation-run-test "${schway__library_test_runner_launcher}")
  schway__compat_write_runner_launcher(schwaystation-run "${schway__library_production_runner_launcher}")
endif()

# Fault prevented: library output losing an exact track and metadata identity join.
schway_add_test(NAME compat.library.synthetic_result
  COMMAND ${CMAKE_COMMAND} -DSOURCE_ROOT=${PROJECT_SOURCE_DIR} -DWORK_ROOT=${PROJECT_BINARY_DIR}
    -DTEST_PROBE=$<TARGET_FILE:schwaystation-test-probe>
    -DLIBRARY_TEST=$<TARGET_FILE:schwaystation-test-library>
    -DRUN=$<TARGET_FILE:schwaystation-run-test>
    "-DEMULATOR=${schway__probe_emulator}"
    "-DRUNNER_LAUNCHER=${schway__library_test_runner_launcher}" -DCASE=library_synthetic_result
    -P ${PROJECT_SOURCE_DIR}/tests/compat/tests.cmake
  LABELS compat library portable TIMEOUT 30)

# Fault prevented: malformed configuration, joins or inputs replacing a private report.
schway_add_test(NAME compat.library.controls
  COMMAND ${CMAKE_COMMAND} -DSOURCE_ROOT=${PROJECT_SOURCE_DIR} -DWORK_ROOT=${PROJECT_BINARY_DIR}
    -DTEST_PROBE=$<TARGET_FILE:schwaystation-test-probe>
    -DLIBRARY_TEST=$<TARGET_FILE:schwaystation-test-library>
    -DRUN=$<TARGET_FILE:schwaystation-run-test>
    -DPRODUCTION_RUN=$<TARGET_FILE:schwaystation-run>
    "-DEMULATOR=${schway__probe_emulator}"
    "-DRUNNER_LAUNCHER=${schway__library_test_runner_launcher}"
    "-DPRODUCTION_RUNNER_LAUNCHER=${schway__library_production_runner_launcher}"
    -DCASE=library_controls
    -P ${PROJECT_SOURCE_DIR}/tests/compat/tests.cmake
  LABELS compat library portable TIMEOUT 60)

# Fault prevented: a raw ISO probe reporting a different synthetic boot identity.
schway_add_test(NAME compat.probe.synthetic_identity
  COMMAND ${CMAKE_COMMAND} -DWORK_ROOT=${PROJECT_BINARY_DIR}
    -DTEST_BINARY=$<TARGET_FILE:schwaystation-test-probe>
    -DRUN=$<TARGET_FILE:schwaystation-run-test> "-DEMULATOR=${schway__probe_emulator}"
    -DCASE=probe_synthetic_identity -P ${PROJECT_SOURCE_DIR}/tests/compat/tests.cmake
  LABELS compat portable TIMEOUT 30)

# Fault prevented: a matching serial, boot MD5, track SHA-1 and DAT title failing to join.
schway_add_test(NAME compat.probe.full_identity
  COMMAND ${CMAKE_COMMAND} -DWORK_ROOT=${PROJECT_BINARY_DIR}
    -DTEST_BINARY=$<TARGET_FILE:schwaystation-test-probe>
    -DRUN=$<TARGET_FILE:schwaystation-run-test> "-DEMULATOR=${schway__probe_emulator}"
    -DCASE=probe_full_identity -P ${PROJECT_SOURCE_DIR}/tests/compat/tests.cmake
  LABELS compat portable TIMEOUT 30)

# Fault prevented: a track SHA-1 implementation disagreeing with standard vectors.
schway_add_test(NAME compat.probe.sha1_vectors
  COMMAND ${CMAKE_COMMAND}
    -DTEST_BINARY=$<TARGET_FILE:schwaystation-test-probe>
    "-DEMULATOR=${schway__probe_emulator}" -DCASE=probe_sha1_vectors
    -P ${PROJECT_SOURCE_DIR}/tests/compat/tests.cmake
  LABELS compat portable TIMEOUT 30)

# Fault prevented: hostile ISO/CUE inputs hanging or private unlisted identity leaking.
schway_add_test(NAME compat.probe.malformed
  COMMAND ${CMAKE_COMMAND} -DWORK_ROOT=${PROJECT_BINARY_DIR}
    -DTEST_BINARY=$<TARGET_FILE:schwaystation-test-probe>
    -DRUN=$<TARGET_FILE:schwaystation-run-test> "-DEMULATOR=${schway__probe_emulator}"
    -DCASE=probe_malformed -P ${PROJECT_SOURCE_DIR}/tests/compat/tests.cmake
  LABELS compat portable TIMEOUT 30)

# Fault prevented: CUE sector representations changing the canonical boot identity.
schway_add_test(NAME compat.probe.cue_modes
  COMMAND ${CMAKE_COMMAND} -DWORK_ROOT=${PROJECT_BINARY_DIR}
    -DTEST_BINARY=$<TARGET_FILE:schwaystation-test-probe>
    -DRUN=$<TARGET_FILE:schwaystation-run-test> "-DEMULATOR=${schway__probe_emulator}"
    -DCASE=probe_cue_modes -P ${PROJECT_SOURCE_DIR}/tests/compat/tests.cmake
  LABELS compat portable TIMEOUT 30)

# Fault prevented: a CUE timestamp, track, mode, path or input-size boundary being accepted incorrectly.
schway_add_test(NAME compat.probe.cue_edges
  COMMAND ${CMAKE_COMMAND} -DWORK_ROOT=${PROJECT_BINARY_DIR}
    -DTEST_BINARY=$<TARGET_FILE:schwaystation-test-probe>
    -DRUN=$<TARGET_FILE:schwaystation-run-test> "-DEMULATOR=${schway__probe_emulator}"
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
