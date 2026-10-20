---
title: "Re-pin policy.gate_budget to the measured gate-line total at phase 1 ship"
status: pending
priority: P2
source: "phase 01, plan 01-15"
created: 2026-10-20
theme: policy
---

## Goal

Lower `policy.gate_budget`'s limit from 1,500 to the measured total of `wc -l scripts/*.sh tests/cmake/*.cmake .githooks/*`, so the budget ratchets down at ship (AGENTS rule 7).

## Context

- Measured at plan 01-15 on the phase branch (commit d8a0ab4): `wc -l scripts/*.sh tests/cmake/*.cmake .githooks/* | tail -1` printed **1392 total** (exit 0).
- The edit is one line in `tests/hygiene/tests.cmake`: `-DLIMIT=1500` becomes `-DLIMIT=<total>`, and the comment above it that names 1500 names the new value.
- Bound to phase 1's `/gsd-ship`, after code review, so review fixes are counted (FRAME-05: "re-pinned to the measured value at ship"). Re-measure then: if code review changed a gate file, the total at ship replaces 1392.

## Acceptance Criteria

- At `/gsd-ship` for phase 1, `wc -l scripts/*.sh tests/cmake/*.cmake .githooks/*` is re-run from the repository root and its total is the new `LIMIT`.
- `ctest --preset hygiene -R '^policy\.gate_budget'` passes with the new limit, and the self test still fails its 10-line fixture against 9.
- The command and its exit status are recorded in the ship notes.
