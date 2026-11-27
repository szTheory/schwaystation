---
phase: 15-guncon-and-justifier
plan: "12"
subsystem: testing
tags: [libretro, controllers, guncon, justifier]
requires:
  - phase: 15-07
    provides:
      - Supported LIGHTGUN controller selection
provides:
  - A valid unsupported-device negative control in the libretro input-controls test
affects: [phase-15-verification, libretro-input-tests]
actuals:
  tokens: 42
  tasks: 1
  commits: 1
tech-stack:
  added: []
  patterns:
    - Invalid controller-selection tests use a device ID outside the supported controller allowlist.
key-files:
  created:
    - .planning/phases/15-guncon-and-justifier/15-12-SUMMARY.md
  modified:
    - tests/libretro/host.c
key-decisions:
  - Keep LIGHTGUN accepted and test unsupported selection with RETRO_DEVICE_KEYBOARD; retain all serialized-state assertions.
patterns-established:
  - Rebuild a changed test target before running its CTest selector so verification cannot use a stale executable.
requirements-completed:
  - GUN-02
coverage:
  - id: D1
    description: An unsupported controller-selection request preserves the selected devices, and LIGHTGUN remains supported through its dedicated tests.
    requirement: GUN-02
    verification:
      kind: integration
      ref: cmake --build --preset ci --target schwaystation-test-libretro-host && ctest --preset ci --no-tests=error -R '^libretro[.]input[.]controls$' --output-on-failure; build exit 0, test passed 1/1 in 1.33s
      status: pass
    human_judgment: false
  - id: D2
    description: The full local CI workflow retains all cases and release checks and passes after the harness correction.
    requirement: GUN-02
    verification:
      kind: integration
      ref: time cmake --workflow --preset ci; exit 0 on 97606163409f6be78c08b8a2249b85bb68930619 (717/717 plus release 8/8)
      status: pass
    human_judgment: false
metrics:
  duration: 9m
  completed: 2026-11-25
status: complete
commits: 1
plan_head_before: 7d76844
plan_head_after: 96b0194
---

# Phase 15 Plan 12: Unsupported libretro controller control

**The libretro input-controls test now rejects an actually unsupported device while preserving LIGHTGUN support coverage.**

## Performance

- **Duration:** about 9 minutes from plan commit through the final local workflow.
- **Completed:** 2026-11-25.
- **Tasks:** 1/1.
- **Files modified:** 1.

## Accomplishments

- Replaced the stale `RETRO_DEVICE_LIGHTGUN` request in the invalid-selection control with `RETRO_DEVICE_KEYBOARD`.
- Kept the serialized-state comparison, invalid-port case and dedicated LIGHTGUN selectors unchanged.
- Added an explicit target build before the focused CTest check after the first check was found to use a stale test executable.

## Task Commits

1. **Task 1: Use an unsupported device in the invalid-request control** — `96b0194`.

**Plan metadata:** `7d76844`; focused-build correction: `9760616`.

## Run Evidence

- `git diff --check`, `cmake --preset ci`, and exact selector inventory — exit 0.
- `cmake --build --preset ci --target schwaystation-test-libretro-host` — exit 0; the changed host test was rebuilt.
- `ctest --preset ci --no-tests=error -R '^libretro[.]input[.]controls$' --output-on-failure` — exit 0, 1/1 passed in 1.33 seconds.
- An earlier focused CTest attempt before rebuilding the changed host harness used the stale executable and exited 8 in 1.34 seconds. The test source was then rebuilt; the repeated focused check passed.
- Final `time cmake --workflow --preset ci` on `97606163409f6be78c08b8a2249b85bb68930619` — exit 0, 717/717 CTest cases, followed by 8/8 release checks; total wall time 74.50 seconds.

## CI Efficiency

The CTest JUnit suite time was 71 seconds; the serial `libretro.input.movies` case took 25.84 seconds at the end of the run. The other slow cases were `policy.quality_metrics` (39.50s) and `libretro.host` (35.56s). All required tests remained enabled. No workflow files changed, so Act would not add coverage beyond this complete local workflow. Hosted Main and Release gates remain required after ship.

## Deviations from Plan

The plan was amended to build `schwaystation-test-libretro-host` before CTest. The initial focused run exposed why this matters: CTest executes the registered binary but does not rebuild it after test-source edits.

## Next Phase Readiness

The focused regression and final local workflow pass. Phase 15 is ready for goal-backward verification; hosted Main/Release evidence remains part of ship.

---
*Phase: 15-guncon-and-justifier*
*Completed: 2026-11-25*

## Self-Check: PASSED

- The summary exists and its coverage entries use the required block-style kind/ref/status contract.
- Task commit `96b0194` is an ancestor of HEAD.
- `git diff --check` passed, and the final local workflow passed on the recorded head.
