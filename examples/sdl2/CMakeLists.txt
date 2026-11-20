cmake_minimum_required(VERSION 3.25)
project(schwaystation_sdl2_example LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
find_package(schwaystation CONFIG REQUIRED)

set(SCHWAY_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
add_executable(schwaystation-sdl2
  main.c
  "${SCHWAY_ROOT}/host/disc_image.c"
  "${SCHWAY_ROOT}/host/disc_chd.c"
  "${SCHWAY_ROOT}/host/xrgb8888.c")
target_include_directories(schwaystation-sdl2 PRIVATE "${SCHWAY_ROOT}/host")
target_compile_definitions(schwaystation-sdl2 PRIVATE SCHWAYSTATION_CHD=0)
target_link_libraries(schwaystation-sdl2 PRIVATE schwaystation::schwaystation PkgConfig::SDL2)
