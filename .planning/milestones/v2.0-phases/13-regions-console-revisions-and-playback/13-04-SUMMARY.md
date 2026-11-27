---
phase: 13-regions-console-revisions-and-playback
plan: "04"
subsystem: state
tags: [save-state, board-profile, deterministic-hash, ctest]
requires:
  - phase: 13-03
    provides: explicit board profiles and the canonical board defaults table
provides:
  - Versioned state bytes carrying board identity
  - Atomic rejection of invalid board/GPU tuples and old state formats
affects: [phase-13-state-consumers, profile-02-verification]
actuals:
  tokens: 1875
  tasks: 2
  commits: 3
plan_head_before: 494259d1c60e725ad284cac9235bcb82a1916c10
plan_head_after: 1391c15c95cbc4c25032ba82215c50b79d61ed03
tech-stack:
  added: []
  patterns:
    - Validate serialized board tuples before applying any saved fields.
    - Reconstruct fixed board identity after APPLY while preserving mutable bus registers.
key-files:
  created: []
  modified:
    - src/internal.h
    - src/instance.c
    - src/state.c
    - tests/core/test_state.c
key-decisions:
  - "Serialize the board once and derive fixed hardware defaults from the existing defaults table."
  - "Keep bus.ram_size as mutable serialized state when restoring board identity."
requirements-completed: []
coverage:
  - id: D1
    description: Board identity round-trips through versioned state bytes and fixed board defaults are restored without replacing mutable RAM_SIZE.
    requirement: PROFILE-02
    verification:
      - kind: unit
        ref: ctest --preset ci --no-tests=error -R '^core[.]state[.](profile|round_trip|stable_bytes)$' --output-on-failure
        status: pass
    human_judgment: false
  - id: D2
    description: Invalid board tuples and the previous state format reject atomically; equal board state hashes repeat and different board identities hash differently.
    requirement: PROFILE-02
    verification:
      - kind: unit
        ref: ctest --preset ci --no-tests=error -R '^core[.]state[.](profile|round_trip|stable_bytes|rejects|visit)$' --output-on-failure
        status: pass
    human_judgment: false
duration: 17m02s
completed: 2026-11-23
status: complete
---

# Phase 13 Plan 13-04: Board Identity in Save States Summary

**Versioned save states now preserve board identity, restore fixed board defaults, and reject incompatible tuples without mutating the target.**

## Performance

- **Duration:** 17m02s of wall time, measured from the saved STATE checkpoint at 2026-11-23T17:19:44Z through final focused acceptance and the last task commit at 2026-11-23T17:36:46Z; executor start and active time were not separately captured.
- **Started:** 2026-11-23T17:19:44Z (saved STATE checkpoint; executor start not separately recorded)
- **Completed:** 2026-11-23T17:36:46Z
- **Runtime/model:** Codex GSD executor, configured `gpt-6-luna` xhigh; primary runtime model ID unavailable.
- **Tasks:** 2
- **Implementation/test files modified:** 4

## Accomplishments

- Bumped the state format and added the canonical board field to state serialization, validation, visitor output, and deterministic state bytes.
- Restored board-derived SPU revision, CD firmware, RAM boot default, and port availability after APPLY while preserving the serialized mutable `bus.ram_size`.
- Added cross-board round-trip, invalid board and GPU tuple, prior-format rejection, and board-sensitive deterministic state-byte hash coverage.
- PROFILE-02 remains a phase-level requirement and is not marked complete by this plan.

## Task Commits

1. **Task 1 RED:** `bf503b3` — test cross-board state round-trip.
2. **Task 1 GREEN:** `f7064ff` — serialize and restore board identity.
3. **Task 2 regression coverage:** `1391c15` — reject invalid board state tuples.

## Test Evidence

| Command | Exit | Duration | Result |
| --- | ---: | ---: | --- |
| LC_ALL=C sort -cu tests/core/inventory.txt | 0 | 0.00s | Core inventory sorted. |
| cmake -DROOT=. -DLIMIT=3901 '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -P tests/cmake/line_budget.cmake | 0 | 0.01s | 3,901-line gate passed. |
| cmake --preset ci | 0 | 0.81s | Task 1 CI configure passed. |
| Task 1 ctest --show-only=json-v1 state selector assertion | 0 | Not separately timed | Exactly 5 selectors including both disc fixture setups. |
| cmake --build --preset ci --target schwaystation-test-state (RED build) | 0 | 0.56s | State test target built before implementation. |
| Task 1 focused CTest selector | 8 | 0.24s | RED: 4/5 passed; core.state.profile failed because PU-7 board identity and its fixed defaults were not restored. Saved mutable RAM_SIZE did restore. |
| gsd_run check tdd-red-evidence .gsd/tdd-evidence/13-04/task1-red.json --raw | 0 | Not separately timed | RED evidence accepted for core.state.profile; 4 passed, 1 failed. |
| git diff --check, sorted inventory, and line-budget preflight | 0 | Not separately timed | Task 1 implementation hygiene passed. |
| cmake --build --preset ci --target schwaystation-test-state (Task 1 GREEN) | 0 | 0.87s | Changed-head target build passed. |
| ctest --preset ci --no-tests=error -R '^core[.]state[.](profile&#124;round_trip&#124;stable_bytes)$' --output-on-failure | 0 | 0.26s | Exact 5-name graph passed 5/5, including fixture setups. |
| Task 2 sorted inventory and 3,901-line preflights | 0 | 0.00s / 0.01s | Both passed after Task 2 test edits. |
| cmake --preset ci | 0 | 0.89s | Task 2 CI configure passed. |
| Task 2 ctest --show-only=json-v1 state selector assertion | 0 | Not separately timed | Exactly 7 selectors including both disc fixture setups; no core.state.record_sweep. |
| cmake --build --preset ci --target schwaystation-test-state (Task 2) | 0 | 0.31s | Rebuilt the changed test target after test edits. |
| ctest --preset ci --no-tests=error -R '^core[.]state[.](profile&#124;round_trip&#124;stable_bytes&#124;rejects&#124;visit)$' --output-on-failure | 0 | 0.60s | Exact selector passed 7/7. |

The Task 1 RED showed the target retained PU-18 (SPU revision 2925, CD firmware 2, RAM boot `0x0B88`) instead of restoring PU-7 (SPU revision 2922, CD firmware 0, RAM boot `0x0B80`). Task 1 GREEN fixed that path and retained the serialized mutable RAM_SIZE. Task 2 added board zero, board above the supported maximum, CRC-valid PU-7/GPU_V0 mismatch, previous-format 18, unchanged-state/audio-ring, and deterministic board-sensitive state-byte hash checks.

No broad suite, hosted CI, Act, Docker, or hardware run was needed because workflow wiring did not change. The initial RED report wrapper looked for the JUnit file at the wrong build-relative path and exited 1 after CTest; the report generated under `build/ci/build/ci/` and `LastTest.log` were used to create the checked RED record. This did not require rerunning the failing test.

## Decisions Made

- Keep one serialized board identity field and derive fixed hardware values through the canonical defaults table.
- Restore fixed board defaults only after the complete APPLY walk, so serialized mutable bus registers remain authoritative.

## Deviations from Plan

**Task 2 TDD sequencing:** Task 2 is tagged TDD, but its board/pair validator was already implemented in Task 1, whose cross-board test had a genuine RED/GREEN cycle. The new Task 2 negative cases therefore passed on their first run; no artificial RED was introduced. This sequencing exception is recorded as waived deviation 25 in `.planning/WINDOWS.md`.

**Total deviations:** 1 sequencing deviation; no behavior scope was added.

## Issues Encountered

- The first RED evidence wrapper searched the wrong CTest JUnit output location. The actual generated report and `LastTest.log` supplied the accepted evidence, and the evidence checker passed.

## User Setup Required

None.

## Next Phase Readiness

Plan 13-05 is next. PROFILE-02 remains pending until remaining Phase 13 work and final phase verification are complete.

---
*Phase: 13-regions-console-revisions-and-playback*
*Completed: 2026-11-23*

## Self-Check: PASSED

The summary file exists, all three task commits are present in git history, and the focused build and CTest evidence is recorded above.
