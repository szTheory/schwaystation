# The full public vectors, replayed through the same harness as the committed
# sample. The nightly preset registers only this file.
#
# Fault prevented: a nightly lane that passes, or skips, because the pinned
# files were never fetched. Configure fails naming the first pinned file that
# SCHWAYSTATION_SST_DIR lacks; fetch them with the commands in pins.txt.
set(SCHWAYSTATION_SST_DIR "" CACHE PATH "Directory of the fetched pinned vector files, NAME.json.bin each")
option(SCHWAYSTATION_GPU_REFERENCE_ONLY "Configure only GPU references without fetching CPU vectors" OFF)
set(SCHWAYSTATION_MEDIA_REFERENCE_ONLY "" CACHE STRING
  "Configure only one media reference lane (cdrom or mdec)")
set_property(CACHE SCHWAYSTATION_MEDIA_REFERENCE_ONLY PROPERTY STRINGS "" cdrom mdec)
if(NOT SCHWAYSTATION_MEDIA_REFERENCE_ONLY STREQUAL "" AND
   NOT SCHWAYSTATION_MEDIA_REFERENCE_ONLY STREQUAL "cdrom" AND
   NOT SCHWAYSTATION_MEDIA_REFERENCE_ONLY STREQUAL "mdec")
  message(FATAL_ERROR "nightly: SCHWAYSTATION_MEDIA_REFERENCE_ONLY must be empty, cdrom or mdec")
endif()
set(cdrom_reference_only FALSE)
if(SCHWAYSTATION_MEDIA_REFERENCE_ONLY STREQUAL "cdrom")
  set(cdrom_reference_only TRUE)
endif()
set(mdec_reference_only FALSE)
if(SCHWAYSTATION_MEDIA_REFERENCE_ONLY STREQUAL "mdec")
  set(mdec_reference_only TRUE)
endif()
if(SCHWAYSTATION_SST_DIR STREQUAL "")
  if(NOT SCHWAYSTATION_GPU_REFERENCE_ONLY AND NOT cdrom_reference_only AND NOT mdec_reference_only)
    message(FATAL_ERROR "nightly: set SCHWAYSTATION_SST_DIR to the directory of the fetched pinned blobs")
  endif()
  set(nightly_sst_dir "${PROJECT_BINARY_DIR}/sst")
else()
  get_filename_component(nightly_sst_dir "${SCHWAYSTATION_SST_DIR}" ABSOLUTE BASE_DIR "${PROJECT_SOURCE_DIR}")
endif()
if(NOT SCHWAYSTATION_GPU_REFERENCE_ONLY AND NOT cdrom_reference_only AND NOT mdec_reference_only)
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

if(cdrom_reference_only OR mdec_reference_only)
  # Keep the full nightly inventory registered while a focused media tree
  # configures only the selected assets and target. Unselected tests still fail
  # normally if run without their missing executable/assets.
  foreach(case IN ITEMS gp0-e1 gpustat mask-bit)
    schway_add_test(NAME gpu.reference.${case}
      COMMAND schwaystation-test-gpu-reference log ${case} ${PROJECT_BINARY_DIR}/gpu-reference/${case}/psx.log
      LABELS gpu nightly portable TIMEOUT 30)
  endforeach()
  schway_add_test(NAME gpu.reference.clipping
    COMMAND schwaystation-test-gpu-reference capture clipping
      ${PROJECT_BINARY_DIR}/gpu-reference/gpu/clipping/clipping.exe
      ${PROJECT_BINARY_DIR}/gpu-reference/clipping-vram.rgb555le 1
      ${PROJECT_SOURCE_DIR}/tests/gpu/allow.txt
    LABELS gpu nightly portable TIMEOUT 120)
  foreach(case IN ITEMS clut-cache lines quad rectangles texture-overflow transparency triangle
                        uv-interpolation vram-to-vram-overlap)
    schway_add_test(NAME gpu.reference.${case}
      COMMAND schwaystation-test-gpu-reference capture ${case}
        ${PROJECT_BINARY_DIR}/gpu-reference/gpu/${case}/${case}.exe
        ${PROJECT_BINARY_DIR}/gpu-reference/${case}-vram.rgb555le 1
        ${PROJECT_SOURCE_DIR}/tests/gpu/allow.txt
      LABELS gpu nightly portable TIMEOUT 120)
  endforeach()
else()
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
endif()

if(cdrom_reference_only OR mdec_reference_only)
  schway_add_test(NAME spu.reference.memory_transfer
    COMMAND schwaystation-test-spu-reference memory-transfer --full --assets
      "${PROJECT_BINARY_DIR}/spu-reference"
    LABELS audio nightly portable TIMEOUT 120)
else()
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
endif()

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

if(cdrom_reference_only)
  include(${PROJECT_SOURCE_DIR}/cmake/CDReferences.cmake)
  schway_prepare_cd_reference_assets(
    cd_reference_dir cd_timing_exe cd_timing_log cd_getloc_exe cd_getloc_log
    cd_timing_exe_sha cd_getloc_exe_sha cd_timing_log_sha cd_getloc_log_sha)
  schway_add_cd_reference_target(
    "${cd_timing_exe_sha}" "${cd_getloc_exe_sha}" "${cd_timing_log_sha}" "${cd_getloc_log_sha}")
  schway_register_cd_reference_tests(
    "${cd_timing_exe}" "${cd_timing_log}" "${cd_getloc_exe}" "${cd_getloc_log}")
elseif(mdec_reference_only)
  foreach(case IN ITEMS timing getloc)
    schway_add_test(NAME cdrom.reference.${case}
      COMMAND schwaystation-test-cdrom-reference ${case}
        "${PROJECT_BINARY_DIR}/cdrom-reference/${case}.exe"
        "${PROJECT_BINARY_DIR}/cdrom-reference/${case}.log"
      LABELS disc nightly portable TIMEOUT 120)
  endforeach()
  schway_add_test(NAME cdrom.reference.controls
    COMMAND schwaystation-test-cdrom-reference controls
      "${PROJECT_BINARY_DIR}/cdrom-reference/getloc.log"
    LABELS disc nightly portable TIMEOUT 30)
else()
  include(${PROJECT_SOURCE_DIR}/cmake/CDReferences.cmake)
  schway_prepare_cd_reference_assets(
    cd_reference_dir cd_timing_exe cd_timing_log cd_getloc_exe cd_getloc_log
    cd_timing_exe_sha cd_getloc_exe_sha cd_timing_log_sha cd_getloc_log_sha)
  schway_add_cd_reference_target(
    "${cd_timing_exe_sha}" "${cd_getloc_exe_sha}" "${cd_timing_log_sha}" "${cd_getloc_log_sha}")
  schway_register_cd_reference_tests(
    "${cd_timing_exe}" "${cd_timing_log}" "${cd_getloc_exe}" "${cd_getloc_log}")
endif()

if(cdrom_reference_only)
  # Keep the MDEC names/labels in the full nightly inventory without fetching
  # or extracting an unselected MDEC corpus in the CD-ROM-only configuration.
  set(mdec_placeholder "${PROJECT_BINARY_DIR}/mdec-reference")
  schway_add_test(NAME mdec.reference.input_preflight
    COMMAND schwaystation-test-mdec-reference input-preflight "${mdec_placeholder}"
    LABELS mdec nightly portable TIMEOUT 10)
  set_tests_properties(mdec.reference.input_preflight PROPERTIES
    FIXTURES_SETUP mdec_reference_input_preflight)
  schway_add_test(NAME mdec.reference.4bit
    COMMAND schwaystation-test-mdec-reference 4bit
      "${mdec_placeholder}/mdec/4bit/4bit.exe" "${mdec_placeholder}/mdec/4bit/psx.log"
    LABELS mdec nightly portable TIMEOUT 120)
  set_tests_properties(mdec.reference.4bit PROPERTIES
    FIXTURES_REQUIRED mdec_reference_input_preflight)
  schway_add_test(NAME mdec.reference.8bit_regression
    COMMAND schwaystation-test-mdec-reference 8bit-regression
      "${mdec_placeholder}/mdec/8bit/8bit.exe" "${mdec_placeholder}/mdec/8bit/psx.log"
    LABELS mdec nightly portable TIMEOUT 120)
  set_tests_properties(mdec.reference.8bit_regression PROPERTIES
    FIXTURES_REQUIRED mdec_reference_input_preflight)
  schway_add_test(NAME mdec.reference.status_budget
    COMMAND schwaystation-test-mdec-reference status-budget
      "${mdec_placeholder}/mdec/step-by-step-log/step-by-step-log.exe"
      "${mdec_placeholder}/mdec/step-by-step-log/psx.log"
    LABELS mdec nightly portable TIMEOUT 10)
  schway_add_test(NAME mdec.reference.coverage_controls
    COMMAND schwaystation-test-mdec-reference coverage-controls
    LABELS mdec nightly portable TIMEOUT 10)
  schway_add_test(NAME mdec.reference.controls
    COMMAND schwaystation-test-mdec-reference controls
      "${mdec_placeholder}/mdec/4bit/psx.log"
      "${mdec_placeholder}/mdec/8bit/psx.log"
      "${mdec_placeholder}/mdec/step-by-step-log/psx.log"
    LABELS mdec nightly portable TIMEOUT 30)
  schway_add_test(NAME mdec.reference.color_assets
    COMMAND schwaystation-test-mdec-reference color-assets
      "${mdec_placeholder}/color/mdec/frame/frame-15bit.exe"
      "${mdec_placeholder}/color/mdec/frame/frame-24bit.exe"
      "${mdec_placeholder}/color/mdec/movie/movie-15bit.exe"
      "${mdec_placeholder}/color/mdec/movie/movie-24bit.exe"
      "${mdec_placeholder}/color/mdec/frame/vram-15bit.png"
      "${mdec_placeholder}/color/mdec/frame/vram-24bit.png"
      "${mdec_placeholder}/color/mdec/movie/vram-15bit.png"
      "${mdec_placeholder}/color/mdec/movie/vram-24bit.png"
    LABELS mdec nightly portable TIMEOUT 20)
  set_tests_properties(mdec.reference.color_assets PROPERTIES
    FIXTURES_SETUP mdec_color_reference_assets)
  schway_add_test(NAME mdec.reference.color_controls
    COMMAND schwaystation-test-mdec-reference color-controls
    LABELS mdec nightly portable TIMEOUT 10)
  foreach(case IN ITEMS frame15 frame24 movie15 movie24)
    schway_add_test(NAME mdec.reference.coverage_${case}
      COMMAND schwaystation-test-mdec-reference color-coverage ${case}
        "${mdec_placeholder}/color/mdec/${case}/${case}.exe"
      LABELS mdec nightly portable TIMEOUT 10)
    set_tests_properties(mdec.reference.coverage_${case} PROPERTIES
      FIXTURES_REQUIRED "mdec_color_reference_assets;mdec_reference_input_preflight")
  endforeach()
else()
  include(${PROJECT_SOURCE_DIR}/cmake/MDECReferences.cmake)
  schway_prepare_mdec_reference_assets(
    mdec_reference_dir mdec_4bit_exe mdec_4bit_log mdec_8bit_exe mdec_8bit_log
    mdec_status_exe mdec_status_log mdec_4bit_exe_sha mdec_8bit_exe_sha
    mdec_status_exe_sha mdec_4bit_log_sha mdec_8bit_log_sha mdec_status_log_sha)
  schway_add_mdec_reference_target(
    "${mdec_4bit_exe_sha}" "${mdec_8bit_exe_sha}" "${mdec_status_exe_sha}"
    "${mdec_4bit_log_sha}" "${mdec_8bit_log_sha}" "${mdec_status_log_sha}")
  schway_register_mdec_reference_tests(
    "${mdec_4bit_exe}" "${mdec_4bit_log}" "${mdec_8bit_exe}" "${mdec_8bit_log}"
    "${mdec_status_exe}" "${mdec_status_log}")
  schway_prepare_mdec_color_reference_assets(
    mdec_color_reference_dir mdec_color_reference_exes mdec_color_reference_pngs
    mdec_color_reference_exe_sha256s mdec_color_reference_png_sha256s
    mdec_color_reference_scene_sha256s)
  schway_add_mdec_color_reference_support(
    "${mdec_color_reference_exe_sha256s}" "${mdec_color_reference_png_sha256s}"
    "${mdec_color_reference_scene_sha256s}")
  schway_register_mdec_color_reference_tests(
    "${mdec_color_reference_dir}" "${mdec_color_reference_exes}"
    "${mdec_color_reference_pngs}")
endif()
