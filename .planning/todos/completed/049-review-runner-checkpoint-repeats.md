---
title: "Review runner and checkpoint repeats against existing frame and state output"
status: complete
priority: P2
resolves_phase: 7
source: "Phase 04 plan 04-22 bounded CI review"
created: 2026-11-11
completed: 2026-11-27
closed_by: "17-02"
theme: ci
audit_acknowledged:
  milestone: v1.0
  at: 2026-11-18
---

## Goal

Measure whether runner or checkpoint tests and CI commands repeat work that can safely reuse already-produced
per-frame or state-row output, while preserving every required row and independent check.

## Context

- Plan 04-22 inspected the current runner and perf repetitions. `runner.hash_state` compares two identical runs
  with a different later frame; input replay repeats intentionally verify determinism; perf controls and base
  samples are independent measurements. No safe low-risk command removal was supported by the current evidence.
- Bind to plan 04-29, the next planned `.github/workflows/ci.yml` editor. It already measures hosted critical
  path and duplicate commands.
- Do not add a CLI, persistent cache, concurrency, or remove every-frame/state-row coverage.

## Phase 17 bounded review

The runner and checkpoint repetition review is complete. The exact invocations, output rows, single-run CTest
durations, and source links are recorded in
[`17-RELIABILITY.md`](../../phases/17-a-measured-and-trustworthy-playability-baseline/17-RELIABILITY.md).
The state-hash, movie-input, and cheat-hash repeats compare separate process results and preserve determinism,
frame-latch, normalized configuration, ordering, and rejection checks. The six libretro movie checkpoints and
the separate RetroArch playback check exercise different saved states and consumers. No existing output can
replace a required independent result, so no command was removed.

## Acceptance Criteria

- Plan 04-29 measures the relevant existing runner/checkpoint invocations and either makes one small, equivalent
  reduction that retains every named row and required independent result, or records why no safe reduction exists.
- Any changed behavior retains its gating test and documentation; no new cache, CLI, dependency, or coverage loss.
