---
title: "Select the 40-clock CD-ROM DMA cost from the CD-ROM delay setting"
status: pending
priority: P3
source: "phase 03 verification, advisory note"
created: 2026-11-10
theme: dma
bound_step: "phase 6, the first plan that builds the CD-ROM device end of DMA channel 3"
---

## Goal

DMA channel 3 costs 24 clocks per word with the BIOS default delay setting and 40 with the setting games use; the
CD-ROM device end chooses between them from its delay register.

## Context

- Phase 3 pins channel 3 at 24 and says in `docs/hardware/dma.md` and the GTE-02 text that 40 is selected by the
  CD-ROM device end in phase 6, because the delay setting does not exist until that device is built.
- Source for both figures: psx-spx "DMA Transfer Rates" (the pin used by `docs/hardware/dma.md`).

## Acceptance Criteria

- A `dma.costs.*` test asserts 24 and 40 for channel 3 from the two delay settings, and the DMA page drops the
  deferral sentence.
- The test runs in `cmake --workflow --preset ci`, and no planning ID appears in the code it adds.
