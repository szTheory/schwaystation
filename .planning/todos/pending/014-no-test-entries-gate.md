---
title: "Port the test-only key check"
status: pending
priority: P2
source: "phase 01, plan 01-15 (a seed gate deferred under D-25 and AGENTS rule 7)"
created: 2026-10-20
theme: policy
---

## Goal

A policy test fails when a key from a test-only list (a digest or serial a test invents) also appears in a table the core bundles, where it would change what the probe reports for a real image.

## Context

- Seed: `tests/cmake/no_test_entries.cmake` in szTheory/muhtari at d698364b180df42894ef6737179d4ed9fcc97817 (fetch it with `gh api`, never from a local checkout).
- Bound to phase 2: the first phase 2 plan that adds a bundled lookup table with a test-only key list next to it (the `--probe` fields, CPU-05); if phase 2 adds none, re-bind this todo at phase 2's close to the phase that does.
- Deferred in phase 1 because no phase 1 requirement names it (01-CONTEXT D-24 and D-25, AGENTS rule 7).

## Acceptance Criteria

- The check or file lands in phase 2, in the plan named above, with a test that `cmake --workflow --preset ci` or `cmake --workflow --preset hygiene` runs.
- Port it from the seed with its notice kept as PROVENANCE.md describes, trimmed to a usage line and a "Fault prevented" line, with a `<name>.self_test` negative control through `expect_fail.cmake`, inside the phase's `policy.gate_budget`.
- It names the fault it prevents, and no planning ID appears in the code it adds.
