# Register-level bus behaviour.
# The bus and the interrupt registers through the module's own interface. One
# executable runs the case named on its command line. It also sees src/, so it
# can drive the bus, load a BIOS image and read the fault record.
add_executable(schwaystation-test-bus ${CMAKE_CURRENT_LIST_DIR}/bus_tests.c)
target_include_directories(schwaystation-test-bus PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-bus PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-bus PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-bus)

# Fault prevented: a RAM mirror that reads another address, a window past the
# banks RAM_SIZE selects that answers, or a RAM access that charges the wrong
# cycles.
schway_add_test(NAME bus.ram_mirrors
  COMMAND schwaystation-test-bus ram_mirrors
  LABELS bus portable TIMEOUT 10)

# Fault prevented: a delay-register write that leaves the port costs stale, or
# a memory-control register that keeps bits the hardware does not have.
schway_add_test(NAME bus.memctl_costs
  COMMAND schwaystation-test-bus memctl_costs
  LABELS bus portable TIMEOUT 10)

# Fault prevented: a scratchpad that answers in KSEG1, runs code, or runs off
# its 1 KiB.
schway_add_test(NAME bus.scratchpad
  COMMAND schwaystation-test-bus scratchpad
  LABELS bus portable TIMEOUT 10)

# Fault prevented: a BIOS window that reads past the host's image, answers with
# none mapped, or loses the image at a power reset.
schway_add_test(NAME bus.bios_window
  COMMAND schwaystation-test-bus bios_window
  LABELS bus portable TIMEOUT 10)

# Fault prevented: an expansion read that returns data, a debug port that acts
# with the profile flag clear, or a neighbour of a debug port that answers.
schway_add_test(NAME bus.exp_regions
  COMMAND schwaystation-test-bus exp_regions
  LABELS bus portable TIMEOUT 10)

# Fault prevented: a segment that classifies wrong, a KSEG2 hole that answers,
# or a KUSEG alias that misses the physical map.
schway_add_test(NAME bus.kseg2
  COMMAND schwaystation-test-bus kseg2
  LABELS bus portable TIMEOUT 10)

# Fault prevented: an interrupt acknowledge that clears the wrong bits, or a
# source number past 10 that sets a bit.
schway_add_test(NAME bus.irq_edge
  COMMAND schwaystation-test-bus irq_edge
  LABELS bus portable TIMEOUT 10)

# Fault prevented: an interrupt line that ignores the mask, or a mask that keeps
# bits the register does not have.
schway_add_test(NAME bus.irq_line
  COMMAND schwaystation-test-bus irq_line
  LABELS bus portable TIMEOUT 10)

# Fault prevented: a CAUSE bit 10 that lags the interrupt registers, or one
# that disturbs CAUSE bits 8-9.
schway_add_test(NAME bus.irq_cause_bit
  COMMAND schwaystation-test-bus irq_cause_bit
  LABELS bus portable TIMEOUT 10)

# The instruction cache, cache isolation and the write queue through the CPU. One
# executable runs the case named on its command line; it sees src/, so it can read
# the cache arrays and the queue count.
add_executable(schwaystation-test-cache ${CMAKE_CURRENT_LIST_DIR}/cache_tests.c)
target_include_directories(schwaystation-test-cache PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-cache PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-cache PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-cache)

# Fault prevented: a cached fetch that reads past the line's end or short of it, a
# hit that costs a bus cycle, a KSEG1 fetch that fills or changes the line, or a
# store that changes what a hit returns.
schway_add_test(NAME bus.icache_fill
  COMMAND schwaystation-test-cache icache_fill
  LABELS bus portable TIMEOUT 10)

# Fault prevented: a tag hit on an invalid word that does not refill, a fill that stops
# short of the line's end for IBLKSZ, a store that clears a valid bit, or a bus error
# in a later word that leaves a fault the CPU never asked for.
schway_add_test(NAME bus.icache_valid_bits
  COMMAND schwaystation-test-cache icache_valid_bits
  LABELS bus portable TIMEOUT 10)

# Fault prevented: an isolated access that reaches the bus, a load that depends on the
# tag, a tag store that ignores TAG or IS1, a cache-control write that isolation
# swallows, or a FlushCache sequence that leaves a line valid.
schway_add_test(NAME bus.icache_isolation
  COMMAND schwaystation-test-cache icache_isolation
  LABELS bus portable TIMEOUT 10)

# Fault prevented: a write queue deeper or shallower than four, a fifth store that
# does not wait, a scratchpad or failed store that joins the queue, or a replayed
# store that does.
schway_add_test(NAME bus.write_queue_depth
  COMMAND schwaystation-test-cache write_queue_depth
  LABELS bus portable TIMEOUT 10)

# Fault prevented: a KSEG1 read, write or fetch that leaves stores queued, a cached
# read that drains them, a drain charged to an empty queue, or stores that land out of
# order.
schway_add_test(NAME bus.write_queue_drain
  COMMAND schwaystation-test-cache write_queue_drain
  LABELS bus portable TIMEOUT 10)

# Fault prevented: an exception that drops, reorders or drains the queued stores.
schway_add_test(NAME bus.write_queue_exception
  COMMAND schwaystation-test-cache write_queue_exception
  LABELS bus portable TIMEOUT 10)

# Fault prevented: a saved state that loses the queue count or the cache, or a load
# that restores them differently from the way they were saved.
schway_add_test(NAME bus.write_queue_state
  COMMAND schwaystation-test-cache write_queue_state
  LABELS bus portable TIMEOUT 10)
