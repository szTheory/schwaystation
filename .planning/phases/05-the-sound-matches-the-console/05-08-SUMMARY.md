---
phase: "05-the-sound-matches-the-console"
plan: "08"
subsystem: audio
tags: [spu, noise, lfsr, capture, irqa, ctest]
requires:
  - phase: "05-01 through 05-07"
    provides: SPU sample tick, register, RAM, voice, envelope and IRQ interfaces
  - phase: "05-16, 05-20, 05-22"
    provides: SPU state serialization, reverb and RAM transfer behavior
provides:
  - Pinned shared noise clock schedule with one LFSR clock at most per sample
  - Envelope-scaled noise capture for voices 1 and 3, with zero Phase-5 CD inputs
  - Silent RAM fetch and capture IRQ coverage through the existing IRQA helpers
affects: [phase-5-spu, SOUND-01]
actuals:
  tokens: 5714
  tasks: 2
  commits: 4
plan_head_before: 9faa2956fcaa27de21956c53501322be6ad65b16
plan_head_after: f1d8631d635f07f1c05ac38acba9cb19857f8404
tech-stack:
  added: []
  patterns:
    - Shared LFSR advances once from a bounded eight-slot sample window
key-files:
  created:
    - src/spu_noise.c
  modified:
    - CMakeLists.txt
    - src/spu.c
    - src/spu.h
    - tests/spu/noise.c
    - tests/spu/tests.cmake
    - tests/spu/inventory.txt
    - docs/hardware/spu.md
    - .planning/preparation/PSX-SPU-CDROM-AND-MDEC.md
key-decisions: []
patterns-established:
  - Keep voice RAM fetches before noise selection so silent and noise voices still exercise IRQA.
requirements-completed: []
coverage:
  - id: D1
    description: Pinned noise slots and the shared 16-bit LFSR follow deterministic sample timing.
    requirement: SOUND-01
    verification:
      - kind: unit
        ref: tests/spu/noise.c and CTest spu.noise
        status: pass
    human_judgment: false
  - id: D2
    description: Voice 1/3 capture receives envelope-scaled noise while Phase-5 CD inputs remain zero.
    requirement: SOUND-01
    verification:
      - kind: unit
        ref: tests/spu/noise.c and CTest spu.noise
        status: pass
    human_judgment: false
  - id: D3
    description: Muted and disabled output preserves modeled fetch, capture, RAM_CTRL, and IRQ behavior.
    requirement: SOUND-01
    verification:
      - kind: unit
        ref: tests/spu/noise.c and tests/spu/model.c; CTest spu.noise and spu.model
        status: pass
    human_judgment: false
metrics:
  duration: 28min
  completed: 2026-11-14
  status: complete
---

# Phase 5 Plan 8: Trace the Pinned Noise Schedule into an Envelope-Scaled Sample Summary

**The SPU now clocks the pinned shared noise LFSR, routes it through voice envelopes, captures the full voice 1/3 signal, and preserves RAM-access IRQ checks.**

## Performance

- **Duration:** About 28 minutes; the executor start timestamp was not captured, so elapsed time is approximate.
- **Runtime/model:** Codex/OpenGSD 1.20.0, gpt-6-luna xhigh.
- **Tasks:** 2.
- **Files modified:** 9.

## Accomplishments

- Implemented the psx-spx eight-slot noise schedule, with a fixed eight-unit check per sample and no more than one LFSR clock per sample.
- Routed the shared signed noise level through the existing voice envelope and amplitude path; kept voice RAM fetches active in noise and silent modes.
- Captured zero Phase-5 CD inputs and envelope-scaled voice 1/3 samples, with ring wrap, STATX half status, and IRQA checks.
- Updated the hardware guide and preparation notes with the pinned schedule and documented capture behavior.

## TDD Evidence

- **RED:** `cmake --build --preset ci --target schwaystation-test-spu-noise && build/ci/tests/schwaystation-test-spu-noise` exited 1. TAP failed on the intended slot-0 assertion: expected LFSR `3`, observed `1`. The expanded RED run also failed the planned schedule, continuity, voice, and capture assertions.
- **GREEN:** `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-spu-noise && ctest --test-dir build/ci -R '^spu\\.noise$' --no-tests=error --output-on-failure` exited 0.
- **Tracer feedback:** Re-ran the GREEN command after the tracer commit; it exited 0.
- **Task 2 verification:** `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-spu-noise schwaystation-test-spu && ctest --test-dir build/ci -R '^spu\\.(noise|model)$' --no-tests=error --output-on-failure` exited 0 with both tests passing.
- The LFSR cycle check confirmed 65,535 clocks return to the reset seed without entering `FFFFh`.

## Task Commits

1. **Task 1: Trace the pinned noise schedule into an envelope-scaled sample**
   - `6ac0bfa` — RED test for the first scheduled slot.
   - `3a304a8` — Expanded RED assertions for the slot table, LFSR, voice mix, and capture.
   - `afb189b` — Implemented the pinned clock, noise voice selection, and capture signal.
2. **Task 2: Expand capture, silent fetch and IRQA coverage across the tick**
   - `f1d8631` — Added silent-access assertions and updated hardware documentation.

## Files Created/Modified

- `src/spu_noise.c` — Bounded slot schedule, LFSR clock, and signed noise level.
- `src/spu.c` and `src/spu.h` — Tick integration, noise selection, capture scaling, and valid serialized LFSR range.
- `CMakeLists.txt`, `tests/spu/tests.cmake`, `tests/spu/inventory.txt`, `tests/spu/noise.c` — Build and exact inventory registration plus deterministic schedule, capture, and IRQ checks.
- `docs/hardware/spu.md`, `.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md` — Pinned noise schedule and capture facts with evidence labels.

## Decisions Made

None — followed the pinned model and the plan's sample-granular ordering.

## Deviations from Plan

None — plan executed as written.

## Issues Encountered

The initial test expectation for the second LFSR clock was corrected from `6` to `7` after applying the documented parity taps to state `3`; the implementation matched the reference recurrence.

## User Setup Required

None.

## Next Phase Readiness

Plan 05-08 is complete. SOUND-01 and SOUND-02 remain Pending because this plan delivers only part of their phase-wide acceptance criteria. No later plan, Phase 6 work, or shipping was started.

---
*Phase: 05-the-sound-matches-the-console*
*Completed: 2026-11-14*

## Self-Check: PASSED

The summary file exists and all four task commits are ancestors of the current branch head.
