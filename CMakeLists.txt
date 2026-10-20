cmake_minimum_required(VERSION 3.25)

# version.txt is the single version source; release-please edits only it.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/version.txt" SCHWAYSTATION_VERSION LIMIT_COUNT 1)
if(NOT SCHWAYSTATION_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
  message(FATAL_ERROR "version.txt must hold MAJOR.MINOR.PATCH, not '${SCHWAYSTATION_VERSION}'")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS version.txt)

# Only C is needed to build the library. A project that embeds this one with
# add_subdirectory gets the library and the schwaystation::schwaystation
# alias; the runner and the tests are built by default only when this is the
# top-level project.
project(schwaystation VERSION ${SCHWAYSTATION_VERSION} LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(SCHWAYSTATION_BUILD_RUNNER "Build the schwaystation-run command" ${PROJECT_IS_TOP_LEVEL})
option(SCHWAYSTATION_BUILD_TESTS "Register the tests" ${PROJECT_IS_TOP_LEVEL})

# The warning set, applied only when this is the top-level project so an
# embedder's older or console compiler never meets an unknown flag. Presets
# make warnings fatal through CMAKE_COMPILE_WARNING_AS_ERROR.
function(schway_warnings target)
  if(PROJECT_IS_TOP_LEVEL)
    target_compile_options(${target} PRIVATE
      $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wstrict-prototypes -Wmissing-prototypes -Wvla -Wundef -Wcast-qual -Wimplicit-fallthrough -Wmissing-variable-declarations>
      $<$<C_COMPILER_ID:GNU>:-Wduplicated-cond -Wlogical-op>
      $<$<C_COMPILER_ID:MSVC>:/W4 /utf-8>)
  endif()
endfunction()

# The library version reaches the public header through a generated header,
# so version.txt stays the only file that holds it.
configure_file(cmake/version.h.in ${PROJECT_BINARY_DIR}/generated/schwaystation_version.h @ONLY)

# The options of every target that holds core code. The core links nothing
# beyond memcpy, memmove, memset and memcmp: no stack protector or fortify
# helpers, no MSVC security cookie and no default runtime library, and no
# build path recorded in the objects.
function(schway_core_options target)
  set_target_properties(${target} PROPERTIES
    C_VISIBILITY_PRESET hidden
    POSITION_INDEPENDENT_CODE ON)
  target_compile_options(${target} PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-fno-stack-protector -U_FORTIFY_SOURCE -ffile-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}=.>
    $<$<C_COMPILER_ID:MSVC>:/GS- /Zl>)
  target_include_directories(${target} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PROJECT_BINARY_DIR}/generated)
  schway_warnings(${target})
endfunction()

# The core library, built from one object library per module so each object
# is compiled once and a test can link the object that ships.
add_library(schwaystation_instance OBJECT
  src/instance.c
  src/frame.c)
add_library(schwaystation_testcard OBJECT src/testcard.c)
add_library(schwaystation_state OBJECT
  src/state.c
  src/crc32.c)
add_library(schwaystation STATIC
  $<TARGET_OBJECTS:schwaystation_instance>
  $<TARGET_OBJECTS:schwaystation_testcard>
  $<TARGET_OBJECTS:schwaystation_state>)
add_library(schwaystation::schwaystation ALIAS schwaystation)
target_include_directories(schwaystation PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/generated>
  $<INSTALL_INTERFACE:include>)
foreach(core_target IN ITEMS schwaystation schwaystation_instance schwaystation_testcard
    schwaystation_state)
  schway_core_options(${core_target})
endforeach()

# The library component: both headers, the static library, a CMake package
# exporting schwaystation::schwaystation and a pkg-config file. Before 1.0 a
# minor version may break the API, so the package accepts a request only from
# the same MAJOR.MINOR. Both files locate the prefix from their own directory,
# so an extracted archive works wherever it lands. An embedding project
# installs nothing of ours.
if(PROJECT_IS_TOP_LEVEL)
  include(CMakePackageConfigHelpers)
  set(SCHWAYSTATION_CMAKE_DIR lib/cmake/schwaystation)
  install(TARGETS schwaystation EXPORT schwaystationTargets
    ARCHIVE DESTINATION lib COMPONENT library)
  install(FILES include/schwaystation.h ${PROJECT_BINARY_DIR}/generated/schwaystation_version.h
    DESTINATION include COMPONENT library)
  install(EXPORT schwaystationTargets NAMESPACE schwaystation::
    DESTINATION ${SCHWAYSTATION_CMAKE_DIR} COMPONENT library)
  configure_package_config_file(cmake/schwaystationConfig.cmake.in
    ${PROJECT_BINARY_DIR}/schwaystationConfig.cmake
    INSTALL_DESTINATION ${SCHWAYSTATION_CMAKE_DIR})
  write_basic_package_version_file(${PROJECT_BINARY_DIR}/schwaystationConfigVersion.cmake
    COMPATIBILITY SameMinorVersion)
  configure_file(cmake/schwaystation.pc.in ${PROJECT_BINARY_DIR}/schwaystation.pc @ONLY)
  install(FILES
      ${PROJECT_BINARY_DIR}/schwaystationConfig.cmake
      ${PROJECT_BINARY_DIR}/schwaystationConfigVersion.cmake
    DESTINATION ${SCHWAYSTATION_CMAKE_DIR} COMPONENT library)
  install(FILES ${PROJECT_BINARY_DIR}/schwaystation.pc
    DESTINATION lib/pkgconfig COMPONENT library)
endif()

# Host helpers shared by the runner and the libretro core. They are outside
# the core's determinism boundary: nothing here is hashed. Built
# position-independent and hidden, as the libretro module links them.
if(SCHWAYSTATION_BUILD_RUNNER)
  add_library(schwaystation-host STATIC host/xrgb8888.c)
  target_include_directories(schwaystation-host
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/host
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/generated)
  set_target_properties(schwaystation-host PROPERTIES
    C_VISIBILITY_PRESET hidden
    POSITION_INDEPENDENT_CODE ON)
  schway_warnings(schwaystation-host)
  add_subdirectory(runner)
endif()

# Release archives, one per install component.
if(PROJECT_IS_TOP_LEVEL)
  include(cmake/packaging.cmake)
endif()
