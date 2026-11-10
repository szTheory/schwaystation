---
title: "Decide the bus contract for SWL and SWR and pin it with a test"
status: pending
priority: P3
source: "phase 02 code review, finding IN-07"
created: 2026-11-10
theme: bus
bound_step: "phase 4, the first plan that adds a width-sensitive GPU register or edits the store path in src/cpu_exec.inc"
---

## Goal

A partial store (SWL, SWR) has one stated bus contract (one write with a byte-lane mask, or the chunk list), pinned
by a test instead of the vector harness's coalescing.

## Context

- Review finding: the core issues each SWL or SWR chunk as a separate 4, 2 or 1 byte write through `cpu_bus_write`,
  so one instruction gets up to two write-queue entries, and to a KSEG1 target a drain between chunks. A device that
  cares about access width sees two narrow writes. The review could not establish from the repository which is
  right on hardware, and vector rule (b) deliberately does not compare the chunk decomposition.
- Phase 3 built the DMA and timer registers, which are word-addressed and not width-sensitive for partial stores;
  the first device that is width-sensitive for them is the GPU.
- Bound to phase 4: the first phase-4 plan that adds a GPU register to the I/O page, before it lands (phase 4 is
  unplanned; name the plan here when it is written).

## Acceptance Criteria

- The decision is written in `docs/hardware/bus.md` with its source or marked as the core's own model.
- A `cpu.*` or `bus.*` test pins the contract for SWL and SWR to RAM and to an I/O register.
- The test runs in `cmake --workflow --preset ci`, and no planning ID appears in the code it adds.
