---
phase: 11-ecm-and-clonecd-images
plan: "15"
subsystem: testing
tags: [ecm, ccd, sdl2, install, cmake]

requires:
  - phase: 11-14
    provides: CHD-OFF package and notice evidence for ECM and CCD
provides:
  - Installed-library SDL2 example compile with ECM and CCD host providers
  - Focused local CHD-ON provider, libretro info-claim and install-consumer evidence
affects: [phase-11-verification, compatibility-ledger]

actuals:
  tokens: 1795
  tasks: 2
  commits: 1
commits: 1
plan_head_before: 06a5c79afeb08d8dc63eae81f97186123201fc1b
plan_head_after: cbc347c8f41f7f90d1bc18bc615d34c57d5117dd

tech-stack:
  added: []
  patterns:
    - Compile the optional example against an explicitly selected staged CMake package.
    - Keep local example and CHD-ON evidence separate from hosted workflow evidence.

key-files:
  created: []
  modified:
    - examples/sdl2/CMakeLists.txt
    - tests/compat/features.csv
    - tests/compat/tests.cmake

key-decisions:
  - "Record local staged-example and CHD-ON checks while leaving hosted Main, Nightly and Release results unknown."

requirements-completed:
  - ECM-01
  - CCD-01
coverage:
  - id: D1
    description: The optional installed-library SDL2 example compiles with ECM and CCD providers and is not launched, tested or packaged.
    requirement: ECM-01
    verification:
      - kind: integration
        ref: "cmake --install build/ci --prefix build/ci/stage --component library && cmake -S examples/sdl2 -B build/example-sdl2-legacy -DCMAKE_PREFIX_PATH=... -Dschwaystation_DIR=... && cmake --build build/example-sdl2-legacy --parallel 2"
        status: pass
    human_judgment: false
  - id: D2
    description: CHD-ON runner, libretro and ECM/CCD provider builds and focused provider, info-claim and install-consumer checks pass locally.
    requirement: CCD-01
    verification:
      - kind: integration
        ref: "cmake --preset ci -DSCHWAYSTATION_CHD=ON -DSCHWAYSTATION_BUILD_CHD_TOOL=ON && cmake --build --preset ci --target schwaystation-run schwaystation_libretro schwaystation-test-disc-ecm schwaystation-test-disc-ccd; ctest --preset ci -R '^disc[.]image[.](ecm|ccd)$|^libretro[.]info[.]claims([.]self_test)?$|^install[.]consumer'"
        status: pass
      - kind: other
        ref: "hosted_main, hosted_nightly and hosted_release remain unknown"
        status: unknown
    human_judgment: false

duration: 6min
completed: 2026-11-22
status: complete
---

# Phase 11 Plan 15: installed SDL2 and CHD-ON consumers

**The installed SDL2 host now compiles ECM and CCD providers, and focused CHD-ON consumers pass locally.**

## Performance

- **Duration:** 6 minutes.
- **Started:** 2026-11-22T20:05:25Z
- **Completed:** 2026-11-22T20:11:54Z
- **Tasks:** 2 (Task 1 was preflight-only; Task 2 contained the changes).
- **Files modified:** 3.
- **Runtime/model:** Codex/OpenGSD 1.20.0; configured executor `gpt-6-luna`; primary runtime model ID unavailable.
- **Bottleneck:** None; the staged example build and focused CHD-ON checks completed quickly.

## Accomplishments

- Confirmed 3,899/3,901 counted lines, discovered all 10 focused provider/info/install selectors, and found installed SDL2 2.32.70 before configuring the example.
- Added `host/disc_ecm.c` and `host/disc_ccd.c` to the SDL2 executable's private source list. The installed-library example built successfully against the staged package with an explicit `schwaystation_DIR`; it was not launched, tested or packaged.
- Configured CHD-ON and built the runner, libretro core and both provider test targets. Focused provider/info/install checks passed 10/10; feature preflight/ratchet checks passed 2/2. Both compatibility rows record only observed local results, with Main, Nightly and Release still unknown.
- Kept `requirements-completed` empty. Local package and consumer evidence do not complete ECM-01 or CCD-01.

## Verification Evidence

| Command | Exit | Result |
| --- | ---: | --- |
| `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` | 0 | 3,899/3,901 counted lines |
| `ctest --test-dir build/ci -N -R '^disc[.]image[.](ecm|ccd)$|^libretro[.]info[.]claims([.]self_test)?$|^install[.]consumer'` | 0 | All 10 focused selectors discovered |
| `pkg-config --exists sdl2` | 0 | Installed SDL2 version 2.32.70 |
| `cmake --install build/ci --prefix build/ci/stage --component library && cmake -S examples/sdl2 -B build/example-sdl2-legacy -DCMAKE_PREFIX_PATH="$PWD/build/ci/stage" -Dschwaystation_DIR="$PWD/build/ci/stage/lib/cmake/schwaystation" && cmake --build build/example-sdl2-legacy --parallel 2` | 0 | Staged-library example compiled; no run, test or package step |
| `cmake --preset ci -DSCHWAYSTATION_CHD=ON -DSCHWAYSTATION_BUILD_CHD_TOOL=ON && cmake --build --preset ci --target schwaystation-run schwaystation_libretro schwaystation-test-disc-ecm schwaystation-test-disc-ccd` | 0 | CHD-ON configure and four requested targets built |
| `ctest --preset ci --no-tests=error -R '^disc[.]image[.](ecm|ccd)$|^libretro[.]info[.]claims([.]self_test)?$|^install[.]consumer' --output-on-failure` | 0 | 10/10 passed, including the `install.stage` fixture dependency |
| `tail -n +2 tests/compat/features.csv \| LC_ALL=C sort -cu` and `cmake -DFEATURES=tests/compat/features.csv -DMODE=features-preflight -P tests/cmake/compat-ratchet.cmake` | 0 | Feature rows sorted and schema/order accepted |
| `ctest --preset ci --no-tests=error -R '^compat[.]features[.](preflight|ratchet)$' --output-on-failure` | 0 | 2/2 passed |
| `git diff --check` | 0 | No whitespace errors |

No hosted workflow, broad CI suite, timed campaign, or example launch/test/package was run.

## Task Commits

Task 1 changed no files; its cheap budget, selector and SDL2 availability preflights are recorded above. Task 2 was committed as `cbc347c` (`feat(11-15): link ECM and CCD SDL2 providers`).

## Files Created/Modified

- `examples/sdl2/CMakeLists.txt` — links both host providers into the optional executable.
- `tests/compat/features.csv` — records the observed staged example and CHD-ON evidence; hosted fields remain unknown.
- `tests/compat/tests.cmake` — updates the independent expected feature rows so the existing ledger ratchet remains exact.

## Deviations from Plan

**1. Updated the independent expected feature rows.**
- **Found during:** Task 2
- **Issue:** The compatibility ratchet compares `features.csv` against independently embedded expected rows; changing only the plan-listed ledger would make the existing test fail.
- **Fix:** Updated the two expected ECM/CCD rows in place with the same observed local evidence and preserved every existing field.
- **Files modified:** `tests/compat/tests.cmake`
- **Verification:** Feature preflight and ratchet passed 2/2.
- **Committed in:** `cbc347c`

## Issues Encountered

None. The existing installed-library CI path worked locally with the already-installed SDL2 package.

## User Setup Required

None.

## Next Phase Readiness

Plan 11-15 is the final implementation plan. Phase 11 source review and exact-head verification remain; hosted Main, Nightly and Release evidence is still unknown. No user action is needed.

## Phase-level requirement closure

The subsequent aggregate Phase 11 verification satisfied ECM-01 and CCD-01 after reviewing the completed provider, runner/libretro, malformed-input, package and consumer paths against exact-head hosted evidence. This phase-level closure is recorded in `11-VERIFICATION.md`; it does not claim that Plan 11-15's local checks alone completed either requirement.

## Self-Check: PASSED

- The summary exists at the expected phase path.
- Task commit `cbc347c` is an ancestor of HEAD.
- The measured plan base is `06a5c79afeb08d8dc63eae81f97186123201fc1b`.

---
*Phase: 11-ecm-and-clonecd-images*
*Completed: 2026-11-22*
