# Host-side disc provider behavior through the installed schway_disc callbacks.
add_executable(schwaystation-test-disc-image ${CMAKE_CURRENT_LIST_DIR}/image.c)
target_link_libraries(schwaystation-test-disc-image PRIVATE schwaystation_disc_image)
set_target_properties(schwaystation-test-disc-image PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-disc-image)
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/tests)

schway_add_test(NAME disc.image.iso_sector
  COMMAND schwaystation-test-disc-image iso_sector ${PROJECT_BINARY_DIR}/tests
  LABELS disc portable TIMEOUT 10)

foreach(case cue_index_limits cue_limits cue_modes cue_pregap cue_reject cue_symlink iso_limits)
  schway_add_test(NAME disc.image.${case}
    COMMAND schwaystation-test-disc-image ${case} ${PROJECT_BINARY_DIR}/tests
    LABELS disc portable TIMEOUT 20)
endforeach()
