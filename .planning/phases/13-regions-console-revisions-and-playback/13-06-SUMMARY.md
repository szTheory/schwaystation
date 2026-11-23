---
phase: 13-regions-console-revisions-and-playback
plan: "06"
subsystem: libretro
tags: [libretro, board-profiles, region-selection, disc-identity]
requires:
  - phase: 13-03
    provides: canonical board identities and profile defaults
  - phase: 13-05
    provides: shared host-side disc-region resolution and runner options
provides:
  - Independent libretro machine and region choices applied at content load
  - PU-18 machine default and fallback with all nine advertised board tokens supported
  - Content identity resolution through the shared host resolver
affects: [libretro, phase-13-profile-verification]
actuals:
  tokens: 6187
  tasks: 2
  commits: 2
commits: 2
plan_head_before: "87eac29d1fec593efb7e019c7cbd4ef6fb3b6408"
plan_head_after: "a8e803beb950881edc3e2073b99aa73edf618ba0"
tech-stack:
  added: []
  patterns:
    - Shared host-side region resolver linked into the libretro production and test targets
key-files:
  created: []
  modified:
    - libretro/CMakeLists.txt
    - libretro/libretro.c
    - tests/libretro/host.c
    - tests/libretro/tests.cmake
    - tests/libretro/inventory.txt
    - docs/libretro.md
    - .planning/phases/13-regions-console-revisions-and-playback/13-06-PLAN.md
key-decisions:
  - Libretro defaults to PU-18 to follow D-01; the library default remains unchanged.
  - Machine selection and region selection remain independent options applied on content load.
requirements-completed: []
coverage:
  - id: D1
    description: Libretro applies independent machine and region choices at content load, including all nine board tokens and the PU-18 default and fallback.
    requirement: PROFILE-02
    verification:
      - kind: integration
        ref: ctest --preset ci --no-tests=error -R '^libretro[.](content[.]profile|options[.]profile|info[.]claims)$'
        status: pass
    human_judgment: false
  - id: D2
    description: Existing PAL and NTSC disc-audio cadence comparisons remain covered by the libretro host test.
    requirement: PROFILE-01
    verification:
      - kind: integration
        ref: ctest --preset ci --no-tests=error -R '^libretro[.]host[.]disc_audio$'
        status: pass
    human_judgment: false
duration: not reliably tracked
completed: 2026-11-23
status: complete
---

# Phase 13 Plan 13-06: Libretro machine and region options Summary

**Libretro now applies a documented board and resolved region independently at content load, defaulting to PU-18 while retaining PAL/NTSC audio-cadence coverage.**

## Performance

- **Duration:** Not reliably tracked; no total task duration is claimed.
- **Started:** Not reliably tracked.
- **Completed:** 2026-11-23.
- **Tasks:** 2.
- **Files modified:** 7.
- **Runtime/model:** Codex GSD executor; configured `gpt-6-luna` xhigh; primary runtime model ID unavailable.
- **Measured task commits:** 2, from `87eac29d1fec593efb7e019c7cbd4ef6fb3b6408` through `a8e803beb950881edc3e2073b99aa73edf618ba0`.

## Accomplishments

- Added independent machine and region options; all nine board tokens and explicit region choices are applied at content load.
- Auto region selection uses the shared host-side identity resolver before a concrete region reaches the library.
- Documented the reload behavior, PU-18 default, hint precedence, and conflict diagnostics.
- Kept the existing PAL and NTSC disc-audio cadence comparison gating.

## Task Commits

1. **Task 1: Resolve board and region options during content load** — `c072d8e` (`feat`).
2. **Task 2: Document reload and preserve PAL audio cadence** — `a8e803b` (`docs`).

## Files Created/Modified

- `libretro/CMakeLists.txt` — links the shared region and disc-probe resolver into the production core.
- `libretro/libretro.c` — defines and applies machine/region options, with PU-18 default/fallback and explicit PM-41(2) parsing.
- `tests/libretro/host.c` — exercises option registration, defaults, fallback, all board/region selections, and content identity behavior.
- `tests/libretro/tests.cmake` — registers profile selectors and their disc-media fixture requirement.
- `tests/libretro/inventory.txt` — adds the profile selectors.
- `docs/libretro.md` — explains machine and region options and their content-reload behavior.
- `.planning/phases/13-regions-console-revisions-and-playback/13-06-PLAN.md` — records CMake target ownership and the shared resolver link requirement.

## Decisions Made

- Libretro's newly selected machine defaults to PU-18, matching D-01. The library's existing default decision was not changed.
- Invalid or absent machine-option values fall back to PU-18; advertised PM-41(2) remains selectable through its explicit token.
- Profile selection tests establish profile application only; they do not establish title compatibility.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking issue] Linked the shared resolver into libretro targets**

- **Found during:** Task 1 configure and target registration.
- **Issue:** The first CI configure attempt failed before any target build; the production and test targets needed the shared `host/disc_probe.c` and `runner/region.c` sources and include paths.
- **Fix:** Added those sources and include paths to the production core and host test targets, and added `libretro/CMakeLists.txt` to the plan's task file list and resolver-link note.
- **Verification:** Corrected CI configure passed in 0.8s; the exact profile selector closure later matched 6/6 tests.
- **Committed in:** `c072d8e`.

**2. [Rule 1 - Bug] Preserved the advertised PM-41(2) option after changing the fallback**

- **Found during:** Final D-01 default/fallback review.
- **Issue:** After the default changed to PU-18, three PM-41(2) profile comparisons failed across the three regions because the advertised `pm41-2` token had depended on the former PM-41(2) fallback.
- **Fix:** Added an explicit `pm41-2` mapping to `SCHWAY_BOARD_PM41_2`; kept PU-18 as the option default/fallback and left the library default unchanged.
- **Verification:** The isolated option-profile test passed, then the exact six-test fixture closure passed.
- **Committed in:** `c072d8e`.

**Total deviations:** 2 auto-fixed issues. Both were required to configure and preserve the documented option behavior.

## Test Evidence

- Sorted `tests/libretro/inventory.txt`: passed.
- Fixed repository-check budget: passed at 3,901 lines.
- CI configure: passed in 0.8s.
- Fixture producers: current; no rebuild work was required.
- `policy.fixture_budget`: passed 1/1.
- Changed-target build (`schwaystation-test-libretro-host`, `schwaystation_libretro`): passed in 0.465s.
- Exact profile selector closure: 6/6 names including fixture setup; all six tests passed in 0.44s.
- Exact `libretro.host.disc_audio` selector closure: 1/1; the test passed in 2.20s, retaining both PAL and NTSC cases and their assertions.
- `git diff --check`: passed.

The first detached audio-test launch produced no authoritative exit status. It was treated as lost and rerun as one managed test session; the 2.20s tracked pass above is authoritative. An earlier profile run after the PU-18 change failed its three PM-41(2) comparisons; the explicit token mapping fixed the cause. The initial configure failure occurred before any build or test and was resolved by adding the shared resolver sources and include paths to the affected targets.

No workflow wiring changed, so no broad CI, Act, hosted CI, or Docker run was used.

## Issues Encountered

The failures described above were resolved and the final focused checks passed. No auth gate or external setup is required.

## User Setup Required

None — no external service configuration is required.

## Next Phase Readiness

Plan 13-07 is next. `PROFILE-01` and `PROFILE-02` remain pending until full Phase 13 verification; this plan does not make a title-compatibility or phase-completion claim.

---
*Phase: 13-regions-console-revisions-and-playback*
*Completed: 2026-11-23*

## Self-Check: PASSED

The summary file exists, YAML frontmatter parses, and both task commits are ancestors of the current plan head.
