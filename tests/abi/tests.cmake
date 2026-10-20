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

# Every header name is in a versioned API test, so a declaration added after
# a release needs tests/api/v2.c and with it SCHWAY_API_VERSION 2. The self
# test reads a copy of the header with one extra function and no bump, and
# expects that function named.
schway_add_test(NAME abi.api_version
  COMMAND ${CMAKE_COMMAND} -DHEADER=${PROJECT_SOURCE_DIR}/include/schwaystation.h
    -DAPI_DIR=${PROJECT_SOURCE_DIR}/tests/api -P ${abi_scripts}/api_version.cmake
  LABELS abi TIMEOUT 10)
file(READ ${PROJECT_SOURCE_DIR}/include/schwaystation.h abi_header)
string(REPLACE "void schway_destroy(schway *inst);"
  "void schway_destroy(schway *inst);\nschway_status schway_added(schway *inst);"
  abi_header "${abi_header}")
if(NOT abi_header MATCHES "schway_added")
  message(FATAL_ERROR "abi.api_version.self_test: the schway_destroy declaration moved")
endif()
file(WRITE ${PROJECT_BINARY_DIR}/tests/abi/added_function.h "${abi_header}")
set(abi_cmd ${CMAKE_COMMAND} -DHEADER=${PROJECT_BINARY_DIR}/tests/abi/added_function.h
  -DAPI_DIR=${PROJECT_SOURCE_DIR}/tests/api -P ${abi_scripts}/api_version.cmake)
string(REPLACE ";" "$<SEMICOLON>" abi_cmd "${abi_cmd}")
schway_add_test(NAME abi.api_version.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${abi_cmd}" -DCODE=1
    "-DREGEX=in no tests/api/vK[.]c[^:]*1:.*schway_added" -P ${abi_scripts}/expect_fail.cmake
  LABELS abi TIMEOUT 10)

# The frozen API v1 program: every function, field and constant of version 1.
add_executable(schwaystation-test-api-v1 ${PROJECT_SOURCE_DIR}/tests/api/v1.c)
target_link_libraries(schwaystation-test-api-v1 PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-api-v1 PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-api-v1)
schway_add_test(NAME abi.api_v1 COMMAND schwaystation-test-api-v1
  LABELS abi portable TIMEOUT 10)

# The public header compiles alone as C17 and as C++17 with warnings fatal.
# Fault prevented: a header that only the core's own build, with its own
# flags and language, can compile. C++ is enabled here only; an embedding
# project never meets it. The targets are built only by these tests, which
# share the build tree and so take turns.
enable_language(CXX)
add_library(schwaystation_header_c17 OBJECT EXCLUDE_FROM_ALL
  ${PROJECT_SOURCE_DIR}/tests/header/header.c)
add_library(schwaystation_header_cxx17 OBJECT EXCLUDE_FROM_ALL
  ${PROJECT_SOURCE_DIR}/tests/header/header.cpp)
set_target_properties(schwaystation_header_cxx17 PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
schway_warnings(schwaystation_header_c17)
foreach(abi_target IN ITEMS schwaystation_header_c17 schwaystation_header_cxx17)
  target_include_directories(${abi_target} PRIVATE
    ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/generated)
  target_compile_options(${abi_target} PRIVATE
    $<$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic -Werror>
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic -Werror>
    $<$<COMPILE_LANG_AND_ID:C,MSVC>:/W4 /WX /TC>
    $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/W4 /WX /TP>)
endforeach()
foreach(abi_lang IN ITEMS c17 cxx17)
  schway_add_test(NAME abi.header.${abi_lang}
    COMMAND ${CMAKE_COMMAND} --build ${PROJECT_BINARY_DIR}
      --target schwaystation_header_${abi_lang} --config $<CONFIG>
    LABELS abi TIMEOUT 120)
  set_tests_properties(abi.header.${abi_lang} PROPERTIES RESOURCE_LOCK build_tree)
endforeach()
