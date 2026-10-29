---
title: "Port the Dependabot auto-merge decision check"
status: pending
priority: P2
source: "phase 01, plan 01-15 (a seed gate deferred under D-25 and AGENTS rule 7)"
created: 2026-10-20
theme: ci
---

## Goal

A policy test runs the auto-merge workflow's decide step's jq program on synthetic pull request events and fails when a major, or a person's push to a Dependabot branch, would merge unattended, or a patch never would.

## Context

- Seed: `tests/cmake/dependabot.cmake` in szTheory/muhtari at d698364b180df42894ef6737179d4ed9fcc97817 (fetch it with `gh api`, never from a local checkout).
- Bound to the v1.0 milestone tune-up (SEED-007): no phase adds a caller; it lands with todo 011.
- Deferred in phase 1 because no phase 1 requirement names it (01-CONTEXT D-24 and D-25, AGENTS rule 7).

## Acceptance Criteria

- The check or file lands in the v1.0 milestone tune-up (SEED-007), in the plan named above, with a test that `cmake --workflow --preset ci` or `cmake --workflow --preset hygiene` runs.
- Port it from the seed with its notice kept as PROVENANCE.md describes, trimmed to a usage line and a "Fault prevented" line, with a `<name>.self_test` negative control through `expect_fail.cmake`, inside the phase's `policy.gate_budget`.
- It names the fault it prevents, and no planning ID appears in the code it adds.
