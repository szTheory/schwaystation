---
phase: 15-guncon-and-justifier
plan: "09"
subsystem: input-state
tags: [guncon, justifier, save-state]
requires:
  - phase: 15-06
    provides:
      - Fixed-width gun input and GPU sensor state owners
provides:
  - Deterministic save-state continuation for active gun polls and beam events
  - State format and behavior revision 21 with measured exact byte bound
affects:
  - 15-10 final-source quality and WASI measurements
actuals:
  tokens: 9912
  tasks: 2
  commits: 4
tech-stack:
  added: []
  patterns:
    - Signed fixed-width fields use the same raw representation in save and visitor passes
    - Validate gun sensor phase and deadline combinations before state apply
key-files:
  created: []
  modified:
    - src/pad.c
    - src/gpu.c
    - src/internal.h
    - tests/sio/pad_tests.c
    - tests/gpu/beam.c
    - tests/core/test_state.c
    - tests/core/test_core.c
    - docs/saves.md
    - quality.json
key-decisions:
  - Pending host input remains outside serialized machine state.
  - The exact state.bytes floor and ceiling track the local stable-state producer at 3757754 bytes.
patterns-established:
  - Gun state validation rejects malformed coordinates, flags, phases and deadlines before applying any field.
  - The state visitor serializes signed gun coordinates byte-for-byte like save-state output.
requirements-completed:
  - GUN-01
  - SIO-05
coverage:
  - id: D1
    description: GunCon and Justifier input, sampled coordinates and scheduled GPU events survive save/load.
    requirement: GUN-01
    verification:
      kind: unit
      ref: ctest --preset ci --no-tests=error -R '^(gpu[.]gun[.]state|core[.]state[.]round_trip|core[.]state[.]visit|core[.]state[.]rejects)$' --output-on-failure
      status: pass
    human_judgment: false
  - id: D2
    description: An in-flight GunCon serial poll resumes at the same ACK and reply byte while pending host input stays excluded.
    requirement: SIO-05
    verification:
      kind: unit
      ref: ctest --preset ci --no-tests=error -R '^(sio[.]gun[.]state|sio[.]state[.]pending_byte|sio[.]state[.]pending_ack)$' --output-on-failure
      status: pass
    human_judgment: false
metrics:
  duration: 38m
  completed: 2026-11-25
status: complete
commits: 4
plan_head_before: 2d59f4759ae2f282ef13c3a550441e716a160b40
plan_head_after: 00f030bf10191c60b41c0407280f9586cfd5efbb
---

# Phase 15 Plan 09: In-flight Gun State Summary

**Save states resume GunCon polls and GPU beam-sensor deadlines at format 21, with host-pending input excluded.**

## Performance

- **Duration:** 38 minutes wall time, including focused implementation and local verification.
- **Started:** 2026-11-25T14:26:43Z.
- **Completed:** 2026-11-25.
- **Tasks:** 2/2.
- **Files modified:** 9.

## Accomplishments

- Serialized latched gun input, enable/results and both GPU sensor records, including exact regional event deadlines. Pending host snapshots remain host-owned and do not alter state bytes.
- Rejected invalid devices, flags, reserved values, negative active aims, sensor phases and inconsistent deadlines during validation, before applying any state field.
- Advanced state format and behavior revision from 20 to 21. Updated save-state guidance and ratcheted only the measured exact `state.bytes` bound from 3757638 to 3757754.

## Task Commits

1. **Task 1: Serialize only hardware-visible gun continuation** — `f879ecc` TDD RED probes; `e9fe132` implementation.
2. **Task 2: Update aggregate state evidence and compatibility docs** — `00f3677` aggregate tests; `00f030b` visitor fix, docs and measured bound.

The four plan commits were measured from `plan_head_before` to `plan_head_after`. The separate metadata commit records this summary and GSD state updates.

## Verification

- `cmake --preset ci` — exit 0. Cheap selector checks found every required selector exactly once.
- `ctest --preset ci --no-tests=error -R '^abi[.]api_v9$' --output-on-failure` — 1/1 passed before edits.
- Task 1 focused SIO/GPU continuation and pending-input selectors — 4/4 passed; rerun after the signed-field visitor fix also passed 4/4.
- Task 2 aggregate selectors plus the public version and no-allocation checks — 8/8 passed. The final added reset/disabled-display cases passed in `gpu.gun.state` (1/1).
- `build/ci/tests/schwaystation-test-state state.stable_bytes` — exit 0, `state_bytes=3757754`. The exact-bound comparison passed with floor and ceiling both 3757754.
- `git diff --check` — exit 0. No broad workflow, hosted CI, Act, Docker, or fuzz campaign was run in this plan.

## TDD Gate Compliance

- **Task 1 RED:** SIO and GPU continuation selectors each failed on the missing load behavior; `tdd-red-evidence` returned `RED_EVIDENCE_OK` for both reports. **GREEN:** the focused selector set passed 4/4 after serialization and revision 21 landed.
- **Task 2 RED:** the stable-state producer measured 3757754 against the prior exact 3757638 bound; the bound-comparison probe returned `RED_EVIDENCE_OK`. **GREEN:** the comparison against the measured exact bound passed, along with the eight aggregate selectors.
- A first aggregate run also exposed a visitor byte mismatch for signed gun coordinates. Saving the same raw fixed-width representation that the visitor exposes fixed it; the subsequent state and gun-state selectors passed.

## Decisions Made

- Keep host-pending input outside the machine snapshot; the host submits it through the usual input API after load.
- Track the exact producer measurement in `quality.json`; do not widen relative instruction or workflow bounds.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Matched signed gun state bytes to visitor output**
- **Found during:** Task 2 aggregate state verification.
- **Issue:** Biased encoding for signed aim coordinates made the public visitor stream differ from the saved state body.
- **Fix:** Serialize the fixed-width raw coordinate representation in both save and apply passes while exposing the same signed fields to the visitor.
- **Files modified:** `src/pad.c`, `src/gpu.c`.
- **Verification:** `core.state.visit`, `core.state.device_fields`, and the full focused aggregate set passed.
- **Committed in:** `00f030b`.

**2. [Rule 1 - Test infrastructure] Increased the state field-record test capacity**
- **Found during:** Task 2 aggregate state verification.
- **Issue:** The added serialized gun fields exceeded the test's fixed 512-field record buffer.
- **Fix:** Raised the test-only inventory capacity to 1024; production state allocation and bounds are unchanged.
- **Files modified:** `tests/core/test_state.c`.
- **Verification:** `core.state.rejects`, `core.state.visit`, and `core.state.device_fields` passed.
- **Committed in:** `00f3677`.

**Total deviations:** 2 auto-fixed issues (2 Rule 1). Both were required to keep state bytes/visitors consistent and their tests able to enumerate every field.

## Auth Gates

None.

## Known Stubs

None.

## Threat Flags

None. This change adds no network, filesystem or trust-boundary surface.

## Self-Check: PASSED

- Summary file exists at the required phase path.
- All four task commits are ancestors of the plan head recorded above.
- `actuals.commits` equals the measured four commits between the persisted plan ledger and `plan_head_after`.
