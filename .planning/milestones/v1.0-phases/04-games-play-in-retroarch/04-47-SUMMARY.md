---
phase: 04-games-play-in-retroarch
plan: "47"
subsystem: gpu
tags: [gpu, hardware-captures, rgb555, rasterization]
requires:
  - phase: 04-46
    provides: Pinned GPU capture inputs and the full-VRAM RGB555 comparison path
  - phase: 04-58
    provides: Source-derived overlap guest completion evidence
provides:
  - Exact full-VRAM checks for the nine build-158 GPU captures
  - Strict, executable-bound rules for the two reviewed source variants
  - Capture-backed line stepping and bounded overlapping-copy models
affects: [04-48 GPU reference logs, Phase 04 verification, GPU rendering]
actuals:
  tokens: 6790
  tasks: 2
  commits: 2
plan_head_before: 06df36260eb6058c815385ca554b6863459957c3
plan_head_after: ac32819ece3ec893404fff33799c93dec794d15e
tech-stack:
  added: []
  patterns:
    - Exact whole-VRAM RGB555 comparison with only source-reviewed case variants
    - Source-counted fixed run bounds for captured guest programs
key-files:
  created: []
  modified:
    - cmake/GPUReferences.cmake
    - .planning/phases/04-games-play-in-retroarch/04-47-CHECKPOINT.md
    - docs/hardware/gpu.md
    - docs/testing.md
    - src/gpu.c
    - src/gpu_span.c
    - tests/gpu/allow.txt
    - tests/gpu/inventory.txt
    - tests/gpu/reference.c
    - tests/gpu/spans.c
    - tests/gpu/tests.cmake
    - tests/gpu/transfers.c
    - tests/nightly/tests.cmake
key-decisions:
  - Only two reviewed pixel variants are accepted; each requires its exact pinned executable SHA-256 and exact use count.
  - The line model uses the capture-supported spatial midpoint rule and signed truncated 12.12 color steps.
  - Overlapping copies read forward in bounded 16-word chunks; this is a capture-backed model, not a measured physical buffer width.
  - Rectangle comparison uses 32 fixed advances for source-derived blend convergence; overlap uses the source-supported 24-advance completion bound.
patterns-established:
  - Validate every RGB555 word and make any permitted pixel change explicit by case, coordinate rule, value, fixture hash, and exact count.
  - Keep caller completion evidence separate from pixel agreement.
requirements-completed: []
coverage:
  - id: D1
    description: The pinned GPU guests are compared against complete hardware VRAM captures with exact reviewed exceptions and fixed source-derived bounds.
    requirement: GAME-01
    verification:
      - kind: integration
        ref: "nightly GPU reference CTests: nine captures passed; clipping CTest passed separately"
        status: pass
      - kind: unit
        ref: "gpu.reference.allowance_controls, gpu.transfer.copy_order/copy_masks/copy_wrap/dimensions, gpu.span.polyline_packet, and gpu.dma.ready_budget"
        status: pass
      - kind: other
        ref: "cmake --workflow --preset ci-j4 and cmake --workflow --preset hygiene-j4"
        status: pass
    human_judgment: false
metrics:
  duration: 125 min
  completed: 2026-11-13
status: complete
---

# Phase 04 Plan 47: Match the Remaining GPU Captures

**The build-158 GPU captures now compare across all VRAM words, with two narrowly pinned source variants and source-supported completion bounds.**

## Performance

- **Duration:** About 125 minutes, from the 06:08 UTC Plan 58 handoff through 08:13 UTC for the resumed execution segment.
- **Runtime/model:** Codex, OpenGSD 1.20.0; gpt-6-luna, xhigh.
- **Tasks:** 2.
- **Files modified:** 13 across the resumed implementation and checkpoint closeout.
- **Commit measurement:** The resumed segment is measured from `06df362`; earlier partial Plan 47 commits are listed separately because intervening commits belong to other plans.

## Accomplishments

- Registered and compared the nine build-158 GPU captures at zero tolerance over all 524,288 VRAM words. The pre-existing clipping case remains compatible with the updated capture command.
- Kept two exact source-supported variants: 64 calculated Gouraud samples for the reviewed lines executable, and 447,488 untouched words outside 320x240 for the reviewed transparency executable. Both require their pinned executable SHA-256, match exact geometry and values, and must be used the exact number of times. All other pixels remain exact.
- Corrected line midpoint and channel stepping from the unchanged line capture, and fixed GP0 VRAM copy overlap with a 16-word read batch while retaining forward row order, independent coordinate wrapping, and E6 masking.
- Raised the rectangle comparison allowance to 32 fixed advances for its source-derived blend convergence and the overlap comparison to 24 advances using the independent caller-progress witness. No frame hunting or capture editing was used.
- Added directed octant, Gouraud midpoint, copy overlap, chunk-tail, fixture-hash, and allowance-refusal checks with hardware behavior documentation.

## Task Commits

- Earlier partial Task 1: `e89f147` — register the nine captures and fix CLUT invalidation.
- Earlier partial raster correction: `cf9f287` — match the captured polygon raster model.
- Resumed Task 1: `b46977c` — finish capture comparison, line/copy models, bounds, and documentation.
- Task 2: `ac32819` — register exact allowance refusal controls.

## Evidence

- Direct copied-archive strict C17 builds and affected tests exited 0: `copy_order`, `copy_masks`, `copy_wrap`, `dimensions`, `dma_ready_budget`, `polyline_packet`, and `allowance_controls`.
- All ten copied-archive capture runs matched all 524,288 words at their fixed bounds: clipping 1, lines 1, quad 1, triangle 1, UV interpolation 2, rectangles 32, transparency 1, texture overflow 2, CLUT cache 2, and overlap 24. The two approved variants were used exactly 64 and 447,488 times.
- `cmake --preset nightly`, the nightly runner/reference build, and the nine registered GPU capture CTests — exit 0, 9/9. `gpu.reference.clipping` — exit 0, 1/1.
- `cmake --preset ci`, the reference-target build, and `gpu.reference.allowance_controls` — exit 0, 1/1. After the task commits, `gpu.reference.allowance_controls` and `build.revision_matches_head` — exit 0, 2/2.
- `cmake --workflow --preset ci-j4` — exit 0, 524/524 ordinary tests and 5/5 release checks.
- `cmake --workflow --preset hygiene-j4` — exit 0, 23/23 tests.
- The GPU capture fetch controls and `git diff --check` — exit 0. The GPU inventory remained byte-sorted.

## Decisions Made

- The exact source variants are bound to both the build-158 archive digest and the matching executable SHA-256 values; changed executable bytes invalidate the allowance.
- GP0 copy overlap uses a deterministic 16-word horizontal read batch because that model matches the captured 2-, 8-, 15-, and 16-word cases. The capture does not identify a unique physical buffer width.
- The rectangle guest's fixed repeated-blend state needs up to 31 steps, so the comparison uses 32 advances including initialization margin. The overlap image comparison uses the separately proven 24-advance caller-completion bound.

## Deviations from Plan

None. The plan explicitly allowed reviewed, bounded renderer fixes when a pinned capture exposed a mismatch; the line and copy changes were reviewed before the affected captures were rerun. The executable hash guard completes the planned strict allowance policy.

## User Setup Required

None. No owner action is needed for this plan.

## Next Plan Readiness

Plan 47 is complete. The independent Plan 48 candidate and Phase 04 closeout remain with the coordinator; no Plan 48 changes were incorporated here. GAME-01 remains phase-pending because its motion, register/log, and fuzz criteria are tracked by other Phase 04 work.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-13*

## Self-Check: PASSED

- The summary file exists and `git diff --check` passed.
- Earlier partial commits `e89f147` and `cf9f287`, and resumed task commits `b46977c` and `ac32819`, are ancestors of the recorded pre-metadata HEAD.
- No TODO, FIXME, placeholder, or skipped-test stubs were found in the changed source, tests, or documentation.
