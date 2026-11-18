---
id: SEED-261116-osn
status: dormant
planted: 2026-11-16
planted_during: milestone v1.0, Phase 6
trigger_when: when the MDEC ordered-status reference is next used to claim exact event matching
scope: unknown
audit_acknowledged:
  milestone: v1.0
  at: 2026-11-18
  status: dormant
---

# SEED-261116-osn: Preserve full MDEC status oracle records

## Why This Matters

The current MDEC status comparator collapses consecutive event kinds and accepts a minimum status-read
count. It can therefore accept a trace with missing or extra status reads, or changed status/data payloads,
as long as the remaining event-kind transitions match. The current pinned guest still fails its bounded run,
so this weakness does not hide the active blocker; it matters when a later implementation reaches the Done
marker and the selector is used to claim exact ordered-record matching.

## When to Surface

**Trigger:** when the MDEC ordered-status reference is next used to claim exact event matching.

## Scope Estimate

**Unknown** — trace one parsed record per status/input/output event, compare exact event counts and order, and
use guest bus traces for payloads hidden by TTY placeholders.

## Breadcrumbs

- `tests/mdec/reference.c`: `collect_status_events` and `status_order_matches`.
- `.planning/phases/06-discs-and-fmv/06-REVIEW.md`: WR-01 review finding.

## Notes

Captured from Plan 06-39's code review. Keep this as deferred test-tooling work per the project review policy;
do not treat it as a DISC-03 implementation requirement by itself.
