---
phase: 03
review: 03-REVIEW.md
titles: json
findings:
  - id: CR-01
    severity: critical
    disposition: fixed
    commit: "15ae4d4"
    test: "dma.read.chain_saves"
    title: "The DMA register read path can chain two units, so the core can save a state its own loader refuses"
  - id: WR-01
    severity: warning
    disposition: fixed
    commit: "9c3aac8"
    test: "dma.stall.engines_decay"
    title: "A DMA stall does not decay the multiply and GTE ready stamps"
  - id: IN-01
    severity: info
    disposition: deferred
    todo: "040"
    bound_to: "phase 4, the first plan that edits the DMA part of src/state.c or src/dma.c (the GPU channel's device end)"
    title: "DMA state validation matches the invariants the engine keeps"
  - id: IN-02
    severity: info
    disposition: deferred
    todo: "041"
    bound_to: "phase 4, the first plan that edits the CHCR write path in src/dma.c"
    title: "A sync-mode-3 rewrite of a busy channel is defined and tested"
  - id: IN-03
    severity: info
    disposition: deferred
    todo: "042"
    bound_to: "phase 4, the first plan that edits runner/trace.c or the fault record in src/internal.h"
    title: "The DMA fault record uses addr and opcode like every other fault kind"
  - id: IN-04
    severity: info
    disposition: deferred
    todo: "043"
    bound_to: "phase 4, the first plan that edits src/timers.c (the GPU line feed)"
    title: "Remove vestigial timer and DMA code"
  - id: IN-05
    severity: info
    disposition: deferred
    todo: "044"
    bound_to: "SEED-007, the tune-up that opens the next milestone"
    title: "Share the register-lane helpers"
  - id: IN-06
    severity: info
    disposition: deferred
    todo: "045"
    bound_to: "phase 4, the first plan that lets a second channel walk a list (the GPU DMA end) or phase 5 (SPU), whichever comes first"
    title: "Count list nodes per channel"
  - id: IN-07
    severity: info
    disposition: deferred
    todo: "046"
    bound_to: "SEED-007, the tune-up that opens the next milestone"
    title: "Put the ISC permission text in the src/gte.c header"
---

# Phase 3 review dispositions

CR-01 and WR-01 were fixed in the first and only fix round (`03-REVIEW-FIX.md`); each has a regression test that fails on the unfixed code. The seven Info findings are deferred, one todo each, bound to the first step that edits the file the finding names (rule 7). No finding is open.
