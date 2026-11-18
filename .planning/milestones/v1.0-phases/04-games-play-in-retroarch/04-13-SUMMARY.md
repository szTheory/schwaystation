---
phase: 04-games-play-in-retroarch
plan: "13"
subsystem: testing
tags: [cmake, cachegrind, github-actions, retroarch]
requires:
  - phase: 04-12
    provides: CPU-running gpu_motion fixture with moving GPU output
provides:
  - Same-fixture control/head Cachegrind measurement and a gated GPU-motion instruction ceiling
  - Three-minute nofp timeout floor with cancellation evidence
  - Hosted timing and outcome evidence for the required CI lanes
affects: [phase-04-verification, ci-performance]
actuals:
  tokens: 5672.5
  tasks: 2
  commits: 5
plan_head_before: 0cd592ef0968d79c9cb8fd228eea1658b2bbe5ae
plan_head_after: 1b24bf15ef35ec335c71791e99622516a0b6c451
tech-stack:
  added: []
  patterns:
    - Measure performance fixtures against the same fixture at an explicit control commit.
    - Keep a negative synthetic slowdown test for every armed performance threshold.
key-files:
  created:
    - .planning/phases/04-games-play-in-retroarch/04-13-SUMMARY.md
  modified:
    - .github/workflows/ci.yml
    - tests/cmake/perf_compare.cmake
    - tests/perf/tests.cmake
    - tests/perf/inventory.txt
    - docs/testing.md
    - src/pad.c
    - src/pad.h
    - src/sio.c
    - host/xrgb8888.c
    - tests/core/tests.cmake
    - tests/fixtures/tests.cmake
    - tests/inventory/asan.txt
    - tests/inventory/wasm32.txt
    - tests/sio/pad_tests.c
    - .planning/todos/completed/034-nofp-job-timeout-floor.md
    - .planning/todos/completed/039-cpu-bound-ir-fixture.md
key-decisions:
  - "Use the same gpu_motion fixture for control and head measurements; do not compare its CPU work to the test-card baseline."
  - "Measure gpu_motion over frames 10 through 60 to keep the hosted comparison within the job budget; retain the test-card 60 through 360 window."
  - "Keep all required CI lanes and set the perf job to a provisional five-minute floor from the latest successful hosted durations."
patterns-established:
  - "A performance ratchet measures the same workload on an explicit control and head commit."
  - "A synthetic slower row proves that the performance threshold rejects regressions."
requirements-completed: []
requirements-addressed: [GAME-01, GAME-02]
coverage:
  - id: D1
    description: "The CPU/GPU gpu_motion workload has a measured control, an armed instruction ceiling, and a slower-row negative check while the test-card ceiling remains."
    requirement: GAME-01
    verification:
      - kind: integration
        ref: "Hosted control/head comparison in Actions run 42985306022; ctest perf.gate.gpu_motion_slow"
        status: pass
    human_judgment: false
  - id: D2
    description: "The moving GPU fixture remains covered by required cross-platform frame checks, and hosted CI retains its required lanes."
    requirement: GAME-02
    verification:
      - kind: integration
        ref: "Hosted CI and hash-equality in Actions run 42985306022"
        status: pass
    human_judgment: false
metrics:
  duration: 1h 11m
  completed: 2026-11-11
  status: complete
---

# Phase 04 Plan 13: CPU-driven GPU performance gate Summary

**Measured the CPU/GPU `gpu_motion` fixture against the exact same control workload, armed its regression ceiling, and removed a reproduced nofp timeout failure.**

## Performance

- **Duration:** 1h 11m
- **Started:** 2026-11-11T07:17:28Z
- **Completed:** 2026-11-11T08:28:33Z
- **Tasks:** 2
- **Files modified:** 16

## Accomplishments

- Added an exact-commit control/head comparison for `gpu_motion`, retaining the existing test-card ceiling and adding a synthetic +1.01% slower-row check.
- Set the nofp job timeout to three minutes and preserved all required CI lanes.
- Corrected hosted GCC/MSVC build failures, the ASAN/WASI test inventories, a BE32/WASI nested-emulator test command, and a macOS state-sweep timeout.
- Measured the hosted critical path and recorded the performance window and timeout rationale without committing run logs.

## Evidence

- `cmake --workflow --preset ci` — exit 0; 442 tests and the release-package checks passed at commit `3bb84fa`.
- `ctest --test-dir build/ci -R '^perf.gate.gpu_motion_slow$' --no-tests=error --output-on-failure` — exit 0; the deliberately slower row was rejected by the gate.
- `cmake --workflow --preset hygiene` — exit 0; 23 checks passed at commit `3bb84fa`.
- `ctest --test-dir build/hygiene -R '^hygiene\.timeout_comments(\.self_test)?$' --no-tests=error --output-on-failure` — exit 0; both timeout-comment checks passed after the final five-minute edit.
- Manual control run [42984345515](https://github.com/szTheory/schwaystation/actions/runs/42984345515) measured `gpu_motion` at 137,678,369 instructions per frame over three repeats with zero spread. The test-card control was 1,620,845 over three repeats with zero spread.
- Full hosted control/head run [42985306022](https://github.com/szTheory/schwaystation/actions/runs/42985306022) passed `CI required` on tested commit `3bb84fa`, comparing control `045b76a` with head `3bb84fa`. Both rows were 0 basis points slower: test-card 1,620,845 vs. 1,620,845 and `gpu_motion` 137,678,369 vs. 137,678,369.
- In that hosted run, `perf` took 1m39s, `be32` was the slowest required job at 4m20s, and the whole workflow took 4m48s. The final task commit `1b24bf1` changes documentation, todo status, and the perf timeout from six to five minutes; no full hosted matrix was repeated for those closeout edits. The focused timeout-comment hygiene check passed, and the next current-head phase verification will exercise the final timeout value.
- nofp passed both Linux legs in runs [42980564011](https://github.com/szTheory/schwaystation/actions/runs/42980564011) and [42985306022](https://github.com/szTheory/schwaystation/actions/runs/42985306022). Earlier one-minute cancellations were reproduced in [run 42840838785](https://github.com/szTheory/schwaystation/actions/runs/42840838785) and [run 42841474140](https://github.com/szTheory/schwaystation/actions/runs/42841474140); the release commit passed in 9 seconds on main and an isolated rerun passed in 10 seconds.

## Task Commits

1. **Task 1: Trace gpu_motion through measured control/head performance comparison** — `c87f40b`, `00880c1`, `045b76a`.
2. **Task 2: Confirm lane cost and preserve all required feedback** — `3bb84fa`, `1b24bf1`.

## Files Created/Modified

- `.github/workflows/ci.yml`, `docs/testing.md`, and the completed TODO034/TODO039 records document the timeout remedy, explicit control measurement, and hosted costs.
- `tests/cmake/perf_compare.cmake`, `tests/perf/tests.cmake`, and `tests/perf/inventory.txt` add the CPU/GPU measurement and negative control.
- `src/pad.c`, `src/pad.h`, `src/sio.c`, and `host/xrgb8888.c` fix compiler portability diagnostics while preserving behavior and serialized pad bytes.
- `tests/core/tests.cmake`, `tests/fixtures/tests.cmake`, `tests/sio/pad_tests.c`, `tests/inventory/asan.txt`, and `tests/inventory/wasm32.txt` restore the affected hosted checks.

## Decisions Made

- `gpu_motion` uses the same exact fixture on both sides of the comparison. Its window is frames 10 through 60 because the original 300-frame window exceeded the hosted perf-job budget; the retained 50-frame window produced stable repeated control measurements.
- The performance threshold rejects instruction counts more than 1.00% above control. The test-card threshold and all required lanes remain enabled.
- The perf job timeout is five minutes, marked provisional, based on successful 1m15s and 1m39s perf-job runs. The hosted required-lane critical path remains `be32` at 4m20s.
- No job-level duplicate could be removed safely: the lanes produce independent test results or artifacts. A deeper runner/checkpoint duplicate-work review is deferred to the next safe CI edit.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed GCC and MSVC warnings that failed hosted builds**
- **Found during:** Task 1
- **Issue:** GCC diagnosed pad-state writes and conversions; MSVC diagnosed a narrowed controller mask and an `int`/`uint32_t` comparison.
- **Fix:** Flattened private pad arrays while retaining serialized byte order, added explicit casts, and made the video-depth comparison unsigned.
- **Files modified:** `src/pad.c`, `src/pad.h`, `src/sio.c`, `host/xrgb8888.c`, `tests/sio/pad_tests.c`
- **Verification:** Full hosted required lanes passed in run 42985306022.
- **Committed in:** `c87f40b`, `00880c1`

**2. [Rule 3 - Blocking] Restored exact ASAN/WASI inventories and nested cross-emulator test invocation**
- **Found during:** Task 1
- **Issue:** Newly registered card/SIO tests were absent from the ASAN/WASI inventories, and the timer negative test passed the emulator list with an invalid separator.
- **Fix:** Added the sorted inventory rows and used the existing nested-test emulator convention.
- **Files modified:** `tests/inventory/asan.txt`, `tests/inventory/wasm32.txt`, `tests/fixtures/tests.cmake`
- **Verification:** ASAN, WASI, and BE32 lanes passed in run 42985306022.
- **Committed in:** `c87f40b`, `00880c1`

**3. [Rule 3 - Blocking] Increased only the measured macOS state-sweep timeout**
- **Found during:** Task 2
- **Issue:** `core.state.record_sweep` exceeded its 10-second limit once at 10.06 seconds on macOS x64 after a 9.81-second pass.
- **Fix:** Raised that case to 20 seconds; local execution took 3.68 seconds and the final hosted run passed it.
- **Files modified:** `tests/core/tests.cmake`
- **Verification:** Local focused test and run 42985306022.
- **Committed in:** `3bb84fa`

**4. [Rule 1 - Performance] Shortened the GPU-motion measurement window after the hosted limit**
- **Found during:** Task 1
- **Issue:** The initial 360-to-60 frame window caused the control/head perf job to exceed six minutes.
- **Fix:** Measure frames 10 through 60 for `gpu_motion`; the fixture still runs the CPU and GPU, and three control repeats had zero spread.
- **Files modified:** `.github/workflows/ci.yml`, `tests/perf/tests.cmake`, `docs/testing.md`
- **Verification:** Control run 42984345515 and exact control/head comparison 42985306022.
- **Committed in:** `045b76a`

**Total deviations:** 4 auto-fixed. All hosted failures attributable to this plan were corrected; no released API or emulated behavior changed.

## Issues Encountered

- Run 42980564011 exposed compiler/inventory failures. [Run 42982923968](https://github.com/szTheory/schwaystation/actions/runs/42982923968) exposed the original GPU-motion window exceeding the perf timeout. [Run 42984345515](https://github.com/szTheory/schwaystation/actions/runs/42984345515) exposed the macOS state sweep barely exceeding its old limit. Each was fixed and verified in later local or hosted checks.
- The final documentation and timeout-floor commit was not dispatched to a new full hosted matrix to avoid repeating the unchanged suite; its focused timeout-comment check passed, and current-head phase verification remains the next hosted check.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

The CPU/GPU performance control is measured and gated, and all required hosted lanes passed at `3bb84fa`. `GAME-01` and `GAME-02` remain pending because this plan adds performance coverage rather than completing the full renderer or display-mode requirements. The current-head phase verification should cover the final five-minute timeout value.

## Self-Check: PASSED

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-11*
