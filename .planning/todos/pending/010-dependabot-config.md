---
title: "Add the Dependabot configuration"
status: pending
priority: P2
source: "phase 01, plan 01-15 (a seed gate deferred under D-25 and AGENTS rule 7)"
created: 2026-10-20
theme: ci
---

## Goal

`.github/dependabot.yml` opens grouped update pull requests for the pinned GitHub Actions.

## Context

- Seed: `.github/dependabot.yml` in szTheory/muhtari at d698364b180df42894ef6737179d4ed9fcc97817 (fetch it with `gh api`, never from a local checkout).
- Bound to the v1.0 milestone tune-up (SEED-007): no phase adds a caller; the tune-up that opens the milestone after v1.0, together with todos 011 and 012.
- Deferred in phase 1 because no phase 1 requirement names it (01-CONTEXT D-24 and D-25, AGENTS rule 7).

## Acceptance Criteria

- The check or file lands in the v1.0 milestone tune-up (SEED-007), in the plan named above, with a test that `cmake --workflow --preset ci` or `cmake --workflow --preset hygiene` runs.
- It names the fault it prevents, and no planning ID appears in the code it adds.
