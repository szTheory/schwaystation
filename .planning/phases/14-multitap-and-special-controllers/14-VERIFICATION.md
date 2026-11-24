---
phase: multitap-and-special-controllers
verified: 2026-11-24T14:33:28Z
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
covered_digest: "v4:sha256:b1166db94023e3d3b7cba362a2c784a25f98d5f637b1d1a3660ab4b86693cf66"
behavior_unverified: 0
overrides_applied: 0
---

# Phase 14: Multitap and Special Controllers Verification Report

**Phase Goal:** Players can use the multitap on either port and use the PlayStation mouse, neGcon, Jogcon and Dual Analog flight mode.  
**Verified:** 2026-11-24 14:33:28 UTC  
**Status:** passed  
**Re-verification:** No — initial verification.

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
| 49 | Quality bounds change only from named producer measurements, and required final verification remains explicit. | ✓ VERIFIED | Documentation/API-name, fixture-hash and producer-backed quality selectors pass in the final workflow. |

**Score:** 49/49 truths verified (0 present, behavior-unverified).

### Required Artifacts

| Artifact group | Expected | Status | Details |
|---|---|---|---|
| Phase 14 plan artifacts | Public API, SIO/pad/card/core, runner/libretro, tests, fixtures, compatibility and documentation | ✓ VERIFIED | 46/46 declared artifact paths across all plans exist and are substantive; `query verify.artifacts` returned `all_passed: true` for every plan. |
| State quality | Stable state byte bound and fixture state hashes | ✓ VERIFIED | Producer measured 3,757,638 bytes; stable-bytes check passed. Updated state hashes were directly reproduced. |
| WASI module metric | Exact linked module size | ✓ DEFERRED TO HOSTED PRODUCER | No supported local WASI compiler/linker; the existing bound is unchanged. The `wasm32` workflow’s “Measure linked WASI module” step remains the producer. |

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
| Integrated controller/card/API/runner/libretro behavior | `cmake --workflow --preset ci` | Exit 0; 705/705 CTest tests passed; all three macOS arm64 packages produced | ✓ PASS |
| Release artifact validation | Workflow CI release-test step | Exit 0; 8/8 release checks passed | ✓ PASS |
| API v9 and endpoint input mapping | `abi.api_v9`, `libretro.input.special`, `libretro.input.multitap` in final workflow | Included in 705/705 pass | ✓ PASS |
| Multitap serial/card/persistence | `sio.tap.*`, `card.tap.*`, `runner.cards.multitap`, `libretro.cards.multitap` in final workflow | Included in 705/705 pass | ✓ PASS |
| Special-controller serial and host mapping | `sio.special.*`, `libretro.input.special` in final workflow | Included in 705/705 pass | ✓ PASS |
| Compatibility ledger | `compat.features.preflight`, `compat.features.ratchet`, `compat.features.controls` | Included in 705/705 pass | ✓ PASS |

## Probe Execution

Not applicable — this peripheral/library phase declares no standalone migration or tooling probes.

## Requirements Coverage

| Requirement | Description | Status | Evidence |
|---|---|---|---|
| SIO-01 | Both-port multitap controller/card sockets, protocol and persistence | ✓ SATISFIED | Serial/card/runner/libretro tap fixtures and final 705-test workflow. |
| SIO-02 | Deterministic mouse movement, buttons, saturation/reset and host mapping | ✓ SATISFIED | Serial and libretro mouse fixtures in the final workflow. |
| SIO-03 | neGcon/Jogcon modes, integer controls and unsupported-operation refusal | ✓ SATISFIED | Native payload/refusal tests and libretro mappings in the final workflow. |
| SIO-04 | Dual Analog flight axes/buttons with size-tagged API and legacy compatibility | ✓ SATISFIED | API v9, flight serial/libretro tests, compatibility rows and final workflow. |

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

**No gaps found.** All four roadmap criteria, all 45 plan truths and all 46 declared artifacts are verified. The local WASI size and Cachegrind instructions-per-frame measurements remain with their named hosted producers; Phase 14 does not guess or widen either bound. Ready for phase completion and ship.

---

_Verified: 2026-11-24 14:33:28 UTC_
_Verifier: the agent (gsd-verifier)_

