# Host-side disc provider behavior through the installed schway_disc callbacks.
add_executable(schwaystation-test-disc-fixture
  ${PROJECT_SOURCE_DIR}/tests/disc/fixture.c)
target_include_directories(schwaystation-test-disc-fixture PRIVATE
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_SOURCE_DIR}/runner
  ${PROJECT_SOURCE_DIR}/tests)
target_link_libraries(schwaystation-test-disc-fixture PRIVATE
  schwaystation schwaystation_disc_image)
set_target_properties(schwaystation-test-disc-fixture PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-disc-fixture)

add_executable(schwaystation-test-disc-image
  ${CMAKE_CURRENT_LIST_DIR}/image.c
  ${PROJECT_SOURCE_DIR}/host/disc_probe.c)
target_link_libraries(schwaystation-test-disc-image PRIVATE
  schwaystation_disc_image schwaystation-host)
set_target_properties(schwaystation-test-disc-image PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-disc-image)
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/tests)

add_executable(schwaystation-test-disc-hash
  ${PROJECT_SOURCE_DIR}/tests/probe/disc_hash.c
  ${PROJECT_SOURCE_DIR}/tests/probe/rc_hash_disc.c
  ${PROJECT_SOURCE_DIR}/runner/md5.c
  ${PROJECT_SOURCE_DIR}/host/disc_probe.c)
if(MSVC)
  set_source_files_properties(${PROJECT_SOURCE_DIR}/tests/probe/rc_hash_disc.c
    PROPERTIES COMPILE_OPTIONS /w)
else()
  set_source_files_properties(${PROJECT_SOURCE_DIR}/tests/probe/rc_hash_disc.c
    PROPERTIES COMPILE_OPTIONS -w)
endif()
target_include_directories(schwaystation-test-disc-hash PRIVATE
  ${PROJECT_SOURCE_DIR}/host
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/runner)
target_link_libraries(schwaystation-test-disc-hash PRIVATE
  schwaystation::schwaystation schwaystation-host schwaystation_disc_image)
set_target_properties(schwaystation-test-disc-hash PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-disc-hash)

schway_add_test(NAME disc.hash
  COMMAND schwaystation-test-disc-hash ${PROJECT_BINARY_DIR}/fixtures/disc_boot.cue
  LABELS disc portable TIMEOUT 30)

schway_add_test(NAME disc.image.iso_sector
  COMMAND schwaystation-test-disc-image iso_sector ${PROJECT_BINARY_DIR}/tests
  LABELS disc portable TIMEOUT 10)

foreach(case cue_index_limits cue_limits cue_modes cue_pregap cue_reject cue_symlink iso_limits)
  schway_add_test(NAME disc.image.${case}
    COMMAND schwaystation-test-disc-image ${case} ${PROJECT_BINARY_DIR}/tests
    LABELS disc portable TIMEOUT 20)
endforeach()

schway_add_test(NAME disc.playlist
  COMMAND schwaystation-test-disc-image playlist ${PROJECT_BINARY_DIR}/tests
  LABELS disc portable TIMEOUT 20)

schway_add_test(NAME disc.cnf
  COMMAND schwaystation-test-disc-image cnf ${PROJECT_BINARY_DIR}/tests
  LABELS disc portable TIMEOUT 20)

schway_add_test(NAME disc.fixture.boot
  COMMAND schwaystation-test-disc-fixture boot
    "${PROJECT_BINARY_DIR}/openbios/openbios-fast.bin"
    "${PROJECT_SOURCE_DIR}/tests/built/fast-expected.txt"
    "${PROJECT_BINARY_DIR}/fixtures/disc_boot.cue"
    "${PROJECT_SOURCE_DIR}/tests/fixtures/src/disc_boot.sbi"
    "${PROJECT_SOURCE_DIR}/tests/diag/expected.csv"
  LABELS disc portable TIMEOUT 180)

schway_add_test(NAME disc.fixture.audio
  COMMAND schwaystation-test-disc-fixture audio
    "${PROJECT_BINARY_DIR}/openbios/openbios-fast.bin"
    "${PROJECT_SOURCE_DIR}/tests/built/fast-expected.txt"
    "${PROJECT_BINARY_DIR}/fixtures/disc_boot.cue"
    "${PROJECT_SOURCE_DIR}/tests/fixtures/src/disc_boot.sbi"
    "${PROJECT_SOURCE_DIR}/tests/diag/expected.csv"
  LABELS disc portable TIMEOUT 300)

schway_add_test(NAME disc.fixture.fmv
  COMMAND schwaystation-test-disc-fixture fmv
    "${PROJECT_BINARY_DIR}/openbios/openbios-fast.bin"
    "${PROJECT_SOURCE_DIR}/tests/built/fast-expected.txt"
    "${PROJECT_BINARY_DIR}/fixtures/disc_boot.cue"
    "${PROJECT_SOURCE_DIR}/tests/fixtures/src/disc_boot.sbi"
    "${PROJECT_SOURCE_DIR}/tests/diag/expected.csv"
    "${PROJECT_BINARY_DIR}/disc/predicates.txt"
  LABELS disc portable TIMEOUT 300)
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/disc")
set_tests_properties(disc.fixture.fmv PROPERTIES FIXTURES_SETUP disc_fmv_predicates)

schway_add_test(NAME disc.fixture.read
  COMMAND schwaystation-test-disc-fixture read
    "${PROJECT_BINARY_DIR}/openbios/openbios-fast.bin"
    "${PROJECT_SOURCE_DIR}/tests/built/fast-expected.txt"
    "${PROJECT_BINARY_DIR}/fixtures/disc_boot.cue"
    "${PROJECT_SOURCE_DIR}/tests/fixtures/src/disc_boot.sbi"
    "${PROJECT_SOURCE_DIR}/tests/diag/expected.csv"
  LABELS disc portable TIMEOUT 300)
