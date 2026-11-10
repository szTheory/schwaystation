---
title: "Port the single version source check (release.single_version_source)"
status: pending
priority: P2
source: "phase 01, plan 01-15 (a seed gate deferred under D-25 and AGENTS rule 7)"
created: 2026-10-20
theme: release
---

## Goal

`release.single_version_source` fails naming every tracked file in scope that writes the version by hand outside `version.txt`, so release-please never leaves a stale copy; `NEXT=ON` also checks the next minor and patch.

## Context

- Seed: `tests/cmake/single_version.cmake` in szTheory/cogmaster at f09ac97096acda99888270e4c1e4dc96e12ffdfe (fetch it with `gh api`, never from a local checkout).
- Rebound at phase 3: bound to SEED-007, the milestone tune-up after v1.0, or to the first plan that writes the version by hand outside `version.txt`, whichever comes first.
- Deferred in phase 1 because no phase 1 requirement names it (01-CONTEXT D-24 and D-25, AGENTS rule 7).

## Acceptance Criteria

- The check or file lands in the plan named above, with a test that `cmake --workflow --preset ci` or `cmake --workflow --preset hygiene` runs.
- Port it from the seed with its notice kept as PROVENANCE.md describes, trimmed to a usage line and a "Fault prevented" line, with a `<name>.self_test` negative control through `expect_fail.cmake`, inside the phase's `policy.gate_budget`.
- It names the fault it prevents, and no planning ID appears in the code it adds.
