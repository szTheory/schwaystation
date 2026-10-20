# cmake -DRUN=<runner> [-DEMULATOR=<cmd>] -DSOURCE=<dir> [-DREVISION=<hex>] -P revision.cmake
# Fault prevented: testing a stale configure after a commit. The revision
# --version prints, embedded at configure, must be the source tree's HEAD;
# -DREVISION replaces the printed value, so a self-test can force a mismatch.
foreach(var RUN SOURCE)
  if(NOT DEFINED ${var} OR "${${var}}" STREQUAL "")
    message(FATAL_ERROR "revision: -D${var} is required")
  endif()
endforeach()
if(NOT DEFINED REVISION)
  execute_process(COMMAND ${EMULATOR} ${RUN} --version RESULT_VARIABLE rc OUTPUT_VARIABLE out)
  if(NOT rc STREQUAL "0" OR NOT out MATCHES " ([0-9a-f]+|unknown)\n$")
    message(FATAL_ERROR "revision: ${RUN} --version exited ${rc} with\n${out}")
  endif()
  set(REVISION ${CMAKE_MATCH_1})
endif()
find_package(Git REQUIRED QUIET)
execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse HEAD WORKING_DIRECTORY ${SOURCE}
  RESULT_VARIABLE rc OUTPUT_VARIABLE head OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT rc STREQUAL "0")
  message(FATAL_ERROR "revision: git rev-parse HEAD failed in ${SOURCE}")
endif()
if(NOT REVISION STREQUAL head)
  message(FATAL_ERROR "revision: the build was configured at ${REVISION} but HEAD is ${head}; "
    "configure again")
endif()
