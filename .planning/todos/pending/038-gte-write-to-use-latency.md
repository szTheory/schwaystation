---
title: "Model the two-instruction write-to-use latency of MTC2, CTC2 and LWC2"
status: pending
priority: P3
source: "phase 03 discussion, decision on GTE stall rules (a deviation)"
created: 2026-11-10
theme: gte
bound_step: "SEED-003 (measured accuracy), the nearest seed"
---

## Goal

On hardware a value written by MTC2, CTC2 or LWC2 is not usable by an instruction that follows it within two
instructions; the core applies the write at once. Model the latency, or keep it as a documented deviation with its
evidence.

## Context

- Phase 3 models the stall that follows a GTE command and applies it to MFC2, CFC2, SWC2 and a new command, not to
  MTC2, CTC2 or LWC2 (nugget `tests/gte-timing/RESULTS.md` at 9db6d88d23c4, MIT, run on hardware).
- The write-to-use latency is a separate measurement. Evidence: nugget `tests/gte-write-to-use/RESULTS.md` at
  9db6d88d23c4.
- `docs/hardware/gte.md` says a move into the engine does not wait but does not yet name this latency; no test pins
  the behaviour either way.
- Bound to SEED-003 (measured accuracy), the nearest seed: no roadmap phase plans hardware-measured CPU timing.
  Re-bind it to a phase plan if one starts to model CPU or GTE timing from hardware runs.

## Acceptance Criteria

- Either the latency is modelled with a test that reads a GTE register one and two instructions after a write, or
  `docs/hardware/gte.md` names the deviation with the evidence above.
- The test runs in `cmake --workflow --preset ci`, and no planning ID appears in the code it adds.
