---
title: "A sync-mode-3 rewrite of a busy channel is defined and tested"
status: completed
priority: P3
source: "phase 03 code review, finding IN-02"
created: 2026-11-10
theme: dma
bound_step: "phase 4, the first plan that edits the CHCR write path in src/dma.c"
completed: 2026-11-10
---

## Goal

A sync-mode-3 rewrite of a busy channel is defined and tested.

## Context

- Review finding: Sync mode 3 on a non-OTC channel is a first fault only on an idle channel; rewriting it onto a busy channel runs it as sync mode 0. The hardware behaviour of a CHCR write while busy is unverified, so decide with a source or document the choice.
- Bound to: phase 4, the first plan that edits the CHCR write path in src/dma.c.

## Acceptance Criteria

- A `dma.*` test pins the chosen behaviour and `docs/hardware/dma.md` states it with its provenance label.
- The test runs in `cmake --workflow --preset ci`, and no planning ID appears in the code it adds.
