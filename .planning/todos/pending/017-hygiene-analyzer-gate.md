---
title: "Add the static analyzer check (hygiene.analyzer)"
status: pending
priority: P2
source: "phase 01, plan 01-15 (a seed gate deferred under D-25 and AGENTS rule 7)"
created: 2026-10-20
theme: hygiene
---

## Goal

`hygiene.analyzer` runs a pinned static analyzer over the core and fails on any finding not on a reviewed allowlist.

## Context

- Seed: none; write the minimum new code.
- Bound to phase 2: the first phase 2 plan that adds a large C module (the CPU core).
- Deferred in phase 1 because no phase 1 requirement names it (01-CONTEXT D-24 and D-25, AGENTS rule 7).

## Acceptance Criteria

- The check or file lands in phase 2, in the plan named above, with a test that `cmake --workflow --preset ci` or `cmake --workflow --preset hygiene` runs.
- It names the fault it prevents, and no planning ID appears in the code it adds.
