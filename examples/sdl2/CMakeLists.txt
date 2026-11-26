cmake_minimum_required(VERSION 3.25)
project(schwaystation_sdl2_example LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
find_package(schwaystation CONFIG REQUIRED)

set(SCHWAY_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
include(GenerateExportHeader)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
add_library(schwaystation_sdl2_pbp_miniz STATIC
  "${SCHWAY_ROOT}/host/miniz/miniz_tinfl.c"
  "${SCHWAY_ROOT}/host/pbp_inflate.c")
target_include_directories(schwaystation_sdl2_pbp_miniz PUBLIC
  "${SCHWAY_ROOT}/host"
  "${SCHWAY_ROOT}/host/miniz"
  "${SCHWAY_ROOT}/host/miniz_compat"
  "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_compile_definitions(schwaystation_sdl2_pbp_miniz PUBLIC
  MINIZ_NO_MALLOC MINIZ_NO_STDIO MINIZ_NO_TIME
  MINIZ_HAS_64BIT_REGISTERS=0
  MINIZ_USE_UNALIGNED_LOADS_AND_STORES=0
  MINIZ_LITTLE_ENDIAN=0
  tinfl_decompress=schway__pbp_tinfl_decompress
  tinfl_decompress_mem_to_heap=schway__pbp_tinfl_decompress_mem_to_heap
  tinfl_decompress_mem_to_mem=schway__pbp_tinfl_decompress_mem_to_mem
  tinfl_decompress_mem_to_callback=schway__pbp_tinfl_decompress_mem_to_callback)
generate_export_header(schwaystation_sdl2_pbp_miniz
  BASE_NAME miniz
  EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/generated/miniz_export.h")
add_executable(schwaystation-sdl2
  main.c
  "${SCHWAY_ROOT}/host/disc_image.c"
  "${SCHWAY_ROOT}/host/disc_patch.c"
  "${SCHWAY_ROOT}/host/disc_bps.c"
  "${SCHWAY_ROOT}/host/disc_sbi.c"
  "${SCHWAY_ROOT}/host/disc_chd.c"
  "${SCHWAY_ROOT}/host/disc_ecm.c"
  "${SCHWAY_ROOT}/host/disc_ccd.c"
  "${SCHWAY_ROOT}/host/disc_pbp.c"
  "${SCHWAY_ROOT}/host/disc_pbp_metadata.c"
  "${SCHWAY_ROOT}/runner/crc32.c"
  "${SCHWAY_ROOT}/runner/sha256.c"
  "${SCHWAY_ROOT}/host/xrgb8888.c")
target_include_directories(schwaystation-sdl2 PRIVATE
  "${SCHWAY_ROOT}/host"
  "${SCHWAY_ROOT}/runner")
target_compile_definitions(schwaystation-sdl2 PRIVATE SCHWAYSTATION_CHD=0)
target_link_libraries(schwaystation-sdl2 PRIVATE
  schwaystation::schwaystation PkgConfig::SDL2 schwaystation_sdl2_pbp_miniz)
