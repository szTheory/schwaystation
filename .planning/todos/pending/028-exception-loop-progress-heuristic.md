---
title: "Stop the exception-loop heuristic reporting a guest that makes progress"
status: pending
priority: P2
source: "phase 02 code review, finding WR-04"
created: 2026-11-10
theme: cpu
bound_step: "phase 4, the first plan that edits src/cop0.c"
---

## Goal

A guest that raises the same exception at the same address more than 1,000 times in a row but whose handler returns
and moves on is not reported as a first fault; a handler that never advances still is.

## Context

- Review finding: `src/cop0.c` counts consecutive exceptions with one EPC and one ExcCode and latches the loop fault
  at 1,000, so a tight polling loop around one SYSCALL, BREAK or trapping ADD trips it. The runner's default stops
  on a first fault with status 7, so the exit code then says the guest hung.
- Options from the review: require that no progress is visible (for example a small retirement count between
  entries), or reset the count when a handler returns past EPC. At minimum document the false-positive shape next
  to the loop-counter paragraph in `docs/hardware/cop0.md`.
- Bound to phase 4: the first phase-4 plan that edits `src/cop0.c` (phase 4 is unplanned; name the plan here when
  it is written), or the first compatibility-ledger run that reports status 7 for a game that still runs.

## Acceptance Criteria

- A `cpu.exception.*` test runs a handler that returns past EPC 1,001 times and sees no first fault, and a handler
  that returns to EPC still latches the loop.
- `docs/hardware/cop0.md` states the rule.
- The test runs in `cmake --workflow --preset ci`, and no planning ID appears in the code it adds.
