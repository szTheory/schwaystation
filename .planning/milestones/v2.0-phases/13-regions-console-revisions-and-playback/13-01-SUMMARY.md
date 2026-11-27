---
phase: 13-regions-console-revisions-and-playback
plan: "01"
subsystem: machine-profile
tags: [c-api, board-revisions, gpu, compatibility]
requires: []
provides:
  - Append-only API v8 board identity for schway_profile
  - Per-instance board defaults with atomic board/GPU validation
  - Public API and core tests for legal, illegal and legacy profile callers
affects: [phase-13-host-profiles, phase-13-state-serialization]
actuals:
  tokens: 6781
  tasks: 2
  commits: 2
tech-stack:
  added: []
  patterns:
    - Validate explicit board/GPU tuples before mutating an instance
    - Keep board defaults independent from region and title identity
key-files:
  created:
    - tests/api/v8.c
  modified:
    - .planning/phases/13-regions-console-revisions-and-playback/13-01-PLAN.md
    - docs/embedding.md
    - include/schwaystation.h
    - src/instance.c
    - src/internal.h
    - tests/abi/inventory.txt
    - tests/abi/tests.cmake
    - tests/core/inventory.txt
    - tests/core/test_core.c
    - tests/core/test_state.c
    - tests/core/tests.cmake
key-decisions:
  - New instances use the documented PU-18 profile defaults.
  - An old-sized profile or board zero inherits current board defaults; GPU_V0 remains a legacy override in that path.
  - Explicit boards accept only their documented GPU pairing and replace the board-derived defaults atomically.
  - API version advances to 8 and behavior revision to 19; state serialization and hashes remain assigned to Plan 13-04.
requirements-completed: []
coverage:
  - id: D1
    description: Core API selects every documented board/GPU tuple, rejects mismatches atomically, and preserves old-sized and board-zero profile behavior.
    requirement: PROFILE-02
    verification:
      - kind: unit
        ref: "core.profile.machine, core.profile.rejected, core.profile.locked; core.state.cpu_fields; abi.api_v8; abi.api_version"
        status: pass
    human_judgment: false
duration: "20m43s"
completed: 2026-11-23
status: complete
plan_head_before: 335d236884b87fca479da0357d0ce078e4daf15a
plan_head_after: 067c6866674d51dc12a675b8488590c36a02d335
---

# Phase 13 Plan 01: Core machine profile selection Summary

Append-only public board selection now applies the documented defaults, validates all legal GPU pairs before mutation, and retains legacy profile semantics.

## Performance

- **Duration:** 20m43s wall interval from the last saved planning-state timestamp to closeout; the exact executor start was not separately captured.
- **Started:** 2026-11-23T14:43:33Z (saved STATE checkpoint used as the start anchor)
- **Completed:** 2026-11-23T15:04:16Z
- **Runtime/model:** Codex GSD executor; configured `gpt-6-luna` xhigh. Runtime model ID was unavailable.
- **Tasks:** 2
- **Files modified:** 12 unique files
- **Actuals:** 27,125 diff bytes / 4 = 6,781 estimate-scale tokens; two task commits measured from the plan ledger.

## Accomplishments

- Added API v8 board constants and an append-only `schway_profile.board` field. New instances select PU-18 defaults.
- Added a per-board defaults table and validate-then-apply selection for GPU, SPU/CD firmware, RAM-control boot value and port availability.
- Covered all nine legal board/GPU pairs, all nine mismatches, unsupported boards, board-zero inheritance, old-sized callers, and rejection atomicity.
- Documented the profile defaults and compatibility behavior in the embedding guide.

## Task Commits

1. **Task 1: Register the machine-profile test skeleton** — `9ceab60` (`test(13-01): register machine profile contract`)
2. **Task 2: Expose and validate documented board profiles** — `067c686` (`feat(13-01): add board profile selection`)

## Verification Evidence

- `git diff --check && LC_ALL=C sort -cu tests/core/inventory.txt` — exit 0.
- `git diff --check && LC_ALL=C sort -cu tests/core/inventory.txt && LC_ALL=C sort -cu tests/abi/inventory.txt && cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake && cmake -DHEADER=include/schwaystation.h -DAPI_DIR=tests/api -P tests/cmake/api_version.cmake` — exit 0; the repository-check budget remained at its 3,901-line ceiling and the API gate covered 249 header names.
- `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-core schwaystation-test-state schwaystation-test-api-v8 && ctest --preset ci --no-tests=error -R '^(core[.]profile[.](machine|rejected|locked)|core[.]state[.]cpu_fields|abi[.](api_v8|api_version))$' --output-on-failure` — first attempt exited 1 during build because `src/internal.h` still asserted a 16-byte profile. Configure succeeded; CTest did not run.
- After correcting the profile-size assertion, `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-core schwaystation-test-state schwaystation-test-api-v8 && ctest --preset ci --no-tests=error -R '^(core[.]profile[.](machine|rejected|locked)|core[.]state[.]cpu_fields|abi[.](api_v8|api_version))$' --output-on-failure` — exit 0; all 8 reported tests passed in 0.21 seconds, including two automatic fixture setup tests.
- All evidence is local; no hosted run URL applies. No workflow changed, so Act was not run.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing critical test coverage] Added the required API v8 version test.**
- **Found during:** Task 2
- **Issue:** The public API version bump requires a `tests/api/v8.c` that touches new public names, with an ABI test selector and sorted inventory row.
- **Fix:** Added the API v8 contract test, registered `abi.api_v8`, updated the ABI inventory and the plan's file/verification lists. Updated the behavior-revision assertion to 19.
- **Files modified:** `13-01-PLAN.md`, `tests/api/v8.c`, `tests/abi/tests.cmake`, `tests/abi/inventory.txt`, `tests/core/test_state.c`
- **Verification:** API version coverage passed for 249 header names; `abi.api_v8`, `abi.api_version` and `core.state.cpu_fields` passed.
- **Committed in:** `067c686`

**2. [Rule 3 - Blocking build issue] Updated the profile-size assertion.**
- **Found during:** Task 2 configure/build
- **Issue:** The first build still asserted `sizeof(schway_profile) == 16` after the planned field append.
- **Fix:** Asserted the board offset at 16 bytes and the new profile size at 20 bytes.
- **Files modified:** `src/internal.h`
- **Verification:** The corrected configure/build and focused CTest command passed.
- **Committed in:** `067c686`

**Total deviations:** 2 auto-fixed (1 Rule 2, 1 Rule 3)
**Impact on plan:** The API test and version assertion preserve the repository's established version contract; the size assertion restores compilation. No unrelated scope was added.

## Issues Encountered

The initial planned build failed on the stale profile-size assertion. It was corrected before retesting; no issue remains.

## Known Stubs

None. Save-state serialization and hashing of board identity are intentionally deferred to Plan 13-04, so PROFILE-02 remains pending.

## User Setup Required

None. No user action is needed.

## Next Phase Readiness

Plan 13-01 is complete. Plan 13-02 can proceed; the full PROFILE-02 requirement remains open until its state validation, serialization and hash coverage lands in the planned follow-up.

## Self-Check: PASSED

The summary and all listed task files exist, both task commits are ancestors of HEAD, and `git diff --check` is clean.
