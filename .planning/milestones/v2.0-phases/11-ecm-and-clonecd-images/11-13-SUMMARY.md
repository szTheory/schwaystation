---
phase: 11-ecm-and-clonecd-images
plan: "13"
subsystem: disc-image
tags: [clonecd, ccd, filesystem, path-confinement]

requires:
  - phase: 11-04
    provides: Fixture-proven single-track CCD reader with same-stem companion lookup.
provides:
  - Separate error outcomes for an unresolved optional SUB path and an out-of-directory SUB path.
  - Regression coverage for missing/empty/nonempty companions, sibling-prefix and symlink escapes, and reader preservation.
affects: [11-06, 11-07, CCD-01]

actuals:
  tokens: 1348
  tasks: 1
  commits: 2
commits: 2
plan_head_before: af378c053e8c73e9a622d15a30bfd199563884ee
plan_head_after: 6696aec487d4865ea78e4f7f353bfcd310098a20

tech-stack:
  added: []
  patterns:
    - Report failed canonicalization as IO and a proven out-of-root path as BAD_IMAGE.
    - Use a valid sibling-prefix image sentinel to show path confinement rejects external companions.

key-files:
  created: []
  modified:
    - host/disc_ccd.c
    - tests/disc/ccd.c

key-decisions:
  - Preserve missing and empty SUB behavior, and reject nonempty SUB until Plan 06 proves extent and conversion.
  - Treat a present dangling SUB symlink as an IO/canonicalization failure, distinct from a malformed companion escape.

patterns-established:
  - Canonicalize an existing companion first, then separately report whether resolution failed or the resolved path escaped the descriptor directory.
  - Keep symlink-specific filesystem tests on POSIX hosts while retaining portable same-stem, missing-file and SUB behavior tests elsewhere.

requirements-completed: []
coverage:
  - id: CCD-COMPANIONS
    description: Same-stem CCD companion outcomes and failure atomicity are exercised, including external path escapes and dangling optional SUB resolution.
    requirement: CCD-01
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^disc[.]image[.]ccd$' --output-on-failure"
        status: pass
    human_judgment: false

duration: 11min
completed: 2026-11-22
status: complete
---

# Phase 11 Plan 13: Confined CCD companions and failure-atomic reads

**CCD companion failures now distinguish path-resolution errors from confirmed escapes, with regression coverage for valid sentinels, optional SUB behavior and preservation.**

## Performance

- **Duration:** 11 minutes 20 seconds.
- **Started:** 2026-11-22T13:18:19Z
- **Completed:** 2026-11-22T13:29:39Z
- **Runtime/model:** Codex GSD executor, `gpt-6-luna` with xhigh reasoning; OpenGSD 1.20.0. The primary runtime model identifier is not exposed.
- **Tasks:** 1.
- **Files modified:** 2.
- **Bottleneck:** Focused test authoring and preserving accurate RED evidence; configure, build and the CCD selector completed in seconds.

## Accomplishments

- Added explicit outcomes for missing required IMG, missing and empty optional SUB, unsupported nonempty SUB, and unreadable SUB where host permissions expose that case.
- Added POSIX symlink tests for valid IMG/SUB sentinels in a sibling-prefix directory and for a dangling optional SUB.
- Changed CCD opening to return IO when SUB canonicalization fails, while retaining BAD_IMAGE for a resolved path outside the descriptor directory.
- Confirmed failed companion opens leave the already-open ISO reader usable.

## Verification Evidence

| Command | Exit | Result |
| --- | ---: | --- |
| `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` | 0 | Preflight counted 3898/3901 lines |
| `rg -q '^disc[.]image[.]ccd[[:space:]]' tests/disc/inventory.txt && LC_ALL=C sort -cu tests/disc/inventory.txt && ctest --test-dir build/ci -N -R '^disc[.]image[.]ccd$'` | 0 | Inventory sorted and exactly one CCD selector discovered |
| `cmake --preset ci -DSCHWAYSTATION_CHD=OFF -DSCHWAYSTATION_BUILD_CHD_TOOL=OFF` | 0 | CI preset configured |
| `cmake --build --preset ci --target schwaystation-test-disc-ccd` | 0 | Focused target built before RED |
| `ctest --preset ci --no-tests=error --output-junit "$PWD/build/ci/tdd-red-11-13.xml" -R '^disc[.]image[.]ccd$' --output-on-failure` | 8 | Intended RED: dangling optional SUB returned BAD_IMAGE instead of IO |
| `node "$HOME/.codex/gsd-core/bin/gsd-tools.cjs" check tdd-red-evidence build/ci/tdd-red-11-13.json --raw` | 0 | `RED_EVIDENCE_OK`; the named test failed on the planned assertion |
| `cmake --build --preset ci --target schwaystation-test-disc-ccd` | 0 | Focused target rebuilt with the status correction |
| `ctest --preset ci --no-tests=error -R '^disc[.]image[.]ccd$' --output-on-failure` | 0 | CCD integration test passed 1/1 before and after commit |
| `git show --check --oneline 6696aec` | 0 | Implementation commit is whitespace-clean |

No broad suite, Windows/WASI hosted lane, `act` job or hosted CI was run for this focused plan. The POSIX symlink cases are guarded for platforms without the needed symlink APIs; cross-platform canonical-path acceptance remains with the later phase checks.

## TDD Execution Notes

- RED commit `d2648c2` added the companion tests. The preserved JUnit report and semantic check show that the exact dangling-SUB assertion failed because canonicalization failure and an actual path escape shared BAD_IMAGE status.
- GREEN commit `6696aec` split the two outcomes. The focused CCD selector then passed, including the escape sentinels and failed-open preservation check.
- An earlier draft used a SUB symlink name that did not match the descriptor stem. That run did not exercise the intended path and was discarded; the fixture path was corrected before the accepted RED evidence. No history rewrite was made.

## Task Commits

1. **Task 1 RED: add failing CCD companion-boundary tests** — `d2648c2` (`test`)
2. **Task 1 GREEN: distinguish SUB path-resolution errors** — `6696aec` (`fix`)

## Deviations from Plan

None. The implementation stays within the planned files and keeps the line cap and all existing test cases.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Plan 13 is complete. Plan 06 can now build on the confined reader for multi-track/index and SUB conversion. Plan 05 remains independently runnable. Plan 02 remains blocked at 3928/3901 counted lines; retain the cap and its cases while evaluating a semantics-preserving reduction before phase verification or ship. CCD-01 remains incomplete until the rest of Phase 11 and final verification pass. No owner action is needed.

## Self-Check: PASSED

- The summary exists at the expected phase path.
- RED `d2648c2` and GREEN `6696aec` are ancestors of HEAD.
- The measured plan commit count from `af378c0` through `6696aec` is 2.
- `git diff --check` passes.

---
*Phase: 11-ecm-and-clonecd-images*
*Completed: 2026-11-22*
