---
title: "Read the interrupted word at an interrupt entry without side effects"
status: complete
priority: P3
source: "phase 02 code review, finding IN-01"
created: 2026-11-10
theme: cpu
bound_step: "phase 3, plan 06"
---

## Goal

The interrupt entry's read of the interrupted instruction no longer runs `schway__bus_unbuilt` (which bumps
`unbuilt_n` and overwrites `last_unbuilt`) and no longer ignores the I-cache line the CPU actually executed.

## Evidence

The review finding named `src/cpu_exec.inc` (the interrupt entry re-fetched the word at EPC from the bus). It also
said the GTE plan would execute that word, so the fix had to land with it.

## Resolution

Completed by phase 3, plan 06. `cpu_irq_enter` no longer fetches from the bus: `cpu_peek_word` reads the
instruction at the program counter (the delay slot's instruction, not the branch EPC names) from the cache line when
it holds it, else RAM, else the BIOS image, else 0, with no fault, count or cost. The tests
`cpu.exception.irq_entry_peek` and `cpu.exception.irq_gte_delay_slot` pin it; the delay-slot test fails if the peek
is pointed back at EPC.
