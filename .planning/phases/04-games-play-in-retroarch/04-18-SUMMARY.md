---
phase: 04-games-play-in-retroarch
plan: "18"
subsystem: runner-inputs
tags: [C, runner, input-scripts, CTest]

requires:
  - phase: 04-15
    provides: API v4 host input and frame-start latching.
  - phase: 04-12
    provides: Stable frame boundaries and state hashing.
  - phase: 04-10
    provides: Native video hash preservation for the test card.
  - phase: 04-35
    provides: Shared runner documentation and host-side file handling patterns.
provides:
  - Bounded ASCII controller scripts parsed completely before core creation or execution.
  - Per-frame two-port snapshots applied with schway_set_input before the frame latch.
  - Registered CLI coverage and the documented script grammar and refusal statuses.
affects: [04-36, runner-inputs]

actuals:
  tokens: 7665
  tasks: 2
  commits: 3
commits: 3
plan_head_before: f506fc420e57e7c0c53757b4db065fb7a5acbc78
plan_head_after: b2b61c993290d8c82e77b661ec6b069c16cdf5fb

tech-stack:
  added: []
  patterns:
    - Host-owned bounded text parsing completes before core creation.
    - Sorted per-frame snapshots are applied immediately before each frame call.

key-files:
  created:
    - runner/inputs.c
    - runner/inputs.h
  modified:
    - runner/main.c
    - runner/CMakeLists.txt
    - tests/runner/tests.cmake
    - tests/runner/inventory.txt
    - docs/runner.md

key-decisions:
  - "Keep the runner's input-file I/O outside the emulator core and validate the entire script before execution."
  - "Use one-based frame records with explicit digital or DualShock devices, documented button symbols, and left-X/left-Y/right-X/right-Y axes."
  - "Keep API revision 4 and preserve the no-content frame hash; the saved state hash proves latched input reached the core."

patterns-established:
  - "Input files have a 64 MiB and 1,000,000-record bound; records sort by frame and conflicting duplicates fail."
  - "The real runner CLI is the test seam for script acceptance, latch timing, and malformed-file refusal."

requirements-completed: []
coverage:
  - id: D1
    description: "The runner parses an empty or scheduled input script and applies it before the frame latch; repeat playback is deterministic and leaves the test-card frame hash unchanged."
    requirement: GAME-03
    verification:
      - kind: integration
        ref: "runner.inputs.script via ctest --test-dir build/ci -R '^runner\\.inputs\\.script$' --no-tests=error --output-on-failure"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci (exit 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The CLI accepts frame, port, button, axis, comment, and ordering endpoints, and refuses malformed or conflicting scripts before producing frame output."
    requirement: GAME-03
    verification:
      - kind: integration
        ref: "runner.inputs.controls via ctest --test-dir build/ci -R '^runner\\.inputs\\.controls$' --no-tests=error --output-on-failure"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset hygiene (exit 0)"
        status: pass
    human_judgment: false

duration: 27min
completed: 2026-11-11
status: complete
---

# Phase 04 Plan 18: Runner Input Movies Summary

**The headless runner now applies validated per-frame controller snapshots before latching, without changing the no-content video hash.**

## Performance

- **Duration:** 27 min
- **Started:** 2026-11-11T10:53:34Z
- **Completed:** 2026-11-11T11:20:43Z
- **Tasks:** 2
- **Files modified:** 7

## Accomplishments

- Added host-side parsing for bounded ASCII input scripts with one-based frame records, comments, two controller ports, button symbols, and axes.
- Validated the full movie before core creation, sorted records by frame, rejected conflicting same-frame snapshots, and applied snapshots through `schway_set_input` before each frame call.
- Verified empty, pre-event, event-frame, and repeated movies through the real CLI using frame and state hashes; added endpoint and refusal controls and documented the grammar.

## Task Commits

1. **Task 1 RED:** `6ecb2e3` — add failing runner input movie contract.
2. **Task 1 GREEN:** `07ebaf4` — apply runner input movies before frame latch.
3. **Task 2:** `b2b61c9` — cover input movie bounds and refusals.

## Files Created/Modified

- `runner/inputs.c`, `runner/inputs.h` — bounded parser, frame ordering, validation, and pre-latch application.
- `runner/main.c`, `runner/CMakeLists.txt` — `--inputs FILE` option and runner integration.
- `tests/runner/tests.cmake`, `tests/runner/inventory.txt` — real CLI tracer and control coverage registered in CI.
- `docs/runner.md` — complete script grammar, examples, bounds, and exit behavior.

## Decisions Made

- Kept all script file access in the runner; the core receives only validated snapshots through its existing API.
- Kept phase 4 API/state/behavior revision 4 and used `--hash-state` to prove latch input while preserving the no-content frame pin.
- Left guest `pad_counter`, pixel, and polling-lag proof to dependent plan 04-36.

## Deviations from Plan

None — the narrowed plan executed as written.

## TDD Gate Compliance

- **RED:** `runner.inputs.script` ran and failed on the planned CLI assertion: the pre-implementation runner returned usage status 2 for unknown `--inputs`. `gsd-tools check tdd-red-evidence build/ci/tdd-red-04-18-task1.json --raw` returned `RED_EVIDENCE_OK`; the target assertion ran, with no discovery, setup, or fixture failure.
- **GREEN:** Commit `07ebaf4` followed the RED commit. The focused tracer passed before commit and again at the committed head.
- **Task 2:** Added regression-only boundary and refusal coverage for the behavior implemented by the tracer; no second RED/GREEN production cycle was needed.

## Evidence

- `cmake --preset ci && cmake --build --preset ci --target schwaystation-run && ctest --test-dir build/ci -R '^runner\.inputs\.script$' --no-tests=error --output-on-failure` — exit 0; the event changes the saved state only at its frame, repeated runs match, and the frame hash stays pinned.
- `cmake --preset ci && cmake --build --preset ci --target schwaystation-run && ctest --test-dir build/ci -R '^runner\.inputs\.controls$' --no-tests=error --output-on-failure` — exit 0; endpoints pass and malformed inputs are refused before any frame output.
- `cmake --workflow --preset ci` — exit 0; all 452 CI tests and both release archive checks passed.
- `cmake --workflow --preset hygiene` — exit 0; all 23 hygiene checks passed.
- `git diff --check` — exit 0.

## User Setup Required

None.

## Next Plan Readiness

Plan 04-36 retains the guest `pad_counter`, visible pixel, held/edge count, and lag proof. This plan provides its runner script input path.

## Self-Check: PASSED

- Summary exists at the expected phase path.
- All three task commits are ancestors of the current HEAD.
