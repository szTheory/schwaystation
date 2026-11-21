---
phase: 10-pbp-disc-images
plan: "12"
subsystem: testing
tags: [c, cmake, ctest, pbp, compatibility, disc-guest]
requires:
  - phase: 10-pbp-disc-images
    provides:
      - Bounded PBP provider, runner and libretro support
      - Deterministic PBP fixture and full source parity
      - CHD ON/OFF package, notice and metadata evidence
provides:
  - Bounded one-data-track PBP boot, read/replay and first-frame guest evidence
  - Independent protected feature-ledger expectation for the exercised PBP subset
  - PBP scope and preflight documentation
affects: [compatibility-ledger, media-testing, phase-10-verification]
actuals:
  tokens: 8338
  tasks: 2
  commits: 2
commits: 2
plan_head_before: 68a253401320b0ba62a2e627997224c89b13c51e
plan_head_after: 7a846f8ee3210e140d8cae04f6686eb3b7282e1a
tech-stack:
  added: []
  patterns:
    - Use a separate one-track guest build when the PBP source fixture cannot exercise the original two-track CDDA sequence.
    - Gate guest launch on fixture preflight, full source parity and compatibility feature preflight.
    - Keep the protected expected feature snapshot independent from the candidate ledger.
key-files:
  created:
    - tests/fixtures/disc_boot_pbp.xml
  modified:
    - cmake/DiscFixtures.cmake
    - cmake/PBPFixtures.cmake
    - tests/fixtures/src/disc_boot.asm
    - tests/disc/fixture.c
    - tests/disc/tests.cmake
    - tests/disc/inventory.txt
    - tests/compat/features.csv
    - tests/compat/tests.cmake
    - docs/compatibility.md
    - docs/media-testing.md
    - tests/fixtures/manifest.txt
key-decisions:
  - Keep PBP guest evidence to the owned 64-sector one-data-track NTSC-U source and omit the CUE/CHD-only CDDA path.
  - Record the PBP feature as a narrow pass with hosted CI explicitly marked not run; leave audio and untested layouts unsupported or unknown.
requirements-completed: []
coverage:
  - id: pbp_one_track_guest
    description: The owned unencrypted one-data-track PBP passes bounded boot, read/replay and first-FMV-frame predicates after complete source parity.
    requirement: PBP-01
    verification:
      - kind: integration
        ref: "cmake --build --preset ci --target schwaystation-test-disc-fixture schwaystation-test-disc-pbp schwaystation-pbp-fixtures; ctest --preset ci --no-tests=error -R '^disc[.]fixture[.]pbp[.](boot|read|fmv)$' --output-on-failure; exit 0, 8/8 including preflights, 7.92s CTest time"
        status: pass
    human_judgment: false
  - id: pbp_feature_ledger
    description: The candidate feature row is checked against a separately authored two-row expected snapshot, and generated compatibility documentation matches the ledger.
    requirement: PBP-01
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^compat[.]features[.]|^compat[.]ratchet[.](baseline|protected_controls|transitions)$|^policy[.]compat_doc_fresh$' --output-on-failure; exit 0, 6/6 in 1.27s"
        status: pass
    human_judgment: false
  - id: phase_hosted_evidence
    description: Exact-head hosted lanes and linked WASI size remain for Phase 10 Plan 10-13; this plan does not claim hosted or cross-variant coverage.
    requirement: PBP-01
    verification:
      - kind: other
        ref: "Phase 10 Plan 10-13 is the authorized exact-head hosted verification step; no hosted CI run was made in Plan 10-12."
        status: unknown
    human_judgment: false
duration: 41m
completed: 2026-11-21
status: complete
---

# Phase 10 Plan 12: Bounded PBP Guest and Compatibility Evidence Summary

**The owned one-data-track PBP now passes bounded boot, read/replay and first-frame guest checks, with only that demonstrated subset recorded as compatible.**

## Performance

- **Runtime/model:** Codex executor; model ID and reasoning effort were not exposed in this context.
- **Elapsed:** About 41 minutes from the saved STATE checkpoint at 14:31:41Z through closeout at 15:12:27Z; active time was not separately measured.
- **Tasks and files:** 2 tasks, 2 task commits, 12 changed source/test/documentation files.
- **Measured bottleneck:** The targeted PBP guest command passed in 7.92 seconds; the compatibility and documentation selector took 1.27 seconds. An overlapping standalone and registered hygiene-tree invocation duplicated work once (61.77 seconds); it added no coverage and was not repeated. Later budget verification used only the two budget selectors.

## Accomplishments

- Added a separate PBP-mode guest and 64-sector one-data-track source image so PBP can assert one-track TOC behavior without changing the original two-track CUE/CHD CDDA sequence.
- Required PBP fixture preflight, full source parity and compatibility feature preflight before the PBP guest selectors. Boot, 16-sector read/replay and first FMV frame passed. PBP makes no CDDA or audio-hash claim.
- Added a PBP compatibility pass with explicit one-track TOC/sector/subcode, runner, libretro, CHD ON/OFF package and bounded guest evidence. The expected CSV remains a separately authored literal; hosted CI is explicitly marked not run.
- Updated generated compatibility output and media-testing instructions to state the exact tested subset and preflight order. Other track layouts, profiles, encrypted/PGD, multidisc and audio behavior remain unsupported or unknown.
- Preserved all existing CUE/CHD two-track and CDDA records, hashes, guest bounds, and the 3,901-line repository-check ceiling (measured 3,900/3,901).

## Commands and Evidence

- Sorted disc inventory check — exit 0:
  ```sh
  LC_ALL=C sort -cu tests/disc/inventory.txt
  ```
- The PBP source-count sanity check confirmed exactly two ledger rows and one passing PBP row for the `guest.boot+read+fmv_first_frame` predicate — exit 0.
- `cmake --preset ci -DSCHWAYSTATION_CHD=ON -DSCHWAYSTATION_BUILD_CHD_TOOL=ON` — exit 0.
- `cmake --build --preset ci --target schwaystation-test-disc-fixture schwaystation-test-disc-pbp schwaystation-pbp-fixtures` — exit 0.
- `ctest --preset ci --no-tests=error -R '^disc[.]fixture[.]pbp[.](boot|read|fmv)$' --output-on-failure` — exit 0, 8/8 including fixture preflight, parity, compatibility preflight and CUE FMV predicate prerequisite. The PBP guest predicates all passed.
- Original CUE/CHD bounded boot/read/audio/FMV selection — exit 0, 11/11; existing two-track and CDDA assertions remain unchanged.
- `ctest --preset ci --no-tests=error -N -R '^compat[.]features[.]|^compat[.]ratchet[.](baseline|protected_controls|transitions)$|^policy[.](compat_doc_fresh|gate_budget)$'` — exit 0; all six affected compatibility/documentation selectors were registered. No CI-preset policy gate-budget selector exists; the dedicated hygiene preset owns that check.
- `ctest --preset ci --no-tests=error -R '^compat[.]features[.]|^compat[.]ratchet[.](baseline|protected_controls|transitions)$|^policy[.]compat_doc_fresh$' --output-on-failure` — exit 0, 6/6 in 1.27 seconds.
- Compatibility documentation freshness command — exit 0:
  ```sh
  scripts/compat-doc.sh tests/compat/ledger.csv docs/compatibility.md --check
  ```
- `ctest --preset hygiene --no-tests=error -R '^policy[.]gate_budget([.]self_test)?$' --output-on-failure` — exit 0, 2/2. Direct measurement: `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` — exit 0, 3,900/3,901.
- Diff and post-commit status/deletion checks — exit 0; no generated or task-source untracked files remain. The plan summary was the only pending planning artifact:
  ```sh
  git diff --check origin/main...HEAD
  git diff --check HEAD
  ```
- No hosted CI or `act` run applied in this plan because workflow routing did not change. Exact-head hosted verification and WASI module measurement remain in Plan 10-13.

## Task Commits

1. **Task 1: Boot and read the owned single-track PBP with existing guest bounds** — `f92ac8c`.
2. **Task 2: Record only demonstrated PBP variants in the protected feature ledger** — `7a846f8`.

**Plan metadata:** summary, STATE and ROADMAP metadata commit is recorded in the plan completion result.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing prerequisite] Added a PBP-specific one-track source guest and wired parity as a fixture prerequisite.**
- **Found during:** Task 1.
- **Issue:** Reusing the original guest sequence on the one-track PBP would request track 2 and CDDA at LBA 214, behavior absent from the PBP fixture.
- **Fix:** Built the source guest in an explicit PBP mode that checks the absent second track and omits CDDA, while compiling the unchanged two-track/CDDA sequence for CUE and CHD. Wired PBP preflight and full source parity before launch.
- **Files modified:** `cmake/DiscFixtures.cmake`, `cmake/PBPFixtures.cmake`, `tests/fixtures/disc_boot_pbp.xml`, `tests/fixtures/src/disc_boot.asm`, `tests/fixtures/manifest.txt`, `tests/disc/fixture.c`, `tests/disc/tests.cmake`, `tests/disc/inventory.txt`.
- **Commit:** `f92ac8c`.

## Self-Check: PASSED
