---
title: "Add the weekly digest workflow"
status: pending
priority: P2
resolves_phase: 7
source: "phase 01, plan 01-15 (a seed gate deferred under D-25 and AGENTS rule 7)"
created: 2026-10-20
theme: ci
audit_acknowledged:
  milestone: v1.0
  at: 2026-11-18
---

## Goal

A scheduled `digest.yml` workflow creates a sanitized weekly artifact summarising open project issues and recent CI status. It does not post issues, comments or messages.

## Context

- Seed: none; write the minimum new code.
- Bound to the v1.0 milestone tune-up (SEED-007): no seed carries the file; SEED-007 lists the weekly triage digest as tune-up work.
- Deferred in phase 1 because no phase 1 requirement names it (01-CONTEXT D-24 and D-25, AGENTS rule 7).
- Reviewed at phase 3: the binding above is unchanged.

## Acceptance Criteria

- The workflow runs on its schedule and produces the read-only artifact with a test that `cmake --workflow --preset ci` or `cmake --workflow --preset hygiene` runs.
- The workflow has no write permission for issues or pull requests, sends no messages and sanitizes untrusted issue/run text before it enters the artifact.
- It names the fault it prevents, and no planning ID appears in the code it adds.
