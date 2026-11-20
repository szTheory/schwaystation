# Opt-in, build-only CHD fixture creator. This target is separate from the C
# reader and never participates in install or package components.
include("${CMAKE_CURRENT_LIST_DIR}/CHDToolPins.cmake")

set(schway__chdtool_root "${PROJECT_BINARY_DIR}/pins/libchdman-rs")
file(MAKE_DIRECTORY "${schway__chdtool_root}")
set(schway__chdtool_archive "${schway__chdtool_root}/source.tar.gz")
set(schway__chdtool_archive_valid FALSE)
if(EXISTS "${schway__chdtool_archive}")
  file(SHA256 "${schway__chdtool_archive}" schway__chdtool_actual_sha256)
  if(schway__chdtool_actual_sha256 STREQUAL schway__chdtool_sha256)
    set(schway__chdtool_archive_valid TRUE)
  endif()
endif()
if(NOT schway__chdtool_archive_valid)
  file(DOWNLOAD "${schway__chdtool_url}" "${schway__chdtool_archive}"
    EXPECTED_HASH "SHA256=${schway__chdtool_sha256}" STATUS schway__chdtool_download_status)
  list(GET schway__chdtool_download_status 0 schway__chdtool_download_code)
  if(NOT schway__chdtool_download_code EQUAL 0)
    file(REMOVE "${schway__chdtool_archive}")
    message(FATAL_ERROR "CHD tool: pinned archive download failed: ${schway__chdtool_download_status}")
  endif()
endif()

set(schway__chdtool_extract "${schway__chdtool_root}/source")
file(REMOVE_RECURSE "${schway__chdtool_extract}")
file(MAKE_DIRECTORY "${schway__chdtool_extract}")
file(ARCHIVE_EXTRACT INPUT "${schway__chdtool_archive}" DESTINATION "${schway__chdtool_extract}")
file(GLOB schway__chdtool_roots LIST_DIRECTORIES TRUE "${schway__chdtool_extract}/libchdman-rs-*")
list(LENGTH schway__chdtool_roots schway__chdtool_root_count)
if(NOT schway__chdtool_root_count EQUAL 1)
  message(FATAL_ERROR "CHD tool: pinned archive has an unexpected root layout")
endif()
list(GET schway__chdtool_roots 0 schway__chdtool_source)
if(NOT EXISTS "${schway__chdtool_source}/build.rs"
    OR NOT EXISTS "${schway__chdtool_source}/mame-chd-src/MAME_VERSION"
    OR NOT EXISTS "${schway__chdtool_source}/sys/chd_shim.cpp")
  message(FATAL_ERROR "CHD tool: pinned archive is missing required build sources")
endif()

# Check the exact upstream base and notices before any build rule is created.
file(READ "${schway__chdtool_source}/mame-chd-src/MAME_VERSION" schway__chdtool_mame_version)
if(NOT schway__chdtool_mame_version MATCHES "version=0[.]289"
    OR NOT schway__chdtool_mame_version MATCHES "commit=f34f02505e32c1993c6a782b6814232cbfc74e36")
  message(FATAL_ERROR "CHD tool: bundled MAME source version or public base commit changed")
endif()
file(READ "${schway__chdtool_source}/.gitmodules" schway__chdtool_gitmodules)
if(NOT schway__chdtool_gitmodules MATCHES "path = deps/mame"
    OR NOT schway__chdtool_gitmodules MATCHES "url = https://github[.]com/mamedev/mame[.]git")
  message(FATAL_ERROR "CHD tool: unexpected source submodule declaration")
endif()

set(schway__chdtool_licenses
  LICENSE
  mame-chd-src/COPYING
  mame-chd-src/docs/legal/BSD-3-Clause
  mame-chd-src/3rdparty/lzma/DOC/lzma-sdk.txt
  mame-chd-src/3rdparty/zlib/LICENSE
  mame-chd-src/3rdparty/utf8proc/LICENSE.md
  mame-chd-src/3rdparty/zstd/LICENSE
  mame-chd-src/3rdparty/zstd/COPYING
  mame-chd-src/3rdparty/flac/COPYING.Xiph)
foreach(schway__chdtool_license IN LISTS schway__chdtool_licenses)
  if(NOT EXISTS "${schway__chdtool_source}/${schway__chdtool_license}")
    message(FATAL_ERROR "CHD tool: required license file is missing: ${schway__chdtool_license}")
  endif()
endforeach()

set(schway__chdtool_cpp_sources
  sys/chd_shim.cpp
  sys/cd_shim.cpp
  sys/minimal_osd.cpp
  mame-chd-src/src/lib/util/avhuff.cpp
  mame-chd-src/src/lib/util/bitmap.cpp
  mame-chd-src/src/lib/util/cdrom.cpp
  mame-chd-src/src/lib/util/chd.cpp
  mame-chd-src/src/lib/util/chdcodec.cpp
  mame-chd-src/src/lib/util/corefile.cpp
  mame-chd-src/src/lib/util/corestr.cpp
  mame-chd-src/src/lib/util/flac.cpp
  mame-chd-src/src/lib/util/hashing.cpp
  mame-chd-src/src/lib/util/huffman.cpp
  mame-chd-src/src/lib/util/ioprocs.cpp
  mame-chd-src/src/lib/util/md5.cpp
  mame-chd-src/src/lib/util/palette.cpp
  mame-chd-src/src/lib/util/path.cpp
  mame-chd-src/src/lib/util/strformat.cpp
  mame-chd-src/src/lib/util/unicode.cpp
  mame-chd-src/src/lib/util/vecstream.cpp
  mame-chd-src/src/osd/strconv.cpp)
set(schway__chdtool_lzma_sources
  mame-chd-src/3rdparty/lzma/C/7zCrc.c
  mame-chd-src/3rdparty/lzma/C/7zCrcOpt.c
  mame-chd-src/3rdparty/lzma/C/Alloc.c
  mame-chd-src/3rdparty/lzma/C/CpuArch.c
  mame-chd-src/3rdparty/lzma/C/LzFind.c
  mame-chd-src/3rdparty/lzma/C/Lzma2Dec.c
  mame-chd-src/3rdparty/lzma/C/Lzma2Enc.c
  mame-chd-src/3rdparty/lzma/C/LzmaDec.c
  mame-chd-src/3rdparty/lzma/C/LzmaEnc.c)
set(schway__chdtool_zlib_sources
  mame-chd-src/3rdparty/zlib/adler32.c
  mame-chd-src/3rdparty/zlib/compress.c
  mame-chd-src/3rdparty/zlib/crc32.c
  mame-chd-src/3rdparty/zlib/deflate.c
  mame-chd-src/3rdparty/zlib/inffast.c
  mame-chd-src/3rdparty/zlib/inflate.c
  mame-chd-src/3rdparty/zlib/inftrees.c
  mame-chd-src/3rdparty/zlib/trees.c
  mame-chd-src/3rdparty/zlib/uncompr.c
  mame-chd-src/3rdparty/zlib/zutil.c)
set(schway__chdtool_utf8proc_sources mame-chd-src/3rdparty/utf8proc/utf8proc.c)
set(schway__chdtool_zstd_sources
  mame-chd-src/3rdparty/zstd/lib/common/debug.c
  mame-chd-src/3rdparty/zstd/lib/common/xxhash.c
  mame-chd-src/3rdparty/zstd/lib/common/pool.c
  mame-chd-src/3rdparty/zstd/lib/common/entropy_common.c
  mame-chd-src/3rdparty/zstd/lib/common/error_private.c
  mame-chd-src/3rdparty/zstd/lib/common/fse_decompress.c
  mame-chd-src/3rdparty/zstd/lib/common/threading.c
  mame-chd-src/3rdparty/zstd/lib/common/zstd_common.c
  mame-chd-src/3rdparty/zstd/lib/decompress/zstd_decompress_block.c
  mame-chd-src/3rdparty/zstd/lib/decompress/zstd_decompress.c
  mame-chd-src/3rdparty/zstd/lib/decompress/zstd_ddict.c
  mame-chd-src/3rdparty/zstd/lib/decompress/huf_decompress.c
  mame-chd-src/3rdparty/zstd/lib/compress/zstd_lazy.c
  mame-chd-src/3rdparty/zstd/lib/compress/zstd_compress_superblock.c
  mame-chd-src/3rdparty/zstd/lib/compress/zstdmt_compress.c
  mame-chd-src/3rdparty/zstd/lib/compress/fse_compress.c
  mame-chd-src/3rdparty/zstd/lib/compress/zstd_compress_literals.c
  mame-chd-src/3rdparty/zstd/lib/compress/zstd_compress_sequences.c
  mame-chd-src/3rdparty/zstd/lib/compress/zstd_fast.c
  mame-chd-src/3rdparty/zstd/lib/compress/zstd_compress.c
  mame-chd-src/3rdparty/zstd/lib/compress/zstd_opt.c
  mame-chd-src/3rdparty/zstd/lib/compress/zstd_double_fast.c
  mame-chd-src/3rdparty/zstd/lib/compress/zstd_ldm.c
  mame-chd-src/3rdparty/zstd/lib/compress/hist.c
  mame-chd-src/3rdparty/zstd/lib/compress/huf_compress.c)
set(schway__chdtool_flac_sources
  mame-chd-src/3rdparty/flac/src/libFLAC/bitmath.c
  mame-chd-src/3rdparty/flac/src/libFLAC/bitreader.c
  mame-chd-src/3rdparty/flac/src/libFLAC/bitwriter.c
  mame-chd-src/3rdparty/flac/src/libFLAC/cpu.c
  mame-chd-src/3rdparty/flac/src/libFLAC/crc.c
  mame-chd-src/3rdparty/flac/src/libFLAC/fixed.c
  mame-chd-src/3rdparty/flac/src/libFLAC/float.c
  mame-chd-src/3rdparty/flac/src/libFLAC/format.c
  mame-chd-src/3rdparty/flac/src/libFLAC/lpc.c
  mame-chd-src/3rdparty/flac/src/libFLAC/md5.c
  mame-chd-src/3rdparty/flac/src/libFLAC/memory.c
  mame-chd-src/3rdparty/flac/src/libFLAC/metadata_iterators.c
  mame-chd-src/3rdparty/flac/src/libFLAC/metadata_object.c
  mame-chd-src/3rdparty/flac/src/libFLAC/stream_decoder.c
  mame-chd-src/3rdparty/flac/src/libFLAC/stream_encoder.c
  mame-chd-src/3rdparty/flac/src/libFLAC/stream_encoder_framing.c
  mame-chd-src/3rdparty/flac/src/libFLAC/window.c)
if(WIN32)
  list(APPEND schway__chdtool_flac_sources
    mame-chd-src/3rdparty/flac/src/share/win_utf8_io/win_utf8_io.c)
endif()

set(schway__chdtool_sources
  ${schway__chdtool_cpp_sources}
  ${schway__chdtool_lzma_sources}
  ${schway__chdtool_zlib_sources}
  ${schway__chdtool_utf8proc_sources}
  ${schway__chdtool_zstd_sources}
  ${schway__chdtool_flac_sources})
list(LENGTH schway__chdtool_sources schway__chdtool_source_count)
if(WIN32)
  set(schway__chdtool_expected_source_count 84)
else()
  set(schway__chdtool_expected_source_count 83)
endif()
if(NOT schway__chdtool_source_count EQUAL schway__chdtool_expected_source_count)
  message(FATAL_ERROR "CHD tool: expected ${schway__chdtool_expected_source_count} pinned source files, got ${schway__chdtool_source_count}")
endif()
set(schway__chdtool_required_files
  ${schway__chdtool_licenses}
  .gitmodules
  build.rs
  Cargo.toml
  mame-chd-src/MAME_VERSION
  mame-chd-src/3rdparty/utf8proc/utf8proc_data.c
  mame-chd-src/3rdparty/flac/src/libFLAC/deduplication/bitreader_read_rice_signed_block.c
  mame-chd-src/3rdparty/flac/src/libFLAC/deduplication/lpc_compute_autocorrelation_intrin.c
  mame-chd-src/3rdparty/flac/src/libFLAC/deduplication/lpc_compute_autocorrelation_intrin_sse2.c
  mame-chd-src/3rdparty/flac/src/libFLAC/deduplication/lpc_compute_autocorrelation_intrin_neon.c
  ${schway__chdtool_sources})
foreach(schway__chdtool_file IN LISTS schway__chdtool_required_files)
  if(NOT EXISTS "${schway__chdtool_source}/${schway__chdtool_file}")
    message(FATAL_ERROR "CHD tool: required pinned source, include or license is missing: ${schway__chdtool_file}")
  endif()
endforeach()

list(TRANSFORM schway__chdtool_sources PREPEND "${schway__chdtool_source}/")
list(TRANSFORM schway__chdtool_cpp_sources PREPEND "${schway__chdtool_source}/")
list(TRANSFORM schway__chdtool_lzma_sources PREPEND "${schway__chdtool_source}/")
list(TRANSFORM schway__chdtool_zstd_sources PREPEND "${schway__chdtool_source}/")
list(TRANSFORM schway__chdtool_flac_sources PREPEND "${schway__chdtool_source}/")
set(schway__chdtool_include_dirs
  "${schway__chdtool_source}/sys"
  "${schway__chdtool_source}/mame-chd-src/src/lib/util"
  "${schway__chdtool_source}/mame-chd-src/src/osd"
  "${schway__chdtool_source}/mame-chd-src/3rdparty/zlib"
  "${schway__chdtool_source}/mame-chd-src/3rdparty/flac/include"
  "${schway__chdtool_source}/mame-chd-src/3rdparty/flac/src/libFLAC/include"
  "${schway__chdtool_source}/mame-chd-src/3rdparty/flac/src/libFLAC/include/private"
  "${schway__chdtool_source}/mame-chd-src/3rdparty/flac/src/libFLAC/include/protected"
  "${schway__chdtool_source}/mame-chd-src/3rdparty/lzma/C"
  "${schway__chdtool_source}/mame-chd-src/3rdparty"
  "${schway__chdtool_source}/mame-chd-src/3rdparty/zstd/lib"
  "${schway__chdtool_source}/mame-chd-src/3rdparty/zstd/lib/common"
  "${schway__chdtool_source}/mame-chd-src/3rdparty/utf8proc")

add_library(schwaystation_chd_tool_core OBJECT ${schway__chdtool_sources})
set_target_properties(schwaystation_chd_tool_core PROPERTIES
  C_STANDARD 17
  C_STANDARD_REQUIRED ON
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
  POSITION_INDEPENDENT_CODE ON)
target_include_directories(schwaystation_chd_tool_core PRIVATE ${schway__chdtool_include_dirs})
target_compile_definitions(schwaystation_chd_tool_core PRIVATE
  FLAC__NO_ASM
  ZSTD_DISABLE_ASM)
set_source_files_properties(${schway__chdtool_lzma_sources} PROPERTIES COMPILE_DEFINITIONS Z7_ST)
set_source_files_properties(${schway__chdtool_zstd_sources} PROPERTIES COMPILE_DEFINITIONS ZSTD_DISABLE_ASM)
if(MSVC)
  # Pinned C++ sources reach MSVC's <limits> before defining its float.h macros.
  set_source_files_properties(${schway__chdtool_cpp_sources} PROPERTIES COMPILE_OPTIONS "/FIfloat.h")
endif()
set(schway__chdtool_flac_posix_definition "")
if(UNIX)
  # FLAC's C sources use POSIX APIs that strict C17 hides without this feature level.
  set(schway__chdtool_flac_posix_definition "_POSIX_C_SOURCE=200809L")
  # Its compatibility header calls strcasecmp without including its declaration.
  set_source_files_properties(${schway__chdtool_flac_sources} PROPERTIES
    COMPILE_OPTIONS "-include;strings.h")
endif()
set_source_files_properties(${schway__chdtool_flac_sources} PROPERTIES
  COMPILE_DEFINITIONS "FLAC__NO_ASM;HAVE_CONFIG_H;NDEBUG;FLAC__HAS_OGG=0;HAVE_LROUND=1;HAVE_INTTYPES_H=1;HAVE_STDBOOL_H=1;HAVE_STDINT_H=1;HAVE_STDIO_H=1;HAVE_STDLIB_H=1;HAVE_STRING_H=1;SIZE_T_MAX=UINT64_MAX;${schway__chdtool_flac_posix_definition}")
if(WIN32)
  set_source_files_properties(
    "${schway__chdtool_source}/mame-chd-src/3rdparty/flac/src/share/win_utf8_io/win_utf8_io.c"
    PROPERTIES COMPILE_DEFINITIONS "FLAC__NO_DLL;UTF8PROC_STATIC")
  set_source_files_properties(
    "${schway__chdtool_source}/mame-chd-src/3rdparty/utf8proc/utf8proc.c"
    PROPERTIES COMPILE_DEFINITIONS UTF8PROC_STATIC)
  target_compile_definitions(schwaystation_chd_tool_core PRIVATE
    OSD_WINDOWS UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0602
    NTDDI_VERSION=0x06020000 FLAC__NO_DLL _CRT_NONSTDC_NO_DEPRECATE
    _CRT_SECURE_NO_DEPRECATE _CRT_STDIO_LEGACY_WIDE_SPECIFIERS)
  target_compile_definitions(schwaystation_chd_tool_core PRIVATE CRLF=3)
elseif(APPLE)
  target_compile_definitions(schwaystation_chd_tool_core PRIVATE
    SDLMAME_UNIX SDLMAME_MACOSX SDLMAME_DARWIN CRLF=2)
else()
  target_compile_definitions(schwaystation_chd_tool_core PRIVATE
    SDLMAME_UNIX SDLMAME_LINUX CRLF=2)
endif()
include(TestBigEndian)
test_big_endian(schway__chdtool_big_endian)
if(NOT schway__chdtool_big_endian)
  target_compile_definitions(schwaystation_chd_tool_core PRIVATE LSB_FIRST)
endif()
target_compile_options(schwaystation_chd_tool_core PRIVATE
  $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-w>
  $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-w>
  $<$<C_COMPILER_ID:MSVC>:/w>
  $<$<CXX_COMPILER_ID:MSVC>:/w>)

find_package(Threads REQUIRED)
add_executable(schwaystation-chd-tool EXCLUDE_FROM_ALL tools/chd-tool/main.cpp
  $<TARGET_OBJECTS:schwaystation_chd_tool_core>)
target_include_directories(schwaystation-chd-tool SYSTEM PRIVATE
  ${schway__chdtool_include_dirs})
set_target_properties(schwaystation-chd-tool PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF)
target_compile_options(schwaystation-chd-tool PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion>
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /utf-8>)
target_link_libraries(schwaystation-chd-tool PRIVATE Threads::Threads)
if(UNIX AND NOT APPLE)
  target_link_libraries(schwaystation-chd-tool PRIVATE m)
endif()
set(schway__chdtool_bin "${PROJECT_BINARY_DIR}/tools")
set_target_properties(schwaystation-chd-tool PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${schway__chdtool_bin}")
foreach(schway__chdtool_config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
  set_target_properties(schwaystation-chd-tool PROPERTIES
    "RUNTIME_OUTPUT_DIRECTORY_${schway__chdtool_config}" "${schway__chdtool_bin}")
endforeach()
set(SCHWAYSTATION_CHD_TOOL "$<TARGET_FILE:schwaystation-chd-tool>")
