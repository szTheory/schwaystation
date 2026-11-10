---
title: "Validate r0 and the COP0 registers when a state loads"
status: pending
priority: P2
source: "phase 02 code review, finding WR-02"
created: 2026-11-10
theme: state
bound_step: "phase 4, the first plan that edits src/cpu.c or the CPU part of the state walk"
---

## Goal

A state whose general register 0 is not zero, or whose SR, EPC, TAR, BadVaddr, PRID or CAUSE holds bits the CPU
can never produce, is refused on load, so a hostile state cannot reach CPU configurations the model rules out.

## Context

- Review finding: `src/cpu.c` walks all 32 general registers and the COP0 array with the full 32-bit range.
  `cpu_step` reads `r[0]` without special-casing it, so a loaded non-zero `r0` corrupts every use of `$zero`. The
  vector harness already treats a non-zero `r0` as malformed; the loader does not.
- SR bits that read 0 (6-7, 20-21, 23-27) and the read-only registers are accepted, and CAUSE bit 10 need not match
  `I_STAT & I_MASK`.
- Nothing is memory-unsafe; this is the rule that every walked field has a bound.
- Bound to phase 4: the first phase-4 plan that edits `src/cpu.c` or adds CPU-adjacent state fields to the walk
  (memory cards and pads add device state there). Bump the state format only if the field layout changes.

## Acceptance Criteria

- `core.state.*` cases show a loaded `r0` other than 0, an SR with a never-writable bit set and a CAUSE bit 10 that
  disagrees with the interrupt registers each return the invalid-state status, and a state the CPU saved still loads.
- The test runs in `cmake --workflow --preset ci`, and no planning ID appears in the code it adds.
