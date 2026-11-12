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
add_dependencies(schwaystation-test-libretro-host schwaystation-fixtures)

# Fault prevented: the core RetroArch loads shows a frame, rate, geometry or
# audio length other than the library's, needs content to start, loses its
# region option, or resets or restores a state other than the library does.
# ntsc-u answers core options version 2 and takes batch audio; pal answers no
# options version and takes audio pair by pair.
foreach(case IN ITEMS "libretro.host;ntsc-u" "libretro.host.pal;pal")
  list(GET case 0 name)
  list(GET case 1 mode)
  schway_add_test(NAME ${name}
    COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
      $<TARGET_FILE:schwaystation_libretro> ${mode}
    LABELS libretro portable TIMEOUT 60)
endforeach()

# Fault prevented: a fullpath PS-EXE that cannot run with the built-in stub
# kernel, or whose SIO input never changes the guest's independently known GPU pixel.
schway_add_test(NAME libretro.content.no_bios
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
    $<TARGET_FILE:schwaystation_libretro> content-no-bios ${PROJECT_BINARY_DIR}/fixtures/pad_counter.exe
  LABELS libretro portable TIMEOUT 120)

# Fault prevented: selected BIOS files silently fall back or replace active
# state on failure, or PS-EXE content accidentally inherits the BIOS option.
schway_add_test(NAME libretro.content.lifecycle
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
    $<TARGET_FILE:schwaystation_libretro> content-lifecycle
    ${PROJECT_BINARY_DIR}/fixtures/pad_counter.exe ${PROJECT_BINARY_DIR}/tests
  LABELS libretro portable TIMEOUT 120)

# Fault prevented: libretro hides SAVE_RAM, writes the frontend-owned first
# card, or fails to load and persist the independent second card.
schway_add_test(NAME libretro.cards.persistence
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
    $<TARGET_FILE:schwaystation_libretro> card-persistence
    ${PROJECT_BINARY_DIR}/fixtures/memcard_roundtrip.exe ${PROJECT_BINARY_DIR}/tests
  LABELS libretro portable TIMEOUT 120)

# Fault prevented: a controller layout, axis endpoint, or once-per-frame input latch diverges from guest play.
schway_add_test(NAME libretro.input.controls
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
    $<TARGET_FILE:schwaystation_libretro> input-controls ${PROJECT_BINARY_DIR}/fixtures/pad_counter.exe
  LABELS libretro portable TIMEOUT 120)

schway_add_test(NAME libretro.input.movies
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
    $<TARGET_FILE:schwaystation_libretro> input-movies ${PROJECT_BINARY_DIR}/fixtures/pad_counter.exe
  LABELS libretro portable TIMEOUT 120)

# Fault prevented: absent interfaces, zero motors, reset or unload leave a stale rumble report.
schway_add_test(NAME libretro.rumble.controls
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
    $<TARGET_FILE:schwaystation_libretro> rumble-controls ${PROJECT_BINARY_DIR}/tests/rumble_controls_guest.exe
  LABELS libretro portable TIMEOUT 120)

# Fault prevented: real guest 43h/4Dh/42h transactions fail to reach both host rumble motors.
schway_add_test(NAME libretro.rumble.guest
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
    $<TARGET_FILE:schwaystation_libretro> rumble-guest ${PROJECT_BINARY_DIR}/tests/rumble_guest.exe
  LABELS libretro portable TIMEOUT 120)

# Fault prevented: a native packed-RGB24 frame is dropped instead of converted,
# dimension changes fail to notify, or a rate notification is repeated.
schway_add_test(NAME libretro.video.geometry
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
    $<TARGET_FILE:schwaystation_libretro> video-geometry ${PROJECT_BINARY_DIR}/tests/native_video.exe
  LABELS libretro portable TIMEOUT 120)

schway_add_test(NAME libretro.video.geometry_edges
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-libretro-host>
    $<TARGET_FILE:schwaystation_libretro> video-geometry-edges ${PROJECT_BINARY_DIR}/tests/native_video_edges.exe
  LABELS libretro portable TIMEOUT 120)
