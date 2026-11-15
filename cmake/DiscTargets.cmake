# Production targets for the CD-ROM controller, MDEC, and host-side disc images.
# The core modules ship in the library; the image helper remains host-only.
add_library(schwaystation_cdrom OBJECT src/cdrom.c src/cdrom_sector.c src/cdrom_state.c)
add_library(schwaystation_mdec OBJECT src/mdec.c)

if(SCHWAYSTATION_BUILD_RUNNER OR SCHWAYSTATION_BUILD_LIBRETRO)
  add_library(schwaystation_disc_image STATIC host/disc_image.c)
  target_include_directories(schwaystation_disc_image PUBLIC
    ${PROJECT_SOURCE_DIR}/host
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_BINARY_DIR}/generated)
  set_target_properties(schwaystation_disc_image PROPERTIES
    C_VISIBILITY_PRESET hidden
    POSITION_INDEPENDENT_CODE ON)
  schway_warnings(schwaystation_disc_image)
endif()
