---
phase: 04-games-play-in-retroarch
plan: "08"
subsystem: gpu
tags: [gpu, integer-dda, lines, polylines, rectangles, texture-flips]
requires:
  - phase: 04-games-play-in-retroarch
    plan: "01"
    provides: GPU test registration and inventory
  - phase: 04-games-play-in-retroarch
    plan: "03"
    provides: GP0 packet decoding and GPU instance state
  - phase: 04-games-play-in-retroarch
    plan: "05"
    provides: Shared RGB555 pixel pipeline
  - phase: 04-games-play-in-retroarch
    plan: "06"
    provides: GPU rasterization and bounded clipping patterns
  - phase: 04-games-play-in-retroarch
    plan: "07"
    provides: GPU texture-page state and pixel interpolation
provides:
  - Integer major-axis DDA lines and bounded flat/Gouraud polyline packet streaming
  - Fixed and variable GP0 rectangle spans with clipping and shared pixel processing
  - Profile-aware v0/v2 rectangle texture flip behavior
affects: [gpu, rendering, GAME-01, 04-30 hardware comparison]
actuals:
  tokens: 11378
  tasks: 2
  commits: 3
commits: 3
plan_head_before: cc92dc42264195fe59c0d6ebecc7620127c5fa3f
plan_head_after: 397e161df258724d43cf045c8316c4fae35df5a1
tech-stack:
  added: []
  patterns:
    - GP0 packet tests inspect native VRAM through the bus path
    - Integer spans emit pixels through the shared instance-owned helper
key-files:
  created: []
  modified:
    - CMakeLists.txt
    - docs/embedding.md
    - docs/hardware/gpu.md
    - include/schwaystation.h
    - src/gpu.c
    - src/gpu.h
    - src/gpu_span.c
    - src/instance.c
    - src/state.c
    - tests/api/v4.c
    - tests/gpu/inventory.txt
    - tests/gpu/spans.c
    - tests/gpu/tests.cmake
key-decisions:
  - "Keep GPU v0 as the additive profile flag value 4; GPU v2 remains the default and frozen API v1-v3 callers remain unchanged."
  - "Keep the odd-UV/odd-width sampling anomaly unresolved until plan 30 compares the pinned hardware captures."
patterns-established:
  - "Polyline packet state is bounded, instance-owned, and validated in save states."
  - "Rectangle texture flips are selected by the GPU profile and use the common texture and pixel paths."
requirements-completed: []
coverage:
  - id: D1
    description: "Flat and Gouraud GP0 lines include endpoints, respect span and clip bounds, and stream bounded flat/Gouraud polyline packets with terminators and save-state resume."
    requirement: GAME-01
    verification:
      - kind: unit
        ref: "cmake --workflow --preset ci (exit 0; gpu.span.polyline_packet passed)"
        status: pass
    human_judgment: false
  - id: D2
    description: "Fixed and variable rectangles cover size limits, E5/E3/E4 clipping, dither behavior, UV/page/VRAM wrapping, raw and modulated texture, and semi-transparent STP handling."
    requirement: GAME-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^gpu\\.span\\.rectangle_' --no-tests=error --output-on-failure (exit 0; 3 cases)"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci (exit 0; 378 tests and 2 release checks)"
        status: pass
    human_judgment: false
  - id: D3
    description: "GPU v0 ignores rectangle texture flips while the default v2 profile applies horizontal and vertical flips; the new profile flag passes API and state validation."
    requirement: GAME-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^gpu\\.span\\.rectangle_profile$|^abi\\.api_v[1-4]$' --no-tests=error --output-on-failure (included in passing full CI)"
        status: pass
      - kind: other
        ref: "cmake --workflow --preset hygiene (exit 0; 23 checks)"
        status: pass
    human_judgment: false
duration: "38min 51s+ (lower bound from first durable task commit)"
completed: 2026-11-10
status: complete
---

# Phase 4 Plan 8: GPU line and rectangle spans summary

**GP0 lines, streaming polylines, and fixed/variable rectangle spans now use bounded integer spans with directed packet tests and profile-aware texture flips.**

## Performance

- **Duration:** At least 38 minutes 51 seconds from the first durable Task 1 commit through summary creation; the original executor start time was lost during the daemon restart, and this elapsed interval includes the restart gap.
- **Started:** 2026-11-10T21:47:03Z (Task 1 RED commit; original start was not preserved).
- **Completed:** 2026-11-10T22:25:54Z (summary creation; final metadata close-out follows).
- **Tasks:** 2.
- **Files modified:** 13.

## Accomplishments

- Added bounded integer major-axis line stepping, Gouraud color interpolation, and incremental polyline parsing with a terminator checked only at the expected vertex or color position. Partial polyline state is included in validated save states.
- Added fixed 1 by 1, 8 by 8, and 16 by 16 rectangles plus variable 10-bit-width/9-bit-height spans, signed offsets, inclusive clipping, texture wrapping, and shared pixel processing. Rectangles do not dither.
- Added the append-compatible GPU v0 profile flag. Directed cases distinguish v0 from the default v2 texture flips and cover raw/modulated texture, semi-transparency, and RGB555 output.

## Task Commits

1. **Task 1 RED:** `418a377` — add the failing GPU line and polyline tracer.
2. **Task 1 GREEN:** `d242918` — implement integer GPU line and rectangle spans.
3. **Task 2:** `397e161` — cover GPU rectangle sizes and texture modes.

## TDD Evidence

- Task 1's registered GP0 tracer failed on the intended missing line/polyline pixels before implementation; its RED evidence was classifier-accepted. The GREEN implementation passed the focused tracer and the pre-restart full tracer workflow.
- Task 2 expanded tests around behavior already implemented in Task 1. No implementation-specific RED was fabricated. The first expansion run found incorrect test setup or expected values; those cases were corrected and the focused rectangle tests then passed.

## Verification

- `cmake --build build/ci --target schwaystation-test-gpu-spans && ctest --test-dir build/ci -R '^gpu\\.span\\.rectangle_' --no-tests=error --output-on-failure` — exit 0; all 3 rectangle cases passed.
- `cmake --workflow --preset ci` — exit 0; all 378 tests and both release checks passed.
- `cmake --workflow --preset hygiene` — exit 0; all 23 checks passed, including the code, tree, line-budget, and packaging checks.
- No hosted run URL was produced; verification was local.

## Decisions Made

- Keep GPU v0 as an additive profile flag at value 4; the default remains GPU v2. Frozen API v1-v3 callers stay unchanged, and API v4 covers the new flag.
- Keep the odd-UV/odd-width one-pixel sampling anomaly unresolved. The present tests pin the deterministic sampler model but do not claim to reproduce an unsupported anomaly pattern; plan 30 owns the pinned capture comparison.

## Deviations from Plan

None. The Task 2 implementation behavior was already present in Task 1, so Task 2 added the planned expansion tests and documentation without another source implementation change.

## Issues Encountered

The initial full CI run and follow-up focused run exposed four test-authoring corrections across the rectangle cases: the maximum-size rectangle needed to start one pixel earlier to include the clipped last edge; a direct texture-memory edit needed GP0 cache invalidation; the flipped-row sample needed the expected vertical reversal; and blended textured output preserves STP bit 15. The tests were corrected to match the existing pixel and cache contracts. No GPU implementation defect remained.

## User Setup Required

None.

## Next Phase Readiness

The line, polyline, and rectangle span families are covered for the current deterministic model. GAME-01 remains incomplete until later GPU work and plan 30's pinned capture comparison establish the remaining phase evidence. The odd-UV/odd-width hardware anomaly remains uncharacterized.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-10*

## Self-Check: PASSED

- SUMMARY exists, all three task commits are ancestors of HEAD, and the coverage classifier accepted all three deliverables.
