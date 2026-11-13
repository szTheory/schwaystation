---
phase: 04-games-play-in-retroarch
plan: "45"
subsystem: libretro
tags: [libretro, CMake, metadata, PS-EXE]
requires:
  - phase: 04-44
    provides:
      - Functional libretro memory descriptors and aliases
      - In-process host fixtures and lifecycle checks
provides:
  - Configure-time validation grounding every true functional .info claim in registered tests
  - Case-insensitive PSEXE content loading through the existing EXE loader
  - Runtime assertions for every published input descriptor
affects:
  - Phase 4 libretro verification
  - Future changes to libretro .info claims
actuals:
  tokens: 6389
  tasks: 2
  commits: 2
commits: 2
plan_head_before: e679b2b17bfb8fe0b5ff48798fb76dbb4d4adb72
plan_head_after: 946325b04ca24ec33d3e2a253e1c34653b7bc664
tech-stack:
  added: []
  patterns:
    - Configure-time truth mapping between advertised functional claims and registered behavior tests
    - In-process host assertions for frontend descriptors and alternate content extensions
key-files:
  created:
    - tests/cmake/info_claims.cmake
    - tests/cmake/info_claims_self_test.cmake
  modified:
    - libretro/schwaystation_libretro.info.in
    - libretro/CMakeLists.txt
    - libretro/libretro.c
    - tests/libretro/tests.cmake
    - tests/libretro/inventory.txt
    - tests/libretro/host.c
    - docs/testing.md
    - THIRD-PARTY-NOTICES.md
    - .planning/todos/completed/009-info-claims-gate.md
key-decisions:
  - Keep disk-control, cheat, save-state, and subsystem claims false until their behavior exists.
  - Advertise PSEXE only after the shared loader successfully runs a case-varied PSEXE fixture.
  - Keep the host's existing input descriptor callback claim only with assertions for all published mappings and lifetime.
  - Preserve the repository CI preset; use a temporary ignored four-job preset for local full-workflow evidence.
patterns-established:
  - A true functional .info capability must map to a registered inventory case and an observed behavior check.
requirements-completed: []
coverage:
  - id: D1
    description: Functional metadata claims are validated at configure time and reject ungrounded true claims.
    requirement: GAME-07
    verification:
      - kind: other
        ref: libretro.info.claims; libretro.info.claims.self_test; throwaway configure with camera=true
        status: pass
    human_judgment: false
  - id: D2
    description: A case-varied PSEXE fixture loads and runs through the existing EXE content path without BIOS.
    requirement: GAME-07
    verification:
      - kind: integration
        ref: libretro.content.psexe; libretro.content.no_bios; libretro host fixture output
        status: pass
    human_judgment: false
  - id: D3
    description: The input-descriptor claim is backed by exact port, device, index, ID, label, sentinel, and lifecycle assertions.
    requirement: GAME-07
    verification:
      - kind: integration
        ref: libretro.input.descriptors; full ci-local-4 workflow
        status: pass
    human_judgment: false
  - id: D4
    description: Memory-descriptor claims remain grounded in tested RAM aliases, boundaries, and lifecycle behavior.
    requirement: GAME-07
    verification:
      - kind: integration
        ref: libretro.memory.aliases; libretro.memory.boundaries; libretro.content.lifecycle
        status: pass
    human_judgment: false
duration: 45min
completed: 2026-11-12
status: complete
---

# Phase 04 Plan 45: Guard only functional libretro metadata claims Summary

**The libretro core now advertises only capabilities backed by registered configure-time and in-process host checks, including real case-insensitive PSEXE loading.**

## Performance

- **Duration:** 45 min
- **Started:** 2026-11-12T16:13:17Z
- **Completed:** 2026-11-12T16:57:57Z
- **Tasks:** 2
- **Files modified:** 11

## Accomplishments

- Added a configure-time metadata checker that validates functional claim values and requires registered behavior coverage for every true capability; unsupported disk, cheat, save-state, and subsystem claims remain false.
- Added a real PSEXE fixture load/run through the existing EXE loader and strengthened the in-process host test to assert all published input descriptors and their lifetime.
- Added 19 negative controls, a real throwaway configure refusal for an ungrounded `camera = "true"` claim, and the pinned MIT seed notice; closed todo 009.

## Task Commits

1. **Task 1: Guard only functional libretro metadata claims** - `e7f36cd` (`feat`)
2. **Task 2: Expand and document the proven path** - `946325b` (`test`)

The final plan metadata commit is recorded after summary and state updates.

## Files Created/Modified

- `tests/cmake/info_claims.cmake` - Exact metadata validation and mapping from true claims to registered behavior cases.
- `tests/cmake/info_claims_self_test.cmake` - Counted refusal controls for malformed and ungrounded metadata.
- `libretro/libretro.c` and `tests/libretro/host.c` - Case-insensitive PSEXE execution and exact descriptor/lifecycle assertions.
- `libretro/schwaystation_libretro.info.in`, `libretro/CMakeLists.txt`, and `tests/libretro/{tests.cmake,inventory.txt}` - Truthful metadata, configure-time guard, and sorted gating registrations.
- `docs/testing.md`, `THIRD-PARTY-NOTICES.md`, and `.planning/todos/completed/009-info-claims-gate.md` - Usage, pinned MIT attribution, and todo closure.

## Decisions Made

- `disk_control`, cheats, save states, and subsystem support remain false because these behaviors are not implemented.
- PSEXE is advertised only after the actual shared loader succeeds with a case-varied PSEXE fixture.
- The existing repository CI defaults remain unchanged; local verification used a temporary ignored four-job preset after evidence showed the empty `execution.jobs` setting selects automatic parallelism.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking issue] Added the required timeout to the new registered test.**
- **Found during:** Task 1
- **Issue:** Configuration rejected the initial `schway_add_test` registration because this repository requires an explicit timeout.
- **Fix:** Added the existing test helper's `TIMEOUT` argument.
- **Files modified:** `libretro/CMakeLists.txt`
- **Verification:** The CI configure and focused metadata cases passed.
- **Committed in:** `e7f36cd`

**2. [Rule 1 - Bug] Retrieved the system-info struct before setting the valid extension string.**
- **Found during:** Task 1
- **Issue:** The first focused build exposed an undefined `sys` identifier in the extension update.
- **Fix:** Use the adapter's local `retro_system_info` value before passing it to the host callback.
- **Files modified:** `libretro/libretro.c`
- **Verification:** The PSEXE host fixture and focused libretro tests passed.
- **Committed in:** `e7f36cd`

**Total deviations:** 2 auto-fixed issues (one build/configuration issue and one source bug).
**Impact on plan:** Both fixes were necessary to complete the declared behavior; no dependency or new product gate was added.

## Evidence

- Focused tracer verification and the post-commit tracer rerun: configure, build of the linked libretro/runner/fixture targets, and `libretro.info.claims` passed.
- Task 1 focused libretro cases passed 9/9; Task 2 claim checker/self-test cases passed 2/2.
- The 19 refusal controls passed through `expect_fail.cmake`. A separate temporary configure using the actual integrated checker exited 1 with `info_claims: no test mapping for true .info claim camera`.
- The standard `cmake --workflow --preset ci` workflow and its `CTEST_PARALLEL_LEVEL=4` retry each passed 508/509 tests but timed out `core.state.record_sweep` at its existing 20-second bound (20.04s and 20.02s). The CTest preset's `execution.jobs: ""` maps to `--parallel` with the count omitted in CMake 4.4.3, so the environment variable did not cap parallelism. The isolated sweep passed in 7.23s. A focused overlap run of the sweep with both movie tests passed at explicit `--parallel 4` (7.23s, 8.96s, and 42.26s respectively), so contention is consistent with the full-run result but not proven as its cause.
- A temporary, ignored `CMakeUserPresets.json` inherited the exact CI configure/build/test/package/release-test steps and set four jobs explicitly. `cmake --workflow --preset ci-local-4` exited 0: 509/509 CI tests, packaging, and 5/5 release tests passed. The temporary file was removed; no tracked default, test data, or timeout changed.
- `cmake --workflow --preset hygiene` exited 0: 23/23 checks passed.

## Issues Encountered

The stock CI preset still selects automatic parallelism on this CMake version, so setting `CTEST_PARALLEL_LEVEL=4` did not limit the workflow and the existing record-sweep timeout recurred. The bounded local workflow completed all CI and release tests without changing the tracked preset or timeout. The reason for the two full-run timeouts remains an inference from the scheduling and timing evidence.

## User Setup Required

None.

## Next Phase Readiness

The metadata guard, PSEXE loader path, and input descriptor claims are proven locally. Phase-wide review and verification remain owned by the phase orchestrator.

## Self-Check: PASSED

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-12*
