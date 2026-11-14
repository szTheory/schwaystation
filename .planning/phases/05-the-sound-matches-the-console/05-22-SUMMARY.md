---
phase: 05-the-sound-matches-the-console
plan: "22"
subsystem: audio
tags: [spu, brr, pmon, stereo, deterministic]
requires:
  - phase: 05-the-sound-matches-the-console
    provides: "BRR interpolation and autonomous envelopes from plans 05-06 and 05-07, plus the scheduled SPU integration from plan 05-21"
provides:
  - "Ordered scheduled playback for all 24 BRR voices"
  - "PMON pitch modulation, BRR loop start/repeat/mute, and ENDX handling"
  - "MMIO sample upload and KON produce saturated signed stereo PCM through the public drain"
affects: ["Phase 05 plans 05-08 and 05-11"]
actuals:
  tokens: 4004
  tasks: 1
  commits: 2
tech-stack:
  added: []
  patterns:
    - "Voice n pitch modulation consumes voice n-1 OUTX in scheduled order"
    - "BRR loop flags are read at block boundaries through the IRQA-aware RAM helper"
key-files:
  created: []
  modified:
    - src/spu_voice.c
    - tests/spu/voices.c
key-decisions:
  - "Reuse the existing visited SPU register, sample-counter, envelope, history, sample, and output fields; no save-state field or visitor change was needed."
  - "Keep voice processing sample-granular and in voice 1–24 order, including silent and NON-selected voices that still perform BRR fetches."
requirements-completed: []
coverage:
  - id: SPU-ORDERED-VOICES
    description: "All 24 voices fetch BRR data at the scheduled sample tick, with PMON using the preceding voice output and loop/ENDX behavior checked at block boundaries."
    requirement: SOUND-01
    verification:
      - kind: unit
        ref: "tests/spu/inventory.txt: spu.voices; TAP cases 3–5"
        status: pass
    human_judgment: false
  - id: SPU-GUEST-PCM
    description: "Guest MMIO upload, KON/KOFF, signed unequal channel volumes, mute, and overflow produce checked output through schway_get_audio."
    requirement: SOUND-01
    verification:
      - kind: integration
        ref: "tests/spu/inventory.txt: spu.voices; TAP cases 2 and 6"
        status: pass
    human_judgment: false
metrics:
  duration: 13 min
  completed: 2026-11-14
status: complete
commits: 2
plan_head_before: 1813a93c57ce338122c39308f84af3526c026c2c
plan_head_after: 2ca4e17e66e43e208f7e4bf86e57bf736c9bbc16
---

# Phase 5 Plan 22: Ordered SPU voices, loops, PMON, and stereo mix Summary

**The scheduled SPU now advances all 24 BRR voices in order, applies pitch modulation and loop flags, and delivers guest-generated stereo PCM.**

## Performance

- **Duration:** 13 minutes
- **Started:** 2026-11-14T04:26:52Z
- **Completed:** 2026-11-14T04:40:00Z
- **Tasks:** 1
- **Files modified:** 2
- **Runtime/model:** Codex/OpenGSD 1.20.0; gpt-6-luna xhigh

## Accomplishments

- Processed all 24 voices in sample-tick order; PMON uses the prior voice's current OUTX and handles the upper PMON halfword.
- Applied loop-start, loop-repeat, end-mute, ENDX, and address changes at BRR block boundaries using the IRQA-aware SPU RAM reader.
- Verified MMIO sample upload and KON through the public stereo drain, including signed channel volumes, saturation, mute, direct ENVX, KOFF, and IRQ-visible reads.
- Reused existing serialized fields. No save-state field, state visitor, or size change was needed.

## Task Commits

1. **Task 1 RED: ordered voice, loop, PMON, and mix checks** - `4c894df` (`test`)
2. **Task 1 GREEN: ordered SPU voice playback** - `2ca4e17` (`feat`)

## TDD Trace

- **RED:** `build/ci/tests/schwaystation-test-spu-voices` exited 1; `gsd_run check tdd-red-evidence` returned `RED_EVIDENCE_OK` for the loop-start/repeat/end-mute/ENDX assertion. The initial run also exposed a test-only out-of-bounds reference phase in the MMIO case, which was corrected during the task.
- **GREEN:** The planned configure/build/CTest command exited 0. The `spu.voices` TAP target passed all six cases, and `spu.model` passed. The exact tracer verification was rerun after the GREEN commit and again passed 2/2 CTests.

## Commands and Evidence

- `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-spu-voices schwaystation-test-spu && ctest --test-dir build/ci -R '^spu\.(voices|model)$' --no-tests=error --output-on-failure` - exit 0; 2/2 tests passed.
- `git diff --check` - exit 0.
- `gsd_run check tdd-red-evidence .planning/phases/05-the-sound-matches-the-console/05-22-red-evidence.json --raw` - RED target classified as expected; the temporary evidence file was removed after classification.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Corrected the MMIO test's Gaussian reference phase**
- **Found during:** Task 1 (guest upload and public PCM test)
- **Issue:** The scalar reference helper accepts a Gaussian table index, but the test passed the raw sample counter phase, causing an out-of-bounds coefficient read and invalid expected values.
- **Fix:** Passed the sample-boundary table index (`0`) and retained exact independent stereo-output assertions.
- **Files modified:** `tests/spu/voices.c`
- **Verification:** The focused `spu.voices` test passes and matches the public PCM drain output.
- **Committed in:** `2ca4e17`

**Total deviations:** 1 auto-fixed (Rule 1)
**Impact on plan:** This corrected a defect in the new test reference; implementation scope and save-state shape stayed as planned.

## Issues Encountered

- The initial RED run's MMIO test failure came from the test reference's invalid phase argument, not audio delivery. The corrected test confirms nonzero, unequal channel samples and the expected scalar mix.

## User Setup Required

None. All checks were deterministic and agent-run.

## Next Phase Readiness

- Plan 05-22 is complete. SOUND-01 and SOUND-02 remain Pending until Phase 5 verification.
- Noise waveform generation remains in its later Phase 5 plan; this plan verifies that NON-selected voices still fetch BRR RAM and preserve IRQA-visible work.
- No other plan, Phase 6 work, or shipping was started.

---
*Phase: 05-the-sound-matches-the-console*
*Completed: 2026-11-14*

## Self-Check: PASSED

- Summary exists, and task commits `4c894df` and `2ca4e17` are ancestors of the plan head.
- Commit count and plan head hashes were measured from the persisted 05-22 ledger.
