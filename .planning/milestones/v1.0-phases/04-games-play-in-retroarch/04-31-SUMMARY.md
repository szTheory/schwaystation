---
phase: 04-games-play-in-retroarch
plan: "31"
subsystem: testing
tags: [fuzz, replay, ctest, asan, wasm]
requires:
  - phase: "04-58"
    provides: registered GPU interrupt and DMA behavior exercised by the hostile replay cases
provides:
  - Deterministic GPU, SIO and memory-card replay regressions with independent state and memory assertions
affects: [04-48, 04-29, 04-32, phase-04-verification]
actuals:
  tokens: 11105
  tasks: 2
  commits: 8
plan_head_before: 84d1274991dccd0418ad5c60253befeb00d6f064
plan_head_after: 2f839fd29819d3d32a7ade8f0348db8fc6a1b1c5
tech-stack:
  added: []
  patterns:
    - Reuse existing replay drivers and deterministic seed records for hostile inputs
    - Register replay cases in portable and sanitizer inventories
key-files:
  created: []
  modified:
    - tests/fuzz/gpu.c
    - tests/fuzz/sio.c
    - tests/fuzz/memcard.c
    - tests/fuzz/seeds.c
    - tests/fuzz/seeds.h
    - tests/fuzz/tests.cmake
    - tests/fuzz/inventory.txt
    - .planning/phases/04-games-play-in-retroarch/04-31-CHECKPOINT.md
key-decisions:
  - "Extend the existing replay infrastructure without adding a harness, dependency, quota or core behavior."
patterns-established:
  - "Hostile deterministic replays assert bounded progress and independent state/memory outcomes, not only absence of a crash."
requirements-completed: []
coverage:
  - id: D1
    description: GPU, SIO and memory-card hostile replay inputs verify bounded behavior and independent expected state, pixels, bytes and generations.
    requirement: GAME-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^fuzz\\.regress\\.(gpu|sio|memcard|exe)$' --no-tests=error --output-on-failure --parallel 4; exit 0, four registered replay cases executed."
        status: pass
      - kind: integration
        ref: https://github.com/szTheory/schwaystation/actions/runs/43404686338
        status: pass
    human_judgment: false
  - id: D2
    description: Replay regressions remain in portable, ASan, WASM and BE32 CI selections.
    requirement: GAME-03
    verification:
      - kind: integration
        ref: "Exact-head CI run 43404686338; ASan, WASM and BE32 lanes passed."
        status: pass
    human_judgment: false
duration: "not measured; checkpointed implementation and deferred acceptance"
completed: 2026-11-13
status: complete
---

# Phase 4 Plan 31: Hostile Replay Regression Summary

**Deterministic GPU, SIO and memory-card replays now assert bounded progress and independent state and memory outcomes.**

## Performance

- **Duration:** Not measured; the implementation was integrated from a checkpoint and acceptance waited for shared exact-head CI.
- **Started:** Original active start was not retained.
- **Completed:** 2026-11-13
- **Tasks:** 2
- **Files modified:** 9, including the checkpoint and contemporaneous state record.
- **Runtime/model:** Codex/OpenGSD 1.20.0; gpt-6-luna xhigh.

## Accomplishments

- Added hostile GPU replays for continued restored packets, exact pixels, clipping/overflow boundaries, split-run equivalence, transfers, resets, DMA request edges and instance ownership.
- Added SIO and card replay cases for ACK timing, watchdog and abort boundaries, empty FIFO/read pause, sector validity, staged/committed bytes, checksum refusal and generations.
- Kept the existing replay drivers and seed format; registered the cases in ordinary, portable, ASan and WASM inventories.

## Task Commits

1. **Task 1: Add deterministic GPU, SIO and card replay cases** — `06df362` (`test`)
2. **Task 2: Expand replay assertions and applicable registrations** — included in `06df362` (`test`)

The persisted measurement ledger begins at the parent of `06df362`. The measured interval contains eight commits because canonical acceptance was deferred while subsequent plan work completed; `06df362` is the plan31 implementation commit.

## Files Created/Modified

- `tests/fuzz/gpu.c`, `tests/fuzz/sio.c`, `tests/fuzz/memcard.c` — deterministic subsystem replay assertions.
- `tests/fuzz/seeds.c`, `tests/fuzz/seeds.h` — shared input and replay seed definitions.
- `tests/fuzz/tests.cmake`, `tests/fuzz/inventory.txt` — registered and byte-sorted replay cases.
- `.planning/phases/04-games-play-in-retroarch/04-31-CHECKPOINT.md` — retained implementation and focused-check evidence.

## Decisions Made

- Reused the existing replay drivers and deterministic seeds, with no new harness, dependency, lifetime counter or core behavior.
- Kept the hostile input cases gating across the existing applicable CI selections.

## Deviations from Plan

None in the implementation. The source commit was integrated before the canonical summary; its acceptance was deferred until the shared exact-head hosted run supplied the applicable CI and sanitizer evidence.

## Evidence

- Focused local registered replay selection exited 0 with all four cases (GPU, SIO, card and the unchanged EXE replay) executed.
- Exact-head hosted run [43404686338](https://github.com/szTheory/schwaystation/actions/runs/43404686338) completed successfully; the replay cases passed in ordinary CI, ASan, WASM and BE32 selections.

## User Setup Required

None.

## Next Phase Readiness

This hostile-replay slice is complete. Plan 29 and plan 32 closeout plus phase verification remain; stop after phase verification before phase 4 ship or phase 5.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-13*

## Self-Check: PASSED

The summary file exists, and the plan31 implementation commit is an ancestor of the recorded plan head.
