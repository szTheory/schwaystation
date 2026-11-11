---
phase: 04-games-play-in-retroarch
plan: "19"
subsystem: runner
tags: [memory-cards, host-files, signals, ctest]
requires:
  - phase: "04-10"
    provides: "Runner video bounds, output behavior, and user-facing runner documentation"
  - phase: "04-12"
    provides: "Deterministic GPU and timer fixture expectations used by runner CI"
  - phase: "04-35"
    provides: "Shared host card import, canonical paths, and atomic save helpers"
  - phase: "04-36"
    provides: "Guest input and SIO behavior exercised by the runner"
provides:
  - "Runner imports canonical memory cards before execution and flushes only completed dirty generations."
  - "Normal and clean SIGTERM exits persist successful card writes while failed saves preserve the prior file."
  - "Gating tests cover imports, per-slot writes, incomplete and rejected transactions, naming, and save failures."
affects: ["04-37", runner, memory-card-persistence]
actuals:
  tokens: 9880
  tasks: 2
  commits: 3
commits: 3
plan_head_before: c86f67be6bda6772043859fd96ae4431f937c4cf
plan_head_after: 07ded34c0fa7393bba2c8c6a0c43b349c7317068
tech-stack:
  added: []
  patterns:
    - "The runner uses the host card helper for file I/O and core generation APIs to decide which slots need saving."
key-files:
  created: []
  modified:
    - runner/main.c
    - runner/exit_status.h
    - tests/runner/tests.cmake
    - tests/runner/inventory.txt
    - docs/runner.md
key-decisions:
  - "Default card basenames come from the executable content name, or schwaystation for a content-free run; serial naming requires an explicit validated serial."
  - "Only a completed successful card Write that advances that slot's generation triggers a save."
patterns-established:
  - "Card files are imported before execution and atomically flushed by the runner after the frame loop, outside signal handlers."
requirements-completed: []
coverage:
  - id: D1
    description: "Canonical card imports and completed card writes persist on normal and clean SIGTERM exits, one slot at a time."
    requirement: GAME-04
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^runner\\.cards\\.(persistence|sigterm)$' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: "Missing and invalid imports, unchanged cards, incomplete and rejected writes, naming rules, and failed atomic saves have bounded outcomes."
    requirement: GAME-04
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^runner\\.cards\\.controls$' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
  - id: D3
    description: "Runner documentation and the runtime exit-status table agree about card read and write failures."
    requirement: GAME-04
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^runner\\.exit_table$' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
duration: 29min
completed: 2026-11-11
status: complete
---

# Phase 04 Plan 19: Runner Card Persistence Summary

**Runner memory cards now load before execution and save only successful per-slot generations, including after a clean signal stop.**

## Performance

- **Duration:** 29 min
- **Started:** 2026-11-11T12:14:47Z
- **Completed:** 2026-11-11T12:44:02Z
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments

- Added `--save-dir`, default content-name paths, and explicit serial-name paths for the runner.
- Imported both slots before execution and saved only slots whose successful card writes advanced their generations.
- Verified that clean SIGTERM finishes its current frame and flushes completed writes, while failed atomic saves preserve the old card.

## Task Commits

Each task was committed atomically:

1. **Task 1: Load runner card files and flush completed generations** - `2ea5f34` (feat)
   - TDD RED contract: `2ec6215` (test)
2. **Task 2: Expand and document the proven path** - `07ded34` (test)

The plan ledger measured three task commits from `c86f67be6bda6772043859fd96ae4431f937c4cf` through `07ded34c0fa7393bba2c8c6a0c43b349c7317068`.

## Files Created/Modified

- `runner/main.c` - Runner card imports, generation-aware flushes, and test-only SIO write controls.
- `runner/exit_status.h` - Exit status 1 includes card file read and write failures.
- `tests/runner/tests.cmake` - Persistence, signal, import, generation, naming, and failure controls.
- `tests/runner/inventory.txt` - Sorted registration for `runner.cards.controls`.
- `docs/runner.md` - Card path, import, save, and failure behavior.

## Decisions Made

- The default basename is the executable content name with its final extension removed, or `schwaystation` for a content-free run. Serial naming requires the caller to provide a validated serial.
- The runner saves a slot only after a successful card Write advances its generation; signal handlers only request a stop.
- GAME-04 remains partial until plan 04-37 supplies the fresh-process guest card proof.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Updated the runtime failure description for card file errors**
- **Found during:** Task 1 verification
- **Issue:** The runner help still described status 1 as write-only, while card imports can also fail before execution.
- **Fix:** Updated the shared exit-status table to describe card reads and writes and kept the documentation in sync.
- **Files modified:** `runner/exit_status.h`, `docs/runner.md`
- **Verification:** `runner.exit_table` passed, followed by the full CI workflow.
- **Committed in:** `2ea5f34`

**Total deviations:** 1 auto-fixed (Rule 1)
**Impact on plan:** The correction kept runtime help, documentation, and new error behavior consistent.

## Issues Encountered

- The first `runner.exit_table` check found stale runtime help text; after correcting it, the focused check passed.
- The first rejected-write-after-success control used the wrong expected card status flag. The test hook expectation was corrected, and `runner.cards.controls` passed.

## TDD Gate Compliance

- **RED:** `runner.cards.persistence` ran before implementation and failed because the runner rejected `--save-dir` as an unknown option (CTest exit 8; target returned usage status 2). `gsd-tools check tdd-red-evidence` returned `RED_EVIDENCE_OK`; semantic assessment confirmed the intended behavior failure.
- **GREEN:** `2ea5f34` implemented the save path; the committed tracer rerun passed `runner.cards.persistence` and `runner.cards.sigterm`.
- **Expansion:** `07ded34` added the already-required control cases without reclassifying a passing expansion as RED.

## Evidence

- `cmake --workflow --preset ci` — exit 0; all 457 CI tests and both release archive checks passed.
- `cmake --workflow --preset hygiene` — exit 0; all 23 hygiene and policy checks passed.
- `ctest --test-dir build/ci -R '^runner\\.cards\\.controls$' --no-tests=error --output-on-failure` — exit 0.
- `ctest --test-dir build/ci -R '^runner\\.cards\\.(persistence|sigterm)$' --no-tests=error --output-on-failure` — exit 0, including the post-commit tracer rerun.
- `ctest --test-dir build/ci -R '^runner\\.exit_table$' --no-tests=error --output-on-failure` — exit 0.
- These are local automated runs; there is no hosted run URL or user-hardware claim in this plan.

## User Setup Required

None.

## Next Phase Readiness

Plan 04-19 is complete. The runner persistence path is ready for the separate fresh-process guest card proof in plan 04-37; no owner action is needed.

## Self-Check: PASSED

- Summary and all five modified implementation, test, inventory, and documentation files exist.
- Task commits `2ec6215`, `2ea5f34`, and `07ded34` are ancestors of the current plan head.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-11*
