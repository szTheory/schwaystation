# Instructions-per-frame gate tests.
#
# This file is also the checks themselves, run as
#   cmake -DSCRIPT=<perf_compare.cmake> -DDIR=<fixture dir> -DCASE=<name> -P tests/perf/tests.cmake
# Each case runs the gate on synthetic Cachegrind Ir per frame values through
# expect_fail.cmake, so it passes only on the exact exit status and verdict.
# The date is fixed, so no case depends on the clock. The real Valgrind run is
# only in the CI perf job.
if(CMAKE_SCRIPT_MODE_FILE STREQUAL CMAKE_CURRENT_LIST_FILE)
  set(today 2026-01-02)

  # Runs the gate in mode with ARGN; requires exit code and output matching regex.
  function(gate code mode regex)
    set(ENV{SCHWAYSTATION_PERF_MODE} ${mode})
    set(CMD ${CMAKE_COMMAND} -DFIXTURE=testcard -DTODAY=${today} ${ARGN} -P "${SCRIPT}")
    set(CODE ${code})
    set(REGEX "${regex}")
    include(${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../cmake/expect_fail.cmake)
  endfunction()

  set(empty -DALLOW=${DIR}/empty.txt)
  set(base -DBASE1=1000000 -DBASE2=1000000)
  if(CASE STREQUAL "identical")
    # Fault prevented: an unchanged core reported slower.
    gate(0 gate "head=1000000 base=1000000 delta_bp=0 verdict=PASS" -DHEAD=1000000 ${base} ${empty})
  elseif(CASE STREQUAL "plus_1_00")
    # Fault prevented: the boundary moving inside the limit, so exactly +1.00 % fails.
    gate(0 gate "delta_bp=100 verdict=PASS" -DHEAD=1010000 ${base} ${empty})
  elseif(CASE STREQUAL "plus_1_01")
    # Fault prevented: a slowdown just past +1.00 % passing in gate mode, or
    # failing in report mode.
    gate(1 gate "delta_bp=101 verdict=FAIL" -DHEAD=1010100 ${base} ${empty})
    gate(0 report "delta_bp=101 verdict=FAIL" -DHEAD=1010100 ${base} ${empty})
  elseif(CASE STREQUAL "unstable")
    # Fault prevented: a noisy base measurement read as a verdict.
    gate(1 gate "base=1000000 delta_bp=0 verdict=UNSTABLE"
      -DHEAD=1000000 -DBASE1=1000000 -DBASE2=1000001 ${empty})
  elseif(CASE STREQUAL "allowed")
    # Fault prevented: an allow line ignored while current, or honoured after
    # its expiry.
    gate(0 gate "delta_bp=200 verdict=PASS" -DHEAD=1020000 ${base} -DALLOW=${DIR}/current.txt)
    gate(1 gate "delta_bp=200 verdict=FAIL" -DHEAD=1020000 ${base} -DALLOW=${DIR}/expired.txt)
  elseif(CASE STREQUAL "no_base")
    # Fault prevented: a missing base tree passing in gate mode, failing in
    # report mode, or hiding a missing head measurement.
    set(none -DNO_BASE=true -DBASE1= -DBASE2=)
    gate(0 report "head=1000000 base=none delta_bp=none verdict=NO_BASE" -DHEAD=1000000 ${none} ${empty})
    gate(1 gate "verdict=NO_BASE" -DHEAD=1000000 ${none} ${empty})
    gate(1 report "perf: error HEAD" -DHEAD= ${none} ${empty})
  elseif(CASE STREQUAL "missing")
    # Fault prevented: an empty base measurement read as a pass, or an unset
    # mode read as report.
    gate(1 report "perf: error BASE1" -DHEAD=1000000 -DBASE1= -DBASE2=1000000 ${empty})
    gate(1 "" "perf: error SCHWAYSTATION_PERF_MODE" -DHEAD=1000000 ${base} ${empty})
  else()
    message(FATAL_ERROR "perf tests: unknown CASE '${CASE}'")
  endif()
  return()
endif()

set(perf_dir ${CMAKE_BINARY_DIR}/perf)
file(WRITE ${perf_dir}/empty.txt "# no exceptions\n")
file(WRITE ${perf_dir}/current.txt "testcard 300 2026-02-01 a reviewed slowdown\n")
file(WRITE ${perf_dir}/expired.txt "testcard 300 2026-01-01 a reviewed slowdown\n")
foreach(case allowed identical missing no_base plus_1_00 plus_1_01 unstable)
  schway_add_test(NAME perf.gate.${case}
    COMMAND ${CMAKE_COMMAND} -DSCRIPT=${CMAKE_CURRENT_LIST_DIR}/../cmake/perf_compare.cmake
      -DDIR=${perf_dir} -DCASE=${case} -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS perf TIMEOUT 10)
endforeach()
