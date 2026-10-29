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
  SCHWAYSTATION_LIBRETRO_VERSION="${PROJECT_VERSION}")
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
# taken from version.txt.
configure_file(schwaystation_libretro.info.in
  ${CMAKE_CURRENT_BINARY_DIR}/schwaystation_libretro.info @ONLY)

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
