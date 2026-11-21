# The libretro core: a module RetroArch loads at run time. Hidden visibility
# here and in the static libraries it links leaves only the retro_*
# functions, which libretro.h marks with default visibility or dllexport, in
# the export table. The core archive is the same schwaystation library the
# abi tests check.
add_library(schwaystation_libretro MODULE libretro.c)
target_include_directories(schwaystation_libretro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(schwaystation_libretro PRIVATE schwaystation::schwaystation
  schwaystation-host)
target_compile_definitions(schwaystation_libretro PRIVATE
  SCHWAYSTATION_LIBRETRO_VERSION="${PROJECT_VERSION}"
  SCHWAYSTATION_CHD=$<BOOL:${SCHWAYSTATION_CHD}>)
# No "lib" prefix: the frontend and the release name it schwaystation_libretro.
# CMake gives a MODULE the suffix .so on macOS; RetroArch expects .dylib.
set_target_properties(schwaystation_libretro PROPERTIES
  PREFIX ""
  C_VISIBILITY_PRESET hidden)
if(APPLE)
  set_target_properties(schwaystation_libretro PROPERTIES SUFFIX ".dylib")
endif()
schway_warnings(schwaystation_libretro)

# Core information for RetroArch's info directory, with display_version
# taken from version.txt. Project the optional CHD provider from the host
# target's actual link closure, not just the requested option.
get_target_property(info_disc_image_links schwaystation_disc_image LINK_LIBRARIES)
set(SCHWAYSTATION_INFO_EXTENSIONS "exe|psexe|cue|iso|m3u|pbp")
set(SCHWAYSTATION_INFO_LICENSE "CC BY-SA 4.0, ISC, MIT")
set(SCHWAYSTATION_INFO_DESCRIPTION "Sony PlayStation core. Loads PS-EXE homebrew, cue/ISO/M3U discs, and supported unencrypted single-data-track PSISO PBP images.")
set(SCHWAYSTATION_INFO_CHD FALSE)
if("chdr-static" IN_LIST info_disc_image_links)
  string(APPEND SCHWAYSTATION_INFO_EXTENSIONS "|chd")
  set(SCHWAYSTATION_INFO_LICENSE "BSD-3-Clause, CC BY-SA 4.0, ISC, MIT, Public domain")
  set(SCHWAYSTATION_INFO_DESCRIPTION "Sony PlayStation core. Loads PS-EXE homebrew, cue/ISO/M3U and CHD discs, and supported unencrypted single-data-track PSISO PBP images.")
  set(SCHWAYSTATION_INFO_CHD TRUE)
endif()
configure_file(schwaystation_libretro.info.in
  ${CMAKE_CURRENT_BINARY_DIR}/schwaystation_libretro.info @ONLY)

# Fault prevented: a functional .info claim can enable a host path without a
# registered test exercising it.
set(info_claims_script ${PROJECT_SOURCE_DIR}/tests/cmake/info_claims.cmake)
set(info_claims_file ${CMAKE_CURRENT_BINARY_DIR}/schwaystation_libretro.info)
set(info_claims_inventory ${PROJECT_SOURCE_DIR}/tests/libretro/inventory.txt)
execute_process(
  COMMAND ${CMAKE_COMMAND} -DINFO_FILE=${info_claims_file}
    -DINVENTORY=${info_claims_inventory}
    -DEXPECTED_EXTENSIONS=${SCHWAYSTATION_INFO_EXTENSIONS}
    -DEXPECTED_LICENSE=${SCHWAYSTATION_INFO_LICENSE}
    -P ${info_claims_script}
  RESULT_VARIABLE info_claims_result
  OUTPUT_VARIABLE info_claims_out
  ERROR_VARIABLE info_claims_err)
if(NOT info_claims_result STREQUAL "0")
  message(FATAL_ERROR "libretro .info claims guard failed: ${info_claims_out}${info_claims_err}")
endif()

# RetroArch's layout: the module in cores/, its information file in info/.
# RUNTIME covers the Windows DLL. An embedding project installs nothing of
# ours.
if(PROJECT_IS_TOP_LEVEL)
  install(TARGETS schwaystation_libretro
    LIBRARY DESTINATION cores COMPONENT libretro
    RUNTIME DESTINATION cores COMPONENT libretro)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/schwaystation_libretro.info
    DESTINATION info COMPONENT libretro)
endif()
