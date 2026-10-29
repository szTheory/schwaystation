# Release tests, label release: they read the archives the package step
# writes, so the ci workflows run them in a second test step after packaging
# and the first test step excludes them.
set(release_scripts ${CMAKE_CURRENT_LIST_DIR}/../cmake)
set(release_suffix ${SCHWAYSTATION_OS}-${SCHWAYSTATION_ARCH})

schway_add_test(NAME release.archive_contents
  COMMAND ${CMAKE_COMMAND} -DDIR=${PROJECT_BINARY_DIR} -DSUFFIX=${release_suffix}
    -DSOURCE_DIR=${PROJECT_SOURCE_DIR} -DBINARY_DIR=${PROJECT_BINARY_DIR}
    -P ${release_scripts}/archive_contents.cmake
  LABELS release TIMEOUT 60)

# The self test checks a libretro zip built without THIRD-PARTY-NOTICES.md
# and expects that file named.
set(release_fixture ${PROJECT_BINARY_DIR}/release/self_test)
set(release_core schwaystation_libretro.so)
if(SCHWAYSTATION_OS STREQUAL "macos")
  set(release_core schwaystation_libretro.dylib)
elseif(SCHWAYSTATION_OS STREQUAL "windows")
  set(release_core schwaystation_libretro.dll)
endif()
file(REMOVE_RECURSE ${release_fixture})
file(COPY ${PROJECT_SOURCE_DIR}/LICENSE DESTINATION ${release_fixture}/stage)
file(WRITE ${release_fixture}/stage/cores/${release_core} "core\n")
file(WRITE ${release_fixture}/stage/info/schwaystation_libretro.info "info\n")
file(MAKE_DIRECTORY ${release_fixture}/zips)
execute_process(
  COMMAND ${CMAKE_COMMAND} -E tar cf
    ${release_fixture}/zips/schwaystation-libretro-${release_suffix}.zip --format=zip
    LICENSE cores info
  WORKING_DIRECTORY ${release_fixture}/stage
  COMMAND_ERROR_IS_FATAL ANY)
set(release_cmd ${CMAKE_COMMAND} -DDIR=${release_fixture}/zips -DSUFFIX=${release_suffix}
  -DSOURCE_DIR=${PROJECT_SOURCE_DIR} -DBINARY_DIR=${release_fixture} -DCOMPONENTS=libretro
  -P ${release_scripts}/archive_contents.cmake)
string(REPLACE ";" "$<SEMICOLON>" release_cmd "${release_cmd}")
schway_add_test(NAME release.archive_contents.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${release_cmd}" -DCODE=1
    "-DREGEX=lacks: THIRD-PARTY-NOTICES.md" -P ${release_scripts}/expect_fail.cmake
  LABELS release TIMEOUT 10)
