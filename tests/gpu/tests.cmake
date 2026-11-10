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

# Fault prevented: transfer dimensions, endian order, wrapping, padding, reset,
# partial-state resume, and malformed transfer cursors drifting from the bus.
foreach(case IN ITEMS copy_masks copy_order copy_wrap roundtrip dimensions partial_state
                      reset_mid_packet invalid_cursor)
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
