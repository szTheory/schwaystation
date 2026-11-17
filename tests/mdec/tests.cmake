# The MDEC first caller: command/status FIFO behavior and bounded block decode.
add_executable(schwaystation-test-mdec ${CMAKE_CURRENT_LIST_DIR}/model.c)
target_include_directories(schwaystation-test-mdec PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-mdec PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-mdec PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-mdec)

# Fault prevented: reset status, empty/full/DREQ bits, or the completed-command
# parameter counter drift from documented MDEC behavior, or the bus still routes
# the device as unbuilt.
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

# Fault prevented: color components decode in Cr,Cb,Y1-Y4 order, signed and
# unsigned channel packing changes, or the 15-bit bit15 flag is lost.
schway_add_test(NAME mdec.color
  COMMAND schwaystation-test-mdec color
  LABELS mdec portable TIMEOUT 10)

# Fault prevented: decoded color output fails to cross DMA0, DMA1, DMA2 and
# the existing native scanout in RGB555 and RGB24 modes.
schway_add_test(NAME mdec.display
  COMMAND schwaystation-test-mdec display
  LABELS mdec portable TIMEOUT 10)

# Fault prevented: nonzero mono pixels pack incorrectly in raw output or DMA1
# writes a different word order or ending MADR in RAM.
schway_add_test(NAME mdec.words
  COMMAND schwaystation-test-mdec words
  LABELS mdec portable TIMEOUT 10)

# Fault prevented: an EOB at an odd halfword boundary discards the next block
# or a save/load loses the retained DC before its next scheduled decode.
schway_add_test(NAME mdec.stream
  COMMAND schwaystation-test-mdec stream
  LABELS mdec portable TIMEOUT 10)

# Fault prevented: nonuniform pixels reverse nibble/byte order, clipping loses
# either signed endpoint, or FE00 padding before DC is treated as coefficients.
schway_add_test(NAME mdec.vectors
  COMMAND schwaystation-test-mdec vectors
  LABELS mdec portable TIMEOUT 10)

# Fault prevented: a completed block is lost when DMA1 must drain a nearly
# full output FIFO; the waiting block and no-deadline state also round-trip.
schway_add_test(NAME mdec.fifo
  COMMAND schwaystation-test-mdec fifo
  LABELS mdec portable TIMEOUT 10)

# Fault prevented: the pinned 4-bit guest's 32-word DMA0 coefficient stream
# fails to publish its completed block when the bounded command ends without EOB.
schway_add_test(NAME mdec.guest_input
  COMMAND schwaystation-test-mdec guest_input
  LABELS mdec portable TIMEOUT 10)

# Fault prevented: the unsigned 4-bit pinned vector or its nearest-nibble
# relation to the pinned 8-bit fixture changes independently of the 8-bit run.
schway_add_test(NAME mdec.guest_4bit
  COMMAND schwaystation-test-mdec guest_4bit
  LABELS mdec portable TIMEOUT 10)

# Fault prevented: the deterministic 8-bit regression selector is accidentally
# omitted while replacing the hardware conformance comparison.
schway_add_test(NAME mdec.guest_8bit_regression
  COMMAND schwaystation-test-mdec guest_8bit_regression
  LABELS mdec portable TIMEOUT 10)

# Fault prevented: a compressed input source ends in a partial MDEC block or
# malformed halfword span is treated as a complete mono block.
schway_add_test(NAME mdec.input_coverage
  COMMAND schwaystation-test-mdec input_coverage
  LABELS mdec portable TIMEOUT 10)
