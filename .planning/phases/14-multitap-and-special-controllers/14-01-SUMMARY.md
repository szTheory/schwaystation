---
phase: 14-multitap-and-special-controllers
plan: "01"
subsystem: testing
tags: [cmake, ctest, abi, sio, libretro, runner]
requires: []
provides:
  - Frozen 28-byte v8 input caller tracer through the public frame latch
  - Sorted CMake registrations and inventories for all Phase 14 peripheral tests
affects: [phase 14 plans 02-11, phase 14 verification]
tech-stack:
  added: []
  patterns: [frozen public-caller ABI tracer, exact CTest selector discovery]
key-files:
  created:
    - tests/api/v9.c
  modified:
    - tests/abi/tests.cmake
    - tests/abi/inventory.txt
    - tests/sio/tests.cmake
    - tests/sio/inventory.txt
    - tests/card/tests.cmake
    - tests/card/inventory.txt
    - tests/libretro/tests.cmake
    - tests/libretro/inventory.txt
    - tests/runner/tests.cmake
    - tests/runner/inventory.txt
key-decisions:
  - Keep the released 28-byte caller layout fixed while testing it against the current API.
  - Register future peripheral tests as gating selectors and leave their behavior to the owning plans.
metrics:
  duration: 12min
  completed: 2026-11-23
  status: complete
commits: 3
plan_head_before: 43d8be0d61108d5b062d012e0c5c236c26ab2262
plan_head_after: 150344e12a7c0526dfffd8e096d9dc5900041403
actuals:
  tokens: 3653
  tasks: 3
  commits: 3
requirements-completed: []
coverage:
  - id: D1
    description: A frozen 28-byte v8 caller carries digital and DualShock values through one frame into observed pad state.
    requirement: SIO-04
    verification:
      - kind: integration
        ref: "cmake --build --preset ci --target schwaystation-test-api-v9; ctest --preset ci --no-tests=error -R '^abi[.]api_v9$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: All nine short SIO and card protocol selectors are registered in sorted inventories.
    requirement: SIO-01
    verification:
      - kind: other
        ref: "cmake --preset ci and exact CTest show-only selector assertion for the nine SIO/card registrations"
        status: pass
    human_judgment: false
  - id: D3
    description: The short libretro mapping/persistence and runner card selectors are registered in sorted inventories.
    requirement: SIO-01
    verification:
      - kind: other
        ref: "cmake --preset ci and exact CTest show-only assertion for the four libretro/runner registrations"
        status: pass
    human_judgment: false
---

# Phase 14 Plan 01: Test Skeleton and Released Input Caller Summary

**A frozen v8 input caller passes through the public frame latch, and the Phase 14 short peripheral selectors are registered before their implementation plans.**

## Performance

- **Runtime/model:** Codex GSD executor; configured `gpt-6-luna` xhigh; primary runtime model ID unavailable.
- **Wall interval:** About 12 minutes from the plan commit checkpoint at 2026-11-23 22:05:13 EST through closeout shortly after 22:16 EST. Executor start was not separately captured; active time was not measured.
- **Tasks:** 3. **Files modified:** 11.
- **Task commits:** `6e81054`, `2f3835a`, `150344e` (three measured commits from `43d8be0` through `150344e`).

## Accomplishments

- Added `tests/api/v9.c` with fixed size/offset assertions and a real 28-byte caller that sends digital and DualShock values through `schway_set_input`, `schway_run_frame`, and public state observation.
- Registered six SIO and three card selectors with existing test binaries and sorted inventories.
- Registered three short libretro selectors and one runner card selector. Test-only card inspection uses the existing uninstalled module; no production exports or guest boot were added.

## Verification Evidence

- **Task 1:** `git diff --check`, `LC_ALL=C sort -cu tests/abi/inventory.txt`, configure, exact `abi.api_v9` selector assertion, target build, and focused CTest passed. Configure took 0.7s; the target build completed 39 Ninja actions; CTest passed 3/3 including fixture setup in 0.13s. Command exit: 0.
- **Task 2:** `git diff --check`, both sorted inventory checks, configure, and exact nine-selector assertion passed. Configure took 0.7s. The nine future behavior selectors were enumerated but not executed. Command exit: 0.
- **Task 3:** `git diff --check`, both sorted inventory checks, configure, and an exact four-selector assertion passed. Configure took 0.8s. The future behavior selectors were enumerated but not executed. Command exit: 0.
- The Task 3 background configure attempt did not retain an authoritative exit result; the same configure was rerun directly and passed in 0.8s. No build or broad suite was needed for registration-only changes.
- No workflow changed, so hosted CI and Act were not needed. No Docker resources were created.

## Intentional Future-Red Selectors

Thirteen registered behavior selectors are intentionally expected to fail until their owning plans add the implementations: nine SIO/card vectors and four libretro/runner vectors. This plan checked their CMake registration and exact discovery only. The released-caller tracer itself is executable and passed.

## Decisions Made

- Requirements SIO-01 through SIO-04 remain pending; this plan establishes tests and ABI compatibility but does not implement the phase behaviors.
- Keep all future behavior selectors gating so missing implementations remain visible.

## Deviations from Plan

None. Task scope and test commands followed the plan.

## Issues Encountered

The first detached Task 3 configure launch had no recorded completion status. A direct rerun established the configure result; no code changes were needed.

## Next Phase Readiness

Plan 14-02 can extend the API tracer while the SIO, card, libretro, and runner selectors are ready for their assigned implementation plans. Phase verification remains for the post-Plan-12 orchestration.

## Self-Check: PASSED

The summary file exists, and all three task commits are ancestors of the recorded plan head.

---
*Phase: 14-multitap-and-special-controllers*
*Completed: 2026-11-23*
