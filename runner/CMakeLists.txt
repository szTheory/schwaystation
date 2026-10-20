# The headless runner, built as ISO C17. It lands at the top of the build
# directory: build/<preset>/schwaystation-run. _POSIX_C_SOURCE keeps the
# POSIX interfaces later runner sources need visible under -std=c17 without
# other extensions.
# The runner's sources, absolute, so the tests can build a second runner from
# the same list.
set(SCHWAYSTATION_RUNNER_SOURCES)
foreach(src main.c bench.c clock.c crc32.c png.c rss.c sha256.c)
  list(APPEND SCHWAYSTATION_RUNNER_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/${src})
endforeach()
set(SCHWAYSTATION_RUNNER_SOURCES ${SCHWAYSTATION_RUNNER_SOURCES} PARENT_SCOPE)
add_executable(schwaystation-run ${SCHWAYSTATION_RUNNER_SOURCES})
set_target_properties(schwaystation-run PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
target_link_libraries(schwaystation-run PRIVATE schwaystation::schwaystation schwaystation-host)
target_compile_definitions(schwaystation-run PRIVATE _POSIX_C_SOURCE=200809L)
schway_warnings(schwaystation-run)
install(TARGETS schwaystation-run RUNTIME DESTINATION bin COMPONENT runner)
