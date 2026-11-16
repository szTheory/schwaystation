---
phase: 06-discs-and-fmv
plan: "34"
subsystem: cdrom-audio
tags: [xa, cdda, spu, reverb, filtering]
requires:
  - phase: 06-33
    provides: XA audio sector route and native mixer integration
provides:
  - Registered mode-enabled XA file/channel mismatch retry and skip controls
  - Registered XA/CDDA/voice/reverb composition control at native SPU ticks
affects: [DISC-02, 06-16]
actuals:
  tokens: 1828.75
  tasks: 1
  commits: 1
  plan_head_before: 2873a37c0c3e846ab6fd9ac5eefce82e67049e7f
  plan_head_after: eff7a031757c7130caff5935e9927f04f7180540
tech-stack:
  added: []
  patterns:
    - Exercise production sector selection and native SPU scheduling through registered core tests.
key-files:
  created: []
  modified:
    - tests/cdrom/audio.c
key-decisions:
  - "Keep the XA mismatch and mixed-source acceptance checks test-only because the existing production paths satisfy both behaviors."
  - "Assert the second mismatched ReadN advances the target LBA after retrying the same sector once."
requirements-completed: []
coverage:
  - id: D1
    description: Mode-enabled file/channel mismatch stays out of the XA queue, retries the same LBA once, then skips without delivering a data sector.
    requirement: DISC-02
    verification:
      - kind: integration
        ref: cmake --preset ci && cmake --build --preset ci --target schwaystation-test-cdrom-audio schwaystation-test-spu schwaystation-test-state && ctest --test-dir build/ci -R '^disc\.audio|^spu\.model$|^core\.state\.' --no-tests=error --output-on-failure
        status: pass
    human_judgment: false
  - id: D2
    description: Accepted XA and CDDA samples mix with an active voice, and enabling the CD reverb send changes native output and reverb capture.
    requirement: DISC-02
    verification:
      - kind: integration
        ref: cmake --preset ci && cmake --build --preset ci --target schwaystation-test-cdrom-audio schwaystation-test-spu schwaystation-test-state && ctest --test-dir build/ci -R '^disc\.audio|^spu\.model$|^core\.state\.' --no-tests=error --output-on-failure
        status: pass
    human_judgment: false
metrics:
  duration: "at least 10m48s wall lower bound"
  completed: 2026-11-16
  commits: 1
status: complete
---

# Phase 6 Plan 34: Verify XA interleave filtering and mixed native audio sources Summary

Registered controls now prove that XA interleave filtering follows the existing retry/skip path and that XA, CDDA, voice and reverb compose in the native SPU mixer.

## Performance

- **Runtime/model:** Codex/OpenGSD 1.20.0; executor gpt-6-luna xhigh.
- **Duration:** At least 10m48s wall lower bound, measured from the recorded plan start at `2026-11-16T07:56:23Z` through the final focused gate at `2026-11-16T08:07:11Z`; summary and metadata closeout followed.
- **Tasks:** 1/1 complete.
- **Files modified:** 1.
- **Hosted run:** None; local focused checks only.

## Completed work

- Added a mode-enabled file/channel mismatch control that checks the first same-LBA retry, then verifies retry clears, no sector is delivered or added to the XA queue, and the target advances to LBA 1.
- Added a mixed-source control that drains XA and CDDA at the native SPU tick alongside an active voice. Direct-only and CD-reverb-send runs verify capture values and distinct PCM output.
- Kept the change test-only because current production behavior passed both controls.

## Gate Evidence

The final focused command was:

```sh
cmake --preset ci && cmake --build --preset ci --target schwaystation-test-cdrom-audio schwaystation-test-spu schwaystation-test-state && ctest --test-dir build/ci -R '^disc\.audio|^spu\.model$|^core\.state\.' --no-tests=error --output-on-failure
```

Exit status: 0. All 12 selected tests passed: three CD audio selectors, `spu.model`, and eight core state tests. The run includes the post-skip target-LBA assertion. `git diff --check` also passed.

## Task Commits

1. **Task 1 controls:** `eff7a03` (`test(06-34): cover XA mismatch and mixed reverb audio`).

## Deviations from Plan

No production repair was needed: the existing implementation already met both tested behaviors. An initial reverb-history comparison sampled after 96 ticks, after the short FIR impulse had advanced; the assertion was moved to the first native tick, where the control directly observes the reverb contribution. This was a test timing correction, and no failed assertion is claimed as valid RED evidence.

DISC-02 remains pending aggregate Phase 6 integration and its owned-disc guest; this plan records partial coverage only.

## Self-Check: PASSED

- Summary file exists.
- Task commit `eff7a03` is an ancestor of the current branch.
- The focused 12-test gate and `git diff --check` passed.
- `verify-summary` passed, and the coverage classifier accepted both automated entries.

---
*Phase: 06-discs-and-fmv*
*Completed: 2026-11-16*
