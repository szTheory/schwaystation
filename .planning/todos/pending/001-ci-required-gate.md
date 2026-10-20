---
title: "Port the CI required roll-up check (policy.ci_required)"
status: pending
priority: P2
source: "phase 01, plan 01-15 (a seed gate deferred under D-25 and AGENTS rule 7)"
created: 2026-10-20
theme: ci
---

## Goal

`policy.ci_required` runs the jq program of `ci.yml`'s `CI required` step on synthetic `needs` and fails when a failed or wrongly skipped job passes, or a job is missing from its `needs`.

## Context

- Seed: `tests/cmake/ci_required.cmake` in szTheory/cogmaster at f09ac97096acda99888270e4c1e4dc96e12ffdfe (fetch it with `gh api`, never from a local checkout).
- Bound to phase 2: the first phase 2 plan that adds a job to `ci.yml` (the armips build on every leg, CPU-04), which is the first edit that can drop a job from `CI required`'s `needs`.
- Deferred in phase 1 because no phase 1 requirement names it (01-CONTEXT D-24 and D-25, AGENTS rule 7).

## Acceptance Criteria

- The check or file lands in phase 2, in the plan named above, with a test that `cmake --workflow --preset ci` or `cmake --workflow --preset hygiene` runs.
- Port it from the seed with its notice kept as PROVENANCE.md describes, trimmed to a usage line and a "Fault prevented" line, with a `<name>.self_test` negative control through `expect_fail.cmake`, inside the phase's `policy.gate_budget`.
- It names the fault it prevents, and no planning ID appears in the code it adds.
