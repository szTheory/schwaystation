cmake_minimum_required(VERSION 3.25)
if(CMAKE_HOST_WIN32)
  find_package(Git QUIET)
  if(Git_FOUND)
    get_filename_component(git_bin "${GIT_EXECUTABLE}" DIRECTORY)
    set(git_usr_bin "${git_bin}/../usr/bin" "${git_bin}/../../usr/bin")
  endif()
endif()
find_program(SH sh PATHS ${git_usr_bin} REQUIRED)
file(MAKE_DIRECTORY "${BIN}")

function(route name count paths heavy release nightly_full pbp_only)
  set(mode "${ARGV7}")
  file(CONFIGURE OUTPUT "${BIN}/paths.txt" CONTENT "${paths}" @ONLY NEWLINE_STYLE UNIX)
  execute_process(COMMAND "${SH}" "${ROOT}/scripts/classify-changes.sh" "${count}" "${mode}"
    INPUT_FILE "${BIN}/paths.txt" RESULT_VARIABLE rc OUTPUT_VARIABLE result ERROR_VARIABLE err)
  if(NOT rc STREQUAL "0" OR NOT result MATCHES "(^|\n)heavy=${heavy}(\r?\n|$)" OR
     NOT result MATCHES "(^|\n)release_inputs=${release}(\r?\n|$)" OR
     NOT result MATCHES "(^|\n)nightly_full=${nightly_full}(\r?\n|$)" OR
     NOT result MATCHES "(^|\n)pbp_only=${pbp_only}(\r?\n|$)")
    message(FATAL_ERROR "ci.classify_policy: ${name} outputs unexpected: ${result} (exit ${rc}) ${err}")
  endif()
endfunction()

route(planning-maintainer-docs 4 ".planning/STATE.md\n.planning/ROADMAP.md\ndocs/testing.md\ndocs/maintaining.md\n" false false false false)
route(non-nightly-metadata 3 "docs/maintaining.md\nquality.json\nexamples/sdl2/CMakeLists.txt\n" true false false false)
route(pbp-only 4 "host/disc_pbp.c\nhost/disc_pbp.h\ntests/disc/pbp.c\ntests/fuzz/pbp.c\n" true false false true)
route(source 2 ".planning/STATE.md\nsrc/core.c\n" true false true false)
route(release 1 ".github/workflows/release.yml\n" true true true false)
route(empty 1 "" true true true false)
route(oversize 3001 ".planning/STATE.md\n" true true true false)
route(bad-count invalid ".planning/STATE.md\n" true true true false)
route(count-mismatch 2 ".planning/STATE.md\n" false false true false)
route(mixed-pbp-docs 2 "host/disc_pbp.c\ndocs/maintaining.md\n" true false true false)
route(release-metadata 3 ".release-please-manifest.json\nCHANGELOG.md\nversion.txt\n" false false true false release-metadata)
route(release-metadata-partial 2 ".release-please-manifest.json\nCHANGELOG.md\n" true false true false release-metadata)
route(release-metadata-duplicate 3 ".release-please-manifest.json\nCHANGELOG.md\nCHANGELOG.md\n" true false true false release-metadata)
route(release-metadata-extra 4 ".release-please-manifest.json\nCHANGELOG.md\nversion.txt\n.planning/STATE.md\n" true false true false release-metadata)
route(release-metadata-release-input 4 ".release-please-manifest.json\nCHANGELOG.md\nversion.txt\n.github/workflows/release.yml\n" true true true false release-metadata)
foreach(path IN ITEMS
  .github/workflows/nightly.yml CMakePresets.json CMakeLists.txt cmake/CHDFixtures.cmake
  cmake/CDReferences.cmake cmake/DiscFixtures.cmake cmake/MDECReferences.cmake cmake/SPUReferences.cmake
  host/disc_image.c host/disc_image.h pins.txt runner/bench.c runner/bench.h runner/clock.c runner/clock.h
  runner/rss.c runner/rss.h tests/cdrom/reference.c tests/disc/chd.c tests/fixtures/manifest.txt tests/fuzz/chd.c
  tests/mdec/reference.c tests/nightly/inventory.txt tests/nightly/tests.cmake tests/spu/reference.c)
  route("${path}" 1 "${path}\n" true false true false)
endforeach()
route(rename-both-sides 3 "__renamed__\ntests/disc/pbp.c\nhost/disc_pbp.h\n" true false true false)
