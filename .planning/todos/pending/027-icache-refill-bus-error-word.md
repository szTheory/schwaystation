---
title: "Keep a valid cached word when a later refill word bus-errors"
status: pending
priority: P2
source: "phase 02 code review, finding WR-03"
created: 2026-11-10
theme: cpu
bound_step: "phase 4, the first plan that edits src/cpu.c"
---

## Goal

A bus error while refilling a later word of an I-cache line leaves that word invalid with its old content untouched,
as `docs/hardware/bus.md` says ("leaves those words invalid"), instead of writing 0 over a word whose valid bit is
still set.

## Context

- Review finding: the refill loop in `src/cpu.c` passes the cache word to `schway__bus_fetch`, which zeroes its
  value argument before it decodes, even on the error return. A same-tag miss keeps words that are already valid, so
  a previously valid word can be overwritten with 0 (a silent NOP) when its address now errors.
- The fix is to fetch into a local, assign only on success and clear the valid bit on failure.
- Bound to phase 4: the first phase-4 plan that edits `src/cpu.c` (phase 4 is unplanned; name the plan here when it
  is written). The scenario needs a guest to rewrite the RAM size or unmap a window under cached code, which no
  phase-3 fixture does.

## Acceptance Criteria

- A `bus.icache_*` case fills a line, makes a later word unmapped, refills from word 0 and checks that word's valid
  bit and content.
- The test runs in `cmake --workflow --preset ci`, and no planning ID appears in the code it adds.
