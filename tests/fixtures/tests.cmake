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
    COMMENT "armips ${source}"
    VERBATIM)
  set_property(GLOBAL APPEND PROPERTY SCHWAY__FIXTURE_OUTPUTS ${out})
endfunction()

schway__fixture(hello.exe hello.asm)

get_property(schway__fixture_outputs GLOBAL PROPERTY SCHWAY__FIXTURE_OUTPUTS)
add_custom_target(schwaystation-fixtures ALL DEPENDS ${schway__fixture_outputs})

# Fault prevented: an assembled image whose header does not describe it, so
# every fixture built from header.inc loads wrongly.
schway_add_test(NAME fixtures.hello.header
  COMMAND ${CMAKE_COMMAND} -DCASE=header -DFILE=${SCHWAYSTATION_FIXTURES_DIR}/hello.exe
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS fixtures portable TIMEOUT 30)
