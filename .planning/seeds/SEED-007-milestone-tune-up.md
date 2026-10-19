---
id: SEED-007
status: dormant
planted: 2026-10-19
planted_during: preparation, before milestone v1.0
trigger_when: every milestone after the first, as its first phase
scope: small
---

# SEED-007: Each milestone starts with a faster, more trustworthy pipeline and docs that match the release

## Why This Matters

CI time, flakiness, documentation and quality drift a little with every phase, and a PlayStation core's CI is
heavier than its siblings' (eight lanes, built fixture discs, OpenBIOS, nightly oracle runs). One short phase at
the start of each milestone removes what built up and raises the bar the rest of the milestone must hold. This is
where the owner's recurring CI-efficiency audit and "weakest quality dimension" review happen, without being asked.

## When to Surface

**Trigger:** every milestone after the first. Keep this seed after it is used; it applies again next time.

## Scope Estimate

**Small**: one phase whose results are merged changes:
- CI efficiency, from the report: slowest job faster; timeouts re-derived (twice the slowest of the last 40 runs,
  run ID beside it); any flaky test fixed or removed with the stronger signal named; cores used; caching decided by
  measurement (compile share at 30 to 50 k lines of C); lowest-value tests removed only when a stronger test covers
  them; the Intel leg on label `portable` only if the critical path demands it.
- Weakest dimension: score performance, CI/CD, ease of use, architecture, docs/README, upgrade path and
  compatibility breadth 1 to 5 with links; the lowest one gets this phase's main change.
- Ratchet: `quality.json` ceilings (`ir_per_frame.<fixture>`, `size.wasm`, `state.bytes`, `ci.critical_path_s`)
  tightened to measured values; floors raised; device budgets re-checked.
- Settings drift: `scripts/apply-repo-settings.sh --check`, Dependabot majors, runner-image changes, the pins of
  armips, mkpsxiso, wasi-sdk, wasmtime, libchdr and the fetched suites.
- Freshness: docs whose code changed, size budgets, the README install line, `docs/hardware/<chip>.md` graduated
  from the preparation files.
- Direction: refresh `.planning/PRODUCT-ROADMAP.md` and `.planning/preparation/PROMPT-COVERAGE.md`; export lessons.
- v2.0's tune-up first builds the weekly triage digest, the CI-efficiency report, `quality.json` and the
  `policy.lessons_unique` check.

## Breadcrumbs

- `.planning/preparation/ENGINEERING.md` sections 4, 7, 8, 11 and 12: CI design, triage, the ratchet, docs budgets,
  recurring loops.
- `.planning/preparation/DECISIONS.md`: the open questions at its end.

## Notes

The seeds are this project's rolling roadmap: the current milestone is `.planning/ROADMAP.md`; direction beyond it
is `.planning/PRODUCT-ROADMAP.md`; SEED-001 to SEED-006 and SEED-008 hold each horizon's narrative.
