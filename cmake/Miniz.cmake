include(GenerateExportHeader)

set(schway__pbp_tinfl_symbol_aliases
  tinfl_decompress=schway__pbp_tinfl_decompress
  tinfl_decompress_mem_to_heap=schway__pbp_tinfl_decompress_mem_to_heap
  tinfl_decompress_mem_to_mem=schway__pbp_tinfl_decompress_mem_to_mem
  tinfl_decompress_mem_to_callback=schway__pbp_tinfl_decompress_mem_to_callback)

add_library(schwaystation_pbp_miniz STATIC
  "${PROJECT_SOURCE_DIR}/host/miniz/miniz_tinfl.c")
target_include_directories(schwaystation_pbp_miniz PUBLIC
  "${PROJECT_SOURCE_DIR}/host/miniz"
  "${PROJECT_SOURCE_DIR}/host/miniz_compat"
  "${PROJECT_BINARY_DIR}/generated")
target_compile_definitions(schwaystation_pbp_miniz PUBLIC
  MINIZ_NO_MALLOC
  MINIZ_NO_STDIO
  MINIZ_NO_TIME
  MINIZ_HAS_64BIT_REGISTERS=0
  MINIZ_USE_UNALIGNED_LOADS_AND_STORES=0
  MINIZ_LITTLE_ENDIAN=0
  ${schway__pbp_tinfl_symbol_aliases})
set_property(SOURCE "${PROJECT_SOURCE_DIR}/host/miniz/miniz_tinfl.c" PROPERTY COMPILE_OPTIONS
  "$<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-Wno-conversion>"
  "$<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-Wno-sign-conversion>")
set_target_properties(schwaystation_pbp_miniz PROPERTIES
  C_STANDARD 17
  C_STANDARD_REQUIRED ON
  C_EXTENSIONS OFF
  C_VISIBILITY_PRESET hidden
  POSITION_INDEPENDENT_CODE ON
  LINKER_LANGUAGE C)
generate_export_header(schwaystation_pbp_miniz
  BASE_NAME miniz
  EXPORT_FILE_NAME "${PROJECT_BINARY_DIR}/generated/miniz_export.h")
schway_warnings(schwaystation_pbp_miniz)
