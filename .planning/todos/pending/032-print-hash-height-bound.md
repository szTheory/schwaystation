---
title: "Bound the video height in the runner's hash printer"
status: pending
priority: P3
source: "phase 02 code review, finding IN-02"
created: 2026-11-10
theme: runner
bound_step: "phase 4, the first plan that makes the display range guest-controlled (the GPU display-mode plan)"
---

## Goal

`print_hash` in `runner/main.c` rejects a video height above 512, as `write_png` does, so a display range with
y2 less than y1 cannot make the hash walk past the video memory.

## Context

- Review finding: `schway_get_video` computes `height = y2 - y1` as an unsigned difference in `src/frame.c`, which
  wraps if the display range is reprogrammed with y2 below y1. The display is a constant today, so it is
  unreachable; the GPU makes it guest-controlled.
- The fix is `|| v.height > 512u` in the check, or one validator shared with `write_png`.
- Bound to phase 4: the first phase-4 plan that lets a guest program the display range (the GPU display-mode plan,
  GAME-01; name the plan here when phase 4 is planned).

## Acceptance Criteria

- A test drives a display range with y2 below y1 and sees the hash path refuse it instead of reading past the
  video memory, and `write_png` and `print_hash` agree on the bound.
- The test runs in `cmake --workflow --preset ci`, and no planning ID appears in the code it adds.
