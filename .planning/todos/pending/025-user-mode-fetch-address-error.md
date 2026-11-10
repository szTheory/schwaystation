---
title: "Raise an address error for a user-mode instruction fetch outside KUSEG"
status: pending
priority: P2
source: "phase 02 code review, finding WR-01"
created: 2026-11-10
theme: cpu
bound_step: "phase 4, the first plan that edits src/cpu_exec.inc"
---

## Goal

With SR.KUc set, an instruction fetch from KSEG0, KSEG1 or the BIOS window raises AdEL with BadVaddr equal to the
program counter, as the data path already does in `cpu_data_check`.

## Context

- Review finding: `cpu_step` checks only that the program counter is word aligned before `cpu_fetch`, and
  `cpu_fetch` decodes the segment for caching but never reads SR.KUc, so user-mode code can run kernel-segment code.
- The vector harness forces SR to 0, so it cannot catch this, and `docs/hardware/cpu.md` does not mention the fetch
  case.
- A handler may take the exception, so it records no first fault, as for the data case.
- Bound to phase 4: the first phase-4 plan that edits `src/cpu_exec.inc` (phase 4 is unplanned; name the plan here
  when it is written). Phase 4 is where programs first run user-mode code under a BIOS, which is the first caller
  that can reach the gap.

## Acceptance Criteria

- A `cpu.exception.*` test enters user mode, jumps to a KSEG0 address and a KSEG1 address, and sees AdEL with
  BadVaddr equal to the program counter and no first fault recorded.
- `docs/hardware/cpu.md` states the rule under Instruction fetch.
- The test runs in `cmake --workflow --preset ci`, and no planning ID appears in the code it adds.
