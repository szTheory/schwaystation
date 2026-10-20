# The libretro core loaded in-process by a test host.

# The in-process frontend. It links the library and the shared converter
# itself, runs a reference instance beside the module and compares the
# delivered frame with that instance's conversion byte for byte.
add_executable(schwaystation-test-libretro-host ${CMAKE_CURRENT_LIST_DIR}/host.c)
target_include_directories(schwaystation-test-libretro-host PRIVATE ${PROJECT_SOURCE_DIR}/libretro)
target_link_libraries(schwaystation-test-libretro-host PRIVATE schwaystation::schwaystation
  schwaystation-host ${CMAKE_DL_LIBS})
set_target_properties(schwaystation-test-libretro-host PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-libretro-host)
add_dependencies(schwaystation-test-libretro-host schwaystation_libretro)

# Fault prevented: the core RetroArch loads shows a frame, rate or geometry
# other than the library's, or needs content to start.
schway_add_test(NAME libretro.host
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
    $<TARGET_FILE:schwaystation_libretro>
  LABELS libretro portable TIMEOUT 60)
