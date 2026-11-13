# The three root counters: sources, targets, sync modes and interrupts.

# The counters on the system clock: closed-form counting, targets, wraps, the
# write hold and the four interrupt modes. One executable runs the case named on
# its command line; it sees src/ to drive the registers through the bus and read
# the scheduler's deadline.
add_executable(schwaystation-test-timers ${CMAKE_CURRENT_LIST_DIR}/timer_tests.c
  ${CMAKE_CURRENT_LIST_DIR}/ref.c)
target_include_directories(schwaystation-test-timers PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-timers PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-timers PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-timers)

# Fault prevented: a counter that does not count the system clock over 8 at
# every eighth tick, a FFFFh wrap that is not 10000h cycles long, an interrupt
# raised at another tick than the event's or by anything but the scheduler.
schway_add_test(NAME timers.div8.wrap_irq
  COMMAND schwaystation-test-timers div8_wrap_irq
  LABELS timers portable TIMEOUT 10)

# Fault prevented: a reset at the target whose loop is not target + 2
# cycles, an extra or missing cycle at 0, or a reached flag on the wrong
# cycle.
schway_add_test(NAME timers.target.period
  COMMAND schwaystation-test-timers target_period
  LABELS timers portable TIMEOUT 10)

# Fault prevented: a wrap from FFFFh to 0 that dwells like the target reset,
# or a loop that is not 10000h cycles.
schway_add_test(NAME timers.wrap.no_dwell
  COMMAND schwaystation-test-timers wrap_no_dwell
  LABELS timers portable TIMEOUT 10)

# Fault prevented: a counter written above the target that compares anyway,
# or one that does not compare again after the wrap.
schway_add_test(NAME timers.target.count_above
  COMMAND schwaystation-test-timers count_above
  LABELS timers portable TIMEOUT 10)

# Fault prevented: a COUNT or MODE write not shown for two cycles, a TARGET
# write that holds the counter, or a lane that is not read or written as its
# bytes.
schway_add_test(NAME timers.write.hold
  COMMAND schwaystation-test-timers write_hold
  LABELS timers portable TIMEOUT 10)

# Fault prevented: a MODE write that leaves the counter, the interrupt line
# or the one-shot as it was, or the wrong pause for counter 2.
schway_add_test(NAME timers.mode.write
  COMMAND schwaystation-test-timers mode_write
  LABELS timers portable TIMEOUT 10)

# Fault prevented: reached flags that survive a MODE read, or a read that
# clears only one of them.
schway_add_test(NAME timers.mode.read_clears
  COMMAND schwaystation-test-timers mode_read_clears
  LABELS timers portable TIMEOUT 10)

# Fault prevented: a one-shot counter that raises twice, or one that a MODE
# write does not re-arm.
schway_add_test(NAME timers.irq.oneshot
  COMMAND schwaystation-test-timers irq_oneshot
  LABELS timers portable TIMEOUT 10)

# Fault prevented: an interrupt raised while the bit is set, one brought
# back by an acknowledgement for an event before it, or none after it.
schway_add_test(NAME timers.irq.repeat
  COMMAND schwaystation-test-timers irq_repeat
  LABELS timers portable TIMEOUT 10)

# Fault prevented: pulse mode that moves bit 10 or skips an edge after an
# acknowledgement.
schway_add_test(NAME timers.irq.pulse
  COMMAND schwaystation-test-timers irq_pulse
  LABELS timers portable TIMEOUT 10)

# Fault prevented: toggle mode that raises on every event or on the flip
# back, loses the parity of an unwatched span, or gives the scheduler an
# event that raises nothing.
schway_add_test(NAME timers.irq.toggle
  COMMAND schwaystation-test-timers irq_toggle
  LABELS timers portable TIMEOUT 10)

# Fault prevented: a repeating pulse counter at target 0 that gives the
# scheduler an event for every hit, or one that raises while the bit is set.
schway_add_test(NAME timers.storm.collapse
  COMMAND schwaystation-test-timers storm_collapse
  LABELS timers portable TIMEOUT 10)

# Fault prevented: a deadline at or before the tick it was made at in any
# mode of any counter, which would run the same event for ever.
schway_add_test(NAME timers.deadline.progress
  COMMAND schwaystation-test-timers deadline_progress
  LABELS timers portable TIMEOUT 10)

# Fault prevented: a state that depends on when the counters were last
# caught up, or a save that changes the instance.
schway_add_test(NAME timers.state.cadence
  COMMAND schwaystation-test-timers state_cadence
  LABELS timers portable TIMEOUT 10)

# Fault prevented: a counter synchronised to the vertical blank that does not
# see the frame end's blank, so that its reset never happens.
schway_add_test(NAME timers.sync.t1_vblank_frame
  COMMAND schwaystation-test-timers t1_vblank_frame
  LABELS timers portable TIMEOUT 10)

# Fault prevented: a counter 0 synchronisation mode that pauses, resets or
# releases at the wrong edge of the horizontal blank, or a MODE write that
# ignores the level already there.
schway_add_test(NAME timers.sync.t0_hblank_modes
  COMMAND schwaystation-test-timers t0_hblank_modes
  LABELS timers portable TIMEOUT 10)

# Fault prevented: the same for counter 1 and the vertical blank.
schway_add_test(NAME timers.sync.t1_vblank_modes
  COMMAND schwaystation-test-timers t1_vblank_modes
  LABELS timers portable TIMEOUT 10)

# Fault prevented: counter 2 that runs when synchronisation stops it, stops
# when it should run, or is moved by a blank pushed to the others.
schway_add_test(NAME timers.sync.t2_stop_modes
  COMMAND schwaystation-test-timers t2_stop_modes
  LABELS timers portable TIMEOUT 10)

# Fault prevented: dots pushed to counter 0 that are not counted on the dot
# clock, that count on the system clock, or whose raise is missed.
schway_add_test(NAME timers.source.dot_feed
  COMMAND schwaystation-test-timers dot_feed
  LABELS timers portable TIMEOUT 10)

# Fault prevented: counter 1 on the horizontal blank that counts the wrong
# edge or repeated levels, or does not raise at its target.
schway_add_test(NAME timers.source.hblank_feed
  COMMAND schwaystation-test-timers hblank_feed
  LABELS timers portable TIMEOUT 10)

# Fault prevented: timer accesses remain valid when the dot source is driven
# by the integrated GPU beam.
schway_add_test(NAME timers.source.beam_register_access
  COMMAND schwaystation-test-timers beam_register_access
  LABELS timers portable TIMEOUT 10)

# Fault prevented: an off-by-one in the hold, the dwell, a landing or a
# synchronisation edge that no hand-made case reaches. A seeded random stream
# of 20,000 operations drives the lazy model and a per-cycle reference, and
# every read and every interrupt bit must agree.
schway_add_test(NAME timers.reference.random
  COMMAND schwaystation-test-timers reference_random
  LABELS timers portable TIMEOUT 30)
