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

# Fault prevented: DPCR not at 07654321h after a power reset, or a reset that
# keeps what the program wrote.
schway_add_test(NAME dma.dpcr.reset
  COMMAND schwaystation-test-dma dpcr_reset
  LABELS dma portable TIMEOUT 10)

# Fault prevented: arbitration that picks by channel number alone, lets the CPU
# field decide, ignores the master enables or breaks a tie the wrong way.
schway_add_test(NAME dma.dpcr.priority
  COMMAND schwaystation-test-dma dpcr_priority
  LABELS dma portable TIMEOUT 10)

# Fault prevented: a channel that starts with its master enable clear, or one
# that waits forever after the enable is set.
schway_add_test(NAME dma.dpcr.enable
  COMMAND schwaystation-test-dma dpcr_enable
  LABELS dma portable TIMEOUT 10)

# Fault prevented: DICR values that differ from the console's: the derived bit 31,
# write-1-to-clear flags, flags set without their enables, stored bits.
schway_add_test(NAME dma.dicr.reads
  COMMAND schwaystation-test-dma dicr_reads
  LABELS dma portable TIMEOUT 10)

# Fault prevented: an interrupt raised on every completion, on a write that keeps
# bit 31 at 1, or never after an acknowledgement.
schway_add_test(NAME dma.dicr.irq_edge
  COMMAND schwaystation-test-dma dicr_irq_edge
  LABELS dma portable TIMEOUT 10)

# Fault prevented: OTC's CHCR reading back anything but its three writable bits
# and the fixed bit 1.
schway_add_test(NAME dma.otc.readback
  COMMAND schwaystation-test-dma otc_readback
  LABELS dma portable TIMEOUT 10)

# Fault prevented: a table that wraps wrongly at the bottom of the address space,
# a word past the RAM window that lands in RAM, or a first fault from it.
schway_add_test(NAME dma.otc.madr_wrap
  COMMAND schwaystation-test-dma otc_madr_wrap
  LABELS dma portable TIMEOUT 10)

# Fault prevented: OTC starting on bit 24 alone, or on bit 28 alone.
schway_add_test(NAME dma.otc.needs_trigger
  COMMAND schwaystation-test-dma otc_needs_trigger
  LABELS dma portable TIMEOUT 10)

# Fault prevented: OTC honouring the direction, step, sync or chopping bits.
schway_add_test(NAME dma.otc.ignores_bits
  COMMAND schwaystation-test-dma otc_ignores_bits
  LABELS dma portable TIMEOUT 10)

# Fault prevented: a word count of 0 moving nothing, or the wrong cost for the
# longest transfer (69,632 cycles).
schway_add_test(NAME dma.otc.bcr_zero
  COMMAND schwaystation-test-dma otc_bcr_zero
  LABELS dma portable TIMEOUT 10)

# Fault prevented: an event call that finishes a unit early, leaves its deadline
# at or before the tick it was called with (the scheduler would not advance),
# or leaves a finished unit's deadline set.
schway_add_test(NAME dma.engine.step_contract
  COMMAND schwaystation-test-dma step_contract
  LABELS dma portable TIMEOUT 10)

# Fault prevented: a CPU read from RAM that runs on while a transfer holds the
# bus.
schway_add_test(NAME dma.stall.cpu_access
  COMMAND schwaystation-test-dma stall_cpu_access
  LABELS dma portable TIMEOUT 30)

# Fault prevented: a CPU that stops for a transfer when it only touches the
# scratchpad, the cache and the registers.
schway_add_test(NAME dma.stall.not_scratch
  COMMAND schwaystation-test-dma stall_not_scratch
  LABELS dma portable TIMEOUT 30)

# Fault prevented: a store that holds the CPU while the write queue has room, a
# fifth store that does not, or a scratchpad store that does.
schway_add_test(NAME dma.stall.write_queue
  COMMAND schwaystation-test-dma stall_write_queue
  LABELS dma portable TIMEOUT 30)

# Fault prevented: a CHCR read that shows the busy value, or one that does not
# hold the CPU until the unit is done.
schway_add_test(NAME dma.otc.read_sees_completion
  COMMAND schwaystation-test-dma otc_read_sees_completion
  LABELS dma portable TIMEOUT 30)

# Fault prevented: a burst from RAM that moves the wrong words to a device,
# forgets to count or checksum them in the stand-in sink, leaves MADR where it
# was, or forgets the completion flag and the interrupt.
schway_add_test(NAME dma.sync0.gpu_sink
  COMMAND schwaystation-test-dma sync0_gpu_sink
  LABELS dma portable TIMEOUT 10)

# Fault prevented: a read from a device that does not write zeros to RAM, a
# backward step that goes forward, or a CD-ROM burst off its 24 clocks per word.
schway_add_test(NAME dma.sync0.from_device
  COMMAND schwaystation-test-dma sync0_from_device
  LABELS dma portable TIMEOUT 10)

# Fault prevented: sync mode 1 that moves every block on one request, loses
# the block count or the address, or finishes without the flag.
schway_add_test(NAME dma.sync1.dreq_blocks
  COMMAND schwaystation-test-dma sync1_dreq_blocks
  LABELS dma portable TIMEOUT 10)

# Fault prevented: a channel waiting for its request line that holds the CPU
# or slows a program reading RAM.
schway_add_test(NAME dma.sync1.no_dreq_no_stall
  COMMAND schwaystation-test-dma sync1_no_dreq_no_stall
  LABELS dma portable TIMEOUT 30)

# Fault prevented: a channel whose burst costs another number of cycles than
# its device rate plus the DRAM row, or a 10000h-word CD-ROM burst past the
# longest the engine bounds.
schway_add_test(NAME dma.costs.per_channel
  COMMAND schwaystation-test-dma costs_per_channel
  LABELS dma portable TIMEOUT 10)

# Fault prevented: a DPCR priority change that does not take effect at the
# next unit boundary.
schway_add_test(NAME dma.dpcr.rearbitrate
  COMMAND schwaystation-test-dma dpcr_rearbitrate
  LABELS dma portable TIMEOUT 10)

# Fault prevented: a list that sends its headers to the device, loses or
# reorders a node, follows the wrong address, ends on anything but the marker's
# bit 23, or finishes without the flag and the interrupt.
schway_add_test(NAME dma.list.walk
  COMMAND schwaystation-test-dma list_walk
  LABELS dma portable TIMEOUT 10)

# Fault prevented: a list whose first header ends it that walks on, or that
# finishes without the flag and the interrupt.
schway_add_test(NAME dma.list.empty
  COMMAND schwaystation-test-dma list_empty
  LABELS dma portable TIMEOUT 10)

# Fault prevented: a list that never ends hanging a step, finishing, raising a
# flag or an interrupt, passing more than 1000h nodes in one step, or a frame
# that does not end.
schway_add_test(NAME dma.list.self_ref
  COMMAND schwaystation-test-dma list_self_ref
  LABELS dma portable TIMEOUT 30)

# Fault prevented: the CHCR word 0 that leaves a looping list running, ends it
# with a flag or an interrupt, or leaves the channel unable to run the next list.
schway_add_test(NAME dma.list.stop
  COMMAND schwaystation-test-dma list_stop
  LABELS dma portable TIMEOUT 30)

# Fault prevented: a chopped transfer whose windows or CPU gaps take a time
# outside the bands around the 64 rows of a public hardware log.
schway_add_test(NAME dma.chopping.band
  COMMAND schwaystation-test-dma chopping_band
  LABELS dma portable TIMEOUT 30)

# Fault prevented: a list that takes the CPU's time at a rate outside the band
# around the slowdown of a public hardware log.
schway_add_test(NAME dma.list.cpu_band
  COMMAND schwaystation-test-dma list_cpu_band
  LABELS dma portable TIMEOUT 30)
