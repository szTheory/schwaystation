---
phase: 14-multitap-and-special-controllers
plan: "04"
subsystem: api
tags: [c, api-v9, multitap, memory-cards, sio, save-state]
requires:
  - phase: 14-multitap-and-special-controllers
    provides: Two-port multitap controller routing and protocol state from Plan 14-03
provides:
  - Eight independent one-based port/socket card spans, imports and generations
  - SIO routing for tap card addresses 81h through 84h with existing card timing
  - Protocol-only state for selected card endpoints while card bytes and generations remain host-owned
affects: [phase-14-controller-plans, card-persistence, embedding-api]
actuals:
  tokens: 13491
  tasks: 2
  commits: 4
tech-stack:
  added: []
  patterns:
    - Per-instance fixed eight-endpoint card storage and one-based public coordinates
    - SIO stores endpoint selection as protocol state and leaves card persistence bytes outside save states
key-files:
  created: []
  modified:
    - include/schwaystation.h
    - src/card.c
    - src/card.h
    - src/instance.c
    - src/sio.c
    - src/sio.h
    - tests/api/v9.c
    - tests/card/card_tests.c
    - docs/embedding.md
key-decisions:
  - "Legacy card 1 and card 2 APIs remain aliases for port 1/socket A and port 2/socket A, independent of tap configuration."
  - "Tap socket selection uses address 81h through 84h only when the tap is enabled; direct 81h remains available without a tap."
  - "Save states retain card protocol selection but never restore card image bytes or generations."
patterns-established:
  - "Map physical coordinates to a stable endpoint index and validate all coordinates before changing caller or card state."
  - "Keep terminal no-ACK and existing ACK/read-pause deadlines unchanged when routing a selected endpoint."
requirements-completed: [SIO-01]
coverage:
  - id: D1
    description: Eight independently addressable card spans and imports with legacy card aliases preserved
    requirement: SIO-01
    verification:
      - kind: unit
        ref: "abi.api_v9: public socket coordinate, import, generation and legacy alias checks"
        status: pass
    human_judgment: false
  - id: D2
    description: Tap card read/write transactions select one endpoint and preserve ACK, read-pause, terminal no-ACK, and abort behavior
    requirement: SIO-01
    verification:
      - kind: integration
        ref: "card.tap.protocol, card.tap.state, card.state.import_atomic, card.state.pending_edges, card.state.pocketstation_metadata"
        status: pass
      - kind: integration
        ref: "card.protocol.read, card.protocol.read_edges, card.protocol.write_commit, card.protocol.write_edges, card.protocol.pocketstation_metadata, sio.tap.poll_methods"
        status: pass
    human_judgment: false
duration: 39min
completed: 2026-11-24
status: complete
plan_head_before: 4489e6bfa787e44b68925687c68dff1982af988b
plan_head_after: 3158414dabfd05d82f3596e69bd5f072614f2315
commits: 4
---

# Phase 14 Plan 04: Eight Independent Tap Card Endpoints Summary

**API v9 exposes eight independent card images, and SIO routes tap card transactions to exactly one selected socket while preserving legacy aliases and save bytes.**

## Performance

- **Duration:** 38m59s wall time
- **Started:** 2026-11-24T04:41:54Z
- **Completed:** 2026-11-24T05:20:53Z
- **Tasks:** 2
- **Implementation files modified:** 9
- **Runtime/model:** Codex GSD executor, configured `gpt-6-luna` xhigh; primary runtime model ID unavailable

## Accomplishments

- Added the API v9 port/socket constants and per-endpoint access, import and generation calls for all eight cards. Legacy card 1/2 APIs still address socket A on physical ports 1/2.
- Routed enabled-tap card addresses `81h`–`84h` through the existing card protocol with exact ACK/read-pause deadlines, terminal no-ACK, and per-endpoint abort handling.
- Serialized SIO's active endpoint selection without including persistent card bytes or generations; documented the embedding API and tap behavior.

## Verification Evidence

All evidence was produced locally from the repository root. No workflow wiring changed, so no hosted CI run or Act execution was needed; no Docker resources were created.

| Check | Result |
| --- | --- |
| `git diff --check` | exit 0 |
| `LC_ALL=C sort -cu tests/card/inventory.txt` and `LC_ALL=C sort -cu tests/sio/inventory.txt` | exit 0 |
| `cmake --preset ci` | exit 0 |
| API selector preflight for `abi.api_v9` and `abi.api_version` | exit 0; both selectors present |
| `cmake --build --preset ci --target schwaystation-test-api-v9` | exit 0 |
| `ctest --test-dir build/ci --no-tests=error -R '^abi[.]api_(v9|version)$' --output-on-failure` | exit 0; selected API checks passed |
| Tap selector preflight for `card.tap.protocol`, `card.tap.state`, five existing card protocol vectors, and `sio.tap.poll_methods` | exit 0; all 8 selectors present |
| `cmake --build --preset ci --target schwaystation-test-card schwaystation-test-sio` | exit 0 |
| Focused tap and card protocol CTest selection | exit 0; 8/8 passed in 0.08s |
| Focused card state selection (`import_atomic`, `pending_edges`, `pocketstation_metadata`) | exit 0; 3/3 passed in 1.00s |
| Task 1 and Task 2 RED evidence classifier | `RED_EVIDENCE_OK` for both tasks; initial assertions demonstrated missing independent slots and missing 8n socket routing |

The exact tap/card protocol selection was:

```sh
ctest --test-dir build/ci --no-tests=error -R '^(card[.]tap[.](protocol|state)|card[.]protocol[.](read|read_edges|write_commit|write_edges|pocketstation_metadata)|sio[.]tap[.]poll_methods)$' --output-on-failure
```

The state continuation selection was:

```sh
ctest --test-dir build/ci --no-tests=error -R '^card[.]state[.](import_atomic|pending_edges|pocketstation_metadata)$' --output-on-failure
```

## Decisions Made

- The endpoint index is `(port - 1) * 4 + (socket - 1)`; this keeps both legacy card identities fixed at socket A while exposing the other six cards additively.
- An active SIO card transaction retains its selected endpoint through command bytes. A tap's controller transaction mode blocks card-address detection so controller payload is not misrouted.
- Card state fields use unique visitor paths per endpoint, preserving the existing port A field names while adding names for the six additional sockets.

## Deviations from Plan

### Auto-fixed Issues

**1. Corrected PocketStation test fixture expectation**
- **Found during:** Task 2
- **Issue:** The new metadata vector expected flag bits that its fixture had not set; the selected endpoint correctly retained zero flags.
- **Fix:** Corrected the fixture expectation to match the unchanged zero-valued metadata flags, then reran the tap and card-state selections.
- **Files modified:** `tests/card/card_tests.c`
- **Commit:** `3158414`

No product behavior or timing requirement changed.

## TDD Gate Compliance

- Task 1 RED was recorded before implementation; its API behavior assertions exposed that the socket accessors and independent slot state were absent. The RED-evidence checker returned `RED_EVIDENCE_OK`.
- Task 2 RED was recorded before routing; it failed because socket B/D addresses did not return the required card ACK. After implementation, the focused tap, existing card protocol and SIO selectors passed.
- Both tasks completed their GREEN verification and were committed separately from their RED test commits.

## Self-Check: PASSED

- Summary exists at `.planning/phases/14-multitap-and-special-controllers/14-04-SUMMARY.md`.
- All four measured plan commits are ancestors of `3158414`.
- `plan_head_before` and `plan_head_after` match the persisted plan ledger and current Plan 04 head at summary creation.
