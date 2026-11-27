---
phase: 11-ecm-and-clonecd-images
plan: "03"
subsystem: disc-image
tags: [ecm, checkpoint-index, bounded-reader, ctest]

requires:
  - phase: 11-01
    provides: Private ECM provider skeleton, shared host image callbacks, and focused test target.
provides:
  - Validated ECM v1 literal streams with full-output EDC and exact-extent checks.
  - Direct case-insensitive ECM and M3U-child routing through the shared MODE2/2352 callback.
  - Bounded random byte reads with checkpoints and per-read work counters.
affects: [11-04, 11-05, ECM-01]

actuals:
  tokens: 11632
  tasks: 2
  commits: 2
commits: 2
plan_head_before: b7afc07fcc3155623ad06086bc9526f9d82a5252
plan_head_after: 99fbc64d6c2ad1cf2c010cebec1fd4a5051c1acd

tech-stack:
  added: []
  patterns:
    - Validate the complete supported encoded stream and its EDC before publishing a host reader.
    - Keep a fixed-size checkpoint index so random reads do not rescan the whole stream.

key-files:
  created: []
  modified:
    - host/disc_ecm.c
    - host/disc_ecm.h
    - host/disc_image.c
    - tests/disc/ecm.c
    - tests/disc/image.c

key-decisions:
  - Support literal type-0 records for bare ECM images; modeled record types remain unsupported pending their independent vectors.
  - Keep ECM unsupported as a CUE track source while enabling direct images and M3U children.
  - Bound per-read decoded work and record-header traversal independently at 70240 units.

patterns-established:
  - Checkpoint records carry source and decoded offsets plus the remaining literal units, allowing restart within long records.
  - Private metrics and limit seams test upper boundaries without allocating a maximum-size image.

requirements-completed: []
coverage:
  - id: D1
    description: Supported literal ECM streams validate the complete byte extent, decoded alignment, terminator, and EDC before exposing a MODE2/2352 disc.
    requirement: ECM-01
    verification:
      - kind: unit
        ref: "ctest --preset ci --no-tests=error -R '^disc[.]image[.]ecm$|^disc[.]image[.]cue_reject$|^disc[.]playlist$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: Checkpoint-based reads preserve exact source bytes while bounding forward work, header traversal, index storage, and allocation cleanup.
    requirement: ECM-01
    verification:
      - kind: unit
        ref: "ctest --preset ci --no-tests=error -R '^disc[.]image[.]ecm$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D3
    description: Direct mixed-case ECM paths and M3U ECM children use the shared host callback, while CUE ECM sources remain rejected.
    requirement: ECM-01
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^disc[.]image[.]ecm$|^disc[.]image[.]cue_reject$|^disc[.]playlist$' --output-on-failure"
        status: pass
    human_judgment: false

duration: 36min
completed: 2026-11-22
status: complete
---

# Phase 11 Plan 03: Validated ECM reader Summary

**Literal ECM images now pass complete stream and EDC validation and support bounded random reads through the host disc callback.**

## Performance

- **Duration:** 36 min
- **Started:** 2026-11-22T02:37:28Z
- **Completed:** 2026-11-22T03:13:26Z
- **Runtime/model:** Codex GSD executor, configured model `gpt-6-luna`, OpenGSD 1.20.0. The primary runtime model identifier is not exposed.
- **Tasks:** 2
- **Files modified:** 5
- **Bottleneck:** Implementation and focused regression work; configure, target builds, and selected tests completed in seconds.

## Accomplishments

- Added a complete ECM v1 literal type-0 reader. It checks the variable count, exact EOF, sector alignment, whole-decoded-output EDC, encoded and decoded size limits, and record count before publishing a reader.
- Routed mixed-case `.ecm` files and M3U children through the shared MODE2/2352 host callback. CUE references still reject ECM children, and failed opens preserve the active ISO.
- Added a bounded checkpoint index with a maximum of 16,386 entries, entries no larger than 48 bytes, and per-read limits of 70,240 decoded bytes and record headers.
- Exercised split and tiny records, first/last and reverse/interleaved reads, malformed and overflowing counts, alignment, checksum mutation, trailing data, and forced index-allocation failure.

## Verification Evidence

| Command | Exit | Result |
| --- | ---: | --- |
| `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` | 0 | 3898/3901 counted lines |
| `LC_ALL=C sort -cu tests/disc/inventory.txt` | 0 | Exact inventory sorted |
| `ctest --test-dir build/ci -N -R '^disc[.]image[.]ecm$|^disc[.]image[.]cue_reject$|^disc[.]playlist$'` | 0 | All three focused tests discovered |
| `cmake --preset ci -DSCHWAYSTATION_CHD=OFF -DSCHWAYSTATION_BUILD_CHD_TOOL=OFF` | 0 | CI configure passed |
| `cmake --build --preset ci --target schwaystation-test-disc-ecm schwaystation-test-disc-image` | 0 | Focused targets built |
| `ctest --preset ci --no-tests=error -R '^disc[.]image[.]ecm$|^disc[.]image[.]cue_reject$|^disc[.]playlist$' --output-on-failure` | 0 | 3/3 passed after Task 1 and again after Task 2 |
| `cmake --build --preset ci --target schwaystation-test-disc-ecm` | 0 | ECM target built after checkpoint changes |
| `ctest --preset ci --no-tests=error -R '^disc[.]image[.]ecm$' --output-on-failure` | 0 | ECM bounds and record-storm controls passed |
| `git diff --check` | 0 | No whitespace errors |
| `node "$HOME/.codex/gsd-core/bin/gsd-tools.cjs" check tdd-red-evidence build/ci/tdd-red-11-03-task1.json --raw` | 0 | `RED_EVIDENCE_OK`; valid direct image was rejected as unsupported before implementation |
| `ctest --test-dir build/ci --output-junit tdd-red-11-03-task2.xml -R '^disc[.]image[.]ecm$' --output-on-failure` | 8 | Expected RED: the old reader accepted a 70,241-byte request |
| `node "$HOME/.codex/gsd-core/bin/gsd-tools.cjs" check tdd-red-evidence build/ci/tdd-red-11-03-task2.json --raw` | 0 | `RED_EVIDENCE_OK`; the named test failed only on the planned first-illegal read bound |

The RED evidence records actual CTest reports under the ignored `build/ci/` directory. Both machine checks passed before their corresponding implementation work. The committed state passed the focused local checks. No hosted CI run, release, or phase-level requirement verification was performed.

## TDD Execution Notes

- **Task 1 RED:** `disc.image.ecm` reached public open and failed because valid literal ECM returned `UNSUPPORTED`. **GREEN:** the provider and dispatcher now validate and read the literal stream; the focused three-test selector passed.
- **Task 2 RED:** the same named test accepted a 70,241-byte request. **GREEN:** the checkpointed reader accepts exactly 70,240 bytes, bounds traversed headers, and passes the 70,560-record storm plus reverse and interleaved reads.
- Task commits were preserved as created after each task's RED evidence and GREEN verification. Each task's test and implementation files are together in its task commit; no history rewrite or additive commit was made to manufacture a different RED/GREEN history shape.

## Task Commits

1. **Task 1: Open and read one literal ECM sector through the public host seam** — `9c00a79` (`feat`)
2. **Task 2: Bound full validation and independent random-read work** — `99fbc64` (`feat`)

## Files Created/Modified

- `host/disc_ecm.c`, `host/disc_ecm.h` — Validates literal streams and services bounded checkpoint-based reads.
- `host/disc_image.c` — Adds direct and M3U routing with the MODE2/2352 TOC projection.
- `tests/disc/ecm.c`, `tests/disc/image.c` — Covers valid, malformed, bounded, and routed inputs.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Test fixture bug] Corrected the literal terminator tag**
- **Found during:** Task 1 GREEN
- **Issue:** The generated sentinel used type bits `3`, so the purported literal terminator decoded as a modeled record.
- **Fix:** Changed the first sentinel byte to the type-0 value and reran the focused selector.
- **Files modified:** `tests/disc/ecm.c`
- **Verification:** Focused ECM, CUE-rejection, and playlist tests passed (3/3).
- **Committed in:** `9c00a79`

**2. [Rule 1 - Random-read cursor bug] Seek to the next record header after skipping a record**
- **Found during:** Task 2 GREEN
- **Issue:** A checkpoint read that skipped to a split-record boundary updated offsets but left the FILE cursor at its old position.
- **Fix:** Seek to the boundary before parsing the next literal record header.
- **Files modified:** `host/disc_ecm.c`
- **Verification:** Split-record, tiny-record, reverse, interleaved, and focused ECM tests passed.
- **Committed in:** `99fbc64`

**Total deviations:** 2 auto-fixed issues (2 Rule 1). **Impact:** Both fixes were needed for the planned test fixtures and random-read correctness; no scope was added.

## Known Stubs

| File | Line | Deferred behavior |
| --- | ---: | --- |
| `host/disc_ecm.c` | 222 | ECM type-1/2/3 record decoding remains unsupported pending independent output vectors in Plan 11-05. |

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Plan 11-03 is complete, and Plan 11-04's dependencies are met. Plan 11-02 remains incomplete because its draft route-control matrix exceeds the fixed 3,901-line cap (3,928 lines); no configure or build was run for that draft. The phase state pointer remains at Plan 11-02 while the orchestrator continues dependency-ready work. ECM-01 remains uncompleted pending the remaining phase scope and final verification. No user action is needed for Plan 11-03.

## Self-Check: PASSED

- Summary file exists at the expected phase path.
- Task commits `9c00a79` and `99fbc64` are ancestors of HEAD.
- The Plan 11-03 ledger measures two commits from `b7afc07` through `99fbc64`.

---
*Phase: 11-ecm-and-clonecd-images*
*Completed: 2026-11-22*
