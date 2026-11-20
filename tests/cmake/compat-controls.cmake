cmake_minimum_required(VERSION 3.25)
if(NOT CASE OR NOT SOURCE_ROOT OR NOT WORK_ROOT)
  message(FATAL_ERROR "compatibility controls require CASE, SOURCE_ROOT and WORK_ROOT")
endif()
execute_process(COMMAND sh "${SOURCE_ROOT}/scripts/compat-controls.sh" "${CASE}" "${SOURCE_ROOT}" "${WORK_ROOT}"
  RESULT_VARIABLE result)
if(NOT "${result}" STREQUAL "0")
  message(FATAL_ERROR "compatibility controls failed: ${result}")
endif()
