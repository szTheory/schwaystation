---
phase: 15-guncon-and-justifier
plan: "02"
subsystem: api
tags:
  - C API
  - ABI
  - gun input
  - size tags
requires:
  - phase: 15-01
    provides:
      - Frozen API v9 caller and registered API compatibility selectors
provides:
  - API v10 appends two direct-port gun input records after the frozen v9 prefix
  - Gun input validation rejects invalid flags, reserved bytes, coordinates and tap topology atomically
  - Public embedding documentation defines API v10 size and coordinate semantics
affects:
  - 15-03 frame latch and pad input
  - 15-05 GPU beam geometry
  - 15-07 libretro gun mapping
actuals:
  tokens: 3233
  tasks: 2
  commits: 3
commits: 3
plan_head_before: 40a73e0451ccd229ec7305aace5b9f6c3b1216f5
plan_head_after: 5fd58ae1dd1a5b5ce05eac95f9f1b0f5d48202d1
tech-stack:
  added: []
  patterns:
    - Normalize only size-tagged caller bytes into a zero-filled current input before validation.
    - Validate the full input snapshot before replacing pending host input.
key-files:
  created: []
  modified:
    - include/schwaystation.h
    - src/frame.c
    - src/internal.h
    - tests/api/v9.c
    - tests/api/v10.c
    - tests/core/test_core.c
    - docs/embedding.md
key-decisions:
  - Preserve the released 228-byte API v9 input prefix and 24-byte nested controller stride.
  - Keep validity explicit so the origin coordinate is a valid aim only when its validity flag is supplied.
  - Reject gun selection on enabled multitap topology and reject invalid snapshots before applying them.
requirements-completed:
  - GUN-01
  - GUN-02
  - SIO-05
coverage:
  - id: D1
    description: API v10 accepts explicit direct-port gun flags and coordinates, including valid aim at the origin, and rejects malformed gun input atomically.
    requirement: GUN-01
    verification:
      - kind: unit
        ref: ctest --preset ci --no-tests=error -R '^(abi[.]api_v10)$' --output-on-failure
        status: pass
    human_judgment: false
  - id: D2
    description: The released v9 caller keeps its 228-byte input prefix and frame-latched legacy pad behavior under the API v10 header.
    requirement: SIO-05
    verification:
      - kind: unit
        ref: ctest --preset ci --no-tests=error -R '^(abi[.]api_v9|abi[.]api_v10)$' --output-on-failure
        status: pass
    human_judgment: false
  - id: D3
    description: Partial gun extensions and zero-filled future tails retain the size-tag contract; nonzero reserved or future bytes are refused without replacing pending input.
    requirement: GUN-02
    verification:
      - kind: unit
        ref: ctest --preset ci --no-tests=error -R '^(core[.]api[.]size_tags|core[.]api[.]error_changes_nothing)$' --output-on-failure
        status: pass
    human_judgment: false
duration: 20m 10s
completed: 2026-11-24
status: complete
---

# Phase 15 Plan 02: Append API v10 gun input Summary

**API v10 adds two 16-byte direct-port gun records after the released 228-byte v9 input and validates the normalized snapshot before applying it.**

## Performance

- **Duration:** 20m 10s
- **Started:** 2026-11-25T03:55:52Z
- **Completed:** 2026-11-25T04:16:02Z
- **Runtime/model:** Codex/OpenGSD 1.23.0; executor gpt-6-luna
- **Tasks:** 2
- **Files modified:** 7

## Accomplishments

- Advanced the public API to v10 and appended two `schway_gun_input` records at offset 228 for a total input size of 260 bytes, preserving the 24-byte controller endpoint stride.
- Added explicit GunCon and Justifier IDs and VALID, TRIGGER, OFFSCREEN and RELOAD bits. The setter accepts an explicit valid `(0, 0)` aim and rejects invalid flags, reserved bytes, negative eligible aim and gun selection through enabled multitaps before replacing pending input.
- Extended v9 compatibility, API v10 and core size-tag tests; documented direct-port scope, coordinate origin, geometry bounds, no-clamp offscreen/reload behavior and older caller compatibility.

## Verification Evidence

- RED: `cmake --build --preset ci --target schwaystation-test-api-v10` exited 0; the intended pre-implementation `abi.api_v10` assertion failed because the header was API 9 and the setter refused the 260-byte nonzero extension (CTest exit 8). The GSD `check tdd-red-evidence .gsd/tdd-evidence/15-02/red.json --raw` classifier exited 0 with `RED_EVIDENCE_OK`.
- ABI/layout, changed inventory sort, `git diff --check`, and required selector presence/uniqueness preflights exited 0 before both configure/build sequences.
- `cmake --preset ci` exited 0 after each task's changes.
- `cmake --build --preset ci --target schwaystation-test-api-v9 schwaystation-test-api-v10 schwaystation-test-core` exited 0 after Task 1; the Task 2 rebuild of `schwaystation-test-api-v10` and `schwaystation-test-core` exited 0.
- `ctest --preset ci --no-tests=error -R '^(abi[.]api_v9|abi[.]api_v10|abi[.]api_version|core[.]api[.]size_tags|core[.]api[.]error_changes_nothing)$' --output-on-failure` exited 0 after both tasks: 5/5 passed each time.
- The broad CI workflow was not run, as directed; it remains part of the phase-level verification.

## Task Commits

1. **Append and validate the public gun extension — RED** — `80475ce` (`test(15-02): add failing API v10 input probe`)
2. **Append and validate the public gun extension — GREEN** — `ac65cea` (`feat(15-02): append validated API v10 gun input`)
3. **Document exact coordinate and compatibility semantics** — `5fd58ae` (`docs(15-02): document API v10 gun input contract`)

The summary and project tracking changes are committed separately after state updates.

## Files Created/Modified

- `include/schwaystation.h` — API v10, appended gun input type, device identifiers and flags.
- `src/frame.c` — normalized flag, reserved-byte, aim and direct-port topology validation before applying input.
- `src/internal.h` — compile-time assertions for the released prefix, endpoint stride and current structure size.
- `tests/api/v9.c` — keeps the frozen v8 caller while asserting API v10 current-view size/version and invalid IDs.
- `tests/api/v10.c` — freezes the full v9 layout and exercises public gun names, atomic rejection, exact allocations, partial records and future tails.
- `tests/core/test_core.c` — checks every partial input size through the appended gun records.
- `docs/embedding.md` — API version, append-only layout, flags, coordinates, topology and compatibility contract.

## Decisions Made

- Kept all v9 bytes and endpoint strides fixed; the two gun records are direct-port fields after the v9 prefix.
- Treat `VALID` as the sole signal that aim coordinates were supplied; `(0, 0)` is not a sentinel.
- Leave active-display geometry checks to beam scheduling and refuse eligible negative coordinates at the input boundary.

## Deviations from Plan

None — the plan's API and documentation scope was followed. Task 2 added regression coverage for already implemented size-tag behavior without changing production code.

## Issues Encountered

CTest resolves relative `--output-junit` paths under `build/ci`; the recorded RED report used a project-root absolute path so the ignored evidence remained under `.gsd/tdd-evidence/15-02/`. The evidence classifier accepted the intended API v10 failure.

## User Setup Required

None — no external configuration is required.

## Next Phase Readiness

Plan 03 can consume the validated API v10 snapshot and own gun frame latching and pad/GPU state declarations. This plan establishes the C API contract only; serial replies, beam sensing and libretro host mapping remain with their later Phase 15 plans. No physical calibration or retail-game compatibility is claimed.

---
*Phase: 15-guncon-and-justifier*
*Completed: 2026-11-24*

## Self-Check: PASSED

The summary exists, all three task commits are ancestors of the current branch head, and the coverage classifier accepted all three automated deliverables.
