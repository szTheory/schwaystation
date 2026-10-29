---
status: pending
priority: P2
phase: 02
plan: 02-01
source: 01-REVIEW.md WR-01
---

## Goal

When phase 2 plan 1 next changes the test helper or adds its next caller,
require exit status zero alongside a tolerant output regex. Use the existing
status-and-regex wrapper instead of CTest's bare PASS_REGULAR_EXPRESSION,
which can override an ordinary nonzero child exit.

## Evidence and scope

Phase 1 review reproduced this test-helper behavior; its fix report retained
it as a tooling seed under AGENTS rule 7. It adds no phase-1 requirement and
does not reopen the completed core review. Keep the regression with the first
caller in that plan, and preserve the per-directory inventories.
