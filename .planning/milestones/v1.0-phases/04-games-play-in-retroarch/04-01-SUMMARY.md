---
phase: 04-games-play-in-retroarch
plan: "01"
subsystem: testing
tags: [cmake, ctest, inventories, gpu]

requires: []
provides:
  - GPU-labeled test-card pixel and native-video smoke registrations
  - SIO, card, compatibility, and built-input test registration seams
  - Exact GPU inventory selection in ci, asan, and wasm32 lanes
affects: [gpu, sio, card, compatibility, built-input]

actuals:
  tokens: 1013
  tasks: 3
  commits: 3
plan_head_before: 6b5553f78e5e8b45b579d36a6cf2746a650e3750
plan_head_after: 98a4505ba9b4ce2118eb416864ce47f014150d88

tech-stack:
  added: []
  patterns:
    - Per-directory CMake registration with an exact sorted test inventory

key-files:
  created:
    - tests/gpu/tests.cmake
    - tests/gpu/inventory.txt
    - tests/sio/tests.cmake
    - tests/sio/inventory.txt
    - tests/card/tests.cmake
    - tests/card/inventory.txt
    - tests/compat/tests.cmake
    - tests/compat/inventory.txt
    - tests/built/tests.cmake
    - tests/built/inventory.txt
  modified:
    - tests/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/asan.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - GPU smoke cases reuse the existing core test executable; the real GPU executable remains with its first implementation caller.
  - SIO, card, compatibility, and built-input inventories remain empty until their behavior tests exist.

duration: 16 min
completed: "2026-11-10"
status: complete

requirements-completed: []
coverage:
  - id: D1
    description: Existing test-card pixels and native-video tuple assertions run through the GPU inventory.
    requirement: GAME-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^gpu\\.(testcard\\.pixels|video\\.tuple)$' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: The five new test directories are included without registering nonexistent tests, and the ci inventory remains exact.
    requirement:
      - GAME-01
      - GAME-03
      - GAME-04
      - GAME-05
      - GAME-06
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci"
        status: pass
    human_judgment: false
  - id: D3
    description: Repository hygiene, provenance, fixture policy, and the existing gate budgets remain valid.
    requirement:
      - GAME-05
      - GAME-07
    verification:
      - kind: integration
        ref: "cmake --workflow --preset hygiene"
        status: pass
    human_judgment: false
---

# Phase 04 Plan 01: GPU Smoke Inventory and Subsystem Registration Summary

**GPU smoke cases now exercise the existing test-card pixel and native-video assertions, with per-directory registration seams ready for phase-4 behavior tests.**

## Performance

- **Duration:** 16 min
- **Started:** 2026-11-10T17:20:04Z
- **Completed:** 2026-11-10T17:36:16Z
- **Tasks:** 3
- **Files modified:** 14

## Accomplishments

- Registered `gpu.testcard.pixels` and `gpu.video.tuple` through `schwaystation-test-core`, with `gpu,portable` labels and ci, asan, and wasm32 selection.
- Added SIO, card, compatibility, and built-input registration files with empty inventories, as required until their behavior tests arrive.
- Preserved existing test-card hashes and the repository's exact registration checks.

## Task Commits

1. **Trace the existing native test-card path through the GPU test inventory** - `0917f9b` (test)
2. **Create pad and card registration ownership seams** - `2552e96` (test)
3. **Finish compatibility and built-input inventories and lane selections** - `98a4505` (test)

## Files Created/Modified

- `tests/gpu/tests.cmake` and `tests/gpu/inventory.txt` register the two existing smoke cases.
- `tests/sio/`, `tests/card/`, `tests/compat/`, and `tests/built/` contain per-directory CMake and inventory seams.
- `tests/tests.cmake` includes all five directories; ci, asan, and wasm32 select the populated GPU inventory.

## Decisions Made

- Reused the existing core test executable for the GPU smoke path; no GPU test source or executable was created.
- Kept the other four new inventories empty rather than registering nonexistent tests.
- Left requirement completion empty because this plan adds registration infrastructure and does not implement the GAME-01, GAME-03, GAME-04, GAME-05, or GAME-06 behavior.

## Deviations from Plan

None - plan executed as written.

## Issues Encountered

The first sandboxed hygiene workflow timed out its two full-tree scans at 60 seconds and printed `/bin/ps: Operation not permitted`; the other 21 checks passed. An isolated tree scan passed in 46.39 seconds. The required full `cmake --workflow --preset hygiene` then passed all 23 checks in 52.50 seconds with unrestricted execution and login-shell startup disabled. No source or gate changes were needed.

## User Setup Required

None.

## Next Phase Readiness

The test registration seams are ready for the GPU, SIO, card, compatibility, and built-input implementation plans. No user action is needed.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-10*

## Self-Check: PASSED

All 14 plan files and the summary exist; task commits 0917f9b, 2552e96, and 98a4505 are ancestors of HEAD; `git diff --check` passed.
