---
phase: 04-games-play-in-retroarch
plan: "33"
subsystem: card
tags: [sio, memory-card, transaction, state]

# Dependency graph
requires:
  - phase: 04-16
    provides: Stable card spans, card identification, and SIO read timing
provides:
  - Staged 57h card writes that commit only after successful terminal 47h
  - Gating protocol tests for successful, rejected, and interrupted writes
affects: [card, sio, state, emulator-save-behavior]

# Actuals use chars/4 over the realized plan diff.
actuals:
  tokens: 7867
  tasks: 2
  commits: 4
plan_head_before: 71d2108e0fc5e4349339cb1901affa17eb747e03
plan_head_after: efb6d88437b54fbb852019c9b936868a403d8ecd

# Tech tracking
tech-stack:
  added: []
  patterns:
    - Per-slot staged write payload remains instance-owned until terminal success
    - State snapshots preserve in-flight card transactions

key-files:
  created: []
  modified:
    - src/card.c
    - src/card.h
    - src/state.c
    - tests/card/card_tests.c
    - tests/card/tests.cmake
    - tests/card/inventory.txt
    - tests/fixtures/tests.cmake
    - docs/hardware/sio.md
    - docs/runner.md

key-decisions:
  - "Stage the 128-byte write per slot and commit one sector plus one generation only on terminal 47h."
  - "Keep the SIO write phase and payload in serialized instance state so an in-flight transaction resumes deterministically."

patterns-established:
  - "Card write mutations are transactional: protocol errors and interruption preserve committed bytes and generation."

requirements-completed: []
coverage:
  - id: D1
    description: "SIO card writes expose old bytes until successful final 47h, then commit one sector and generation; invalid and interrupted writes preserve committed data."
    requirement: GAME-04
    verification:
      - kind: unit
        ref: "card.protocol.write_commit and card.protocol.write_edges (CTest)"
        status: pass
    human_judgment: false

duration: 35min
completed: 2026-11-11
status: complete
---

# Phase 4 Plan 33: Transactional Card Writes Summary

**SIO 57h writes now stage a complete sector and update card bytes and generation only after the successful final 47h response.**

## Performance

- **Duration:** 35 min
- **Started:** 2026-11-11T08:38:05Z
- **Completed:** 2026-11-11T09:12:39Z
- **Tasks:** 2
- **Files modified:** 9

## Accomplishments

- Implemented bounded SIO write staging, checksum and address rejection, and atomic commit at the established terminal response.
- Added registered MMIO protocol coverage for mid-write state save/load, first and last sectors, both slots, repeated successes, checksum failure, reset, and deselection.
- Updated hardware behavior documentation and the affected interim revision-4 state hash expectations.

## Task Commits

1. **Task 1 RED:** `087630f` — add failing staged card write transaction test
2. **Task 1 GREEN:** `cce1570` — implement staged SIO card writes
3. **Task 2:** `a5228c0` — cover staged card write edge cases
4. **Integration expectations:** `efb6d88` — refresh staged card state expectations

## Files Created/Modified

- `src/card.c`, `src/card.h` — per-slot staged transaction and terminal commit.
- `src/state.c` — state layout annotation for staged payload.
- `tests/card/card_tests.c`, `tests/card/tests.cmake`, `tests/card/inventory.txt` — registered protocol tests and byte-sorted inventory.
- `tests/fixtures/tests.cmake` — revision-4 state hashes for frames 150 and 180.
- `docs/hardware/sio.md` — write protocol and modeled failure behavior.
- `docs/runner.md` — updated frame-150 state hash example.

## Decisions Made

- Stage all 128 bytes separately from live card data and commit exactly once on terminal success.
- Preserve the staged phase and payload in instance state; reset or deselection discards only the incomplete transaction.
- Keep `requirements-completed` empty because this plan completes only a slice of GAME-04; dependent phase plans still implement the rest of that requirement.

## Evidence

- RED: `ctest --test-dir build/ci -R '^card\.protocol\.write_commit$' --no-tests=error --output-on-failure --output-junit card-write-red.xml` exited 8 because the new protocol assertion observed the unsupported 57h path returning FFh without ACK. The target case ran and failed at the expected protocol assertions; it was not a build, discovery, or fixture failure. Evidence is recorded in `build/ci/card-write-red.xml` and `build/ci/card-write-red-evidence.json`; `gsd-tools check tdd-red-evidence` returned `RED_EVIDENCE_OK`.
- GREEN: `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-card && ctest --test-dir build/ci -R '^card\.protocol\.write_commit$' --no-tests=error --output-on-failure` exited 0.
- Edge tests: the equivalent configure/build flow selecting `^card\.protocol\.write_edges$` exited 0.
- Integration checks: `ctest --test-dir build/ci -R '^(card\.protocol\.read_edges|fixtures\.gpu_motion\.state\.(150|180))$' --no-tests=error --output-on-failure` exited 0. Runner checks for frames 150 and 180 exited 0 and produced the updated revision-4 state hashes.
- Full CI: `cmake --workflow --preset ci` exited 0; all 444 CI tests and both release archive tests passed.
- Hygiene: `cmake --workflow --preset hygiene` exited 0; all 23 checks passed.

## TDD Gate Compliance

- Task 1 RED was the focused `card.protocol.write_commit` run above. Its actual assertion failures confirm the pre-implementation 57h rejection path. The evidence checker accepted the JUnit report and semantic assessment.
- Task 1 GREEN passed the same registered protocol test after implementation.
- Task 1 commits preserve RED before GREEN: `087630f`, then `cce1570`.
- Task 2 expanded already implemented behaviors and was not treated as a second RED cycle.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Updated the old unsupported-command control after 57h became supported.**
- **Found during:** Full CI integration
- **Issue:** An existing read-edge test used 57h as its unsupported command control.
- **Fix:** Changed that control to 54h so the test continues to exercise an unsupported command.
- **Files modified:** `tests/card/card_tests.c`
- **Verification:** Read-edge and focused write tests passed; final full CI passed.
- **Committed in:** `a5228c0`

**2. [Rule 1 - Bug] Refreshed interim revision-4 state hashes after serializing staged transaction state.**
- **Found during:** Full CI integration
- **Issue:** The additive in-flight write fields change content-state hashes for the affected fixture frames.
- **Fix:** Recorded the new frame-150 and frame-180 state hashes and refreshed the runner example. State/API revision remains 4; released API versions and no-content pins are unchanged.
- **Files modified:** `tests/fixtures/tests.cmake`, `docs/runner.md`
- **Verification:** Both runner hash commands and affected fixture tests passed; final CI and hygiene passed.
- **Committed in:** `efb6d88`

**Total deviations:** 2 auto-fixed issues (Rule 1)
**Impact on plan:** Both changes keep existing regression controls accurate after the planned behavior and state additions. No unrelated scope was added.

## Issues Encountered

The first full CI run identified the two existing expectations documented above. Both were corrected, and the final complete CI and hygiene workflows passed.

## User Setup Required

None.

## Next Phase Readiness

The card-write protocol slice is complete. Plan 04-34 retains the separate import/state integration work; this plan does not absorb that dependent scope.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-11*

## Self-Check: PASSED

- SUMMARY exists and its tracked key files are present.
- All four task commits are ancestors of the current HEAD.
- The worktree diff is whitespace-clean, and no task-file stub markers were found.
