---
phase: 13-regions-console-revisions-and-playback
plan: "07"
subsystem: core
tags: [bios, fast-boot, profile-flags, save-states]
requires:
  - phase: 13-04
    provides: Serialized board identity and profile flags
  - phase: 13-06
    provides: Region and machine profile selection for host adapters
provides:
  - Explicit opt-in fast-boot profile flag with state validation and round-trip coverage
  - Bounded shell-entry handoff at 0x80030000 after BIOS initialization
affects: [phase-13-boot-integration, phase-13-verification]
actuals:
  tokens: 3989
  tasks: 2
  commits: 2
commits: 2
plan_head_before: "cfe434109311ce2244630edb5798dee06e98e9f2"
plan_head_after: "ec6d167085be33b4b53b4ff117a7773aeac59c18"
tech-stack:
  added: []
  patterns:
    - Opt-in profile behavior gated at a precise CPU run-loop boundary
key-files:
  created: []
  modified:
    - include/schwaystation.h
    - src/instance.c
    - src/cpu.c
    - src/state.c
    - tests/core/test_core.c
    - tests/core/test_state.c
    - tests/core/tests.cmake
    - tests/core/inventory.txt
key-decisions:
  - Full BIOS boot remains the default; only the new explicit profile flag enables the shell handoff.
  - State format and behavior revision remain 19 because the existing serialized profile field carries the new opt-in input without changing default behavior.
patterns-established:
  - Test fast-boot eligibility and adjacent-PC controls with bounded core execution and owned runtime setup.
requirements-completed: []
coverage:
  - id: D1
    description: The fast-boot profile flag is accepted, survives state round trips, and unknown profile bits remain atomically rejected.
    requirement: BOOT-01
    verification:
      - kind: unit
        ref: ctest --preset ci --no-tests=error -R '^(core[.]profile[.](fast_boot|machine|rejected)|core[.]state[.](profile|rejects))$' --output-on-failure
        status: pass
    human_judgment: false
  - id: D2
    description: The CPU skips only the initialized BIOS shell entry in eligible fast mode; default boot, missing BIOS/disc, and nearby PCs execute normally.
    requirement: BOOT-01
    verification:
      - kind: unit
        ref: ctest --preset ci --no-tests=error -R '^core[.]boot[.]fast$' --output-on-failure
        status: pass
    human_judgment: false
duration: not reliably tracked
completed: 2026-11-23
status: complete
---

# Phase 13 Plan 13-07: BIOS shell fast boot Summary

**The core now has an explicit fast-boot profile that returns from the initialized BIOS shell entry while preserving ordinary full boot by default.**

## Performance

- **Duration:** Not reliably tracked; no total task duration is claimed.
- **Started:** Not reliably tracked.
- **Completed:** 2026-11-23.
- **Tasks:** 2.
- **Files modified:** 8.
- **Runtime/model:** Codex GSD executor; configured `gpt-6-luna` xhigh; primary runtime model ID unavailable.
- **Measured task commits:** 2, from `cfe434109311ce2244630edb5798dee06e98e9f2` through `ec6d167085be33b4b53b4ff117a7773aeac59c18`.

## Accomplishments

- Added `SCHWAY_PROFILE_FAST_BOOT` as an opt-in profile flag and covered acceptance, state round-trip, and atomic rejection of unknown bits.
- Added a bounded CPU hook at the initialized shell entry `0x80030000`; it returns to `r31` only with fast mode enabled, a loaded BIOS, and an attached disc.
- Tested ordinary shell execution, missing BIOS/disc, and adjacent PCs to ensure the hook is narrowly gated.

## Task Commits

1. **Task 1: Add an opt-in fast-boot profile flag** — `16c6c7b` (`feat`).
2. **Task 2: Intercept only the initialized BIOS shell entry** — `ec6d167` (`feat`).

## Files Created/Modified

- `include/schwaystation.h` — declares the fast-boot profile flag.
- `src/instance.c` — accepts the flag during profile selection.
- `src/state.c` — validates and serializes the flag in the existing profile field.
- `src/cpu.c` — returns to the initialized caller at the exact shell-entry boundary when eligible.
- `tests/core/test_state.c` — tests profile round-trip and unknown-bit rejection.
- `tests/core/test_core.c` — tests eligible handoff, normal boot, and negative controls.
- `tests/core/tests.cmake` — registers profile and boot selectors.
- `tests/core/inventory.txt` — adds the two sorted selector names.

## Decisions Made

- Full BIOS boot remains the default; fast boot requires the explicit profile flag and does not synthesize BIOS services.
- State format remains 19 because profile flags were already serialized. Behavior revision remains 19 because the new option is an explicit input and the default behavior is unchanged.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking issue] Replaced an unavailable test helper in the state test**

- **Found during:** Task 1 target build.
- **Issue:** The new `test_state.c` case called `make_profile`, a helper defined only in `test_core.c`; the first build failed in 0.55s before CTest ran.
- **Fix:** Constructed the profile locally with zero initialization and explicit size and region fields.
- **Files modified:** `tests/core/test_state.c`.
- **Verification:** The corrected changed-target build and focused profile/state CTest command passed; all seven selected tests passed.
- **Committed in:** `16c6c7b`.

**Total deviations:** 1 auto-fixed blocking issue. No scope was added.

## Test Evidence

- `LC_ALL=C sort -cu tests/core/inventory.txt`: exit 0.
- `cmake -DROOT=. -DLIMIT=3901 '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -P tests/cmake/line_budget.cmake`: exit 0 at 3,901 lines.
- `cmake --preset ci`: exit 0 in 0.7s.
- After Task 1 registration, the exact profile/state selector closure matched 7/7 names, including fixture setup.
- `cmake --build --preset ci --target schwaystation-test-core schwaystation-test-state && ctest --preset ci --no-tests=error -R '^(core[.]profile[.](fast_boot|machine|rejected)|core[.]state[.](profile|rejects))$' --output-on-failure`: exit 0 in 0.656s; CTest passed 7/7 in 0.52s.
- `cmake --build --preset ci --target schwaystation-test-core`: exit 0 in 0.359s.
- `ctest --preset ci --no-tests=error -R '^core[.]boot[.]fast$' --output-on-failure`: exit 0; exact closure included `core.boot.fast` and the two disc fixtures (3/3), all passed in 0.16s.
- `git diff --check`: exit 0.

The first Task 1 compilation failed before tests because of the test-helper scope error described above; the local profile setup fixed it. No other final focused selector failures remained.

No workflow wiring changed, so no broad CI, Act, hosted CI, or Docker run was used. These focused core checks do not complete the broader runner/core eligibility acceptance: `BOOT-01` remains pending until Phase 13 verification.

## Issues Encountered

The initial state-test compile failure was fixed and the corrected focused checks passed. No auth gate or external setup is required.

## User Setup Required

None — no external service configuration is required.

## Next Phase Readiness

The core hook is ready for Plan 13-08 host integration. `BOOT-01` remains pending until Phase 13 verification. Plan 13-08 is next.

---
*Phase: 13-regions-console-revisions-and-playback*
*Completed: 2026-11-23*

## Self-Check: PASSED

The summary file exists, parses as YAML, and both task commits are ancestors of the recorded plan head.
