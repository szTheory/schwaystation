---
phase: 04-games-play-in-retroarch
plan: "12"
subsystem: gpu-timing
tags: [GPU, DMA2, beam, timers, fixtures, cross-platform-hashes]
requires:
  - phase: 04-games-play-in-retroarch (plans 01-11)
    provides: Fixture assembly and runner protocol, native scan-out, beam/timer scheduling, and DMA2 GPU access.
provides:
  - Moving GP0/DMA2 fixture with frame hashes at 60, 120 and 180 and resume-state hashes at 150 and 180.
  - Profile-specific dot-divider timing fixtures for NTSC-U, NTSC-J and PAL with exact dot and HBlank totals.
  - Cross-lane hash rows for the GPU-motion frames and continuation states.
affects: [04-13, gpu, beam-timing, GAME-01, GAME-02]
actuals:
  tokens: 9859.75
  tasks: 2
  commits: 4
commits: 4
plan_head_before: 97d32a2cb5db015573de4f750cb0ddaea5a848aa
plan_head_after: 331ee30022f91213458c07011d6e5fed1170c65b
tech-stack:
  added: []
  patterns:
    - Fixture variants use integer assembler parameters and share the existing mailbox protocol.
    - Timing fixtures use phase-restoring frame windows and independently derived exact counter expectations.
key-files:
  created:
    - tests/fixtures/src/gpu_motion.asm
  modified:
    - tests/fixtures/manifest.txt
    - tests/fixtures/tests.cmake
    - tests/fixtures/inventory.txt
    - tests/diag/expected.csv
    - tests/fixtures/src/timers_lines.asm
    - tests/cmake/hashes.cmake
    - tests/hygiene/tests.cmake
    - docs/testing.md
    - .planning/todos/pending/039-cpu-bound-ir-fixture.md
key-decisions:
  - Keep the dot oracle derived from documented beam rates and line lengths; correct the sample endpoint from traced CPU timing, not by changing the expected count.
  - Use the same GPU-motion hashes in the portable lane artifacts, including be32.
patterns-established:
  - Each parameterized timer fixture reports VBlank count, dots, HBlanks per frame and total HBlanks in the bounded mailbox.
  - GPU-motion hash artifacts include both visible frame output and saved state needed for continuation.
requirements-completed: []
coverage:
  - id: D1
    description: The GP0/DMA2 moving-pixel fixture has distinct frame pins and deterministic continuation from frame 150 to 180.
    requirement: GAME-01
    verification:
      - kind: integration
        ref: "fixtures.gpu_motion; fixtures.gpu_motion.hash.{60,120,180}; fixtures.gpu_motion.state.{150,180}; fixtures.gpu_motion.hash.self_test; cmake --workflow --preset ci"
        status: pass
    human_judgment: false
  - id: D2
    description: NTSC-U, NTSC-J and PAL report the documented line counts and exact dot and HBlank totals for dividers 10, 8, 7, 5 and 4.
    requirement: GAME-02
    verification:
      - kind: integration
        ref: "fixtures.timers_lines.{ntsc_u,ntsc_j,pal}.div{10,8,7,5,4}; fixtures.timers_lines.ntsc_u.div10.self_test; fixtures.timers_lines.mailbox_duplicate.self_test; ctest --test-dir build/ci -L timers --output-on-failure"
        status: pass
    human_judgment: false
  - id: D3
    description: Portable hash artifacts include GPU-motion frame and continuation-state rows for the existing cross-lane comparison.
    requirement: GAME-01
    verification:
      - kind: other
        ref: "cmake -DMODE=write -DRUN=build/ci/schwaystation-run -DFIXTURES=build/ci/fixtures -DOUT=build/ci -P tests/cmake/hashes.cmake; runner.hashes_file"
        status: pass
    human_judgment: false
metrics:
  duration: "at least 69min"
  completed: 2026-11-11
status: complete
---

# Phase 4 Plan 12: Moving GPU and Beam Timing Summary

**GPU motion now has visible frame and continuation-state pins, while timer fixtures assert exact per-profile dot and HBlank totals.**

## Performance

- **Duration:** At least 69 minutes from the first task commit through the final plan-local commit; execution started earlier.
- **Completed:** 2026-11-11
- **Tasks:** 2
- **Files modified:** 10
- **Runtime/model:** Codex, OpenGSD 1.20.0; `gpt-6-luna` at `xhigh` effort.

## Accomplishments

- Added the moving GP0/DMA2 fixture, independently checked rendered pixels and metadata, pinned distinct frame hashes at 60/120/180, and verified state continuation from frame 150 to 180.
- Added a parameterized timer fixture across NTSC-U, NTSC-J and PAL for dot dividers 10/8/7/5/4. Each run records VBlank count, total dots, HBlanks per frame and total HBlanks without increasing the 256-byte mailbox or its 30-record limit.
- Added the GPU-motion frame and saved-state rows to the portable hash writer, including the existing be32 lane, and retained the 1513-line Phase 3 gate history under the approved 1850-line Phase 4 ceiling.
- Bound todo039's CPU-bound instructions-per-frame measurement to plan 04-13, which owns its control measurement and separate ceiling.

## Task Commits

1. **Task 1 RED: GPU-motion behavior assertions** - `f9d468b` (test)
2. **Task 1 GREEN: GPU motion through DMA and state resume** - `335576b` (feat)
3. **Task 2: Exact timer fixture and lane hashes** - `39ff744` (feat)
4. **Plan-required todo binding to plan 04-13** - `331ee30` (docs)

The first two commits are the tracer task's RED/GREEN pair. The separate plan metadata commit is recorded after the bookkeeping step.

## Files Created/Modified

- `tests/fixtures/src/gpu_motion.asm` - MIPS fixture that moves visible pixels through DMA2 each frame.
- `tests/fixtures/src/timers_lines.asm`, `tests/diag/expected.csv`, `tests/fixtures/tests.cmake`, `tests/fixtures/inventory.txt` - Profile/divider matrix and mailbox records with strict missing/count/duplicate checks.
- `tests/cmake/hashes.cmake` - Portable GPU-motion frame and state hash rows.
- `tests/fixtures/manifest.txt` - First-party fixture source pin.
- `tests/hygiene/tests.cmake`, `docs/testing.md` - Approved gate ceiling and test/evidence documentation.
- `.planning/todos/pending/039-cpu-bound-ir-fixture.md` - Explicit assignment to plan 04-13.

## Decisions Made

- The expected dot totals remain derived from the published GPU/CPU ratio, line lengths and dot-divider phase. The observed one-dot-short result was corrected by stabilizing the CPU sample endpoint.
- Cross-lane outputs carry both moving frame hashes and GPU-motion saved-state hashes, so comparisons exercise rendered pixels and continuation state.

## TDD Evidence

- Task 1 landed RED assertions in `f9d468b`, then GREEN fixture behavior and pins in `335576b`; the tracer CI gate passed before expansion.
- Task 2 RED: `cmake --build build/ci --target schwaystation-fixtures -j2 && ctest --test-dir build/ci -R 'fixtures\\.timers_lines\\.ntsc_u\\.div10' --output-on-failure` exited 8 against the old legacy fixture, which did not provide the new dot/line/HBlank records.
- Task 2 GREEN: after adding the parameterized timing path, `ctest --test-dir build/ci -L timers --output-on-failure` exited 0 with all 45 tests passing, including the 15 profile/divider cases and the one-count and duplicate-record controls.

## Evidence

- `cmake --workflow --preset ci` — exit 0; all 439 CI tests and both release archive checks passed.
- `cmake --workflow --preset hygiene` — exit 0; all 23 hygiene checks passed, including the 1850-line gate budget.
- `cmake -DMODE=write -DRUN=build/ci/schwaystation-run -DFIXTURES=build/ci/fixtures -DOUT=build/ci -P tests/cmake/hashes.cmake` — exit 0; generated rows match the GPU-motion pins.
- PAL /4 timer trace — exit 0. VBlank events were at CPU ticks 555067 and 1235890; Timer 0 bus reads were at 555081 and 1235904, exactly 14 cycles later. The beam-ratio carries at the edges were 75185 and 203030 modulo 451584. An 8-cycle sample would span 12 versus 13 GPU clocks, while the final fixed 14-cycle endpoint spans 22 at both edges. This attributes the former one-dot deficit to CPU interrupt/instruction endpoint latency; the expected dot count remains unchanged.
- NTSC /10 trace — exit 0. VBlank events at 551134 and 5080764 were followed by Timer 0 reads at 551148 and 5080778, confirming the same 14-cycle read endpoint in an NTSC profile.
- The hash writer was exercised locally on macOS arm64. The existing hosted eight-lane hash-equality job, including be32, will compare lane artifacts when CI runs; no hosted run URL exists yet because this plan execution did not push a branch.

## Deviations from Plan

None. The additional todo039 edit records an action explicitly required by Task 1 and assigns the measurement to plan 04-13.

## Issues Encountered

The initial PAL /4 counter sample was one dot below the documented-model total. Trace evidence isolated the difference to endpoint latency at the CPU instruction/IRQ boundary. A fixed I/O-read preamble made both samples land 14 CPU cycles after VBlank and yielded the derived 22-GPU-clock offset at each endpoint; the oracle and phase-restoring window were not weakened or selected to fit the result.

## User Setup Required

None. Local CI and hygiene checks require no owner interaction. Hosted multi-lane equality remains for the existing CI job after a later authorized push.

## Next Phase Readiness

Plan 04-13 can use `gpu_motion.exe` for the CPU/GPU instructions-per-frame control and regression ceiling. It owns the measurement and closure of todo039.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-11*

## Self-Check: PASSED

The summary and all listed implementation files exist, and task commits `f9d468b`, `335576b`, `39ff744` and `331ee30` are ancestors of the current HEAD.
