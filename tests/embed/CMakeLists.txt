# A C-only project that embeds schwaystation with add_subdirectory, as a host
# using FetchContent would, with the tests, the runner and the libretro core
# off. The test embed.subdirectory builds it and runs it. Configuring fails if
# embedding enabled C++, defined any target besides the library, its object
# libraries and its alias, or registered schwaystation's tests; building
# fails if schwaystation added an install rule.
if(CMAKE_SCRIPT_MODE_FILE)
  # Run after the build with -DINSTALL_SCRIPT=<the subdirectory's cmake_install.cmake>.
  file(READ "${INSTALL_SCRIPT}" install_script)
  if(install_script MATCHES "file\\(INSTALL")
    message(FATAL_ERROR "embed: schwaystation added install rules to an embedding project")
  endif()
  return()
endif()

cmake_minimum_required(VERSION 3.25)
project(embed C)

if(NOT DEFINED SCHWAYSTATION_SOURCE_DIR)
  message(FATAL_ERROR "embed: set SCHWAYSTATION_SOURCE_DIR")
endif()
set(SCHWAYSTATION_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SCHWAYSTATION_BUILD_RUNNER OFF CACHE BOOL "" FORCE)
set(SCHWAYSTATION_BUILD_LIBRETRO OFF CACHE BOOL "" FORCE)
add_subdirectory(${SCHWAYSTATION_SOURCE_DIR} schwaystation)

get_property(languages GLOBAL PROPERTY ENABLED_LANGUAGES)
if("CXX" IN_LIST languages)
  message(FATAL_ERROR "embed: schwaystation enabled C++ in an embedding project")
endif()
foreach(target IN ITEMS schwaystation-run schwaystation_libretro schwaystation-host)
  if(TARGET ${target})
    message(FATAL_ERROR "embed: schwaystation defined the target ${target}")
  endif()
endforeach()
get_property(targets DIRECTORY ${SCHWAYSTATION_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS)
list(SORT targets)
if(NOT targets STREQUAL "schwaystation;schwaystation_instance;schwaystation_state;schwaystation_testcard")
  message(FATAL_ERROR "embed: schwaystation added the targets '${targets}', not only the library")
endif()
if(NOT TARGET schwaystation::schwaystation)
  message(FATAL_ERROR "embed: schwaystation did not add the alias schwaystation::schwaystation")
endif()
get_property(tests DIRECTORY ${SCHWAYSTATION_SOURCE_DIR} PROPERTY TESTS)
if(tests)
  message(FATAL_ERROR "embed: schwaystation registered tests in an embedding project: ${tests}")
endif()

add_executable(embed ${CMAKE_CURRENT_SOURCE_DIR}/../consumer/main.c)
target_link_libraries(embed PRIVATE schwaystation::schwaystation)
add_custom_command(TARGET embed POST_BUILD
  COMMAND ${CMAKE_COMMAND} -DINSTALL_SCRIPT=${CMAKE_BINARY_DIR}/schwaystation/cmake_install.cmake
    -P ${CMAKE_CURRENT_LIST_FILE}
  VERBATIM)
