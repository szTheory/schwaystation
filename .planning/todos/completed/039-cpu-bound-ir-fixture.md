---
title: "Add a CPU-bound instructions-per-frame fixture to the performance gate"
status: completed
priority: P3
source: "phase 03 discussion, decision on the performance gate"
created: 2026-11-10
theme: perf
bound_step: "phase 04-13, measure gpu_motion CPU/GPU instructions per frame against a control and arm the separate ceiling"
completed: 2026-11-11
closed_by: "04-13"
---

## Goal

The performance gate (instructions per frame under callgrind) measures a fixture that runs the CPU, so a slowdown in
the CPU loop, the GTE or the DMA engine moves a number the gate watches.

## Context

- The only gated fixture is the test card, which never runs the CPU, the GTE or DMA, so it measures none of them
  (1,620,424 instructions per frame at the phase 1 pin; plan 03-03 moved it to 1,620,545 with the scheduler).
- Phase 3 changed `cpu_run` and re-measured the gate once; it added no gate of its own and recorded one `--bench`
  wall-clock run of a looping `gte_vectors`.
- Plan 04-12 lands `gpu_motion`; plan 04-13 measures its CPU/GPU instructions per frame against an actual control
  and closes this todo with the command, exit status and hosted run URL.

## Acceptance Criteria

- A fixture that runs the CPU (and the GPU once it exists) has an `ir_per_frame` row in the performance gate, measured
  on a control before the gate is armed, with the run URL recorded in the plan summary.
- The change runs in `cmake --workflow --preset ci` or the perf preset, and no planning ID appears in the code it adds.

## Resolution and evidence

The performance gate compares the CPU/GPU `gpu_motion` workload with the same fixture on a control commit and
retains the existing test-card ceiling. A no-baseline control run at the 50-frame window measured 137,678,369
instructions per frame on three repeats with zero spread. Command: `gh workflow run ci.yml --ref
phase/04-games-play-in-retroarch`; dispatch exit 0, run
[42984345515](https://github.com/szTheory/schwaystation/actions/runs/42984345515). The final control/head run
compared `045b76a` to `3bb84fa`; both GPU-motion and test-card rows passed at 0 basis points in
[42985306022](https://github.com/szTheory/schwaystation/actions/runs/42985306022), whose required CI check passed.
