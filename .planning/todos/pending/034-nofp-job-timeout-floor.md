---
title: "Give the nofp job the same timeout floor as the other required jobs"
status: pending
priority: P3
source: "phase 02 code review, finding IN-04"
created: 2026-11-10
theme: ci
bound_step: "phase 4, the first plan that edits .github/workflows/ci.yml"
---

## Goal

The `nofp` job's `timeout-minutes` is at least 3 (the other jobs keep a 5-minute floor with a stated reason), so a
slow checkout or image pull cannot fail a required job with no code cause.

## Context

- Review finding: `.github/workflows/ci.yml` gives `nofp` one minute against a measured 28 seconds, derived from
  five samples that do not bound a hosted runner's tail.
- Timeouts are re-derived from measured runs, so this lands either with the first phase-4 plan that edits
  `ci.yml` or with the tune-up that re-derives every timeout (SEED-007), whichever comes first.
- Bound to phase 4: the first phase-4 plan that edits `.github/workflows/ci.yml` (the compatibility ledger and the
  RetroArch session both add jobs); otherwise SEED-007.

## Acceptance Criteria

- `grep -A3 'nofp:' .github/workflows/ci.yml` shows a `timeout-minutes` of at least 3 with the reason beside it, and
  the existing timeout checks in `cmake --workflow --preset ci` still pass.
- No planning ID appears in the code it adds.
