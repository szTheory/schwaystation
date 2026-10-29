---
title: "Port the firmware scan (release.firmware_scan)"
status: pending
priority: P2
source: "phase 01, plan 01-15 (a seed gate deferred under D-25 and AGENTS rule 7)"
created: 2026-10-20
theme: release
---

## Goal

`release.firmware_scan` hashes every member of every release archive, and every tracked file, and fails when one matches a retail BIOS digest from the vendored `System.dat` table (ASSET_POLICY.md).

## Context

- Seed: `tests/cmake/firmware_scan.cmake` in szTheory/muhtari at d698364b180df42894ef6737179d4ed9fcc97817 (fetch it with `gh api`, never from a local checkout).
- Bound to phase 4: the phase 4 plan that builds OpenBIOS and vendors the `System.dat` digest table (GAME-05).
- Deferred in phase 1 because no phase 1 requirement names it (01-CONTEXT D-24 and D-25, AGENTS rule 7).

## Acceptance Criteria

- The check or file lands in phase 4, in the plan named above, with a test that `cmake --workflow --preset ci` or `cmake --workflow --preset hygiene` runs.
- Port it from the seed with its notice kept as PROVENANCE.md describes, trimmed to a usage line and a "Fault prevented" line, with a `<name>.self_test` negative control through `expect_fail.cmake`, inside the phase's `policy.gate_budget`.
- It names the fault it prevents, and no planning ID appears in the code it adds.
