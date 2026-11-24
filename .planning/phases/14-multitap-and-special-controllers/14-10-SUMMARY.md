---
phase: 14-multitap-and-special-controllers
plan: "10"
subsystem: testing
tags: [libretro, sio, callback, serial]
requires:
  - phase: 14-06
    provides: Existing libretro controller callback and serial device support
  - phase: 14-08
    provides: Stable physical port and socket identities
provides:
  - Test-only MMIO polling helper for socket A on either physical port
  - Green default two-player callback-to-serial baseline
affects: [libretro input tests, Phase 14 Plan 14-11]
actuals:
  tokens: 1658
  tasks: 1
  commits: 1
  plan_head_before: 35738c11d8dbee6dbf9ccd9e9127cd39ff02cc7e
  plan_head_after: 4cfe18e482fe3d961abed33705085248b8017a74
tech-stack:
  added: []
  patterns:
    - Test-only MMIO inspection is compiled into the uninstalled card-test module only
key-files:
  created: []
  modified:
    - libretro/libretro.c
    - tests/libretro/host.c
key-decisions:
  - Keep this plan limited to default socket-A behavior; Plan 14-11 owns tap topology and routing.
patterns-established:
  - Exercise frontend input callbacks through real SIO replies using a private test-module helper.
requirements-completed: []
coverage:
  - id: D1
    description: "Default players 0 and 1 reach their separate socket-A serial replies with distinct buttons and one input poll per retro_run; this is baseline coverage only."
    requirement: SIO-01
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^libretro[.]input[.](multitap|descriptors)$' --output-on-failure (2/2)"
        status: pass
    human_judgment: false
duration: 9min
completed: 2026-11-24
status: complete
---

# Phase 14 Plan 14-10: Default two-pad serial baseline Summary

**The default libretro players now have a focused callback-to-serial regression check using private MMIO inspection.**

## Performance

- **Duration:** 9m wall time
- **Started:** 2026-11-24T09:30:01Z
- **Completed:** 2026-11-24T09:39:18Z
- **Runtime/model:** Codex GSD executor, configured `gpt-6-luna` xhigh; primary runtime model ID unavailable
- **Tasks:** 1
- **Files modified:** 2

## Accomplishments

- Added a test-only MMIO helper, available only in the uninstalled card-test module, that reads the real socket-A button payload from either physical controller port.
- The `input-multitap` mode now drives player 0's Cross and player 1's Circle independently, checks each matching serial reply, and confirms exactly one `input_poll` per `retro_run`.
- The existing descriptor selector confirms the shipped module does not expose the private test helper.
- This plan establishes only the default two-player baseline. It does not claim multitap options, expanded descriptors, B–D routing, or completed SIO-01 coverage.

## Task Commits

1. **Task 1: Prove the default two-pad callback and serial inspection path** — `4cfe18e` (test)

**Plan metadata:** `27f1351` — summary, State, and Roadmap closeout.

## Files Created/Modified

- `libretro/libretro.c` — test-only MMIO read and serial polling helper.
- `tests/libretro/host.c` — per-player callback tracking and default socket-A wire assertions.

## Decisions Made

- Keep taps disabled and test only players 0 and 1 on their fixed socket-A endpoints. Plan 14-11 owns tap options and expanded routing.

## Deviations from Plan

None — the revised baseline plan was followed as written.

## Issues Encountered

The preserved WIP mixed all four future tap topologies with the baseline test and treated the final callback port as the selected player. The callback can visit both default players in one run, and socket B–D is unavailable while taps are disabled. The test now tracks each player's callback visits separately and polls only that player's physical socket A. No new RED history was created.

## Verification and Iteration Time

- `git diff --check` — exit 0.
- `LC_ALL=C sort -cu tests/libretro/inventory.txt` — exit 0; inventory unchanged.
- `cmake --preset ci` — exit 0; reused `build/ci` configure tree.
- CTest JSON selector preflight for `libretro.input.multitap` and `libretro.input.descriptors` — exit 0; both selectors present.
- `cmake --build --preset ci --target schwaystation-test-libretro-host schwaystation_libretro` — exit 0.
- `ctest --preset ci --no-tests=error -R '^libretro[.]input[.](multitap|descriptors)$' --output-on-failure` — exit 0; 2/2 passed in 0.03 seconds.

No broad suite, hosted CI, Act, or Docker was needed for this baseline-only test change. No Docker resources were created.

## Next Phase Readiness

Plan 14-11 can add per-port tap options and expanded input topology against this proven default callback-to-serial seam. No user action is needed.

## Self-Check: PASSED

- Summary file exists at the phase path.
- Task commit `4cfe18e` is an ancestor of HEAD.
- The changed files contain no new TODO/FIXME or placeholder stubs.

---
*Phase: 14-multitap-and-special-controllers*
*Completed: 2026-11-24*
