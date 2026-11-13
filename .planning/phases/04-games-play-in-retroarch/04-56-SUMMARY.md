---
phase: 04-games-play-in-retroarch
plan: "56"
subsystem: testing
tags: [cmake, portability, fixtures, libretro, msvc]
requires:
  - phase: 04-games-play-in-retroarch
    provides:
      - Private binary-open helper pattern and selected-inventory test registration
      - Existing armips fixture producer and libretro fixture consumers
provides:
  - Portable binary opening at all five host disc-probe sites
  - ASan and fuzz fixture production for selected libretro consumers without standalone fixture registration
affects: [04-29, phase-04-hosted-verification]
actuals:
  tokens: 1356
  tasks: 2
  commits: 2
commits: 2
plan_head_before: bc8c247a2c8eac0bbd94a809bd5c4aa0a39b7834
plan_head_after: 8e8d9b8c3941450e445328e7a8a534b2bf042bd4
tech-stack:
  added: []
  patterns:
    - Private fopen_s/fopen binary opener for host disc probing
    - Fixture production is independent of inventory-selected standalone registrations
key-files:
  created: []
  modified:
    - host/disc_probe.c
    - CMakePresets.json
    - tests/tests.cmake
    - tests/fixtures/tests.cmake
    - tests/inventory/asan.txt
    - docs/testing.md
key-decisions:
  - Use a file-local, NULL-initialized fopen_s/fopen helper at every host disc-probe binary open.
  - Keep ASan and fuzz fixture production enabled for real libretro consumers while the selected inventory controls standalone fixture test registration.
patterns-established:
  - Selected inventories control which standalone fixture tests register, independently of generated fixture availability.
requirements-completed: []
coverage:
  - id: D1
    description: All five host disc-probe binary opens use the portable helper; local strict CI build and identity, CUE, malformed-input and ISO edge controls pass.
    requirement: GAME-06
    verification:
      - kind: integration
        ref: cmake --preset ci; build schwaystation-test-probe schwaystation-run schwaystation-run-test; six compat.probe cases, exit 0 before and after the commit
        status: pass
      - kind: integration
        ref: Next changed-head hosted Windows builds, pending
        status: unknown
    human_judgment: false
  - id: D2
    description: ASan and fuzz provide generated inputs to every selected libretro consumer while preserving inventory registration counts.
    requirement: GAME-07
    verification:
      - kind: other
        ref: ASan and fuzz configure with Apple Clang overrides; producer and libretro host build; content.no_bios and cards.persistence pass
        status: pass
      - kind: other
        ref: CTest JSON inventory comparison: CI 74 fixtures/18 libretro, ASan 0/18, fuzz 0/18
        status: pass
      - kind: integration
        ref: cmake --workflow --preset ci-local-4; 509 CI tests and 5 release tests pass; packages produced
        status: pass
      - kind: integration
        ref: Next hosted Linux clang-21 ASan and native Windows matrix, pending
        status: unknown
    human_judgment: false
duration: 16min
completed: 2026-11-12
status: complete
---

# Phase 04 Plan 56: Portable Disc Probe and Sanitizer Fixtures Summary

**Five host disc-probe opens now use a private MSVC-compatible binary opener, while ASan and fuzz build real libretro fixture inputs without changing selected test inventories.**

## Performance

- **Duration:** 16m 19s
- **Started:** 2026-11-12T20:22:51Z
- **Completed:** 2026-11-12T20:39:10Z
- **Tasks:** 2
- **Files modified:** 6
- **Runtime/model:** Codex, OpenGSD 1.20.0; gpt-6-luna, xhigh

## Accomplishments

- Reused a file-local `fopen_s`/`fopen` helper at all five host disc-probe opens. The focused identity, CUE, malformed-input and ISO edge cases passed without changing the existing refusal or cleanup behavior.
- Enabled the existing fixture producer in the ASan and fuzz presets. The selected inventory now governs standalone fixture-test registration separately from generated fixture availability, retaining the CI, ASan and fuzz registration sets.
- Built the producer and libretro host with the local Apple Clang toolchain, then passed the selected content and card consumers. The local compiler override does not establish Linux clang-21 or Windows results.

## Task Commits

1. **Task 1: Run the existing disc identity path through a portable binary opener** — `c0670f5` (`fix`)
2. **Task 2: Build real fixture inputs for every selected sanitizer consumer** — `8e8d9b8` (`fix`)

## Files Created/Modified

- `host/disc_probe.c` — Adds the private portable opener and uses it at all five binary-open sites.
- `CMakePresets.json` — Enables fixture production in the ASan and fuzz presets and corrects the fuzz description.
- `tests/tests.cmake` — Parses the selected inventory before fixture production and shares it with compatibility selection.
- `tests/fixtures/tests.cmake` — Keeps the producer available while skipping standalone test registration when fixtures are unselected.
- `tests/inventory/asan.txt` — Describes producer use and standalone test selection accurately; directory rows are unchanged.
- `docs/testing.md` — Documents the portable openers and producer/registration split.

## Decisions Made

- Keep the opener private to the host disc-probe file and preserve binary mode and existing NULL/error handling.
- Enable the existing producer for actual ASan/fuzz libretro consumers; do not weaken or change their selected inventories.

## Deviations from Plan

None — the plan was executed as written.

## Evidence

- Task 1 verification command, run before and after its commit: `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-probe schwaystation-run schwaystation-run-test && ctest --test-dir build/ci --parallel 4 -R '^compat[.]probe[.](synthetic_identity|full_identity|malformed|cue_modes|cue_edges|iso_edges)$' --no-tests=error --output-on-failure` — exit 0 both times; all six cases passed.
- ASan and fuzz graph configure commands with `-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++` — exit 0 for both. These use Apple Clang locally; they are not Linux clang-21 evidence.
- `cmake --build build/asan-graph --target schwaystation-fixtures schwaystation-test-libretro-host` — exit 0; the pinned fixture producer and host test target built.
- `ctest --test-dir build/asan-graph --parallel 1 -R '^libretro[.]content[.]no_bios$' --no-tests=error --output-on-failure` and `ctest --test-dir build/asan-graph --parallel 1 -R '^libretro[.]cards[.]persistence$' --no-tests=error --output-on-failure` — exit 0 for both selected consumers.
- One-shot CTest JSON comparison — exit 0; CI registered 74 standalone fixture and 18 libretro cases, while ASan and fuzz each registered zero standalone fixture and all 18 libretro cases.
- `cmake --preset ci` and `git diff --check HEAD` — exit 0 after the task-2 edits.
- `cmake --workflow --preset ci-local-4` — exit 0; 509/509 CI tests and 5/5 release tests passed, and all three packages were produced. The input-movie case took 74.55 seconds.
- `cmake --workflow --preset hygiene` — exit 0; all 23 checks passed, including the repository-check and end-to-end budget checks. The workflow reused the existing timeout scale of four; total test time was 204.53 seconds.

## Issues Encountered

No implementation blockers. Native Windows builds and Linux clang-21 sanitizer acceptance remain pending the coordinator's next changed-head hosted run. No new hosted run was started by this plan.

## Next Phase Readiness

Plan 29 remains incomplete. The coordinator continues with the separate movie/BE32 timing diagnosis, then the next hosted changed-head run and Plan 29's distinct-frame acceptance. The existing local RetroArch stop remains unchanged; no owner action is needed for this plan.

## Self-Check: PASSED

- Summary file exists.
- Task commits `c0670f5` and `8e8d9b8` are ancestors of HEAD.
- The persisted plan ledger measures two commits from `bc8c247a2c8eac0bbd94a809bd5c4aa0a39b7834` through `8e8d9b8c3941450e445328e7a8a534b2bf042bd4`.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-12*
