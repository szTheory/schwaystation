# The documented exit statuses, run as
#   cmake -DRUN=<schwaystation-run> [-DEMULATOR=<list>] -DDOC=<docs/runner.md> -P exit_table.cmake
# Fault prevented: a documented exit status that differs from what the runner returns. Every table
# line --help prints after "exit status:" must appear as a whole line in the document, and the
# document must have no other line in that form.
string(REPLACE "|" ";" EMULATOR "${EMULATOR}")
execute_process(COMMAND ${EMULATOR} "${RUN}" --help OUTPUT_VARIABLE out RESULT_VARIABLE rc)
string(FIND "${out}" "exit status:\n" at)
if(NOT rc STREQUAL "0" OR at EQUAL -1)
  message(FATAL_ERROR "schwaystation-run --help: expected exit 0 and an exit status table, got exit ${rc}\n${out}")
endif()
math(EXPR at "${at} + 13")
string(SUBSTRING "${out}" ${at} -1 table)
string(REGEX MATCHALL "[^\n]+" rows "${table}")
file(READ "${DOC}" doc)
foreach(row IN LISTS rows)
  string(FIND "\n${doc}" "\n${row}\n" found)
  if(found EQUAL -1)
    message(FATAL_ERROR "${DOC} lacks the exit status line --help prints:\n${row}")
  endif()
endforeach()
list(LENGTH rows n)
# And the other way: the document lists no status that --help does not.
string(REGEX MATCHALL "\n  [0-9]+  [a-z]+ [^\n]*" doc_rows "${doc}")
list(LENGTH doc_rows doc_n)
if(NOT doc_n EQUAL n)
  message(FATAL_ERROR "${DOC} has ${doc_n} exit status lines; --help prints ${n}")
endif()
message(STATUS "exit_table: ${n} lines of --help found in ${DOC}")
