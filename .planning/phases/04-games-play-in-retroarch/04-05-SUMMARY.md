---
phase: 04-games-play-in-retroarch
plan: "05"
subsystem: gpu
tags:
  - gpu
  - pixel-pipeline
  - rgb555
  - textured-rectangles

requires:
  - phase: 04-games-play-in-retroarch
    plan: "01"
    provides: GPU test registration and inventory seams.
  - phase: 04-games-play-in-retroarch
    plan: "03"
    provides: Shared GPU decoder, bus path, and validated GPU state.
  - phase: 04-games-play-in-retroarch
    plan: "04"
    provides: Instance-owned texture and CLUT lookup caches.
provides:
  - GP0 textured 1x1 rectangles that fetch, shade, mask, and write native RGB555 pixels.
  - Integer modulation, blending, dithering, and primitive eligibility rules with exact known-answer tests.
affects:
  - gpu
  - rendering
  - pixel-pipeline

actuals:
  tokens: 7556
  tasks: 2
  commits: 5
plan_head_before: 1d6984bedaeae60fa023ce79c1b23f23f30c3e5a
plan_head_after: 3cce27e6861e3d063659e9270be5e0f0bb0cb4b4

tech-stack:
  added: []
  patterns:
    - Keep texture decode and pixel arithmetic in the instance-owned GPU pipeline.
    - Pass dither eligibility from the primitive into the shared pixel path.
    - Treat blend width and truncation as a documented model choice until hardware captures discriminate them.

key-files:
  created:
    - src/gpu_pixel.c
    - .planning/todos/pending/048-split-gpu-pixel-test-cases.md
  modified:
    - src/gpu.c
    - src/gpu.h
    - CMakeLists.txt
    - tests/gpu/pixels.c
    - tests/gpu/tests.cmake
    - tests/gpu/inventory.txt
    - docs/hardware/gpu.md

key-decisions:
  - "Use the direct fifteen-bit decode model for reserved texture depth value three; record it as modeled behavior."
  - "Only eligible primitive paths dither; rectangles and raw textured semi-transparent polygons do not acquire dither."
  - "Keep blend width and truncation doc/derived because existing captures do not isolate those arithmetic branches."

patterns-established:
  - "Exercise rendering behavior through GP0 and inspect destination VRAM words for the rectangle tracer."
  - "Use small exact-word discriminators for arithmetic and masks independently of image references."

requirements-completed: []
coverage:
  - id: D1
    description: A real GP0 textured 1x1 rectangle fetches 4-bit, 8-bit, and 15-bit texels through page, texture-window, and CLUT addressing, then applies transparency, texel blend control, and E6 destination masks.
    requirement: GAME-01
    verification:
      - kind: integration
        ref: "gpu.pixel.textured_rectangle_mmio in cmake --workflow --preset ci (exit 0)"
        status: pass
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^gpu\.pixel\.(arithmetic|palette_wrap|textured_rectangle_mmio)$' --no-tests=error --output-on-failure (exit 0; 3 tests)"
        status: pass
    human_judgment: false
  - id: D2
    description: Integer modulation, all four five-bit blend modes, saturation and subtraction bounds, all sixteen dither offsets, eligibility policy, palette wrapping, reserved depth handling, and mask outcomes have exact known-answer checks.
    requirement: GAME-01
    verification:
      - kind: unit
        ref: "gpu.pixel.arithmetic and gpu.pixel.palette_wrap in cmake --workflow --preset ci (exit 0)"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci (exit 0; 357 tests and 2 release archive checks)"
        status: pass
    human_judgment: false

duration: 39min
completed: 2026-11-10
status: complete
---

# Phase 4 Plan 5: Textured pixel pipeline summary

**The GPU now renders a real GP0 textured rectangle through integer texel lookup, RGB555 shading, blending, dither policy, and destination masks.**

## Performance

- **Duration:** 39 min
- **Started:** 2026-11-10T19:51:07Z
- **Completed:** 2026-11-10T20:30:29Z
- **Tasks:** 2
- **Files modified:** 9

## Accomplishments

- Added a fixed 1x1 GP0 rectangle path that reads texture and CLUT data through the instance-owned caches and writes exact destination RGB555 words.
- Implemented integer modulation, four blend modes, dither, texture bit-15 blending, and E6 skip/force-mask behavior. Added discriminators for direct, 4-bit, 8-bit, reserved-depth, window and palette wrap cases, transparency, raw texture, and rectangle dither exclusion.
- Documented modeled versus capture-backed GPU behavior, including the current doc/derived status of blend width and truncation.

## TDD RED Evidence

- Task 1: `build/ci/tdd/04-05-task1-red.json`; the classifier accepted the expected failure because the rectangle destination RGB555 word was wrong.
- Task 2: `build/ci/tdd/04-05-task2-red.json`; the classifier accepted the expected failure that exposed dither exclusion incorrectly applying to untextured semi-transparent pixels.
- Both reports passed `gsd-tools check tdd-red-evidence`; the corresponding GREEN changes and final coverage additions passed the full CI workflow.

## Task Commits

1. **Task 1: Textured rectangle RED case** — `641a2d4` (test).
2. **Task 1: GP0 textured rectangle pixel path** — `4fbcdfe` (feat).
3. **Task 2: Arithmetic and dither RED discriminators** — `153cbc5` (test).
4. **Task 2: Correct raw-texture dither exclusion** — `f31dae4` (fix).
5. **Review follow-up: close rectangle policy coverage gaps** — `3cce27e` (fix).

## Files Created/Modified

- `src/gpu_pixel.c` adds texture decoding, modulation, dither, blending, and pixel mask helpers.
- `src/gpu.c` routes fixed textured rectangles through the shared pixel pipeline.
- `src/gpu.h` declares the pixel helpers and their instance-owned inputs.
- `CMakeLists.txt`, `tests/gpu/tests.cmake`, and `tests/gpu/inventory.txt` register the pixel test executable and gated cases.
- `tests/gpu/pixels.c` covers GP0 rectangle output and exact arithmetic/policy answers.
- `docs/hardware/gpu.md` distinguishes documented model rules from hardware-measured behavior.
- `.planning/todos/pending/048-split-gpu-pixel-test-cases.md` records the review seed to split long test cases in the next tune-up.

## Decisions Made

- Reserved texture depth three follows the direct fifteen-bit model, explicitly documented as a model rule.
- Primitive callers decide dither eligibility; the rectangle path is undithered, while policy tests keep raw textured semi-transparent polygons out of the dither path.
- Blend width and truncation stay doc/derived because the available captures do not isolate those internal arithmetic choices.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Limited raw semi-transparent dither exclusion to textured pixels**
- **Found during:** Task 2 arithmetic RED test.
- **Issue:** The exclusion predicate also disabled dither for untextured semi-transparent pixels.
- **Fix:** Scoped that policy branch to textured pixels and added a regression discriminator.
- **Files modified:** `src/gpu_pixel.c`, `tests/gpu/pixels.c`.
- **Verification:** `gpu.pixel.arithmetic` and the full CI workflow passed.
- **Committed in:** `f31dae4`.

**2. [Rule 2 - Missing coverage] Added two missing policy discriminators after review**
- **Found during:** Final code review.
- **Issue:** The planned reserved-depth direct model and undithered GP0 rectangle behavior under enabled E1 dither were not independently exercised.
- **Fix:** Added both known-answer cases and split the production rectangle helpers for focused control flow. Test-case length cleanup is recorded as seed 048 under the project rule for test/tooling review findings.
- **Files modified:** `src/gpu.c`, `src/gpu_pixel.c`, `tests/gpu/pixels.c`, `docs/hardware/gpu.md`.
- **Verification:** Focused pixel CTest selection and full CI/hygiene workflows passed.
- **Committed in:** `3cce27e`.

**Total deviations:** 2 auto-fixed (one bug, one coverage gap).
**Impact on plan:** All deviations directly closed correctness or planned-coverage gaps; the test-only cleanup remains a pending seed.

## Verification

- `cmake --workflow --preset ci` — exit 0; 357 tests and 2 release archive checks passed.
- `cmake --workflow --preset hygiene` — exit 0; all 23 checks passed.
- `ctest --test-dir build/ci -R '^gpu\.pixel\.(arithmetic|palette_wrap|textured_rectangle_mmio)$' --no-tests=error --output-on-failure` — exit 0; all 3 selected cases passed.
- No hosted run URL was produced; these were local workflows.

## Issues Encountered

The first arithmetic RED run exposed a policy predicate that excluded untextured pixels too. The fix and a focused regression test resolved it. The final review found two planned policy cases missing; both were added before the final verification run.

## User Setup Required

None — no external service configuration is required.

## Next Phase Readiness

The shared pixel path is ready for later primitive renderers. `GAME-01` remains incomplete because its broader triangle, line, clipping, register-oracle, and nightly reference requirements are owned by other phase plans. No owner action is needed for this plan.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-10*

## Self-Check: PASSED

- Summary file exists at the expected phase path.
- All five task commits are ancestors of the current plan head.
