# The PS-EXE loader and its probe.
# The validator, the probe and the loader through the public header, plus the
# instance's RAM and state. One executable runs the case named on its command
# line. White-box: the test sees src/, so it can read guest RAM directly.

# Run as a script (cmake -DCASE=<case> ... -P tests.cmake), this file is one
# probe check of the runner; include()d by tests/tests.cmake, it registers the tests.
if(CMAKE_SCRIPT_MODE_FILE)
  # Runs the runner with ARGN; sets rc, out (standard output) and err (standard error).
  function(probe_run)
    execute_process(COMMAND ${EMULATOR} ${RUN} ${ARGN} RESULT_VARIABLE rc
      OUTPUT_VARIABLE out ERROR_VARIABLE err)
    set(rc "${rc}" PARENT_SCOPE)
    set(out "${out}" PARENT_SCOPE)
    set(err "${err}" PARENT_SCOPE)
  endfunction()

  file(REMOVE_RECURSE "${DIR}")
  file(MAKE_DIRECTORY "${DIR}")
  if(CASE STREQUAL "probe_refusals")
    # The test program writes one good file and five bad ones. Each bad one,
    # and a file that is not there, exits 1 with the file and a reason on
    # standard error and no report; the good one exits 0 with its report.
    execute_process(COMMAND ${EMULATOR} ${WRITER} write "${DIR}" RESULT_VARIABLE rc
      OUTPUT_VARIABLE out ERROR_VARIABLE out)
    if(NOT rc STREQUAL "0")
      message(FATAL_ERROR "schwaystation-test-exe write: exit ${rc}\n${out}")
    endif()
    foreach(name bad_signature truncated no_entry empty oversized missing)
      probe_run(--probe "${DIR}/${name}.exe")
      if(NOT rc STREQUAL "1" OR NOT err MATCHES "^schwaystation-run: [^\n]*${name}[.]exe: [^\n]+\n$"
          OR NOT out STREQUAL "")
        message(FATAL_ERROR "--probe ${name}.exe: expected exit 1, a one-line error naming the file "
          "and no report, got exit ${rc}\nstdout: ${out}\nstderr: ${err}")
      endif()
    endforeach()
    probe_run(--probe "${DIR}/valid.exe")
    if(NOT rc STREQUAL "0" OR NOT out MATCHES "(^|\n)licence_area=europe\n"
        OR NOT out MATCHES "(^|\n)region_guess=pal\n" OR NOT out MATCHES "(^|\n)exe_name=valid[.]exe\n"
        OR NOT out MATCHES "(^|\n)lint=0x00000000\n" OR NOT err STREQUAL "")
      message(FATAL_ERROR "--probe valid.exe: exit ${rc}\nstdout: ${out}\nstderr: ${err}")
    endif()
  elseif(CASE STREQUAL "probe_usage")
    # --probe stands alone: with another option, with no file or with two
    # files it exits 2 with the usage text, before the file is opened, and
    # writes nothing.
    foreach(args IN ITEMS
        "--probe;f.exe;--frames;1" "--frames;1;--probe;f.exe" "--probe;f.exe;--hash"
        "--probe;f.exe;--png;1:${DIR}/x.png" "--probe;f.exe;--bench;1" "--probe;f.exe;--region;pal"
        "--probe" "--probe;f.exe;g.exe" "--hash;--probe" "--probe;--hash")
      probe_run(${args})
      if(NOT rc STREQUAL "2" OR NOT err MATCHES "usage" OR NOT out STREQUAL "")
        message(FATAL_ERROR "${args}: expected exit 2 and a usage message, got exit ${rc}\n"
          "stdout: ${out}\nstderr: ${err}")
      endif()
    endforeach()
    if(EXISTS "${DIR}/x.png")
      message(FATAL_ERROR "a refused --probe wrote ${DIR}/x.png")
    endif()
  else()
    message(FATAL_ERROR "unknown CASE '${CASE}'")
  endif()
  return()
endif()
add_executable(schwaystation-test-exe ${CMAKE_CURRENT_LIST_DIR}/exe_tests.c
  ${PROJECT_SOURCE_DIR}/runner/md5.c)
target_include_directories(schwaystation-test-exe PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-exe PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-exe PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-exe)

# Fault prevented: a header field read from the wrong offset or byte order, or
# a probe that disagrees with the header it was given.
schway_add_test(NAME exe.probe_valid
  COMMAND schwaystation-test-exe probe_valid
  LABELS exe portable TIMEOUT 10)

# Fault prevented: a hostile header that passes a rule, or a refused file that
# writes a byte of RAM or marks the instance loaded.
schway_add_test(NAME exe.rejects
  COMMAND schwaystation-test-exe rejects
  LABELS exe portable TIMEOUT 10)

# Fault prevented: a limit off by one: text that overruns the end of RAM by a
# sector, a refused empty text, a largest file that is rejected or one byte
# more that is accepted, or a segment alias that loads elsewhere.
schway_add_test(NAME exe.boundaries
  COMMAND schwaystation-test-exe boundaries
  LABELS exe portable TIMEOUT 10)

# Fault prevented: a low-64-KiB overlap or a trailing byte that is refused
# instead of noted, or one that goes unreported.
schway_add_test(NAME exe.lint
  COMMAND schwaystation-test-exe lint
  LABELS exe portable TIMEOUT 10)

# Fault prevented: a region text compared loosely, so one byte off or one byte
# later still names a region.
schway_add_test(NAME exe.licence
  COMMAND schwaystation-test-exe licence
  LABELS exe portable TIMEOUT 10)

# Fault prevented: a text segment at the wrong physical address, a launch
# block with the wrong words, or any other byte of the instance changed by a
# load.
schway_add_test(NAME exe.load_writes
  COMMAND schwaystation-test-exe load_writes
  LABELS exe portable TIMEOUT 10)

# Fault prevented: a refused load that leaves any byte of the saved state
# changed.
schway_add_test(NAME exe.error_changes_nothing
  COMMAND schwaystation-test-exe error_changes_nothing
  LABELS exe portable TIMEOUT 10)

# Fault prevented: a load after the first frame that changes the running
# instance, or a power reset that does not allow a new one.
schway_add_test(NAME exe.after_first_frame
  COMMAND schwaystation-test-exe after_first_frame
  LABELS exe portable TIMEOUT 10)

# Fault prevented: a debug-port bit that is not stored, an unknown flag bit
# that is accepted, or an older caller's profile that keeps a stale bit.
schway_add_test(NAME exe.profile_flags
  COMMAND schwaystation-test-exe profile_flags
  LABELS exe portable TIMEOUT 10)

# Fault prevented: a runner that accepts a file the core refuses, prints a
# report for it, or fails without naming the file, so a script cannot tell a
# bad image from a good one.
string(REPLACE ";" "$<SEMICOLON>" schway__exe_emulator "${CMAKE_CROSSCOMPILING_EMULATOR}")
set(schway__probe_check ${CMAKE_COMMAND} -DRUN=$<TARGET_FILE:schwaystation-run>
  "-DEMULATOR=${schway__exe_emulator}" -DWRITER=$<TARGET_FILE:schwaystation-test-exe>)
schway_add_test(NAME exe.probe_refusals
  COMMAND ${schway__probe_check} -DCASE=probe_refusals -DDIR=${PROJECT_BINARY_DIR}/tests/probe_refusals
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS exe portable TIMEOUT 60)

# Fault prevented: --probe combined with a run option, so a script that meant
# to probe starts a run, or one that forgot the file gets a report for nothing.
schway_add_test(NAME exe.probe_usage
  COMMAND ${schway__probe_check} -DCASE=probe_usage -DDIR=${PROJECT_BINARY_DIR}/tests/probe_usage
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS exe portable TIMEOUT 60)

# Fault prevented: a wrong MD5 round, padding or length, so the achievement
# hash the probe prints names no real image.
schway_add_test(NAME exe.md5_rfc1321
  COMMAND schwaystation-test-exe md5_rfc1321
  LABELS exe portable TIMEOUT 10)
