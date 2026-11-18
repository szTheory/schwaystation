---
phase: 04-games-play-in-retroarch
plan: "06"
subsystem: gpu
tags:
  - gpu
  - flat-polygons
  - rasterization
  - clipping

requires:
  - phase: 04-games-play-in-retroarch
    plan: "01"
    provides: GPU test registration and per-directory inventory.
  - phase: 04-games-play-in-retroarch
    plan: "03"
    provides: GP0 packet decoding, bus entry, and GPU instance state.
  - phase: 04-games-play-in-retroarch
    plan: "04"
    provides: Instance-owned GPU cache and VRAM interfaces.
  - phase: 04-games-play-in-retroarch
    plan: "05"
    provides: Shared native RGB555 pixel pipeline.
provides:
  - Flat untextured GP0 triangles and quads with integer top-left edge coverage.
  - Bounded polygon span rejection before inclusive E3/E4 clipping.
  - Independent packet-level tests for winding, shared-edge blending, span limits, offsets, clipping, and instance isolation.
affects:
  - gpu
  - rendering
  - retroarch-video

actuals:
  tokens: 5953
  tasks: 2
  commits: 5
plan_head_before: e5a218dc95b08be8fbd55322497b712f1a89faf5
plan_head_after: 600155a9fd691acbba72c20e04a158a0a504372f
tech-stack:
  added: []
  patterns:
    - Use doubled integer pixel-center samples and widened edge setup for deterministic polygon coverage.
    - Exercise renderer behavior through real GP0 packets and independently enumerated VRAM footprints.
key-files:
  created:
    - src/gpu_triangle.c
    - tests/gpu/triangles.c
  modified:
    - CMakeLists.txt
    - src/gpu.c
    - src/gpu.h
    - tests/gpu/tests.cmake
    - tests/gpu/inventory.txt
    - docs/hardware/gpu.md
key-decisions:
  - Use 64-bit integer area and edge setup, normalize winding, and assign exact-edge samples with a top-left rule.
  - Reject offset-applied spans above 1023 by 511 before clipping; retain inclusive E3/E4 edges.
  - Split quads into triangles (1,2,3) then (2,3,4) and write each covered pixel immediately through the shared helper.
requirements-completed: []
coverage:
  - id: D1
    description: Real flat GP0 triangle and quad packets render winding-invariant coverage; zero-area and one-pixel footprints are pinned, and shared-diagonal pixels blend exactly once.
    requirement: GAME-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^gpu\\.triangle\\.flat_packet$' --no-tests=error --output-on-failure (exit 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: Horizontal and vertical span boundaries, signed extrema and offsets, inclusive and inverted clip windows, offscreen rejection, and independent instances are checked through packet behavior.
    requirement: GAME-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^gpu\\.triangle\\.' --no-tests=error --output-on-failure (exit 0; 7 cases)"
        status: pass
    human_judgment: false
  - id: D3
    description: All registered GPU, ABI, fixture, inventory, packaging, hygiene, and repository policy checks pass with the polygon renderer included.
    requirement: GAME-01
    verification:
      - kind: other
        ref: "cmake --workflow --preset ci (exit 0; 364 CI cases and 2 release archive checks)"
        status: pass
      - kind: other
        ref: "cmake --workflow --preset hygiene (exit 0; 23 checks)"
        status: pass
    human_judgment: false

duration: 20min
completed: 2026-11-10
status: complete
---

# Phase 4 Plan 6: Flat GP0 polygon coverage summary

**Flat GP0 triangles and quads now use integer edge coverage, exact shared-edge ownership, bounded spans, and directed clipping tests.**

## Performance

- **Duration:** 20 min
- **Started:** 2026-11-10T20:36:32Z
- **Completed:** 2026-11-10T20:56:07Z
- **Tasks:** 2
- **Files modified:** 8

## Accomplishments

- Added real GP0 dispatch for flat untextured triangles and quads. Signed vertices and E5 offsets feed widened integer edge setup, winding normalization, top-left ownership, inclusive draw-area clipping, and the existing pixel helper.
- Split quads into the planned triangle order and proved all winding permutations, zero-area rejection, independently enumerated one-pixel coverage, and single-owner transparent blending on the shared diagonal.
- Rejected offset-applied spans above 1023 horizontally or 511 vertically before clipping. Added exact boundary, signed-extreme, draw-area, offscreen, and instance-isolation cases, and documented the implemented rules with source references.

## TDD RED Evidence

- Task 1's `gpu.triangle.flat_packet` assertion failed before the polygon renderer existed; the accepted report is `build/ci/tdd/04-06-task1-red.json`.
- A second task 1 test commit exposed incorrect winding for a valid area-one triangle with a 500-line span; `build/ci/tdd/04-06-task1-area-red.json` records the accepted failure.
- Task 2's horizontal 1024-span assertion failed before span rejection; `build/ci/tdd/04-06-task2-red.json` records the accepted failure.
- Each report passed `gsd_run check tdd-red-evidence`; the corresponding registered behavior tests pass after implementation.

## Task Commits

1. **Task 1: Trace an untextured triangle packet** — `9af591b` (test), `e7a4947` (test regression), `78a97bc` (feat).
2. **Task 2: Pin span rejection and clip arithmetic boundaries** — `027e618` (test), `600155a` (feat).

## Files Created/Modified

- `src/gpu_triangle.c` implements signed vertex decoding, widened integer setup, bounded edge traversal, span rejection, and triangle/quad rendering.
- `src/gpu.c`, `src/gpu.h`, and `CMakeLists.txt` connect the polygon packet path and ship the module.
- `tests/gpu/triangles.c`, `tests/gpu/tests.cmake`, and `tests/gpu/inventory.txt` register seven packet-level polygon cases.
- `docs/hardware/gpu.md` documents the pixel-center, top-left, span, offset, clipping, and quad-split rules with source references.

## Decisions Made

- Pixel-center samples use doubled integer coordinates, which preserves half-pixel coverage without floating point.
- A quad is treated as one polygon for span rejection, then written as the two specified triangles so the shared diagonal is owned once.
- Flat untextured polygons use the pixel helper without dither eligibility; their semi-transparent flag still follows the GP0 opcode.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Separated signed triangle area from pixel-sample edge evaluation**
- **Found during:** Task 1 review after the first focused pass.
- **Issue:** Winding normalization used an edge function evaluated at a pixel center. A narrow valid triangle could therefore be assigned the wrong winding and lose its independently expected pixel.
- **Fix:** Added a dedicated widened integer cross-product for signed area and an area-one, 500-line-span regression case.
- **Files modified:** `src/gpu_triangle.c`, `tests/gpu/triangles.c`.
- **Verification:** The regression failed before the fix and `gpu.triangle.flat_packet` plus the full CI workflow passed afterward.
- **Committed in:** `e7a4947` (test) and `78a97bc` (fix).

**Total deviations:** 1 auto-fixed (Rule 1 bug).
**Impact on plan:** The correction tightened the planned winding and one-pixel coverage contract; no unrelated changes were made.

## Verification

- `ctest --test-dir build/ci -R '^gpu\.triangle\.' --no-tests=error --output-on-failure` — exit 0; all 7 polygon cases passed.
- `cmake --workflow --preset ci` — exit 0; 364 CI cases and 2 release archive checks passed.
- `cmake --workflow --preset hygiene` — exit 0; all 23 checks passed, including line budgets, no-float, inventory, and tree hygiene.
- No hosted run URL was produced; verification ran locally.

## Issues Encountered

The initial inventory edit was not byte-sorted and was corrected before the first RED run. The narrow-triangle RED uncovered the area-setup bug recorded above; both issues were resolved in-plan.

## User Setup Required

None — no external service configuration or owner verification is required for these deterministic renderer cases.

## Next Phase Readiness

The flat polygon packet path is ready for later polygon-attribute and primitive plans. `GAME-01` remains incomplete because other GPU families and reference comparisons are assigned to later plans. No owner action is needed.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-10*

## Self-Check: PASSED

- Summary file exists at the expected phase path.
- All five task commits are ancestors of the plan head recorded above.
