---
phase: 14-multitap-and-special-controllers
plan: "02"
subsystem: sio-input
tags: [api-v9, multitap, controllers, save-state]
requires:
  - phase: 14-multitap-and-special-controllers
    provides: Plan 14-01 selector registration and phase input contract
provides:
  - API v9 size-bounded controller input preserving the exact v8 prefix
  - Eight independent frame-latched controller endpoint snapshots
  - State format and behavior revision 20 with endpoint input state
affects: [phase-14-multitap-and-special-controllers]
actuals:
  tokens: 16095
  tasks: 3
  commits: 4
commits: 4
plan_head_before: f6538f0f0f5d1cc46fa21b3d2d1562ff4c9de2c3
plan_head_after: 72e2c63bf302e5986fe19255c728fd015e3bd680
tech-stack:
  added: []
  patterns:
    - Caller-sized input copied into a zeroed value before validation
    - Stable endpoint indices and frame-start input snapshots
key-files:
  created: []
  modified:
    - include/schwaystation.h
    - src/frame.c
    - src/internal.h
    - src/pad.h
    - src/pad.c
    - tests/api/v9.c
    - tests/core/test_core.c
    - tests/sio/pad_tests.c
    - tests/core/test_state.c
    - docs/embedding.md
key-decisions:
  - Keep the 28-byte v8 input prefix fixed and use 28 as the literal v9 minimum.
  - Resolve tap precedence once at frame start; keep run-result rumble tied to socket A.
  - Store signed mouse deltas in a deterministic offset representation in saved state.
requirements-completed: []
coverage:
  - id: D1
    description: API v9 accepts the frozen v8 input prefix and validates caller-sized extensions atomically.
    requirement: SIO-04
    verification:
      - kind: unit
        ref: tests/api/v9.c#abi.api_v9
        status: pass
      - kind: unit
        ref: tests/core/test_core.c#core.api.size_tags
        status: pass
    human_judgment: false
  - id: D2
    description: Eight controller endpoints latch independently and round-trip with format 20.
    requirement: SIO-01
    verification:
      - kind: unit
        ref: tests/sio/pad_tests.c#sio.pad.input_latch
        status: pass
      - kind: unit
        ref: tests/core/test_state.c#core.state.round_trip
        status: pass
      - kind: unit
        ref: tests/core/test_state.c#core.state.rejects
        status: pass
    human_judgment: false
metrics:
  duration: 36min
  completed: 2026-11-24
status: complete
---

# Phase 14 Plan 02: API v9 input and endpoint state summary

**API v9 adds a bounded 28-byte-compatible input snapshot and eight independently latched controller endpoints under state format 20.**

## Performance

- **Runtime/model:** Codex GSD executor, configured `gpt-6-luna` xhigh; primary runtime model ID unavailable.
- **Duration:** approximately 36 minutes wall time; active time was not separately measured.
- **Started:** 2026-11-24T03:23:01Z
- **Completed:** 2026-11-24T03:59:00Z (approximate closeout timestamp)
- **Tasks:** 3
- **Files modified:** 10

## Accomplishments

- Added API v9 input fields and validation while preserving the exact 28-byte API v8 prefix. Inputs are copied only up to the caller's size into a zero-initialized value, then validated before the pending snapshot changes.
- Added eight endpoint states using the stable `p1A=0`, `p2A=1`, `p1B/C/D=2/3/4`, `p2B/C/D=5/6/7` mapping. Tap precedence is resolved at frame start, and later host input cannot change an in-progress frame.
- Serialized the latched endpoint inputs under format and behavior revision 20, rejected format 19, and documented the API compatibility and endpoint fields.

## TDD Evidence

- **RED:** The `abi.api_v9` test executed against the new layout and failed only because `schway_set_input` returned `SCHWAY_STATUS_ERR_STRUCT_SIZE` for the required 28-byte v8 caller. The JUnit classifier returned `RED_EVIDENCE_OK`; evidence is in the ignored `.gsd/tdd-evidence/14-02/red.json`.
- **GREEN:** The bounded-copy setter accepted all byte-sized prefixes from 28 through 228 bytes and rejected invalid fields and nonzero unknown tails without replacing the prior pending input.
- **Final focused verification:** 11/11 tests passed across API v1–v9, API version, input size tags, the eight-endpoint latch, independent instances, DualShock config, state round-trip, state rejection and state visitor selectors. The CTest run took 0.53 seconds.

## Verification Evidence

- `git diff --check` — exit 0.
- `LC_ALL=C sort -cu tests/sio/inventory.txt` — exit 0; no test inventory needed an edit.
- Exact selector preflight with `ctest --preset ci --show-only=json-v1` and the listed API, SIO and state selectors — exit 0; no required selectors were missing.
- `cmake --preset ci` — exit 0.
- `cmake --build --preset ci --target schwaystation-test-api-v1 schwaystation-test-api-v2 schwaystation-test-api-v3 schwaystation-test-api-v4 schwaystation-test-api-v5 schwaystation-test-api-v6 schwaystation-test-api-v7 schwaystation-test-api-v8 schwaystation-test-api-v9 schwaystation-test-core schwaystation-test-sio-pad schwaystation-test-state` — exit 0.
- `ctest --preset ci --no-tests=error -R '^(abi[.]api_(v9|version)|core[.]api[.]size_tags|core[.]state[.](round_trip|rejects|visit)|sio[.]pad[.](input_latch|independent_ports_instances|config_commands))$' --output-on-failure` — exit 0; 11/11 passed.
- `ctest --preset ci --no-tests=error -R '^abi[.]api_v[1-9]$' --output-on-failure` — exit 0; all nine frozen API caller programs passed.
- The planned RED command `ctest --preset ci --no-tests=error -R '^abi[.]api_v9$' --output-on-failure --output-junit ../../.gsd/tdd-evidence/14-02/red-report.xml` — expected exit 8; the target assertion observed status 2 instead of OK. `gsd_run check tdd-red-evidence .gsd/tdd-evidence/14-02/red.json --raw` — exit 0, `RED_EVIDENCE_OK`.
- No hosted CI or Act run was needed for this plan because workflow wiring did not change. No Docker resources were created.

## Task Commits

1. **Task 1 RED:** `66e47f6` — pin v8 input compatibility on API v9.
2. **Task 1 GREEN:** `3ba1b90` — accept bounded API v9 input snapshots.
3. **Task 2:** `491b837` — latch eight controller endpoints per frame.
4. **Task 3:** `72e2c63` — version eight endpoint state as format 20.

Plan commit count and heads were measured from the persisted plan ledger: 4 commits from `f6538f0f0f5d1cc46fa21b3d2d1562ff4c9de2c3` through `72e2c63bf302e5986fe19255c728fd015e3bd680`. The separate metadata commit is recorded in the executor closeout.

## Decisions Made

- Socket A keeps the legacy fields when a tap is off; appended socket-A fields contribute only special-controller input in that mode.
- Pending host input is excluded from save states; current latched endpoint values and protocol state are serialized.
- Public run-result rumble remains tied to the two socket-A endpoints.

## Deviations from Plan

None. The existing compile-time assertion for the 28-byte struct size was updated to pin the released prefix and the new full layout before the expected RED run.

## Remaining Phase Scope

This plan delivers the shared input and state foundation only. Phase requirements SIO-01 through SIO-04 remain pending until their serial protocols and host mappings are implemented and verified by the remaining Phase 14 plans.

## Next Phase Readiness

Plan 14-03 can build serial multitap transport on the stable input and endpoint contract. No owner action is needed. This executor stops here and does not start Plan 03 or phase verification.

## Self-Check: PASSED

- The summary file exists at the planned path.
- All four measured task commits are ancestors of the current plan head.
- The worktree was clean before summary and state bookkeeping.

---
*Phase: 14-multitap-and-special-controllers*
*Completed: 2026-11-24*
