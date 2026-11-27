---
phase: 15-guncon-and-justifier
plan: "05"
subsystem: gpu
tags: [guncon, lightgun, gpu-beam, rgb555, rgb24]
requires:
  - phase: 15-03
    provides: [frame-latched direct-port gun input and per-port sensor state]
provides:
  - Bounded scheduler events that sample live displayed pixels at the aimed beam position
  - RGB555/RGB24 thresholding and geometry-bound no-hit behavior
affects: [15-06-PLAN.md, 15-09-PLAN.md]
actuals:
  tokens: 8360
  tasks: 2
  commits: 3
commits: 3
plan_head_before: cf4a32d6118a909bbf61454021879ea408b18ae7
plan_head_after: bc72f569b45aaf709150ce5945a976e58239a82c
tech-stack:
  added: []
  patterns: [deadline-driven beam sensing, scanout-equivalent VRAM sampling]
key-files:
  created: []
  modified: [src/gpu.c, src/frame.c, tests/gpu/beam.c, docs/hardware/gpu.md]
key-decisions:
  - "Sample the live displayed pixel at one scheduled GPU beam deadline per latched sensor."
  - "Use RGB5 channel sum >= 48 as an explicitly uncalibrated model and truncate regional 8 MHz X conversion."
patterns-established:
  - "Add bounded sensor deadlines to the GPU scheduler instead of retaining per-pixel history."
  - "Decode RGB24 gun samples with the same packed byte addressing as scanout."
requirements-completed: [GUN-01, GUN-02]
coverage:
  - id: D1
    description: "A valid direct-port aim samples live displayed VRAM once at its GPU beam deadline and returns deterministic coordinates."
    requirement: GUN-01
    verification:
      - kind: unit
        ref: "tests/gpu/beam.c#gpu.gun.sample"
        status: pass
      - kind: unit
        ref: "tests/gpu/beam.c#gpu.beam.midline_dot_irq and gpu.beam.equal_timer_beam_dma"
        status: pass
    human_judgment: false
  - id: D2
    description: "Threshold, active geometry, RGB555/RGB24 mapping, display changes and unavailable/offscreen inputs produce bounded deterministic hits or no hits."
    requirement: GUN-02
    verification:
      - kind: unit
        ref: "tests/gpu/beam.c#gpu.gun.geometry"
        status: pass
      - kind: unit
        ref: "tests/gpu/beam.c#gpu.beam.reset_call_bound and gpu.beam.field_edges"
        status: pass
    human_judgment: false
duration: 2h 28m
completed: 2026-11-25
status: complete
---

# Phase 15 Plan 05: Beam-Time GunCon Pixel Sampling Summary

**GunCon input now samples live RGB555/RGB24 display pixels at bounded beam deadlines using the existing GPU scheduler.**

## Performance

- **Duration:** 2h 28m (actual wall time; started at 2026-11-25T06:01:43Z)
- **Completed:** 2026-11-25
- **Tasks:** 2
- **Files modified:** 4
- **Runtime/model:** Codex executor, gpt-6-luna

## Accomplishments

- Connected the latched direct-port aim to one scheduler deadline and sampled live VRAM at that time; split budgets and before/after writes are deterministic.
- Matched RGB555 and packed RGB24 scanout addressing, RGB5 thresholding, regional 8 MHz coordinate conversion, active display bounds, and geometry retargeting.
- Added gating tests for threshold 47/48/49, five dot divisors, display edges and padding, VRAM wrapping, NTSC-J/U/PAL, interlace, invalid/no-hit input, and display changes.

## Task Commits

1. **Task 1: Schedule and sample one real bright GunCon path** — `6d3fba9` (feat; test and implementation landed together).
2. **Task 2: Expand geometry, threshold and no-hit cases** — RED tests `b519b6a`; implementation and matrix `bc72f56`.

The plan commit ledger measured three commits from `cf4a32d6118a909bbf61454021879ea408b18ae7` through `bc72f569b45aaf709150ce5945a976e58239a82c`.

## Files Modified

- `src/gpu.c` — sensor arming, deadline scheduling, RGB555/RGB24 sampling, and display retargeting.
- `src/frame.c` — synchronize the scheduler after input latching.
- `tests/gpu/beam.c` — public-run beam sample and geometry/no-hit vectors.
- `docs/hardware/gpu.md` — threshold, coordinate rounding, and scanout model documentation.

## Verification Evidence

- ABI and budget preflight: `abi.api_v9` and `policy.fixture_budget`, 2/2 passed.
- Task 1 target build and `./build/ci/tests/schwaystation-test-gpu-beam gun_sample`, exit 0; `gpu.gun.sample`, `gpu.beam.midline_dot_irq`, and `gpu.beam.equal_timer_beam_dma`, 3/3 passed.
- Task 2 `git diff --check`, `cmake --preset ci`, exact-selector inventory check, and `cmake --build --preset ci --target schwaystation-test-gpu-beam`, all exit 0.
- `gpu.gun.sample`, `gpu.gun.geometry`, `gpu.beam.reset_call_bound`, and `gpu.beam.field_edges`, 4/4 passed in 0.03 seconds.
- No workflow changed, so no `act` or hosted CI run was needed. No Docker resources were created.

## Decisions Made

- The sensor reads the current displayed VRAM value at the scheduled beam point; no pixel timestamp history is stored.
- Brightness is the sum of RGB5 channels with a threshold of 48. This is a deterministic model choice without physical sensor calibration.
- GunCon X conversion uses integer division (truncation toward zero); Y is the target beam line. Rounded scanout padding is outside the active sample range.

## Deviations from Plan

### TDD commit sequencing

Task 1 produced classifier-accepted RED evidence and then passed, but its test and implementation were combined in `6d3fba9` instead of separate RED and GREEN commits. Task 2 followed the separate test-only RED commit (`b519b6a`) and GREEN commit (`bc72f56`). This did not change behavior or verification results.

### Estimate overrun

The low-confidence estimate was 20–28 minutes; actual elapsed time was about 1 hour 47 minutes. Most of the extra time went to scheduler deadline/retarget reasoning and expanding boundary coverage. The first useful preflight was the single RGB555 beam path; future work should land and compile that slice before extending the format and geometry matrix, then keep the remaining checks targeted.

## Issues Encountered

- An initial zero-budget test expected a newly submitted input to be latched. The public run returns before latching when the budget is zero, so the probe was corrected to one cycle; once latched, the pixel at beam position (0,0) is sampled immediately. The final matrix passed.
- Evidence is synthetic. It does not establish retail-game compatibility or calibrated physical lightgun behavior; later Phase 15 plans cover host mapping and save-state continuation.

## Next Phase Readiness

The GPU beam sample and deterministic geometry contract are ready for Plan 15-06's Justifier IRQ timing work. The phase is not complete; later host integration and save-state plans remain.

## Self-Check: PASSED

- The summary file exists at the planned phase path.
- Task commits `6d3fba9`, `b519b6a`, and `bc72f56` are ancestors of HEAD.
- The plan ledger measures three commits from the recorded base.

---
*Phase: 15-guncon-and-justifier*
*Completed: 2026-11-25*
