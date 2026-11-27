# Production targets for the CD-ROM controller, MDEC, and host-side disc images.
# The core modules ship in the library; the image helper remains host-only.
add_library(schwaystation_cdrom OBJECT src/cdrom.c src/cdrom_audio.c src/cdrom_sector.c
  src/cdrom_state.c src/cdxa.c)
add_library(schwaystation_mdec OBJECT src/mdec.c)

if(SCHWAYSTATION_BUILD_RUNNER OR SCHWAYSTATION_BUILD_LIBRETRO)
  include("${PROJECT_SOURCE_DIR}/cmake/Miniz.cmake")
  add_library(schwaystation_disc_image STATIC
    host/disc_image.c host/disc_patch.c host/disc_bps.c runner/crc32.c
    host/disc_sbi.c host/disc_chd.c host/disc_chd_metadata.c
    host/disc_pbp.c host/disc_pbp_metadata.c
    host/disc_ecm.c host/disc_ccd.c)
  target_include_directories(schwaystation_disc_image PUBLIC
    ${PROJECT_SOURCE_DIR}/host
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_BINARY_DIR}/generated)
  target_include_directories(schwaystation_disc_image PRIVATE
    ${PROJECT_SOURCE_DIR}/runner)
  set_target_properties(schwaystation_disc_image PROPERTIES
    C_VISIBILITY_PRESET hidden
    POSITION_INDEPENDENT_CODE ON)
  schway_warnings(schwaystation_disc_image)
  target_link_libraries(schwaystation_disc_image PRIVATE
    schwaystation_pbp_miniz schwaystation_sha256)
  if(SCHWAYSTATION_CHD)
    include(${PROJECT_SOURCE_DIR}/cmake/Libchdr.cmake)
    target_compile_definitions(schwaystation_disc_image PRIVATE SCHWAYSTATION_CHD=1)
    target_link_libraries(schwaystation_disc_image PRIVATE chdr-static)
  else()
    target_compile_definitions(schwaystation_disc_image PRIVATE SCHWAYSTATION_CHD=0)
  endif()
endif()
