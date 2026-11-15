---
phase: 05-the-sound-matches-the-console
plan: "02"
subsystem: audio
tags: [C17, SPU, scheduler, PCM, CMake]
requires:
  - phase: 05-the-sound-matches-the-console
    provides: Public PCM ring and audio drain API from plan 05-01
provides:
  - Fixed-size instance-owned SPU state with 512 KiB RAM
  - Scheduled stereo sample production every 768 cycles through the public PCM ring
affects: [05-the-sound-matches-the-console]
actuals:
  tokens: 4441.25
  tasks: 2
  commits: 4
tech-stack:
  added: []
  patterns:
    - Device events produce samples through the existing bounded PCM ring
    - Scheduler catches up every SPU sample deadline crossed by bounded CPU or DMA work
key-files:
  created: [src/spu.h, src/spu.c]
  modified: [CMakeLists.txt, src/frame.c, src/instance.c, src/internal.h, src/sched.c, tests/spu/model.c]
key-decisions:
  - "SPU sample production is owned by the scheduler; frame completion no longer pads silence."
  - "All mutable SPU machine state is embedded per instance and uses fixed-width storage."
patterns-established:
  - "Scheduled chip output is delivered through the existing public PCM ring without a host audio device."
requirements-completed: [SOUND-01, SOUND-02]
coverage:
  - id: D1
    description: "A scheduled SPU tick emits one stereo pair per 768 cycles from fixed-size, instance-owned state."
    requirement: SOUND-01
    verification:
      - kind: unit
        ref: "tests/spu/model.c#SPU event and instance ownership checks; spu.model (exit 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: "CPU and no-content runs deliver scheduler-produced PCM with split-run equivalence and bounded deadline catch-up."
    requirement: SOUND-02
    verification:
      - kind: unit
        ref: "tests/spu/model.c#scheduler deadline checks; spu.model, core.audio.ring, core.audio.pairs (exit 0)"
        status: pass
    human_judgment: false
metrics:
  duration: 19min
  completed: 2026-11-13
status: complete
---

# Phase 5 Plan 2: Trace one SPU Event Summary

**A fixed-size, instance-owned SPU now feeds one stereo pair per scheduled 768-cycle tick into the public PCM ring.**

## Performance

- **Duration:** 19 min
- **Started:** 2026-11-13T21:50:49Z
- **Completed:** 2026-11-13T22:09:41Z
- **Tasks:** 2
- **Files modified:** 8

## Accomplishments

- Added private SPU state with 512 KiB RAM and fixed-size storage for the later voice, FIFO, noise, capture, reverb, and sweep work.
- Added little-endian SPU RAM helpers with centralized IRQA checks and a bounded PCM pair producer using the existing ring's drop behavior.
- Wired SPU deadlines into the scheduler so CPU and DMA overshoot, split public runs, mute, and disable preserve sample timing without frame-end silence padding.

## Task Commits

1. **Task 1 test:** `fb23137` — failing instance-owned SPU storage check.
2. **Task 1 implementation:** `9141a99` — instance-owned SPU sample tick.
3. **Task 2 test:** `aeb6d64` — failing SPU scheduler deadline checks.
4. **Task 2 implementation:** `f47ed51` — scheduled samples from both run paths.

## Files Created/Modified

- `src/spu.h`, `src/spu.c` — private SPU model, RAM access, reset, event timing, and PCM enqueue.
- `src/internal.h`, `src/instance.c` — per-instance embedding and power-on reset.
- `CMakeLists.txt` — SPU object target and shipping library linkage.
- `src/sched.c`, `src/frame.c` — scheduled SPU dispatch and removal of frame-end silent padding.
- `tests/spu/model.c` — event, state ownership, overflow, scheduler deadline, and split-run checks.

## Decisions Made

- SPU sample production belongs to the device scheduler and uses the existing public PCM ring.
- SPU storage is fixed-size and belongs to each emulator instance.

## TDD Gate Compliance

- **Task 1 RED:** `spu.model` failed because the new instance-owned RAM size was initially zero instead of 524288 bytes. RED evidence classifier passed; test commit `fb23137`.
- **Task 1 GREEN:** the required configure/build/CTest command passed (`spu.model`, exit 0); implementation commit `9141a99`.
- **Task 2 RED:** `spu.model` failed because the public run crossed the SPU deadline while `spu_next` remained at cycle 768. RED evidence classifier passed; test commit `aeb6d64`.
- **Task 2 GREEN:** the required configure/build/CTest command passed (`spu.model`, `core.audio.ring`, and `core.audio.pairs`, exit 0); implementation commit `f47ed51`.
- The tracer task's focused `spu.model` verification was rerun after expansion and passed (exit 0).

## Deviations from Plan

None — plan executed as written.

## Issues Encountered

The expected RED runs failed on their newly added behavior assertions while building successfully; both were resolved by the corresponding planned implementation changes. No hosted lane applies to this plan.

## Known Stubs

- `src/spu.c:76` — a reset chip emits a zero stereo pair until planned voice and reverb synthesis is implemented in later Phase 5 plans. This preserves the scheduled event behavior delivered here.

## User Setup Required

None — no external service configuration required.

## Next Phase Readiness

The scheduled event, instance storage, and public PCM delivery path are ready for the subsequent register, voice, and reverb plans in Phase 5. The reset sample remains silent until those synthesis paths are implemented.

---
*Phase: 05-the-sound-matches-the-console*
*Completed: 2026-11-13*

## Self-Check: PASSED

- Summary file exists at the planned path.
- All four task commits (`fb23137`, `9141a99`, `aeb6d64`, `f47ed51`) are ancestors of HEAD.
