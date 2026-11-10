---
phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
plan: 09
subsystem: timers
tags:
  - timers
  - root-counters
  - scheduler
  - interrupts
  - state

requires:
  - phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
    provides: the timers state struct, reset and bounded state walk (plan 01); the block clock, the deadline, the event runner and the no-program run loop (plan 03); the bus dispatch to schway__timers_io and the I_STAT read sync (plan 04)
provides:
  - the three root counters as lazy closed-form devices on the system clock and, for counter 2, the system clock over 8
  - targets with the target + 2 loop, FFFFh wraps with no dwell, the two-cycle write hold, the reached flags and every MODE rule
  - one-shot, repeat, pulse and toggle interrupts, with a storm collapsing to one raise and toggle reduced to parity
  - one derived deadline per counter, only for the next event that would make a new I_STAT edge
  - a state that walks the view as of the instance's tick, independent of when a catch-up ran
  - tests/timers/timer_tests.c with 14 cases and docs/hardware/timers.md
affects:
  - 03-11
  - 03-14
  - 03-15
  - phase-3 plans that add the sync modes, the beam entry points, the oracle and the timers_lines fixture

actuals:
  tokens: 16200
  tasks: 3
  commits: 3

plan_head_before: e880abf3b8b750bdfa8265234a1a136ebbfb0e48
plan_head_after: cb05725cb209f90d4dd10b0e596ce946866b53fc
commits: 3

tech-stack:
  added: []
  patterns:
    - "a counter is a view as of one tick plus a pure function that carries it forward by n of its own cycles (value, hold, reached bits, interrupt line, one-shot), so the same function applies a span, gives the saved view and, by bisection, finds the next raise"
    - "an interrupt span is reduced to an event count: pulse and repeat need only whether it is nonzero, toggle needs its parity and whether it reached two, one-shot needs the first two"
    - "an advance runs a deadline that falls on its end tick before it returns, so a quiescent instance never holds an event behind its own tick"

key-files:
  created:
    - tests/timers/timer_tests.c
    - docs/hardware/timers.md
  modified:
    - src/timers.c
    - src/timers.h
    - src/bus.c
    - src/frame.c
    - tests/core/test_state.c
    - tests/timers/tests.cmake
    - tests/timers/inventory.txt

key-decisions:
  - "MODE keeps the 10 written bits, irq_line is bit 10 (1 idle), reached has the target flag in bit 0 and the FFFFh flag in bit 1; power-on is all zero, and every program writes MODE before it relies on bit 10"
  - "The write hold is two of the counter's own cycles after the cycle of the write: a write at tick W shows its value at W+1 and W+2 and counts from W+3; a reset at the target is the same hold with 0 counting the landing cycle, giving 0,0,1 at target 1 and a loop of target + 2"
  - "A landing is the counter taking a value by counting or by a reset, never by a software write, so a COUNT or MODE write cannot raise an event; with the target at FFFFh the target and FFFFh landings are one event"
  - "A MODE read of any width returns bits 11 and 12 and clears both; counters 0 and 1 on the dot clock or hblank do not advance and counter 2 with sync mode 0 or 3 is paused, derived at the MODE write"
  - "A counter whose I_STAT bit is set gives no deadline; the I_STAT write catches the counters up first, so events that happened while the bit was set cannot come back as an interrupt after the acknowledgement"
  - "next is found by bisection over the span function within a horizon of 30000h counter cycles, so the deadline and the applied span cannot disagree"

patterns-established:
  - "a whole frame of a pulse-repeat counter at target 0 with no acknowledgement is one event and leaves the scheduler no deadline; one acknowledgement lets exactly one more through"

requirements-completed: []

coverage:
  - id: D1
    description: "Counter 2 on the system clock over 8 counts at every eighth absolute tick, reaches FFFFh at tick 8 * 10001h after a MODE write at 0, raises I_STAT bit 6 through the scheduler at exactly that tick, loops every 10000h cycles and wraps to 0 with no dwell"
    requirement: "GTE-03"
    verification:
      - kind: unit
        ref: "tests/timers/timer_tests.c#div8_wrap_irq (timers.div8.wrap_irq)"
        status: pass
    human_judgment: false
  - id: D2
    description: "A reset at the target takes target + 2 cycles (0,0,1,0,0,1 at target 1; 7 cycles at target 5), the FFFFh wrap is 10000h cycles with no dwell, and a counter written above the target compares only after it wraps"
    requirement: "GTE-03"
    verification:
      - kind: unit
        ref: "tests/timers/timer_tests.c#target_period,wrap_no_dwell,count_above (timers.target.period, timers.wrap.no_dwell, timers.target.count_above)"
        status: pass
    human_judgment: false
  - id: D3
    description: "A COUNT or MODE write shows its value for two cycles before counting, a TARGET write has no hold, byte and halfword lanes read and write as their bytes, and offset 0Ch reads 0"
    requirement: "GTE-03"
    verification:
      - kind: unit
        ref: "tests/timers/timer_tests.c#write_hold (timers.write.hold)"
        status: pass
    human_judgment: false
  - id: D4
    description: "A MODE write zeroes the counter, starts the hold, sets bit 10, re-arms the one-shot and derives counter 2's pause from the synchronisation modes; a MODE read clears bits 11 and 12 once"
    requirement: "GTE-03"
    verification:
      - kind: unit
        ref: "tests/timers/timer_tests.c#mode_write,mode_read_clears (timers.mode.write, timers.mode.read_clears)"
        status: pass
    human_judgment: false
  - id: D5
    description: "One-shot raises once and is re-armed by MODE; repeat raises after each acknowledgement and an acknowledgement does not bring back an event that happened while the bit was set; pulse leaves bit 10 at 1; toggle flips bit 10, raises on every second event, gives the scheduler the next raise and keeps the parity of an unwatched span"
    requirement: "GTE-03"
    verification:
      - kind: unit
        ref: "tests/timers/timer_tests.c#irq_oneshot,irq_repeat,irq_pulse,irq_toggle (timers.irq.oneshot, timers.irq.repeat, timers.irq.pulse, timers.irq.toggle)"
        status: pass
    human_judgment: false
  - id: D6
    description: "A pulse-repeat storm at target 0 is one scheduler event over a whole frame, and in all 24 mode combinations on all three counters every deadline is strictly after the tick of the event or acknowledgement that produced it"
    requirement: "GTE-03"
    verification:
      - kind: unit
        ref: "tests/timers/timer_tests.c#storm_collapse,deadline_progress (timers.storm.collapse, timers.deadline.progress)"
        status: pass
    human_judgment: false
  - id: D7
    description: "The saved state is the view as of the instance's tick: two instances running one program, one caught up before and after every step, save identical bytes at every step, and saving changes nothing in the instance; core.state.* still passes"
    requirement: "GTE-03"
    verification:
      - kind: unit
        ref: "tests/timers/timer_tests.c#state_cadence (timers.state.cadence); ctest -R '^core\\.state\\.' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D8
    description: "docs/hardware/timers.md (about 11 KB) covers the registers and MODE bits, the sources and which do not advance yet, the lazy model, targets, wraps, the hold, the four interrupt modes, the deadline rule, the zero-width vertical blank, the state view and the sources, with no planning identifier"
    requirement: "GTE-03"
    verification:
      - kind: other
        ref: "grep -qE 'D-[0-9]{2}|GTE-0[0-9]|PX\\.[0-9]|DEC\\.[0-9]|XFER-' docs/hardware/timers.md src/timers.c tests/timers/timer_tests.c (exit 1, no match); cmake --workflow --preset hygiene (exit 0)"
        status: pass
    human_judgment: false

duration: 19min
completed: 2026-11-10
status: complete
---

# Phase 3 Plan 09: The root counters on the system clock Summary

**Lazy closed-form root counters for the system clock and the clock over 8: targets with the target + 2 loop, FFFFh wraps, the two-cycle write hold, every MODE rule and all four interrupt modes, one deadline per counter only for a new I_STAT edge, and a state that walks the view as of the tick, with 14 `timers.*` tests and the first timers page.**

## Performance

- **Duration:** 19 min
- **Started:** 2026-11-10T06:02:30Z (approximate; the start time was not captured)
- **Completed:** 2026-11-10T06:21:38Z
- **Tasks:** 3
- **Files modified:** 9 (2 created)

## Accomplishments

- `src/timers.c` is a pure span function: it carries one counter's view (value, hold, reached bits, interrupt line, one-shot, pause) forward by n of its own cycles in a bounded number of segments, never per cycle. The same function applies a catch-up, produces the saved view and, by bisection within a 30000h-cycle horizon, finds the next cycle that would raise an interrupt, so a deadline and an applied span cannot disagree.
- Interrupt spans collapse to counts: pulse and repeat need only that an event happened, toggle needs the parity and whether two happened, one-shot needs the first two. A counter at target 0 in repeat mode is one scheduler event over a whole frame, and the next deadline is none while its I_STAT bit is set.
- The state walks the view as of the instance's tick on a local copy and writes it back (with the tick as the base) only on a load, so the bytes do not depend on when a catch-up last ran and saving changes nothing in the instance.
- The closed form was checked once, outside the repository, against a naive per-cycle stepper on 120,000 random counters (targets and values at the edges, all reset points, interrupt bits, hold, pause, one-shot state): final view, raise, a composed run of random chunks, and the distance to the next raise all matched (exit 0).

## Task Commits

Each task was committed atomically:

1. **Task 1: Tracer - Timer 2 on system clock / 8 counts, wraps and raises its interrupt through the scheduler** - `fe16b2d` (feat)
2. **Task 2: Targets, the write hold, the MODE rules and the four interrupt modes** - `a03dab7` (test)
3. **Task 3: Deadlines and storms, the normalized state view, and the timers page** - `cb05725` (feat)

**Plan metadata:** the docs commit that carries this summary (docs: complete plan).

The tracer gate: `HUMAN_VERIFY_MODE` is end-of-phase and the tracer's verify is automated only, so `timers.div8.wrap_irq` was re-run end to end (exit 0) and expansion went on.

## Files Created/Modified

- `src/timers.c` - the register window, the span function, the interrupt event reduction, the deadline, the catch-up and the state view
- `src/timers.h` - the field comments for the view
- `src/bus.c` - an I_STAT write now catches the counters up first, as a read does
- `src/frame.c` - an advance runs a deadline that falls on its end tick before it returns
- `tests/core/test_state.c` - the device-field case sets counters that do not move, so it still shows the base tick is no part of the state
- `tests/timers/timer_tests.c`, `tests/timers/tests.cmake`, `tests/timers/inventory.txt` - 14 cases, target `schwaystation-test-timers`
- `docs/hardware/timers.md` - the timers page

## Decisions Made

See `key-decisions` above. The reading of the write hold is the one the plan asks to be tested as an observable: the value shows at the two ticks after the write, and counting resumes on the third; psx-spx says "2 clock cycles" and does not say whether the write's own cycle counts, so the page and the tests pin this reading and the page says the model chose it.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] An I_STAT write did not catch the counters up before changing the register**
- **Found during:** Task 1 (designing the deadline)
- **Issue:** the bus synced the counters only on an I_STAT read. With a counter's bit set it has no deadline, so its view stays old; an acknowledgement then recomputed the deadline from that old view and found an event from before the acknowledgement, which would raise a spurious interrupt at once.
- **Fix:** the bus syncs on any I_STAT access at the access's tick (`src/bus.c`, 8 lines). `timers.irq.repeat` fails without it (a mutation run showed exactly that case failing).
- **Files modified:** src/bus.c
- **Verification:** `ctest --test-dir build/ci -L '^timers$'` (exit 0)
- **Committed in:** fe16b2d

**2. [Rule 1 - Bug] A save or load at the tick a deadline fell on lost the interrupt**
- **Found during:** Task 1 (designing the state view)
- **Issue:** a run that ends exactly on a deadline (or one instruction past it) returned before running the event. The saved view is as of the instance's tick, so it had already used the event while `I_STAT` had not been raised; a load, or a host that saved there, would lose the interrupt.
- **Fix:** `advance` in `src/frame.c` runs a deadline at or before its end tick before it returns (one compare). A run that ends on a deadline now raises at that tick, the same tick as one run through it would. The tracer test asserts it: `timers.div8.wrap_irq` fails without the change.
- **Files modified:** src/frame.c
- **Verification:** `ctest --test-dir build/ci -L '^timers$'` (exit 0); the whole ci workflow (288 tests, exit 0)
- **Committed in:** fe16b2d

**3. [Rule 1 - Bug] The device-field state test set a moving counter and expected its stored fields back**
- **Found during:** Task 3 (state view)
- **Issue:** `core.state.device_fields` set timer fields directly and expected them to be saved as written, and that changing the base tick does not change the bytes. With the state holding the view as of the tick, a counter that counts moves between the base and the tick, so both expectations became wrong for a moving counter.
- **Fix:** the case sets counters that do not move (sources with no clock yet, and counter 2 paused), which keeps the assertion's meaning: the base tick is no part of the state when the view does not depend on it. The adaptation is necessary and no check was dropped or weakened.
- **Files modified:** tests/core/test_state.c
- **Verification:** `ctest --test-dir build/ci -R '^core\.state\.' --no-tests=error` (exit 0)
- **Committed in:** cb05725

---

**Total deviations:** 3 auto-fixed (3 Rule 1)
**Impact on plan:** all three are correctness requirements of the lazy model that the plan names (an I_STAT write recomputes the deadline, the bytes do not depend on when a catch-up ran). Two touch `src/bus.c` and `src/frame.c`, which the plan's file list does not name; each change is under 10 lines and carries a test that fails without it.

## Issues Encountered

- A tracer run without the frame.c change shows the effect of deviation 2: with no tail run, the interrupt appeared only on the next read of `I_STAT`, because that read syncs. The test reads `inst->irq.stat` directly so the scheduler path itself is what is pinned.
- Not runnable locally: the asan lane and the be32 lane need clang-21, and the nofp preset needs GCC (this host has AppleClang). Hosted CI runs them; the code uses fixed-width unsigned arithmetic only and the new test compares struct fields rather than padding, for those lanes.
- The perf lane (valgrind) is not available on this host. The change to the hot path is one compare per advance in `src/frame.c`, outside the CPU's block loop.

## Triage

None requested by the plan. Open seed for the next timers plan (03-11): the unbuilt-source rule for counter 0 on the dot clock and counter 1 on hblank (an access should count as `unbuilt dev=timer`) is not done here, as the plan assigns it there; and `schway__timers_vblank`, `_hblank` and `_dots` are still empty.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 03-11 can add the synchronisation modes, the beam entry points, the unbuilt-source rule and the naive reference oracle on top of `advance`, `run_steps` and the `paused` and `free_after` fields; the oracle used for this plan's check is not committed.
- The `timers_lines` fixture can read counter 2 on the system clock over 8 and count FFFFh interrupts from the instance as it is.

## Self-Check: PASSED

- Created files exist: `tests/timers/timer_tests.c`, `docs/hardware/timers.md`.
- Commits `fe16b2d`, `a03dab7` and `cb05725` are ancestors of HEAD.
- `cmake --workflow --preset ci` (exit 0, 288 tests), `cmake --workflow --preset dev` (exit 0, 288 tests), `cmake --workflow --preset hygiene` (exit 0, 23 tests); `ctest --test-dir build/ci -L '^timers$' --no-tests=error` (exit 0, 14 tests); `ctest --test-dir build/ci -R '^core\.state\.' --no-tests=error` (exit 0).
- Acceptance greps: `grep -q 'schway__irq_raise' src/timers.c && grep -q 'schway__sched_recalc' src/timers.c` (exit 0); the planning-ID grep over `docs/hardware/timers.md src/timers.c tests/timers/timer_tests.c` (exit 1, no match).

---
*Phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers*
*Completed: 2026-11-10*
