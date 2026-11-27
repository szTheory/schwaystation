---
phase: 15-guncon-and-justifier
plan: "11"
subsystem: testing
tags: [guncon, justifier, save-state, fixtures]
requires:
  - phase: 15-09
    provides:
      - Format 21 serialized gun continuation fields
provides:
  - Current gpu_motion state hashes at frames 150 and 180
affects: [phase-15-verification, fixture-hashes]
actuals:
  tokens: 76
  tasks: 1
  commits: 1
tech-stack:
  added: []
  patterns:
    - Serialized-state pins follow exact runner output when an intentional format revision changes state bytes.
key-files:
  created:
    - .planning/phases/15-guncon-and-justifier/15-11-SUMMARY.md
  modified:
    - tests/fixtures/tests.cmake
key-decisions:
  - The Format 21 state hashes changed as expected after Plan 15-09 added fixed-width gun continuation fields; rendered-frame pins remain unchanged.
patterns-established:
  - State digests are measured from the current runner and checked independently from rendered-frame hashes.
requirements-completed:
  - GUN-01
coverage:
  - id: D1
    description: The gpu_motion save-state fixtures pin the exact Format 21 state at frames 150 and 180 while retaining rendered-frame pins.
    requirement: GUN-01
    verification:
      kind: integration
      ref: ctest --preset ci --no-tests=error -R '^fixtures[.]gpu_motion[.]state[.](150|180)$' --output-on-failure; passed 2/2 in 0.67s on c79b694
      status: pass
    human_judgment: false
  - id: D2
    description: The final reviewed source and both gap closures pass the complete local native and release workflow.
    requirement: GUN-01
    verification:
      kind: integration
      ref: time cmake --workflow --preset ci; exit 0 on 97606163409f6be78c08b8a2249b85bb68930619 (717/717 plus release 8/8)
      status: pass
    human_judgment: false
metrics:
  duration: 27m
  completed: 2026-11-25
status: complete
commits: 1
plan_head_before: a731baf
plan_head_after: c79b694
---

# Phase 15 Plan 11: Format 21 gpu_motion state pins

**The gpu_motion fixtures now pin the measured Format 21 state hashes without changing rendered-frame expectations.**

## Performance

- **Duration:** about 27 minutes from the plan commit through the final full-workflow pass, including the evidence-backed follow-up harness correction.
- **Completed:** 2026-11-25.
- **Tasks:** 1/1.
- **Files modified:** 1.

## Accomplishments

- Updated the state hashes at frames 150 and 180 to the current runner output: `8b8ad482d710e39fd0b0f296a42a52389be74cd4b7ffa5956e5799a14aff3e60` and `5c53ec895577a2638a22e14261157a22a7dbffcac552af8c449d0bb0d6f40da0`.
- Changed the fixture comment to Format 21 and preserved all frame hashes, selectors and assertions.
- Confirmed the focused state selectors passed 2/2 in 0.67 seconds. The first full-workflow attempt on `c79b694` exited 8 after 49 seconds because the older `libretro.input.controls` harness still treated now-supported LIGHTGUN as an invalid device; Plan 15-12 corrected that test assumption.

## Task Commits

1. **Task 1: Update the Format 21 gpu_motion state pins** — `c79b694`.

**Plan metadata:** `a731baf`.

## Run Evidence and CI Efficiency

- `git diff --check`, `cmake --preset ci`, and the order-independent selector-count preflight — exit 0.
- `ctest --preset ci --no-tests=error -R '^fixtures[.]gpu_motion[.]state[.](150|180)$' --output-on-failure` — exit 0, 2/2 passed in 0.67 seconds.
- The final `time cmake --workflow --preset ci` on `97606163409f6be78c08b8a2249b85bb68930619` passed 717/717 CTest cases and 8/8 release checks. CTest reported 71 seconds; the full workflow took 74.50 seconds.
- The first failed workflow took 49 seconds (CTest 48.32 seconds) and did not reach package or release checks. Inspection found the separate stale libretro test assumption; no emulator behavior change was needed.
- The current run's slowest tests were `policy.quality_metrics` (39.50s), `libretro.host` (35.56s), `libretro.input.movies` (25.84s, serial tail), `runner.disc.legacy` (20.48s), and `runner.hashes_file` (16.48s). Workflow files did not change; Act would add no coverage beyond the local workflow. Hosted Main and Release CI remain for ship.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Corrected the stale libretro invalid-device control in a follow-up gap plan**
- **Found during:** Full local workflow after the focused state selectors passed.
- **Issue:** The pre-existing test selected LIGHTGUN while asserting that an unsupported device request left the selected controller unchanged. Phase 15 correctly made LIGHTGUN selectable.
- **Fix:** Plan 15-12 uses the unsupported `RETRO_DEVICE_KEYBOARD` request and retains the serialized-state comparison.
- **Files modified:** `tests/libretro/host.c`.
- **Verification:** Focused selector and final full workflow passed; details are in `15-12-SUMMARY.md`.
- **Committed in:** `96b0194`.

**Total deviations:** 1 evidence-backed follow-up.
**Impact on plan:** The required state fixture correction remained isolated; the unrelated integration test received a separate, minimal gap closure without changing emulator behavior.

## Next Phase Readiness

The state-hash gap is closed on the same final head as the Phase 15 local workflow. Final goal-backward verification and hosted ship gates remain.

---
*Phase: 15-guncon-and-justifier*
*Completed: 2026-11-25*

## Self-Check: PASSED

- The summary exists and its coverage entries use the required block-style kind/ref/status contract.
- Task commit `c79b694` is an ancestor of HEAD.
- `git diff --check` passed, and the final local workflow passed on the recorded head.
