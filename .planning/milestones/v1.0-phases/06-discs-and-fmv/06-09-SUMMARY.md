---
phase: 06-discs-and-fmv
plan: "09"
subsystem: disc-audio
tags: [cd-xa, adpcm, integer-resampling, save-state]
requires:
  - phase: 06-discs-and-fmv
    provides: CD-ROM instance state and raw sector model
provides:
  - Bounded Mode2/Form2 XA ADPCM stereo decoding
  - Deterministic seven-phase integer resampling with instance history
affects: [cdrom, xa-audio, save-states]
actuals:
  tokens: 10940
  tasks: 2
  commits: 2
plan_head_before: 4b7bddeee17ba0ae64ad3de8d244a7a1bf8fe989
plan_head_after: cf29b03ca11ee840ae195c374d979d5797ab74f7
commits: 2
tech-stack:
  added: []
  patterns: [integer-only XA decoder, per-instance predictor and resampler history]
key-files:
  created: [src/cdxa.c, src/cdxa.h, tests/cdrom/xa.c]
  modified: [src/cdrom.h, src/cdrom_state.c, src/internal.h, tests/core/test_state.c, docs/hardware/cdrom.md, docs/embedding.md]
key-decisions:
  - "Keep ADPCM predictor history separate from resampler history so duplicated low-rate samples do not alter predictor chronology."
  - "Use the documented Q15 tap table with integer truncation as a deterministic baseline; this does not claim silicon waveform proof."
  - "Reject emphasis and reserved coding/header bits until their effects are modeled."
requirements-completed: []
coverage:
  - id: xa-form2-decode-and-resample
    description: Decode supported XA Form2 coding combinations and resample them with persistent integer state.
    requirement: DISC-02
    verification:
      kind: unit
      ref: >-
        cmake --preset ci && cmake --build --preset ci --target schwaystation-test-cdxa && ctest --test-dir build/ci -R '^disc\.xa$' --no-tests=error --output-on-failure (exit 0)
      status: pass
    human_judgment: false
metrics:
  duration: 32min
  completed_date: 2026-11-16
status: complete
---

# Phase 6 Plan 09: XA decode and resample Summary

**XA Form2 sectors now decode into bounded stereo PCM with persistent predictor state and a deterministic seven-phase integer resampler.**

## Performance

- **Duration:** 31m59s from plan start through closeout preparation; the exact final gate completed after 28 minutes
- **Started:** 2026-11-16T03:29:33Z
- **Completed:** 2026-11-16T04:01:32Z
- **Tasks:** 2
- **Files modified:** 13 implementation, test, configuration, plan, and documentation files, plus this summary at closeout
- **Executor model:** gpt-6-luna xhigh; Codex/OpenGSD 1.20.0

The 20–30 minute forecast was exceeded by closeout work after the implementation and focused gate had finished. The code/test/review work and final gate completed within 28 minutes; the remaining wall time was required summary, roadmap, and state bookkeeping. No acceptance criteria were removed.

## Accomplishments

- Decoded supported 4-bit and 8-bit mono/stereo Form2 audio with bounded fixed arrays, all predictor filters and shift values, documented channel ordering, saturation, and persistent predictor history.
- Added the seven-phase, 29-tap integer resampler, including low-rate input duplication, phase/history continuation across calls and sectors, and deterministic rounding documentation.
- Serialized the new predictor history and advanced state format and behavior revision from 10 to 11; registered `disc.xa` and expanded direct-state and resampler controls.

## Task Commits

1. **Task 1: Decode a Form2 sector into deterministic stereo samples (RED)** - `aa36afc` (test)
2. **Tasks 1–2: Decode and resample XA** - `cf29b03` (feat)

## Verification Evidence

- RED evidence: the registered `disc.xa` test failed on the missing decoder behavior (CTest exit 8); `gsd_run check tdd-red-evidence` classified the stored report as `RED_EVIDENCE_OK`.
- Focused decoder and state controls: configure/build for `schwaystation-test-cdxa` and `schwaystation-test-state`, followed by selected CTest; 6/6 passed, exit 0.
- Exact plan gate: `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-cdxa && ctest --test-dir build/ci -R '^disc\.xa$' --no-tests=error --output-on-failure`; exit 0, `disc.xa` passed 1/1.
- `git diff --check`; exit 0. `src/cdrom.c` is 1,178 lines, below its 1,200-line cap.
- No hosted run URL applies to this local gate.

## TDD Gate Compliance

- **RED:** `disc.xa` was registered before implementation and failed on the expected absent decode behavior; semantic inspection and the generated evidence report both confirmed a valid RED.
- **GREEN:** the exact plan configure/build/CTest command passed after implementation.
- **REFACTOR:** the final focused decoder and five state tests passed after adding malformed-input, coding-matrix, predictor, phase, and state-format controls.

## Decisions Made

- Predictor state is separate from the 29-sample resampler history, preserving ADPCM chronology when 18.9-kHz input is duplicated for resampling.
- Emphasis and reserved coding/header values are rejected before history mutation; the reference describes emphasis but does not define its output formula.
- State format and behavior revision advance to 11 because the new predictor samples are serialized and decoded audio behavior is added.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing correctness coverage] Serialized XA predictor history and advanced compatibility revisions.**
- **Found during:** Task 1
- **Issue:** Predictor history is part of emulated instance state and must survive save/load; keeping it outside serialization would break deterministic continuation.
- **Fix:** Added the predictor field to CD state visitation, round-trip coverage, and documentation; advanced state format and behavior revision from 10 to 11.
- **Files modified:** `src/cdrom.h`, `src/cdrom_state.c`, `src/internal.h`, `tests/core/test_state.c`, `docs/embedding.md`, `docs/hardware/cdrom.md`.
- **Verification:** Five focused state tests passed.
- **Committed in:** `cf29b03`.

**2. [Rule 2 - Input validation] Reject unsupported emphasis and reserved XA bits before state mutation.**
- **Found during:** Task 1 review
- **Issue:** The reference leaves emphasis output behavior unknown and reserves coding/portion header bits; silently decoding these as ordinary audio would claim unsupported behavior.
- **Fix:** Reject emphasis, coding bit 3, coding bit 7, and nonzero portion-header reserved bits, with malformed-input no-mutation controls.
- **Files modified:** `src/cdxa.c`, `tests/cdrom/xa.c`, `docs/hardware/cdrom.md`.
- **Verification:** Exact `disc.xa` gate passed.
- **Committed in:** `cf29b03`.

**Total deviations:** 2 auto-fixed correctness/validation additions. Both preserve the plan's bounded decoder scope.

## Requirements Coverage

- **DISC-02:** The XA decoder/resampler portion is verified by `disc.xa`; the phase-wide requirement remains pending aggregate integration because the decoder is not yet connected to the SPU playback path.
- **DISC-01:** No coverage is claimed by this plan; the CD controller acceptance remains pending its later integration plans.

## Deferred Issues

- Connect accepted XA sectors to the SPU audio queue in the later XA/SPU integration plan; this plan intentionally provides and tests the decoder/resampler seam only.
- Phase-wide DISC-01 and DISC-02 acceptance remains pending.

## Next Phase Readiness

The standalone XA decode/resample seam and serialized histories are ready for the next sequential Phase 6 plan. No owner verification or external configuration is required.

## Self-Check: PASSED

- Summary exists at `.planning/phases/06-discs-and-fmv/06-09-SUMMARY.md`.
- Task commits `aa36afc` and `cf29b03` are ancestors of HEAD.
- The exact final Plan 09 gate completed with exit 0.

---
*Phase: 06-discs-and-fmv*
*Completed: 2026-11-16*
