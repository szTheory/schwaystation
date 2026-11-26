---
phase: 16-patches-cheats-and-compatibility-evidence
plan: "01"
subsystem: testing
tags: [cmake, ctest, inventories, c17]
requires: []
provides:
  - Released API v10 caller baseline that still reads memory and runs a frame
  - Gating patch, cheat, compatibility, and fuzz regression selector graph
affects: [phase-16-plans-02-through-16]
actuals:
  tokens: 2868
  tasks: 3
  commits: 3
plan_head_before: edf15cc257f2ce047467e649b62f8178c160be5f
plan_head_after: 667355d29023bc0c614f468c2d8a8af4aadfb7b2
tech-stack:
  added: []
  patterns:
    - Exact sorted CTest inventories for every expanded test directory
    - Fail-first entry points for behavior assigned to later plans
key-files:
  created:
    - tests/disc/patch.c
    - tests/runner/cheats.c
    - .planning/phases/16-patches-cheats-and-compatibility-evidence/deferred-items.md
  modified:
    - tests/disc/tests.cmake
    - tests/disc/inventory.txt
    - tests/runner/tests.cmake
    - tests/runner/inventory.txt
    - tests/libretro/tests.cmake
    - tests/libretro/inventory.txt
    - tests/compat/tests.cmake
    - tests/compat/inventory.txt
    - tests/fuzz/tests.cmake
    - tests/fuzz/inventory.txt
    - .planning/WINDOWS.md
key-decisions:
  - Keep abi.api_v10 as the released caller baseline before API v11 is added.
  - Register unfinished selectors as gating failures for their owning plans.
  - Use concrete build-tree paths for patch and cheat replay executables until Plan 11 creates their targets.
duration: 17 min
completed: 2026-11-25
status: complete
requirements-completed:
  - PATCH-01
  - CHEAT-01
  - COMPAT-01
  - COMPAT-03
coverage:
  - id: D1
    description: The released API v10 caller reads memory and survives a real frame.
    requirement: CHEAT-01
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^(abi[.]api_v10)$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: Patch and fuzz regression selectors are registered exactly once in sorted inventories.
    requirement: PATCH-01
    verification:
      - kind: other
        ref: "Phase 16 Plan 01 Task 3 CTest selector enumeration and six-inventory sort preflight"
        status: pass
    human_judgment: false
  - id: D3
    description: Runner cheat selectors expose the released memory-read baseline and leave future parser cases visibly failing.
    requirement: CHEAT-01
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^runner[.]cheats[.]memory_read$' --output-on-failure"
        status: pass
      - kind: other
        ref: "Phase 16 Plan 01 Task 2 exact selector enumeration"
        status: pass
    human_judgment: false
  - id: D4
    description: DAT inventory accounting selectors enumerate exactly once in the compatibility inventory.
    requirement: COMPAT-01
    verification:
      - kind: other
        ref: "Phase 16 Plan 01 Task 3 CTest selector enumeration and six-inventory sort preflight"
        status: pass
    human_judgment: false
  - id: D5
    description: Required-capability evaluation selectors enumerate exactly once in the compatibility inventory.
    requirement: COMPAT-03
    verification:
      - kind: other
        ref: "Phase 16 Plan 01 Task 3 CTest selector enumeration and six-inventory sort preflight"
        status: pass
    human_judgment: false
# Phase 16 Plan 01: Released Caller and Test Graph Summary

**API v10 frame behavior is preserved, and the planned patch, cheat, compatibility, and fuzz test graph is registered.**

## Performance

- **Runtime/model:** Codex GSD executor configured as `gpt-6-luna`.
- **Duration:** 17 minutes
- **Started:** 2026-11-26T00:13:20Z
- **Completed:** 2026-11-26T00:30:15Z
- **Tasks:** 3/3
- **Files modified:** 12 plan-owned test files, plus the summary and stub/deferred tracking artifacts

## Accomplishments

- Kept the released `abi.api_v10` caller as the pre-write-API baseline; it built and completed a real frame.
- Registered seven patch selectors, runner and libretro cheat/CLI selectors, five compatibility selectors, and two fuzz replay selectors in their exact CTest inventories.
- Added passing memory-read coverage using the released `schway_peek` API and confirmed that future behavior remains fail-first.

## Task Commits

1. **Freeze the released caller and patch-test graph** — `48095bb` (`test`)
2. **Register host cheat and CLI integration cases** — `0d273cc` (`test`)
3. **Register DAT, capability and fuzz replay cases** — `667355d` (`test`)

## Files Created and Modified

- `tests/disc/patch.c` — bounded patch-test entry point that rejects unknown and not-yet-implemented cases.
- `tests/runner/cheats.c` — released RAM-read baseline and fail-first cheat parser cases.
- `tests/disc/tests.cmake`, `tests/runner/tests.cmake`, `tests/libretro/tests.cmake`, `tests/compat/tests.cmake`, and `tests/fuzz/tests.cmake` — target and selector registrations.
- `tests/disc/inventory.txt`, `tests/runner/inventory.txt`, `tests/libretro/inventory.txt`, `tests/compat/inventory.txt`, and `tests/fuzz/inventory.txt` — sorted exact inventories.
- `.planning/WINDOWS.md` — six open stub entries for behavior assigned to later plans.
- `.planning/phases/16-patches-cheats-and-compatibility-evidence/deferred-items.md` — records an unrelated pinned-armips compiler warning.

## Verification Evidence

- `git diff --check HEAD`, the ABI and disc inventory sorts, `cmake --preset ci`, the `abi.api_v10` selector preflight, the API-v10 and patch-target build, and the real-frame `abi.api_v10` CTest all exited 0. The tracer check was repeated after its task commit and passed again.
- `cmake --preset ci`, runner/libretro inventory sorts, exact enumeration of all nine planned selectors, and builds of `schwaystation-test-cheats` and `schwaystation-test-libretro-host` exited 0. `runner.cheats.memory_read` passed 1/1.
- The ABI, disc, runner, libretro, compatibility, and fuzz inventories all passed byte-sort checks. `cmake --preset ci` and enumeration of all 14 patch, compatibility, and fuzz selectors exited 0.
- Unimplemented expansion selectors were enumerated but not run; they intentionally fail until their assigned implementation plans land. No broad workflow or hosted CI was required for this registration-only plan.

## Known Stubs

| File and line | Intentional state | Planned owner |
| --- | --- | --- |
| `tests/disc/patch.c:26` | Patch cases return failure until source mapping and PPF/BPS behavior are implemented. | Plans 07–09 |
| `tests/runner/cheats.c:54` | Parser, bounds, and write-order cases return failure until the shared cheat table is implemented. | Plan 03 |
| `tests/runner/tests.cmake:37–41` | Runner cheat/hash and patch CLI modes fail until those options are implemented. | Plans 06 and 10 |
| `tests/libretro/tests.cmake:444` | Registered host subcommands await cheat callback and state-envelope behavior. | Plans 04–05 |
| `tests/compat/tests.cmake:1292` | Inventory and capability evidence modes fail until the accounting and evaluator are implemented. | Plans 12–15 |
| `tests/fuzz/tests.cmake:337` | Replay executable paths are registered before the targets exist. | Plan 11 |

## Deferred Issues

- The first `schwaystation-test-libretro-host` build emitted AppleClang `sprintf` deprecation warnings from the pinned armips dependency while building existing guest fixtures. The warning is unrelated to this plan and is recorded in `deferred-items.md`.

## Deviations from Plan

None. The task outputs and focused checks followed the plan. The sandbox initially blocked writing the worktree Git sentinel under `.git`; after confirming the assigned worktree root, normal commits completed through the authorized Git operation and the sentinel was kept in `/private/tmp`.

## Next Phase Readiness

Plan 02 can add API v11 while retaining the passing API-v10 caller. Later Phase 16 plans own the deliberately failing patch, cheat, compatibility-evidence, and replay behavior.

## Self-Check: PASSED

All planned files and task commits are present; `git diff --check HEAD` and all six inventory sort checks pass.
