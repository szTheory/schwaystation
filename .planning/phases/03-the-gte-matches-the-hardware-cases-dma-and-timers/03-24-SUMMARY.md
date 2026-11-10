---
phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
plan: 24
subsystem: core-tests
tags:
  - split-property
  - timers
  - dma
  - determinism
  - state-bytes

requires:
  - phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
    provides: the unit engine with its carried CPU hold (plans 08 and 14), the lazy root counters and their deadlines (plans 09 and 11), the scheduler spine and the stall carry (plan 03)
provides:
  - core.run.split_devices, the split property with Timer 2, Timer 0 and a 60,000-word OTC unit armed
  - a split-by-target helper that stays valid with a program loaded, where a run stops on the instruction boundary at or after its budget
  - a state comparison that prints the first differing byte offset and the state field it belongs to
affects:
  - phase-4 GPU, phase-5 SPU and MDEC, phase-6 CD-ROM (a device that keeps time joins this case by one arm flag)

actuals:
  tokens: 3535
  tasks: 2
  commits: 2

plan_head_before: eb4f43d885ba43932331340ab31400f675dece64
plan_head_after: 1793099825a675d0f47a2766370281c5bdc26811
commits: 2

tech-stack:
  added: []
  patterns:
    - "a split is made by tick targets, not by lengths: run_to(split, n) then run_to(split, n + m) against run_to(whole, n + m), so the same instruction boundaries are passed whatever the cut"
    - "a probe instance armed the same way gives the ticks to cut at (the first timer deadline, the unit's end, a later deadline), so the test never hard-codes a figure the model could move"

key-files:
  created: []
  modified:
    - tests/core/test_run.c
    - tests/core/tests.cmake
    - tests/core/inventory.txt

key-decisions:
  - "With a program loaded a run stops on the first instruction boundary at or after its budget, so the split case compares run_to targets and not run lengths; the plain equivalence case's run_cycles would give a different total on a cut inside an instruction"
  - "The program-loaded runs are made in NTSC-U and PAL only, and the runs of several million cycles in NTSC-U alone: the two NTSC regions share their frame constants and the saved state's CRC-32 over about 3 MB per comparison is most of the case's time (2.2 s native)"
  - "The no-program runs go through every region and every split of the plain equivalence case, where a run stops exactly on the tick asked for, so a cut exactly on a timer deadline or on the unit's end tick is exact there"

requirements-completed: []

coverage:
  - id: D1
    description: "A run split once with Timer 2 on system clock over 8 interrupting at FFFFh in repeat mode, polled and acknowledged by a program, gives the unsplit run's state bytes, frame, ticks and audio, and the program saw the interrupt (status bit 6) before the run ended"
    requirement: "GTE-03"
    verification:
      - kind: unit
        ref: "tests/core/test_run.c#test_split_devices (core.run.split_devices), first check_split"
        status: pass
    human_judgment: false
  - id: D2
    description: "With Timer 0 counting to a target of 1000 and a 60,000-word OTC unit in flight as well, every split of the plain equivalence case, with and without the program loaded and in every region, gives the unsplit run's state bytes, frame, ticks and audio; the program saw status bits 3, 4 and 6"
    requirement: "GTE-02"
    verification:
      - kind: unit
        ref: "tests/core/test_run.c#test_split_devices (core.run.split_devices), the splits table"
        status: pass
    human_judgment: false
  - id: D3
    description: "Cuts in the middle of the OTC unit, one tick either side of its end, and one tick either side of the first Timer 0 deadline and of a later deadline of a probe run give the unsplit run's state bytes, with and without the program, with all three devices armed and with Timer 2 alone"
    requirement: "GTE-02"
    verification:
      - kind: unit
        ref: "tests/core/test_run.c#check_edges (core.run.split_devices)"
        status: pass
    human_judgment: false
  - id: D4
    description: "The comparison names the first differing state byte and its state field; a scratch mutant of src/cpu.c that drops the rest of a DMA hold at the end of a budget fails the case at the first cut and names the header and ticks difference, then the source file was restored"
    requirement: "GTE-02"
    verification:
      - kind: other
        ref: "scratch mutation of src/cpu.c followed by build/ci/tests/schwaystation-test-run split_devices, non-zero exit; git checkout -- src/cpu.c restored it; cmake --workflow --preset ci exit 0 after"
        status: pass
    human_judgment: false

duration: 6min
completed: 2026-11-10
status: complete
---

# Phase 3 Plan 24: The split property with the devices armed Summary

**`core.run.split_devices` runs a polling program with Timer 2 (system clock / 8, FFFFh repeat), Timer 0 (target 1000) and a 60,000-word OTC unit armed, and shows that splits at the plain equivalence case's points, in the middle of the unit and one tick either side of its end and of timer deadlines give the unsplit run's state bytes, frame, ticks and audio; it passed first time, with no core change.**

## Performance

- **Duration:** 6 min
- **Started:** 2026-11-10T07:36:59Z
- **Completed:** 2026-11-10T07:42:40Z
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments

- The case arms the devices through bus writes at tick 0 and loads a 9-instruction PS-EXE that reads I_STAT, acknowledges every bit, reads the Timer 0 and Timer 2 counts and keeps what it saw in t4 (the status bits, ORed) and t5 and t6 (the counts, summed). The registers then depend on the exact tick of every access, and the acknowledgement lets each counter raise its next interrupt. A run long enough for every counter to fire also asserts that the program saw status bits 6, 4 and 3, so the comparison is never of two idle machines.
- With no program loaded the same arming is compared too (I_STAT stays set, so only the first interrupts fire); a run there stops exactly on the tick asked for, which makes a cut on a timer deadline or the unit's end tick exact.
- `check_edges` takes the cut ticks from a probe armed the same way: the first Timer 0 deadline from `schway__timers_next`, the unit's end from `dma.busy_until`, and a later deadline found by running the probe to 100,000 cycles and stepping until a counter has one armed. Each is cut at, one before and one after, and the middle of the unit is cut too, where the CPU is held and the rest of the hold stays in the state.
- On a difference `same_state` prints the first differing byte offset and the state field it falls in, which names the device whose state depends on the cut.
- A scratch mutant of `src/cpu.c` that drops the remainder of a DMA hold at the end of a budget fails the case at its first cut (ticks 861 against 768, the state differing in the header); `src/cpu.c` was restored. Two other scratch mutants (a stale timer view in the save, and no deadline stepping in the no-program advance) pass, because the state keeps no timestamps and the timer view is closed-form in the tick: the first is `core.state.*` and the timers tests' property, the second changes only when an event runs, not what it does.

## Task Commits

1. **Task 1: Tracer - one split with Timer 2 interrupting gives the unsplit run's state bytes** - `6636d94` (test)
2. **Task 2: Every split point with Timer 0 on a target and a DMA unit across the split** - `1793099` (test)

The tracer gate ran in the default end-of-phase mode with an automated-only verify: `cmake --workflow --preset ci` and `ctest -R '^core\.run\.split_(equivalence|devices)$'` were run, passed, and the expansion task went ahead.

**Plan metadata:** the docs commit that carries this summary, STATE.md and ROADMAP.md.

## Files Created/Modified

- `tests/core/test_run.c` - `split_devices` with the program, the arming, `run_to`, `same_state`, `check_split` and `check_edges`; 310 lines added
- `tests/core/tests.cmake` - the `core.run.split_devices` registration with its `Fault prevented:` comment (labels `core portable`, TIMEOUT 30)
- `tests/core/inventory.txt` - the new line

## Decisions Made

See `key-decisions` above.

## Deviations from Plan

None - plan executed exactly as written. No file outside the three test files changed, and no stop-and-triage arose: every split equals the unsplit run.

## Triage

No split differed from the unsplit run, so no device needed naming and no fix to `src/dma.c` or `src/timers.c` was made. The comparison is not loosened: it is a byte compare of the whole saved state, then the frame, ticks and audio.

## Issues Encountered

- The case takes 2.2 s natively (most of it the CRC-32 over the 3 MB state in every save, two saves per comparison, about 150 comparisons). TIMEOUT is the plan's 30 s. Dropping the program-loaded runs of the NTSC-J region (same frame constants as NTSC-U) and keeping the multi-million-cycle ones in NTSC-U only brought it from 3.9 s to 2.2 s.
- The `ci` workflow's JUnit check counts 330 tests once each.

## Not runnable locally

The `asan`, `be32` and `wasm32` lanes need clang-21 or wasmtime that this host lacks; hosted CI runs them, and `core.run.split_devices` is in the `portable` set they select. Evidence run locally: `cmake --workflow --preset ci` exit 0; `ctest --test-dir build/ci -R '^core\.run\.split_(equivalence|devices)$' --no-tests=error` exit 0; `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake` exit 0. The only risk on the slow lanes is the 30 s timeout against 2.2 s native.

## Known Stubs

None. The case drives the stand-in DMA sink that earlier plans documented; this plan adds no stub.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- The split property now covers the devices of this phase; a device that keeps time in a later phase adds one arm flag and its edge ticks to `check_edges`.
- Plans 03-15 and 03-16 are done; the remaining plans of the phase share no file with this one.

---
*Phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers*
*Completed: 2026-11-10*

## Self-Check: PASSED
