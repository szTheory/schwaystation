# The MDEC first caller: command/status FIFO behavior and bounded block decode.
add_executable(schwaystation-test-mdec ${CMAKE_CURRENT_LIST_DIR}/model.c)
target_include_directories(schwaystation-test-mdec PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-mdec PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-mdec PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-mdec)

# Fault prevented: reset status and the empty/full/DREQ status bits drift from
# the documented MDEC port behavior or the bus still routes it as unbuilt.
schway_add_test(NAME mdec.status
  COMMAND schwaystation-test-mdec status
  LABELS mdec portable TIMEOUT 10)

# Fault prevented: monochrome 4/8-bit blocks fail to wait for their 1,000-cycle
# availability deadline or fail to move through DMA0 and DMA1 to RAM.
schway_add_test(NAME mdec.block
  COMMAND schwaystation-test-mdec block
  LABELS mdec portable TIMEOUT 10)

# Fault prevented: malformed RLE input can overrun the coefficient block or a
# reset leaves a scheduled block/DMA request able to publish stale output.
schway_add_test(NAME mdec.bounds
  COMMAND schwaystation-test-mdec bounds
  LABELS mdec portable TIMEOUT 10)

# Fault prevented: an EOB at an odd halfword boundary discards the next block
# or a save/load loses the retained DC before its next scheduled decode.
schway_add_test(NAME mdec.stream
  COMMAND schwaystation-test-mdec stream
  LABELS mdec portable TIMEOUT 10)

# Fault prevented: a completed block is lost when DMA1 must drain a nearly
# full output FIFO; the waiting block and no-deadline state also round-trip.
schway_add_test(NAME mdec.fifo
  COMMAND schwaystation-test-mdec fifo
  LABELS mdec portable TIMEOUT 10)
