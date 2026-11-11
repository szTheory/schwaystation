# schwaystation-run: frame hashes, PNG output, bench report, usage and exit statuses, build revision.
#
# Run as a script (cmake -DCASE=<case> ... -P tests.cmake), this file is one
# runner check; include()d by tests/tests.cmake, it registers the tests.
if(CMAKE_SCRIPT_MODE_FILE)
  string(REPLACE "|" ";" EMULATOR "${EMULATOR}")
  # Runs the runner with ARGN and fails unless it exits with code and its
  # merged output matches regex.
  function(expect_exit code regex)
    execute_process(COMMAND ${EMULATOR} ${RUN} ${ARGN} RESULT_VARIABLE rc
      OUTPUT_VARIABLE out ERROR_VARIABLE out)
    if(NOT rc STREQUAL code OR NOT out MATCHES "${regex}")
      message(FATAL_ERROR "schwaystation-run ${ARGN}: expected exit ${code} and output matching\n"
        "  ${regex}\ngot exit ${rc} and output\n${out}")
    endif()
  endfunction()

  # Runs the runner with ARGN through expect_fail.cmake, which passes only
  # for exit 2 and a usage message.
  function(refuse)
    set(CMD ${EMULATOR} ${RUN} ${ARGN})
    set(CODE 2)
    set(REGEX "usage")
    include(${CMAKE_CURRENT_LIST_DIR}/../cmake/expect_fail.cmake)
  endfunction()

  # Runs one case of the bench unit test and fails unless it exits 0.
  function(unit case)
    execute_process(COMMAND ${EMULATOR} ${UNIT} ${case} RESULT_VARIABLE rc
      OUTPUT_VARIABLE out ERROR_VARIABLE out)
    if(NOT rc STREQUAL "0")
      message(FATAL_ERROR "schwaystation-test-bench ${case}: exit ${rc}\n${out}")
    endif()
  endfunction()

  if(CASE STREQUAL "bench_format")
    # The report's text from fixed samples, then the runner's own report:
    # every line present, in order, as a whole number. Times are never
    # compared, as they vary from run to run.
    unit(format)
    set(num "[0-9]+")
    set(report "frames 30\np50_us ${num}\np90_us ${num}\np99_us ${num}\np99[.]9_us ${num}\n"
      "p99[.]99_us ${num}\np99[.]999_us ${num}\nmax_us ${num}\nmax_rss_kib ${num}\n")
    string(CONCAT report ${report})
    expect_exit(0 "^${report}$" --bench 30)
    # One frame: every percentile is the one sample, so all seven times are
    # equal.
    execute_process(COMMAND ${EMULATOR} ${RUN} --bench 1 RESULT_VARIABLE rc OUTPUT_VARIABLE out)
    string(REGEX MATCHALL "_us [0-9]+" times "${out}")
    list(REMOVE_DUPLICATES times)
    list(LENGTH times n)
    if(NOT rc STREQUAL "0" OR NOT out MATCHES "^frames 1\n" OR NOT n EQUAL 1)
      message(FATAL_ERROR "--bench 1: expected exit 0 and seven equal times, got exit ${rc}\n${out}")
    endif()
  elseif(CASE STREQUAL "bench_bytes")
    # The sample buffer is sized exactly up to the cap; a count that is zero,
    # negative, missing or above the command line's limit is refused before
    # anything runs, never a wrapped allocation.
    unit(bytes)
    refuse(--bench 0)
    refuse(--bench -1)
    refuse(--bench)
    refuse(--bench 1000001)
    refuse(--bench 1073741824)
    refuse(--bench 4294967295)
  elseif(CASE STREQUAL "stop")
    # The flag, then a runner built to raise SIGTERM inside frame 5: the run
    # ends after that frame, still prints what was asked, and exits 0.
    unit(stop)
    set(RUN ${HOOKED})
    expect_exit(0 "^frame 5 sha256 ${EXPECTED}\n$" --frames 60 --hash)
    expect_exit(0 "^frames 5\n.*max_rss_kib [0-9]+\n$" --bench 60)
  elseif(CASE STREQUAL "cards_persistence")
    set(RUN ${HOOKED})
    string(REPEAT "[0-9a-f]" 64 digits)
    set(card_dir "${OUT}/runner-cards-persistence")
    file(MAKE_DIRECTORY "${card_dir}")
    file(WRITE "${card_dir}/schwaystation.srm" "old-slot-one")
    file(WRITE "${card_dir}/schwaystation.1.mcr" "old-slot-two")
    file(SHA256 "${card_dir}/schwaystation.1.mcr" old_slot_two)
    expect_exit(0 "frame 3 sha256 ${digits}" --frames 3 --hash --save-dir "${card_dir}"
      --test-card-write slot1)
    file(SIZE "${card_dir}/schwaystation.srm" slot_one_size)
    file(READ "${card_dir}/schwaystation.srm" slot_one_sector HEX OFFSET 896 LIMIT 8)
    file(SHA256 "${card_dir}/schwaystation.1.mcr" new_slot_two)
    if(NOT slot_one_size EQUAL 131072 OR NOT slot_one_sector STREQUAL "314e6b88a5c2dffc" OR
       NOT old_slot_two STREQUAL new_slot_two)
      message(FATAL_ERROR "normal exit did not save only slot 1's completed card generation")
    endif()
  elseif(CASE STREQUAL "cards_sigterm")
    set(RUN ${HOOKED})
    string(REPEAT "[0-9a-f]" 64 digits)
    set(card_dir "${OUT}/runner-cards-sigterm")
    file(MAKE_DIRECTORY "${card_dir}")
    file(WRITE "${card_dir}/schwaystation.srm" "unchanged-slot-one")
    file(SHA256 "${card_dir}/schwaystation.srm" old_slot_one)
    expect_exit(0 "frame 5 sha256 ${EXPECTED}" --frames 60 --hash --save-dir "${card_dir}"
      --test-card-write slot2)
    file(SIZE "${card_dir}/schwaystation.1.mcr" slot_two_size)
    file(READ "${card_dir}/schwaystation.1.mcr" slot_two_sector HEX OFFSET 896 LIMIT 8)
    file(SHA256 "${card_dir}/schwaystation.srm" new_slot_one)
    if(NOT slot_two_size EQUAL 131072 OR NOT slot_two_sector STREQUAL "627f9cb9d6f3102d" OR
       NOT old_slot_one STREQUAL new_slot_one)
      message(FATAL_ERROR "SIGTERM did not flush only slot 2 after frame 5")
    endif()
  elseif(CASE STREQUAL "cards_controls")
    set(RUN ${HOOKED})
    string(REPEAT "[0-9a-f]" 64 digits)

    # Missing cards begin as formatted blanks and stay absent until a game writes them.
    set(missing_dir "${OUT}/runner-cards-missing")
    file(MAKE_DIRECTORY "${missing_dir}")
    expect_exit(0 "frame 1 sha256 ${digits}" --frames 1 --hash --save-dir "${missing_dir}")
    if(EXISTS "${missing_dir}/schwaystation.srm" OR EXISTS "${missing_dir}/schwaystation.1.mcr")
      message(FATAL_ERROR "an unchanged blank card was written")
    endif()

    # An imported short raw image is preserved byte-for-byte when its generation is unchanged.
    set(unchanged_dir "${OUT}/runner-cards-unchanged")
    file(MAKE_DIRECTORY "${unchanged_dir}")
    file(WRITE "${unchanged_dir}/schwaystation.srm" "unchanged-short-card")
    file(SHA256 "${unchanged_dir}/schwaystation.srm" old_unchanged)
    expect_exit(0 "frame 1 sha256 ${digits}" --frames 1 --hash --save-dir "${unchanged_dir}")
    file(SHA256 "${unchanged_dir}/schwaystation.srm" new_unchanged)
    if(NOT old_unchanged STREQUAL new_unchanged OR EXISTS "${unchanged_dir}/schwaystation.1.mcr")
      message(FATAL_ERROR "an unchanged short card was replaced or an absent slot was created")
    endif()

    # Empty imports fail before the first frame and leave the source untouched.
    set(invalid_dir "${OUT}/runner-cards-invalid-import")
    file(MAKE_DIRECTORY "${invalid_dir}")
    file(WRITE "${invalid_dir}/schwaystation.srm" "")
    file(SHA256 "${invalid_dir}/schwaystation.srm" old_invalid)
    execute_process(COMMAND ${EMULATOR} ${RUN} --frames 1 --hash --save-dir "${invalid_dir}"
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    file(SHA256 "${invalid_dir}/schwaystation.srm" new_invalid)
    if(NOT rc STREQUAL "1" OR NOT err MATCHES "cannot import card file" OR
       out MATCHES "frame 1 sha256" OR NOT old_invalid STREQUAL new_invalid)
      message(FATAL_ERROR "an invalid card did not fail before execution and preserve its source\n${out}${err}")
    endif()

    # Half-transactions and rejected checksums cannot dirty a durable generation.
    foreach(mode partial1 bad1)
      set(preserve_dir "${OUT}/runner-cards-${mode}")
      file(MAKE_DIRECTORY "${preserve_dir}")
      file(WRITE "${preserve_dir}/schwaystation.srm" "durable-${mode}")
      file(SHA256 "${preserve_dir}/schwaystation.srm" old_preserved)
      expect_exit(0 "frame 1 sha256 ${digits}" --frames 1 --hash --save-dir "${preserve_dir}"
        --test-card-write "${mode}")
      file(SHA256 "${preserve_dir}/schwaystation.srm" new_preserved)
      if(NOT old_preserved STREQUAL new_preserved)
        message(FATAL_ERROR "${mode} changed a card without a completed successful write")
      endif()
    endforeach()

    # A successful slot 1 write followed by a rejected write flushes only the completed data.
    set(rejected_after_success_dir "${OUT}/runner-cards-rejected-after-success")
    file(MAKE_DIRECTORY "${rejected_after_success_dir}")
    expect_exit(0 "frame 1 sha256 ${digits}" --frames 1 --hash --save-dir "${rejected_after_success_dir}"
      --test-card-write slot1_then_bad)
    file(SIZE "${rejected_after_success_dir}/schwaystation.srm" slot_one_size)
    file(READ "${rejected_after_success_dir}/schwaystation.srm" slot_one_sector HEX OFFSET 896 LIMIT 8)
    if(NOT slot_one_size EQUAL 131072 OR NOT slot_one_sector STREQUAL "314e6b88a5c2dffc" OR
       EXISTS "${rejected_after_success_dir}/schwaystation.1.mcr")
      message(FATAL_ERROR "a rejected write replaced completed data or dirtied the other slot")
    endif()

    # A slot 2 write must preserve slot 1 and create only slot 2.
    set(slot_two_dir "${OUT}/runner-cards-slot-two")
    file(MAKE_DIRECTORY "${slot_two_dir}")
    file(WRITE "${slot_two_dir}/schwaystation.srm" "keep-slot-one")
    file(SHA256 "${slot_two_dir}/schwaystation.srm" old_slot_one)
    expect_exit(0 "frame 1 sha256 ${digits}" --frames 1 --hash --save-dir "${slot_two_dir}"
      --test-card-write slot2)
    file(SHA256 "${slot_two_dir}/schwaystation.srm" new_slot_one)
    file(SIZE "${slot_two_dir}/schwaystation.1.mcr" slot_two_size)
    file(READ "${slot_two_dir}/schwaystation.1.mcr" slot_two_sector HEX OFFSET 896 LIMIT 8)
    if(NOT old_slot_one STREQUAL new_slot_one OR NOT slot_two_size EQUAL 131072 OR
       NOT slot_two_sector STREQUAL "627f9cb9d6f3102d")
      message(FATAL_ERROR "slot 2 persistence changed the wrong slot or saved incorrect data")
    endif()

    # Explicit serial naming creates only the serial card paths.
    set(serial_dir "${OUT}/runner-cards-serial")
    file(MAKE_DIRECTORY "${serial_dir}")
    expect_exit(0 "frame 1 sha256 ${digits}" --frames 1 --hash --save-dir "${serial_dir}"
      --memcard-names serial --serial SLUS_123.45 --test-card-write slot1)
    if(NOT EXISTS "${serial_dir}/SLUS_123.45.srm" OR
       EXISTS "${serial_dir}/schwaystation.srm")
      message(FATAL_ERROR "serial card naming did not use the validated serial basename")
    endif()

    # A forced atomic replacement failure returns status 1, preserves the old card, and cleans its temp.
    set(failed_save_dir "${OUT}/runner-cards-failed-save")
    file(MAKE_DIRECTORY "${failed_save_dir}")
    file(WRITE "${failed_save_dir}/schwaystation.srm" "durable-before-failure")
    file(SHA256 "${failed_save_dir}/schwaystation.srm" old_save)
    expect_exit(1 "cannot save card file" --frames 1 --save-dir "${failed_save_dir}"
      --test-card-write slot1 --test-save-failure rename)
    file(SHA256 "${failed_save_dir}/schwaystation.srm" new_save)
    file(GLOB temporary_files "${failed_save_dir}/*.schwaystation.tmp.*")
    if(NOT old_save STREQUAL new_save OR temporary_files)
      message(FATAL_ERROR "a failed atomic replacement damaged the old card or left a temporary file")
    endif()

    # A missing save directory is not created and cannot produce a successful dirty-card exit.
    set(no_directory "${OUT}/runner-cards-no-directory")
    file(REMOVE_RECURSE "${no_directory}")
    expect_exit(1 "cannot save card file" --frames 1 --save-dir "${no_directory}"
      --test-card-write slot1)
    if(EXISTS "${no_directory}")
      message(FATAL_ERROR "the runner created a save directory that the caller did not provide")
    endif()

    # Invalid naming options are refused as usage errors before any run.
    refuse(--serial SLUS_123.45)
    refuse(--save-dir "${serial_dir}" --memcard-names serial --serial ../escape)
  elseif(CASE STREQUAL "hash_state")
    # One well-formed line, the same twice, and a different one a frame later. CMake regexes have no
    # {64}, so the sixty-four digits are repeated.
    string(REPEAT "[0-9a-f]" 64 digits)
    foreach(frames 5 5 6)
      execute_process(COMMAND ${EMULATOR} ${RUN} --frames ${frames} --hash-state RESULT_VARIABLE rc
        OUTPUT_VARIABLE out ERROR_VARIABLE err)
      if(NOT rc STREQUAL "0" OR NOT out MATCHES "^state sha256 ${digits}\n$")
        message(FATAL_ERROR "--frames ${frames} --hash-state: expected exit 0 and one 64-digit line, "
          "got exit ${rc}\n${out}${err}")
      endif()
      list(APPEND states "${out}")
    endforeach()
    list(GET states 0 first)
    list(GET states 1 second)
    list(GET states 2 third)
    if(NOT first STREQUAL second OR first STREQUAL third)
      message(FATAL_ERROR "the state hash is not stable for the same run or not different for another:\n"
        "${first}${second}${third}")
    endif()
  elseif(CASE STREQUAL "inputs_script")
    file(WRITE "${OUT}/runner-empty.inputs" "")
    file(WRITE "${OUT}/runner-button.inputs" "2 p1=dualshock:X:128,128,128,128 p2=.\n")
    string(REPEAT "[0-9a-f]" 64 digits)
    function(run_movie frames movie result_name)
      set(movie_arg)
      if(NOT movie STREQUAL "none")
        set(movie_arg --inputs "${movie}")
      endif()
      execute_process(COMMAND ${EMULATOR} ${RUN} --frames ${frames} --hash --hash-state ${movie_arg}
        RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
      if(NOT rc STREQUAL "0" OR NOT out MATCHES "frame ${frames} sha256 ${digits}" OR
         NOT out MATCHES "state sha256 ${digits}")
        message(FATAL_ERROR "--frames ${frames} input '${movie}': expected exit 0 and frame/state hashes, "
          "got exit ${rc}\\n${out}${err}")
      endif()
      string(REGEX MATCH "frame ${frames} sha256 ([0-9a-f]+)" frame_line "${out}")
      set(frame_hash "${CMAKE_MATCH_1}")
      string(REGEX MATCH "state sha256 ([0-9a-f]+)" state_line "${out}")
      set(state_hash "${CMAKE_MATCH_1}")
      set(${result_name}_frame "${frame_hash}" PARENT_SCOPE)
      set(${result_name}_state "${state_hash}" PARENT_SCOPE)
      set(${result_name}_output "${out}" PARENT_SCOPE)
    endfunction()
    run_movie(1 none no_movie_before)
    run_movie(1 "${OUT}/runner-button.inputs" event_before)
    run_movie(2 none no_movie_after)
    run_movie(2 "${OUT}/runner-empty.inputs" empty_movie)
    run_movie(2 "${OUT}/runner-button.inputs" event_after)
    run_movie(2 "${OUT}/runner-button.inputs" event_repeat)
    if(NOT no_movie_before_frame STREQUAL event_before_frame OR
       NOT no_movie_before_state STREQUAL event_before_state)
      message(FATAL_ERROR "an input scheduled for frame 2 changed the frame-1 state")
    endif()
    if(NOT no_movie_after_frame STREQUAL empty_movie_frame OR
       NOT no_movie_after_state STREQUAL empty_movie_state)
      message(FATAL_ERROR "an empty input movie changed the no-input run")
    endif()
    if(NOT no_movie_after_frame STREQUAL event_after_frame)
      message(FATAL_ERROR "input changed the content-free frame hash")
    endif()
    if(no_movie_after_state STREQUAL event_after_state)
      message(FATAL_ERROR "the event-frame input did not change the saved latched state")
    endif()
    if(NOT event_after_output STREQUAL event_repeat_output)
      message(FATAL_ERROR "replaying the same input movie changed its hashes")
    endif()
  elseif(CASE STREQUAL "inputs_controls")
    string(REPEAT "[0-9a-f]" 64 digits)
    function(accept_input_movie name contents frames result_name)
      set(path "${OUT}/runner-input-${name}.txt")
      file(CONFIGURE OUTPUT "${path}" NEWLINE_STYLE UNIX CONTENT "${contents}")
      execute_process(COMMAND ${EMULATOR} ${RUN} --frames ${frames} --hash --hash-state --inputs "${path}"
        RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
      if(NOT rc STREQUAL "0" OR NOT out MATCHES "frame ${frames} sha256 ${digits}" OR
         NOT out MATCHES "state sha256 ${digits}")
        message(FATAL_ERROR "valid input '${name}': expected exit 0 and frame/state hashes, "
          "got exit ${rc}\n${out}${err}")
      endif()
      set(${result_name} "${out}" PARENT_SCOPE)
    endfunction()
    function(refuse_input_movie name contents)
      set(path "${OUT}/runner-input-${name}.txt")
      file(CONFIGURE OUTPUT "${path}" NEWLINE_STYLE UNIX CONTENT "${contents}")
      execute_process(COMMAND ${EMULATOR} ${RUN} --frames 1 --hash --hash-state --inputs "${path}"
        RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
      if(NOT rc STREQUAL "2" OR NOT out STREQUAL "" OR NOT err MATCHES "invalid input movie" OR
         NOT err MATCHES "usage")
        message(FATAL_ERROR "invalid input '${name}': expected bounded usage refusal before output, "
          "got exit ${rc}\n${out}${err}")
      endif()
    endfunction()
    string(CONCAT endpoints "# controller endpoints\r\n"
      "1000000 p1=dualshock:.:0,255,0,255 p2=.\r\n"
      "1 p1=dualshock:UDLRSsTOXQ1234<>M:0,255,0,255 p2=digital:UDLRSsTOXQ1234:128,128,128,128 # frame one\r\n")
    accept_input_movie(endpoints "${endpoints}" 1 endpoints_output)
    string(CONCAT duplicate "2 p1=dualshock:X:128,128,128,128 p2=.\n"
      "2 p1=dualshock:X:128,128,128,128 p2=.\n")
    accept_input_movie(duplicate "${duplicate}" 2 duplicate_output)
    if(NOT duplicate_output MATCHES "frame 2 sha256 ${digits}")
      message(FATAL_ERROR "an identical same-frame record was not accepted")
    endif()
    refuse_input_movie(frame_zero "0 p1=. p2=.\n")
    refuse_input_movie(frame_over "1000001 p1=. p2=.\n")
    refuse_input_movie(frame_overflow "4294967295 p1=. p2=.\n")
    refuse_input_movie(port_zero "1 p0=. p2=.\n")
    refuse_input_movie(port_three "1 p1=. p3=.\n")
    refuse_input_movie(port_order "1 p2=. p1=.\n")
    refuse_input_movie(device "1 p1=analog:.:128,128,128,128 p2=.\n")
    refuse_input_movie(button_symbol "1 p1=digital:Z:128,128,128,128 p2=.\n")
    refuse_input_movie(button_repeat "1 p1=digital:XX:128,128,128,128 p2=.\n")
    string(ASCII 233 non_ascii)
    refuse_input_movie(non_ascii "1 p1=digital:${non_ascii}:128,128,128,128 p2=.\n")
    refuse_input_movie(axis_low "1 p1=dualshock:.:-1,128,128,128 p2=.\n")
    refuse_input_movie(axis_high "1 p1=dualshock:.:256,128,128,128 p2=.\n")
    refuse_input_movie(axis_encoding "1 p1=dualshock:.:1x,128,128,128 p2=.\n")
    refuse_input_movie(missing_axis "1 p1=dualshock:.:128,128,128 p2=.\n")
    string(CONCAT conflicting_frame "2 p1=dualshock:X:128,128,128,128 p2=.\n"
      "2 p1=dualshock:.:128,128,128,128 p2=.\n")
    refuse_input_movie(conflicting_frame "${conflicting_frame}")
    string(CONCAT late_invalid "1 p1=dualshock:X:128,128,128,128 p2=.\n"
      "2 p1=dualshock:.:999,128,128,128 p2=.\n")
    refuse_input_movie(late_invalid "${late_invalid}")
    refuse(--frames 1 --inputs)
    refuse(--frames 1 --inputs "${OUT}/runner-input-endpoints.txt" --inputs "${OUT}/runner-input-endpoints.txt")
    expect_exit(1 "cannot read input movie" --frames 1 --inputs "${OUT}/runner-input-missing.txt")
  elseif(CASE STREQUAL "no_args")
    refuse()
  elseif(CASE STREQUAL "unknown")
    # An option this release does not have, and --help or --version with
    # anything else.
    refuse(--frames 1 --bios)
    refuse(--frames 1 --disc x.cue)
    refuse(--frames 1 --help)
    refuse(--version --frames 1)
  elseif(CASE STREQUAL "exe")
    # --exe takes one file, never with --probe, and --tty needs it.
    refuse(--exe)
    refuse(--frames 1 --exe)
    refuse(--exe --hash)
    refuse(--exe a.exe --exe b.exe)
    refuse(--exe a.exe --probe b.exe)
    refuse(--probe b.exe --exe a.exe)
    refuse(--tty)
    refuse(--frames 1 --tty)
    refuse(--bench 1 --tty)
    # --stop-on takes mailbox, once, and needs --exe.
    refuse(--stop-on mailbox)
    refuse(--frames 1 --stop-on mailbox)
    refuse(--exe a.exe --stop-on)
    refuse(--exe a.exe --stop-on exit)
    refuse(--exe a.exe --stop-on mailbox --stop-on mailbox)
    # --trace takes first-fault, once, and needs --exe; --no-stop-on-fault needs --exe.
    refuse(--trace first-fault)
    refuse(--frames 1 --trace first-fault)
    refuse(--exe a.exe --trace)
    refuse(--exe a.exe --trace cpu)
    refuse(--exe a.exe --trace first-fault --trace first-fault)
    refuse(--no-stop-on-fault)
    refuse(--frames 1 --no-stop-on-fault)
  elseif(CASE STREQUAL "exe_missing")
    # A file that is not there, or is not a PS-EXE, exits 1 naming the file
    # and the reason; no frame runs.
    expect_exit(1 "schwaystation-run: [^\n]*missing[.]exe: cannot (open|read)" --exe "${OUT}/missing.exe")
    expect_exit(1 "schwaystation-run: [^\n]*tests[.]cmake: PS-EXE invalid" --exe "${CMAKE_CURRENT_LIST_FILE}"
      --tty)
  elseif(CASE STREQUAL "positional")
    # This release loads no disc, so a file name is a usage error.
    refuse(--frames 1 game.cue)
    refuse(game.cue)
  elseif(CASE STREQUAL "frame_range")
    refuse(--frames 0)
    refuse(--frames 1000001)
    refuse(--frames 4294967296)
    refuse(--frames -1)
    refuse(--frames 1x)
    refuse(--frames)
    refuse(--hash --frames)
    refuse(--frames 1 --frames 1)
  elseif(CASE STREQUAL "png_syntax")
    # Each malformed or out-of-range --png is refused, whichever order the
    # options come in, and writes nothing.
    file(REMOVE "${OUT}")
    refuse(--frames 5 --png)
    refuse(--frames 5 --png "${OUT}")
    refuse(--frames 5 --png "5:")
    refuse(--frames 5 --png ":${OUT}")
    refuse(--frames 5 --png "0:${OUT}")
    refuse(--frames 5 --png "x:${OUT}")
    refuse(--frames 5 --png "6:${OUT}")
    refuse(--png "6:${OUT}" --frames 5)
    refuse(--frames 5 --png "5:${OUT}" --png "5:${OUT}")
    if(EXISTS "${OUT}")
      message(FATAL_ERROR "a refused --png wrote ${OUT}")
    endif()
  elseif(CASE STREQUAL "region")
    refuse(--frames 1 --region)
    refuse(--frames 1 --region secam)
    refuse(--frames 1 --region NTSC-U)
    refuse(--frames 1 --region auto)
  elseif(CASE STREQUAL "png_file")
    # The decode check runs first, so the pinned digest is only ever
    # compared for a file whose IHDR and every pixel are already known good.
    file(REMOVE "${OUT}")
    expect_exit(0 "^$" --frames 5 --png "5:${OUT}")
    execute_process(COMMAND ${EMULATOR} ${DECODE} png_file "${OUT}" RESULT_VARIABLE rc
      OUTPUT_VARIABLE out ERROR_VARIABLE out)
    if(NOT rc STREQUAL "0")
      message(FATAL_ERROR "the PNG does not decode to the card (exit ${rc})\n${out}")
    endif()
    file(SHA256 "${OUT}" sha256)
    if(NOT sha256 STREQUAL EXPECTED)
      message(FATAL_ERROR "PNG SHA-256 mismatch: expected ${EXPECTED} got ${sha256}")
    endif()
  else()
    message(FATAL_ERROR "unknown CASE '${CASE}'")
  endif()
  return()
endif()

# Every command that starts the runner goes through the build's
# CMAKE_CROSSCOMPILING_EMULATOR, so an emulated lane runs its own guest binary.
set(runner_cmd ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-run>)

# Fault prevented: the runner's printed card hash, the line hosts and CI
# compare, drifts from the pinned card or differs on another platform.
schway_add_test(NAME runner.hash
  COMMAND ${runner_cmd} --frames 60 --hash
  PASS_REGEX "frame 60 sha256 ${SCHWAYSTATION_TESTCARD_SHA256}"
  LABELS portable runner TIMEOUT 30)

# Fault prevented: a card that does not move, so the frame hash cannot tell
# frame 5 from frame 60.
schway_add_test(NAME runner.hash.frame5
  COMMAND ${runner_cmd} --frames 5 --hash
  PASS_REGEX "frame 5 sha256 ${SCHWAYSTATION_TESTCARD_SHA256_FRAME5}"
  LABELS portable runner TIMEOUT 30)

# Fault prevented: --region not reaching the frame the host sees.
schway_add_test(NAME runner.hash.ntsc_j
  COMMAND ${runner_cmd} --frames 60 --hash --region ntsc-j
  PASS_REGEX "frame 60 sha256 ${SCHWAYSTATION_TESTCARD_SHA256_NTSC_J}"
  LABELS portable runner TIMEOUT 30)
schway_add_test(NAME runner.hash.pal
  COMMAND ${runner_cmd} --frames 60 --hash --region pal
  PASS_REGEX "frame 60 sha256 ${SCHWAYSTATION_TESTCARD_SHA256_PAL}"
  LABELS portable runner TIMEOUT 30)

# Fault prevented: a lane whose hashes are never compared with the others.
# Writes build/<preset>/hashes.txt, the file CI collects from every lane and
# compares byte for byte.
string(REPLACE ";" "$<SEMICOLON>" runner_emulator "${CMAKE_CROSSCOMPILING_EMULATOR}")
set(runner_hashes ${PROJECT_SOURCE_DIR}/tests/cmake/hashes.cmake)
set(runner_fixtures_arg "")
if(SCHWAYSTATION_FIXTURES)
  set(runner_fixtures_arg -DFIXTURES=${PROJECT_BINARY_DIR}/fixtures)
endif()
schway_add_test(NAME runner.hashes_file
  COMMAND ${CMAKE_COMMAND} -DMODE=write -DRUN=$<TARGET_FILE:schwaystation-run>
    "-DEMULATOR=${runner_emulator}" ${runner_fixtures_arg} -DOUT=${PROJECT_BINARY_DIR}
    -P ${runner_hashes}
  LABELS portable runner TIMEOUT 60)

# Fault prevented: a comparison that passes lanes which differ, or a lane
# that silently drops out. All eight files must match, a missing eighth must
# fail, and a different eighth must fail even when the first seven match.
set(runner_fixture ${PROJECT_BINARY_DIR}/tests/hashes)
set(runner_files "")
foreach(lane linux-x64 linux-arm64 macos-x64 macos-arm64 windows-x64 windows-arm64 be32 wasm32)
  file(WRITE ${runner_fixture}/${lane}.txt "frame 60 sha256 00\n")
  list(APPEND runner_files ${runner_fixture}/${lane}.txt)
endforeach()
string(REPLACE ";" "$<SEMICOLON>" runner_compare_files "${runner_files}")
schway_add_test(NAME runner.hashes_compare.eight
  COMMAND ${CMAKE_COMMAND} "-DFILES=${runner_compare_files}" -DCOUNT=8 -DMODE=compare -P ${runner_hashes}
  LABELS runner TIMEOUT 10)
list(POP_BACK runner_files)
file(WRITE ${runner_fixture}/b.txt "frame 60 sha256 01\n")
# CMake wraps long messages, so the regexes allow a break between words.
foreach(case IN ITEMS "count;;expected 8 files,[^/]*got 7" "byte;b.txt;b.txt[^/]*differs")
  list(GET case 0 name)
  list(GET case 1 eighth)
  list(GET case 2 regex)
  set(files ${runner_files})
  if(eighth)
    list(APPEND files ${runner_fixture}/${eighth})
  endif()
  string(REPLACE ";" "\\;" files "${files}")
  set(cmd ${CMAKE_COMMAND} "-DFILES=${files}" -DCOUNT=8 -DMODE=compare -P ${runner_hashes})
  string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
  schway_add_test(NAME runner.hashes_compare.${name}.self_test
    COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=${regex}"
      -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
    LABELS runner TIMEOUT 10)
endforeach()

# The PNG encoder and its checksums, checked by an independent parse, and the
# shared converter's known answers. The same program decodes the runner's PNG
# for runner.png_file.
add_executable(schwaystation-test-runner
  ${CMAKE_CURRENT_LIST_DIR}/test_png.c
  ${PROJECT_SOURCE_DIR}/runner/png.c
  ${PROJECT_SOURCE_DIR}/runner/crc32.c
  ${PROJECT_SOURCE_DIR}/runner/frame_hash.c
  ${PROJECT_SOURCE_DIR}/runner/sha256.c)
target_include_directories(schwaystation-test-runner PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-runner PRIVATE schwaystation::schwaystation
  schwaystation-host)
set_target_properties(schwaystation-test-runner PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-runner)

# Fault prevented: an encoder whose chunk CRCs, Adler-32 or stored blocks are
# wrong, so a viewer or the RetroArch compare rejects or misreads the file.
schway_add_test(NAME runner.png
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-runner> png
  LABELS portable runner TIMEOUT 10)

# Fault prevented: a wrong 15-bit to XRGB8888 rule. The runner's PNG and the
# libretro core share the converter, so comparing them cannot catch it.
schway_add_test(NAME runner.xrgb8888
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-runner> xrgb8888
  LABELS portable runner TIMEOUT 10)

foreach(case rgb24 video_bounds native_hash)
  schway_add_test(NAME runner.${case}
    COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-runner> ${case}
    LABELS portable runner TIMEOUT 10)
endforeach()

string(REPLACE ";" "|" runner_emulator_bars "${CMAKE_CROSSCOMPILING_EMULATOR}")
set(runner_check ${CMAKE_COMMAND} -DRUN=$<TARGET_FILE:schwaystation-run>
  "-DEMULATOR=${runner_emulator_bars}")

# Fault prevented: a state hash that is unstable, malformed or blind to the frames run, so devices
# a test-card frame cannot show are not compared across lanes.
schway_add_test(NAME runner.hash_state
  COMMAND ${runner_check} -DCASE=hash_state -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# Fault prevented: a movie applied after the frame latch, rejected after execution starts, or
# changes the content-free frame instead of the serialized latched input state.
schway_add_test(NAME runner.inputs.script
  COMMAND ${runner_check} -DCASE=inputs_script -DOUT=${PROJECT_BINARY_DIR}/tests
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# Fault prevented: malformed frame, port, button, axis, ordering, or encoding values reaching the core.
schway_add_test(NAME runner.inputs.controls
  COMMAND ${runner_check} -DCASE=inputs_controls -DOUT=${PROJECT_BINARY_DIR}/tests
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# Fault prevented: the reference PNG the RetroArch screenshot is compared
# with changes bytes (converter, layout or encoder) while the frame hash
# still passes, or no longer shows the card.
schway_add_test(NAME runner.png_file
  COMMAND ${runner_check} -DCASE=png_file -DOUT=${PROJECT_BINARY_DIR}/tests/card.png
    -DDECODE=$<TARGET_FILE:schwaystation-test-runner> -DEXPECTED=${SCHWAYSTATION_TESTCARD_PNG_SHA256}
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# Fault prevented: a command line that runs something it should refuse, so a
# CI step hashes a frame nobody asked for: no arguments, an option this
# release lacks, a file name, --exe or --tty misused, a frame count out of
# range or missing, a malformed or out-of-range --png, or an unknown region.
foreach(case no_args unknown positional exe frame_range png_syntax region)
  schway_add_test(NAME runner.usage.${case}
    COMMAND ${runner_check} -DCASE=${case} -DOUT=${PROJECT_BINARY_DIR}/tests/refused.png
      -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS portable runner TIMEOUT 30)
endforeach()

# The bench statistics and the stop flag, tested directly, and a runner built
# with a hook that raises SIGTERM from inside frame 5.
add_executable(schwaystation-test-bench
  ${CMAKE_CURRENT_LIST_DIR}/test_runner.c
  ${PROJECT_SOURCE_DIR}/runner/bench.c
  ${PROJECT_SOURCE_DIR}/runner/stop.c)
set_target_properties(schwaystation-test-bench PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-bench)

add_executable(schwaystation-test-run-stop ${SCHWAYSTATION_RUNNER_SOURCES})
target_include_directories(schwaystation-test-run-stop PRIVATE ${SCHWAYSTATION_RUNNER_INCLUDE}
  ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-run-stop PRIVATE schwaystation::schwaystation
  schwaystation-host)
target_compile_definitions(schwaystation-test-run-stop PRIVATE _POSIX_C_SOURCE=200809L
  SCHWAY_RUN_RAISE_AT=5 SCHWAY_RUN_TEST_HOOKS=1 SCHWAY_HOST_CARD_TESTING=1)
set_target_properties(schwaystation-test-run-stop PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-run-stop)

set(bench_check ${runner_check} -DUNIT=$<TARGET_FILE:schwaystation-test-bench>)

# Fault prevented: a percentile that interpolates, picks the wrong rank or
# reads unsorted samples, so the report names a time no frame took.
schway_add_test(NAME runner.bench.percentile
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-bench> percentile
  LABELS bench portable runner TIMEOUT 10)

# Fault prevented: hostile guest terminal control bytes reaching the user's terminal.
schway_add_test(NAME runner.tty_sanitize
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-bench> tty_sanitize
  LABELS portable runner TIMEOUT 10)

# Fault prevented: a bench report missing a line, out of order or not in
# whole units, so a script reading it misreads the times.
schway_add_test(NAME runner.bench.format
  COMMAND ${bench_check} -DCASE=bench_format -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS bench portable runner TIMEOUT 30)

# Fault prevented: a sample count whose buffer size wraps to a small
# allocation, so the frame loop writes past it.
schway_add_test(NAME runner.bench.bytes
  COMMAND ${bench_check} -DCASE=bench_bytes -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS bench portable runner TIMEOUT 30)

# Fault prevented: SIGINT or SIGTERM killing the run mid-frame or losing the
# output already asked for, so an interrupted CI job reports nothing.
schway_add_test(NAME runner.stop
  COMMAND ${bench_check} -DHOOKED=$<TARGET_FILE:schwaystation-test-run-stop>
    -DEXPECTED=${SCHWAYSTATION_TESTCARD_SHA256_FRAME5} -DCASE=stop -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# Fault prevented: completed card generations are not flushed on normal exit or after a clean
# SIGTERM, or an unchanged slot is needlessly replaced.
set(runner_cards_check ${runner_check} -DHOOKED=$<TARGET_FILE:schwaystation-test-run-stop>
  -DEXPECTED=${SCHWAYSTATION_TESTCARD_SHA256_FRAME5} -DOUT=${PROJECT_BINARY_DIR}/tests)
foreach(case controls persistence sigterm)
  schway_add_test(NAME runner.cards.${case}
    COMMAND ${runner_cards_check} -DCASE=cards_${case} -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS portable runner TIMEOUT 60)
endforeach()

# Fault prevented: a runner that exits 0 after failing to write a file, or
# calls a failure a usage error.
set(runner_failure_cmd ${runner_cmd} --frames 1 --png "1:${PROJECT_BINARY_DIR}/tests/missing/x.png")
string(REPLACE ";" "$<SEMICOLON>" runner_failure_cmd "${runner_failure_cmd}")
schway_add_test(NAME runner.exit.failure
  COMMAND ${CMAKE_COMMAND} "-DCMD=${runner_failure_cmd}" -DCODE=1 "-DREGEX=cannot write"
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
  LABELS portable runner TIMEOUT 30)

# Fault prevented: a missing or invalid --exe file that is run anyway, or that
# is reported as a usage error, so a CI step passes on nothing.
schway_add_test(NAME runner.exe.missing
  COMMAND ${runner_check} -DCASE=exe_missing -DOUT=${PROJECT_BINARY_DIR}/tests
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# The trace record formatter, tested directly: it links the formatter alone, with no instance.
add_executable(schwaystation-test-trace-format
  ${CMAKE_CURRENT_LIST_DIR}/test_trace.c
  ${PROJECT_SOURCE_DIR}/runner/trace.c)
target_link_libraries(schwaystation-test-trace-format PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-trace-format PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-trace-format)

# Fault prevented: a trace line whose fields, order, hex width or event and device names drift from
# what docs/runner.md says, so a script that reads the tail misparses it. One golden line per event.
schway_add_test(NAME runner.trace.golden
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-trace-format>
  LABELS portable runner TIMEOUT 10)

# Fault prevented: a documented exit status that differs from what the runner returns. --help's
# table and docs/runner.md's must match line for line, in either direction; the self test checks a
# copy of the page without the hang row, which must fail naming that row.
set(runner_doc ${PROJECT_SOURCE_DIR}/docs/runner.md)
set(runner_doc_copy ${PROJECT_BINARY_DIR}/tests/runner_without_hang.md)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${runner_doc})
file(READ ${runner_doc} runner_doc_text)
string(REGEX REPLACE "\n  8  hang[^\n]*" "" runner_doc_text "${runner_doc_text}")
file(WRITE ${runner_doc_copy} "${runner_doc_text}")
schway_add_test(NAME runner.exit_table
  COMMAND ${runner_check} -DDOC=${runner_doc}
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/exit_table.cmake
  LABELS portable runner TIMEOUT 30)
# The emulator's own semicolons would be split a second time inside -DCMD, so it travels with a bar
# between its words and exit_table.cmake turns the bars back.
string(REPLACE ";" "|" runner_emulator_bars "${CMAKE_CROSSCOMPILING_EMULATOR}")
set(runner_table_cmd ${CMAKE_COMMAND} -DRUN=$<TARGET_FILE:schwaystation-run>
  "-DEMULATOR=${runner_emulator_bars}" -DDOC=${runner_doc_copy}
  -P ${PROJECT_SOURCE_DIR}/tests/cmake/exit_table.cmake)
string(REPLACE ";" "$<SEMICOLON>" runner_table_cmd "${runner_table_cmd}")
schway_add_test(NAME runner.exit_table.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${runner_table_cmd}" -DCODE=1
    "-DREGEX=lacks the exit status line[^/]*8[^/]*hang"
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
  LABELS portable runner TIMEOUT 30)

# Fault prevented: --version printing something a script cannot parse.
schway_add_test(NAME runner.version
  COMMAND ${runner_cmd} --version
  PASS_REGEX "schwaystation-run [0-9]+[.][0-9]+[.][0-9]+ ([0-9a-f]+|unknown)"
  LABELS portable runner TIMEOUT 10)

# Fault prevented: testing a stale configure after a commit, so the results
# describe a tree other than HEAD. The self-test forces a mismatch.
set(runner_revision ${runner_check} -DSOURCE=${PROJECT_SOURCE_DIR}
  -P ${PROJECT_SOURCE_DIR}/tests/cmake/revision.cmake)
schway_add_test(NAME build.revision_matches_head
  COMMAND ${runner_revision}
  LABELS runner TIMEOUT 30)
set(runner_revision_cmd ${CMAKE_COMMAND} -DRUN=$<TARGET_FILE:schwaystation-run>
  -DREVISION=0000000000000000000000000000000000000000 -DSOURCE=${PROJECT_SOURCE_DIR}
  -P ${PROJECT_SOURCE_DIR}/tests/cmake/revision.cmake)
string(REPLACE ";" "$<SEMICOLON>" runner_revision_cmd "${runner_revision_cmd}")
schway_add_test(NAME build.revision_matches_head.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${runner_revision_cmd}" -DCODE=1
    "-DREGEX=configured at[^/]*0000000000000000000000000000000000000000[^/]*HEAD[^/]*is[^/]*[0-9a-f]"
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
  LABELS runner TIMEOUT 30)
