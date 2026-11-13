---
phase: 04-games-play-in-retroarch
plan: "55"
subsystem: testing
tags: [libretro, ctest, movie-canaries, pixel-coverage]
requires:
  - phase: 04-games-play-in-retroarch
    provides:
      - Existing packed movie comparator and independent reference frames
      - Compiler-safe libretro host from plan 04-54
provides:
  - Movie-only active-pixel poisoning bounded by validated reference dimensions
  - Measured buffer-write work and maintainer documentation
affects: [04-29, phase-04-hosted-verification]
actuals:
  tokens: 509
  tasks: 2
  commits: 2
commits: 2
plan_head_before: c9fd339b7406a0a0d3dd802f4fd4cd7311979b59
plan_head_after: 041790b3563d7bbdee232e4f284effe2a5be9758
tech-stack:
  added: []
  patterns:
    - Bounded poisoning uses dimensions captured independently from the module callback
key-files:
  created: []
  modified:
    - tests/libretro/host.c
    - docs/testing.md
key-decisions:
  - Use the independent reference extent to poison all expected active pixels and retain full-fill fallback for invalid extents.
  - Treat the byte reduction as measured work only; runtime improvement and timeout causality require actual hosted timing.
patterns-established:
  - Movie-only helpers preserve the shared full-buffer behavior used by other host cases.
requirements-completed: []
coverage:
  - id: D1
    description: Movie frames retain expected-dimension checks and poison coverage for every active pixel, with safe fallback for invalid extents.
    requirement: GAME-03
    verification:
      - kind: unit
        ref: ctest --test-dir build/ci --parallel 1 -R '^libretro[.]input[.]movies$' --no-tests=error --output-on-failure
        status: pass
      - kind: other
        ref: One-shot omitted-pixel mutation failed at frame 1 pixel 0,0 with the retained POISON value (expected CTest exit 8).
        status: pass
      - kind: other
        ref: One-shot zero and 641x577 extent probes checked every frame word remained poisoned before p_run.
        status: pass
    human_judgment: false
  - id: D2
    description: Active poison writes were measured across all movie frames and the preserved coverage is documented without a runtime-causality claim.
    requirement: GAME-07
    verification:
      - kind: other
        ref: Throwaway instrumented movie run exited 0; 2400 module frames summed to 589824000 expected and poisoned bytes.
        status: pass
      - kind: integration
        ref: cmake --workflow --preset ci-local-4; exit 0; 509 CI tests and 5 release tests passed.
        status: pass
      - kind: other
        ref: cmake --workflow --preset hygiene; exit 0; 23 checks passed.
        status: pass
    human_judgment: false
duration: 17min
completed: 2026-11-12
status: complete
---

# Phase 4 Plan 55: Movie Canary Efficiency Summary

**The movie harness poisons all 589,824,000 expected active bytes across 2,400 module frames while preserving the full pixel, audio, input and serialized-state regression coverage.**

## Performance

- **Duration:** 16m 53s
- **Started:** 2026-11-12T18:58:25Z
- **Completed:** 2026-11-12T19:15:18Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- Added a movie-only fill helper that validates both independently captured dimensions before multiplication, poisons the packed active extent, and falls back to the full buffer for zero or out-of-range dimensions.
- Kept all other `run_one` calls and their full-buffer poison fill unchanged; the movie comparator still checks dimensions and every active pixel.
- Measured 589,824,000 poison bytes over all 2,400 module movie frames, compared with 3,538,944,000 bytes for full-buffer fills.

## Task Commits

1. **Task 1: Poison every expected active movie pixel without filling unread capacity** - `f65775a` (perf)
2. **Task 2: Measure work and document the preserved contract** - `041790b` (docs)

## Files Created/Modified

- `tests/libretro/host.c` - Adds bounded poisoning only to the input movie loop.
- `docs/testing.md` - Explains the reference-dimension canary contract and measured byte work.

## Decisions Made

- Keep the reference frame as the independent source of expected width and height. An invalid or unknown extent uses full-buffer poisoning.
- Do not infer a runtime speedup or attribute the prior hosted timeout to these writes without the actual combined hosted run.

## Deviations from Plan

None - plan executed as written. Task 1 was committed before Task 2 probes and documentation.

## Issues Encountered

- CMake 4.4.3 does not support a workflow `--dry-run` option. The temporary preset parsed successfully and the full workflow ran normally.

## Evidence

- Baseline movie case: `ctest --test-dir build/ci --parallel 1 -R '^libretro[.]input[.]movies$' --no-tests=error --output-on-failure` — exit 0, 27.35 s.
- Changed-source tracer check and required post-commit tracer re-run — exit 0, 27.58 s and 42.38 s. These timings vary and do not establish a speedup.
- Throwaway instrumentation on the movie case — exit 0, 45.74 s; all 2,400 module frames had valid reference extents and summed to 589,824,000 expected active bytes and the same poisoned-byte total. Zero and 641x577 probes verified full-buffer fallback before `p_run`.
- One-shot omitted-pixel mutation: the movie case returned the expected CTest exit 8; the comparator reported `(0,0)` as `0xa5a5a5a5` versus reference `0x00000000`. The mutation and instrumentation were removed before the final workflow.
- Five adjacent controller, rumble and geometry CTest selections — exit 0; `git diff --check HEAD` — exit 0.
- `cmake --workflow --preset ci-local-4` — exit 0 in 62.69 s, using `build/ci` and four test jobs; all 509 main tests and all 5 release tests passed, and all three packages were produced.
- `cmake --workflow --preset hygiene` — exit 0 in 63.28 s; 23/23 checks passed.
- `cmake -DOPENBIOS_INPUT=build/ci/openbios/openbios.bin -DEXPECTED_FILE=tests/built/expected.txt -P cmake/OpenBIOS.cmake` — exit 0; pinned 524288-byte input digest accepted.

## User Setup Required

None.

## Next Phase Readiness

The local movie harness and its coverage are ready. The actual hosted movie timing and cross-platform acceptance remain pending the coordinator's combined run after the separate 04-29 wiring; this plan does not close 04-29 or complete GAME-03/GAME-07. No owner action is needed for this plan.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-12*

## Self-Check: PASSED
