---
phase: 14-multitap-and-special-controllers
plan: "07"
subsystem: controller-state
tags: [multitap, serial-io, save-state, memory-card]
requires:
  - phase: 14-multitap-and-special-controllers
    provides: Eight controller endpoints, multitap/card protocol visitors, format-20 state
provides:
  - Atomic cross-validation of saved controller topology and in-flight protocol state
  - Eight-card continuation checks proving receiver-owned media and generations survive restore
  - Documented distinction between API, state-format and behavior revisions
affects: [state, controllers, multitap, memory-card]
actuals:
  tokens: 9007
  tasks: 2
  commits: 4
commits: 4
plan_head_before: 6fb876b13c7352e146709d768d99dcc871dce6e0
plan_head_after: c0e2b82a8bd751f799238cdaab37c6f4ef35dd4f
tech-stack:
  added: []
  patterns: [validate candidate state before apply, preserve receiver-owned persistent media]
key-files:
  created: []
  modified:
    - src/card.c
    - src/pad.c
    - src/sio.c
    - src/state.h
    - tests/card/card_tests.c
    - tests/core/test_state.c
    - docs/embedding.md
key-decisions:
  - Keep card bytes and generation counters receiver-owned; save only protocol progress.
  - Validate SIO, pad and card transaction combinations against the candidate serialized state before apply.
  - Keep format 20 and behavior revision 20; this plan changes no serialized field layout.
patterns-established:
  - Candidate-state visitors cross-check topology and active transactions before mutating the instance.
  - Persistent card media is tested independently from resumable serial protocol state.
requirements-completed: [SIO-01, SIO-03]
coverage:
  - id: D1
    description: Eight card endpoints resume split protocol transfers without restoring card images or generations.
    requirement: SIO-01
    verification:
      - kind: unit
        ref: "ctest card.tap.state"
        status: pass
    human_judgment: false
  - id: D2
    description: CRC-valid impossible topology, device, tap and card transaction combinations reject atomically.
    requirement: SIO-01
    verification:
      - kind: unit
        ref: "ctest core.state.rejects"
        status: pass
      - kind: unit
        ref: "ctest card.tap.state"
        status: pass
    human_judgment: false
  - id: D3
    description: Supported controller endpoint input state round-trips and the format-20 serialized bytes remain stable.
    requirement: SIO-02
    verification:
      - kind: unit
        ref: "ctest core.state.round_trip"
        status: pass
      - kind: unit
        ref: "ctest core.state.stable_bytes"
        status: pass
    human_judgment: false
  - id: D4
    description: Tap reply history and pending byte/ACK deadlines restore through the focused SIO state cases.
    requirement: SIO-03
    verification:
      - kind: unit
        ref: "ctest sio.tap.history_edges"
        status: pass
      - kind: unit
        ref: "ctest sio.state.pending_byte and sio.state.pending_ack"
        status: pass
    human_judgment: false
# Metrics
duration: 26m10s
completed: 2026-11-24
status: complete
---

# Phase 14 Plan 07: Aggregate Endpoint State Summary

**Controller and card state restore now validates cross-device transactions before apply and preserves receiver-owned card media across all eight endpoints.**

## Performance

- **Duration:** 26m10s wall time (active time not separately measured)
- **Started:** 2026-11-24T06:23:04Z
- **Completed:** 2026-11-24T06:49:14Z
- **Tasks:** 2
- **Files modified:** 7
- **Runtime/model:** Codex GSD executor, configured `gpt-6-luna` xhigh; primary runtime model ID unavailable

## Accomplishments

- Tested saved card protocol continuation across all eight sockets, including staged writes and terminal-byte handling. State load preserves every receiving card's bytes and generation; only a newly completed valid write changes the selected receiver card.
- Added candidate-state validation for controller topology, device presence, pad phase/index, mouse latch bounds, tap endpoint mapping and active SIO/card transaction combinations. Invalid CRC-valid states fail before changing the instance.
- Confirmed endpoint round-trip, reset, visitor and stable-byte behavior. The existing producer reports `state_bytes=3757638`; no state-size or quality bound was changed.
- Updated embedding documentation to distinguish API v9, state format 20 and behavior revision 20 and to describe protocol-only card restore.

## Task Commits

1. **Task 1 RED:** `4412a0f` — reject inconsistent active card transaction state.
2. **Task 1 GREEN:** `139fd5f` — cross-validate candidate card transaction state.
3. **Task 2 RED:** `d1bb44c` — reject impossible aggregate endpoint state.
4. **Task 2 GREEN:** `c0e2b82` — validate aggregate endpoint state.

The plan metadata commit is recorded separately after GSD bookkeeping.

## Verification Evidence

- `LC_ALL=C sort -cu tests/card/inventory.txt tests/sio/inventory.txt tests/core/inventory.txt` — exit 0.
- `git diff --check` — exit 0.
- `cmake --preset ci` — exit 0.
- Required selector preflights confirmed the Task 1 and Task 2 CTest registrations before builds — exit 0.
- Focused builds for `schwaystation-test-card`, `schwaystation-test-sio`, `schwaystation-test-state`, `schwaystation-test-api-v9` and `schwaystation-test-sio-pad` — exit 0.
- Combined focused selection for card continuation, state validation/round-trip, API v9, Tap history and pending SIO events — 15/15 passed in 3.67 seconds (including two CTest fixture setup cases).
- The corrected `core.state.round_trip` selector and its fixture setup — 3/3 passed in 0.23 seconds.
- Both malformed-state TDD records were classified `RED_EVIDENCE_OK` before their corresponding fixes. Evidence files are ignored under `.gsd/tdd-evidence/14-07/`.
- No workflow files changed, so no broad workflow, Act or hosted CI run was needed. No Docker resources were created.

## Decisions Made

- State load resumes protocol progress while retaining receiver-owned card images and generation counters.
- Cross-field checks read the candidate serialized state rather than stale values in the receiving instance.
- State format and behavior revision remain at 20 because the current plan changes validation behavior, not the serialized layout.

## Deviations from Plan

### Auto-fixed issues

**1. [Rule 2 - Missing correctness validation] Validate candidate state across pad, tap and card visitors**
- **Found during:** Task 1 and Task 2
- **Issue:** The CRC-valid malformed-state regressions showed that visitor checks could consult receiving-instance values instead of candidate serialized values and did not reject several impossible cross-device combinations.
- **Fix:** Added bounded candidate validation fields and cross-checks for active card/tap transactions, topology, endpoint mapping, controller phase/index and mouse latch ranges before apply.
- **Files modified:** `src/card.c`, `src/pad.c`, `src/sio.c`, `src/state.h`
- **Verification:** `core.state.rejects`, `card.tap.state`, and the full focused 15-test selection passed.
- **Committed in:** `139fd5f`, `c0e2b82`

**2. [Rule 1 - Test bug] Correct aggregate round-trip input expectations**
- **Found during:** Task 2 focused state selector
- **Issue:** The test expected mouse movement and generic button polarity for non-mouse devices, while frame latching centers non-mouse deltas and mouse buttons use their device-specific mask.
- **Fix:** Made the checker derive expected button and mouse latch values from each endpoint's device type.
- **Files modified:** `tests/core/test_state.c`
- **Verification:** `core.state.round_trip` and the full focused selection passed.
- **Committed in:** `c0e2b82`

**Total deviations:** 2 correctness fixes. Both were required by focused regression evidence; no broad scope or quality gate changed.

## Issues Encountered

The first Task 2 focused run exposed the incorrect test expectation described above. Updating the expectation made the focused round-trip pass. The first GSD staging attempt was denied by the workspace's `.git` write restriction; the same scoped GSD helper then succeeded with approved elevated Git access. No source or test changes were lost.

## User Setup Required

None.

## Next Phase Readiness

Plan 14-07 is complete. Plan 14-08 remains outside this execution scope. Phase-wide review and verification remain for their authorized orchestration point.

---
*Phase: 14-multitap-and-special-controllers*
*Completed: 2026-11-24*

## Self-Check: PASSED

- SUMMARY exists at the expected phase path.
- All four task commits are ancestors of HEAD.
- The plan ledger measures four commits from `6fb876b` through `c0e2b82`.
