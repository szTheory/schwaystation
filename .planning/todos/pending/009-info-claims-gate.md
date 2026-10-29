---
title: "Port the libretro .info claims check (libretro.info_claims)"
status: pending
priority: P2
source: "phase 01, plan 01-15 (a seed gate deferred under D-25 and AGENTS rule 7)"
created: 2026-10-20
theme: libretro
---

## Goal

`libretro.info_claims` maps every `.info` key set to `"true"` to a test in the inventory and fails a claim no test exercises.

## Context

- Seed: `tests/cmake/info_claims.cmake` in szTheory/cogmaster at f09ac97096acda99888270e4c1e4dc96e12ffdfe (fetch it with `gh api`, never from a local checkout).
- Bound to phase 4: the phase 4 plan that sets `supports_no_game`, `input_descriptors`, `core_options`, `memory_descriptors` and `disk_control` in the `.info` (GAME-07).
- Deferred in phase 1 because no phase 1 requirement names it (01-CONTEXT D-24 and D-25, AGENTS rule 7).

## Acceptance Criteria

- The check or file lands in phase 4, in the plan named above, with a test that `cmake --workflow --preset ci` or `cmake --workflow --preset hygiene` runs.
- Port it from the seed with its notice kept as PROVENANCE.md describes, trimmed to a usage line and a "Fault prevented" line, with a `<name>.self_test` negative control through `expect_fail.cmake`, inside the phase's `policy.gate_budget`.
- It names the fault it prevents, and no planning ID appears in the code it adds.
