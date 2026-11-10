---
title: "Add a CPU-bound instructions-per-frame fixture to the performance gate"
status: pending
priority: P3
source: "phase 03 discussion, decision on the performance gate"
created: 2026-11-10
theme: perf
bound_step: "phase 4, the plan that lands the gpu_motion fixture"
---

## Goal

The performance gate (instructions per frame under callgrind) measures a fixture that runs the CPU, so a slowdown in
the CPU loop, the GTE or the DMA engine moves a number the gate watches.

## Context

- The only gated fixture is the test card, which never runs the CPU, the GTE or DMA, so it measures none of them
  (1,620,424 instructions per frame at the phase 1 pin; plan 03-03 moved it to 1,620,545 with the scheduler).
- Phase 3 changed `cpu_run` and re-measured the gate once; it added no gate of its own and recorded one `--bench`
  wall-clock run of a looping `gte_vectors`.
- Bound to phase 4: the plan that lands the `gpu_motion` fixture, the first fixture that runs the CPU every frame
  and so can carry a second Ir ceiling.

## Acceptance Criteria

- A fixture that runs the CPU (and the GPU once it exists) has an `ir_per_frame` row in the performance gate, measured
  on a control before the gate is armed, with the run URL recorded in the plan summary.
- The change runs in `cmake --workflow --preset ci` or the perf preset, and no planning ID appears in the code it adds.
