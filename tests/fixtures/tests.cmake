# The PS-EXE fixtures: armips sources under tests/fixtures/src, assembled into the build tree.
# Only the sources are tracked; no assembled image is ever committed.
#
# Run as a script (cmake -DCASE=<case> ... -P tests.cmake), this file is one
# check; include()d by tests/tests.cmake, it builds armips and registers the tests.
if(CMAKE_SCRIPT_MODE_FILE)
  if(CASE STREQUAL "header")
    # The magic, the size in whole sectors and the load size at 1Ch, from the bytes alone.
    file(SIZE "${FILE}" size)
    file(READ "${FILE}" head HEX LIMIT 32)
    string(SUBSTRING "${head}" 0 16 magic)
    string(SUBSTRING "${head}" 56 8 t_size_hex)
    string(REGEX REPLACE "(..)(..)(..)(..)" "0x\\4\\3\\2\\1" t_size_hex "${t_size_hex}")
    math(EXPR t_size "${t_size_hex}")
    math(EXPR remainder "${size} % 2048")
    math(EXPR body "${size} - 2048")
    if(NOT magic STREQUAL "50532d5820455845")
      message(FATAL_ERROR "${FILE}: the first eight bytes are ${magic}, not PS-X EXE")
    elseif(NOT remainder EQUAL 0 OR size LESS_EQUAL 2048)
      message(FATAL_ERROR "${FILE}: ${size} bytes is not a header plus whole 2048-byte sectors")
    elseif(NOT t_size EQUAL body)
      message(FATAL_ERROR "${FILE}: t_size is ${t_size}, but the file holds ${size} - 2048 bytes after the header")
    endif()
  elseif(CASE STREQUAL "probe")
    # The runner's --probe report for the image: the identity fields, the
    # size and the achievement-hash MD5, each from an independent reading of
    # the file (the image's text is its whole body, so the MD5 covers the file).
    file(SIZE "${FILE}" size)
    file(MD5 "${FILE}" md5)
    execute_process(COMMAND ${EMULATOR} ${RUN} --probe "${FILE}" RESULT_VARIABLE rc
      OUTPUT_VARIABLE out ERROR_VARIABLE out)
    set(want "format=exe" "serial=n/a" "licence_area=north_america" "region_guess=ntsc-u"
      "exe_name=hello[.]exe" "exe_size=${size}" "pc0=0x80010100" "t_addr=0x80010000"
      "rcheevos_md5=${md5}")
    if(NOT rc STREQUAL "0")
      message(FATAL_ERROR "schwaystation-run --probe ${FILE}: exit ${rc}\n${out}")
    endif()
    foreach(line IN LISTS want)
      if(NOT out MATCHES "(^|\n)${line}\n")
        message(FATAL_ERROR "--probe report lacks ${line}:\n${out}")
      endif()
    endforeach()
  elseif(CASE STREQUAL "tty")
    # The runner's --tty report for hello.exe: each of the two lines, from the
    # debug port and from the kernel's putchar, printed whole after the prefix.
    execute_process(COMMAND ${EMULATOR} ${RUN} --exe "${FILE}" --tty --frames 10 RESULT_VARIABLE rc
      OUTPUT_VARIABLE out ERROR_VARIABLE out)
    if(NOT rc STREQUAL "0")
      message(FATAL_ERROR "schwaystation-run --exe ${FILE} --tty: exit ${rc}\n${out}")
    endif()
    foreach(line IN ITEMS "hello port" "hello putchar")
      if(NOT out MATCHES "(^|\n)tty ${line}\n")
        message(FATAL_ERROR "--tty report lacks the line ${line}:\n${out}")
      endif()
    endforeach()
  elseif(CASE STREQUAL "mailbox" OR CASE STREQUAL "criterion4")
    # hello.exe seals its mailbox, so --stop-on mailbox ends the run with exit 0: its text, the
    # mailbox line and its two records (gp and sp), printed in that order. The criterion4 case gives
    # no --frames, which is the 600-frame default cap.
    set(args --exe "${FILE}" --tty --stop-on mailbox)
    if(CASE STREQUAL "mailbox")
      list(APPEND args --frames 30)
    endif()
    execute_process(COMMAND ${EMULATOR} ${RUN} ${args} RESULT_VARIABLE rc
      OUTPUT_VARIABLE out ERROR_VARIABLE out)
    if(NOT rc STREQUAL "0")
      message(FATAL_ERROR "schwaystation-run ${args}: exit ${rc}\n${out}")
    endif()
    foreach(line IN ITEMS "tty hello port" "mailbox SWMB status=00 records=2"
        "mailbox 0001=80018000" "mailbox 0002=80200000")
      if(NOT out MATCHES "(^|\n)${line}\n")
        message(FATAL_ERROR "schwaystation-run ${args} printed no line ${line}:\n${out}")
      endif()
    endforeach()
  elseif(CASE STREQUAL "mailbox_judge")
    # Runs the fixture to its sealed mailbox and judges every record against the fixture's rows in CSV
    # (fixture,test,profile,expected,source,citation): the nth row of the fixture is the record with
    # id n, expected is a hex value or lo..hi, and a record outside its row, a row with no record or a
    # record with no row fails. ALLOW lines (date, fixture, row, new value, evidence) accept another
    # value for a row; one that applies to no record is an error. FLAGS are extra runner options and
    # FAULT_KIND, when given, is the kind the printed first-fault line must name. Only the rows whose
    # profile is * or PROFILE are judged and numbered; SHIFT, a negative control's knob, is added to both
    # bounds of every row.
    separate_arguments(flags UNIX_COMMAND "${FLAGS}")
    if(NOT DEFINED SHIFT)
      set(SHIFT 0)
    endif()
    execute_process(COMMAND ${EMULATOR} ${RUN} --exe "${IMAGE}" --stop-on mailbox --frames ${FRAMES}
      ${flags} RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
    set(errors "")
    if(NOT rc STREQUAL "0")
      list(APPEND errors "the runner exited ${rc}")
    endif()
    if(NOT "${FAULT_KIND}" STREQUAL "" AND NOT out MATCHES "(^|\n)fault frame=[0-9]+ [^\n]* kind=${FAULT_KIND}\n")
      list(APPEND errors "no first-fault line with kind=${FAULT_KIND}")
    endif()
    string(REGEX MATCHALL "(^|\n)mailbox [0-9a-f]+=[0-9a-f]+" lines "${out}")
    foreach(line IN LISTS lines)
      string(REGEX REPLACE "^\n?mailbox ([0-9a-f]+)=([0-9a-f]+)$" "\\1;\\2" pair "${line}")
      list(GET pair 0 id_hex)
      list(GET pair 1 value)
      math(EXPR id "0x${id_hex}")
      set(record_${id} "${value}")
    endforeach()
    file(STRINGS "${CSV}" csv_lines)
    set(nrows 0)
    foreach(line IN LISTS csv_lines)
      string(REPLACE "," ";" fields "${line}")
      list(GET fields 0 first)
      if(first STREQUAL FIXTURE)
        list(LENGTH fields nfields)
        list(GET fields 2 profile)
        list(GET fields 4 source)
        if(NOT nfields EQUAL 6 OR NOT source MATCHES "^(doc|hw)$")
          list(APPEND errors "malformed row (six fields, source doc or hw): ${line}")
        endif()
        if(profile STREQUAL "*" OR profile STREQUAL "${PROFILE}")
          math(EXPR nrows "${nrows} + 1")
          set(row_${nrows} "${fields}")
        endif()
      endif()
    endforeach()
    set(allow_lines "")
    if(EXISTS "${ALLOW}")
      file(STRINGS "${ALLOW}" allow_lines)
    endif()
    set(allowed "")
    foreach(line IN LISTS allow_lines)
      if(NOT line MATCHES "^[ \t]*(#|$)" AND line MATCHES "^[^ ]+ ${FIXTURE} ")
        if(NOT line MATCHES "^[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9] [^ ]+ ([^ ]+) ([0-9a-f]+) [^ ].*$")
          list(APPEND errors "malformed allow line: ${line}")
        else()
          list(APPEND allowed "${CMAKE_MATCH_1}=${CMAKE_MATCH_2}")
        endif()
      endif()
    endforeach()
    set(used "")
    set(id 0)
    while(id LESS nrows)
      math(EXPR id "${id} + 1")
      list(GET row_${id} 1 test)
      list(GET row_${id} 3 expected)
      if(NOT DEFINED record_${id})
        list(APPEND errors "${test}: no record ${id}")
        continue()
      endif()
      set(value "${record_${id}}")
      unset(record_${id})
      if(expected MATCHES "^([0-9a-f]+)[.][.]([0-9a-f]+)$")
        set(lo ${CMAKE_MATCH_1})
        set(hi ${CMAKE_MATCH_2})
      else()
        set(lo ${expected})
        set(hi ${expected})
      endif()
      math(EXPR value_n "0x${value}")
      math(EXPR lo_n "0x${lo} + ${SHIFT}")
      math(EXPR hi_n "0x${hi} + ${SHIFT}")
      if(value_n LESS lo_n OR value_n GREATER hi_n)
        if("${test}=${value}" IN_LIST allowed)
          list(APPEND used "${test}=${value}")
        else()
          list(APPEND errors "${test}: record ${id} is ${value}, the row says ${expected}")
        endif()
      endif()
    endwhile()
    foreach(entry IN LISTS allowed)
      if(NOT entry IN_LIST used)
        list(APPEND errors "allow line ${entry} applies to no record")
      endif()
    endforeach()
    get_cmake_property(leftover VARIABLES)
    foreach(var IN LISTS leftover)
      if(var MATCHES "^record_([0-9]+)$")
        list(APPEND errors "record ${CMAKE_MATCH_1} has no row")
      endif()
    endforeach()
    if(errors)
      string(REPLACE ";" "\n  " errors "${errors}")
      message(FATAL_ERROR "${FIXTURE}: judged against ${CSV}:\n  ${errors}\n${out}")
    endif()
  elseif(CASE STREQUAL "trace")
    # The runner's first-fault report for a fault.exe of one class: exit 7, the fault line naming the
    # kind, then a tail of exactly 4,096 twelve-field lines that ends in the fault event. An exception
    # loop's tail ends with its repeat count, NOTE, in the last field.
    execute_process(COMMAND ${EMULATOR} ${RUN} --exe "${FILE}" --trace first-fault --frames 30
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
    if(NOT rc STREQUAL "7" OR NOT out MATCHES "(^|\n)fault frame=[0-9]+ [^\n]* kind=${KIND}\n")
      message(FATAL_ERROR "schwaystation-run --exe ${FILE} --trace first-fault: expected exit 7 and a "
        "fault line with kind=${KIND}, got exit ${rc}")
    endif()
    string(REPEAT "\t[^\t]+" 11 rest)
    string(REGEX MATCHALL "[0-9]+${rest}\n" tail "${out}")
    list(LENGTH tail n)
    list(GET tail -1 last)
    if(NOT n EQUAL 4096)
      message(FATAL_ERROR "${KIND}: the tail has ${n} twelve-field lines, not 4096")
    elseif(NOT last MATCHES "^[0-9]+\t[0-9]+\t[0-9]+\t[0-9]+\t0x[0-9a-f]+\t0x[0-9a-f]+\tfault\t")
      message(FATAL_ERROR "${KIND}: the last tail line is not a fault event:\n${last}")
    elseif(NOT "${NOTE}" STREQUAL "" AND NOT last MATCHES "\t${NOTE}\n$")
      message(FATAL_ERROR "${KIND}: the fault event's note is not ${NOTE}:\n${last}")
    endif()
  elseif(CASE STREQUAL "exit_port")
    # The runner's outcome for the exit port byte: 00h exits 0 and prints no report; 2Ah exits 6 and
    # prints the line that names it. The port needs the debug-port flag, so --tty is given.
    function(expect_exit file code regex)
      execute_process(COMMAND ${EMULATOR} ${RUN} --exe "${file}" --tty --frames 10
        RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
      if(NOT rc STREQUAL code OR NOT out MATCHES "${regex}")
        message(FATAL_ERROR "schwaystation-run --exe ${file}: expected exit ${code} and output matching\n"
          "  ${regex}\ngot exit ${rc} and output\n${out}")
      endif()
    endfunction()
    expect_exit("${OK}" 0 "^$")
    expect_exit("${FAIL}" 6 "(^|\n)exit code=0x2a\n")
  elseif(CASE STREQUAL "array")
    # Writes the assembled stub kernel as src/stub_kernel.c: 12 bytes to a line, the
    # array and its size, with the command that regenerates it.
    #   cmake -DCASE=array -DFILE=<build>/fixtures/stub.bin -DOUT=src/stub_kernel.c -P tests/fixtures/tests.cmake
    file(READ "${FILE}" hex HEX)
    string(REGEX MATCHALL ".." bytes "${hex}")
    set(body "")
    set(count 0)
    foreach(byte IN LISTS bytes)
      math(EXPR column "${count} % 12")
      if(column EQUAL 0)
        string(APPEND body "   ")
      endif()
      string(APPEND body " 0x${byte},")
      math(EXPR count "${count} + 1")
      math(EXPR column "${count} % 12")
      if(column EQUAL 0)
        string(APPEND body "\n")
      endif()
    endforeach()
    if(NOT body MATCHES "\n$")
      string(APPEND body "\n")
    endif()
    file(CONFIGURE OUTPUT "${OUT}" NEWLINE_STYLE UNIX @ONLY CONTENT
"/* The stub kernel: the BIOS image the core maps at BFC00000h when the host gives none.
   Generated from tests/fixtures/src/stub/stub.asm; do not edit. To regenerate, assemble
   the fixtures, then
     cmake -DCASE=array -DFILE=<build>/fixtures/stub.bin -DOUT=src/stub_kernel.c \\
       -P tests/fixtures/tests.cmake
   and update the digests in tests/fixtures/manifest.txt and PROVENANCE.md.
   fixtures.stub_drift fails while the two disagree. */
#include \"internal.h\"

const uint8_t schway__stub_kernel_image[] = {
${body}};

const uint32_t schway__stub_kernel_size = (uint32_t)sizeof schway__stub_kernel_image;
")
  else()
    message(FATAL_ERROR "unknown CASE '${CASE}'")
  endif()
  return()
endif()

include(${PROJECT_SOURCE_DIR}/cmake/Armips.cmake)

# schway__fixture(<name> <source.asm> [armips options...]): assembles
# tests/fixtures/src/<source> into ${SCHWAYSTATION_FIXTURES_DIR}/<name> with
# the pinned armips; the output path reaches the source as IMAGE.
set(SCHWAYSTATION_FIXTURES_DIR ${PROJECT_BINARY_DIR}/fixtures)
set(schway__fixture_src ${CMAKE_CURRENT_LIST_DIR}/src)
function(schway__fixture name source)
  set(out ${SCHWAYSTATION_FIXTURES_DIR}/${name})
  add_custom_command(OUTPUT ${out}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SCHWAYSTATION_FIXTURES_DIR}
    COMMAND ${SCHWAYSTATION_ARMIPS} -strequ IMAGE ${out} ${ARGN} ${source}
    WORKING_DIRECTORY ${schway__fixture_src}
    DEPENDS schwaystation-armips ${schway__fixture_src}/${source} ${schway__fixture_src}/header.inc
      ${schway__fixture_src}/mailbox.inc ${schway__fixture_extra}
    COMMENT "armips ${source}"
    VERBATIM)
  set_property(GLOBAL APPEND PROPERTY SCHWAY__FIXTURE_OUTPUTS ${out})
endfunction()

schway__fixture(hello.exe hello.asm)
schway__fixture(exc_default.exe exc_default.asm)
schway__fixture(exc_handler.exe exc_default.asm -definelabel USE_HANDLER 1)
schway__fixture(exit_port.exe exit_port.asm -equ CODE 0)
schway__fixture(exit_port_fail.exe exit_port.asm -equ CODE 0x2A)
schway__fixture(exc_matrix.exe exc_matrix.asm)
schway__fixture(hang.exe hang.asm)
schway__fixture(mailbox_fault.exe mailbox_fail.asm -equ MODE 0)
schway__fixture(mailbox_crc.exe mailbox_fail.asm -equ MODE 1)
schway__fixture(unbuilt.exe unbuilt.asm -equ MODE 0)
schway__fixture(gap_write.exe unbuilt.asm -equ MODE 1)
set(schway__fixture_extra ${schway__fixture_src}/gte_cases.inc) # the include only gte_vectors.asm reads
schway__fixture(gte_vectors.exe gte_vectors.asm)
set(schway__fixture_extra "")
schway__fixture(dma_chain.exe dma_chain.asm)
schway__fixture(timers_lines.exe timers_lines.asm)
set(schway__fault_kinds unmapped_read unmapped_write unmapped_fetch misaligned_read misaligned_write
  misaligned_fetch reserved_instruction cop_unusable exception_loop)
set(schway__fault_index 1)
foreach(kind IN LISTS schway__fault_kinds)
  schway__fixture(fault_${kind}.exe fault.asm -equ KIND ${schway__fault_index})
  math(EXPR schway__fault_index "${schway__fault_index} + 1")
endforeach()
schway__fixture(stub.bin stub/stub.asm)

get_property(schway__fixture_outputs GLOBAL PROPERTY SCHWAY__FIXTURE_OUTPUTS)
add_custom_target(schwaystation-fixtures ALL DEPENDS ${schway__fixture_outputs})

# Fault prevented: an assembled image whose header does not describe it, so
# every fixture built from header.inc loads wrongly.
schway_add_test(NAME fixtures.hello.header
  COMMAND ${CMAKE_COMMAND} -DCASE=header -DFILE=${SCHWAYSTATION_FIXTURES_DIR}/hello.exe
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures portable TIMEOUT 30)

# Fault prevented: a probe report that disagrees with the image's own bytes:
# a header field, the size or the achievement-hash MD5 read wrongly.
string(REPLACE ";" "$<SEMICOLON>" schway__probe_emulator "${CMAKE_CROSSCOMPILING_EMULATOR}")
schway_add_test(NAME fixtures.probe.hello
  COMMAND ${CMAKE_COMMAND} -DCASE=probe -DFILE=${SCHWAYSTATION_FIXTURES_DIR}/hello.exe
    -DRUN=$<TARGET_FILE:schwaystation-run> "-DEMULATOR=${schway__probe_emulator}"
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures portable TIMEOUT 30)

# Fault prevented: a runner that loads an image but loses its text, or prints
# one of the two sources only: hello.exe's lines reach standard output.
schway_add_test(NAME fixtures.hello.tty
  COMMAND ${CMAKE_COMMAND} -DCASE=tty -DFILE=${SCHWAYSTATION_FIXTURES_DIR}/hello.exe
    -DRUN=$<TARGET_FILE:schwaystation-run> "-DEMULATOR=${schway__probe_emulator}"
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures portable TIMEOUT 30)

# Fault prevented: a runner that never reads the fixture's mailbox, or reads it wrongly: the sealed
# mailbox of hello.exe ends the run with exit 0 and prints its records. The second test is the
# command the project's success criterion for this protocol names, with the default frame cap.
foreach(case mailbox criterion4)
  schway_add_test(NAME fixtures.hello.${case}
    COMMAND ${CMAKE_COMMAND} -DCASE=${case} -DFILE=${SCHWAYSTATION_FIXTURES_DIR}/hello.exe
      -DRUN=$<TARGET_FILE:schwaystation-run> "-DEMULATOR=${schway__probe_emulator}"
      -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS fixtures portable TIMEOUT 30)
endforeach()

# Fault prevented: an exit byte that is lost or reported as another value, or a
# zero that fails the run. Byte 00h exits 0 with no report; 2Ah exits 6 and
# the runner prints exit code=0x2a.
schway_add_test(NAME fixtures.exit_port
  COMMAND ${CMAKE_COMMAND} -DCASE=exit_port -DOK=${SCHWAYSTATION_FIXTURES_DIR}/exit_port.exe
    -DFAIL=${SCHWAYSTATION_FIXTURES_DIR}/exit_port_fail.exe
    -DRUN=$<TARGET_FILE:schwaystation-run> "-DEMULATOR=${schway__probe_emulator}"
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures portable TIMEOUT 30)

# The runner's outcome for a fixture: expect_runner(<test> <exit status> <regex> <runner arguments>)
# runs the runner on a built fixture and passes only for that exit status and a matching output.
function(expect_runner name code regex)
  set(cmd ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-run> ${ARGN})
  string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
  schway_add_test(NAME ${name}
    COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=${code} "-DREGEX=${regex}"
      -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
    LABELS fixtures portable TIMEOUT 30)
endfunction()

# Fault prevented: a program that never finishes, a fixture that reports a fault and a mailbox whose
# CRC is wrong, each reported as success or as the wrong outcome. A jump to itself is a hang, status 8,
# naming the frame cap and the program counter; a fault status and a CRC mismatch are status 6.
string(REPEAT "[0-9a-f]" 8 schway__hex8) # CMake regular expressions have no {n} count
expect_runner(fixtures.hang 8 "^hang frames=[0-9]+ pc=0x${schway__hex8}"
  --exe ${SCHWAYSTATION_FIXTURES_DIR}/hang.exe --stop-on mailbox --frames 5)
expect_runner(fixtures.mailbox_fault 6 "mailbox SWMB status=ff records=1"
  --exe ${SCHWAYSTATION_FIXTURES_DIR}/mailbox_fault.exe --stop-on mailbox --frames 30)
expect_runner(fixtures.mailbox_crc 6 "mailbox crc mismatch"
  --exe ${SCHWAYSTATION_FIXTURES_DIR}/mailbox_crc.exe --stop-on mailbox --frames 30)

# Fault prevented: an exception whose CAUSE, EPC or BadVaddr is wrong for its class, taken with or without
# a branch delay slot: exc_matrix.exe records them in its mailbox and tests/diag/expected.csv judges
# each record. Under --no-stop-on-fault the run goes on past the faulting rows and still names the first.
schway_add_test(NAME fixtures.exc_matrix
  COMMAND ${CMAKE_COMMAND} -DCASE=mailbox_judge -DFIXTURE=exc_matrix
    -DIMAGE=${SCHWAYSTATION_FIXTURES_DIR}/exc_matrix.exe -DFRAMES=30 -DFLAGS=--no-stop-on-fault
    -DFAULT_KIND=misaligned_read -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv
    -DALLOW=${PROJECT_SOURCE_DIR}/tests/diag/allow.txt
    -DRUN=$<TARGET_FILE:schwaystation-run> "-DEMULATOR=${schway__probe_emulator}"
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures portable TIMEOUT 30)

# Fault prevented: a GTE whose results are right when called directly but wrong behind the CPU's COP2 moves,
# load delays and busy stall: gte_vectors.exe replays hardware-captured cases with real COP2 instructions and
# records the CRC-32 of the 64 registers read back; tests/diag/expected.csv judges each against the hardware.
schway_add_test(NAME fixtures.gte_vectors
  COMMAND ${CMAKE_COMMAND} -DCASE=mailbox_judge -DFIXTURE=gte_vectors
    -DIMAGE=${SCHWAYSTATION_FIXTURES_DIR}/gte_vectors.exe -DFRAMES=30
    -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv
    -DRUN=$<TARGET_FILE:schwaystation-run> "-DEMULATOR=${schway__probe_emulator}"
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures gte portable TIMEOUT 30)

# Fault prevented: a DMA transfer that is right when the engine is called directly but wrong behind the CPU's
# stores to the DMA registers and its reads that wait: dma_chain.exe runs the ordering-table transfer and the
# linked-list cases of the public hardware tests and times the transfer with Timer 2, and
# tests/diag/expected.csv judges each record.
schway_add_test(NAME fixtures.dma_chain
  COMMAND ${CMAKE_COMMAND} -DCASE=mailbox_judge -DFIXTURE=dma_chain
    -DIMAGE=${SCHWAYSTATION_FIXTURES_DIR}/dma_chain.exe -DFRAMES=30
    -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv
    -DRUN=$<TARGET_FILE:schwaystation-run> "-DEMULATOR=${schway__probe_emulator}"
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS dma fixtures portable TIMEOUT 30)

# Fault prevented: a timer or vertical-blank interrupt that is right for one machine profile and wrong for
# the other: timers_lines.exe takes the blanks through its own handler on the NTSC and the PAL machine, and
# the rows of tests/diag/expected.csv for that profile (and the rows for every profile) judge its records.
foreach(schway__profile ntsc_u pal)
  string(REPLACE "_" "-" schway__region ${schway__profile})
  schway_add_test(NAME fixtures.timers_lines.${schway__profile}
    COMMAND ${CMAKE_COMMAND} -DCASE=mailbox_judge -DFIXTURE=timers_lines
      -DIMAGE=${SCHWAYSTATION_FIXTURES_DIR}/timers_lines.exe -DFRAMES=30
      -DPROFILE=${schway__region} "-DFLAGS=--region ${schway__region}"
      -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv
      -DRUN=$<TARGET_FILE:schwaystation-run> "-DEMULATOR=${schway__probe_emulator}"
      -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS fixtures portable timers TIMEOUT 30)

  # Fault prevented: ranges too wide to tell a wrong counter from a right one: the same run judged against
  # the rows moved up by one counter period (10000h) must fail naming both counters' records.
  set(schway__judge ${CMAKE_COMMAND} -DCASE=mailbox_judge -DFIXTURE=timers_lines
    -DIMAGE=${SCHWAYSTATION_FIXTURES_DIR}/timers_lines.exe -DFRAMES=30 -DSHIFT=65536
    -DPROFILE=${schway__region} "-DFLAGS=--region ${schway__region}"
    -DCSV=${PROJECT_SOURCE_DIR}/tests/diag/expected.csv
    -DRUN=$<TARGET_FILE:schwaystation-run> "-DEMULATOR=${schway__probe_emulator}"
    -P ${CMAKE_CURRENT_LIST_FILE})
  string(REPLACE ";" "$<SEMICOLON>" schway__judge "${schway__judge}")
  schway_add_test(NAME fixtures.timers_lines.${schway__profile}.self_test
    COMMAND ${CMAKE_COMMAND} "-DCMD=${schway__judge}" -DCODE=1
      "-DREGEX=div8_ticks: record 2 is.*sysclk_ticks: record 3 is"
      -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
    LABELS fixtures portable timers TIMEOUT 30)
endforeach()

# Fault prevented: a first fault that the runner misses, mislabels or reports with a short tail. Each
# fault.exe class runs more than 4,096 trace records of preamble, then faults once: exit 7, the fault line
# and a tail of exactly 4,096 twelve-field lines ending in the fault event.
foreach(kind IN LISTS schway__fault_kinds)
  set(note "")
  if(kind STREQUAL exception_loop)
    set(note 1000)
  endif()
  schway_add_test(NAME fixtures.trace.${kind}
    COMMAND ${CMAKE_COMMAND} -DCASE=trace -DFILE=${SCHWAYSTATION_FIXTURES_DIR}/fault_${kind}.exe
      -DKIND=${kind} -DNOTE=${note} -DRUN=$<TARGET_FILE:schwaystation-run>
      "-DEMULATOR=${schway__probe_emulator}" -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS fixtures portable TIMEOUT 30)
endforeach()

# Fault prevented: the success-criterion command, with no frame count, that no longer ends in a fault
# line and exit 7 (the default cap is 600 frames).
expect_runner(fixtures.trace.criterion5 7 "(^|\n)fault frame="
  --exe ${SCHWAYSTATION_FIXTURES_DIR}/fault_unmapped_read.exe --trace first-fault)

# Fault prevented: accesses to devices that are not built yet that leave no sign of a program stuck on
# one, and a write to a gap that is dropped without a fault. SPU and GPU writes end with exit 0 and an
# unbuilt line; a write to 1F801030h is a first fault.
expect_runner(fixtures.unbuilt_footer 0
  "(^|\n)unbuilt dev=(gpu|spu) n=[0-9]+ last_pc=0x${schway__hex8}(\n|$)"
  --exe ${SCHWAYSTATION_FIXTURES_DIR}/unbuilt.exe --stop-on mailbox --frames 30)
expect_runner(fixtures.gap_write 7 "(^|\n)fault frame=[0-9]+ [^\n]* kind=unmapped_write(\n|$)"
  --exe ${SCHWAYSTATION_FIXTURES_DIR}/gap_write.exe --frames 30)

# Fault prevented: a tampered or wrongly pinned assembler accepted. The
# digest check runs on a local file:// URL with an all-zero digest and must
# exit 1 naming the SHA-256 check; no network is used.
string(REPEAT 0 64 schway__zero_digest)
string(REGEX REPLACE "^([^/])" "/\\1" schway__pin_path "${PROJECT_SOURCE_DIR}/pins.txt")
set(schway__pin_cmd ${CMAKE_COMMAND} -DURL=file://${schway__pin_path}
  -DSHA256=${schway__zero_digest} -DOUT=${PROJECT_BINARY_DIR}/tests/armips_pin.txt
  -P ${PROJECT_SOURCE_DIR}/cmake/Armips.cmake)
string(REPLACE ";" "$<SEMICOLON>" schway__pin_cmd "${schway__pin_cmd}")
schway_add_test(NAME fixtures.armips_pin.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${schway__pin_cmd}" -DCODE=1 "-DREGEX=armips:.*SHA-256"
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
  LABELS fixtures portable TIMEOUT 30)

# The stub kernel in the core, white-box: the test sees src/ to read the committed array.
add_executable(schwaystation-test-boot ${CMAKE_CURRENT_LIST_DIR}/boot_tests.c)
target_include_directories(schwaystation-test-boot PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-boot PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-boot PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-boot)

# Fault prevented: a BIOS window that maps another image than the stub, reads
# its bytes in the wrong order, or lets a runaway read on past the image.
schway_add_test(NAME fixtures.stub.window
  COMMAND schwaystation-test-boot stub_window
  LABELS fixtures portable TIMEOUT 10)

# Fault prevented: a committed stub that no longer matches its source. The
# assembled stub.bin and the array in src/stub_kernel.c are compared byte for
# byte on every lane, and the first differing offset is printed.
schway_add_test(NAME fixtures.stub_drift
  COMMAND schwaystation-test-boot stub_drift ${SCHWAYSTATION_FIXTURES_DIR}
  LABELS fixtures portable TIMEOUT 10)

# Fault prevented: an image that never starts, or starts with the wrong global
# pointer or stack: hello.exe boots through the stub and seals its mailbox with
# the records the stub set up.
schway_add_test(NAME fixtures.hello.boot
  COMMAND schwaystation-test-boot hello ${SCHWAYSTATION_FIXTURES_DIR}
  LABELS fixtures portable TIMEOUT 30)

# Fault prevented: a TTY byte that is lost, reordered or reaches the host by
# only one of its two sources, the debug port and the kernel's putchar.
schway_add_test(NAME fixtures.tty.hello
  COMMAND schwaystation-test-boot tty_hello ${SCHWAYSTATION_FIXTURES_DIR}
  LABELS fixtures portable TIMEOUT 30)

# Fault prevented: a TTY ring that overwrites old bytes or loses its drop count,
# and debug ports decoded on a retail profile.
schway_add_test(NAME fixtures.tty.ring
  COMMAND schwaystation-test-boot tty_ring
  LABELS fixtures portable TIMEOUT 30)
schway_add_test(NAME fixtures.tty.flag_off
  COMMAND schwaystation-test-boot tty_flag_off ${SCHWAYSTATION_FIXTURES_DIR}
  LABELS fixtures portable TIMEOUT 30)

# Fault prevented: a default exception handler that records the wrong
# registers, or does not hand the exception to the handler an image registered.
schway_add_test(NAME fixtures.boot.default_handler
  COMMAND schwaystation-test-boot default_handler ${SCHWAYSTATION_FIXTURES_DIR}
  LABELS fixtures portable TIMEOUT 30)
schway_add_test(NAME fixtures.boot.registered_handler
  COMMAND schwaystation-test-boot registered_handler ${SCHWAYSTATION_FIXTURES_DIR}
  LABELS fixtures portable TIMEOUT 30)

# Fault prevented: a runaway past the stub's image that slides on as no-operations.
schway_add_test(NAME fixtures.boot.past_stub
  COMMAND schwaystation-test-boot past_stub
  LABELS fixtures portable TIMEOUT 30)

# Fault prevented: an exit that is lost, repeated or reported without its code.
schway_add_test(NAME fixtures.boot.exit_port
  COMMAND schwaystation-test-boot exit_port
  LABELS fixtures portable TIMEOUT 30)

# Fault prevented: a stop on a first fault that comes late or never, and a
# double fault the host is never told about.
schway_add_test(NAME fixtures.boot.stop_on_fault
  COMMAND schwaystation-test-boot stop_on_fault
  LABELS fixtures portable TIMEOUT 30)
schway_add_test(NAME fixtures.boot.double_fault
  COMMAND schwaystation-test-boot double_fault
  LABELS fixtures portable TIMEOUT 30)

# Fault prevented: a run that never returns for a looping or hostile guest.
schway_add_test(NAME fixtures.boot.bounded
  COMMAND schwaystation-test-boot bounded
  LABELS fixtures portable TIMEOUT 30)

# Fault prevented: a run that carries overshoot in the state, or whose stop
# points differ with how the cycles are split.
schway_add_test(NAME fixtures.boot.split_with_exe
  COMMAND schwaystation-test-boot split_with_exe ${SCHWAYSTATION_FIXTURES_DIR}
  LABELS fixtures portable TIMEOUT 30)

# Fault prevented: an expected value that drifted from the vector file it claims to come from: every
# gte_vectors row is recomputed from tests/vectors/gte/ps1tests-1150.bin in C and compared with the CSV.
schway_add_test(NAME fixtures.gte_vectors.guard
  COMMAND schwaystation-test-boot gte_vectors_guard
    ${PROJECT_SOURCE_DIR}/tests/vectors/gte/ps1tests-1150.bin ${PROJECT_SOURCE_DIR}/tests/diag/expected.csv
  LABELS fixtures gte portable TIMEOUT 30)
