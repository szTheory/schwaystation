# Production targets for the CD-ROM controller, MDEC, and host-side disc images.
# The core modules ship in the library; the image helper remains host-only.
add_library(schwaystation_cdrom OBJECT src/cdrom.c)
add_library(schwaystation_mdec OBJECT src/mdec.c)

if(SCHWAYSTATION_BUILD_RUNNER OR SCHWAYSTATION_BUILD_LIBRETRO)
  add_library(schwaystation_disc_image STATIC host/disc_image.c)
  target_include_directories(schwaystation_disc_image PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/host)
  set_target_properties(schwaystation_disc_image PROPERTIES
    C_VISIBILITY_PRESET hidden
    POSITION_INDEPENDENT_CODE ON)
  schway_warnings(schwaystation_disc_image)
endif()
