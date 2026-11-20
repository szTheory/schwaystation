---
title: "Port the notice checks (policy.notices and release.notices)"
status: pending
priority: P2
resolves_phase: 10
source: "phase 01, plan 01-15 (a seed gate deferred under D-25 and AGENTS rule 7)"
created: 2026-10-20
theme: policy
audit_acknowledged:
  milestone: v1.0
  at: 2026-11-18
---

## Goal

The manifest-driven `policy.notices` landed in phase 3 (every `tests/fixtures/manifest.txt` row with a notice is on an Applies-to line of THIRD-PARTY-NOTICES.md, and every path on such a line exists). What stays pending is the generic tracked-source SPDX scan: every SPDX identifier in a tracked C source must be admitted, and each non-MIT source must be named in THIRD-PARTY-NOTICES.md with existing paths. Phase 9 separately records the pinned CHD build-tree license closure and checks the generated `.info` license against the linked provider; those checks do not scan tracked source files.

## Context

- Seed: `tests/cmake/notices.cmake` in szTheory/cogmaster at f09ac97096acda99888270e4c1e4dc96e12ffdfe (fetch it with `gh api`, never from a local checkout).
- Bound to the Phase 10 plan that first adds a tracked SPDX-bearing source; Phase 10 planning will set the exact plan number. Do not add the generic scanner to a phase that only fetches an audited source into the build tree.
- Deferred in phase 1 because no phase 1 requirement names it (01-CONTEXT D-24 and D-25, AGENTS rule 7).

## Acceptance Criteria

- The source-wide SPDX checks land with the first tracked SPDX-bearing source in Phase 10, with a test that `cmake --workflow --preset ci` or `cmake --workflow --preset hygiene` runs.
- Port it from the seed with its notice kept as PROVENANCE.md describes, trimmed to a usage line and a "Fault prevented" line, with a `<name>.self_test` negative control through `expect_fail.cmake`, inside the phase's `policy.gate_budget`.
- Generated `.info` claims continue to be checked against the actual linked source closure by the Phase 9 configure-time check; do not duplicate that work here.
- It names the fault it prevents, and no planning ID appears in the code it adds.
