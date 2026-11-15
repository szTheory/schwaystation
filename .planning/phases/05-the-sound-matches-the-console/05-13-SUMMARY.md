---
phase: 05-the-sound-matches-the-console
plan: "13"
subsystem: testing
tags: [spu, fuzz, scheduler, cachegrind, cmake, ci]
requires:
  - phase: 05-08
    provides: SPU register, FIFO, RAM, DMA, scheduler, and audio-state interfaces
provides:
  - Bounded deterministic SPU hostile-stream replay through the real bus and DMA paths
  - Idle SPU scheduler progress checks and a Cachegrind comparison entry point
affects: [phase-05-verification, hosted-perf-ci]
actuals:
  tokens: 9382
  tasks: 2
  commits: 2
  plan_head_before: 19c5c4fe839988740c1d06237291b425432e0a3f
  plan_head_after: 1952b1a020fb3b19271a1bc2f7c08db3be12acb1
tech-stack:
  added: []
  patterns:
    - Bounded hostile-input replay with independent-instance and save/load PCM comparisons
    - Cachegrind producer and comparator controls in the existing gating perf script
key-files:
  created:
    - tests/fuzz/spu.c
  modified:
    - tests/fuzz/seeds.c
    - tests/fuzz/tests.cmake
    - tests/fuzz/inventory.txt
    - CMakePresets.json
    - tests/core/test_run.c
    - tests/core/tests.cmake
    - tests/core/inventory.txt
    - tests/perf/tests.cmake
    - tests/perf/inventory.txt
    - .github/workflows/ci.yml
key-decisions:
  - "Kept production SPU code unchanged because the existing scheduled-sample implementation already satisfied the new deterministic and bounded assertions."
  - "Connected the hosted perf lane to CASE=spu_idle and added the fuzzer target to the explicit fuzz preset build list."
patterns-established:
  - "SPU replay exercises public bus/DMA entry points and checks post-load future PCM on independent instances."
  - "Perf threshold, parser, and producer-failure directions can be tested without requiring Valgrind locally."
requirements-completed: []
coverage:
  - id: D1
    description: "Deterministic bounded SPU seed replay covers FIFO, IRQA, voice, noise, reverb, DMA, and valid state-load continuation."
    requirement: SOUND-01
    verification:
      - kind: unit
        ref: "cmake --preset ci && cmake --build --preset ci --target schwaystation-fuzz-spu-replay && ctest --test-dir build/ci -R '^fuzz\\.regress\\.spu$' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: "Idle, muted, DMA-busy, and maximum legal SPU states make exact scheduled progress within public run bounds."
    requirement: SOUND-02
    verification:
      - kind: unit
        ref: "cmake --preset ci && cmake --build --preset ci --target schwaystation-test-run && ctest --test-dir build/ci -R '^core\\.no_busy_loop$|^perf\\.gate\\.' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
  - id: D3
    description: "The hosted perf job invokes the tested CASE=spu_idle producer and comparator through its existing Cachegrind lane."
    requirement: SOUND-02
    verification:
      - kind: integration
        ref: ".github/workflows/ci.yml perf Cachegrind step and tests/perf/tests.cmake CASE=spu_idle; local perf.gate controls passed"
        status: pass
    human_judgment: false
duration: "13 min minimum; start timestamp was recorded after implementation began"
completed: 2026-11-14
status: complete
commits: 2
---

# Phase 5 Plan 13: Replay hostile SPU inputs and bound idle work

**Bounded hostile SPU replay and idle-scheduler checks now run through the real bus paths, with the paired Cachegrind gate wired into hosted CI.**

## Performance

- **Duration:** At least 13 minutes; the recorded start timestamp followed implementation start.
- **Started:** 2026-11-14T06:50:13Z
- **Completed:** 2026-11-14T07:03:29Z
- **Tasks:** 2
- **Files modified or created:** 11
- **Runtime/model:** Codex/OpenGSD 1.21.0; gpt-6-luna xhigh

## Accomplishments

- Added bounded fuzz and seed replay through real SPU bus, FIFO, and DMA paths, checking ranges, deterministic independent instances, and future PCM after valid state load.
- Added `core.no_busy_loop` coverage for exact sample deadlines and bounded public runs across idle, muted, DMA-busy, and maximum legal loaded states.
- Added a self-contained paired Cachegrind producer/comparator with failure-direction CTests and connected it to the hosted perf job.

## Task Commits

1. **Task 1: Trace hostile transfer bytes through the production SPU and seed replay** — `1753b12` (test)
2. **Task 2: Prove idle scheduler progress and existing host-call bounds** — `1952b1a` (test)

## Commands and Evidence

- Task 1 focused command exited 0; `fuzz.regress.spu` passed (1/1). The same command was rerun after the task commit and exited 0.

  ```sh
  cmake --preset ci && cmake --build --preset ci --target schwaystation-fuzz-spu-replay && ctest --test-dir build/ci -R '^fuzz\.regress\.spu$' --no-tests=error --output-on-failure
  ```

- Task 2 focused command exited 0; `core.no_busy_loop` and all 15 `perf.gate.*` tests passed (16/16).

  ```sh
  cmake --preset ci && cmake --build --preset ci --target schwaystation-test-run && ctest --test-dir build/ci -R '^core\.no_busy_loop$|^perf\.gate\.' --no-tests=error --output-on-failure
  ```

- `git diff --check 19c5c4fe839988740c1d06237291b425432e0a3f..HEAD` exited 0.
- `command -v valgrind` exited 1 because Valgrind is not installed. The local Cachegrind measurement was therefore unavailable; the parser, threshold, malformed/missing report, producer failure, and mutated-count controls all passed in the focused CTest run.
- The configured `clang-21` path was stale. Retrying with the installed Apple Clang built the fuzzer object and core library, but the fuzzer executable could not link because `libclang_rt.fuzzer_osx.a` is unavailable. No package was installed. The portable seed replay remained green.
- No hosted workflow was launched for this local plan commit, so there is no hosted run URL. The workflow hook is wired for the next hosted perf run.

## TDD Evidence

No behavioral RED occurred: the existing production scheduler already met the new scheduled-sample and bounded-run assertions. The initial prototype/build issue was corrected and was not treated as a behavior failure. No failing behavior was fabricated to satisfy the planned sequence.

## Deviations from Plan

### Auto-added missing integration

**1. [Rule 2 - Missing required functionality] Added the SPU fuzzer to the explicit fuzz preset target list.**
- **Found during:** Task 1
- **Issue:** The existing fuzz preset enumerates its build targets explicitly, so the new fuzzer would not be built by that sanitizer/fuzz convention without updating the preset.
- **Fix:** Added the SPU replay and libFuzzer targets to the existing preset target list.
- **Files modified:** `CMakePresets.json`
- **Verification:** The focused portable replay build and CTest passed; the local fuzzer link was blocked by the missing Apple Clang runtime.
- **Committed in:** `1753b12`

**2. [Rule 2 - Missing required functionality] Connected the existing hosted perf job to the new Cachegrind case.**
- **Found during:** Task 2
- **Issue:** The plan requires the hosted perf job to reuse the `spu_idle` entry point, but the existing workflow directly invoked Cachegrind for other workloads and did not call this case.
- **Fix:** Added the `CASE=spu_idle` invocation to the existing hosted perf step and reused its build output and Valgrind path.
- **Files modified:** `.github/workflows/ci.yml`
- **Verification:** All local `perf.gate.*` controls passed (15/15); the hosted job was not run for this local commit.
- **Committed in:** `1952b1a`

**Total deviations:** 2 auto-added integrations (Rule 2).

## Requirements and Phase Boundary

`SOUND-01` and `SOUND-02` remain pending in REQUIREMENTS.md until final Phase 5 verification. This plan does not ship Phase 5 or start Phase 6.

## Self-Check: PASSED

- Summary file exists and `gsd_run verify-summary` confirmed the required files and both task commits.
- Commits `1753b12` and `1952b1a` are ancestors of the current branch head.
