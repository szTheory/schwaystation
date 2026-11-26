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

## Ship preflight correction

The first pull-request CI run failed its `changes` preflight because it still enforced the Phase 15 ceiling of 3,901 lines; the producer measured 4,692 against the approved Phase 16 ceiling of 4,694. Nightly's `fuzz` preflight had the same stale limit. CI run [45879737052](https://github.com/szTheory/schwaystation/actions/runs/45879737052) failed in 54 seconds before its platform matrix started; the title and hygiene jobs passed. Nightly run [45879737752](https://github.com/szTheory/schwaystation/actions/runs/45879737752) also failed its `fuzz` preflight. Its vector and three platform benchmark jobs completed; the remaining long jobs were canceled after the same cause was confirmed.

Both hosted preflight commands and the hygiene budget now use the final measured 4,693-line cap. The workflow registration test checks the CI `changes` job and Nightly `fuzz` job. Local correction checks exited 0:

- `cmake -DMODE=pins -DDIR=.github/workflows -P tests/cmake/workflows.cmake && cmake -DMODE=timeouts -DDIR=.github/workflows -P tests/cmake/workflows.cmake && cmake -DCASE=disc_registration -DROOT=. -P tests/cmake/workflows.cmake` — action pins, timeout comments and hosted registration passed.
- `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=4693 -P tests/cmake/line_budget.cmake` — measured 4,693 lines at the ratcheted limit.
- `git diff --check` — passed.

The supplemental review found no issues in this correction: [ship correction review](16-SHIP-REVIEW.md). The hosted jobs must be rerun on the corrected head; their final run URLs are recorded after completion.

The configured regression command `cmake --workflow --preset ci` exited 0 on the corrected tree. It passed all 742 CI tests and all 8 release checks; CTest reported 148.37 seconds for the main suite and 2.68 seconds for release checks. The longest cases were `policy.quality_metrics` (88.89 seconds), `libretro.host` (73.25 seconds), and `runner.disc.legacy` (47.41 seconds). The command completed normally after all tests and package steps returned.
