# The GPU tests issue register writes through the same bus path as the CPU and
# inspect the instance's native VRAM. One executable runs the named case.
if(CMAKE_SCRIPT_MODE_FILE)
  string(REPLACE "|" ";" EMULATOR "${EMULATOR}")
  function(expect_runner code regex)
    execute_process(COMMAND ${EMULATOR} ${RUN} ${ARGN} RESULT_VARIABLE rc
      OUTPUT_VARIABLE out ERROR_VARIABLE err)
    string(APPEND out "${err}")
    if(NOT rc STREQUAL "${code}" OR NOT out MATCHES "${regex}")
      message(FATAL_ERROR "runner ${ARGN}: expected exit ${code} and output matching ${regex}; got ${rc}\n${out}")
    endif()
  endfunction()
  if(CASE STREQUAL "cli")
    expect_runner(0 "^vram sha256 67222605a58edbac567cba036618e52bb4a3eb933c32006c891d06adbb035041\n$"
      --frames 1 --hash-vram)
    expect_runner(2 "usage" --frames 1 --hash-vram --hash-vram)
    expect_runner(2 "usage" --frames 1 --hash-vram --probe unused.exe)
  elseif(CASE STREQUAL "controls")
    execute_process(COMMAND ${EMULATOR} ${UNIT} controls RESULT_VARIABLE rc
      OUTPUT_VARIABLE out ERROR_VARIABLE err)
    string(APPEND out "${err}")
    if(NOT rc STREQUAL "0")
      message(FATAL_ERROR "GPU reference controls exited ${rc}\n${out}")
    endif()
  else()
    message(FATAL_ERROR "unknown GPU reference case '${CASE}'")
  endif()
  return()
endif()

add_executable(schwaystation-test-gpu ${CMAKE_CURRENT_LIST_DIR}/gpu_tests.c)
target_include_directories(schwaystation-test-gpu PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-gpu PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-gpu PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-gpu)

add_executable(schwaystation-test-gpu-transfer ${CMAKE_CURRENT_LIST_DIR}/transfers.c)
target_include_directories(schwaystation-test-gpu-transfer PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-gpu-transfer PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-gpu-transfer PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-gpu-transfer)

add_executable(schwaystation-test-gpu-pixels ${CMAKE_CURRENT_LIST_DIR}/pixels.c)
target_include_directories(schwaystation-test-gpu-pixels PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-gpu-pixels PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-gpu-pixels PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-gpu-pixels)

add_executable(schwaystation-test-gpu-triangles ${CMAKE_CURRENT_LIST_DIR}/triangles.c)
target_include_directories(schwaystation-test-gpu-triangles PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-gpu-triangles PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-gpu-triangles PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-gpu-triangles)

add_executable(schwaystation-test-gpu-interpolation ${CMAKE_CURRENT_LIST_DIR}/interpolation.c)
target_include_directories(schwaystation-test-gpu-interpolation PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-gpu-interpolation PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-gpu-interpolation PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-gpu-interpolation)

add_executable(schwaystation-test-gpu-spans ${CMAKE_CURRENT_LIST_DIR}/spans.c)
target_include_directories(schwaystation-test-gpu-spans PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-gpu-spans PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-gpu-spans PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-gpu-spans)

add_executable(schwaystation-test-gpu-beam ${CMAKE_CURRENT_LIST_DIR}/beam.c)
target_include_directories(schwaystation-test-gpu-beam PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-gpu-beam PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-gpu-beam PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-gpu-beam)

add_executable(schwaystation-test-gpu-display ${CMAKE_CURRENT_LIST_DIR}/display.c)
target_include_directories(schwaystation-test-gpu-display PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-gpu-display PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-gpu-display PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-gpu-display)

include(${PROJECT_SOURCE_DIR}/cmake/GPUReferences.cmake)
schway_add_gpu_reference_target()
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Fault prevented: a bad PNG adapter input or a wrong/mutated fetched reference
# being accepted before it can reach the pinned capture comparison.
schway_add_test(NAME gpu.reference.fetch_controls
  COMMAND ${CMAKE_COMMAND}
    "-DSCHWAY_GPU_REFERENCE_CONTROL_DIR=${PROJECT_BINARY_DIR}/tests/gpu/reference-fetch-controls"
    -P ${PROJECT_SOURCE_DIR}/cmake/GPUReferences.cmake
  LABELS gpu portable TIMEOUT 30)

schway_add_test(NAME gpu.reference.png_controls
  COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/gpu/reference_png.py" self-test
  LABELS gpu portable TIMEOUT 30)

# Fault prevented: GP1 mode dividers, display-window wrap, field height,
# 24-bit packing, display-off output or malformed ranges drifting from scanout.
foreach(case IN ITEMS width_modes native15_wrap native24_wrap interlace_and_off malformed_windows)
  schway_add_test(NAME gpu.display.${case}
    COMMAND schwaystation-test-gpu-display ${case}
    LABELS gpu portable TIMEOUT 10)
endforeach()

# Fault prevented: Timer0 dot-target IRQs drifting to a line boundary or across split runs.
schway_add_test(NAME gpu.beam.midline_dot_irq
  COMMAND schwaystation-test-gpu-beam midline_dot_irq
  LABELS gpu portable TIMEOUT 10)

# Fault prevented: Y1/Y2 equality or range cannot alter the fixed field wrap.
schway_add_test(NAME gpu.beam.field_edges
  COMMAND schwaystation-test-gpu-beam field_edges
  LABELS gpu portable TIMEOUT 10)

# Fault prevented: GPUSTAT field phase, revision visibility, read purity, and reset/state retention.
schway_add_test(NAME gpu.beam.status_fields
  COMMAND schwaystation-test-gpu-beam status_fields
  LABELS gpu portable TIMEOUT 10)

# Fault prevented: status range edges, mode retention, malformed phase state, and split-run drift.
schway_add_test(NAME gpu.beam.status_controls
  COMMAND schwaystation-test-gpu-beam status_controls
  LABELS gpu portable TIMEOUT 10)

# Fault prevented: loaded run_frame uses the GPU field wrap when Y2 suppresses VBlank.
schway_add_test(NAME gpu.beam.public_field_wrap
  COMMAND schwaystation-test-gpu-beam public_field_wrap
  LABELS gpu portable TIMEOUT 10)

# Fault prevented: repeated GP1(00h) resets cannot restart the public call deadline.
schway_add_test(NAME gpu.beam.reset_call_bound
  COMMAND schwaystation-test-gpu-beam reset_call_bound
  LABELS gpu portable TIMEOUT 10)

# Fault prevented: tied dot, horizontal blank and DMA deadlines are dispatched together.
schway_add_test(NAME gpu.beam.equal_timer_beam_dma
  COMMAND schwaystation-test-gpu-beam equal_timer_beam_dma
  LABELS gpu portable TIMEOUT 10)

# Fault prevented: malformed serialized beam phase or position mutates a live instance.
schway_add_test(NAME gpu.beam.reject_malformed_state
  COMMAND schwaystation-test-gpu-beam reject_malformed_beam_state
  LABELS gpu portable TIMEOUT 10)

# Fault prevented: lines or Gouraud polylines lose endpoints, interpolated colors, span rejection,
# terminator placement, or bounded streaming state.
schway_add_test(NAME gpu.span.polyline_packet
  COMMAND schwaystation-test-gpu-spans polyline_packet
  LABELS gpu portable TIMEOUT 10)

# Fault prevented: rectangle size modes, zero/max fields, clipping, E5, or dither change coverage.
schway_add_test(NAME gpu.span.rectangle_sizes
  COMMAND schwaystation-test-gpu-spans rectangle_sizes
  LABELS gpu portable TIMEOUT 10)

# Fault prevented: rectangle texture UV/page wrapping, raw/modulated pixels, or texture ABE changes.
schway_add_test(NAME gpu.span.rectangle_texture
  COMMAND schwaystation-test-gpu-spans rectangle_texture
  LABELS gpu portable TIMEOUT 10)

# Fault prevented: v0 profile accidentally applies v2-only rectangle texture flips.
schway_add_test(NAME gpu.span.rectangle_profile
  COMMAND schwaystation-test-gpu-spans rectangle_profile
  LABELS gpu portable TIMEOUT 10)

# Fault prevented: a GP0 fill that is lost between the bus and VRAM, or uses
# byte rather than halfword coordinates, sizing, or color conversion.
schway_add_test(NAME gpu.decoder.fill_mmio
  COMMAND schwaystation-test-gpu fill_mmio
  LABELS gpu portable TIMEOUT 10)

# Fault prevented: an incomplete packet that runs after GP1(01h) resets the
# command buffer.
schway_add_test(NAME gpu.decoder.partial_packet_reset
  COMMAND schwaystation-test-gpu partial_packet_reset
  LABELS gpu portable TIMEOUT 10)

# Fault prevented: GPUSTAT, draw-environment query latches, IRQ1 and its
# acknowledgement drifting from their register images.
schway_add_test(NAME gpu.decoder.register_images
  COMMAND schwaystation-test-gpu register_images
  LABELS gpu portable TIMEOUT 10)

# Fault prevented: a saved partial fill packet that loses its parser state or
# a corrupt GPU field that is applied before validation finishes.
schway_add_test(NAME gpu.state.partial_fill
  COMMAND schwaystation-test-gpu state_partial_fill
  LABELS gpu portable TIMEOUT 10)

# Fault prevented: cached texture/CLUT data or tags that are lost on save/load
# or malformed cache metadata that is applied before state validation.
schway_add_test(NAME gpu.state.cache_roundtrip
  COMMAND schwaystation-test-gpu state_cache_roundtrip
  LABELS gpu portable TIMEOUT 10)

# Fault prevented: textured rectangles fail to fetch and write one native RGB555 pixel.
schway_add_test(NAME gpu.pixel.textured_rectangle_mmio
  COMMAND schwaystation-test-gpu-pixels textured_rectangle_mmio
  LABELS gpu portable TIMEOUT 10)

# Fault prevented: real flat triangle and quad packets lose coverage, winding invariance, or single blend ownership.
schway_add_test(NAME gpu.triangle.flat_packet
  COMMAND schwaystation-test-gpu-triangles flat_packet
  LABELS gpu portable TIMEOUT 10)

# Fault prevented: flat and Gouraud textured packets lose their independently tabulated affine
# color/UV values or fail to update the texture page from vertex two.
# Fault prevented: polygon flags, signed/edge UV gradients, quad interpolation, or split-state
# replay changes visible VRAM or escapes the bounded fixed-point coordinate range.
foreach(case IN ITEMS gouraud_textured texture_page_clut clip_interpolation flag_matrix
                      equal_gouraud uv_negative_wrap quad_attributes split_state hostile_slope
                      channel_midpoint vertex_plane)
  schway_add_test(NAME gpu.triangle.${case}
    COMMAND schwaystation-test-gpu-interpolation ${case}
    LABELS gpu portable TIMEOUT 10)
endforeach()

# Fault prevented: the polygon size limit rejects exactly the documented horizontal spans.
schway_add_test(NAME gpu.triangle.horizontal_spans
  COMMAND schwaystation-test-gpu-triangles horizontal_spans
  LABELS gpu portable TIMEOUT 10)

# Fault prevented: the polygon size limit rejects exactly the documented vertical spans.
schway_add_test(NAME gpu.triangle.vertical_spans
  COMMAND schwaystation-test-gpu-triangles vertical_spans
  LABELS gpu portable TIMEOUT 10)

# Fault prevented: inclusive and inverted E3/E4 windows clip polygon coverage correctly.
schway_add_test(NAME gpu.triangle.clip_windows
  COMMAND schwaystation-test-gpu-triangles clip_windows
  LABELS gpu portable TIMEOUT 10)

# Fault prevented: signed E5 offsets and signed 11-bit vertices stay within bounded VRAM clipping.
schway_add_test(NAME gpu.triangle.extreme_coordinates
  COMMAND schwaystation-test-gpu-triangles extreme_coordinates
  LABELS gpu portable TIMEOUT 10)

# Fault prevented: fully offscreen polygons do not wrap into visible VRAM.
schway_add_test(NAME gpu.triangle.offscreen_rejection
  COMMAND schwaystation-test-gpu-triangles offscreen_rejection
  LABELS gpu portable TIMEOUT 10)

# Fault prevented: polygon writes and draw state remain owned by the selected instance.
schway_add_test(NAME gpu.triangle.instances
  COMMAND schwaystation-test-gpu-triangles instances
  LABELS gpu portable TIMEOUT 10)

# Fault prevented: pixel arithmetic, dither eligibility, raw texture, and write masks drift.
foreach(case IN ITEMS arithmetic palette_wrap)
  schway_add_test(NAME gpu.pixel.${case}
    COMMAND schwaystation-test-gpu-pixels ${case}
    LABELS gpu portable TIMEOUT 10)
endforeach()

# Fault prevented: transfer dimensions, endian order, wrapping, padding, reset,
# partial-state resume, and malformed transfer cursors drifting from the bus.
foreach(case IN ITEMS cache_contract copy_masks copy_order copy_wrap roundtrip dimensions
                      partial_state reset_mid_packet invalid_cursor)
  schway_add_test(NAME gpu.transfer.${case}
    COMMAND schwaystation-test-gpu-transfer ${case}
    LABELS gpu portable TIMEOUT 10)
endforeach()

# Keep the original test-card pixel and native-video contracts selected.
foreach(case IN ITEMS testcard.pixels video.tuple)
  schway_add_test(NAME gpu.${case}
    COMMAND schwaystation-test-core ${case}
    LABELS gpu portable TIMEOUT 10)
endforeach()

# Fault prevented: GPU ready bits and DMA request state staying asserted during
# primitive work, or failing to follow the selected GP1 DMA direction.
schway_add_test(NAME gpu.dma.ready_budget
  COMMAND schwaystation-test-gpu dma_ready_budget
  LABELS gpu portable TIMEOUT 10)

# Fault prevented: a canonical whole-VRAM hash differs from the independent
# little-endian RGB555 frame-1 known answer.
string(REPLACE ";" "|" gpu_reference_emulator "${CMAKE_CROSSCOMPILING_EMULATOR}")
schway_add_test(NAME gpu.reference.cli
  COMMAND ${CMAKE_COMMAND} -DCASE=cli -DRUN=$<TARGET_FILE:schwaystation-run>
    "-DEMULATOR=${gpu_reference_emulator}" -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS gpu portable TIMEOUT 30)

# Fault prevented: PNG channel quantization or any canonical whole-VRAM pixel changes.
schway_add_test(NAME gpu.reference.quantization
  COMMAND schwaystation-test-gpu-reference quantization
  LABELS gpu portable TIMEOUT 30)

# Fault prevented: reference color order, mask handling, dimensions, or bad pixels drifting.
schway_add_test(NAME gpu.reference.controls
  COMMAND ${CMAKE_COMMAND} -DCASE=controls -DUNIT=$<TARGET_FILE:schwaystation-test-gpu-reference>
    "-DEMULATOR=${gpu_reference_emulator}" -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS gpu portable TIMEOUT 30)
