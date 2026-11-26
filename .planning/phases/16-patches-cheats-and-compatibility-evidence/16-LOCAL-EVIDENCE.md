# Phase 16 local evidence

## Native graphs and consumers

- `git diff --check`, the ABI/compatibility/disc/fuzz/libretro/runner inventory sorts, the repository-check budget, and the workflow registration check exited 0. The inventory measured 4,692 lines on the reviewed source under the approved 4,694-line ceiling. Both generated OpenBIOS fixture digest checks exited 0.
- `ctest --preset ci --show-only=json-v1` with the exact ABI, patch, cheat, inventory, capability, replay, documentation and installed-consumer predicates confirmed all 32 selected tests and fixtures were present once. The targeted `cmake --build --preset ci --target` command for those executables exited 0, and `ctest --preset ci --no-tests=error -R <focused selectors> --output-on-failure` exited 0. The broad local workflow recorded below ran before the final hosted-CI follow-up and does not validate those later corrections.
- Separate `cmake -S . -B <private-dir> -G Ninja -DCMAKE_BUILD_TYPE=Release -DSCHWAYSTATION_BUILD_TESTS=OFF -DSCHWAYSTATION_CHD=ON` and CHD-off configurations, `cmake --build <private-dir> --target schwaystation_libretro` and component installations exited 0. Each package showed the corresponding CHD extension closure, `cheats "true"`, and `is_experimental "true"`.
- `install.stage` and all three installed-consumer tests passed. Two temporary consumers independently called `schway_write_ram` through the installed C and C++ configurations. Their configure, link and execution commands exited 0. Both private graph/stage roots and the temporary consumer were removed after the commands returned.

## Quality producers

- `cmake --build --preset ci --target schwaystation-test-state` and `build/ci/tests/schwaystation-test-state state.stable_bytes` exited 0. The measured 3,757,754 bytes matched the active bound. `cmake -DQUALITY=quality.json -DMETRIC=state.bytes -DMEASURED=3757754 -P tests/cmake/quality.cmake` exited 0.
- The tests-enabled, CHD-on Release WASI runner built with Clang/LLD 21.1.8 in an x86_64 Ubuntu 26.04 container. CMake 4.2.3 used the same `wasm32-chd` cache values through direct configure. The pinned Wasmtime 48.0.7 and native CHD generator were staged before configuration. `cmake --build build/wasm32 --target schwaystation-run` exited 0, and `wc -c < build/wasm32/schwaystation-run` measured 1,014,609 bytes. The old 957,144-byte bound rejected that exact artifact. The updated exact bound passed `cmake -DQUALITY=quality.json -DMETRIC=size.wasm -DARTIFACT=<module> -P tests/cmake/quality.cmake` with exit 0. The hosted WASI measurement remains a ship gate.
- `ctest --test-dir build/wasm32 --no-tests=error -R '^(abi[.]api_v11|disc[.]patch[.]|runner[.](patch|cheats)[.])' --output-on-failure --parallel 4` exited 0. All 15 focused cases passed under pinned Wasmtime. The sanitizer-configured Ubuntu build also built all four patch/cheat campaign and replay executables, and `ctest --test-dir build/fuzz-check --no-tests=error -R '^fuzz[.]regress[.](ppf|cheat)$'` exited 0. No campaign ran. The temporary checkout, WASI binary and task container were removed after both commands exited.
- The authoritative Cachegrind comparison remains the hosted PR `perf` job's exact current-head and merge-base testcard and GPU-motion instruction counts. The previous phase demonstrated that a local container of the same distribution produces different absolute values. The relative gate and absolute bounds carry forward pending that measurement.
- The CI dependency path needs the hosted workflow attempt and a matching workload fingerprint. No local elapsed time substitutes for that value.

## Requirement evidence and remaining gaps

| Requirement | Local evidence |
| --- | --- |
| PATCH-01 | The seven patch predicates and runner CLI passed. Source identity, malformed-input rejection, commit-or-preserve behavior and image geometry remain tested. |
| CHEAT-01 | API v11, parser, limits, order, runner CLI/hash, and libretro apply/controls/serialization passed. |
| COMPAT-01 | The inventory accounting, rejection controls and generated document passed. The pinned inventory has 13,407 occurrences and no measured retail title results. |
| COMPAT-03 | Coverage, negative results, staged claims and the protected feature baseline passed. Libretro remains experimental pending boot with input-driven control, changing picture and audible output for each provider and each region/board profile. |

The missing boot-to-play rows are ISO, raw CUE, M3U selection, M3U control, PBP, ECM, CCD and CHD when enabled,
plus all nine board profiles in each of the three regions. Current build evidence applies only to the predicates
whose tests executed. The declared matrix leaves unsupported claims unknown.

The local check precedes source review and reviewed-head phase verification. Hosted platforms, hash equality,
RetroArch, and release integration remain ship gates.

## Ship preflight correction

The first pull-request CI run failed its `changes` preflight because it still enforced the Phase 15 ceiling of 3,901 lines; the producer measured 4,692 against the approved Phase 16 ceiling of 4,694. Nightly's `fuzz` preflight had the same stale limit. CI run [45879737052](https://github.com/szTheory/schwaystation/actions/runs/45879737052) failed in 54 seconds before its platform matrix started; the title and hygiene jobs passed. Nightly run [45879737752](https://github.com/szTheory/schwaystation/actions/runs/45879737752) also failed its `fuzz` preflight. Its vector and three platform benchmark jobs completed; the remaining long jobs were canceled after the same cause was confirmed.

Both hosted preflight commands and the hygiene budget now use the final measured 4,693-line cap. The workflow registration test checks the CI `changes` job and Nightly `fuzz` job. Local correction checks exited 0:

- `cmake -DMODE=pins -DDIR=.github/workflows -P tests/cmake/workflows.cmake && cmake -DMODE=timeouts -DDIR=.github/workflows -P tests/cmake/workflows.cmake && cmake -DCASE=disc_registration -DROOT=. -P tests/cmake/workflows.cmake` — action pins, timeout comments and hosted registration passed.
- `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=4693 -P tests/cmake/line_budget.cmake` — measured 4,693 lines at the ratcheted limit.
- `git diff --check` — passed.

The supplemental review found no issues in this correction: [ship correction review](16-SHIP-REVIEW.md). The hosted jobs must be rerun on the corrected head; their final run URLs are recorded after completion.

The configured regression command `cmake --workflow --preset ci` exited 0 on the earlier correction tree. It passed all 742 CI tests and all 8 release checks; CTest reported 148.37 seconds for the main suite and 2.68 seconds for release checks. The longest cases were `policy.quality_metrics` (88.89 seconds), `libretro.host` (73.25 seconds), and `runner.disc.legacy` (47.41 seconds). The command completed normally after all tests and package steps returned; this result predates the final hosted-CI follow-up below.

## Hosted CI follow-up

CI run [45885388357](https://github.com/szTheory/schwaystation/actions/runs/45885388357) ran on `25ba6bb749c968018e23618df35951a12c092645` and was canceled after the known failures were identified. Hygiene passed in 56 seconds, the three build legs for Linux ARM64 and macOS passed, and RetroArch passed in 1 minute 44 seconds. The perf job passed both relative regression checks, but its exact hosted values were 1,620,381 testcard and 138,406,446 GPU-motion instructions per frame; the checked-in exact limits now track those producer values. The WASI module measured 1,014,665 bytes and its exact size limit now tracks that hosted measurement. `quality.json` records both source jobs.

The same CI attempt exposed three integration faults, now corrected locally: MSVC `/WX` rejected `tmpfile()` with C4996, so the MSVC branch uses `tmpfile_s()`; the installed-library SDL2 example omitted patch/BPS and hash implementations, so its executable source list now includes them; and the malformed PPF3 bounds fixture passed a two-byte read length with a one-byte source, so it now supplies a two-byte fixture. BE32 was canceled after 18 minutes 15 seconds while this known-failing head was being retired.

Nightly run [45885388337](https://github.com/szTheory/schwaystation/actions/runs/45885388337) completed its benchmark jobs but the fuzz job reached its 15-minute timeout after 12 of 13 bounded campaigns. The timeout is now 18 minutes, allowing the final cheat campaign and a two-minute margin. `vectors-full` failed during the external asset download with curl error 28 for the pinned JaCzekanski test archive; no code failure was reported. A fresh Nightly run is still needed to confirm the longer campaign window and vector download.

The first corrected-head rerun, CI [45888744986](https://github.com/szTheory/schwaystation/actions/runs/45888744986) and Nightly [45888744907](https://github.com/szTheory/schwaystation/actions/runs/45888744907), was canceled after Windows x64 failed its build. The failing step was [job 140146183852](https://github.com/szTheory/schwaystation/actions/runs/45888744986/job/140146183852): MSVC `/WX` reported C4996 for `fopen()` in `host/dat_inventory.c:554`. The MSVC helper now uses `fopen_s()` for both reads and exclusive writes and restores the returned error to `errno`, preserving the existing `EEXIST` retry. The other Windows leg and full CI/Nightly validation remain pending.

Focused local follow-up checks exited 0:

- `cmake --build --preset ci --target schwaystation-test-disc-patch && ctest --preset ci --no-tests=error -R '^disc[.]patch[.]' --output-on-failure` — all seven patch tests passed after correcting the fixture source length.
- `cmake --preset asan -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ && cmake --build --preset asan --target schwaystation-test-disc-patch && ctest --test-dir build/asan --no-tests=error -R '^disc[.]patch[.]' --output-on-failure` — all seven patch tests passed with address and undefined-behavior sanitizers.
- `cmake --install build/ci --prefix build/ci --component library && cmake -S examples/sdl2 -B build/example-sdl2 -DCMAKE_PREFIX_PATH="$PWD/build/ci" -Dschwaystation_DIR="$PWD/build/ci/lib/cmake/schwaystation" && cmake --build build/example-sdl2 --parallel 2` — the installed-library SDL2 example configured and linked successfully.
- `cmake -DQUALITY=quality.json -DMETRIC=ir_per_frame.testcard -DMEASURED=1620381 -P tests/cmake/quality.cmake && cmake -DQUALITY=quality.json -DMETRIC=ir_per_frame.gpu_motion -DMEASURED=138406446 -P tests/cmake/quality.cmake && cmake -DQUALITY=quality.json -DMETRIC=size.wasm -DMEASURED=1014665 -P tests/cmake/quality.cmake` — exact hosted measurements matched all three active bounds.
- `cmake -DMODE=pins -DDIR=.github/workflows -P tests/cmake/workflows.cmake && cmake -DMODE=timeouts -DDIR=.github/workflows -P tests/cmake/workflows.cmake && cmake -DCASE=disc_registration -DROOT=. -P tests/cmake/workflows.cmake` — action pins, timeout comments and hosted registration passed.
- `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=4693 -P tests/cmake/line_budget.cmake` — the repository-check budget remained at 4,693 lines under the 4,694-line limit.
- `git diff --check` — passed after the final evidence and source edits.
- `cmake --build --preset ci --target schwaystation-run schwaystation-test-library && ctest --preset ci --no-tests=error -R '^compat[.]inventory[.]' --output-on-failure` exited 0 — the DAT compatibility preflight and both inventory cases passed after the MSVC file-open change.

The second corrected-head attempt, CI [45889677135](https://github.com/szTheory/schwaystation/actions/runs/45889677135) and Nightly [45889677194](https://github.com/szTheory/schwaystation/actions/runs/45889677194), was canceled after Windows x64 and ARM64 both failed MSVC `/WX` with C4701 for the test host's potentially uninitialized `core_size`. The WASI lane also trapped in `fuzz.regress.ppf` after 358 of its 361 replay inputs: the nested PPF case keeps four 39,984-byte buffers and a 2 KiB baseline on the 128 KiB default stack. `core_size` now starts at zero, and that WASI replay target links with a 256 KiB stack.

Local checks for these two corrections exited 0:

- `cmake --build --preset ci --target schwaystation-test-libretro-host` — the libretro host test compiled after initializing `core_size`.
- In the Ubuntu 26.04 Clang/LLD 21.1.8 WASI container, `cmake -S /workspace -B /workspace/build/wasi-ppf-repro -G Ninja -DCMAKE_TOOLCHAIN_FILE=/workspace/cmake/toolchains/wasm32-wasi.cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_COMPILE_WARNING_AS_ERROR=ON -DSCHWAYSTATION_CHD=OFF -DSCHWAYSTATION_BUILD_LIBRETRO=OFF -DSCHWAYSTATION_INVENTORY=wasm32 -DSCHWAYSTATION_TIMEOUT_SCALE=5 "-DCMAKE_CROSSCOMPILING_EMULATOR=/workspace/scripts/wasm-test-emulator.sh;/workspace/build/wasi-ppf-repro/pins/wasmtime/wasmtime;/workspace;/workspace/build/wasi-ppf-repro"` configured successfully, and `cmake --build build/wasi-ppf-repro --target schwaystation-fuzz-ppf-replay` linked the replay target with the larger stack.
- `ctest --test-dir /workspace/build/wasi-ppf-repro --no-tests=error -R '^fuzz[.]regress[.]ppf$' --output-on-failure` exited 0; the formerly trapping PPF replay passed in 0.55 seconds under pinned Wasmtime 48.0.7.
- `git diff --check` exited 0.

The hosted platform matrix and Nightly results still need a fresh run on this correction.
