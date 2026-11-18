---
id: SEED-261116-mrh
status: dormant
planted: 2026-11-16
planted_during: milestone v1.0, Phase 6, Plan 13 review
trigger_when: Plan 06-16 supplies a complete owned STR frame through the MDEC caller
scope: tests/mdec/color_reference.c and tests/disc/fixture.c
audit_acknowledged:
  milestone: v1.0
  at: 2026-11-18
  status: dormant
---

# SEED-261116-mrh: Hash real guest MDEC output before DMA1

## Why This Matters

Plan 13 hashes controlled raw FIFO words before DMA1 and checks their reordered RAM copy, while its real guest
reference harness compares VRAM and native display output without recording that guest's raw MDEC words. The
plan requires these evidence domains to stay separate. A complete frame input is needed before the real-caller
hash can be meaningful.

## When to Surface

**Trigger:** Plan 06-16 adds the first complete owned STR frame caller, or an independently pinned complete MDEC
guest input becomes available sooner.

## Scope Estimate

**Small** — capture a canonical hash of the real caller's FIFO words before DMA1, then retain separate checks for
DMA1 RAM layout and native display interpretation. Do not substitute a self-frozen hash for hardware image
acceptance.

## Breadcrumbs

- `.planning/phases/06-discs-and-fmv/06-13-REVIEW.md`: WR-02.
- `tests/mdec/color_reference.c`: real frame/movie caller harness.
- `.planning/phases/06-discs-and-fmv/06-16-PLAN.md`: first owned STR caller.

This is deferred test-tooling work from code review, not a new DISC-03 requirement.
