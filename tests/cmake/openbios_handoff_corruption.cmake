# Fault prevented: corrupted or missing run-local firmware being accepted.
foreach(required OPENBIOS_INPUT EXPECTED_FILE OPENBIOS_CHECK_SCRIPT)
  if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
    message(FATAL_ERROR "OpenBIOS handoff control: -D${required} is required")
  endif()
endforeach()
if(NOT EXISTS "${OPENBIOS_INPUT}" OR NOT EXISTS "${EXPECTED_FILE}"
   OR NOT EXISTS "${OPENBIOS_CHECK_SCRIPT}")
  message(FATAL_ERROR "OpenBIOS handoff control: input, digest or checker is missing")
endif()

get_filename_component(openbios_input_dir "${OPENBIOS_INPUT}" DIRECTORY)
set(openbios_control_dir "${openbios_input_dir}/handoff-controls")
file(MAKE_DIRECTORY "${openbios_control_dir}")
set(openbios_corrupt_input "${openbios_control_dir}/openbios-corrupt.bin")
file(COPY_FILE "${OPENBIOS_INPUT}" "${openbios_corrupt_input}")
file(APPEND "${openbios_corrupt_input}" "x")

function(expect_openbios_refusal label input pattern)
  execute_process(
    COMMAND "${CMAKE_COMMAND}" "-DOPENBIOS_INPUT=${input}"
      "-DEXPECTED_FILE=${EXPECTED_FILE}" -P "${OPENBIOS_CHECK_SCRIPT}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error)
  string(CONCAT output "${output}" "${error}")
  if(result STREQUAL "0" OR NOT output MATCHES "${pattern}")
    message(FATAL_ERROR "OpenBIOS handoff control accepted ${label} or failed unexpectedly:\n${output}")
  endif()
  message(STATUS "OpenBIOS handoff control refused ${label}")
endfunction()

expect_openbios_refusal("one-byte-corrupted transfer" "${openbios_corrupt_input}"
  "OpenBIOS: expected 524288 bytes")
set(openbios_missing_input "${openbios_control_dir}/missing.bin")
file(REMOVE "${openbios_missing_input}")
expect_openbios_refusal("missing transfer" "${openbios_missing_input}"
  "OpenBIOS: input is missing or not a file")
