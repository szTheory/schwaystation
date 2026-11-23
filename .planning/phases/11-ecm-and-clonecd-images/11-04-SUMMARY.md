---
phase: 11-ecm-and-clonecd-images
plan: "04"
subsystem: disc-image
tags: [clonecd, ccd, toc, disc-provider]

requires:
  - phase: 11-01
    provides: Private CCD provider skeleton and shared host image callbacks.
  - phase: 11-03
    provides: Proven shared dispatcher pattern and focused disc image selectors.
provides:
  - Fixture-proven Version 3 single-track CCD data reads through the shared host callback.
  - Direct and M3U-child CCD routing with explicit mode, extent and TOC checks.
affects: [11-05, 11-06, 11-07, 11-13, CCD-01]

actuals:
  tasks: 1
  commits: 2
plan_head_before: 9dbb49e
plan_head_after: 043403f

tech-stack:
  added: []
  patterns:
    - Validate a fixture-proven CCD descriptor before exposing its raw-sector reader.
    - Route direct and playlist media through the existing private host callback.

key-files:
  created: []
  modified:
    - host/disc_ccd.c
    - tests/disc/ccd.c
    - host/disc_image.c
    - tests/disc/image.c

key-decisions:
  - Accept only the fixture-proven Version 3, single-session, one-data-track mapping; reject inferred pregaps and unsupported metadata.
  - Keep CCD-as-CUE-source unsupported and use direct or M3U-child routing.
  - Leave canonical companion confinement and failure-atomic replacement to Plan 13, and multi-track/index/SUB behavior to Plan 06.

requirements-completed: []
coverage:
  - id: CCD-R1
    description: Fixture-proven direct and M3U CCD data-track reads return matching first/last sectors and TOC while malformed extents or unsupported mappings fail.
    requirement: CCD-01
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^disc[.]image[.]ccd$|^disc[.]image[.]cue_reject$|^disc[.]playlist$' --output-on-failure"
        status: pass
    human_judgment: false

duration: about 33min
completed: 2026-11-22
status: complete
---

# Phase 11 Plan 04: Fixture-proven CCD reader summary

**A supported single-track CCD fixture now opens directly or from an M3U playlist and returns its proven raw sectors and TOC.**

## Performance

- **Duration:** About 33 minutes; exact start time was not retained.
- **Runtime/model:** OpenGSD 1.20.0 executor, configured `gpt-6-luna` with xhigh reasoning. The primary runtime model identifier is not exposed.
- **Tasks:** 1 of 1 after the unstarted companion-confinement task was split into Plan 13.
- **Files modified:** 4.
- **Bottleneck:** CCD parser and assertion correction; the post-commit configure, target build and focused selector completed without a broad suite.

## Accomplishments

- Added fixture-proven Version 3, single-session data-track reading for direct CCD paths and M3U children.
- Checked first/last sector reads, mode and TOC mapping, malformed extents and unsupported metadata.
- Kept the shared callback contract and the 3,901-line repository-check cap unchanged.

## Verification Evidence

| Command | Exit | Result |
| --- | ---: | --- |
| `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` | 0 | 3898/3901 counted lines |
| `LC_ALL=C sort -cu tests/disc/inventory.txt` | 0 | Exact inventory sorted |
| `cmake --preset ci -DSCHWAYSTATION_CHD=OFF -DSCHWAYSTATION_BUILD_CHD_TOOL=OFF` | 0 | CI preset configured |
| `cmake --build --preset ci --target schwaystation-test-disc-ccd` | 0 | Focused CCD test target built |
| `ctest --preset ci --no-tests=error -R '^disc[.]image[.]ccd$|^disc[.]image[.]cue_reject$|^disc[.]playlist$' --output-on-failure` | 0 | 3/3 focused CCD, CUE-rejection and playlist tests passed after commit |
| `git show --check --oneline 5780f40 && git show --check --oneline 043403f` | 0 | Both task commits are whitespace-clean |

No broad local suite or hosted CI was run. The focused verification proves this reader slice only; CCD-01 remains incomplete pending companion confinement, multi-track/index/SUB behavior and phase-level verification.

## TDD Execution Notes

- Task 1 RED was recorded in `5780f40`: valid direct and M3U CCD opens failed as unsupported at the intended boundary.
- GREEN was committed in `043403f`. The first focused run exposed three stale or incomplete expectations, corrected before the final post-commit 3/3 pass.
- Both task commits are preserved; no history rewrite was used to change their order.

## Next Phase Readiness

Plan 13 owns companion-path confinement and failure-atomic replacement; Plan 06 owns explicit multi-track/index and SUB behavior. Plan 05 can proceed independently in Wave 4. Plan 02 remains blocked because its draft exceeds the fixed cap by 27 lines; preserve its cases and cap while evaluating a semantics-preserving remedy. No user action is needed.

---
*Phase: 11-ecm-and-clonecd-images*
*Completed: 2026-11-22*
