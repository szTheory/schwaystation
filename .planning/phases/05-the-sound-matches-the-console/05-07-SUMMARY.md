---
phase: 05-the-sound-matches-the-console
plan: "07"
subsystem: audio
tags: [spu, adsr, pcm, fixed-point]
requires:
  - phase: 05-02
    provides: instance-owned SPU state, register, voice and audio-ring interfaces
provides:
  - Baseline ADSR rates and phase transitions for keyed voices
  - Direct and swept stereo voice/main volume with audible PCM output
affects: [spu, audio, libretro-audio]
actuals:
  tokens: 9915
  tasks: 2
  commits: 5
commits: 5
plan_head_before: fef5ba2452d25ea04a0a035ca04ddbddc70c1fac
plan_head_after: 68ee660b1dd35afa956e0f8b500b39deb3b8a490
tech-stack:
  added: []
  patterns:
    - Integer countdown periods derived from the documented rate table
    - Separate left/right signed sweep state serialized with the SPU
key-files:
  created:
    - src/spu_envelope.c
    - tests/spu/envelopes.c
  modified:
    - src/spu.c
    - src/spu.h
    - CMakeLists.txt
    - tests/spu/tests.cmake
    - tests/spu/inventory.txt
    - docs/hardware/spu.md
    - .planning/preparation/PSX-SPU-CDROM-AND-MDEC.md
key-decisions:
  - "Use documented envelope behavior as the baseline; ares rates are conformance data, not a hardware waveform."
  - "Do not add a profile option for unresolved high-rate and phase-negative sweep quirks."
requirements-completed: []
coverage:
  - id: D1
    description: ADSR, volume sweeps and enabled/muted stereo PCM path
    requirement: SOUND-01
    verification:
      - kind: unit
        ref: "cmake --preset ci && cmake --build --preset ci --target schwaystation-test-spu-envelopes schwaystation-test-spu-voices && ctest --test-dir build/ci -R '^spu\\.(envelopes|voices)$' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
duration: 29min
completed: 2026-11-14
status: complete
---

# Phase 5 Plan 07: ADSR and Volume Envelopes Summary

**Documented ADSR rates now advance keyed voices into audible stereo PCM, with signed sweeps, ENVX readback and mute-safe voice work.**

## Performance

- **Duration:** 29 minutes
- **Started:** 2026-11-14T03:26:37Z
- **Completed:** 2026-11-14T03:55:16Z
- **Tasks:** 2
- **Files modified:** 9

## Accomplishments

- Added an independent test generator for all 2×128 baseline rate entries and checked attack, decrease, exponential threshold, and all-ones no-step behavior.
- Wired KON/KOFF, ADSR phase transitions, direct ENVX writes, stereo sweeps and current-volume readback into the scheduled 24-voice path.
- Mixed volume-scaled voice samples into the public PCM ring; mute suppresses PCM while envelope and BRR work continues.
- Documented the implemented baseline and retained the hardware uncertainty without a model-profile switch.

## TDD Gate Compliance

- **RED 1:** `./build/ci/tests/schwaystation-test-spu-envelopes`, exit 1. `gsd check tdd-red-evidence build/ci/spu-envelopes-red.json --raw` returned `RED_EVIDENCE_OK`; the attack level stayed at 1000 instead of reaching 15336.
- **RED 2:** `./build/ci/tests/schwaystation-test-spu-envelopes`, exit 1. `gsd check tdd-red-evidence build/ci/spu-envelopes-red-exp.json --raw` returned `RED_EVIDENCE_OK`; exponential attack failed to advance from 0x7000 to 0x71c0.
- **RED 3:** `./build/ci/tests/schwaystation-test-spu-envelopes`, exit 1. `gsd check tdd-red-evidence build/ci/spu-envelope-phases-red.json --raw` returned `RED_EVIDENCE_OK`; the public PCM case failed on the silent, unwired path.
- **GREEN:** The final configure/build/CTest command in coverage passed both `spu.envelopes` and `spu.voices` (exit 0). No refactor-only change was needed.

## Task Commits

1. **Task 1 RED:** `d19a5de` — initial rate and attack checks.
2. **Task 1 RED expansion:** `48179ce` — decrease and threshold assertions.
3. **Task 1 GREEN:** `53842e1` — baseline rate and attack stepping.
4. **Task 2 RED:** `d746d0c` — phase, MMIO, sweep and PCM assertions.
5. **Task 2 GREEN:** `68ee660` — ADSR/sweep wiring, mixer and hardware documentation.

## Decisions Made

The core follows the documented integer baseline. The ares ISC rate generator at pin `9408cb43d494` is used only for baseline conformance; no test claims captured hardware envelope PCM. Rates above 26 and phase-negative sweep behavior remain open without a direct hardware capture.

## Deviations from Plan

None. The extra RED commits preserve the full planned TDD sequence while adding explicit checks for counter intervals and voice 24.

## Issues Encountered

The initial test build had an incorrect private-header include path; it was corrected before RED evidence was accepted.

## Global Requirement Status

SOUND-01 and SOUND-02 remain globally Pending until the remaining Phase 5 plans and phase acceptance complete.

## Next Phase Readiness

Plan 05-07's ADSR and volume-sweep slice is complete. Continue with the other ready Phase 5 plans; no later plan was started here.

---
*Phase: 05-the-sound-matches-the-console*
*Completed: 2026-11-14*

## Self-Check: PASSED

The summary exists, all five task commits are ancestors of HEAD, the measured plan commit count is 5, and the modified files contain no known stub patterns.
