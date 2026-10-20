# The headless runner, built as ISO C17. It lands at the top of the build
# directory: build/<preset>/schwaystation-run. _POSIX_C_SOURCE keeps the
# POSIX interfaces later runner sources need visible under -std=c17 without
# other extensions.

# The runner's sources, absolute, so the tests can build a second runner from
# the same list.
set(SCHWAYSTATION_RUNNER_SOURCES)
foreach(src main.c bench.c clock.c crc32.c png.c rss.c sha256.c stop.c)
  list(APPEND SCHWAYSTATION_RUNNER_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/${src})
endforeach()
set(SCHWAYSTATION_RUNNER_SOURCES ${SCHWAYSTATION_RUNNER_SOURCES} PARENT_SCOPE)

# The Git revision the build is configured from, for --version; "unknown"
# outside a Git checkout. It is read again at every configure, never at build
# time, so a commit after the configure leaves it stale, which the revision
# test catches. The core never sees it.
set(revision unknown)
find_package(Git QUIET)
if(GIT_FOUND)
  execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(rc EQUAL 0 AND out MATCHES "^[0-9a-f]+$")
    set(revision ${out})
  endif()
endif()
file(CONFIGURE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated/revision.h
  CONTENT "#define SCHWAY__RUNNER_REVISION \"@revision@\"\n" @ONLY)
set(SCHWAYSTATION_RUNNER_INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/generated PARENT_SCOPE)

add_executable(schwaystation-run ${SCHWAYSTATION_RUNNER_SOURCES})
target_include_directories(schwaystation-run PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)
set_target_properties(schwaystation-run PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
target_link_libraries(schwaystation-run PRIVATE schwaystation::schwaystation schwaystation-host)
target_compile_definitions(schwaystation-run PRIVATE _POSIX_C_SOURCE=200809L)
schway_warnings(schwaystation-run)
install(TARGETS schwaystation-run RUNTIME DESTINATION bin COMPONENT runner)
