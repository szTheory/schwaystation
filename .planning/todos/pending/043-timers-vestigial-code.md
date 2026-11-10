---
title: "Remove vestigial timer and DMA code"
status: pending
priority: P3
source: "phase 03 code review, finding IN-04"
created: 2026-11-10
theme: cleanup
bound_step: "phase 4 plan 11, the first plan that edits the DMA arbiter"
---

## Goal

Remove vestigial timer and DMA code.

## Context

- Review finding: The timer's unreachable index guard, unread `free_after` field and DMA's constant arbitrate mask are left from earlier plans.
- Timer side resolved in phase 4 plan 9: the beam owns timer dot/hblank feeds, and timer state no longer stores `free_after`.
- Remaining action: remove the DMA constant arbitrate mask when its first real arbiter caller lands.
- Bound to: phase 4 plan 11, the DMA arbiter implementation.

## Acceptance Criteria

- The DMA constant mask is removed or gets its first reader; `core.state.*`, timers and DMA labels pass.
- The test runs in `cmake --workflow --preset ci`, and no planning ID appears in the code it adds.
