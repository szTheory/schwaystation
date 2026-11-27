---
title: "Evaluate BE32 shards for long libretro host tests"
status: pending
priority: P2
resolves_phase: 16
source: "Phase 14 and Phase 15 final Main CI runs 45442543076 and 45685800743; CI efficiency review"
created: 2026-11-24
theme: ci
audit_acknowledged:
  milestone: v2.0
  at: 2026-11-26
---

## Goal

Shorten the BE32 Main CI critical path while preserving every registered portable test, assertion, timeout, fixture, and downstream hash artifact.

## Context

On final-head Main CI run [45442543076](https://github.com/szTheory/schwaystation/actions/runs/45442543076), the BE32 job took 64m09s and ran 657 tests. `libretro.input.movies` is marked `RUN_SERIAL`; it took 1,106.63 seconds after the other 656 tests completed. A dedicated movie-test job is estimated to save about 18 minutes from the critical path, with roughly 115 seconds of repeated setup/build. The estimate is based on observed runtimes and is not a billing estimate: the timing API reported zero billable milliseconds.

On the final Phase 15 Main CI run [45685800743](https://github.com/szTheory/schwaystation/actions/runs/45685800743), the BE32 job ran 669 tests for 4,150.08 seconds. `libretro.host` ran for 1,456.53 seconds and `libretro.input.movies` for 1,041.84 seconds after the other tests completed. Evaluate both long tests together; the movie-only estimate above is incomplete.

Phase 16 may implement a small shard only after the feature and test scope is stable and the exact selectors, full test/JUnit union, fixture outputs, and workflow routing are proven. Keep the existing full CI gates if local evidence cannot show a complete contract and a measured gain after repeated setup. A larger implementation remains a separate follow-up.

## Acceptance Criteria

- The broad BE32 shard and dedicated audio and movie shards use private, same-configuration build and fixture workspaces.
- The exact shard selectors are disjoint, cover the registered portable test inventory, and run every test exactly once. A cheap preflight proves the set before the long job runs.
- The JUnit validator proves the union of shard reports represents the complete test inventory; it does not rely on one report or tolerant total counts alone.
- Existing assertions, timeouts, fixture dependencies, `runner.hashes_file`, the `hashes-be32` artifact and downstream hash-equality checks remain intact.
- Local selector, fixture, JUnit-union and workflow-routing checks pass before hosted CI; `act --dryrun` checks routing only. Hosted Linux/BE32 runner and QEMU behavior remain required hosted evidence.
- Compare the before/after exact-head Main CI critical path. Keep the split only if all required tests, artifacts, hashes and quality bounds pass and measured wall time improves.

## Phase 16 evidence

[Phase 15 final PR](https://github.com/szTheory/schwaystation/actions/runs/45685800743) and its
[post-merge Main CI](https://github.com/szTheory/schwaystation/actions/runs/45697774644) each ran all 669 portable
tests. The latest release metadata run skipped BE32 and supplies no comparison.
`gh run view <run> --json status,conclusion,url,jobs` and the job-log API for both BE32 jobs exited 0.

| Run | BE32 job wall | CTest wall | Audio host test | Movie test |
| --- | ---: | ---: | ---: | ---: |
| Phase 15 PR | 72m 24s | 4,150.08s | 1,456.53s | 1,041.84s |
| Post-merge Main CI | 72m 40s | 4,248.00s | 1,569.57s | 1,046.08s |

The two tests were the final completions on both runs. After the last other test finished, the remaining tail
took 30m 38s on the PR and 32m 33s after merge. This is observed wall time. Repeating the complete existing
pre-build setup in two extra jobs would add about 158–316 runner-seconds across both jobs before compilation,
estimated from 79–158 seconds of setup on each existing job. Queueing, isolated compilation and fixture reuse
would change the outcome. Billing for the proposed jobs has not been measured.

A read-only comparison of four `ctest --preset ci --show-only=json-v1 -L '^portable$'` enumerations exited 0.
The current CI inventory has 694 unique portable tests. The exact audio selector `^libretro[.]host$`, movie
selector `^libretro[.]input[.]movies$`, and broad exclusion `^libretro[.](host|input[.]movies)$` form a disjoint
union of 1, 1 and 692 tests. The local graph matches the declared CI inventory. The hosted BE32 runtime and
JUnit union remain unmeasured after sharding.

The audio shard would build the existing `spu_tone.exe` fixture, runner test executable and libretro host/module.
The movie shard would build `pad_counter.exe` and the same host/module. Neither CTest test has an inter-test
fixture dependency. The broad shard retains `runner.hashes_file`, its OpenBIOS and disc fixture dependencies,
and the existing `hashes-be32` artifact consumed by hash equality. All three jobs would use the same CHD-on
PowerPC configuration and individual build paths.

## Phase 16 binding

Evaluate this after the phase's feature and test scope is stable. Do not block patch, cheat, or compatibility implementation to pursue an unmeasured CI redesign. Preserve every gate if the split cannot be validated with a small, exact shard contract.

Phase 16 leaves the production job intact. The next workflow change that splits BE32 owns the exact hosted
CTest/JUnit union and a comparable before/after run, including the duplicated build cost. Keep this todo pending
until that evidence is available.
