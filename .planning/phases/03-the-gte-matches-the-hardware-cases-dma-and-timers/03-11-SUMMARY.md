---
phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
plan: 11
subsystem: timers
tags:
  - timers
  - sync-modes
  - beam-feeds
  - property-testing
  - reference-model

requires:
  - phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
    provides: the lazy closed-form counters, the deadline and the state view (plan 09); the frame end's zero-width vertical blank push (plan 03); schway__bus_unbuilt (plan 04)
provides:
  - the synchronisation modes of counters 0 and 1 against pushed horizontal and vertical blanks, with the pause derived from the blank level on a MODE write
  - schway__timers_vblank, schway__timers_hblank and schway__timers_dots as working entry points, with the blank levels in the state
  - counter 0 on the dot clock and counter 1 on the horizontal blank counted by pushes, with no deadline of their own
  - an access to a counter whose MODE waits on a signal nothing makes yet counted as unbuilt dev=timer, never a fault
  - tests/prop.h, an integer seeded splitmix64 harness that the GTE property test reuses
  - tests/timers/ref.c, a per-cycle reference, and timers.reference.random comparing it with the lazy model
affects:
  - 03-14
  - 03-15
  - 03-17
  - phase-4 GPU line machine (feeds the three entry points)

actuals:
  tokens: 10600
  tasks: 3
  commits: 3

plan_head_before: f4383c4a90b811ebe448869afed92adc853e898e
plan_head_after: 6acaaf008ac114f3cb69fefc6ded21f2bf3d38fe
commits: 3

tech-stack:
  added: []
  patterns:
    - "a beam signal is a stored level plus an edge function: a push of the level already held changes nothing, the begin of a blank is the 0 to 1 push, and the push first catches the counters up to its tick"
    - "a counter on a beam clock has no tick to wait for: its steps are applied by the push through the same span function that applies a tick span, and it raises there"
    - "the per-cycle reference shares no code with the lazy model; a seeded stream with irregular catch-up cadences compares every read and every I_STAT bit"

key-files:
  created:
    - tests/prop.h
    - tests/timers/ref.c
    - tests/timers/ref.h
  modified:
    - src/timers.c
    - src/timers.h
    - tests/timers/timer_tests.c
    - tests/timers/tests.cmake
    - tests/timers/inventory.txt
    - docs/hardware/timers.md

key-decisions:
  - "A blank is a stored level (state field timers.blank, bit 0 vertical and bit 1 horizontal); the beginning of a blank is the push 0 to 1, a push of the level held is not an edge"
  - "A reset by a blank takes effect at the tick of the push with no hold, so a counter reset at the frame end reads the cycles since it; a pending write hold is dropped by it"
  - "Mode 3 written inside a blank still waits for a blank to begin; the hold of a write runs while the counter is paused; both are the model's reading of a page that does not say, and the docs and the oracle pin them"
  - "The unbuilt count is made while a MODE that needs the beam is in force, whether or not a test pushes signals; counter 1's vertical-blank sync and everything on counter 2 are not unbuilt"
  - "A counter on the dot clock or the horizontal blank gives the scheduler no deadline (its raise happens in the push), which also fixes a stale deadline such a counter would have got from the system-clock horizon"

patterns-established:
  - "Mutation check of an oracle: seven one-line mutations of src/timers.c (hold, dwell, period, pause edge, mode-3 release, hblank step, prescaler phase) each fail timers.reference.random within 1,000 operations"

requirements-completed: []

coverage:
  - id: D1
    description: "Counter 1 in sync mode 1 resets at the vertical blank the frame end pushes: after a program spins to the frame end the counter reads 0 at the end and the cycles since it afterwards, not the cycles since the MODE write"
    requirement: "GTE-03"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^timers\\.sync\\.t1_vblank_frame$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: "All four synchronisation modes of counter 0 against pushed horizontal blanks and of counter 1 against pushed vertical blanks, written with the blank low and high, including mode 3 releasing once and mode 2 pausing outside the blank"
    requirement: "GTE-03"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^timers\\.sync\\.(t0_hblank_modes|t1_vblank_modes)$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D3
    description: "Counter 2's synchronisation modes 0 and 3 stop the counter for ever, modes 1 and 2 free-run, with the prescaler on or off, unmoved by blanks pushed to the others and across a frame end"
    requirement: "GTE-03"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^timers\\.sync\\.t2_stop_modes$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D4
    description: "Pushed dots count on counter 0 when the dot clock is its source and raise at the target, pushed horizontal blanks count on counter 1 one step per beginning, a big push equals the same number of small ones and returns, and neither feed counts for the system clock"
    requirement: "GTE-03"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^timers\\.source\\.(dot_feed|hblank_feed)$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D5
    description: "An access to a counter whose MODE waits on the beam adds one to unbuilt_n for the timer, raises no fault and leaves the counter where it is however many ticks pass; power-on and the other modes count nothing; the bus sweep still reports no timer address unbuilt at power-on"
    requirement: "GTE-03"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^(timers\\.source\\.unfed_unbuilt|core\\.bus_unbuilt)$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D6
    description: "A naive per-cycle reference and the lazy model agree on every COUNT, MODE and I_STAT bit 4 to 6 over 20,000 seeded operations with gaps of 0 to 300 cycles, and the test fails unless all 20,000 ran; seven mutations of the lazy model are each caught"
    requirement: "GTE-03"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^timers\\.reference\\.random$' --no-tests=error (exit 0, seed 7154F4B1E5C0DE05)"
        status: pass
    human_judgment: false
  - id: D7
    description: "tests/prop.h is a header-only integer splitmix64 harness (54 lines) with seed, next, below and edge32, free of floating point and of planning identifiers, ready for the GTE property test"
    requirement: "GTE-03"
    verification:
      - kind: other
        ref: "grep -q splitmix64 tests/prop.h && ! grep -qE '\\b(float|double)\\b' tests/prop.h tests/timers/ref.c (exit 0); cmake --workflow --preset hygiene (exit 0)"
        status: pass
    human_judgment: false
  - id: D8
    description: "docs/hardware/timers.md gains the synchronisation and beam-source sections, including the integer dot accumulator the GPU phase must use, and the state, sources and not-built sections are brought up to date (15 KB, no planning identifier)"
    requirement: "GTE-03"
    verification:
      - kind: other
        ref: "cmake --workflow --preset hygiene (exit 0); grep for planning identifiers over docs/hardware/timers.md src/timers.c tests/timers (no match)"
        status: pass
    human_judgment: false

duration: 10min
completed: 2026-11-10
status: complete
---

# Phase 3 Plan 11: Sync modes, beam feeds and the per-cycle oracle Summary

**Counters 0 and 1 now synchronise to pushed horizontal and vertical blanks in all four modes (counter 1's reset at the frame end included), count pushed dots and horizontal blanks as clocks, count an access that waits on the missing beam as unbuilt instead of faking it, and a 20,000-operation seeded stream proves the lazy model equal to a per-cycle reference.**

## Performance

- **Duration:** about 10 min (the start time was not captured; the previous plan's state update was at 06:40Z and this summary was written at about 06:50Z)
- **Started:** 2026-11-10T06:40Z (approximate)
- **Completed:** 2026-11-10T06:52Z
- **Tasks:** 3
- **Files modified:** 9 (3 created)

## Accomplishments

- `schway__timers_vblank` and `schway__timers_hblank` keep a level per signal (new state field `timers.blank`) and apply the edge to the counter that listens to it: mode 0 pauses in the blank, mode 1 resets at its beginning, mode 2 resets and pauses outside it, mode 3 releases the pause once. A MODE write takes the pause from the level already there. The frame end's zero-width vertical blank now resets counter 1 in mode 1 (`timers.sync.t1_vblank_frame`, a real EXE spinning to the frame end).
- `schway__timers_dots` and the horizontal-blank step feed counter 0 on the dot clock and counter 1 on the horizontal blank through the same span function that applies a tick span; they raise in the push and give the scheduler no deadline, which also removes a deadline such a counter would have invented from the system-clock horizon.
- An access to a counter whose MODE waits on a signal the core does not make yet (counter 0 on the dot clock or with sync on, counter 1 on the horizontal blank) calls `schway__bus_unbuilt` with the timer device and is never a fault; the counter stays where it is. `core.bus_unbuilt` still sees no timer address unbuilt at power-on.
- `tests/prop.h` (54 lines) and `tests/timers/ref.c` (a per-cycle model written from the public timing rules, sharing nothing with `src/timers.c`) give `timers.reference.random`: 20,000 operations of writes, reads, acknowledgements, blank and dot pushes and 0 to 300 cycle gaps, comparing every COUNT, MODE and I_STAT bit 4 to 6, failing unless all ran. Seed: `7154F4B1E5C0DE05`.
- `docs/hardware/timers.md` has the synchronisation and beam-source sections and the integer dot accumulator the GPU phase must use (`dots += (phase + line_clocks) / div`, never a tick times the numerator).

## Task Commits

Each task was committed atomically:

1. **Task 1: Tracer - Timer 1 in sync mode 1 resets at the vertical blank the frame end pushes** - `dbbf113` (feat)
2. **Task 2: Every sync mode, the fed and unfed sources, and the unbuilt count** - `d9721e7` (feat)
3. **Task 3: The property harness and the naive per-cycle reference oracle** - `6acaaf0` (test)

**Plan metadata:** the docs commit that carries this summary (docs: complete plan).

The tracer gate: `HUMAN_VERIFY_MODE` is end-of-phase and the tracer's verify is automated only, so `timers.sync.t1_vblank_frame` was re-run end to end (exit 0, and failing under a mutation that removes the reset) before expansion went on.

## Files Created/Modified

- `src/timers.c` - blank levels and edges, the pause derivation, the beam feeds, the unbuilt rule, no deadline for beam-clocked counters, the state walk of the levels
- `src/timers.h` - the `blank` field and its comment
- `tests/prop.h` - the seeded splitmix64 harness
- `tests/timers/ref.c`, `tests/timers/ref.h` - the per-cycle reference and its interface
- `tests/timers/timer_tests.c`, `tests/timers/tests.cmake`, `tests/timers/inventory.txt` - eight new cases (22 `timers.*` tests in all)
- `docs/hardware/timers.md` - synchronisation and beam-source sections, state and sources brought up to date

## Decisions Made

See `key-decisions` above. The model's choices where the public page is silent (mode 3 written inside a blank, the hold running while paused, a blank reset with no hold) are written on the docs page and pinned by the tests and the oracle; they are not hardware claims.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing critical] A counter on the dot clock or horizontal blank had a deadline**
- **Found during:** Task 2 (designing the beam feeds)
- **Issue:** `schway__timers_next` computed an edge tick from the system-clock horizon for a counter whose source is the dot clock or the horizontal blank, so the scheduler would have stopped at an event that raises nothing and the counter's real raise (in the push) could be doubled or mistimed.
- **Fix:** such counters give no deadline; the raise happens in the push (`beam_feed`).
- **Files modified:** src/timers.c
- **Verification:** `timers.source.dot_feed` asserts the scheduler is left with no deadline after the raise; `timers.reference.random` (exit 0)
- **Committed in:** d9721e7

**2. [Rule 3 - Blocking] The blank levels needed a state field**
- **Found during:** Task 1
- **Issue:** a MODE write derives the pause from the current blank level and a loaded state must give the same answer, but the level lived nowhere. The plan names no state change.
- **Fix:** one `u32` field `timers.blank` (0 to 3) after the counters in the timers walk; the state format was already bumped once in this phase and no tests pin the timers walk's size, so no format bump was needed. `core.state.*` and `timers.state.cadence` pass.
- **Files modified:** src/timers.c, src/timers.h
- **Verification:** `ctest --test-dir build/ci -R '^(core\.state\.|timers\.state\.cadence)' --no-tests=error` (exit 0, inside the full ci workflow)
- **Committed in:** dbbf113

**3. [Rule 3 - Blocking] A small header for the reference**
- **Found during:** Task 3
- **Issue:** the test file needs the reference's declarations and the plan's file list names no header for them.
- **Fix:** `tests/timers/ref.h` (23 lines), included only by the two timers test files.
- **Files modified:** tests/timers/ref.h (new)
- **Verification:** `cmake --workflow --preset ci` (exit 0, 312 tests)
- **Committed in:** 6acaaf0

---

**Total deviations:** 3 auto-fixed (1 Rule 2, 2 Rule 3)
**Impact on plan:** all three are needed by the plan's own truths (no spurious deadline, a MODE write that derives `paused` from the current level, a reference the test can call). No scope creep.

**Size note:** `tests/timers/ref.c` is 190 lines with its comments and one-statement-per-line braces, against the plan's "about 100"; its logic is about 100 statements. Not trimmed, since compacting it would only hurt reading.

## Issues Encountered

- Two of the new hand-written script expectations were wrong at first (a paused counter's value after a second blank in the mode 0 table, and the count of horizontal blanks to reach a target of 3 with the two hold steps); both were the test's arithmetic, not the model, and the oracle agreed with the model throughout.
- The reference test passed on its first run, so it was mutation-checked outside the repository's history: seven one-line mutations of `src/timers.c` (reset hold 1 to 2, write hold 2 to 1, the end-of-blank pause, the mode 3 release, the target period, one horizontal-blank step doubled, the prescaler phase) each made `timers.reference.random` exit 1 within 1,000 operations; the file was restored and the test passes (exit 0).
- Not runnable locally: the asan and be32 lanes need clang-21 and the nofp preset needs GCC (this host has AppleClang); hosted CI runs them. The new test files use fixed-width unsigned arithmetic only and compare values, not padding. The perf lane (valgrind) is not available here; nothing on the CPU hot path changed.

## Triage

None requested by the plan.

## Known Stubs

None. (`schway__timers_dots`, `_hblank` and `_vblank` are no longer empty; nothing in the core pushes dots or horizontal blanks until the GPU phase, by design, and the footer counts the accesses that depend on them.)

## Threat Flags

None - no network, auth, file or trust-boundary surface was added; a hostile state's `timers.blank` is bounded to 0 to 3 by the state walk.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- `tests/prop.h` is ready for the GTE property test (`prop_below`, `prop_edge32`).
- The `timers_lines` fixture can use counter 2 on the system clock over 8 and counter 1 against the frame end's vertical blank as the code stands.
- The GPU phase feeds `schway__timers_hblank`, `schway__timers_vblank` (with a real width) and `schway__timers_dots`, and then drops the `schway__bus_unbuilt` call in `schway__timers_io`.

## Self-Check: PASSED

- Created files exist: `tests/prop.h`, `tests/timers/ref.c`, `tests/timers/ref.h`.
- Commits `dbbf113`, `d9721e7` and `6acaaf0` are ancestors of HEAD.
- `cmake --workflow --preset ci` (exit 0, 312 tests), `cmake --workflow --preset dev` (exit 0, 312 tests), `cmake --workflow --preset hygiene` (exit 0); `ctest --test-dir build/ci -L '^timers$' --no-tests=error` (exit 0, 22 tests); `ctest --test-dir build/ci -R '^core\.bus_unbuilt$' --no-tests=error` (exit 0).
- Acceptance greps: `grep -q 'SCHWAY__DEV_TIMER' src/timers.c && ! grep -q 'schway__fault_latch' src/timers.c` (exit 0); `grep -q 'splitmix64' tests/prop.h && ! grep -qE '\b(float|double)\b' tests/prop.h tests/timers/ref.c` (exit 0); the planning-identifier grep over `tests/prop.h tests/timers/ref.c` (exit 1, no match).

---
*Phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers*
*Completed: 2026-11-10*
