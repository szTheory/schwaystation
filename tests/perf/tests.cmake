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
  set(fixture testcard)
  set(spu_idle_allowance 4000000)

  # One NTSC frame has 737 sample events and 24 voice slots per event. The
  # four-million instruction allowance is about 226 instructions per slot,
  # enough for bounded event overhead while rejecting per-cycle polling
  # (.planning/preparation/ARCHITECTURE.md section 7;
  # .planning/preparation/PSX-SPU-CDROM-AND-MDEC.md section 1.1).
  function(spu_idle_parse_report out path role)
    if(NOT EXISTS "${path}")
      message(FATAL_ERROR "perf: error missing ${role} Cachegrind report")
    endif()
    file(STRINGS "${path}" summaries REGEX "^summary:")
    list(LENGTH summaries summary_count)
    if(NOT summary_count EQUAL 1)
      message(FATAL_ERROR "perf: error ${role} Cachegrind report needs one summary")
    endif()
    list(GET summaries 0 summary)
    if(NOT summary MATCHES "^summary:[ \t]*([0-9]+)$")
      message(FATAL_ERROR "perf: error malformed ${role} Cachegrind summary")
    endif()
    set(count "${CMAKE_MATCH_1}")
    string(LENGTH "${count}" count_length)
    if(count_length GREATER 18)
      message(FATAL_ERROR "perf: error ${role} Cachegrind count exceeds the comparison range")
    endif()
    set(${out} "${count}" PARENT_SCOPE)
  endfunction()

  function(spu_idle_compare control idle)
    if(NOT "${control}" MATCHES "^[0-9]+$" OR NOT "${idle}" MATCHES "^[0-9]+$")
      message(FATAL_ERROR "perf: error SPU idle counts must be unsigned integers")
    endif()
    if(idle LESS control)
      message(FATAL_ERROR "perf: error SPU idle instruction count is below control")
    endif()
    math(EXPR extra "${idle} - ${control}")
    if(extra GREATER spu_idle_allowance)
      message(FATAL_ERROR "perf: error SPU idle work ${extra} exceeds allowance ${spu_idle_allowance}")
    endif()
    message(STATUS "perf spu_idle control=${control} idle=${idle} extra=${extra} allowance=${spu_idle_allowance} verdict=PASS")
  endfunction()

  function(spu_idle_compare_reports)
    foreach(variable CONTROL_STATUS IDLE_STATUS CONTROL_REPORT IDLE_REPORT)
      if(NOT DEFINED ${variable})
        message(FATAL_ERROR "perf: error ${variable} is required")
      endif()
    endforeach()
    if(NOT "${CONTROL_STATUS}" STREQUAL "0")
      message(FATAL_ERROR "perf: error SPU idle control producer exited ${CONTROL_STATUS}")
    endif()
    if(NOT "${IDLE_STATUS}" STREQUAL "0")
      message(FATAL_ERROR "perf: error SPU idle producer exited ${IDLE_STATUS}")
    endif()
    spu_idle_parse_report(control "${CONTROL_REPORT}" control)
    spu_idle_parse_report(idle "${IDLE_REPORT}" idle)
    spu_idle_compare("${control}" "${idle}")
  endfunction()

  function(spu_idle_measure)
    foreach(variable VALGRIND UNIT OUT)
      if(NOT DEFINED ${variable} OR "${${variable}}" STREQUAL "")
        message(FATAL_ERROR "perf: error ${variable} is required for CASE=spu_idle")
      endif()
    endforeach()
    if(NOT EXISTS "${UNIT}")
      message(FATAL_ERROR "perf: error UNIT does not exist")
    endif()
    file(MAKE_DIRECTORY "${OUT}")
    foreach(mode control idle)
      set(report "${OUT}/spu-${mode}.cachegrind")
      file(REMOVE "${report}")
      execute_process(
        COMMAND "${VALGRIND}" --tool=cachegrind --cache-sim=no --error-exitcode=99
          "--cachegrind-out-file=${report}" "${UNIT}" "spu_idle_${mode}" 1
        RESULT_VARIABLE status OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr)
      if(NOT "${status}" STREQUAL "0")
        message(FATAL_ERROR "perf: error SPU idle ${mode} producer exited ${status}: ${stderr}${stdout}")
      endif()
      if(mode STREQUAL "control")
        set(control_report "${report}")
      else()
        set(idle_report "${report}")
      endif()
    endforeach()
    set(CONTROL_STATUS 0)
    set(IDLE_STATUS 0)
    set(CONTROL_REPORT "${control_report}")
    set(IDLE_REPORT "${idle_report}")
    spu_idle_compare_reports()
  endfunction()

  # Runs the gate in mode with ARGN; requires exit code and output matching regex.
  function(gate code mode regex)
    set(ENV{SCHWAYSTATION_PERF_MODE} ${mode})
    set(CMD ${CMAKE_COMMAND} -DFIXTURE=${fixture} -DTODAY=${today} ${ARGN} -P "${SCRIPT}")
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
  elseif(CASE STREQUAL "gpu_motion_slow")
    # Fault prevented: a deliberately slower CPU/GPU workload accepted by its separate row.
    set(fixture gpu_motion)
    gate(1 gate "delta_bp=101 verdict=FAIL" -DHEAD=1010100 ${base} ${empty})
  elseif(CASE STREQUAL "spu_idle")
    spu_idle_measure()
  elseif(CASE STREQUAL "spu_idle_compare_reports")
    spu_idle_compare_reports()
  elseif(CASE MATCHES "^spu_idle_(valid|boundary|over|missing|malformed|nonzero|mutated)$")
    set(fixture_dir "${DIR}/spu_idle/${CASE}")
    file(MAKE_DIRECTORY "${fixture_dir}")
    set(control_report "${fixture_dir}/control.out")
    set(idle_report "${fixture_dir}/idle.out")
    file(WRITE "${control_report}" "events: Ir\nsummary: 1000000\n")
    file(WRITE "${idle_report}" "events: Ir\nsummary: 4500000\n")
    set(expected_code 0)
    set(expected_regex "perf spu_idle control=1000000 idle=4500000 extra=3500000 allowance=4000000 verdict=PASS")
    set(control_status 0)
    set(idle_status 0)
    if(CASE STREQUAL "spu_idle_boundary")
      file(WRITE "${idle_report}" "events: Ir\nsummary: 5000000\n")
      set(expected_regex "extra=4000000 allowance=4000000 verdict=PASS")
    elseif(CASE STREQUAL "spu_idle_over")
      file(WRITE "${idle_report}" "events: Ir\nsummary: 5000001\n")
      set(expected_code 1)
      set(expected_regex "perf: error SPU idle work 4000001 exceeds allowance 4000000")
    elseif(CASE STREQUAL "spu_idle_missing")
      file(REMOVE "${control_report}")
      set(expected_code 1)
      set(expected_regex "perf: error missing control Cachegrind report")
    elseif(CASE STREQUAL "spu_idle_malformed")
      file(WRITE "${control_report}" "events: Ir\nsummary: malformed\n")
      set(expected_code 1)
      set(expected_regex "perf: error malformed control Cachegrind summary")
    elseif(CASE STREQUAL "spu_idle_nonzero")
      set(control_status 23)
      set(expected_code 1)
      set(expected_regex "perf: error SPU idle control producer exited 23")
    elseif(CASE STREQUAL "spu_idle_mutated")
      file(WRITE "${idle_report}" "events: Ir\nsummary: 999999\n")
      set(expected_code 1)
      set(expected_regex "perf: error SPU idle instruction count is below control")
    endif()
    set(CMD ${CMAKE_COMMAND} "-DCASE=spu_idle_compare_reports"
      "-DCONTROL_STATUS=${control_status}" "-DIDLE_STATUS=${idle_status}"
      "-DCONTROL_REPORT=${control_report}" "-DIDLE_REPORT=${idle_report}"
      -P "${CMAKE_CURRENT_LIST_FILE}")
    set(CODE ${expected_code})
    set(REGEX "${expected_regex}")
    include(${CMAKE_CURRENT_LIST_DIR}/../cmake/expect_fail.cmake)
  else()
    message(FATAL_ERROR "perf tests: unknown CASE '${CASE}'")
  endif()
  return()
endif()

set(perf_dir ${CMAKE_BINARY_DIR}/perf)
file(WRITE ${perf_dir}/empty.txt "# no exceptions\n")
file(WRITE ${perf_dir}/current.txt "testcard 300 2026-02-01 a reviewed slowdown\n")
file(WRITE ${perf_dir}/expired.txt "testcard 300 2026-01-01 a reviewed slowdown\n")
foreach(case allowed gpu_motion_slow identical missing no_base plus_1_00 plus_1_01 unstable
    spu_idle_boundary spu_idle_malformed spu_idle_missing spu_idle_mutated spu_idle_nonzero
    spu_idle_over spu_idle_valid)
  schway_add_test(NAME perf.gate.${case}
    COMMAND ${CMAKE_COMMAND} -DSCRIPT=${CMAKE_CURRENT_LIST_DIR}/../cmake/perf_compare.cmake
      -DDIR=${perf_dir} -DCASE=${case} -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS perf TIMEOUT 10)
endforeach()
