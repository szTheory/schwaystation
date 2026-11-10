# The GPU tests issue register writes through the same bus path as the CPU and
# inspect the instance's native VRAM. One executable runs the named case.
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

# Fault prevented: lines or Gouraud polylines lose endpoints, interpolated colors, span rejection,
# terminator placement, or bounded streaming state.
schway_add_test(NAME gpu.span.polyline_packet
  COMMAND schwaystation-test-gpu-spans polyline_packet
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
                      channel_midpoint)
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
