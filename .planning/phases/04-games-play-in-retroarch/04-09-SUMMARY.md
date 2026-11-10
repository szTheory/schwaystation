---
phase: 04-games-play-in-retroarch
plan: "09"
subsystem: gpu-timing
tags: [C, GPU, beam, timers, CMake, CTest]
requires:
  - phase: 03
    provides: CPU scheduler, timers, DMA, and serialized instance state
  - phase: 04-08
    provides: GPU command path and raster span implementation
provides:
  - Ratio-carried lazy beam timing and scheduled Timer0 dot-target deadlines
  - Field-wrap frame completion with an independent reset-resistant run deadline
affects: [04-10, 04-11, GPU, timers, scheduler, state]
actuals:
  tokens: 18687
  tasks: 2
  commits: 5
commits: 5
plan_head_before: 38be52c6269e6352f39bbbcd8ae2b5da242fe2ad
plan_head_after: ac8f78dd60cef45729fb247f6379add61dcb5fa0
tech-stack:
  added: []
  patterns:
    - Instance-owned rational beam carry with lazy deadline catch-up
    - Field completion and per-call emulated-time bound are independent
key-files:
  created:
    - tests/gpu/beam.c
  modified:
    - src/gpu.c
    - src/timers.c
    - src/sched.c
    - src/frame.c
    - src/state.c
    - tests/gpu/tests.cmake
    - tests/gpu/inventory.txt
decisions:
  - Timer, beam, DMA, and SIO events retain deterministic same-tick ordering.
  - The no-content test-card frame path remains separate from loaded-content field timing.
metrics:
  duration: 64min
  completed: 2026-11-10
status: complete
requirements-completed: []
coverage:
  - id: D1
    description: Timer0 dot-target IRQ fires at its derived mid-line tick and whole/split execution agrees.
    requirement: GAME-02
    verification:
      - kind: unit
        ref: tests/gpu/beam.c#midline_dot_irq; gpu.beam.midline_dot_irq
        status: pass
    human_judgment: false
  - id: D2
    description: Beam field wrap remains bounded across display ranges and repeated video resets, with equal-tick ordering and malformed state controls.
    requirement: GAME-02
    verification:
      - kind: unit
        ref: tests/gpu/beam.c#field_edges,public_field_wrap,reset_call_bound,equal_timer_beam_dma,reject_malformed_state
        status: pass
    human_judgment: false
---

# Phase 4 Plan 9: Exact beam timing and bounded field completion

**GPU beam edges now drive timer deadlines and field completion, with rational carry and a separate reset-resistant call bound.**

## Performance

- **Duration:** 64 min
- **Started:** 2026-11-10T22:28:18Z
- **Completed:** 2026-11-10T23:32:13Z
- **Tasks:** 2
- **Files modified:** 23

## Accomplishments

- Added lazy, instance-owned beam timing with exact ratio carry, blank/line deadlines, and an independently scheduled mid-line Timer0 dot IRQ.
- Made loaded-content runs stop at field wrap while keeping the regional emulated-time bound stable across GP1 video resets; preserved the no-content test-card path.
- Covered Y-range edges, all regional profiles, split-run state, simultaneous timer/beam/DMA events, malformed state, and reset-driven hostile execution.

## Task Commits

1. **Task 1 RED: mid-line Timer0 IRQ tracer** - `9ec738b` (test)
2. **Task 1 GREEN: ratio-carried beam timing** - `5510100` (feat)
3. **Task 2 RED: field wrap and reset bounds** - `d3b6778` (test)
4. **Task 2 GREEN: bounded content runs by field wrap** - `c41b0b7` (feat)

**Plan metadata:** `ac8f78d` (docs: complete beam timing plan)

## Files Created/Modified

- `src/gpu.c`, `src/gpu.h`, `src/timers.c`, `src/timers.h`, `src/sched.c`, `src/bus.c`, `src/state.c`, `src/internal.h`, `src/cpu.c`, and `src/frame.c` - Beam/timer scheduling, state validation, event integration, and bounded frame execution.
- `tests/gpu/beam.c`, GPU and timer CMake registrations/inventories, and `tests/timers/timer_tests.c` - Exact deadline, field edge, reset, ordering, and integration coverage. The obsolete `timers.source.unfed_unbuilt` case was replaced one-for-one by `timers.source.beam_register_access`; after the fresh CMake configure the suite had 379 tests (the prior cached enumeration showed 381), and Task 2 added five cases for 384 total.
- `tests/core/sched_tests.c`, `tests/core/test_trace.c`, and `tests/timers/inventory.txt` - Updated scheduler and timer expectations for beam-driven timing.
- `docs/embedding.md`, `docs/hardware/cpu.md`, `docs/hardware/gpu.md`, `docs/hardware/timers.md`, and `.planning/todos/pending/043-timers-vestigial-code.md` - Documented timing/bounds and closed the timer portion of todo 043; DMA cleanup remains assigned to plan 11.

## Decisions Made

- Beam catch-up remains lazy and deadline-driven; it does not poll each dot or recursively schedule callbacks.
- Loaded-content field completion is driven by beam wrap, while the no-content test card keeps its existing frame behavior.
- The independent run ceiling uses the maximum regional interval and is not extended by GP1 reset; instruction/DMA overshoot stays within existing bounds.

## Deviations from Plan

None - execution stayed within the planned GPU/timer/scheduler integration. Existing tests were adjusted where their prior expectations depended on the replaced synthetic frame boundary.

## Issues Encountered

- The first integrated CI run exposed stale scheduler/timer assertions for synthetic frame IRQs and one rational frame-end rounding mismatch in a split fixture. The assertions now target beam edges and the frame-end update uses the carried rational phase; focused checks and the full workflow pass.
- The local GSD plan ledger had an unresolvable base SHA. It was corrected to the verified Task 1 parent, `38be52c6269e6352f39bbbcd8ae2b5da242fe2ad`, before measuring commits.

## Verification

- Task 1: `cmake --workflow --preset ci` — exit 0; configured suite 379/379 and 2 release checks passed.
- Task 2 RED: five new behavior/control cases against Task 1 implementation — expected nonzero; three field-wrap/reset behaviors failed and the equal-tick and malformed-state controls passed.
- Task 2 GREEN: `ctest --test-dir build/ci -R '^gpu\.beam\.' --output-on-failure` — exit 0; 6/6 passed.
- Final Task 2: `cmake --workflow --preset ci` — exit 0; 384/384 CI tests and 2 release checks passed.
- Final Task 2: `cmake --workflow --preset hygiene` — exit 0; 23 checks passed. `git diff --check` — exit 0.
- No hosted run URL was produced. No owner UAT is required.

## User Setup Required

None - no external configuration is required.

## Next Phase Readiness

The timer side of todo 043 is resolved. Plan 11 retains the DMA-side cleanup. GAME-02 remains incomplete until display scan-out and its remaining phase deliverables are implemented and verified.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-10*

## Self-Check: PASSED

- SUMMARY file exists.
- Task commits `9ec738b`, `5510100`, `d3b6778`, and `c41b0b7` are ancestors of HEAD.
