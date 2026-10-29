---
title: "Port the vendored-file digest check (libretro.vendored)"
status: pending
priority: P2
source: "phase 01, plan 01-15 (a seed gate deferred under D-25 and AGENTS rule 7)"
created: 2026-10-20
theme: libretro
---

## Goal

`libretro.vendored` hashes every file PROVENANCE.md section 8 lists and fails when one no longer matches its SHA-256 (a vendored file edited in place).

## Context

- Seed: `tests/cmake/vendored.cmake` in szTheory/cogmaster at f09ac97096acda99888270e4c1e4dc96e12ffdfe (fetch it with `gh api`, never from a local checkout).
- Bound to phase 6: the phase 6 plan that vendors rcheevos `rc_hash_disc.c` (DISC-02), the second vendored file after `libretro/libretro.h`.
- Deferred in phase 1 because no phase 1 requirement names it (01-CONTEXT D-24 and D-25, AGENTS rule 7).

## Acceptance Criteria

- The check or file lands in phase 6, in the plan named above, with a test that `cmake --workflow --preset ci` or `cmake --workflow --preset hygiene` runs.
- Port it from the seed with its notice kept as PROVENANCE.md describes, trimmed to a usage line and a "Fault prevented" line, with a `<name>.self_test` negative control through `expect_fail.cmake`, inside the phase's `policy.gate_budget`.
- It names the fault it prevents, and no planning ID appears in the code it adds.
