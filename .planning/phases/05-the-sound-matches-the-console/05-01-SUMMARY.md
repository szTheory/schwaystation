---
phase: 05-the-sound-matches-the-console
plan: "01"
subsystem: testing
tags: [audio, pcm, cmake, ctest]
requires: []
provides:
  - Public PCM frame-to-drain test under the audio label
  - SPU test directory selected in CI, ASan, and wasm32 inventories
affects: [phase-05-audio, portability-lanes]
actuals:
  tokens: 2602
  tasks: 2
  commits: 2
plan_head_before: 4f8aec5e91f78602661d17af46674db8c7087318
plan_head_after: 310327919bc164abae5e0c1d96796283b153f2ef
tech-stack:
  added: []
  patterns:
    - Public API tests verify PCM counts and bounded drain behavior
key-files:
  created:
    - tests/spu/model.c
    - tests/spu/tests.cmake
    - tests/spu/inventory.txt
  modified:
    - tests/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/asan.txt
    - tests/inventory/wasm32.txt
key-decisions: []
requirements-completed:
  - SOUND-01
  - SOUND-02
coverage:
  - id: D1
    description: The audio-labelled public PCM tracer is registered in the exact CI inventory, with the SPU directory also selected by ASan and wasm32 inventories.
    requirement: SOUND-01
    verification:
      - kind: unit
        ref: "cmake --preset ci && cmake --build --preset ci --target schwaystation-test-spu && ctest --test-dir build/ci -R '^spu\\.model$|^core\\.audio\\.' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: NTSC-J, NTSC-U, and PAL keep their AV metadata and carried floor(ticks/768) pair counts, while split runs, ring wrap/drop, reset, and state load preserve the public drain contract.
    requirement: SOUND-02
    verification:
      - kind: unit
        ref: "tests/spu/inventory.txt#spu.model; tests/core/inventory.txt#core.audio.pairs; targeted CTest command (exit 0)"
        status: pass
    human_judgment: false
duration: 14 min
completed: 2026-11-13
status: complete
---

# Phase 5 Plan 1: Build and inventory the public PCM tracer Summary

**A public library-to-drain test now checks regional PCM cadence, ring accounting, and host-queue clearing across the portability inventories.**

## Performance

- **Duration:** 14 min
- **Started:** 2026-11-13T21:25:52Z
- **Completed:** 2026-11-13T21:39:38Z
- **Tasks:** 2
- **Files modified:** 7
- **Runtime/model:** Codex/OpenGSD 1.20.0; executor gpt-6-luna xhigh

## Accomplishments

- Added `spu.model`, which creates a library instance, runs frames, drains `schway_get_audio`, and checks sample-pair counts and interleaved stereo output.
- Covered NTSC-J, NTSC-U, and PAL AV rates, fractional frame cadence, cycle splits at 767/768/769 cycles, zero budget, partial drain, ring wrap/drop counts, reset, and state-load clearing.
- Registered the SPU directory in the CI, ASan, and wasm32 inventories; the BE32 lane selects the test through the existing portable label.

## Verification Evidence

- Task 1 command: `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-spu && ctest --test-dir build/ci -R '^spu\.model$' --no-tests=error --output-on-failure` — exit 0, 1/1 test passed.
- Task 2 command: `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-spu && ctest --test-dir build/ci -R '^spu\.model$|^core\.audio\.' --no-tests=error --output-on-failure` — exit 0, 3/3 tests passed.
- Hosted URL: not applicable; this plan's required checks are local CMake/CTest checks.

## Task Commits

1. **Task 1: Trace a bounded frame into the public PCM drain** — `5a25cf1` (`feat`).
2. **Task 2: Cover regional cadence and select the same tests in portability lanes** — `3103279` (`test`).

## Files Created/Modified

- `tests/spu/model.c` — public frame-to-drain cadence, ring, reset, and state-load checks.
- `tests/spu/tests.cmake`, `tests/spu/inventory.txt` — SPU test executable, audio label, and exact registration.
- `tests/tests.cmake`, `tests/inventory/ci.txt`, `tests/inventory/asan.txt`, `tests/inventory/wasm32.txt` — central inclusion and portability-lane selection.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Initialized the test instance allocator**
- **Found during:** Task 1
- **Issue:** The first focused run showed `schway_create` rejects a zeroed allocator configuration.
- **Fix:** Added test-local `malloc`/`free` callbacks to the test configuration.
- **Files modified:** `tests/spu/model.c`
- **Verification:** The Task 1 CMake/CTest command passed after the fix.
- **Committed in:** `5a25cf1`

**Total deviations:** 1 auto-fixed (Rule 1 - Bug). **Impact:** Test setup only; no product behavior or plan scope changed.

## Issues Encountered

The initial Task 1 run failed on test-instance setup and was corrected before the task commit. All required verification passed afterward.

## User Setup Required

None — no external service configuration is required.

## Next Phase Readiness

The test path and inventory contract are ready for the remaining Phase 5 SPU implementation plans. No user action is needed.

## Self-Check: PASSED

All seven planned files and this summary exist, and both task commit hashes are ancestors of the production HEAD. Stub-pattern matches are only intentional null checks in the test setup.
