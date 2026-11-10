---
title: "Zero the vector harness case record and reject doubly covered store bytes"
status: pending
priority: P3
source: "phase 02 code review, finding IN-03"
created: 2026-11-10
theme: tooling
bound_step: "SEED-007 (the v1.0 milestone tune-up); a tooling seed, not phase work"
---

## Goal

`tests/vectors/vectors.c` clears each case record before `parse_case`, and `coalesce` returns failure when two store
chunks cover the same byte, so a malformed file cannot make the harness read an uninitialised cycle record and a
doubled write cannot compare equal to a single correct one.

## Context

- Review finding: `struct vcase c` is uninitialised on each iteration, so the rule (b) error report can read
  `c.cycle[1]` that `parse_case` never wrote, and `coalesce` ORs chunk bytes without noticing overlap.
- It is test code: the vectors themselves carry exactly the right chunks, so nothing fails today. Review findings
  about test or tooling code become seeds (AGENTS rule 7).
- Bound to SEED-007, the v1.0 milestone tune-up, or to the first plan that edits `tests/vectors/vectors.c`,
  whichever comes first.

## Acceptance Criteria

- `memset(&c, 0, sizeof c)` runs before each `parse_case`, and `coalesce` fails a case whose chunks cover a byte twice.
- A negative control through the harness shows a doubled store chunk is reported.
- The check runs in `cmake --workflow --preset ci`, and no planning ID appears in the code it adds.
