---
title: "Add the Dependabot auto-merge workflow"
status: pending
priority: P2
source: "phase 01, plan 01-15 (a seed gate deferred under D-25 and AGENTS rule 7)"
created: 2026-10-20
theme: ci
---

## Goal

`.github/workflows/dependabot-automerge.yml` arms auto-merge for Dependabot patch and minor updates only, never for majors or for a branch a person pushed to.

## Context

- Seed: `.github/workflows/dependabot-automerge.yml` in szTheory/muhtari at d698364b180df42894ef6737179d4ed9fcc97817 (fetch it with `gh api`, never from a local checkout).
- Bound to the v1.0 milestone tune-up (SEED-007): no phase adds a caller; it needs todo 010 and the release App, and answers the DECISIONS open question on `vars` in Dependabot runs.
- Deferred in phase 1 because no phase 1 requirement names it (01-CONTEXT D-24 and D-25, AGENTS rule 7).

## Acceptance Criteria

- The check or file lands in the v1.0 milestone tune-up (SEED-007), in the plan named above, with a test that `cmake --workflow --preset ci` or `cmake --workflow --preset hygiene` runs.
- It names the fault it prevents, and no planning ID appears in the code it adds.
