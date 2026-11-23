---
phase: 11-ecm-and-clonecd-images
plan: "01"
subsystem: disc-image
tags: [ecm, clonecd, cmake, ctest]

requires:
  - phase: 10-pbp-disc-images
    provides: bounded host disc-image provider conventions and shared media limits
provides:
  - Private ECM and CCD rejecting providers wired into the host-only disc target
  - Focused ECM and CCD malformed-input tests and sorted directory inventory
  - Workflow-control consolidation with the 3901-line budget preserved
affects: [11-03-ecm-decoder, 11-04-clonecd-reader, 11-05-cue-ecm-integration]

actuals:
  tokens: 7121
  tasks: 3
  commits: 4
commits: 4
plan_head_before: fe42a51700709a6fa2c73902258783c6776cc587
plan_head_after: fbb74d87c2e4432d1981352857b84687bcba01ac

tech-stack:
  added: []
  patterns:
    - Private host provider APIs reject input until the format-specific reader has validated it.
    - Self-generated malformed inputs verify output clearing and active-source preservation.

key-files:
  created:
    - host/disc_ecm.c
    - host/disc_ecm.h
    - host/disc_ccd.c
    - host/disc_ccd.h
    - tests/disc/ecm.c
    - tests/disc/ccd.c
  modified:
    - cmake/DiscTargets.cmake
    - tests/cmake/workflows.cmake
    - tests/disc/tests.cmake
    - tests/disc/inventory.txt

key-decisions:
  - Keep both providers private and fail closed until their planned positive readers validate complete media.
  - Treat a missing ECM continuation byte as malformed and leave the output reader null.
  - Preserve every routing control and keep the counted repository checks at 3898 of 3901 lines.

patterns-established:
  - Each new disc format owns one portable test target and one exact sorted inventory row.
  - Failed provider opens must not change an already open ISO source.

requirements-completed: []
coverage:
  - id: D1
    description: ECM provider rejects malformed signatures and truncated record counts without publishing a reader or disturbing the active ISO.
    requirement: ECM-01
    verification:
      - kind: unit
        ref: ctest --preset ci --no-tests=error -R '^disc[.]image[.](ecm|ccd)$' --output-on-failure
        status: pass
    human_judgment: false
  - id: D2
    description: CCD provider rejects malformed descriptors and unsupported versions without publishing a reader or disturbing the active ISO.
    requirement: CCD-01
    verification:
      - kind: unit
        ref: ctest --preset ci --no-tests=error -R '^disc[.]image[.](ecm|ccd)$' --output-on-failure
        status: pass
    human_judgment: false
  - id: D3
    description: Host target wiring, exact test inventory, and repository-check ceiling cover both providers.
    verification:
      - kind: other
        ref: cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake; cmake -DCASE=disc_registration -DROOT=. -P tests/cmake/workflows.cmake; ctest --preset ci --no-tests=error -R '^ci[.]classify_policy$' --output-on-failure
        status: pass
    human_judgment: false

duration: 37min
completed: 2026-11-22
status: complete
---

# Phase 11 Plan 01: Bounded provider skeleton and exact inventories Summary

**Private ECM/CCD host readers compile, reject malformed input without publishing handles, and have focused portable test coverage.**

## Performance

- **Duration:** 37 min
- **Started:** 2026-11-22T01:34:33Z
- **Completed:** 2026-11-22T02:11:22Z
- **Runtime/model:** Codex GSD executor, configured model `gpt-6-luna`, OpenGSD 1.20.0.
- **Bottleneck:** Evidence capture and commit bookkeeping took longer than native verification; the final configure/build/two-test rerun completed in under one second.
- **Tasks:** 3
- **Files modified:** 10

## Accomplishments

- Added private ECM and CCD providers to the host-only static target. Inputs over the planned bounds, malformed headers, unsupported CCD versions, and incomplete ECM count fields fail before a reader is published.
- Added `disc.image.ecm` and `disc.image.ccd` tests. Both tests confirm a failed open preserves the existing active ISO source.
- Consolidated duplicated workflow assertions while retaining all existing cases; the counted scripts, CMake controls, and hooks remain at 3898/3901 lines.

## Verification Evidence

| Command | Exit | Result |
| --- | ---: | --- |
| `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` | 0 | 3898/3901 lines |
| `cmake -DCASE=disc_registration -DROOT=. -P tests/cmake/workflows.cmake` | 0 | Route and inventory wiring passed |
| `cmake --preset ci` | 0 | CI configure passed |
| `ctest --preset ci --no-tests=error -R '^ci[.]classify_policy$' --output-on-failure` | 0 | Classifier control passed |
| `cmake --preset ci -DSCHWAYSTATION_CHD=OFF -DSCHWAYSTATION_BUILD_CHD_TOOL=OFF && cmake --build --preset ci --target schwaystation_disc_image` | 0 | Host-only target compiled |
| `LC_ALL=C sort -cu tests/disc/inventory.txt && cmake --preset ci -DSCHWAYSTATION_CHD=OFF -DSCHWAYSTATION_BUILD_CHD_TOOL=OFF && cmake --build --preset ci --target schwaystation-test-disc-ecm schwaystation-test-disc-ccd && ctest --preset ci --no-tests=error -R '^disc[.]image[.](ecm|ccd)$' --output-on-failure` | 0 | Inventory sorted; both tests passed (2/2), including the committed-state tracer rerun |
| Initial `ctest --test-dir build/ci --output-junit build/ci/tdd-red-11-01.xml -R '^disc[.]image[.]ecm$' --output-on-failure` | 8 | Expected RED: the named test failed on the truncated count classification assertion |
| `node "$HOME/.codex/gsd-core/bin/gsd-tools.cjs" check tdd-red-evidence build/ci/tdd-red-11-01.json --raw` | 0 | `RED_EVIDENCE_OK`; one target JUnit failure and no setup errors |

No hosted CI run was started during this plan. The phase execution and final verification workflow will handle hosted gates after their local preflights.

## TDD Gate Compliance

- **RED:** `build/ci/tdd-red-11-01.json` records the actual CTest JUnit report and console output. `disc.image.ecm` executed and asserted that a one-byte continuation marker with no following count byte must return `SCHWAY_DISC_IMAGE_BAD_IMAGE` and a null reader. It observed `SCHWAY_DISC_IMAGE_UNSUPPORTED` and a null reader, so the failure was the planned behavior assertion rather than setup or discovery.
- **GREEN:** The ECM open path now checks the bounded continuation chain and reports a truncated count as malformed. The focused ECM/CCD selector passed after the fix and again on the committed state.
- **REFACTOR:** No additional refactor was needed after GREEN.
- **Commits:** RED test `b526ee8`; GREEN fix `fbb74d8`.

## Task Commits

1. **Task 1: Preserve gate cases while making measured room for routing controls** — `c43af29` (`refactor`)
2. **Task 2: Wire private ECM and CCD rejecting paths** — `d14b31d` (`feat`)
3. **Task 3 RED: Register provider checks and exact inventory** — `b526ee8` (`test`)
4. **Task 3 GREEN: Reject truncated ECM record counts** — `fbb74d8` (`fix`)

## Files Created/Modified

- `host/disc_ecm.c`, `host/disc_ecm.h` — Bounded ECM header and record-count checks with private reader entry points.
- `host/disc_ccd.c`, `host/disc_ccd.h` — Bounded CloneCD descriptor validation and private reader entry points.
- `cmake/DiscTargets.cmake` — Adds both providers to the host-only disc target.
- `tests/disc/ecm.c`, `tests/disc/ccd.c`, `tests/disc/tests.cmake`, `tests/disc/inventory.txt` — Focused provider checks and sorted inventory entries.
- `tests/cmake/workflows.cmake` — Consolidates repeated workflow controls without removing assertions.

## Known Stubs

These are intentional stage boundaries; the plan goal is a bounded rejecting provider skeleton. Full positive reads remain owned by later plans.

| File | Line | Deferred behavior |
| --- | ---: | --- |
| `host/disc_ecm.c` | 132 | Structurally valid ECM record streams remain unsupported until Plan 11-03 validates and decodes them. |
| `host/disc_ecm.c` | 149 | ECM TOC generation remains unsupported until Plan 11-03 validates the complete record stream. |
| `host/disc_ecm.c` | 157 | ECM decoded-size reporting remains unsupported until Plan 11-03 builds bounded checkpoints. |
| `host/disc_ecm.c` | 165 | ECM random byte reads remain unsupported until Plan 11-03 builds bounded checkpoints. |
| `host/disc_ecm.c` | 175 | ECM sector reads remain unsupported until Plan 11-03 validates its sector mapping. |
| `host/disc_ccd.c` | 171 | CloneCD v3 track metadata remains unsupported until Plan 11-04 validates tracks and companions. |
| `host/disc_ccd.c` | 188 | CloneCD TOC generation remains unsupported until Plan 11-04 validates the descriptor tracks. |
| `host/disc_ccd.c` | 198 | CloneCD sector reads remain unsupported until Plan 11-04 validates its sector mapping. |

Each item was appended to `.planning/WINDOWS.md` as an open stub entry. ECM-01 and CCD-01 are not marked complete; the positive readers and phase-level evidence remain outstanding.

## Decisions Made

- The public dispatcher and ABI remain unchanged until complete positive readers land in their owning plans.
- A valid signature alone does not publish a reader; malformed counts and unsupported CCD metadata fail closed.
- The exact inventory remains byte-sorted, and the established line ceiling stays at 3901.

## Deviations from Plan

None. The RED/GREEN cycle completed as planned. The intentionally unsupported positive paths are listed under Known Stubs and belong to later phase plans.

## Issues Encountered

The first ECM RED exposed that a truncated continuation byte was classified as unsupported. The implementation now distinguishes that malformed count from a complete but not-yet-validated ECM stream. No other blocker remains.

## User Setup Required

None - no external service configuration required.

## Next Plan Readiness

Plan 11-01 is complete and Plan 11-02 can proceed. Plans 11-03 and 11-04 own the remaining positive ECM and CCD reader behavior. Neither phase requirement is complete yet. No user action is needed.

## Self-Check: PASSED

- All ten created/modified source, CMake, and test files exist.
- Task commits `c43af29`, `d14b31d`, `b526ee8`, and `fbb74d8` are ancestors of the current branch head.
- `commits`, `plan_head_before`, and `plan_head_after` are measured from the plan ledger.

---
*Phase: 11-ecm-and-clonecd-images*
*Completed: 2026-11-22*
