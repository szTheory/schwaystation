---
phase: 04-games-play-in-retroarch
plan: "07"
subsystem: gpu
tags: [fixed-point, gouraud, affine-textures, polygon-tests]
requires:
  - phase: 04-games-play-in-retroarch
    provides: [GP0 polygon dispatch, pixel pipeline, GPU page and CLUT state]
provides:
  - 16.16 Gouraud color and affine UV interpolation with safe wide setup arithmetic
  - Flat, Gouraud, textured, untextured, raw, modulated and semi-transparent polygon packet handling
  - Directed polygon interpolation, wrapping, flag and state-resume regression tests
affects: [GPU, GAME-01, phase-04 verification]
actuals:
  tokens: 7931
  tasks: 2
  commits: 3
commits: 3
plan_head_before: cfde9f4eef5fd38ae8956722879ff1beaebd2303
plan_head_after: adc060b38e72556ebc3a02a92eddd1238c0868f1
tech-stack:
  added: []
  patterns: [16.16 signed attribute steppers, wide setup with deterministic fallback]
key-files:
  created: [tests/gpu/interpolation.c]
  modified: [src/gpu_triangle.c, src/gpu.c, src/gpu.h, tests/gpu/tests.cmake, tests/gpu/inventory.txt, docs/hardware/gpu.md]
key-decisions:
  - "Use 16 fractional bits in stored 32-bit color and UV steppers; compute setup with wider integers and signed ties-away rounding."
  - "Treat this arithmetic as the deterministic model; hardware precision awaits the plan 30 capture comparison."
patterns-established:
  - "Directed GP0 packet tests pin interior pixels and side effects independently of a reference renderer."
  - "Wide direct-evaluation fallback handles attribute slopes that do not fit a stored 32-bit step."
requirements-completed: []
coverage:
  - id: D1
    description: "Gouraud colors and affine UVs interpolate across polygon interiors and clipped span starts."
    requirement: GAME-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^gpu\\.triangle\\.(gouraud_textured|texture_page_clut|clip_interpolation)$' --no-tests=error --output-on-failure (exit 0)"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci (exit 0; 374 CI tests and 2 release checks)"
        status: pass
    human_judgment: false
  - id: D2
    description: "Polygon flags, texture depths, negative UV gradients, VRAM wrapping and quad edge sharing have directed coverage."
    requirement: GAME-01
    verification:
      - kind: unit
        ref: "cmake --workflow --preset ci (exit 0; 36 GPU tests)"
        status: pass
    human_judgment: false
  - id: D3
    description: "Partial textured Gouraud packets resume deterministically from saved GPU state."
    requirement: GAME-01
    verification:
      - kind: unit
        ref: "cmake --workflow --preset ci (exit 0; split-state GPU regression included)"
        status: pass
    human_judgment: false
duration: 26min
completed: 2026-11-10
status: complete
---

# Phase 4 Plan 7: Gouraud and affine texture interpolation summary

**GP0 polygon attributes now use deterministic 16.16 Gouraud and affine UV steppers across supported triangle and quad packet forms.**

## Performance

- **Duration:** 26 min
- **Started:** 2026-11-10T21:09:50Z (first recorded execution check)
- **Completed:** 2026-11-10T21:35:57Z
- **Tasks:** 2
- **Files modified:** 7

## Accomplishments

- Implemented Gouraud and UV attribute stepping from original geometry, preserving interpolation when spans are clipped; setup uses wide integer arithmetic and an explicit wide fallback when a stored step cannot fit.
- Decoded flat/Gouraud and textured/untextured triangle and quad packet forms, preserving texture-page and CLUT side effects.
- Added directed tests for polygon flags and texture depths, equal Gouraud colors and dither, signed/wrapping UVs, quad sharing and degeneracy, channel midpoint rounding, hostile slopes, and save/load across a partial GP0 packet.
- Documented the arithmetic model and its confidence limits in `docs/hardware/gpu.md`.

## TDD Evidence

- Task 1 RED ran the registered real GP0 packet test and failed the intended interior-pixel and texture-page assertions. `gsd-tools.cjs check tdd-red-evidence` accepted `build/ci/tdd/04-07-task1-red.json`; semantic inspection confirmed behavioral assertions ran rather than setup or discovery failing.
- Task 1 GREEN passed the focused interpolation, texture-page/CLUT, and clipping cases. The tracer feedback gate then passed CI and the GPU suite before Task 2 expansion.
- Task 2's added directed cases were already green against the Task 1 implementation, so no additional source change was needed. The expansion tests validate the existing behavior; they are not represented as a separate RED/implementation cycle.

## Task Commits

1. **Task 1 RED:** `47651bf` — add the failing Gouraud textured interpolation contract.
2. **Task 1 GREEN:** `3d13c2b` — implement Gouraud and textured polygon interpolation.
3. **Task 2:** `adc060b` — cover polygon flags and interpolation edges.

## Files Created/Modified

- `src/gpu_triangle.c` — attribute setup, row/pixel stepping, clipping-origin interpolation and wide fallback.
- `src/gpu.c` — polygon packet decoding and texture-page side effects.
- `src/gpu.h` — updated the internal polygon function signature to pass decoded attributes; this direct interface adaptation was necessary to connect packet decoding to rasterization.
- `tests/gpu/interpolation.c` — independent GP0 packet and pixel/state assertions.
- `tests/gpu/tests.cmake` and `tests/gpu/inventory.txt` — registered and bytewise-sorted the interpolation cases.
- `docs/hardware/gpu.md` — fixed-point behavior, rounding, channel conversion and evidence limits.

## Verification

- Focused interpolation CTest selection: exit 0.
- Tracer gate: `cmake --workflow --preset ci && ctest --test-dir build/ci -L '^gpu$' --no-tests=error --output-on-failure`, exit 0; 367 CI tests, 2 release checks and 29 GPU tests.
- Final CI after Task 2: `cmake --workflow --preset ci`, exit 0; 374 CI tests, 2 release checks and 36 GPU tests.
- Repository hygiene: `cmake --workflow --preset hygiene`, exit 0; all 23 checks passed.

No hosted run URL was produced. No build-158 image comparison was run in this plan: 16.16 and the rounding rules are documented as a deterministic model, and no claim of hardware precision is made pending plan 30's capture comparison.

## Decisions Made

- Keep stored attribute steps in signed 32-bit 16.16 form, use wide setup arithmetic, and fall back to wide direct evaluation for unrepresentable slopes.
- Advance attributes from original polygon geometry to the clipped span start so clipping does not alter interpolation.
- Keep hardware precision explicitly unproven until the planned capture comparison supplies evidence.

## Deviations from Plan

### Direct integration adaptation

The internal `schway__gpu_polygon` declaration in `src/gpu.h` needed its signature updated to carry decoded attributes from `src/gpu.c` into the rasterizer. This was a necessary direct integration change omitted from the plan's explicit file list; it shipped with the Task 1 GREEN commit.

Plan 30 owns comparison plumbing for the pinned captures. Per execution direction, this plan kept the documented 16.16 model and directed contracts without reproducing that comparison.

## Issues Encountered

Two test expectations/setup details were corrected while authoring the Task 2 cases (E1 dither flag value and exact quad midpoint geometry). The corrected cases passed; no implementation defect remained.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

The polygon interpolation slice is implemented and covered. GAME-01 remains incomplete until the remaining GPU primitives and phase-wide evidence land; plan 30 must compare the pinned captures before hardware precision can be characterized.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-10*

## Self-Check: PASSED

- SUMMARY file exists and all three task commits are ancestors of HEAD.
- Coverage metadata validates with all three deliverables automatically covered by passing verification.
- No stub patterns were found in the files changed by this plan.
