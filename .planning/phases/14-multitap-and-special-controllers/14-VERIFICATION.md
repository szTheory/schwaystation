---
phase: multitap-and-special-controllers
verified: 2026-11-25T02:06:11Z
status: passed
score: 49/49 must-haves verified
covered_files:
  - .planning/phases/14-multitap-and-special-controllers/14-01-PLAN.md
  - .planning/phases/14-multitap-and-special-controllers/14-01-SUMMARY.md
  - .planning/phases/14-multitap-and-special-controllers/14-02-PLAN.md
  - .planning/phases/14-multitap-and-special-controllers/14-02-SUMMARY.md
  - .planning/phases/14-multitap-and-special-controllers/14-03-PLAN.md
  - .planning/phases/14-multitap-and-special-controllers/14-03-SUMMARY.md
  - .planning/phases/14-multitap-and-special-controllers/14-04-PLAN.md
  - .planning/phases/14-multitap-and-special-controllers/14-04-SUMMARY.md
  - .planning/phases/14-multitap-and-special-controllers/14-05-PLAN.md
  - .planning/phases/14-multitap-and-special-controllers/14-05-SUMMARY.md
  - .planning/phases/14-multitap-and-special-controllers/14-06-PLAN.md
  - .planning/phases/14-multitap-and-special-controllers/14-06-SUMMARY.md
  - .planning/phases/14-multitap-and-special-controllers/14-07-PLAN.md
  - .planning/phases/14-multitap-and-special-controllers/14-07-SUMMARY.md
  - .planning/phases/14-multitap-and-special-controllers/14-08-PLAN.md
  - .planning/phases/14-multitap-and-special-controllers/14-08-SUMMARY.md
  - .planning/phases/14-multitap-and-special-controllers/14-09-PLAN.md
  - .planning/phases/14-multitap-and-special-controllers/14-09-SUMMARY.md
  - .planning/phases/14-multitap-and-special-controllers/14-10-PLAN.md
  - .planning/phases/14-multitap-and-special-controllers/14-10-SUMMARY.md
  - .planning/phases/14-multitap-and-special-controllers/14-11-PLAN.md
  - .planning/phases/14-multitap-and-special-controllers/14-11-SUMMARY.md
  - .planning/phases/14-multitap-and-special-controllers/14-12-PLAN.md
  - .planning/phases/14-multitap-and-special-controllers/14-12-SUMMARY.md
  - .planning/phases/14-multitap-and-special-controllers/14-13-PLAN.md
  - .planning/phases/14-multitap-and-special-controllers/14-13-SUMMARY.md
  - .planning/phases/14-multitap-and-special-controllers/14-14-PLAN.md
  - .planning/phases/14-multitap-and-special-controllers/14-14-SUMMARY.md
  - .planning/phases/14-multitap-and-special-controllers/14-15-PLAN.md
  - .planning/phases/14-multitap-and-special-controllers/14-15-SUMMARY.md
  - .planning/phases/14-multitap-and-special-controllers/14-16-PLAN.md
  - .planning/phases/14-multitap-and-special-controllers/14-16-SUMMARY.md
  - docs/compatibility.md
  - docs/controllers.md
  - docs/embedding.md
  - docs/hardware/sio.md
  - docs/runner.md
  - docs/saves.md
  - docs/testing.md
  - host/cards.c
  - host/cards.h
  - include/schwaystation.h
  - libretro/libretro.c
  - quality.json
  - runner/main.c
  - src/card.c
  - src/card.h
  - src/frame.c
  - src/instance.c
  - src/internal.h
  - src/pad.c
  - src/pad.h
  - src/sio.c
  - src/sio.h
  - src/state.h
  - tests/abi/inventory.txt
  - tests/abi/tests.cmake
  - tests/api/v9.c
  - tests/card/card_tests.c
  - tests/card/files.c
  - tests/card/inventory.txt
  - tests/card/tests.cmake
  - tests/compat/features.csv
  - tests/compat/tests.cmake
  - tests/core/test_core.c
  - tests/core/test_state.c
  - tests/disc/tests.cmake
  - tests/fixtures/tests.cmake
  - tests/libretro/host.c
  - tests/libretro/inventory.txt
  - tests/libretro/tests.cmake
  - tests/runner/inventory.txt
  - tests/runner/tests.cmake
  - tests/sio/inventory.txt
  - tests/sio/pad_tests.c
  - tests/sio/sio_tests.c
  - tests/sio/tests.cmake
covered_digest: "v4:sha256:dad9dabed5bbda8f47271fddca5ab527532bdda84e26b8427f1ffb08c84c12e5"
behavior_unverified: 0
overrides_applied: 0
---

# Phase 14: Multitap and Special Controllers Verification Report

**Phase Goal:** Players can use the multitap on either port and use the PlayStation mouse, neGcon, Jogcon and Dual Analog flight mode.
**Verified:** 2026-11-25 02:06:11 UTC
**Status:** passed
**Re-verification:** Documentation-only freshness pass after `docs/testing.md` changed. The Phase 14 implementation and passing test/hosted CI evidence are unchanged.

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|---|---|---|
| 1 | A tap on either port independently selects four controller and card slots. Fixtures show documented lengths, acknowledgements, interrupts and empty slots, and tap cards round trip without cross-slot changes. | ✓ VERIFIED | Final CTest passed 705/705; direct selector groups: sio.tap.* / card.tap.* / runner.cards.* / libretro.cards.*. |
| 2 | Serial and libretro input fixtures reproduce the mouse's frame-latched movement, buttons, saturation and reset behavior. | ✓ VERIFIED | Final CTest passed 705/705; direct selector groups: sio.special.mouse / libretro.input.special. |
| 3 | neGcon and Jogcon report documented serial modes and integer controls, and explicitly reject unsupported modes. | ✓ VERIFIED | Final CTest passed 705/705; direct selector groups: sio.special.negcon / sio.special.jogcon / libretro.input.special. |
| 4 | The size-tagged input and libretro mappings carry Dual Analog flight axes and buttons. Existing digital-pad and DualShock callers still pass their compatibility checks. Ledger rows record the exercised controller modes. | ✓ VERIFIED | Final CTest passed 705/705; direct selector groups: abi.api_v9 / libretro.input.special / compat.features.*. |
| 5 | The released 28-byte input caller still reaches the frame latch without reading extension bytes. | ✓ VERIFIED | API v9/ABI and configure-time inventory selectors pass in the final 705-test workflow. |
| 6 | Every planned peripheral selector is registered in its existing directory and exact inventory before its implementation starts. | ✓ VERIFIED | API v9/ABI and configure-time inventory selectors pass in the final 705-test workflow. |
| 7 | Missing feature implementations are visible as failing tests until their owner lands them. | ✓ VERIFIED | API v9/ABI and configure-time inventory selectors pass in the final 705-test workflow. |
| 8 | Old 28-byte callers retain their device/buttons/axes meaning and absent fields default to zero. | ✓ VERIFIED | Frozen-v8/API-v9 compatibility and frame-latch selectors pass in the final workflow. |
| 9 | Eight controller endpoints latch independently at frame start without mid-frame host mutation. | ✓ VERIFIED | Frozen-v8/API-v9 compatibility and frame-latch selectors pass in the final workflow. |
| 10 | Invalid tags, reserved bits and device fields reject atomically without reading beyond the caller. | ✓ VERIFIED | Frozen-v8/API-v9 compatibility and frame-latch selectors pass in the final workflow. |
| 11 | Either port supports individual sockets and correctly sequenced four-controller long replies. | ✓ VERIFIED | Tap polling, request-history, ACK/abort selectors pass in the final workflow. |
| 12 | The REQ byte affects the next transaction, including repeated-request short replies, without changing the current reply. | ✓ VERIFIED | Tap polling, request-history, ACK/abort selectors pass in the final workflow. |
| 13 | Empty/invalid slots and aborted transactions cannot produce late ACKs or cross-port state leakage. | ✓ VERIFIED | Tap polling, request-history, ACK/abort selectors pass in the final workflow. |
| 14 | Stable one-based port/socket calls access eight different cards and preserve the legacy card 1/2 aliases. | ✓ VERIFIED | Eight-card protocol, state and independent-write selectors pass in the final workflow. |
| 15 | 8n card addressing selects exactly one card behind either tap with existing read/write timings. | ✓ VERIFIED | Eight-card protocol, state and independent-write selectors pass in the final workflow. |
| 16 | Only a successful terminal write commits its selected sector and generation; invalid/aborted operations leave every card unchanged. | ✓ VERIFIED | Eight-card protocol, state and independent-write selectors pass in the final workflow. |
| 17 | Mouse polls return ID 5A12h, correct active-low buttons and saturated signed integer movement. | ✓ VERIFIED | Mouse/neGcon serial payload, saturation, latch and refusal selectors pass in the final workflow. |
| 18 | neGcon polls return ID 5A23h and independent raw twist/I/II/L controls without response curves. | ✓ VERIFIED | Mouse/neGcon serial payload, saturation, latch and refusal selectors pass in the final workflow. |
| 19 | Each endpoint consumes only its own latched input and rejected commands terminate without a stale ACK. | ✓ VERIFIED | Mouse/neGcon serial payload, saturation, latch and refusal selectors pass in the final workflow. |
| 20 | Dual Analog flight polls return ID 5A53h and the documented four-axis/button payload. | ✓ VERIFIED | Flight/Jogcon payload and unsupported-command selectors pass in the final workflow. |
| 21 | Jogcon native polls return ID 5AE3h with raw dial-word and direction bits from the frame latch. | ✓ VERIFIED | Flight/Jogcon payload and unsupported-command selectors pass in the final workflow. |
| 22 | Unsupported Jogcon software configuration and feedback operations fail explicitly without being accepted as DualShock commands. | ✓ VERIFIED | Flight/Jogcon payload and unsupported-command selectors pass in the final workflow. |
| 23 | Loading a state resumes each selected endpoint and pending SIO event at exactly the original byte/ACK deadline. | ✓ VERIFIED | State round-trip/rejection and card-preservation selectors pass in the final workflow. |
| 24 | A state load cannot roll back any receiving card image or generation. | ✓ VERIFIED | State round-trip/rejection and card-preservation selectors pass in the final workflow. |
| 25 | Invalid topology, device and in-flight combinations reject atomically before any state is applied. | ✓ VERIFIED | State round-trip/rejection and card-preservation selectors pass in the final workflow. |
| 26 | Every port/socket produces a unique stable filename while A sockets retain .srm and .1.mcr. | ✓ VERIFIED | Card path mapping, invalid-input and atomic replacement selectors pass in the final workflow. |
| 27 | Invalid/oversized components or insufficient buffers reject without partial output. | ✓ VERIFIED | Card path mapping, invalid-input and atomic replacement selectors pass in the final workflow. |
| 28 | Existing wrapped-card import and atomic replacement behavior remains unchanged. | ✓ VERIFIED | Card path mapping, invalid-input and atomic replacement selectors pass in the final workflow. |
| 29 | Runner options independently enable either tap and leave both disabled by default. | ✓ VERIFIED | Runner multitap lifecycle, fresh-process persistence and failed-write selectors pass in the final workflow. |
| 30 | All eight selected card endpoints survive runner exit and fresh-process reload without cross-slot changes. | ✓ VERIFIED | Runner multitap lifecycle, fresh-process persistence and failed-write selectors pass in the final workflow. |
| 31 | Failed writes preserve the previous selected save and report failure without rewriting clean files. | ✓ VERIFIED | Runner multitap lifecycle, fresh-process persistence and failed-write selectors pass in the final workflow. |
| 32 | The existing default two-pad frontend callbacks reach their separate physical socket A replies. | ✓ VERIFIED | Libretro callback-to-serial host selectors pass in the final workflow. |
| 33 | The test-only MMIO helper observes real serial responses without adding shipped exports. | ✓ VERIFIED | Libretro callback-to-serial host selectors pass in the final workflow. |
| 34 | One retro_run polls once; callback evidence is tracked per player and excludes disabled tap sockets. | ✓ VERIFIED | Libretro callback-to-serial host selectors pass in the final workflow. |
| 35 | All four per-port tap topologies expose the correct compact active player order. | ✓ VERIFIED | Libretro per-player/topology and inactive-device mapping selectors pass in the final workflow. |
| 36 | Every active frontend player reaches its own physical endpoint with one input poll per frame. | ✓ VERIFIED | Libretro per-player/topology and inactive-device mapping selectors pass in the final workflow. |
| 37 | Invalid or inactive player selection and deinit cannot alias or retain another endpoint. | ✓ VERIFIED | Libretro per-player/topology and inactive-device mapping selectors pass in the final workflow. |
| 38 | Mouse, flight, neGcon and Jogcon callback values reach the expected direct or tap serial endpoint. | ✓ VERIFIED | Mouse/flight/neGcon/Jogcon host mapping and refusal selectors pass in the final workflow. |
| 39 | One frontend poll yields a consistent frame snapshot with documented integer conversion and neutral reset. | ✓ VERIFIED | Mouse/flight/neGcon/Jogcon host mapping and refusal selectors pass in the final workflow. |
| 40 | Exact device IDs are accepted while arbitrary subclasses and unsupported modes remain explicit. | ✓ VERIFIED | Mouse/flight/neGcon/Jogcon host mapping and refusal selectors pass in the final workflow. |
| 41 | Physical port1A remains frontend SAVE_RAM with its unchanged pointer and size contract. | ✓ VERIFIED | Libretro SAVE_RAM alias and sidecar persistence selectors pass in the final workflow. |
| 42 | Seven adapter sidecars round-trip independently under stable socket filenames. | ✓ VERIFIED | Libretro SAVE_RAM alias and sidecar persistence selectors pass in the final workflow. |
| 43 | Clean cards remain unwritten and failed replacement preserves prior bytes and dirty generation. | ✓ VERIFIED | Libretro SAVE_RAM alias and sidecar persistence selectors pass in the final workflow. |
| 44 | State restore keeps all receiving card contents and generations current while restoring protocol state. | ✓ VERIFIED | State/reset/load/disc-swap persistence selectors pass in the final workflow. |
| 45 | Reset, failed load and disc swap preserve each socket and the initial content save identity. | ✓ VERIFIED | State/reset/load/disc-swap persistence selectors pass in the final workflow. |
| 46 | Deinit clears every sidecar path, saved generation and temporary test hook. | ✓ VERIFIED | State/reset/load/disc-swap persistence selectors pass in the final workflow. |
| 47 | The feature ledger identifies only the controller/tap predicates actually proved by passing selectors. | ✓ VERIFIED | Feature preflight, independent ratchet and generated-doc selectors pass in the final workflow. |
| 48 | Controller and save documentation is reachable from existing embedding/testing docs. | ✓ VERIFIED | Documentation/API-name, fixture-hash and producer-backed quality selectors pass in the final workflow. |
| 49 | Quality bounds change only from named producer measurements, and required final verification remains explicit. | ✓ VERIFIED | The matching Ubuntu 26.04 x86_64 Clang/LLD 21.1.8 WASI producer with CHD and test registration enabled measured 947,969 bytes; `quality.cmake` and `policy.quality_metrics` passed after the exact pin update. The final Main CI run [45442543076](https://github.com/szTheory/schwaystation/actions/runs/45442543076) passed on the corrected pin. Cachegrind measured 1,620,101 / 138,319,540 Ir/frame on its hosted producer. |

**Score:** 49/49 truths verified (0 present, behavior-unverified).

### Required Artifacts

| Artifact group | Expected | Status | Details |
|---|---|---|---|
| Phase 14 plan artifacts | Public API, SIO/pad/card/core, runner/libretro, tests, fixtures, compatibility and documentation | ✓ VERIFIED | 46/46 declared artifact paths across all plans exist and are substantive; `query verify.artifacts` returned `all_passed: true` for every plan. |
| State quality | Stable state byte bound and fixture state hashes | ✓ VERIFIED | Producer measured 3,757,638 bytes; stable-bytes check passed. Updated state hashes were directly reproduced. |
| WASI module metric | Exact linked module size | ✓ VERIFIED | The earlier 947,899-byte local result used Ubuntu 26.04 arm64 with tests off and did not match the hosted `wasm32-chd` preset. On head `d46ec1e`, Main CI run [45436318507](https://github.com/szTheory/schwaystation/actions/runs/45436318507) measured exactly 947,969 bytes and failed only against the then-old 947,899 ceiling; setup, fixture generation, configure, runner build and WASM magic check passed. A task-named Ubuntu 26.04 x86_64 reproduction with Clang/LLD 21.1.8, matching WASI package versions, CHD enabled and test registration enabled built `schwaystation-run` at 947,969 bytes. It did not run WASI CTest; emulator/fixture-tool placeholders were used only to configure test registration. After updating the exact floor/ceiling to 947969, `quality.cmake` exited 0 and focused `policy.quality_metrics` passed 1/1 in 47.15 seconds. `jq`, the SIO inventory sort and `git diff --check` also passed. A corrected hosted run is still pending. |
| Cachegrind metrics | Testcard and GPU-motion instructions per frame | ✓ VERIFIED | Linux x64 producer measured 1,620,101 and 138,319,540 Ir/frame in [PR run 45420382438, job 138538117721](https://github.com/szTheory/schwaystation/actions/runs/45420382438/job/138538117721); both relative comparisons passed, the old exact pins rejected them, and `quality.json` now records the exact values. |

### Key Link Verification

| `include/schwaystation.h` | `src/frame.c` → `src/pad.c` | Versioned input snapshot → frame latch → endpoint payload | ✓ WIRED | `schway_set_input` stores normalized input; `schway_run_frame` latches it; SIO reads it via `schway__pad_byte`. API v9 and controller tests exercise this path. |
| `src/sio.c` | `src/pad.c` / `src/card.c` | Serial endpoint dispatch and ACK scheduling | ✓ WIRED | SIO dispatches pad and card bytes through timed events; tap and card protocol selectors pass. |
| `src/instance.c` | `src/card.c` | Public port/socket card load, view and generation access | ✓ WIRED | Coordinates map to eight slots; runner and libretro call these APIs; independent lifecycle tests pass. |
| `runner/main.c` | `host/cards.c` | CLI topology and file paths to socket APIs | ✓ WIRED | Runner selects either tap; load/save loops use socket APIs and named paths; fresh-process tests pass. |
| `libretro/libretro.c` | `src/frame.c` | Host callbacks → snapshot → core frame | ✓ WIRED | `retro_run` polls once, maps each player/device, calls `schway_set_input`, then runs a frame; host packet tests pass. |
| `tests/compat/features.csv` | `tests/compat/tests.cmake` | Independent row preflight and ratchet | ✓ WIRED | CMake validates schema and expected rows; preflight/ratchet/controls selectors pass. |
| `docs/embedding.md` | `docs/controllers.md` / `docs/saves.md` | Reachable controller and save contracts | ✓ WIRED | Embedding and testing guides link these pages; docs link/budget checks pass.

The built-in `verify.key-links` heuristic found 1/30 declared plan links and reported the other 29 as “Target not referenced in source.” Manual C/CMake/data-flow inspection confirmed the production and test connections above. The heuristic does not follow cross-file function calls, inventory generation or generated documentation links; these are parser false negatives, not missing wiring.

### Data-Flow Trace (Level 4)

| Input/data | Source | Flow | Status |
|---|---|---|---|
| Controller buttons, axes and special-device values | Public size-tagged input or libretro callbacks | Normalized snapshot → frame latch → endpoint serial bytes | ✓ FLOWING |
| Runner card images | Named files and selected port/socket | Eight card slots → protocol/generation → atomic save path | ✓ FLOWING |
| Libretro card images | SAVE_RAM alias and socket sidecars | Host load → socket APIs → dirty-generation save | ✓ FLOWING |
| Controller support claims | Registered CTest selectors | Independent feature rows → preflight, ratchet and generated docs | ✓ FLOWING |

## Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|---|---|---|---|
| Integrated controller/card/API/runner/libretro behavior | `cmake --workflow --preset ci` after the GCC watchdog-table correction | Exit 0; 705/705 CTest tests passed in 73.01 seconds; all three macOS arm64 packages produced; release checks passed 8/8 in 1.47 seconds | ✓ PASS |
| Roadmap criteria and key controller seams | `ctest --test-dir build/ci --output-on-failure --no-tests=error -R '^(abi[.]api_v9|card[.]tap[.](protocol|state)|compat[.]features[.](controls|preflight|ratchet)|libretro[.](cards[.]multitap|input[.](special|multitap))|runner[.]cards[.]multitap|sio[.](special[.](flight|jogcon|mouse|negcon)|tap[.](history_edges|poll_methods)))$'` | Exit 0; all 16 selected tests passed in 3.74 seconds. Covers the four roadmap criteria's ABI, serial, card, compatibility, runner and libretro paths. | ✓ PASS |
| Release artifact validation | Workflow CI release-test step | Exit 0; 8/8 release checks passed | ✓ PASS |
| API v9 and endpoint input mapping | `abi.api_v9`, `libretro.input.special`, `libretro.input.multitap` in final workflow | Included in 705/705 pass | ✓ PASS |
| Multitap serial/card/persistence | `sio.tap.*`, `card.tap.*`, `runner.cards.multitap`, `libretro.cards.multitap` in final workflow | Included in 705/705 pass | ✓ PASS |
| Special-controller serial and host mapping | `sio.special.*`, `libretro.input.special` in final workflow | Included in 705/705 pass | ✓ PASS |
| Compatibility ledger | `compat.features.preflight`, `compat.features.ratchet`, `compat.features.controls` | Included in 705/705 pass | ✓ PASS |
| GCC portability and multitap state follow-up | Linux GCC 15.2 configure/build of `schwaystation_sio` with warnings-as-errors; focused `core.state.*` and `sio.tap.*` CTest selectors | Exit 0; the SIO object compiled and all four selected tests passed in 2.07 seconds on the corrected source | ✓ PASS |
| GCC C17 libretro card-path compatibility | Linux GCC 15.2 no-FP configure/build of `schwaystation_libretro` with warnings-as-errors | Exit 0 after passing a temporary array of `const char *` views to the synchronous loader | ✓ PASS |
| Libretro card-path behavior | Build `schwaystation-test-libretro-host`; CTest selectors `libretro.cards.persistence`, `libretro.cards.controls`, `libretro.cards.multitap`, and `libretro.input.multitap` | Exit 0; all four tests passed in 5.16 seconds | ✓ PASS |
| GCC no-FP ABI data classification | GCC 13 C17 compile of `src/pad.c` with `-mgeneral-regs-only`, then GNU `nm -P` for `name_watchdogs` | Exit 0; `name_watchdogs r 300 70` confirms the fixed-width constant string table is emitted as read-only data. The task-named `--rm` container was removed automatically. | ✓ PASS |
| Final-source WASI module size | Task-named Ubuntu 26.04 x86_64 Clang/LLD 21.1.8 Release producer, CHD and test registration enabled | `cmake --build /tmp/schwaystation-phase14-wasi-x64-exact-build --target schwaystation-run` and `stat -c 'WASI_MODULE_BYTES=%s' ...` exited 0; measured 947,969 bytes. This producer-only run did not run WASI CTest. The hosted run on the same head independently measured 947,969 and failed only because its pin was 947,899. | ✓ VERIFIED |
| Focused quality metric test | `ctest --test-dir build/ci --output-on-failure -R '^policy[.]quality_metrics$' --no-tests=error` after pin update | Exit 0; 1/1 passed in 47.15 seconds. The exact `quality.cmake` comparison for `MEASURED=947969` also exited 0. | ✓ PASS |
| Quality pin preflight | `jq -e '.size.wasm == {"floor":947969,"ceiling":947969}' quality.json`; SIO inventory sort; `git diff --check` | Each command exited 0. | ✓ PASS |
| Hosted Cachegrind instructions | `perf` job’s instructions-per-frame producer on PR run 45420382438 | Measured testcard 1,620,101 and GPU motion 138,319,540 Ir/frame; both relative comparisons passed, old exact pins rejected them, and the updated pins match | ✓ VERIFIED |

The hosted GCC build found undefined pointer arithmetic when the state serializer treated the first inner `[4]` array as an eight-byte object. The storage is now a flat eight-byte array, with a separate `port * 4 + socket` index preserving the original serialized slot order and per-port reset range. The next no-FP run exposed a C17 issue: a mutable 2D path array could not be passed to a nested `const` array pointer under `-Wpedantic`. The helper now receives a temporary array of `const char *` views. The first hosted WASI measurement was 947,976 bytes. After the watchdog-table representation fix, an arm64 local build with tests disabled measured 947,899 bytes; this was not comparable to the hosted x86_64 tests-enabled preset. The matching Ubuntu 26.04 x86_64 producer with tests registered measured 947,969 bytes, matching the hosted Main CI result. `quality.json` now tracks that exact value. Cachegrind remains pinned to its exact hosted outputs, and its relative regression limit is unchanged.

The following final-head attempt, PR run [45425051636](https://github.com/szTheory/schwaystation/actions/runs/45425051636), failed both no-FP Linux lanes in `abi.globals` on the local symbol `d name_watchdogs`. The prior `static const char *const` table was semantically constant, but its relocations caused GNU `nm` to classify the archive entry as data. The table is now fixed-width `const char` rows, preserving all eight names, indices and NUL terminators. A focused GCC 13 compile with the no-FP flag and `nm -P` reports `name_watchdogs r`; the independent source review found no behavior change. PR run [45425051636](https://github.com/szTheory/schwaystation/actions/runs/45425051636) was canceled after 219 seconds and Nightly run [45425051737](https://github.com/szTheory/schwaystation/actions/runs/45425051737) after 189 seconds once the shared cause was confirmed. GitHub's timing API returned zero per-job milliseconds for both canceled runs, so no runner-time or billing savings are claimed. The latest local workflow after this correction passed 705/705 CTest cases, produced all three macOS arm64 packages and passed release checks 8/8.

The bound refresh is producer-backed locally. Main CI run [45428490941](https://github.com/szTheory/schwaystation/actions/runs/45428490941) failed before compiling WASI because the runner's apt indexes lacked Ubuntu feeds. Same-head Nightly run [45428491372](https://github.com/szTheory/schwaystation/actions/runs/45428491372) installed the same packages and completed successfully, but it predates the pin update and does not verify the refreshed bound.

On head `d46ec1e`, Main CI run [45436318507](https://github.com/szTheory/schwaystation/actions/runs/45436318507) measured 947,969 bytes at the WASI gate and failed only because the configured ceiling was still 947,899. Setup, fixture generation, configure, runner build and WASM magic validation passed before the comparison. The exact x86_64 local producer reproduced 947,969 and the corrected local comparator passed. Main CI was canceled after 323 seconds; Nightly run [45436318741](https://github.com/szTheory/schwaystation/actions/runs/45436318741) was canceled after 292 seconds. Its fuzz job lasted 283 seconds wall, and its `Check media seeds and run bounded campaigns` step ran 180 seconds before cancellation. Nightly's WASI benchmark, all three native benchmarks and full vectors had passed. Both GitHub timing APIs returned zero billed milliseconds, so no billing or runner-time savings are claimed. `quality.json` now pins 947969; its focused policy CTest passed 1/1 in 47.15 seconds, alongside `jq -e . quality.json`, the SIO inventory sort and `git diff --check`. The corrected hosted Main CI retry remains required before ship and Phase 15.

## Probe Execution

Not applicable — this peripheral/library phase declares no standalone migration or tooling probes.

## Requirements Coverage

| Requirement | Description | Status | Evidence |
|---|---|---|---|
| SIO-01 | Both-port multitap controller/card sockets, protocol and persistence | ✓ SATISFIED | Focused serial/card/runner/libretro tests passed as part of the 16/16 selected CTest run; full 705-test workflow also passed on the unchanged implementation. |
| SIO-02 | Deterministic mouse movement, buttons, saturation/reset and host mapping | ✓ SATISFIED | `sio.special.mouse` and `libretro.input.special` passed in the 16/16 selected CTest run; the full workflow also passed. |
| SIO-03 | neGcon/Jogcon modes, integer controls and unsupported-operation refusal | ✓ SATISFIED | `sio.special.negcon`, `sio.special.jogcon` and libretro mapping passed in the 16/16 selected CTest run; the full workflow also passed. |
| SIO-04 | Dual Analog flight axes/buttons with size-tagged API and legacy compatibility | ✓ SATISFIED | `abi.api_v9`, `sio.special.flight`, libretro mapping and compatibility checks passed in the 16/16 selected CTest run; the full workflow also passed. |

**Coverage:** 4/4 requirements satisfied; no orphan Phase 14 requirements.

## Decision Coverage

Skipped cleanly — Phase 14 context has no trackable decisions; the decision-coverage query returned `skipped: true`.

## Test Quality Audit

| Test area | Disabled tests | Circular expected-value generation | Assertion coverage | Verdict |
|---|---:|---|---|---|
| API v9, SIO, card, runner, libretro and compatibility tests | 0 | No circular controller/card expectations found; wire bytes and host callback values are asserted against independent constants | Value-level and multi-step protocol/persistence assertions | ✓ PASS |

No disabled-test markers were found in the requirement-linked test directories. Deterministic state hashes are regression pins only, not external hardware accuracy evidence.

## Anti-Patterns Found

None in the Phase 14 implementation and controller/save documentation. Source scans found no unresolved `TODO`, `HACK`, `PLACEHOLDER`, `TBD`, `FIXME` or `XXX` markers. Remaining `return NULL` paths are intentional input/parse errors, not controller stubs.

## Human Verification Required

N/A — no visual UI or owner-hardware-only acceptance criterion exists. The libretro callback path and serial behavior are exercised by the automated host/core tests.

## Gaps Summary

**No implementation gaps found.** All four roadmap criteria, all 45 plan truths and all 46 declared artifacts are verified. The WASI bound matches the exact final-source x86_64 tests-enabled producer measurement (947,969 bytes); the corrected local value comparison and focused policy test pass. A 16-test focused rerun covering the roadmap criteria also passed. Cachegrind pins match their hosted producer measurements. At the time this report was first written, the corrected hosted Main CI retry remained pending. It subsequently passed on the exact corrected head in run [45442543076](https://github.com/szTheory/schwaystation/actions/runs/45442543076); Nightly [45442543657](https://github.com/szTheory/schwaystation/actions/runs/45442543657), post-merge Main CI [45454749260](https://github.com/szTheory/schwaystation/actions/runs/45454749260), post-merge Release [45454749282](https://github.com/szTheory/schwaystation/actions/runs/45454749282), release-PR Main CI [45455115354](https://github.com/szTheory/schwaystation/actions/runs/45455115354) and Release [45455115400](https://github.com/szTheory/schwaystation/actions/runs/45455115400) also passed. PR #42 and release PR #43 merged; v0.14.0 was published. Final timings and the measured CI efficiency candidate are recorded in the Phase 14 Plan 16 summary and STATE.md.

---

_Verified: 2026-11-24 16:50:33 UTC_
_Verifier: Codex; the initial goal-backward verification was produced by gsd-verifier._
