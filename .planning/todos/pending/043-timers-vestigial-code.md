---
title: "Remove vestigial timer and DMA code"
status: pending
priority: P3
source: "phase 03 code review, finding IN-04"
created: 2026-11-10
theme: cleanup
bound_step: "phase 4, the first plan that edits src/timers.c (the GPU line feed)"
---

## Goal

Remove vestigial timer and DMA code.

## Context

- Review finding: A dead `n >= SCHWAY__TIMERS` guard, the write-only `free_after` field and the constant arbitrate mask are left from earlier plans.
- Bound to: phase 4, the first plan that edits src/timers.c (the GPU line feed).

## Acceptance Criteria

- Each item is removed or given its first reader; `core.state.*` and the timers and dma labels still pass.
- The test runs in `cmake --workflow --preset ci`, and no planning ID appears in the code it adds.
