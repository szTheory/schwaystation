---
title: "Evaluate BE32 shards for long libretro host tests"
status: pending
priority: P2
resolves_phase: 16
source: "Phase 14 and Phase 15 final Main CI runs 45442543076 and 45685800743; CI efficiency review"
created: 2026-11-24
theme: ci
---

## Goal

Shorten the BE32 Main CI critical path while preserving every registered portable test, assertion, timeout, fixture, and downstream hash artifact.

## Context

On final-head Main CI run [45442543076](https://github.com/szTheory/schwaystation/actions/runs/45442543076), the BE32 job took 64m09s and ran 657 tests. `libretro.input.movies` is marked `RUN_SERIAL`; it took 1,106.63 seconds after the other 656 tests completed. A dedicated movie-test job is estimated to save about 18 minutes from the critical path, with roughly 115 seconds of repeated setup/build. The estimate is based on observed runtimes and is not a billing estimate: the timing API reported zero billable milliseconds.

On the final Phase 15 Main CI run [45685800743](https://github.com/szTheory/schwaystation/actions/runs/45685800743), the BE32 job ran 669 tests for 4,150.08 seconds. `libretro.host` ran for 1,456.53 seconds and `libretro.input.movies` for 1,041.84 seconds after the other tests completed. Evaluate both long tests together; the movie-only estimate above is incomplete.

Phase 16 may implement a small shard only after the feature and test scope is stable and the exact selectors, full test/JUnit union, fixture outputs, and workflow routing are proven. Keep the existing full CI gates if local evidence cannot show a complete contract and a measured gain after repeated setup. A larger implementation remains a separate follow-up.

## Acceptance Criteria

- The broad BE32 shard and dedicated movie-test shard use private, same-configuration build and fixture workspaces.
- The exact shard selectors are disjoint, cover the registered portable test inventory, and run every test exactly once. A cheap preflight proves the set before the long job runs.
- The JUnit validator proves the union of shard reports represents the complete test inventory; it does not rely on one report or tolerant total counts alone.
- Existing assertions, timeouts, fixture dependencies, `runner.hashes_file`, the `hashes-be32` artifact and downstream hash-equality checks remain intact.
- Local selector, fixture, JUnit-union and workflow-routing checks pass before hosted CI; `act --dryrun` checks routing only. Hosted Linux/BE32 runner and QEMU behavior remain required hosted evidence.
- Compare the before/after exact-head Main CI critical path. Keep the split only if all required tests, artifacts, hashes and quality bounds pass and measured wall time improves.

## Phase 16 binding

Evaluate this after the phase's feature and test scope is stable. Do not block patch, cheat, or compatibility implementation to pursue an unmeasured CI redesign. Preserve every gate if the split cannot be validated with a small, exact shard contract.
