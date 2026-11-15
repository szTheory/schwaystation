---
phase: 05-the-sound-matches-the-console
plan: "10"
subsystem: audio
tags: [SPU, reverb, Q15, CTest]
requires:
  - phase: 05-02
    provides: SPU register, RAM, IRQ, and serialized instance state interfaces
  - phase: 05-09
    provides: Reverb FIR and alternating channel timing
provides:
  - Register-driven IIR, comb, and all-pass feedback into the SPU work buffer
  - Focused preset, arithmetic-boundary, RAM-access, and save/load regressions
  - Hardware notes for the implemented reverb path and unresolved rounding evidence
affects: [phase-05-verification]
actuals:
  tokens: 10318
  tasks: 2
  commits: 4
plan_head_before: 1f6b80c29cebd8d04882c448ce32b8033f58cec8
plan_head_after: 46db53c9fa88628b535ecc5ed1066c8a013f2932
tech-stack:
  added: []
  patterns:
    - Deterministic signed Q15 feedback with explicit saturating 16-bit stages
    - ESA-relative work-buffer accesses through shared little-endian RAM helpers
    - Local preset impulse digests used as regression baselines
key-files:
  created: []
  modified:
    - src/spu.c
    - src/spu_reverb.c
    - tests/spu/reverb.c
    - docs/hardware/spu.md
    - .planning/preparation/PSX-SPU-CDROM-AND-MDEC.md
key-decisions:
  - "Keep per-stage rounding and saturation placement documented as unresolved; the integer order is a deterministic baseline."
  - "Keep preset impulse digests as local regression values, not hardware waveform claims."
  - "Leave SOUND-01 and SOUND-02 pending until final Phase 5 verification."
requirements-completed: []
coverage:
  - id: D1
    description: Guest reverb registers drive IIR, comb, APF, RAM, and audible wet PCM behavior.
    requirement: SOUND-01
    verification:
      - kind: unit
        ref: cmake --preset ci && cmake --build --preset ci --target schwaystation-test-spu-reverb && ctest --test-dir build/ci -R '^spu\.reverb$' --no-tests=error --output-on-failure
        status: pass
    human_judgment: false
  - id: D2
    description: ESA-relative address wrapping, inhibited-write read substitution, and IRQA checks are covered.
    requirement: SOUND-01
    verification:
      - kind: unit
        ref: ctest spu.reverb in build/ci
        status: pass
    human_judgment: false
  - id: D3
    description: Ten documented preset vectors and measured arithmetic boundaries have deterministic regressions.
    requirement: SOUND-01
    verification:
      - kind: unit
        ref: ctest spu.reverb in build/ci
        status: pass
    human_judgment: false
  - id: D4
    description: Active feedback, FIR history, continued public PCM, and saved state remain identical after load.
    requirement: SOUND-01
    verification:
      - kind: unit
        ref: ctest spu.reverb in build/ci
        status: pass
    human_judgment: false
duration: 31min
completed: 2026-11-14
status: complete
---

# Phase 5 Plan 10: SPU Reverb Feedback Through Measured Saturation Constraints Summary

**The SPU reverb register path now drives an ESA-wrapped feedback buffer and interpolated wet PCM return.**

## Performance

- **Duration:** 31 min
- **Started:** 2026-11-14T07:51:00Z
- **Completed:** 2026-11-14T08:22:23Z
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments

- Implemented same-side and cross-side IIR, four comb taps, two APF stages, signed Q15 scaling, saturation, ESA-relative wrapping, and IRQA-checked RAM access.
- Added regressions for all nine active presets and Reverb off, negative coefficients, intermediate saturation, write-disable reads, ESA cursor reset, and active feedback save/load continuation.
- Updated the hardware guide and preparation notes to distinguish measured behavior, documented formulas, and local regression digests.

## Task Commits

- Task 1 RED: `8178248` test(05-10): add failing reverb feedback RAM regression
- Task 1 GREEN: `fd65519` feat(05-10): implement register-driven SPU reverb feedback
- Task 2 RED: `29723e8` test(05-10): cover negative IIR coefficient behavior
- Task 2 GREEN: `46db53c` feat(05-10): complete reverb regression coverage

## Files Created/Modified

- src/spu.c — resets the current work-buffer cursor when ESA changes.
- src/spu_reverb.c — applies the register-driven feedback stages and their RAM access rules.
- tests/spu/reverb.c — covers presets, arithmetic limits, disabled writes, IRQs, and active-state continuation.
- docs/hardware/spu.md — describes the implemented reverb signal and memory paths.
- .planning/preparation/PSX-SPU-CDROM-AND-MDEC.md — records current evidence and unresolved hardware details.

## Decisions Made

- Keep the reverb calculation deterministic while treating exact per-stage rounding and saturation positions as unresolved.
- Use preset impulse digests as local implementation regressions; do not present them as captured hardware output.
- SOUND-01 and SOUND-02 remain pending until final Phase 5 verification.

## Deviations from Plan

None. The write-disable coverage verifies that each inhibited state write becomes an IRQA-checked RAM read while APF output reads continue contributing to the return.

## TDD Gate Compliance

- Task 1 RED: build and CTest ran target spu.reverb and exited 8 on the intended feedback-RAM and IRQA assertions. The record at build/ci/05-10-task1-red-evidence.json classified as RED_EVIDENCE_OK (classifier exit 0).
- Task 1 GREEN: the focused spu.reverb command passed with 1/1 tests.
- Task 2 RED: build and CTest ran target spu.reverb and exited 8 on the intended vIIR=-8000h completed-result assertion. The record at build/ci/05-10-task2-red-evidence.json classified as RED_EVIDENCE_OK (classifier exit 0).
- Task 2 GREEN: the focused spu.reverb command passed with 1/1 tests, including the preset and disabled-write cases.
- Post-commit tracer check: the focused spu.reverb command passed with 1/1 tests after commit 46db53c.
- No separate refactor pass was needed after GREEN.

## Next Phase Readiness

The reverb slice is ready for final Phase 5 verification. This plan does not complete SOUND-01 or SOUND-02 globally, and does not start Phase 6 or ship Phase 5.

## Commands and Evidence

Task 1 RED command (exit status 8 as expected; one focused assertion failure):

```text
cmake --preset ci && cmake --build --preset ci --target schwaystation-test-spu-reverb && ctest --test-dir build/ci -R '^spu\.reverb$' --no-tests=error --output-on-failure --output-junit build/ci/05-10-task1-red.junit.xml
```

Task 1 RED classifier command (exit status 0; verdict RED_EVIDENCE_OK):

```text
gsd_run check tdd-red-evidence build/ci/05-10-task1-red-evidence.json --raw
```

Task 2 RED command (exit status 8 as expected; one focused assertion failure):

```text
cmake --preset ci && cmake --build --preset ci --target schwaystation-test-spu-reverb && ctest --test-dir build/ci -R '^spu\.reverb$' --no-tests=error --output-on-failure --output-junit build/ci/05-10-task2-red.junit.xml
```

Task 2 RED classifier command (exit status 0; verdict RED_EVIDENCE_OK):

```text
gsd_run check tdd-red-evidence build/ci/05-10-task2-red-evidence.json --raw
```

Focused GREEN and post-commit tracer command (exit status 0; 1/1 CTest passed):

```text
cmake --preset ci && cmake --build --preset ci --target schwaystation-test-spu-reverb && ctest --test-dir build/ci -R '^spu\.reverb$' --no-tests=error --output-on-failure
```

`git diff --check` passed before the task-2 implementation commit. No hosted run was required for this deterministic local unit slice.

## Self-Check: PASSED

The summary file exists, and all four task commits are ancestors of HEAD.
