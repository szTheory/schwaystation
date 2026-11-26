# Phase 16 local evidence

## Native graphs and consumers

- `git diff --check`, the ABI/compatibility/disc/fuzz/libretro/runner inventory sorts, the repository-check budget, and the workflow registration check exited 0. The inventory measured 4,692 lines on the reviewed source under the approved 4,694-line ceiling. Both generated OpenBIOS fixture digest checks exited 0.
- `ctest --preset ci --show-only=json-v1` with the exact ABI, patch, cheat, inventory, capability, replay, documentation and installed-consumer predicates confirmed all 32 selected tests and fixtures were present once. The targeted `cmake --build --preset ci --target` command for those executables exited 0, and `ctest --preset ci --no-tests=error -R <focused selectors> --output-on-failure` exited 0. No full-suite result is claimed in this report.
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
