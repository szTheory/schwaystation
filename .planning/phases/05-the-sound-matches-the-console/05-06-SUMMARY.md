---
phase: 05-the-sound-matches-the-console
plan: "06"
subsystem: audio
tags: [SPU, BRR, Gaussian interpolation, C, CMake]
requires:
  - phase: 05-02
    provides: Scheduled SPU events and instance-owned audio state
  - phase: 05-20
    provides: SPU RAM access with IRQA visibility and bounded transfer state
provides:
  - One BRR voice decode/interpolation primitive with carried history
  - Independently checked BRR filters, signed nibbles, shifts and all 256 Gaussian phases
affects: [05-07, 05-08, 05-11, 05-14, 05-18, 05-22]
commits: 3
plan_head_before: e4429c3aa19fdfc47921cdb88dc65ef4c3fee99b
plan_head_after: 5c5c967ea50e32ac8540e85c579db643e0c78523
actuals:
  tokens: 14944
  tasks: 1
  commits: 3
tech-stack:
  added: []
  patterns:
    - Fixed-point BRR decoding with explicit sign extension and arithmetic shifts
    - Per-product Gaussian interpolation truncation against an independent scalar oracle
key-files:
  created:
    - src/spu_voice.c
    - src/spu_tables.inc
    - tests/spu/voices.c
  modified:
    - src/spu.c
    - CMakeLists.txt
    - tests/spu/tests.cmake
    - tests/spu/inventory.txt
key-decisions:
  - "Reuse the already serialized per-voice registers, history and sample counter; no machine-state fields were added."
  - "Move 24-voice scheduling, loop/ENDX, PMON and final stereo mixing to plan 05-22 to keep this slice within the plan size limit."
patterns-established:
  - "BRR reference checks independently compute filters, signed nibbles, shifts and interpolation phases."
  - "Voice decode state stays embedded in the SPU instance and uses the existing save-state visitor fields."
requirements-completed: []
coverage:
  - id: SOUND-01-BRR-PRIMITIVE
    description: One real BRR block decodes and interpolates into independently checked voice samples.
    requirement: SOUND-01
    verification:
      - kind: unit
        ref: "cmake --preset ci && cmake --build --preset ci --target schwaystation-test-spu-voices && ctest --test-dir build/ci -R '^spu\\.voices$' --no-tests=error --output-on-failure"
        status: pass
      - kind: unit
        ref: "cmake --build --preset ci --target schwaystation-test-state && ctest --test-dir build/ci -R '^core\\.state\\.' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
duration: 43min
completed: 2026-11-14
status: complete
---

# Phase 5 Plan 06: BRR Voice Interpolation Summary

**A single-voice BRR decoder now matches independent integer references across filters, shifts, history boundaries and all Gaussian interpolation phases.**

## Performance

- **Duration:** 43 minutes, including the required plan split and state/roadmap closeout
- **Started:** 2026-11-14T01:40:52Z
- **Completed:** 2026-11-14T02:23:53Z
- **Tasks:** 1
- **Files modified:** 7 implementation/test files; planning closeout also updated the roadmap, plans 05-07/08/11/14 and added plan 05-22

## Accomplishments

- Added one real scheduled BRR voice path with explicit signed-nibble handling, prediction history and integer Gaussian interpolation.
- Added an independent scalar reference covering all filters, nibble values, shifts, block history and interpolation phases.
- Kept the decoder state within fields already serialized by the SPU visitor; all eight existing save-state cases still pass.
- Moved 24-voice ordering, loop/ENDX, PMON and stereo output to follow-up plan 05-22. SOUND-01 remains Pending until Phase 5 verification.

## Task Commits

Task 1 used separate TDD commits: RED `d2baed7`, expanded RED `1fa2054`, and GREEN implementation `5c5c967`.

## Files Created/Modified

- `src/spu_voice.c` and `src/spu_tables.inc` — fixed-point BRR decoding and the cited Gaussian coefficient table.
- `tests/spu/voices.c` — independent decode and interpolation reference checks.
- `CMakeLists.txt`, `tests/spu/tests.cmake`, `tests/spu/inventory.txt` — test target and inventory registration.
- `src/spu.c` — initialize and schedule the one-voice tracer path.
- `.planning/phases/05-the-sound-matches-the-console/05-22-PLAN.md` — follow-up plan for full voice playback and mixing.

## Decisions Made

- Reused the existing per-voice `reg[8]`, `history[3]`, and serialized `sample_counter`; no save-state schema fields were added.
- Kept full voice sequencing and stereo mixing out of this plan. Plan 05-22 follows envelope plan 05-07 and precedes plans 05-08 and 05-11.

## TDD Evidence

- Before implementation, the `schwaystation-test-spu-voices` target exited 1 because BRR samples were not produced. `gsd_run check tdd-red-evidence` classified the failure as `RED_EVIDENCE_OK`.
- Expanded filter/nibble/shift and 256-phase checks also exited 1 on missing decoder behavior and were committed separately before production code.
- The task verification command in `coverage` passed with exactly the `spu.voices` test selected.

## Verification

- `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-spu-voices && ctest --test-dir build/ci -R '^spu\.voices$' --no-tests=error --output-on-failure` — exit 0; focused BRR test passed.
- `cmake --build --preset ci --target schwaystation-test-state && ctest --test-dir build/ci -R '^core\.state\.' --no-tests=error --output-on-failure` — exit 0; all eight existing state tests passed.

## Deviations from Plan

### Scope split

The original second task combined 24-voice ordering, loop/end flags, PMON and stereo mixing. To honor the 30-minute plan limit, those behaviors now live in plan 05-22. Plans 05-08 and 05-11 depend on 05-22 because their noise and guest-tone paths consume mixed voice output. Plan 05-14 explicitly covers the current voice state keys and any future fields from 05-22. Plan 05-21 remains a separate Wave 7 plan and is sequenced after this plan.

### Test expectation correction

The scalar integer reference produced 4206 for the initial sample vector, not the hand-calculated 4204. The test expectation was corrected and the focused suite passed.

## Next Phase Readiness

Plan 05-07 can continue after the Wave 7 SPU work. Plan 05-22 owns the remaining scheduled playback path before noise, guest-tone, state-continuation and audio-delivery plans proceed. No user action is needed. SOUND-01 and SOUND-02 remain Pending until Phase 5 verification.

## Self-Check: PASSED

- The summary and follow-up plan files exist.
- RED commits `d2baed7` and `1fa2054`, and GREEN commit `5c5c967`, are ancestors of HEAD.
