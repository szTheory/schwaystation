---
title: "Add the rehearsal and cleanup rows to the CI table in docs/testing.md"
status: completed
completed: 2026-11-09
closed_by: "02-03"
priority: P2
source: "phase 01, plan 01-15"
created: 2026-10-20
theme: docs
---

## Goal

The CI job table in `docs/testing.md` names every job in `ci.yml`, including `rehearsal` and `cleanup`.

## Context

- Noted in plan 01-13 and carried by 01-14 and 01-15: the table lists every job but `rehearsal` and `cleanup`.
- Bound to phase 1's code-review fix round, or else the first phase 2 plan that edits `docs/testing.md`.

## Acceptance Criteria

- `grep -c '^| `rehearsal`' docs/testing.md` and `grep -c '^| `cleanup`' docs/testing.md` each print at least 1.

## Completion

Completed by plan 02-03. docs/testing.md names the rehearsal and cleanup jobs in its CI table. Plan 02-27 moved this file from pending to completed.
