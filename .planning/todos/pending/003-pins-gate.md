---
title: "Port the pins file check (policy.pins)"
status: pending
priority: P2
source: "phase 01, plan 01-15 (a seed gate deferred under D-25 and AGENTS rule 7)"
created: 2026-10-20
theme: policy
---

## Goal

`policy.pins` fails a `pins.txt` row that is not exactly `name version url sha256` with a unique name, an https URL and a 64-digit lower-case SHA-256.

## Context

- Seed: `tests/cmake/pins.cmake` in szTheory/cogmaster at f09ac97096acda99888270e4c1e4dc96e12ffdfe (fetch it with `gh api`, never from a local checkout).
- Rebound at phase 3: bound to the first plan, in phase 4 or later, that adds a row to `pins.txt` after the armips row (phase 4's OpenBIOS build from nugget is the first planned one); otherwise SEED-007, the milestone tune-up.
- Deferred in phase 1 because no phase 1 requirement names it (01-CONTEXT D-24 and D-25, AGENTS rule 7).

## Acceptance Criteria

- The check or file lands in the plan named above, with a test that `cmake --workflow --preset ci` or `cmake --workflow --preset hygiene` runs.
- Port it from the seed with its notice kept as PROVENANCE.md describes, trimmed to a usage line and a "Fault prevented" line, with a `<name>.self_test` negative control through `expect_fail.cmake`, inside the phase's `policy.gate_budget`.
- It names the fault it prevents, and no planning ID appears in the code it adds.
