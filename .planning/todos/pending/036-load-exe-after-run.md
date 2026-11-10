---
title: "Refuse schway_load_exe on an instance that has already run"
status: pending
priority: P3
source: "phase 02 code review, finding IN-06"
created: 2026-11-10
theme: api
bound_step: "phase 4, the first plan that edits src/exe.c (the adapter's PS-EXE content path)"
---

## Goal

`schway_load_exe` returns `SCHWAY_STATUS_ERR_ARGUMENT` when `inst->ticks` is non-zero or an executable is already
loaded, not only when `frame` is non-zero, and `docs/embedding.md` says so.

## Context

- Review finding: `src/exe.c` guards on `frame != 0` only, so an instance that ran part of a frame (`schway_run`
  with a small budget) accepts a second load that overwrites text and the launch block under a CPU that already ran
  and leaves the first image's other bytes in RAM. The result is deterministic but neither the new image's boot nor
  the old one's run, and the public header does not specify it.
- Bound to phase 4: the first phase-4 plan that edits `src/exe.c`; the libretro adapter's stub-kernel path for
  PS-EXE content (a phase-4 research flag) is the first host that loads an executable after creating an instance.

## Acceptance Criteria

- A `core.exe.*` or `exe.*` test loads an executable, runs fewer cycles than a frame, loads again and sees the
  argument error; a load on a fresh instance still succeeds.
- `docs/embedding.md` states the rule.
- The test runs in `cmake --workflow --preset ci`, and no planning ID appears in the code it adds.
