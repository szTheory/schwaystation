---
phase: 11-ecm-and-clonecd-images
plan: "14"
subsystem: testing
tags: [ecm, ccd, cpack, libretro]

requires:
  - phase: 11-12
    provides: Nightly ECM and CCD fixture/replay producer closure
provides:
  - Local CHD-OFF runner/libretro archive and notice evidence for ECM and CCD rows
  - Separate unknown Main, Nightly and Release hosted evidence fields
affects: [11-15, phase-11-verification, compatibility-ledger]

actuals:
  tokens: 1553
  tasks: 2
  commits: 1
commits: 1
plan_head_before: ca465a9a69bb065cfa158905a4116463ffef7ffc
plan_head_after: 7f31a0fab3b62ad43da713bba53c3da15eb5aa34

tech-stack:
  added: []
  patterns:
    - Keep local CHD-OFF package evidence distinct from Main, Nightly and Release workflow evidence.

key-files:
  created: []
  modified:
    - tests/compat/features.csv
    - tests/compat/tests.cmake
    - docs/media-testing.md

key-decisions:
  - "Record the observed local package result and leave each hosted workflow status unknown until that workflow runs."

requirements-completed: []
coverage:
  - id: D1
    description: Local CHD-OFF runner and libretro archives passed existing content and notice checks for ECM support.
    requirement: ECM-01
    verification:
      - kind: integration
        ref: "cmake --preset ci -DSCHWAYSTATION_CHD=OFF -DSCHWAYSTATION_BUILD_CHD_TOOL=OFF; cmake --build --preset ci --target schwaystation-run schwaystation_libretro; cpack --config build/ci/CPackConfig.cmake -C Release; ctest --test-dir build/ci --no-tests=error -R '^release[.](archive_contents|notices)([.]self_test)?$' --output-on-failure"
        status: pass
      - kind: other
        ref: "hosted_main, hosted_nightly and hosted_release fields remain unknown"
        status: unknown
    human_judgment: false
  - id: D2
    description: Local CHD-OFF runner and libretro archives passed existing content and notice checks for CloneCD support.
    requirement: CCD-01
    verification:
      - kind: integration
        ref: "cmake --preset ci -DSCHWAYSTATION_CHD=OFF -DSCHWAYSTATION_BUILD_CHD_TOOL=OFF; cmake --build --preset ci --target schwaystation-run schwaystation_libretro; cpack --config build/ci/CPackConfig.cmake -C Release; ctest --test-dir build/ci --no-tests=error -R '^release[.](archive_contents|notices)([.]self_test)?$' --output-on-failure"
        status: pass
      - kind: other
        ref: "hosted_main, hosted_nightly and hosted_release fields remain unknown"
        status: unknown
    human_judgment: false

duration: 13min
completed: 2026-11-22
status: complete
---

# Phase 11 Plan 14: CHD-OFF package evidence

**CHD-OFF runner and libretro Release archives pass the existing local package, metadata and notice checks.**

## Performance

- **Duration:** 13 minutes.
- **Started:** 2026-11-22T19:48:06Z
- **Completed:** 2026-11-22T20:01:03Z
- **Tasks:** 2 (Task 1 was preflight-only; Task 2 contained the edits).
- **Files modified:** 3.
- **Runtime/model:** Codex/OpenGSD 1.20.0; configured executor `gpt-6-luna`; primary runtime model ID unavailable.
- **Bottleneck:** The checks were local and fast. The only adjustment was using the registered CTest locations and a header-aware sort check.

## Accomplishments

- Configured CI with CHD and the optional CHD tool disabled. Built `schwaystation-run` and `schwaystation_libretro`; the generated build graph and static archive include both `disc_ecm.c.o` and `disc_ccd.c.o`.
- Passed libretro vendored and info-claim selectors (4/4), the existing hygiene source-notice selector (2/2), and Release archive-content/notice selectors (4/4).
- Generated the three local macOS arm64 Release CPack archives. The ECM and CCD feature rows now record local CHD-OFF archive/notice evidence separately from unknown Main, Nightly and Release hosted fields.
- Kept `requirements-completed` empty. Local package checks do not complete ECM-01 or CCD-01.

## Verification Evidence

| Command | Exit | Result |
| --- | ---: | --- |
| `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` | 0 | 3,899/3,901 counted lines |
| `ctest --test-dir build/ci -N -R '^libretro[.](vendored|info[.]claims)([.]self_test)?$|^policy[.]notices([.]self_test)?$|^release[.](archive_contents|notices)([.]self_test)?$'` | 0 | Discovered 8 package selectors; the hygiene-only `policy.notices` selectors were discovered separately in `build/hygiene` |
| `tail -n +2 tests/compat/features.csv \| LC_ALL=C sort -cu` and `cmake -DFEATURES=tests/compat/features.csv -DMODE=features-preflight -P tests/cmake/compat-ratchet.cmake` | 0 | Data rows byte-sorted; feature schema, keys and order accepted |
| `cmake --preset ci -DSCHWAYSTATION_CHD=OFF -DSCHWAYSTATION_BUILD_CHD_TOOL=OFF` | 0 | CHD-OFF CI preset configured |
| `cmake --build --preset ci --target schwaystation-run schwaystation_libretro` | 0 | Requested products built; archive contains both new provider objects |
| `ctest --preset ci --no-tests=error -R '^libretro[.](vendored|info[.]claims)([.]self_test)?$' --output-on-failure` | 0 | 4/4 passed |
| `ctest --test-dir build/hygiene --no-tests=error -R '^policy[.]notices([.]self_test)?$' --output-on-failure` | 0 | 2/2 passed |
| `cpack --config build/ci/CPackConfig.cmake -C Release` | 0 | Library, runner and libretro archives generated |
| `ctest --test-dir build/ci --no-tests=error -R '^release[.](archive_contents|notices)([.]self_test)?$' --output-on-failure` | 0 | 4/4 passed |
| `ctest --test-dir build/ci -N --no-tests=error -R '^compat[.]features[.](preflight|ratchet)$'` and the same selector without `-N` | 0 | Both feature-ledger checks discovered and passed (2/2) |
| `git diff --check` | 0 | No whitespace errors |

No hosted Main, Nightly or Release workflow was dispatched. No timed campaign or broad CI suite was run.

## Task Commits

Task 1 changed no files; its line-budget, selector-discovery and ledger-order preflights are recorded above. Task 2 was committed as `7f31a0f` (`docs(11-14): record local CHD-OFF package evidence`).

## Files Created/Modified

- `tests/compat/features.csv` — records local CHD-OFF package evidence and leaves hosted workflow fields unknown.
- `tests/compat/tests.cmake` — updates the independent expected ledger rows without weakening the compatibility check.
- `docs/media-testing.md` — describes the exact local package scope and hosted-evidence boundary.

## Deviations from Plan

1. The plan's `sort -cu tests/compat/features.csv` includes the CSV header, so it reported disorder even though the data rows are sorted. Used `tail -n +2 ... | LC_ALL=C sort -cu` and the existing CMake feature preflight instead; no rows were reordered.
2. `policy.notices` is registered only in the hygiene inventory, not the CI preset. Ran the exact existing selector from `build/hygiene`; archive notices were separately checked by the Release selectors.
3. `ctest --preset ci` excludes the `release` label and returned exit 8 with no tests for the plan's Release selector. Reused the exact discovered tests directly from `build/ci`; archive-content and notice checks passed 4/4.
4. An initial ad-hoc feature selector omitted the `.preflight`/`.ratchet` name components and matched zero tests. The corrected exact selector discovered and passed both tests.

## Issues Encountered

The local preset-specific selector boundaries differed from the plan's command text. The registered tests ran through their existing hygiene and Release CTest directories without changing or dropping checks.

## User Setup Required

None.

## Next Phase Readiness

Plan 15 owns the installed SDL2 example link and CHD-ON consumer checks. Main, Nightly and Release hosted evidence, source review, and final Phase 11 verification remain outstanding. No user action is needed.

## Self-Check: PASSED

- The summary exists at the expected phase path.
- Task commit `7f31a0f` is an ancestor of HEAD.
- The plan base is the measured ledger value `ca465a9a69bb065cfa158905a4116463ffef7ffc`.

---
*Phase: 11-ecm-and-clonecd-images*
*Completed: 2026-11-22*
