# The full public vectors, replayed through the same harness as the committed
# sample. The nightly preset registers only this file.
#
# Fault prevented: a nightly lane that passes, or skips, because the pinned
# files were never fetched. Configure fails naming the first pinned file that
# SCHWAYSTATION_SST_DIR lacks; fetch them with the commands in pins.txt.
set(SCHWAYSTATION_SST_DIR "" CACHE PATH "Directory of the fetched pinned vector files, NAME.json.bin each")
option(SCHWAYSTATION_GPU_REFERENCE_ONLY "Configure only GPU references without fetching CPU vectors" OFF)
if(SCHWAYSTATION_SST_DIR STREQUAL "")
  if(NOT SCHWAYSTATION_GPU_REFERENCE_ONLY)
    message(FATAL_ERROR "nightly: set SCHWAYSTATION_SST_DIR to the directory of the fetched pinned blobs")
  endif()
  set(nightly_sst_dir "${PROJECT_BINARY_DIR}/sst")
else()
  get_filename_component(nightly_sst_dir "${SCHWAYSTATION_SST_DIR}" ABSOLUTE BASE_DIR "${PROJECT_SOURCE_DIR}")
endif()
if(NOT SCHWAYSTATION_GPU_REFERENCE_ONLY)
  file(STRINGS ${PROJECT_SOURCE_DIR}/pins.txt nightly_rows REGEX "^r3000-")
  if(NOT nightly_rows)
    message(FATAL_ERROR "nightly: pins.txt has no r3000- rows")
  endif()
  foreach(row IN LISTS nightly_rows)
    if(NOT row MATCHES "^r3000-([A-Za-z]+) ")
      message(FATAL_ERROR "nightly: pins.txt row is not 'r3000-NAME pin sha1 size': ${row}")
    endif()
    if(NOT EXISTS "${nightly_sst_dir}/${CMAKE_MATCH_1}.json.bin")
      message(FATAL_ERROR "nightly: ${nightly_sst_dir}/${CMAKE_MATCH_1}.json.bin is missing; fetch the pinned blobs first")
    endif()
  endforeach()
endif()

include(${PROJECT_SOURCE_DIR}/cmake/GPUReferences.cmake)
schway_add_gpu_reference_target()
schway_prepare_gpu_reference_capture(clipping clipping_exe clipping_png clipping_golden)

# The log guests' source-defined MMIO predicates are judged against their real ordered hardware pass logs.
foreach(case IN ITEMS gp0-e1 gpustat mask-bit)
  schway_prepare_gpu_reference_log(${case} case_log)
  schway_add_test(NAME gpu.reference.${case}
    COMMAND schwaystation-test-gpu-reference log ${case} ${case_log}
    LABELS gpu nightly portable TIMEOUT 30)
endforeach()

# Fault prevented: the pinned clipping executable's complete VRAM differs from
# its independently captured hardware reference at any RGB555 word.
schway_add_test(NAME gpu.reference.clipping
  COMMAND schwaystation-test-gpu-reference capture clipping ${clipping_exe} ${clipping_golden} 1
    ${PROJECT_SOURCE_DIR}/tests/gpu/allow.txt
  LABELS gpu nightly portable TIMEOUT 120)

# Every guest draws before its first VSync loop. Bounds are fixed from the
# pinned source: rectangles settle their 5-bit repeated-blend state within 31
# repeats; UV interpolation issues 768 polygons; overflow uploads two textures;
# CLUT caching writes 3,712 halfwords; overlapping copies write 27,657 pixels
# and copy 147 blocks. The small primitive guests use one call, the bulk cases
# use two, rectangles allow 32 total calls for initialization, and overlap uses
# its separately verified 24-call source-work bound.
foreach(case IN LISTS SCHWAY_GPU_REFERENCE_CASES)
  schway_prepare_gpu_reference_capture(${case} case_exe case_png case_golden)
  set(case_advances 1)
  if(case STREQUAL "rectangles" OR case STREQUAL "uv-interpolation" OR
     case STREQUAL "texture-overflow" OR case STREQUAL "clut-cache")
    set(case_advances 2)
  endif()
  if(case STREQUAL "rectangles")
    set(case_advances 32)
  elseif(case STREQUAL "vram-to-vram-overlap")
    set(case_advances 24)
  endif()

  # Fault prevented: a pinned GPU guest's full VRAM differs from its hardware PNG.
  schway_add_test(NAME gpu.reference.${case}
    COMMAND schwaystation-test-gpu-reference capture ${case} ${case_exe} ${case_golden}
      ${case_advances} ${PROJECT_SOURCE_DIR}/tests/gpu/allow.txt
    LABELS gpu nightly portable TIMEOUT 120)
endforeach()

include(${PROJECT_SOURCE_DIR}/cmake/SPUReferences.cmake)
schway_prepare_spu_reference_assets(
  spu_reference_dir spu_reference_exe spu_reference_log
  spu_reference_exe_sha256 spu_reference_log_sha256)
schway_add_spu_reference_target("${spu_reference_exe_sha256}" "${spu_reference_log_sha256}")

# Fault prevented: nightly omits any of the pinned memory-transfer guest's
# seven hardware cases or skips the bounded end-to-end transfer check.
schway_add_test(NAME spu.reference.memory_transfer
  COMMAND schwaystation-test-spu-reference memory-transfer --full --assets "${spu_reference_dir}"
  LABELS audio nightly portable TIMEOUT 120)

add_executable(schwaystation-test-vectors ${PROJECT_SOURCE_DIR}/tests/vectors/vectors.c)
target_include_directories(schwaystation-test-vectors PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-vectors PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-vectors PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-vectors)

# Fault prevented: an instruction that fails a case outside the committed
# sample. All 55,000 run, and the first records of each file must equal the
# sample, so the sample cannot drift from the files the pins name.
set(vectors_dir ${PROJECT_SOURCE_DIR}/tests/vectors/r3000)
schway_add_test(NAME vectors.r3000.full
  COMMAND schwaystation-test-vectors --upstream ${nightly_sst_dir}
    --prefix-of ${vectors_dir}/sst-sample.bin --expect 55000 --allow ${vectors_dir}/allow.txt
  LABELS portable vectors TIMEOUT 120)
