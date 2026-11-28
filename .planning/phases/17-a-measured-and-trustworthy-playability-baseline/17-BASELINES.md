# Phase 17 quality baselines

## Scope and disposition

This report inventories the five existing keys in `quality.json`. It records exact current producer outputs where observable, active bounds, and any remaining evidence gap. No producer is replaced with a surrogate, no device was benchmarked, and no bound is widened.

The current source tree and the producer inputs used by the cited runs were compared directly:

- State and WASI inputs: `git diff --exit-code 39e3ba1a8e89a15473d9e4e6126dd4034f41e043 origin/main -- src include tests/core tests/perf tests/fixtures tests/runner quality.json CMakePresets.json cmake/toolchains/wasm32-wasi.cmake pins.txt scripts/fetch-pin.sh scripts/ci-critical-path.sh tests/cmake/quality.cmake` exited 0. The later CI workflow edits affect the path-classifier comment and its early budget preflight, not either metric producer.
- Cachegrind inputs: `git diff --exit-code 06bb5da9c5213b81c53d5de606a10af87f9b2a37 origin/main -- src include runner tests/core tests/perf tests/fixtures tests/runner CMakeLists.txt CMakePresets.json pins.txt scripts/fetch-pin.sh tests/cmake/perf_compare.cmake tests/cmake/quality.cmake` exited 0. The `perf` job block is byte-identical at those two revisions (`cmp` exited 0).

Both exact producer runs below used attempt 1. Their Ubuntu image releases are linked in the rows. The image manifest records CMake 4.5.0 on the state/WASI run and CMake 4.5.1 on the Cachegrind run; the build logs identify GNU 15.2.0 or Clang 21.1.8 as noted. The pinned image manifest is [Ubuntu 26.04 image 20261116.202](https://github.com/actions/runner-images/blob/ubuntu26/20261116.202/images/ubuntu/Ubuntu2604-Readme.md) or [image 20261122.9](https://github.com/actions/runner-images/blob/ubuntu26/20261122.9/images/ubuntu/Ubuntu2604-Readme.md).

| Metric | Existing producer and configuration | Exact available result and source | Active bound | Disposition |
|---|---|---|---|---|
| `ir_per_frame.testcard` | CI `perf` job; Release runner-only preset, tests off, CHD default off; host compiler; Cachegrind with cache simulation off. `testcard` runs at frames 60 and 360, `(Ir360 - Ir60) / 300`. No external executable or BIOS/disc fixture. | 1,620,381 Ir/frame. The same run measured base 1,620,373 and reported `delta_bp=0 verdict=PASS`. [Run 45902643250, attempt 1, job 140191914798](https://github.com/szTheory/schwaystation/actions/runs/45902643250/job/140191914798). | 1,620,381–1,620,381 | Exact output matches both bounds. |
| `ir_per_frame.gpu_motion` | Same CI `perf` job and Release runner-only configuration. A separate CI-preset build generates `tests/fixtures/src/gpu_motion.asm` as `gpu_motion.exe`; its source SHA-256 is `c9af0565caa7a5786c270fde6aaf661faea346ed51f597294fc0d9e7bec7eb1f`. Frames 10 and 60 use `(Ir60 - Ir10) / 50`. The fixture generator is the pinned `armips` producer. | 138,406,446 Ir/frame. The same run measured base 138,406,436 and reported `delta_bp=0 verdict=PASS`. [Run 45902643250, attempt 1, job 140191914798](https://github.com/szTheory/schwaystation/actions/runs/45902643250/job/140191914798). | 138,406,446–138,406,446 | Exact output matches both bounds. |
| `size.wasm` | `cmake --preset wasm32-chd`; build `schwaystation-run`; the existing `quality.cmake` checks the linked `build/wasm32/schwaystation-run` file size and Wasm magic. Release, tests on, CHD on, stack 256 KiB; `clang-21`, `lld-21`, `wasi-libc`, and `libclang-rt-21-dev-wasm32`. Current toolchain-file SHA-256: `b68c05e0e1b24e1e1de70f264afa208576c85dac76a4206700fa7e11c56a916a`. The lane uses pinned Wasmtime 48.0.7 for tests and hashes, not for measuring browser speed. | 1,014,665 linked bytes, repeated by the exact current producer on [run 46165917340, attempt 1, job 141043516845](https://github.com/szTheory/schwaystation/actions/runs/46165917340/job/141043516845) and [run 46170139230, attempt 1, job 141056653305](https://github.com/szTheory/schwaystation/actions/runs/46170139230/job/141056653305). Both successful WASI jobs reported the exact integer after validation. | 1,014,665–1,014,665 | Repeated exact hosted output supports the point bound. The 1,014,617-byte local CMake 4.2.3 sample remains a separate toolchain observation. |
| `state.bytes` | Linux x64 CI `build` job; `cmake --preset ci` (Release, tests on, CHD on), build `schwaystation-test-state`, then run `build/ci/tests/schwaystation-test-state state.stable_bytes`. The fixed NTSC-U state test runs 45 frames, saves, loads, saves again, and checks the complete byte sequence including the 20-byte header. It uses no external fixture. | `state_bytes=3757754`. [Run 45923003995, attempt 1, job 140258044481](https://github.com/szTheory/schwaystation/actions/runs/45923003995/job/140258044481). | 3,757,754–3,757,754 | Exact output matches both bounds. |
| `ci.critical_path_s` | `scripts/ci-critical-path.sh` reads completed jobs from the exact current Actions attempt, derives the dependency critical path, and compares only a matching workload fingerprint. | 2,761 seconds from the successful exact-head PR attempt; producer printed fingerprint `453c9901d914abd0a6a9b8410ece574ec75f0f835423bdb2662e76d03a3a14de`. The PR route was full (`heavy=true`), uncalled (`CI_CALLED=false`), non-main, with no release inputs. [Run 46170139230, attempt 1, CI required job 141069205674](https://github.com/szTheory/schwaystation/actions/runs/46170139230/job/141069205674). | 2,249–3,293 seconds | Exact result is within the unchanged interval. Fingerprint now identifies this PR route; the historical push/main fingerprint was `316f86729c5129fc618ae0acf71014a5b18784d9e7a4f6c03ff067d0b4326ed8`. No speedup or timeout change is inferred. |

## Producer detail and limits

The `gpu_motion` fixture is generated with `armips` commit `62adab4ef30da765f5cf22a451eb08a59c54dc8b`, pinned by SHA-256 `f7a65102febb4811a336a06e6d93c7e84cc86548182d54e1b1b4e2bffe562f56` in `pins.txt`.

### Cachegrind

The latest successful exact producer comparison is [CI run 45902643250](https://github.com/szTheory/schwaystation/actions/runs/45902643250). Its runner was `ubuntu-26.04`, image 20261122.9.1; CMake was 4.5.1 from that image, GNU C/C++ was 15.2.0, and the job installed Valgrind 3.26.0. `perf` builds only the Release runner with the host compiler and `SCHWAYSTATION_BUILD_TESTS=OFF`; its separate fixture build uses the CI preset (Release, tests on, CHD on). The two fixtures use the fixed windows above. The exact measurement lines and the relative comparison both passed. The relative limit remains +100 basis points; the absolute producer check remains integer-only and exact.

### Linked WASI bytes

The two exact current hosted producers are [run 46165917340, job 141043516845](https://github.com/szTheory/schwaystation/actions/runs/46165917340/job/141043516845) and [run 46170139230, job 141056653305](https://github.com/szTheory/schwaystation/actions/runs/46170139230/job/141056653305). Both used runner label `ubuntu-26.04`, Clang 21.1.8, configured `lld-21`, `libclang-rt-21-dev-wasm32` 1:21.1.8-6ubuntu1, and `wasi-libc` 0.0~git20250726.3f7eb4c-4. The workflow configured `wasm32-chd` with Release, tests on, CHD on and a 256 KiB stack; the exact producer command exited 0 and printed `quality: size.wasm: measured 1014665; within configured bounds` in both attempts. The job logs do not expose a runner-image patch or exact CMake version, so neither is inferred. `pins.txt` pins Wasmtime 48.0.7 for portable tests and hashes; this is not a browser-speed measurement. The identical bytes support the active exact hosted bound. The local 1,014,617-byte result used CMake 4.2.3 in an x86_64 Ubuntu 26.04 task container and remains separately attributed.

### Stable state bytes

The exact hosted producer is the Linux x64 build job in run 45923003995. It used runner image 20261116.202.1, CMake 4.5.0, GNU 15.2.0, and the `ci` Release preset with tests and CHD enabled. The test prints one `state_bytes` line and returns success only after save/load/save byte equality. The output is the complete serialized state length, not an estimate or a hash-manifest length.

### Low-power and browser assumptions

The only measured instructions-per-frame values above are Cachegrind counts on the hosted Ubuntu runner for the two named fixtures. The device table in `.planning/preparation/ENGINEERING.md` §8 computes assumed budgets from assumed sustained rates: Apple Silicon M1 class 12,000 MIPS → 100 million Ir/frame; Raspberry Pi 4 A72 2,250 MIPS → 18.8 million; RG35XX-H A53 1,500 MIPS → 12.5 million. `.planning/preparation/PSX-ECOSYSTEM-AND-USERS.md` §4 records the derived work-per-frame model and the source references behind these device assumptions. Those rates are hypotheses, not measurements on any of those devices; no owner hardware was run. `gpu_motion` exceeds each listed assumed budget, while that comparison does not establish device frame rate. The WASI hash lane proves deterministic output under the pinned Wasmtime 48.0.7 environment; it provides no browser-speed measurement.

## Bound disposition

Plan 17-03 retained the WASI interval and historical timing fingerprint because no current exact values were available. Plan 17-11 supersedes those pending dispositions with repeated exact hosted WASI output and exact PR timing evidence below. The two Cachegrind outputs, stable-state output, 100-basis-point relative performance limit, Cachegrind frame windows, metric keys, units, and serialized-state definition remain unchanged.

## Validation record

- Task 1: `cmake -DQUALITY=quality.json -P tests/cmake/quality.cmake && git diff --check origin/main` — exit 0.
- Task 2: the same registry and diff check — exit 0. No `quality.json` value changed; `git diff --exit-code origin/main -- quality.json` exited 0.
- Existing exact comparator: the `testcard` 1,620,381, `gpu_motion` 138,406,446 and `state.bytes` 3,757,754 outputs passed. The known WASI samples 1,014,617 and 1,014,665 each passed against the retained interval. Each invocation of `tests/cmake/quality.cmake` exited 0.
- Existing boundary, malformed-input, integer and producer-wiring controls: `cmake -DROOT=<absolute repo root> -DBIN=<task temporary directory>/quality-controls -P tests/cmake/quality_controls.cmake` — exit 0. An initial direct invocation with `ROOT=.` exited 1 because that script requires an absolute root; the corrected invocation completed all controls successfully.
- `git diff --check origin/main` — exit 0.

## Plan 17-09 reconciliation

The Plan 17-09 observations below were accurate at that handoff and are superseded where stated by Plan 17-11.

## Plan 17-11 reconciliation

The exact hosted WASI measurement is 1,014,665 bytes in two current PR attempt-1 producer jobs, each exit 0 and using the same logged toolchain package versions and wasm32-chd configuration. The active bound is now tightened to that exact integer. The earlier 1,014,617 local result remains a distinct CMake 4.2.3 observation and is not represented as the hosted producer.

The successful [PR run 46170139230, CI required job 141069205674](https://github.com/szTheory/schwaystation/actions/runs/46170139230/job/141069205674) produced 2,761 seconds with fingerprint `453c9901d914abd0a6a9b8410ece574ec75f0f835423bdb2662e76d03a3a14de` on the full, uncalled, non-main PR route. The exact value lies inside 2,249–3,293 seconds, so the interval is unchanged and the stored fingerprint is refreshed to this route. The old push/main result is retained in `quality.json`'s dated reason as provenance. This is a current workload baseline, not a speedup claim.

`ci.critical_path_s` fingerprints the workflow, CMake inputs, selected tests, scripts, pins, and exact runner labels; `quality.json` is not in its workload projection. These registry metadata edits therefore do not alter the measured workload. Local reconciliation commands and statuses are recorded in `17-MEASUREMENT.md`. Fresh Phase 17 verification remains the orchestrator's next action; TUNE-10 is not declared closed by this plan alone. Device and browser performance remain unmeasured.
