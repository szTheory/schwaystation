---
title: "Port the notice checks (policy.notices and release.notices)"
status: pending
priority: P2
source: "phase 01, plan 01-15 (a seed gate deferred under D-25 and AGENTS rule 7)"
created: 2026-10-20
theme: policy
---

## Goal

The manifest-driven `policy.notices` landed in phase 3 (every `tests/fixtures/manifest.txt` row with a notice is on an Applies-to line of THIRD-PARTY-NOTICES.md, and every path on such a line exists). What stays pending is the rest of the seed: it checks that every SPDX line in a tracked C file is MIT or an admitted licence, that each file with more than MIT is named in THIRD-PARTY-NOTICES.md, that every named path exists, and `release.notices` that the libretro `.info` licence field lists exactly those licences.

## Context

- Seed: `tests/cmake/notices.cmake` in szTheory/cogmaster at f09ac97096acda99888270e4c1e4dc96e12ffdfe (fetch it with `gh api`, never from a local checkout).
- Bound to the first plan that adds a non-MIT licence or an SPDX-bearing derived file and so needs the SPDX scan and the `.info` check.
- Deferred in phase 1 because no phase 1 requirement names it (01-CONTEXT D-24 and D-25, AGENTS rule 7).

## Acceptance Criteria

- The remaining checks land in the plan named above, with a test that `cmake --workflow --preset ci` or `cmake --workflow --preset hygiene` runs.
- Port it from the seed with its notice kept as PROVENANCE.md describes, trimmed to a usage line and a "Fault prevented" line, with a `<name>.self_test` negative control through `expect_fail.cmake`, inside the phase's `policy.gate_budget`.
- It names the fault it prevents, and no planning ID appears in the code it adds.
