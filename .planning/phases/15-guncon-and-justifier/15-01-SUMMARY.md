---
phase: 15-guncon-and-justifier
plan: "01"
subsystem: testing
tags:
  - ABI
  - CMake
  - CTest
  - libretro
  - controller-input
requires:
  - phase: 14
    provides:
      - Released API v9 input layout and current test targets
provides:
  - Frozen 228-byte API v9 caller tracer through the public frame boundary
  - Gating API, SIO, GPU beam, and libretro gun selector registrations
affects:
  - Phase 15 implementation plans 02-10
actuals:
  tokens: 4010
  tasks: 3
  commits: 3
commits: 3
plan_head_before: af072eeb4000517697f343659946aaf1fc970262
plan_head_after: 4c012b39539e87c8192203071811c0f18eff0a58
tech-stack:
  added: []
  patterns:
    - Freeze a released public struct locally in an ABI caller test.
    - Register future behavior selectors in existing suite targets and inventories.
key-files:
  created:
    - tests/api/v10.c
  modified:
    - tests/abi/tests.cmake
    - tests/abi/inventory.txt
    - tests/sio/tests.cmake
    - tests/sio/inventory.txt
    - tests/gpu/tests.cmake
    - tests/gpu/inventory.txt
    - tests/libretro/tests.cmake
    - tests/libretro/inventory.txt
key-decisions:
  - Freeze the complete released v9 input and endpoint layouts without requiring API v10.
  - Keep future gun behavior selectors gating and enumerate them without executing their unimplemented cases.
duration: "14m 14s"
completed: 2026-11-24
status: complete
requirements-completed:
  - GUN-01
  - GUN-02
  - SIO-05
coverage:
  - id: D1
    description: The released 228-byte v9 caller preserves direct-port and eight-endpoint state through a real frame.
    requirement: SIO-05
    verification:
      - kind: unit
        ref: ctest --preset ci --no-tests=error -R '^(abi[.]api_v10)$' --output-on-failure
        status: pass
    human_judgment: false
  - id: D2
    description: GunCon, Justifier, and GPU beam cases are present in the exact SIO and GPU CTest graph.
    requirement: GUN-01
    verification:
      - kind: other
        ref: ctest --preset ci --show-only=json-v1 with the eight sio.gun and gpu.gun selectors plus a missing and duplicate selector assertion
        status: pass
    human_judgment: false
  - id: D3
    description: LIGHTGUN, POINTER, and unavailable-input cases are present in the in-process libretro host graph.
    requirement: GUN-02
    verification:
      - kind: other
        ref: ctest --preset ci --show-only=json-v1 with the three libretro.input gun selectors plus a missing and duplicate selector assertion
        status: pass
    human_judgment: false
---

# Phase 15 Plan 01: Freeze the Released Caller and Register Gun Selectors Summary

**A frozen 228-byte API v9 caller reaches the frame latch, and 12 planned gun selectors are registered in the existing test graph.**

## Performance

- **Duration:** 14m 14s
- **Started:** 2026-11-25T03:29:14Z
- **Completed:** 2026-11-25T03:43:28Z
- **Runtime/model:** Codex/OpenGSD 1.23.0; executor model gpt-6-luna; primary session model ID unavailable
- **Tasks:** 3
- **Files modified:** 9

## Accomplishments

- Added a local frozen declaration of the complete released v9 input and controller layouts, with compile-time checks for all prefix and endpoint offsets, strides, and the 228-byte total size.
- Allocated exactly 228 bytes, submitted direct-port and eight-endpoint values through `schway_set_input`, advanced real frames, and checked the public state visitor for the latched values.
- Registered one API tracer, four SIO, four GPU beam, and three in-process libretro selectors in their existing binaries and sorted inventories.

## Verification Evidence

- Frozen-layout Python preflight, `git diff --cached --check`, and `LC_ALL=C sort -cu tests/abi/inventory.txt`: exit 0.
- `cmake --preset ci`: exit 0.
- `ctest --preset ci --show-only=json-v1 -R '^(abi[.]api_v10)$'` with missing/duplicate selector assertion: exit 0.
- `cmake --build --preset ci --target schwaystation-test-api-v10`: exit 0.
- `ctest --preset ci --no-tests=error -R '^(abi[.]api_v10)$' --output-on-failure`: exit 0; 1/1 passed. This command was rerun after the tracer commit and passed again.
- SIO/GPU selector enumeration via `ctest --preset ci --show-only=json-v1` and the eight-name assertion: exit 0.
- Libretro selector enumeration via `ctest --preset ci --show-only=json-v1` and the three-name assertion: exit 0.
- Final combined `ctest --preset ci --show-only=json-v1` registration check: exit 0; 12/12 expected selectors, no duplicates.
- All four owned inventories passed `LC_ALL=C sort -cu`; `git diff --check` passed; `scripts/hygiene.sh --tree` exited 0.

The SIO, GPU, and libretro behavior cases were deliberately enumerated but not executed. Their subcommands belong to later Phase 15 plans, and the current registrations remain gating so missing implementations fail visibly.

## Task Commits

1. **Trace the released caller through frame input** — `5518c2a` (`test(15-01): freeze released API v9 caller`)
2. **Register serial and beam cases** — `7c1d06c` (`test(15-01): register SIO and GPU gun selectors`)
3. **Register short in-process host cases** — `4c012b3` (`test(15-01): register libretro gun selectors`)

## Files Created/Modified

- `tests/api/v10.c` — locally freezes API v9 and checks direct and eight-endpoint frame latching.
- `tests/abi/tests.cmake`, `tests/abi/inventory.txt` — registers the passing API v10 compatibility tracer.
- `tests/sio/tests.cmake`, `tests/sio/inventory.txt` — registers the four future SIO gun cases.
- `tests/gpu/tests.cmake`, `tests/gpu/inventory.txt` — registers the four future beam sensor cases.
- `tests/libretro/tests.cmake`, `tests/libretro/inventory.txt` — registers the three future in-process host cases.

## Decisions Made

No design changes. The test freezes the v9 declaration locally and makes no assertion that the current public API version has advanced to v10.

## Deviations from Plan

None — plan executed as written.

## Issues Encountered

An initial configure ran before the new `abi.api_v10` directory-inventory row had been added and correctly rejected the mismatch. Adding the row restored the inventory contract; all subsequent configure and selector checks passed. No aggregate inventory or workflow changes were needed.

## Known Stubs

None. The registered SIO, GPU, and libretro selectors intentionally await their implementation plans and do not prevent Plan 01's registration and compatibility-tracer goals.

## User Setup Required

None — no external configuration is required.

## Next Phase Readiness

Plan 02 can append the API v10 gun extension while the new caller test protects the complete v9 prefix. Plans owning SIO, GPU, and libretro behavior now have exact selectors in their existing gating suites.

## Self-Check: PASSED

`tests/api/v10.c` and this summary exist, and task commits `5518c2a`, `7c1d06c`, and `4c012b3` are ancestors of the current branch head.
