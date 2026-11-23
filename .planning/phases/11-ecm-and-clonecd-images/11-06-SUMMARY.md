---
phase: 11-ecm-and-clonecd-images
plan: "06"
subsystem: disc-image
tags: [ccd, clonecd, toc, subcode]
requires:
  - phase: 11
    provides: Confined CloneCD companion paths and the shared zero-based disc TOC contract.
provides:
  - Version 3 single-session CCD parsing for contiguous MODE0 audio and MODE1/MODE2 data tracks.
  - Explicit INDEX 0/1 and exact leadout mapping, plus channel-major SUB conversion into callback P-W bytes.
affects: [host-disc-image, cdrom-subcode]
actuals:
  tokens: 8491
  tasks: 2
  commits: 4
plan_head_before: 79eeae9ad2510698cd6bb50b08de49960f4a60ba
plan_head_after: 065b064b0e6ee6c8986d9ff2f5d0508418d91298
tech-stack:
  added: []
  patterns:
    - Reconcile every CCD track entry against ordered MODE/INDEX metadata before publishing a reader.
    - Keep a validated SUB stream open and transpose each bounded 96-byte sector group on demand.
key-files:
  created: []
  modified:
    - host/disc_ccd.c
    - tests/disc/ccd.c
key-decisions:
  - An omitted INDEX 0 maps to INDEX 1 and yields no stored pregap; no geometry is inferred.
  - A nonempty SUB is accepted only when its length is exactly 96 bytes per IMG sector; missing and empty SUB keep the zero-subcode path.
requirements-completed: []
coverage:
  - id: CCD-TRACK-INDEX-EXTENTS
    description: Mixed data/audio track entries preserve indexes, stored pregaps, and first/last sector boundaries; malformed ordering and Entry disagreements reject.
    requirement: CCD-01
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^disc[.]image[.]ccd$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: CCD-SUB-PW-TRANSPOSE
    description: An independent P-W vector, exact SUB extents, repeated reads, null output, and Q/GetlocP field ordering are checked.
    requirement: CCD-01
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^disc[.]image[.]ccd$' --output-on-failure"
        status: pass
    human_judgment: false
duration: 27min
completed: 2026-11-22
status: complete
---

# Phase 11 Plan 06: CloneCD tracks and subcode

**CloneCD now preserves multi-track indexes and converts exact channel-major SUB streams into the callback's P-W layout.**

## Performance

- **Duration:** About 27 minutes, from 2026-11-22T14:36:57Z through final state closeout at 2026-11-22T15:03:34Z.
- **Runtime/model:** OpenGSD 1.20.0 Codex executor, configured `gpt-6-luna` xhigh; primary runtime model identifier is not exposed.
- **Tasks:** 2.
- **Files modified:** 2.
- **Bottleneck:** CCD grammar/index reconciliation and independent byte-vector checks took about 19 minutes; configure, target builds, and focused CTest completed in seconds, with the remaining time spent on summary/state closeout.

## Accomplishments

- Parses contiguous Version 3 single-session track and TOC entry sequences, mapping MODE0 audio and MODE1/MODE2 data entries into the shared TOC.
- Preserves explicit INDEX 0/1 positions, uses INDEX 1 when INDEX 0 is absent, and reconciles per-track Entry control/point/PLBA values with exact IMG extents.
- Accepts absent/empty SUB as zero subcode; validates nonempty SUB length with checked arithmetic and transposes each sector's channel-major P-W bits into callback bytes.
- Added independent literal P-W and valid Q vectors, GetlocP byte-order checks, repeated and null-destination reads, extent mismatches, index boundaries, overlap, unsupported modes/sessions, and Entry disagreement cases.
- The repository-control inventory remains 3898/3901 counted lines.

## Task Commits

1. **Task 1 RED: mixed track boundary coverage** — `b13e65f`.
2. **Task 1 GREEN: preserve CCD track indexes and extents** — `37f3efe`.
3. **Task 2 RED: independent SUB and Q vectors** — `0e55424`.
4. **Task 2 GREEN: transpose exact SUB streams** — `065b064`.

## TDD Gate Compliance

- Task 1's named `disc.image.ccd` target failed on the new mixed two-track descriptor assertion because the current provider rejected it. Its fresh JUnit report passed `gsd_run check tdd-red-evidence` as `RED_EVIDENCE_OK`; the report and record are ignored build outputs under `build/ci/plan11-06-task1-red.*`.
- Task 1 GREEN passed the focused target build and selector. Coverage includes data/audio TOC mapping, omitted and explicit INDEX 0 behavior, transition/first/last sector reads, overlapping/out-of-range indexes, and Entry disagreement.
- Task 2's named target failed on the exact-size nonempty SUB open assertion while the provider still rejected all nonempty SUB files. Its JUnit report also passed the RED evidence checker; the ignored record is under `build/ci/plan11-06-task2-red.*`.
- Task 2 GREEN passed after correcting two test-fixture mistakes (an oversized literal initializer and an off-by-one test buffer length). The final run checked the independent P-W vector, Q bit 6 extraction and GetlocP field order, short/long/whole-sector size mismatches, empty SUB behavior, repeat reads, and a null subcode destination.
- The two RED and two GREEN commits preserve the actual task order; no history rewrite or extra refactor commit was needed.

## Verification Evidence

| Command | Exit | Result |
| --- | ---: | --- |
| `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` | 0 | 3898 counted lines, below the fixed cap |
| `LC_ALL=C sort -cu tests/disc/inventory.txt` and `ctest --test-dir build/ci -N -R '^disc[.]image[.]ccd$'` | 0 | Sorted inventory and exactly one CCD selector discovered |
| `cmake --preset ci -DSCHWAYSTATION_CHD=OFF -DSCHWAYSTATION_BUILD_CHD_TOOL=OFF` | 0 | CI preset configured |
| Task 1 RED `ctest --preset ci --output-junit "$PWD/build/ci/plan11-06-task1-red.xml" --no-tests=error -R '^disc[.]image[.]ccd$' --output-on-failure` | 8 | Intended mixed-track assertion failed; `gsd_run check tdd-red-evidence build/ci/plan11-06-task1-red.json --raw` exited 0 |
| Task 1 GREEN `cmake --build --preset ci --target schwaystation-test-disc-ccd` and focused CTest | 0 | CCD target built and `disc.image.ccd` passed (1/1) |
| Task 2 RED `ctest --preset ci --output-junit "$PWD/build/ci/plan11-06-task2-red.xml" --no-tests=error -R '^disc[.]image[.]ccd$' --output-on-failure` | 8 | Intended exact-SUB assertion failed; `gsd_run check tdd-red-evidence build/ci/plan11-06-task2-red.json --raw` exited 0 |
| Final preflight, target build, and `ctest --preset ci --no-tests=error -R '^disc[.]image[.]ccd$' --output-on-failure` | 0 | Inventory sorted, line budget remained 3898, target rebuilt, and CCD test passed (1/1) |
| `git diff --check` | 0 | Clean diff |

No broad suite, Act job, or hosted CI run was needed for this host-only plan. `CCD-01` remains open until Phase 11 verification.

## Decisions Made

- An absent INDEX 0 means no explicit stored pregap; the TOC sets index0 to index1 and does not add a conventional pregap.
- Empty/missing SUB retains the existing zero-subcode behavior. Any nonempty SUB must cover every IMG sector with exactly 96 bytes.

## Deviations from Plan

None. Test fixture mistakes were corrected before accepting the corresponding verification result.

## Issues Encountered

- The first task-2 build caught an oversized literal initializer; compilation was fixed before the valid RED run.
- The first task-2 GREEN attempt exposed an off-by-one in the test's size-mismatch buffer, which had produced a full-size SUB instead of a one-byte-short file. The buffer lengths were corrected and the targeted selector passed.

## Next Plan Readiness

Plan 11-07 is dependency-ready now that Plans 11-05 and 11-06 are complete. It owns the generated ECM/CCD fixtures and their clean build dependencies; no user action is needed.

## Self-Check: PASSED

- Summary file exists at the required phase path.
- All four measured task commits are ancestors of the plan head.
- The persisted plan-base ledger measures four commits through `065b064`.

---
*Phase: 11-ecm-and-clonecd-images*
*Completed: 2026-11-22*
