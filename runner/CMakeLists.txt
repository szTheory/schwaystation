# The headless runner, built as ISO C17. It lands at the top of the build
# directory: build/<preset>/schwaystation-run. _POSIX_C_SOURCE keeps the
# POSIX interfaces later runner sources need visible under -std=c17 without
# other extensions.
add_executable(schwaystation-run
  main.c
  sha256.c)
set_target_properties(schwaystation-run PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
target_link_libraries(schwaystation-run PRIVATE schwaystation::schwaystation schwaystation-host)
target_compile_definitions(schwaystation-run PRIVATE _POSIX_C_SOURCE=200809L)
schway_warnings(schwaystation-run)
install(TARGETS schwaystation-run RUNTIME DESTINATION bin COMPONENT runner)
