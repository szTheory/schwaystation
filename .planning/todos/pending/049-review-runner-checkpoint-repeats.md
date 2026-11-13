---
title: "Review runner and checkpoint repeats against existing frame and state output"
status: pending
priority: P2
source: "Phase 04 plan 04-22 bounded CI review"
created: 2026-11-11
theme: ci
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

## Acceptance Criteria

- Plan 04-29 measures the relevant existing runner/checkpoint invocations and either makes one small, equivalent
  reduction that retains every named row and required independent result, or records why no safe reduction exists.
- Any changed behavior retains its gating test and documentation; no new cache, CLI, dependency, or coverage loss.
