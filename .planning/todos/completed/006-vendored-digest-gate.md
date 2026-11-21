---
title: "Extend the vendored-file digest check (libretro.vendored)"
status: completed
completed: 2026-11-21
closed_by: "10-02"
priority: P2
resolves_phase: 10
source: "phase 01, plan 01-15 (a seed gate deferred under D-25 and AGENTS rule 7)"
created: 2026-10-20
theme: libretro
audit_acknowledged:
  milestone: v1.0
  at: 2026-11-18
---

## Goal

`libretro.vendored` hashes every file PROVENANCE.md section 8 lists and fails when one no longer matches its SHA-256 (a vendored file edited in place).

## Context

- Seed: `tests/cmake/vendored.cmake` in szTheory/cogmaster at f09ac97096acda99888270e4c1e4dc96e12ffdfe (fetch it with `gh api`, never from a local checkout).
- Bound to Phase 10 Plan 10-02 (PBP-01): extend the original two hashes with the three standalone miniz source files, while keeping libchdr's build-tree copy separate.
- Deferred in phase 1 because no phase 1 requirement names it (01-CONTEXT D-24 and D-25, AGENTS rule 7).

## Acceptance Criteria

- The check lands in Phase 10 Plan 10-02 and remains registered as `libretro.vendored` in CI.
- The original libretro and rcheevos digests remain covered; the three exact miniz hashes are checked.
- The self-test independently mutates and restores `rc_hash_disc.c` and `miniz_tinfl.c`, rejecting each with the digest-mismatch diagnostic.
- Port it from the seed with its notice kept as PROVENANCE.md describes, trimmed to a usage line and a "Fault prevented" line, with a `<name>.self_test` negative control through `expect_fail.cmake`, inside the phase's `policy.gate_budget`.
- It names the fault it prevents, and no planning ID appears in the code it adds.

## Completion

Completed by Phase 10 Plan 10-02. The existing check retains the libretro and rcheevos hashes and adds all three pinned standalone miniz files. The self-test passed with separate digest-mismatch failures for the restored rcheevos source and miniz inflater source; the full MIT grant and source hashes are present in the third-party notice and provenance.
