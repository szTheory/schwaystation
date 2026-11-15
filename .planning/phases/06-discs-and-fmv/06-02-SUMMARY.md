---
phase: 06-discs-and-fmv
plan: "02"
subsystem: disc-api
tags: [c, public-api, disc, determinism]
requires:
  - 06-01
provides:
  - API v5 host disc callback registration, TOC validation, lid control, and atomic swaps.
  - Core behavior coverage for copied metadata, non-run sector silence, and closed-lid rejection.
affects:
  - DISC-02 host disc providers
  - DISC-04 disc control consumers
  - 06-25 API v5 compatibility gate
tech-stack:
  added: []
  patterns:
    - Host callback pointers and checked TOC metadata live outside serialized device state.
    - Disc replacement validates into temporary local data before applying it.
key-files:
  created:
    - src/disc.c
    - tests/core/disc_api.c
  modified:
    - include/schwaystation.h
    - src/internal.h
    - CMakeLists.txt
    - tests/core/tests.cmake
    - tests/core/inventory.txt
decisions:
  - "The first disc attaches once through schway_open_disc; later replacements require an open lid through schway_swap_disc."
  - "Callbacks and TOC data are copied into host-only instance state; invalid metadata and closed-lid swaps leave it untouched."
metrics:
  duration: 26min
  completed: 2026-11-15
actuals:
  tokens: 4965
  tasks: 2
  commits: 2
commits: 2
plan_head_before: 6b69e546a438fd65672bf7f858145fa42de77d45
plan_head_after: 93c065ccb1c962c9ea21555ce334a61bc85d3a4b
requirements-completed: []
coverage:
  - id: D1
    description: A valid host disc installs callbacks and a copied, validated TOC without reading sectors during attach or non-run APIs.
    requirement: DISC-02
    verification:
      kind: integration
      ref: "cmake --preset ci && cmake --build --preset ci --target schwaystation-test-disc-api && ctest --test-dir build/ci -R '^core\\.disc_api$' --no-tests=error --output-on-failure; exit 0; local run, no hosted URL"
      status: pass
    human_judgment: false
  - id: D2
    description: A closed-lid swap returns the appended status without changing callbacks, TOC, queued audio, or serialized machine state.
    requirement: DISC-04
    verification:
      kind: integration
      ref: "cmake --preset ci && cmake --build --preset ci --target schwaystation-test-disc-api && ctest --test-dir build/ci -R '^core\\.disc_api$' --no-tests=error --output-on-failure; exit 0; local run, no hosted URL"
      status: pass
    human_judgment: false
  - id: D3
    description: Existing API v1-v4 consumers still compile and pass with the v5 header and appended status.
    requirement: DISC-02
    verification:
      kind: integration
      ref: "cmake --preset ci && cmake --build --preset ci --target schwaystation-test-disc-api schwaystation-test-api-v1 schwaystation-test-api-v2 schwaystation-test-api-v3 schwaystation-test-api-v4 && ctest --test-dir build/ci -R '^core\\.disc_api$|^abi\\.api_v[1-4]$' --no-tests=error --output-on-failure; exit 0; LC_ALL=C sort -c tests/core/inventory.txt; exit 0; local run, no hosted URL"
      status: pass
    human_judgment: false
status: complete
---

# Phase 06 Plan 02: Consumer-Minimal Disc Callback API Summary

**The core now accepts a host-owned disc through a v5 callback API, validates and copies its TOC, and rejects closed-lid swaps atomically.**

## Performance

- **Duration:** 26 minutes
- **Started:** 2026-11-15T15:51:55Z
- **Completed:** 2026-11-15T16:17:49Z
- **Tasks:** 2
- **Files modified:** 7 planned implementation/test/build files
- **Runtime/model:** Codex, OpenGSD 1.20.0; executor gpt-6-luna xhigh

## Accomplishments

- Added API v5 `schway_disc`, `schway_toc`, and size-tagged track records with sector, TOC, and bad-Q callbacks, plus `schway_open_disc`, `schway_set_lid`, and `schway_swap_disc`.
- Stored callbacks and copied TOC data in host-only instance state that survives reset and is not serialized. TOC validation rejects invalid counts, ordering, sizes, and reserved data before apply.
- Verified open, reset, peek, save, and load do not call `read_sector`. The test seeds queued audio and confirms a closed-lid swap leaves audio and machine state unchanged.
- Kept the released API v1-v4 consumers building and passing. Plan 06-25 owns the new v5 consumer and the separate ABI registration gate; that later gate is not claimed here.

## Task Commits

1. **Task 1 RED: Add the failing disc callback API test** — `ab7fd0d` (`test`)
2. **Task 1 GREEN: Implement validated host disc attachment** — `93c065c` (`feat`)
3. **Task 2: Verify CMake registration and v1-v4 compatibility** — no new commit; its wiring landed in the RED commit so the task 1 test target could be built.

## Automated Verification

All automated checks ran from the repository root without activating a UI. Local runs have no hosted run URL.

| Check | Result |
|---|---|
| Focused disc callback API check (coverage D1 and D2) | Exit 0; the selected core API test passed. |
| Existing API compatibility and inventory checks (coverage D3) | Exit 0; the disc API test and all four released consumer tests passed, and the core inventory is sorted. |
| RED evidence before implementation | Exit 8 on the valid-attach assertion, not a compile failure; `gsd check tdd-red-evidence` returned `RED_EVIDENCE_OK`. The temporary report and record were removed after validation. |

## Files Created/Modified

- `include/schwaystation.h`
- `src/disc.c`
- `src/internal.h`
- `tests/core/disc_api.c`
- `CMakeLists.txt`
- `tests/core/tests.cmake`
- `tests/core/inventory.txt`
- `.planning/phases/06-discs-and-fmv/06-02-SUMMARY.md`

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking issue] Registered the test target before Task 1 RED**

- **Found during:** Task 1, while preparing the RED run.
- **Issue:** Task 1's required verification builds `schwaystation-test-disc-api`, but Plan 06-01 had not created that target and Task 2 owned its CMake registration.
- **Fix:** Added `src/disc.c` to the existing integer-only core object target and registered the behavior executable and exact inventory entry in the RED commit. Task 2 then ran the specified v1-v4 compatibility command successfully.
- **Files modified:** `CMakeLists.txt`, `tests/core/tests.cmake`, `tests/core/inventory.txt`.
- **Commit:** `ab7fd0d`.

## Threat Flags

| Flag | File | Description |
|---|---|---|
| threat_flag: host_callback | `include/schwaystation.h` | The core stores host-provided callback pointers; TOC data is validated before use, and host callback code is trusted by design. |

## Issues Encountered

No unresolved issues. The later API v5 consumer and ABI gate remain assigned to Plan 06-25.

## User Setup Required

None. No owner verification was needed.

## Next Plan Readiness

The core API seam is ready for the phase's host provider and CD-ROM plans. DISC-02 and DISC-04 remain incomplete as whole requirements until their later host, controller, and libretro acceptance checks pass.

## Self-Check: PASSED

Both task commits exist on the phase branch; the seven planned implementation, test, and build files are present. The plan ledger measures two task commits from `6b69e546a438fd65672bf7f858145fa42de77d45` through `93c065ccb1c962c9ea21555ce334a61bc85d3a4b`.

*Phase: 06-discs-and-fmv*
*Completed: 2026-11-15*
