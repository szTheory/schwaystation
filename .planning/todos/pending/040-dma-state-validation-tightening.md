---
title: "DMA state validation matches the invariants the engine keeps"
status: pending
priority: P3
source: "phase 03 code review, finding IN-01"
created: 2026-11-10
theme: state
bound_step: "phase 4, the first plan that edits the DMA part of src/state.c or src/dma.c (the GPU channel's device end)"
---

## Goal

DMA state validation matches the invariants the engine keeps.

## Context

- Review finding: `phase` accepts 6 and 7 and `stall_until` is not tied to `cur`, so a hostile state can hold values the engine never produces. Not exploitable (the engine bounds them), but the loader should refuse them.
- Bound to: phase 4, the first plan that edits the DMA part of src/state.c or src/dma.c (the GPU channel's device end).

## Acceptance Criteria

- A `core.state.*` case loads a state with `phase` above 1 or a `stall_until` with no running channel and sees `ERR_STATE_FIELD`; a normal state still loads.
- The test runs in `cmake --workflow --preset ci`, and no planning ID appears in the code it adds.
