---
phase: 11-ecm-and-clonecd-images
plan: "05"
subsystem: disc-image
tags: [ecm, cue, cdrom, bounded-reader]
requires:
  - phase: 11-03
    provides: Literal ECM reader, direct-image fallback and bounded random-byte interface.
  - phase: 11-04
    provides: Existing CUE source geometry, track indexes and pregap policy.
provides:
  - ECM type 1, 2 and 3 reconstruction checked against independent known-answer trailers.
  - Random reads through modeled units and raw 2352-byte CUE tracks using decoded extents.
  - Documented limits for direct ECM and ECM-backed CUE sources.
affects: [11-06, ECM-01, disc-image]
actuals:
  tokens: 11686
  tasks: 2
  commits: 4
commits: 4
plan_head_before: f12f34421e84f3b96920b99ec95fac8c7ba064cf
plan_head_after: 90cc6d1e458525126ce7f7cc3c1b0150ac85aa9e
tech-stack:
  added: []
  patterns:
    - Store checkpoint offsets within expanded ECM units so random reads need bounded decode-forward work.
    - Route CUE child reads through the decoded source extent while preserving the shared CUE track policy.
key-files:
  created: []
  modified:
    - host/disc_ecm.c
    - host/disc_image.c
    - tests/disc/ecm.c
    - tests/disc/image.c
    - docs/discs.md
key-decisions:
  - Type 1 expands to 2352 bytes; types 2 and 3 expand to 2336 bytes and never synthesize the absent raw header.
  - Bare ECM keeps the exact 2352-byte MODE2 fallback; CUE ECM sources accept only existing raw 2352-byte track modes.
  - ECM CUE track bounds and duplicate-source accounting use decoded extent after canonical child confinement.
requirements-completed: []
coverage:
  - id: ECM-MODELED-RECORDS
    description: Types 1–3 reconstruct independently calculated bytes; mixed literals, modeled checkpoints, checksum rejection and 2336-only rejection are covered.
    requirement: ECM-01
    verification:
      - kind: unit
        ref: "ctest --preset ci --no-tests=error -R '^disc[.]image[.]ecm$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: ECM-CUE-SOURCES
    description: ECM CUE sources match raw CUE TOCs and boundary reads, preserve pregap policy, confine child paths and reject cooked source modes.
    requirement: ECM-01
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^disc[.]image[.]ecm$|^disc[.]image[.]cue_|^disc[.]playlist$' --output-on-failure"
        status: pass
    human_judgment: false
duration: 29min
completed: 2026-11-22
status: complete
---

# Phase 11 Plan 05: Modeled ECM records and CUE track sources

**Modeled ECM records now reconstruct exact sector bytes, and raw 2352-byte CUE tracks read through decoded ECM extents.**

## Performance

- **Duration:** 29 minutes.
- **Started:** 2026-11-22T14:05:13Z
- **Completed:** 2026-11-22T14:34:10Z
- **Runtime/model:** OpenGSD 1.20.0 GSD executor, `gpt-6-luna` xhigh; the primary runtime model identifier is not exposed.
- **Tasks:** 2.
- **Files modified:** 5.
- **Bottleneck:** The modeled-record decoder and independent sector vectors; configure, focused builds and selectors completed in seconds.

## Accomplishments

- Reconstructed Mode 1 type 1 sectors and Mode 2 form 1/2 type 2/3 bodies, with the actual 2352-byte stream checksum covering expanded output.
- Added independent fixed EDC/ECC known answers, mixed literal/model records, modeled checkpoint restart, truncated data, checksum mutation and bare 2336-byte rejection cases.
- Routed CUE ECM children through canonicalized sources and decoded extents. Equivalent multi-track raw and ECM CUEs match their TOCs and out-of-order reads across stored/virtual pregaps.
- Kept the counted repository-control inventory at 3898/3901 lines; no CI routing controls changed.

## Task Commits

1. **Task 1 RED: modeled ECM known-answer vectors** — `8afd490`.
2. **Task 1 GREEN: decode modeled ECM records** — `10c9bd7`.
3. **Task 2 RED: ECM-backed CUE equivalence** — `3b23e90`.
4. **Task 2 GREEN: route ECM CUE sources** — `90cc6d1`.

## TDD Gate Compliance

- Task 1 RED ran `disc.image.ecm` and failed on the intended assertion: the valid mixed type 1/2/3 stream returned `unsupported ECM record type`. The JUnit report was accepted by `gsd_run check tdd-red-evidence` as `RED_EVIDENCE_OK`; the complete local record is under the ignored build directory.
- Task 1 GREEN passed the focused ECM target, including the independent byte vectors and modeled checkpoint restart. The tracer gate then reran configure, target build and the focused selector successfully before Task 2 began.
- Task 2 RED ran the raw-CUE oracle and failed at ECM CUE open with the current unsupported-source error. Its JUnit report also passed the RED evidence checker.
- The first Task 2 compile exposed a missing test-helper declaration, and the next run exposed invalid PREGAP ordering in the fixture. Both were corrected before accepting RED; neither was treated as feature evidence. After the intended RED, the CUE route was implemented, then path-escape, cooked-mode and duplicate-source assertions were added and the focused selector rerun.
- No separate refactor commit was needed. The actual test/implementation sequence is preserved in the four commits above; no history was amended or split.

## Verification Evidence

| Command | Exit | Result |
| --- | ---: | --- |
| `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` | 0 | 3898 counted lines; sorted `tests/disc/inventory.txt` |
| `cmake --preset ci -DSCHWAYSTATION_CHD=OFF -DSCHWAYSTATION_BUILD_CHD_TOOL=OFF` | 0 | CI preset configured |
| `cmake --build --preset ci --target schwaystation-test-disc-ecm schwaystation-test-disc-image` | 0 | Focused targets built |
| `ctest --preset ci --no-tests=error -R '^disc[.]image[.]ecm$|^disc[.]image[.]cue_|^disc[.]playlist$' --output-on-failure` | 0 | All 9 selected ECM, CUE and playlist tests passed |
| `ctest --preset ci --no-tests=error -R '^disc[.]image[.]ecm$' --output-on-failure` | 0 | Final ECM test passed after duplicate-source coverage was added |

No hosted CI or broad suite was run for this plan. Review and final phase verification remain with the orchestrator; this summary does not mark ECM-01 complete for the phase.

## Deviations from Plan

- Updated the existing `tests/disc/image.c` CUE rejection assertion: ECM is now an accepted child type, while a missing ECM child still fails with `BAD_CUE`. The new supported path is separately compared against an uncompressed CUE.

## Self-Check: PASSED

- All five modified source, test and documentation files exist.
- The four task commits are ancestors of `90cc6d1e458525126ce7f7cc3c1b0150ac85aa9e`.
- Measured plan history from `f12f34421e84f3b96920b99ec95fac8c7ba064cf` contains 4 commits; the realized diff is 46,744 characters (11,686 estimate-scale tokens).
