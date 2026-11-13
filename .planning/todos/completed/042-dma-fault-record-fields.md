---
title: "The DMA fault record uses addr and opcode like every other fault kind"
status: completed
priority: P3
source: "phase 03 code review, finding IN-03"
created: 2026-11-10
theme: api
bound_step: "phase 4, the first plan that edits runner/trace.c or the fault record in src/internal.h"
---

## Goal

The DMA fault record uses addr and opcode like every other fault kind.

## Context

- Review finding: `SCHWAY_FAULT_DMA` puts the physical register address in `addr` and the written value in `opcode`, unlike the other kinds, which carry the faulting address and instruction.
- Bound to: phase 4, the first plan that edits runner/trace.c or the fault record in src/internal.h.

## Acceptance Criteria

- The record's meaning is documented in `docs/embedding.md` and the runner footer, or aligned with the other kinds, with `tests/api/v3.c` and `dma.fault.sync3` updated.
- The test runs in `cmake --workflow --preset ci`, and no planning ID appears in the code it adds.

## Resolution

Completed in Phase 4 plan 32: docs/embedding.md specifies the word-aligned physical CHCR address, raw written value and writing instruction PC; docs/runner.md describes the same DMA fault fields. The frozen API v3 caller and dma.fault.sync3 remain gating and passed in hosted CI run 43404686338. No fault-record layout changed.
