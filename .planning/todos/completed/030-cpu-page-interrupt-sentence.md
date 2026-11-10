---
title: "Correct the CPU page's sentence about interrupts"
status: complete
priority: P3
source: "phase 02 code review, finding WR-06"
created: 2026-11-10
theme: docs
bound_step: "phase/03, plan 21"
---

## Goal

`docs/hardware/cpu.md` ("The end of a frame") said the CPU does not take interrupts yet, which contradicted
`docs/hardware/cop0.md` (Interrupts), the retirement stamp in the run loop and the `cpu.exception.*` tests.

## Evidence

The review finding named `docs/hardware/cpu.md:320-321`. Read-only reproduction before the fix:
`grep -q 'does not take interrupts' docs/hardware/cpu.md` exited 0; after it the same command exits 1.

## Resolution

Completed by phase 3, plan 21. The sentence now says the CPU takes interrupts from the vertical blank, the DMA and the
timers, that each raises its own `I_STAT` bit, and that the COP0 page's Interrupts section describes how the CPU takes
one. The page's other preparation-file citations were re-pointed to psx-spx in the same plan.
