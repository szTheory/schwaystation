# Symbol, global, undefined-reference and header checks on the core archive, and the API version touch tests.
# Each binary check runs tests/cmake/abi.cmake on the built archive; each
# fixture is a negative control built with the core's own options, so its
# check fails on every lane where it runs.
set(abi_scripts ${PROJECT_SOURCE_DIR}/tests/cmake)
# MSVC sets no CMAKE_NM; llvm-nm reads its COFF archives.
set(abi_nm ${CMAKE_NM})
if(NOT abi_nm)
  find_program(SCHWAYSTATION_NM NAMES llvm-nm nm REQUIRED)
  set(abi_nm ${SCHWAYSTATION_NM})
endif()
set(abi_args -DNM=${abi_nm} "-DSYSTEM=${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}"
  -DHEADER=${PROJECT_SOURCE_DIR}/include/schwaystation.h)

get_target_property(abi_options schwaystation_instance COMPILE_OPTIONS)
foreach(fixture IN ITEMS clock global libc symbol)
  add_library(schwaystation-abi-${fixture} STATIC ${CMAKE_CURRENT_LIST_DIR}/${fixture}_fixture.c)
  target_compile_options(schwaystation-abi-${fixture} PRIVATE ${abi_options})
  set_target_properties(schwaystation-abi-${fixture} PROPERTIES
    C_VISIBILITY_PRESET hidden
    POSITION_INDEPENDENT_CODE ON
    ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests/abi)
endforeach()

# The PS1 has no real-time clock, so the core needs no injected time; the
# clock check proves it reads none of the host's.
foreach(case IN ITEMS "abi.symbols;symbols" "abi.globals;globals" "abi.undefined;undefined"
                      "core.no_host_clock;clock")
  list(GET case 0 name)
  list(GET case 1 mode)
  schway_add_test(NAME ${name}
    COMMAND ${CMAKE_COMMAND} -DMODE=${mode} -DLIB=$<TARGET_FILE:schwaystation> ${abi_args}
      -P ${abi_scripts}/abi.cmake
    LABELS abi TIMEOUT 30)
endforeach()

# CMake wraps a long message at a column that depends on the build path, so
# each regex allows a line break anywhere in the finding's title.
foreach(case IN ITEMS "abi.symbols;symbols;symbol;outside[^:]*namespace:.*abi_symbol_fixture"
                      "abi.globals;globals;global;writable[^:]*symbol:.*counter"
                      "abi.undefined;undefined;libc;unexpected[^:]*symbol:.*abort"
                      "core.no_host_clock;clock;clock;host[^:]*clock[^:]*read:.*time")
  list(GET case 0 name)
  list(GET case 1 mode)
  list(GET case 2 fixture)
  list(GET case 3 regex)
  set(abi_cmd ${CMAKE_COMMAND} -DMODE=${mode} -DLIB=$<TARGET_FILE:schwaystation-abi-${fixture}>
    ${abi_args} -P ${abi_scripts}/abi.cmake)
  string(REPLACE ";" "$<SEMICOLON>" abi_cmd "${abi_cmd}")
  schway_add_test(NAME ${name}.self_test
    COMMAND ${CMAKE_COMMAND} "-DCMD=${abi_cmd}" -DCODE=1 "-DREGEX=${regex}"
      -P ${abi_scripts}/expect_fail.cmake
    LABELS abi TIMEOUT 30)
endforeach()
