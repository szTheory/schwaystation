# Host-side disc provider behavior through the installed schway_disc callbacks.
if(SCHWAYSTATION_CHD AND NOT SCHWAYSTATION_BUILD_CHD_TOOL
    AND NOT SCHWAYSTATION_CHD_TOOL_EXECUTABLE)
  message(FATAL_ERROR "CHD tests need an in-tree or external run-local fixture generator")
endif()

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
  ${PROJECT_SOURCE_DIR}/host/disc_probe.c
  ${PROJECT_SOURCE_DIR}/host/save_game_identity.c
  ${PROJECT_SOURCE_DIR}/runner/sha256.c)
target_include_directories(schwaystation-test-disc-image PRIVATE
  ${PROJECT_SOURCE_DIR}/host ${PROJECT_SOURCE_DIR}/runner)
target_link_libraries(schwaystation-test-disc-image PRIVATE
  schwaystation_disc_image schwaystation-host)
set_target_properties(schwaystation-test-disc-image PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-disc-image)
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/tests)

add_executable(schwaystation-test-disc-media ${CMAKE_CURRENT_LIST_DIR}/media.c)
target_link_libraries(schwaystation-test-disc-media PRIVATE schwaystation_disc_image)
if(SCHWAYSTATION_CHD)
  target_sources(schwaystation-test-disc-media PRIVATE
    ${PROJECT_SOURCE_DIR}/runner/sha256.c
    ${PROJECT_SOURCE_DIR}/host/disc_probe.c
    ${PROJECT_SOURCE_DIR}/host/save_game_identity.c)
  target_include_directories(schwaystation-test-disc-media PRIVATE
    ${PROJECT_SOURCE_DIR}/runner ${PROJECT_SOURCE_DIR}/host)
  target_link_libraries(schwaystation-test-disc-media PRIVATE schwaystation-host)
  target_compile_definitions(schwaystation-test-disc-media PRIVATE SCHWAYSTATION_CHD=1)
else()
  target_compile_definitions(schwaystation-test-disc-media PRIVATE SCHWAYSTATION_CHD=0)
endif()
set_target_properties(schwaystation-test-disc-media PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-disc-media)

add_executable(schwaystation-test-disc-chd ${CMAKE_CURRENT_LIST_DIR}/chd.c)
target_include_directories(schwaystation-test-disc-chd PRIVATE ${PROJECT_SOURCE_DIR}/tests)
target_link_libraries(schwaystation-test-disc-chd PRIVATE schwaystation_disc_image)
if(SCHWAYSTATION_CHD)
  target_compile_definitions(schwaystation-test-disc-chd PRIVATE SCHWAYSTATION_CHD=1)
else()
  target_compile_definitions(schwaystation-test-disc-chd PRIVATE SCHWAYSTATION_CHD=0)
endif()
set_target_properties(schwaystation-test-disc-chd PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-disc-chd)

add_executable(schwaystation-test-disc-pbp
  ${CMAKE_CURRENT_LIST_DIR}/pbp.c
  ${PROJECT_SOURCE_DIR}/runner/sha256.c)
target_include_directories(schwaystation-test-disc-pbp PRIVATE ${PROJECT_SOURCE_DIR}/runner)
target_link_libraries(schwaystation-test-disc-pbp PRIVATE schwaystation_disc_image)
set_target_properties(schwaystation-test-disc-pbp PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-disc-pbp)

add_executable(schwaystation-test-disc-legacy
  ${CMAKE_CURRENT_LIST_DIR}/legacy.c
  ${PROJECT_SOURCE_DIR}/host/disc_probe.c
  ${PROJECT_SOURCE_DIR}/host/save_game_identity.c
  ${PROJECT_SOURCE_DIR}/runner/sha256.c)
target_include_directories(schwaystation-test-disc-legacy PRIVATE
  ${PROJECT_SOURCE_DIR}/host ${PROJECT_SOURCE_DIR}/runner ${PROJECT_SOURCE_DIR}/tests)
target_link_libraries(schwaystation-test-disc-legacy PRIVATE
  schwaystation_disc_image schwaystation-host)
set_target_properties(schwaystation-test-disc-legacy PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-disc-legacy)

foreach(format IN ITEMS ecm ccd)
  add_executable(schwaystation-test-disc-${format}
    ${CMAKE_CURRENT_LIST_DIR}/${format}.c)
  target_link_libraries(schwaystation-test-disc-${format} PRIVATE
    schwaystation_disc_image)
  set_target_properties(schwaystation-test-disc-${format} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
  schway_warnings(schwaystation-test-disc-${format})
  schway_add_test(NAME disc.image.${format}
    COMMAND schwaystation-test-disc-${format} "${PROJECT_BINARY_DIR}/tests"
    LABELS disc portable TIMEOUT 15)
endforeach()

schway_add_test(NAME disc.image.pbp_headers
  COMMAND schwaystation-test-disc-pbp ${PROJECT_BINARY_DIR}/tests
  LABELS disc portable TIMEOUT 15)
schway_add_test(NAME disc.image.pbp_inflate
  COMMAND schwaystation-test-disc-pbp --inflate
  LABELS disc portable TIMEOUT 15)
schway_add_test(NAME disc.image.pbp_metadata
  COMMAND schwaystation-test-disc-pbp --metadata
    "${PROJECT_BINARY_DIR}/fixtures/schwaystation-disc_boot.pbp"
  LABELS disc portable TIMEOUT 15)
schway_add_test(NAME disc.image.pbp_raw
  COMMAND schwaystation-test-disc-pbp --raw
    "${PROJECT_BINARY_DIR}/fixtures/schwaystation-disc_boot.pbp"
    "${PROJECT_BINARY_DIR}/tests"
  LABELS disc portable TIMEOUT 15)
schway_add_test(NAME disc.image.pbp_bounds
  COMMAND schwaystation-test-disc-pbp --bounds
    "${PROJECT_BINARY_DIR}/fixtures/schwaystation-disc_boot.pbp"
    "${PROJECT_BINARY_DIR}/tests"
  LABELS disc portable TIMEOUT 15)
schway_add_test(NAME disc.image.pbp_preflight
  COMMAND schwaystation-test-disc-pbp --preflight
    "${PROJECT_BINARY_DIR}/fixtures/schwaystation-disc_boot.pbp"
    "${PROJECT_BINARY_DIR}/fixtures/disc_boot_pbp.cue"
    "${PROJECT_BINARY_DIR}/fixtures/disc_boot_pbp.bin"
    "${PROJECT_BINARY_DIR}/tests/disc_boot_pbp.manifest"
    "${PROJECT_SOURCE_DIR}/tools/gen_pbp.py"
    "${PROJECT_BINARY_DIR}/fixtures/disc_boot_pbp_source.cue"
  LABELS disc portable TIMEOUT 15)
schway_add_test(NAME disc.image.pbp_fixture_controls
  COMMAND schwaystation-test-disc-pbp --fixture-controls
    "${PROJECT_BINARY_DIR}/tests"
    "${PROJECT_BINARY_DIR}/fixtures/schwaystation-disc_boot.pbp"
    "${PROJECT_BINARY_DIR}/fixtures/disc_boot_pbp.cue"
    "${PROJECT_BINARY_DIR}/fixtures/disc_boot_pbp.bin"
    "${PROJECT_BINARY_DIR}/tests/disc_boot_pbp.manifest"
    "${PROJECT_SOURCE_DIR}/tools/gen_pbp.py"
  LABELS disc portable TIMEOUT 30)

schway_add_test(NAME disc.image.legacy_preflight
  COMMAND schwaystation-test-disc-legacy --preflight
    "${PROJECT_BINARY_DIR}/fixtures" "${PROJECT_BINARY_DIR}/tests"
    "${PROJECT_SOURCE_DIR}/tools/gen_ecm.py" "${PROJECT_SOURCE_DIR}/tools/gen_ccd.py"
    64 222
  LABELS disc portable TIMEOUT 15)
schway_add_test(NAME disc.image.legacy_parity
  COMMAND schwaystation-test-disc-legacy --parity "${PROJECT_BINARY_DIR}/fixtures"
  LABELS disc portable TIMEOUT 30)
set_tests_properties(disc.image.pbp_preflight PROPERTIES FIXTURES_SETUP disc_pbp_preflight)
set_tests_properties(disc.image.pbp_fixture_controls disc.image.pbp_metadata
  disc.image.pbp_bounds PROPERTIES FIXTURES_REQUIRED disc_pbp_preflight)
set_tests_properties(disc.image.pbp_raw PROPERTIES FIXTURES_REQUIRED disc_pbp_preflight)
schway_add_test(NAME disc.image.pbp_open
  COMMAND schwaystation-test-disc-image pbp_open
    "${PROJECT_BINARY_DIR}/tests"
    "${PROJECT_BINARY_DIR}/fixtures/schwaystation-disc_boot.pbp"
    "${PROJECT_BINARY_DIR}/fixtures/disc_boot_pbp.cue"
  LABELS disc portable TIMEOUT 20)
schway_add_test(NAME disc.image.pbp_boundaries
  COMMAND schwaystation-test-disc-image pbp_boundaries
    "${PROJECT_BINARY_DIR}/fixtures/schwaystation-disc_boot.pbp"
    "${PROJECT_BINARY_DIR}/fixtures/disc_boot_pbp.cue"
  LABELS disc portable TIMEOUT 20)
schway_add_test(NAME disc.image.pbp_parity
  COMMAND schwaystation-test-disc-image pbp_parity
    "${PROJECT_BINARY_DIR}/fixtures/schwaystation-disc_boot.pbp"
    "${PROJECT_BINARY_DIR}/fixtures/disc_boot_pbp.cue"
  LABELS disc portable TIMEOUT 30)
set_tests_properties(disc.image.pbp_open disc.image.pbp_boundaries disc.image.pbp_parity
  PROPERTIES FIXTURES_REQUIRED disc_pbp_preflight)
set_tests_properties(disc.image.pbp_parity PROPERTIES FIXTURES_SETUP disc_pbp_parity)

schway_add_test(NAME disc.image.chd_headers
  COMMAND schwaystation-test-disc-chd ${PROJECT_BINARY_DIR}/tests
  LABELS disc portable TIMEOUT 15)
schway_add_test(NAME disc.image.chd_metadata
  COMMAND schwaystation-test-disc-chd --metadata-controls
  LABELS disc portable TIMEOUT 15)
if(SCHWAYSTATION_CHD)
  schway_add_test(NAME disc.image.chd_toc
    COMMAND schwaystation-test-disc-chd --toc ${PROJECT_BINARY_DIR}/fixtures/disc_boot.chd
    LABELS disc portable TIMEOUT 15)
  set_tests_properties(disc.image.chd_toc PROPERTIES FIXTURES_REQUIRED disc_media_preflight)
  schway_add_test(NAME disc.image.chd_open
    COMMAND schwaystation-test-disc-chd --open
      "${PROJECT_BINARY_DIR}/fixtures/disc_boot.cue"
      "${PROJECT_BINARY_DIR}/fixtures/disc_boot.chd" "${PROJECT_BINARY_DIR}/tests"
    LABELS disc portable TIMEOUT 20)
  schway_add_test(NAME disc.image.chd_boundaries
    COMMAND schwaystation-test-disc-chd --boundaries
      "${PROJECT_BINARY_DIR}/fixtures/disc_boot.cue"
      "${PROJECT_BINARY_DIR}/fixtures/disc_boot.chd"
    LABELS disc portable TIMEOUT 20)
  schway_add_test(NAME disc.image.chd_parity
    COMMAND schwaystation-test-disc-media --chd-parity
      "${PROJECT_BINARY_DIR}/fixtures/disc_boot.cue"
      "${PROJECT_BINARY_DIR}/fixtures/disc_boot.chd"
    LABELS disc portable TIMEOUT 30)
  set_tests_properties(disc.image.chd_open disc.image.chd_boundaries disc.image.chd_parity
    PROPERTIES FIXTURES_REQUIRED disc_media_preflight)
  set_tests_properties(disc.image.chd_parity PROPERTIES FIXTURES_SETUP disc_chd_parity)
else()
  schway_add_test(NAME disc.image.chd_toc
    COMMAND schwaystation-test-disc-chd --toc disabled-feature.chd
    LABELS disc portable TIMEOUT 15)
  schway_add_test(NAME disc.image.chd_open
    COMMAND schwaystation-test-disc-chd --open disabled-feature.chd
    LABELS disc portable TIMEOUT 15)
  schway_add_test(NAME disc.image.chd_boundaries
    COMMAND schwaystation-test-disc-chd --boundaries disabled-feature.chd
    LABELS disc portable TIMEOUT 15)
  schway_add_test(NAME disc.image.chd_parity
    COMMAND schwaystation-test-disc-media --chd-parity
    LABELS disc portable TIMEOUT 15)
endif()

if(SCHWAYSTATION_CHD)
  schway_add_test(NAME disc.media.preflight
    COMMAND schwaystation-test-disc-media
      "${PROJECT_BINARY_DIR}/fixtures/disc_boot.cue" "${PROJECT_BINARY_DIR}/tests"
      "${PROJECT_BINARY_DIR}/fixtures/disc_boot.chd"
      "${PROJECT_BINARY_DIR}/tests/disc_boot.chd.manifest"
      "${SCHWAYSTATION_CHD_TOOL}"
    LABELS disc portable TIMEOUT 20)
  schway_add_test(NAME disc.media.fixture_controls
    COMMAND schwaystation-test-disc-media --fixture-controls
      "${PROJECT_BINARY_DIR}/fixtures/disc_boot.cue" "${PROJECT_BINARY_DIR}/tests"
      "${PROJECT_BINARY_DIR}/fixtures/disc_boot.chd"
      "${PROJECT_BINARY_DIR}/tests/disc_boot.chd.manifest"
      "${SCHWAYSTATION_CHD_TOOL}"
    LABELS disc portable TIMEOUT 30)
else()
  schway_add_test(NAME disc.media.preflight
    COMMAND schwaystation-test-disc-media
      "${PROJECT_BINARY_DIR}/fixtures/disc_boot.cue" "${PROJECT_BINARY_DIR}/tests"
    LABELS disc portable TIMEOUT 10)
  schway_add_test(NAME disc.media.fixture_controls
    COMMAND schwaystation-test-disc-media --fixture-controls
      "${PROJECT_BINARY_DIR}/fixtures" "${PROJECT_BINARY_DIR}/tests"
    LABELS disc portable TIMEOUT 10)
endif()
set_tests_properties(disc.media.preflight PROPERTIES FIXTURES_SETUP disc_media_preflight)
set_tests_properties(disc.media.fixture_controls PROPERTIES FIXTURES_REQUIRED disc_media_preflight)

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

schway_add_test(NAME disc.image.cue_root_child
  COMMAND schwaystation-test-disc-image cue_root_child ${PROJECT_BINARY_DIR}
  LABELS disc portable TIMEOUT 20)

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

set_tests_properties(disc.fixture.boot disc.fixture.read disc.fixture.audio disc.fixture.fmv
  PROPERTIES FIXTURES_REQUIRED disc_media_preflight)

# Run the exact bounded guest cases against the shipping CHD callbacks. These
# setup fixtures make structure, complete source parity and feature-key checks
# finish before any CHD guest starts.
foreach(case boot read audio fmv)
  set(timeout 300)
  if(case STREQUAL "boot")
    set(timeout 180)
  endif()
  if(case STREQUAL "fmv")
    schway_add_test(NAME disc.fixture.chd.${case}
      COMMAND schwaystation-test-disc-fixture chd-${case}
        "${PROJECT_BINARY_DIR}/openbios/openbios-fast.bin"
        "${PROJECT_SOURCE_DIR}/tests/built/fast-expected.txt"
        "${PROJECT_BINARY_DIR}/fixtures/disc_boot.chd"
        "${PROJECT_SOURCE_DIR}/tests/fixtures/src/disc_boot.sbi"
        "${PROJECT_SOURCE_DIR}/tests/diag/expected.csv"
        "${PROJECT_BINARY_DIR}/disc/predicates.txt"
      LABELS disc portable TIMEOUT ${timeout})
    set_tests_properties(disc.fixture.chd.${case} PROPERTIES
      FIXTURES_REQUIRED "disc_media_preflight;disc_chd_parity;compat_features_preflight;disc_fmv_predicates")
  else()
    schway_add_test(NAME disc.fixture.chd.${case}
      COMMAND schwaystation-test-disc-fixture chd-${case}
        "${PROJECT_BINARY_DIR}/openbios/openbios-fast.bin"
        "${PROJECT_SOURCE_DIR}/tests/built/fast-expected.txt"
        "${PROJECT_BINARY_DIR}/fixtures/disc_boot.chd"
        "${PROJECT_SOURCE_DIR}/tests/fixtures/src/disc_boot.sbi"
        "${PROJECT_SOURCE_DIR}/tests/diag/expected.csv"
      LABELS disc portable TIMEOUT ${timeout})
    set_tests_properties(disc.fixture.chd.${case} PROPERTIES
      FIXTURES_REQUIRED "disc_media_preflight;disc_chd_parity;compat_features_preflight")
  endif()
endforeach()

# The owned PBP fixture proves one 64-sector data track, including XA and the
# first FMV frame. It has no second track or CD-DA sectors.
schway_add_test(NAME disc.fixture.pbp.boot
  COMMAND schwaystation-test-disc-fixture pbp-boot
    "${PROJECT_BINARY_DIR}/openbios/openbios-fast.bin"
    "${PROJECT_SOURCE_DIR}/tests/built/fast-expected.txt"
    "${PROJECT_BINARY_DIR}/fixtures/schwaystation-disc_boot.pbp"
    "${PROJECT_SOURCE_DIR}/tests/fixtures/src/disc_boot.sbi"
    "${PROJECT_SOURCE_DIR}/tests/diag/expected.csv"
  LABELS disc portable TIMEOUT 180)
schway_add_test(NAME disc.fixture.pbp.read
  COMMAND schwaystation-test-disc-fixture pbp-read
    "${PROJECT_BINARY_DIR}/openbios/openbios-fast.bin"
    "${PROJECT_SOURCE_DIR}/tests/built/fast-expected.txt"
    "${PROJECT_BINARY_DIR}/fixtures/schwaystation-disc_boot.pbp"
    "${PROJECT_SOURCE_DIR}/tests/fixtures/src/disc_boot.sbi"
    "${PROJECT_SOURCE_DIR}/tests/diag/expected.csv"
  LABELS disc portable TIMEOUT 300)
schway_add_test(NAME disc.fixture.pbp.fmv
  COMMAND schwaystation-test-disc-fixture pbp-fmv
    "${PROJECT_BINARY_DIR}/openbios/openbios-fast.bin"
    "${PROJECT_SOURCE_DIR}/tests/built/fast-expected.txt"
    "${PROJECT_BINARY_DIR}/fixtures/schwaystation-disc_boot.pbp"
    "${PROJECT_SOURCE_DIR}/tests/fixtures/src/disc_boot.sbi"
    "${PROJECT_SOURCE_DIR}/tests/diag/expected.csv"
  LABELS disc portable TIMEOUT 300)
set_tests_properties(disc.fixture.pbp.boot disc.fixture.pbp.read PROPERTIES
  FIXTURES_REQUIRED "disc_pbp_preflight;disc_pbp_parity;compat_features_preflight")
set_tests_properties(disc.fixture.pbp.fmv PROPERTIES
  FIXTURES_REQUIRED "disc_pbp_preflight;disc_pbp_parity;compat_features_preflight;disc_fmv_predicates")

set_tests_properties(disc.image.legacy_preflight PROPERTIES
  FIXTURES_SETUP disc_legacy_preflight)
set_tests_properties(disc.image.legacy_parity PROPERTIES
  FIXTURES_SETUP disc_legacy_parity FIXTURES_REQUIRED disc_legacy_preflight)
get_property(schway__disc_registered_tests DIRECTORY PROPERTY TESTS)
list(REMOVE_ITEM schway__disc_registered_tests
  disc.image.legacy_preflight disc.image.legacy_parity)
set_property(TEST ${schway__disc_registered_tests} APPEND PROPERTY
  FIXTURES_REQUIRED "disc_legacy_preflight;disc_legacy_parity")
