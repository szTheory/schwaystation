# The seven DMA channels: registers, units, costs and the CPU stall.

# The DMA registers, arbitration, the unit engine, OTC and the CPU hold. One
# executable runs the case named on its command line; it sees src/ to drive the
# registers through the bus and read the engine's state.
add_executable(schwaystation-test-dma ${CMAKE_CURRENT_LIST_DIR}/dma_tests.c)
target_include_directories(schwaystation-test-dma PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-dma PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-dma PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-dma)

# Fault prevented: an ordering table that is wrong, an OTC that reads back or
# completes wrongly, finishes at another tick than 4,096 words on the 17 per 16
# row give, or forgets the flag or the interrupt.
schway_add_test(NAME dma.otc.clear
  COMMAND schwaystation-test-dma otc_clear
  LABELS dma portable TIMEOUT 10)
