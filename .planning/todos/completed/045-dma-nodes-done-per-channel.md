---
title: "Count list nodes per channel"
status: completed
priority: P3
source: "phase 03 code review, finding IN-06"
created: 2026-11-10
theme: dma
bound_step: "phase 4, the first plan that lets a second channel walk a list (the GPU DMA end) or phase 5 (SPU), whichever comes first"
completed: 2026-11-10
---

## Goal

Count list nodes per channel.

## Context

- Review finding: `nodes_done` is one counter shared by all channels, which is correct while one unit runs at a time but couples channels if list walking ever interleaves.
- Bound to: phase 4, the first plan that lets a second channel walk a list (the GPU DMA end) or phase 5 (SPU), whichever comes first.

## Acceptance Criteria

- A test with two list-walking channels shows each yields at its own 1000h nodes, or a comment and the page state why one counter is enough.
- The test runs in `cmake --workflow --preset ci`, and no planning ID appears in the code it adds.
