---
id: SEED-261122-vlf
status: dormant
planted: 2026-11-22
planted_during: v2.0 phase 11
trigger_when: when CI quality-control runtime or CI/CD efficiency is next planned
scope: unknown
audit_acknowledged:
  milestone: v2.0
  at: 2026-11-26
  status: dormant
---

# SEED-261122-vlf: Reduce quality-gate test runtime without losing coverage

## Why This Matters

The exact-head local run at 961964a passed all 670 CI tests and 7 release checks. The CI suite took 105.02 seconds wall time; `policy.quality_metrics` was the longest individual test at 65.82 seconds. Its harness repeatedly launches isolated CMake and shell scenarios to exercise production quality and timing checks. This is a measured optimization candidate; every required case, assertion, platform, and bound must remain intact.

## When to Surface

**Trigger:** when CI quality-control runtime or CI/CD efficiency is next planned

Review whether the production-block scenarios can share setup or reduce process startup while retaining every positive and negative control. Measure the critical path before and after; do not rerun a green broad suite for bookkeeping.

## Scope Estimate

**Unknown** — inspect `tests/cmake/quality_controls.cmake` and the production CI timing gate before planning.

## Breadcrumbs

- `tests/policy/tests.cmake`
- `tests/cmake/quality_controls.cmake`
- `tests/cmake/quality.cmake`
- `scripts/ci-critical-path.sh`
- `.planning/AUTONOMOUS.md`

## Notes

The same suite's `libretro.host` case took 59.36 seconds and is substantive guest coverage. Release validation runs 7 distinct package checks after package creation; no duplicate broad suite was observed. No test was skipped or weakened to reduce this run's time.
