---
title: "Port the release floor check (release.floor)"
status: pending
priority: P2
source: "phase 01, plan 01-15 (a seed gate deferred under D-25 and AGENTS rule 7)"
created: 2026-10-20
theme: release
---

## Goal

`release.floor` reads each packaged binary's minimum OS or glibc version, its needed libraries and the libretro module's `retro_*` exports, and fails when a released binary would refuse to load on a platform the README lists.

## Context

- Seed: `tests/cmake/floor.cmake` in szTheory/cogmaster at f09ac97096acda99888270e4c1e4dc96e12ffdfe (fetch it with `gh api`, never from a local checkout).
- Bound to phase 2: the phase 2 plan that lands `tests/api/v2.c` (CPU-04), the first addition to the public surface after the first release.
- Deferred in phase 1 because no phase 1 requirement names it (01-CONTEXT D-24 and D-25, AGENTS rule 7).

## Acceptance Criteria

- The check or file lands in phase 2, in the plan named above, with a test that `cmake --workflow --preset ci` or `cmake --workflow --preset hygiene` runs.
- Port it from the seed with its notice kept as PROVENANCE.md describes, trimmed to a usage line and a "Fault prevented" line, with a `<name>.self_test` negative control through `expect_fail.cmake`, inside the phase's `policy.gate_budget`.
- It names the fault it prevents, and no planning ID appears in the code it adds.
