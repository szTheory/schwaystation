# Phase 7: Measured quality baselines

These are producer-bound observations for the current library and CI. A number is eligible for a hard bound only
when its producer, units, source inputs and workload match the recorded evidence. The WASI module size was
measured on the hosted Linux producer, and the checked-in bound matches that exact linked-module measurement.

## Baseline table

| Metric | Units | Producer and input identity | Measured result | Candidate bound |
| --- | --- | --- | ---: | --- |
| `ir_per_frame.testcard` | Cachegrind Ir per emulated frame | Release `schwaystation-run`; built-in test card; `ubuntu-26.04`; Valgrind 3.26.0; matching manual `perf` producer, three samples | 1,619,573, 1,619,573, 1,619,573; spread 0 | floor 1,619,573; ceiling 1,619,573 |
| `ir_per_frame.gpu_motion` | Cachegrind Ir per emulated frame | Same Release runner and toolchain; `tests/fixtures/src/gpu_motion.asm`, built by the pinned armips tool; same manual `perf` producer, three samples | 137,752,334, 137,752,334, 137,752,334; spread 0 | floor 137,752,334; ceiling 137,752,334 |
| `size.wasm` | linked module bytes | Release `schwaystation-run` target from the `wasm32` preset; the runner module at `build/wasm32/schwaystation-run`, not a host wrapper or compressed archive; `ubuntu-26.04` | 608,540 bytes; [Phase 8 run 44597715729, WASI job 135820879050](https://github.com/szTheory/schwaystation/actions/runs/44597715729/job/135820879050) | floor 608,540; ceiling 608,540 |
| `state.bytes` | serialized bytes including header | Current Release `schwaystation` library; NTSC-U instance matching `tests/core/test_state.c::make`; public `schway_state_size` and `schway_save_state` API | 3,755,328 total; [Phase 8 run 44600162797, state measurement job 135829636881](https://github.com/szTheory/schwaystation/actions/runs/44600162797/job/135829636881) | floor 3,755,328; ceiling 3,755,328 |
| `ci.critical_path_s` | seconds from recorded job DAG | Exact matching `ci.yml` push cohort from Plan 02; `changes → openbios → be32 → hash-equality → ci-required`; fingerprint and runs below | 2,249 s and 3,293 s; observed spread 1,044 s | floor 2,249; ceiling 3,293 |

## Instructions per frame

The canonical producer is the `perf` job in `.github/workflows/ci.yml`. On a manual dispatch with no `control_ref`,
the workflow builds `schwaystation-run` with `cmake --workflow --preset perf`, installs Valgrind, runs the test card at
60 and 360 frames and `gpu_motion.exe` at 10 and 60 frames, then computes integer Ir per frame as
`(end_ir - start_ir) / (end_frame - start_frame)`. It reports three independent samples per fixture and their
spread. The run used the unchanged producer commands on `ubuntu-26.04`; its install log identifies Valgrind
`1:3.26.0-0ubuntu1`.

The attributable run is [CI run 44218206100](https://github.com/szTheory/schwaystation/actions/runs/44218206100),
[perf job 134513316956](https://github.com/szTheory/schwaystation/actions/runs/44218206100/job/134513316956), at source
commit `28683ebe4c9940c95d539b5c18733c139f940ffa`. The log reports:

```text
perf control testcard runs=1619573,1619573,1619573 spread=0
perf control gpu_motion runs=137752334,137752334,137752334 spread=0
```

The current tree has no changes to `src/`, `runner/`, `tests/core/`, `tests/fixtures/`, `CMakeLists.txt`,
`CMakePresets.json`, `cmake/`, `tests/perf/`, or `pins.txt` since that run's commit. The intervening `ci.yml` changes
are confined to changed-path classification and the job timeout; the Cachegrind commands and frame windows are
unchanged. The run therefore measures the same source, fixture and producer inputs. The older values in
`docs/testing.md` describe an earlier toolchain run and are not used as the current measurements.

## Serialized state

The state harness constructs a default instance with its NTSC-U profile, reads `schway_state_size`, allocates that
capacity and calls `schway_save_state`. The original Phase 7 diagnostic in ignored `build/phase07/` used the same
setup and verified the header's total-size field at offset 12 against the API result. It recorded 3,755,020 bytes
before Phase 8 added per-game card identity state. The current serialized size is 3,755,328 bytes, measured by
the producer in [Phase 8 run 44600162797, state measurement job 135829636881](https://github.com/szTheory/schwaystation/actions/runs/44600162797/job/135829636881).
The original diagnostic commands were:

```text
cmake --build build/perf --target schwaystation
/usr/bin/cc -std=c17 -Wall -Wextra -Wpedantic -Iinclude -Ibuild/perf/generated build/phase07/state_size_probe.c build/perf/libschwaystation.a -o build/phase07/state_size_probe
build/phase07/state_size_probe
```

The build exited 0. The diagnostic exited 0 and printed:

```text
region=NTSC-U header_bytes=20 state_size_api=3755020 body_bytes=3755000 encoded_total=3755020
```

The state existed only in process memory; no state bytes or diagnostic source were committed. `state.bytes` counts
the complete API-reported length, including the header.

## WASI module measurement

The configured `build/wasm32/CMakeCache.txt` names `cmake/toolchains/wasm32-wasi.cmake` and Release mode, but this
checkout has no generated `build.ninja` or Makefile for that tree. `cmake --build build/wasm32 --target help` exited 1
with `ninja: error: loading 'build.ninja': No such file or directory`; `clang-21`, `clang++-21`, `wasm-ld-21` and a
WASI sysroot are absent locally. The initial Phase 7 hosted measurement was 595,820 bytes. After Phase 8 added
per-game card identity support, the hosted `wasm32` lane measured the linked target at 608,540 bytes in
[run 44597715729, WASI job 135820879050](https://github.com/szTheory/schwaystation/actions/runs/44597715729/job/135820879050).
This is the module itself, not a host wrapper or compressed archive.

The narrow measurement can run on the existing `ubuntu-26.04` WASI lane after its current toolchain setup installs
`wasi-libc` and `libclang-rt-21-dev-wasm32`:

```sh
cmake --preset wasm32
cmake --build build/wasm32 --target schwaystation-run
wc -c build/wasm32/schwaystation-run
```

Confirm the target is a WebAssembly module and record the OS image version plus package versions. This focused
target build does not need to run the WASI test inventory or Wasmtime. For a future toolchain or source change,
remeasure on this producer and review the bound before updating it.

## Comparable CI critical path

Use the exact repeatable full-CI push group, not the single-run 73:17 outlier or the full Release workflow DAG.
Plan 02 identifies two successful, completed attempts with the same full fingerprint
`316f86729c5129fc618ae0acf71014a5b18784d9e7a4f6c03ff067d0b4326ed8`:
[run 44253375502](https://github.com/szTheory/schwaystation/actions/runs/44253375502) and
[run 44307395327](https://github.com/szTheory/schwaystation/actions/runs/44307395327). Their critical paths were 54:53
(3,293 s) and 37:29 (2,249 s), both through `changes → openbios → be32 → hash-equality → ci-required`; `be32` was
the slowest job in each. The 1,044-second spread is retained as observed rather than normalized away. The
single-sample 73:17 group has another fingerprint and remains a visible outlier, not a substitute for this repeated
cohort. See the full DAG, workload contract, per-run job evidence and exclusions in
[`07-CI-EFFICIENCY.md`](07-CI-EFFICIENCY.md).

No before/after speedup is inferred from these baseline observations.
