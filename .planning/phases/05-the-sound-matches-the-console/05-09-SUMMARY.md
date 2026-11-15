---
phase: 05-the-sound-matches-the-console
plan: "09"
subsystem: audio
tags: [spu, reverb, fir, pcm, ctest]
requires:
  - phase: 05-02
    provides: SPU instance state, serializable reverb history, and scheduled mixer
provides:
  - Centered 39-tap integer reverb FIR with a verified combined 38-sample delay
  - EON-selected wet mixing, alternating channel phase, EVOL scaling, and a zero-wet Off path
affects: [05-10, SOUND-01]
actuals:
  tokens: 7120
  tasks: 2
  commits: 4
commits: 4
plan_head_before: 3c27b7156c1778b70b8e9035d6801802173ffff3
plan_head_after: 6b0e7a6096c4054c157c13ae1212a3b7f8f8ee2c
tech-stack:
  added: []
  patterns:
    - Integer FIR history with explicit signed shifts, saturation, and serialized cursors
    - Public PCM tests for wet routing and split-run timing
key-files:
  created:
    - src/spu_reverb.c
    - tests/spu/reverb.c
  modified:
    - CMakeLists.txt
    - docs/hardware/spu.md
    - src/spu.c
    - src/spu.h
    - src/spu_tables.inc
    - tests/spu/inventory.txt
    - tests/spu/tests.cmake
key-decisions:
  - Keep the 22.05 kHz feedback stage as a unity seam so this plan isolates the measured FIR delay and phase; plan 05-10 supplies the register-driven feedback network.
requirements-completed: []
coverage:
  - id: D1
    description: The centered 39-tap reverb FIRs produce the verified 38-sample delay and preserve channel phase across split runs.
    requirement: SOUND-01
    verification:
      - kind: unit
        ref: tests/spu/reverb.c#spu.reverb; focused CMake and CTest command in Commands and Evidence
        status: pass
    human_judgment: false
metrics:
  duration: "at least 22 min"
  completed: 2026-11-14
status: complete
---

# Phase 5 Plan 09: Reverb FIR Timing and Channel Alternation Summary

**The 39-tap integer FIR now carries a verified 38-sample delay through alternating EON-selected reverb PCM, signed EVOL scaling, and a stable Off path.**

## Performance

- **Duration:** At least 22 minutes, measured from the first task commit; plan work began earlier.
- **Started:** 2026-11-14T06:07:36Z (first recorded task commit; execution started earlier)
- **Completed:** 2026-11-14
- **Tasks:** 2
- **Files modified:** 9

## Accomplishments

- Added independent scalar and production checks for the symmetric 39-tap FIR, polyphase alignment, and combined 38-sample group delay.
- Routed EON-selected voice sums through signed input volumes, alternating left/right processing, EVOL return scaling, and the public PCM mixer.
- Verified asymmetric wet returns, signed EVOL, a zero-wet Off configuration, and identical PCM across split public run calls.

## Task Commits

1. **Task 1 RED:** `b50cf1d` — added failing FIR and phase checks.
2. **Task 1 GREEN:** `e5a96f9` — implemented centered FIR resampling.
3. **Task 2 RED:** `f15d542` — added failing public PCM wet-return checks.
4. **Task 2 GREEN:** `6b0e7a6` — connected the alternating wet return to the mixer.

## Files Created/Modified

- `src/spu_reverb.c` — bounded FIR, signed scaling, channel phase, and the 22.05 kHz seam.
- `tests/spu/reverb.c` — scalar FIR comparison and scheduled public PCM coverage.
- `src/spu.c`, `src/spu.h`, `src/spu_tables.inc` — EON routing, private reverb interface, and coefficient facts.
- `CMakeLists.txt`, `tests/spu/tests.cmake`, `tests/spu/inventory.txt` — registered the focused portable audio test.
- `docs/hardware/spu.md` — records the cited timing constraint and arithmetic limitations.

## Decisions Made

- Preserve a unity 22.05 kHz feedback seam in this plan; the register-driven feedback memory network belongs to plan 05-10.
- Treat the 38-sample FIR delay and channel alternation as measured constraints without claiming a captured silicon preset waveform.

## Deviations from Plan

None — the plan was executed as written.

## Issues Encountered

- The first polyphase test cursor did not model the alternating right-channel phase. Correcting the isolated test alignment made the RED case represent the documented hardware schedule; the focused test passed afterward.
- The test inventory required bytewise ordering for its new entry; it was sorted before the test was committed.

## Known Stubs

- `src/spu_reverb.c:79` — the 22.05 kHz feedback stage currently forwards its input as a unity stage so FIR timing can be tested in isolation. Plan 05-10 replaces it with the register-driven feedback network.

## Commands and Evidence

- Task 1 expected RED: cmake --preset ci && cmake --build --preset ci --target schwaystation-test-spu-reverb && ctest --test-dir build/ci -R '^spu\.reverb$' --no-tests=error --output-on-failure — exit status 8; the new FIR assertions were red before implementation.
- Task 1 post-commit tracer recheck: cmake --preset ci && cmake --build --preset ci --target schwaystation-test-spu-reverb && ctest --test-dir build/ci -R '^spu\.reverb$' --no-tests=error --output-on-failure — exit status 0; 1/1 spu.reverb passed.
- Task 2 expected RED: cmake --preset ci && cmake --build --preset ci --target schwaystation-test-spu-reverb && ctest --test-dir build/ci -R '^spu\.reverb$' --no-tests=error --output-on-failure — exit status 8; the public wet-return assertions were red before mixer integration.
- Task 2 focused GREEN: cmake --preset ci && cmake --build --preset ci --target schwaystation-test-spu-reverb && ctest --test-dir build/ci -R '^spu\.reverb$' --no-tests=error --output-on-failure — exit status 0; 1/1 spu.reverb passed, including public PCM and split-run checks.
- Hosted run URL: not applicable; this plan has no hosted verification lane.

## Next Phase Readiness

The FIR path and mixer seam are ready for plan 05-10 to implement register-driven feedback. SOUND-01 and SOUND-02 remain pending until final Phase 5 verification.

## Self-Check: PASSED

The summary file exists, and all four task commits are ancestors of the current branch head.

---
*Phase: 05-the-sound-matches-the-console*
*Completed: 2026-11-14*
