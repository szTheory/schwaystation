# Phase 17 CI reliability and repeat-work review

Captured 2026-11-27 from the 40-run cohort in [17-CI-EFFICIENCY.md](17-CI-EFFICIENCY.md). No workflow, timeout, test, inventory, fixture, or assertion was changed in this audit.

## Timeout evidence

The current values below are read from the live workflow files. PR timing samples are not comparable because Actions did not preserve their merge-ref SHA. The two full-route main CI pushes have different selected workflow/classifier/test-control inputs; the two scheduled Nightly runs have 31 selected-input changes. A single full Release call is reported separately as one observation. The exact no-release-created Release route has three matching release-please samples.

| Workflow | Job | Current min | Repeated exact group | Slowest matched duration | Formula / disposition |
| --- | --- | ---: | ---: | ---: | --- |
| ci.yml | changes | 5 | none | — | Retain; no repeated exact CI workload group. |
| ci.yml | gpu-reference | 5 | none | — | Retain; PR executions lack the merge SHA. |
| ci.yml | openbios | 5 | none | — | Retain; no repeated exact CI workload group. |
| ci.yml | hygiene | 3 | none | — | Retain; no repeated exact CI workload group. |
| ci.yml | title | 5 | none | — | Retain; PR executions lack the merge SHA. |
| ci.yml | build matrix | 30 | none | — | Retain; full main runs have different fingerprints. |
| ci.yml | asan | 38 | none | — | Retain; no repeated exact CI workload group. |
| ci.yml | nofp matrix | 3 | none | — | Retain; no repeated exact CI workload group. |
| ci.yml | be32 | 109 | none | — | Retain; no repeated exact CI workload group. |
| ci.yml | hash-equality | 5 | none | — | Retain; no repeated exact CI workload group. |
| ci.yml | wasm32 | 9 | none | — | Retain; no repeated exact CI workload group. |
| ci.yml | retroarch | 5 | none | — | Retain; PR executions lack the merge SHA. |
| ci.yml | perf | 10 | none | — | Retain; no exact timed sample in this cohort. |
| ci.yml | rehearsal | 13 | none | — | Retain; no exact timed sample in this cohort. |
| ci.yml | cleanup | 5 | none | — | Retain; no exact timed sample in this cohort. |
| ci.yml | CI required | 1 | none | — | Retain; no repeated exact CI workload group. |
| nightly.yml | changes | 3 | none | — | Retain; the two schedule fingerprints differ and PR merge SHAs are unavailable. |
| nightly.yml | bench matrix | 3 | none | — | Retain; no repeated exact Nightly workload group. |
| nightly.yml | bench-wasm32 | 5 | none | — | Retain; no repeated exact Nightly workload group. |
| nightly.yml | vectors-full | 5 | none | — | Retain; no repeated exact Nightly workload group. |
| nightly.yml | fuzz | 18 | none | — | Retain; no repeated exact Nightly workload group. |
| nightly.yml | report | 5 | none | — | Retain; no repeated exact Nightly workload group. |
| release.yml | release-please, no release created, version 0.16.0 inputs | 5 | 2 | 11 s | ceil(2 × 11 / 60) = 1 min; existing 5-minute floor keeps 5 min. |
| release.yml | release-please, release created | 5 | 1 | 10 s | One full-release observation; no repeated group. Keep 5 min. |
| release.yml | publish | 13 | 1 | 380 s | One full-release observation; ceil(2 × 380 / 60) = 13 min. Keep 13 min. |
| release.yml | install-line-latest | 5 | 1 | 10 s | One full-release observation; ceil(2 × 10 / 60) = 1 min. Existing 5-minute floor keeps 5 min. |

The repeated exact timeout group is the version 0.16.0 no-release-created release-please job. Its source jobs are [9 s](https://github.com/szTheory/schwaystation/actions/runs/45949889050/job/140343048693) and [11 s](https://github.com/szTheory/schwaystation/actions/runs/46071535475/job/140749369826). The [22 s job in run 45923004092](https://github.com/szTheory/schwaystation/actions/runs/45923004092/job/140257867424) is a separate 0.15.0 release-metadata singleton. The current timeout remains 5 minutes.

For the single full Release route, the reusable CI jobs provide additional observations, not a comparable baseline. The largest observed job was be32 at 4,110 seconds in [run 45923155197](https://github.com/szTheory/schwaystation/actions/runs/45923155197/job/140258731538); the doubled arithmetic is ceil(2 × 4,110 / 60) = 137 minutes, while the current CI value is 109 minutes. It is one exact Release-route sample, not a matching CI cohort, so it does not support a timeout edit. Other single-sample reusable-CI timings are changes 6 s, hygiene 59 s, gpu-reference 21 s, openbios 55 s, slowest build matrix leg macos-x64 724 s, asan 1,284 s, slowest nofp leg 20 s, wasm32 350 s, retroarch 111 s, hash-equality 6 s, and CI required 4 s. Their formulas are diagnostic only; current values remain unchanged. Jobs skipped in that Release call (title, perf, rehearsal, cleanup) have no duration sample.

Timeout sources are [.github/workflows/ci.yml](../../../.github/workflows/ci.yml), [.github/workflows/nightly.yml](../../../.github/workflows/nightly.yml), and [.github/workflows/release.yml](../../../.github/workflows/release.yml). The exact job links for the one full-release path appear in 17-CI-EFFICIENCY.md.

## Failed runs and flake disposition

| Run and job | Observed failure | Classification |
| --- | --- | --- |
| [45897859856, be32](https://github.com/szTheory/schwaystation/actions/runs/45897859856/job/140176458231) | compat.inventory.accounting failed while a shell tried to execute a PowerPC ELF directly; the error is “not found” followed by an unterminated quoted string. The test did not invoke the configured cross-compiling emulator. | Test-harness invocation failure; it does not establish a product-core regression or a timeout. |
| [45897157942, fuzz](https://github.com/szTheory/schwaystation/actions/runs/45897157942/job/140173849372) | Cheap preflight reported 4,694 counted lines against a 4,693 limit. | Reproducible repository policy-bound failure, seen again in CI below; not a flaky runtime result. |
| [45879737052, changes](https://github.com/szTheory/schwaystation/actions/runs/45879737052/job/140115746874) | The same 4,694-versus-4,693 line-budget failure stopped the CI route before its heavy jobs. | Same policy-bound condition, not an independent product failure. |
| [45885388337, vectors-full](https://github.com/szTheory/schwaystation/actions/runs/45885388337/job/140134796099) | CMake could not download the pinned PS1 test archive from GitHub and then could not compute its hash. | External fixture acquisition failure; no product test result. |

The available evidence identifies the immediate cause in all four failed runs. It does not demonstrate a recurring flaky product test. There were no rerun attempts in the cohort. The 4,694-line preflight failures are repeat observations of the same bound mismatch; they are not treated as random flakes. No failure is used to justify weakening a test or changing a timeout.

## Windows DLL consumer evidence

The latest ten completed Windows-x64 build-job records in this cohort are listed below. Each job used the windows-2025 runner and attempt 1. Seven emitted an explicit passing result for install.consumer.dll_crt. The other three do not count as clean samples.

| Run | Exact job | Job outcome | DLL consumer signal |
| --- | --- | --- | --- |
| [45949888885](https://github.com/szTheory/schwaystation/actions/runs/45949888885/job/140343426992) | build (windows-x64) | success | Passed, 2.94 s |
| [45943008239](https://github.com/szTheory/schwaystation/actions/runs/45943008239/job/140321571873) | build (windows-x64) | success | Passed, 3.94 s |
| [45923155197](https://github.com/szTheory/schwaystation/actions/runs/45923155197/job/140258731576) | release-ci / build (windows-x64) | success | Passed, 4.95 s |
| [45923003995](https://github.com/szTheory/schwaystation/actions/runs/45923003995/job/140258044471) | build (windows-x64) | success | Passed, 4.04 s |
| [45913678576](https://github.com/szTheory/schwaystation/actions/runs/45913678576/job/140227915946) | build (windows-x64) | success | Passed, 3.09 s |
| [45902643250](https://github.com/szTheory/schwaystation/actions/runs/45902643250/job/140192291688) | build (windows-x64) | success | Passed, 4.35 s |
| [45902217602](https://github.com/szTheory/schwaystation/actions/runs/45902217602/job/140190729407) | build (windows-x64) | cancelled | No DLL consumer row before cancellation. |
| [45897859856](https://github.com/szTheory/schwaystation/actions/runs/45897859856/job/140176458318) | build (windows-x64) | success | Passed, 4.26 s |
| [45892261219](https://github.com/szTheory/schwaystation/actions/runs/45892261219/job/140157760606) | build (windows-x64) | failure | No DLL consumer row in the job log. |
| [45889677135](https://github.com/szTheory/schwaystation/actions/runs/45889677135/job/140149253272) | build (windows-x64) | failure | No DLL consumer row in the job log. |

Seven clean rows do not satisfy the ten-clean-run acceptance condition. TODO-022 stays pending with a three-run clean-sample shortfall; the earlier one-off failure cause remains unproven. The current registration in tests/install/tests.cmake runs the consumer under ctest --build-and-test and checks PASS_REGEX; it does not add a separate child-exit diagnostic. This bounded cohort does not identify the original silent exit cause, so no test fix or todo closure is claimed.

## Runner and checkpoint repetition audit

The current CTest code intentionally repeats a small number of runner executions to compare independent outputs. The latest successful Windows-x64 CI job exposes one exact, single-run duration per CTest row: [run 45949888885, build (windows-x64)](https://github.com/szTheory/schwaystation/actions/runs/45949888885/job/140343426992).

| Test | Invocations and output rows | Independent purpose | Observed CTest row |
| --- | --- | --- | ---: |
| runner.hash_state | 3 runner calls for frames 5, 5, and 6; one state-hash row per call | The repeated frame-5 output checks deterministic equality; frame 6 checks frame sensitivity. | 0.11 s |
| runner.inputs.script | 8 calls, each emitting frame and state hashes (16 output rows total) | Distinct cases check frame-1 latch timing, neutral empty movie, content-free frame hash, changed latched state, multitap input, and an identical replay. | 0.27 s |
| runner.cheats.hash | 8 valid state-hash calls plus two refusal calls | Checks default repeat stability, normalized spelling, single versus duplicate cheat identity, order sensitivity, invalid-cheat early refusal, and disc incompatibility. | 0.25 s |
| runner.hash and runner.hash.frame5 | One independent CLI check each at frames 60 and 5 | Compares different expected frame hashes; these are separate fixed answers, not duplicate output. | Included in the same job log; 0.04 s and 0.03 s. |
| libretro.input.movies | One registered test uses six state checkpoints at frames 1, 2, 60, 100, 120, and 300 across its variants | Preserves checkpoint save/restore and per-frame movie comparisons. | 38.34 s in the same Windows job. |
| RetroArch movie check | A separate real-host path parses the replay records and validates playback restoration through the pinned frontend | Exercises the consumer path and checkpoint format under the real host. | Separate job took 102 s in [run 45949888885](https://github.com/szTheory/schwaystation/actions/runs/45949888885/job/140347490480). |

Sources: [tests/runner/tests.cmake](../../../tests/runner/tests.cmake), [tests/libretro/host.c](../../../tests/libretro/host.c), [tests/retroarch/verify.py](../../../tests/retroarch/verify.py), and [ci.yml](../../../.github/workflows/ci.yml).

The CI workflow repeats the portable suite across six build matrix legs and again in distinct ASan, no-float, BE32, and WASI configurations. Each lane has a different host, architecture, instrumentation, or target and writes independent test/hash evidence. The hash-equality job consumes the lane artifacts; Release CI separately proves its exact release commit. These are independent checks and outputs, not duplicate commands within one equivalent environment.

No repeated invocation can be safely replaced with an already-produced output: the equality checks require a second process result, and other calls produce distinct frames, input state, normalized configurations, or rejection signals. The six libretro checkpoints and real RetroArch replay validate different saved states and a different consumer. Reuse would remove the independent comparison or required output row, so there is no small equivalent command reduction to carry forward.

## Todo disposition and commands

- TODO-022 remains pending. Its ten-clean-run condition is short by three named DLL consumer pass signals.
- TODO-049 is complete as an audit: the bounded review found no safe output reuse; no command or required row was removed.
- TODO-051 remains unchanged and pending, preserving its prior Phase 11 deferral.
- No build or test was run while authoring this report. This plan used read-only Actions records and source inspection.
