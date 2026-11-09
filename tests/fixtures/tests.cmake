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
      ${schway__fixture_src}/mailbox.inc
    COMMENT "armips ${source}"
    VERBATIM)
  set_property(GLOBAL APPEND PROPERTY SCHWAY__FIXTURE_OUTPUTS ${out})
endfunction()

schway__fixture(hello.exe hello.asm)
schway__fixture(exc_default.exe exc_default.asm)
schway__fixture(exc_handler.exe exc_default.asm -definelabel USE_HANDLER 1)
schway__fixture(exit_port.exe exit_port.asm -equ CODE 0)
schway__fixture(exit_port_fail.exe exit_port.asm -equ CODE 0x2A)
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

# Fault prevented: an exit byte that is lost or reported as another value, or a
# zero that fails the run. Byte 00h exits 0 with no report; 2Ah exits 6 and
# the runner prints exit code=0x2a.
schway_add_test(NAME fixtures.exit_port
  COMMAND ${CMAKE_COMMAND} -DCASE=exit_port -DOK=${SCHWAYSTATION_FIXTURES_DIR}/exit_port.exe
    -DFAIL=${SCHWAYSTATION_FIXTURES_DIR}/exit_port_fail.exe
    -DRUN=$<TARGET_FILE:schwaystation-run> "-DEMULATOR=${schway__probe_emulator}"
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures portable TIMEOUT 30)

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
