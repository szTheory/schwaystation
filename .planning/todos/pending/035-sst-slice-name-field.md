---
title: "Reject a long file stem in the SST slice tool"
status: pending
priority: P3
source: "phase 02 code review, finding IN-05"
created: 2026-11-10
theme: tooling
bound_step: "SEED-007 (the v1.0 milestone tune-up); a tooling seed, not phase work"
---

## Goal

`tools/sst_slice.py` asserts that each file stem is shorter than 16 characters before packing and checks that the
input has at least the kept number of cases before the loop that would already have raised, so a long stem fails at
its cause instead of shifting every later field.

## Context

- Review finding: `name.encode('ascii').ljust(16, b'\0')` pads but never truncates or rejects, so a stem of 17 or
  more characters shifts the following fields and the harness fails far away with "short file"; the `KEEP <= n`
  assert runs after the loop.
- Maintainer tool, never run by CI; review findings about tooling code become seeds (AGENTS rule 7).
- Bound to SEED-007, the v1.0 milestone tune-up, or to the first plan that regenerates the vector sample with it,
  whichever comes first.

## Acceptance Criteria

- `assert len(name) < 16, name` runs before packing and the `n >= KEEP` check runs ahead of the loop.
- Regenerating the committed sample with the tool still reproduces it byte for byte (the procedure is in
  `docs/maintaining.md`).
