---
title: "Add the weekly digest workflow"
status: pending
priority: P2
source: "phase 01, plan 01-15 (a seed gate deferred under D-25 and AGENTS rule 7)"
created: 2026-10-20
theme: ci
---

## Goal

A scheduled `digest.yml` workflow writes one weekly issue summarising CI and nightly results, open nightly issues and pending Dependabot pull requests.

## Context

- Seed: none; write the minimum new code.
- Bound to the v1.0 milestone tune-up (SEED-007): no seed carries the file; SEED-007 lists the weekly triage digest as tune-up work.
- Deferred in phase 1 because no phase 1 requirement names it (01-CONTEXT D-24 and D-25, AGENTS rule 7).

## Acceptance Criteria

- The check or file lands in the v1.0 milestone tune-up (SEED-007), in the plan named above, with a test that `cmake --workflow --preset ci` or `cmake --workflow --preset hygiene` runs.
- It names the fault it prevents, and no planning ID appears in the code it adds.
