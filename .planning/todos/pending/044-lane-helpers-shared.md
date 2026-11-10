---
title: "Share the register-lane helpers"
status: pending
priority: P3
source: "phase 03 code review, finding IN-05"
created: 2026-11-10
theme: cleanup
bound_step: "SEED-007, the tune-up that opens the next milestone"
---

## Goal

Share the register-lane helpers.

## Context

- Review finding: The sub-word register lane helpers are copied in `src/bus.c`, `src/dma.c` and `src/timers.c`.
- Bound to: SEED-007, the tune-up that opens the next milestone.

## Acceptance Criteria

- One definition is used by the three files, with no change in behaviour (all fixture digests and `hashes.txt` unchanged).
- The test runs in `cmake --workflow --preset ci`, and no planning ID appears in the code it adds.
