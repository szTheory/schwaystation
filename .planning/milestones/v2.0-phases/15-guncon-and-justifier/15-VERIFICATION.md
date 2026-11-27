---
phase: 15-guncon-and-justifier
verified: 2026-11-27T04:16:27Z
status: passed
score: 23/23 must-haves verified
covered_files:
  - .planning/phases/15-guncon-and-justifier/15-01-PLAN.md
  - .planning/phases/15-guncon-and-justifier/15-01-SUMMARY.md
  - .planning/phases/15-guncon-and-justifier/15-02-PLAN.md
  - .planning/phases/15-guncon-and-justifier/15-02-SUMMARY.md
  - .planning/phases/15-guncon-and-justifier/15-03-PLAN.md
  - .planning/phases/15-guncon-and-justifier/15-03-SUMMARY.md
  - .planning/phases/15-guncon-and-justifier/15-04-PLAN.md
  - .planning/phases/15-guncon-and-justifier/15-04-SUMMARY.md
  - .planning/phases/15-guncon-and-justifier/15-05-PLAN.md
  - .planning/phases/15-guncon-and-justifier/15-05-SUMMARY.md
  - .planning/phases/15-guncon-and-justifier/15-06-PLAN.md
  - .planning/phases/15-guncon-and-justifier/15-06-SUMMARY.md
  - .planning/phases/15-guncon-and-justifier/15-07-PLAN.md
  - .planning/phases/15-guncon-and-justifier/15-07-SUMMARY.md
  - .planning/phases/15-guncon-and-justifier/15-08-PLAN.md
  - .planning/phases/15-guncon-and-justifier/15-08-SUMMARY.md
  - .planning/phases/15-guncon-and-justifier/15-09-PLAN.md
  - .planning/phases/15-guncon-and-justifier/15-09-SUMMARY.md
  - .planning/phases/15-guncon-and-justifier/15-10-PLAN.md
  - .planning/phases/15-guncon-and-justifier/15-10-SUMMARY.md
  - .planning/phases/15-guncon-and-justifier/15-11-PLAN.md
  - .planning/phases/15-guncon-and-justifier/15-11-SUMMARY.md
  - .planning/phases/15-guncon-and-justifier/15-12-PLAN.md
  - .planning/phases/15-guncon-and-justifier/15-12-SUMMARY.md
  - include/schwaystation.h
  - libretro/libretro.c
  - src/instance.c
  - tests/api/v11.c
  - tests/libretro/host.c
covered_digest: "v4:sha256:d1dcaab6fba23de767914a58e5912ffd76e2e3d33abb689b8d49f21a4ca16888"
behavior_unverified: 0
overrides_applied: 0
re_verification:
  previous_status: passed
  previous_score: 23/23
  gaps_closed: []
  gaps_remaining: []
  regressions: []
advisory: []
gaps: []
---

# Phase 15: GunCon and Justifier Verification Report

**Phase Goal:** GunCon and Justifier trigger, aim and reload behave deterministically using the emulated beam and available host input.
**Verified:** 2026-11-27T04:16:27Z
**Status:** passed
**Re-verification:** Yes — current exact-head release check

## Goal Achievement

The phase 15 protocol, beam sampling and host-input paths were rechecked against release head `a2f37b46eab24f53e30f99c6b3faff6eedc8f2f8`. Full Release CI [45923155197](https://github.com/szTheory/schwaystation/actions/runs/45923155197) passed on the exact release head, and shipped source PR CI [45913678576](https://github.com/szTheory/schwaystation/actions/runs/45913678576) passed on the tested source head. The release-commit Main CI route [45923155084](https://github.com/szTheory/schwaystation/actions/runs/45923155084) is metadata-only and is not counted as full-suite evidence. The phase's core gun/frame/GPU code remains present; later libretro cheat-state additions are exercised by exact-head full Release CI. No local tests or review were rerun for this refresh.

### Observable Truths

| # | Truth | Status | Evidence |
|---:|---|---|---|
| 1 | GunCon and Justifier protocol fixtures reproduce bright, dark, edge and offscreen cases, controls, coordinates, IRQ timing, the documented threshold and GPU beam timestamp. | ✓ VERIFIED | `gpu.gun.sample`, `gpu.gun.geometry`, `gpu.gun.irq_order`, `sio.gun.guncon`, `sio.gun.justifier`, and `sio.gun.latch` passed; GPU/SIO docs specify threshold and timestamp behavior. |
| 2 | Supported LIGHTGUN/POINTER inputs map to game inputs; unsupported hosts report unsupported without false hits. | ✓ VERIFIED | `libretro.input.lightgun`, `.pointer`, `.gun_unavailable`, and `.multitap` passed on the reviewed head. |
| 3 | Public mapping covers delivered controller classes with official identifiers, frame-latched input and a reproducible integration check; compatibility ledger includes gun evidence. | ✓ VERIFIED | `libretro.input.descriptors`, gun input selectors, `sio.gun.latch`, compatibility preflight/ratchet and generated-document checks passed. |
| 4 | An exactly 228-byte released caller reaches the frame latch without reading gun bytes. | ✓ VERIFIED | `abi.api_v9` and `abi.api_v10` passed; the frozen prefix and size-tag normalization are exercised. |
| 5 | Planned gun selectors are inventoried and wired to executable tests. | ✓ VERIFIED | Six affected inventories passed `sort -cu`; the required selector preflight found 24 registered cases, with no missing or duplicate selectors. |
| 6 | Current callers submit explicit gun validity, coordinates and reload state atomically. | ✓ VERIFIED | `abi.api_v10` and `sio.gun.latch` passed; the frame latch stores each snapshot together. |
| 7 | Released input callers retain the frozen prefix and legacy behavior. | ✓ VERIFIED | `abi.api_v9`, `abi.api_v10`, and the legacy-size assertion inside `sio.gun.latch` passed. |
| 8 | Aim, trigger, offscreen/reload and selected device change together only at frame start. | ✓ VERIFIED | `sio.gun.latch` passed its pending-input and frame-boundary assertions. |
| 9 | Ports, instances and older controller classes retain independent snapshots. | ✓ VERIFIED | `sio.gun.latch` verifies independent ports and instances; `libretro.input.descriptors` and `.multitap` passed. |
| 10 | Direct-port GunCon returns ID 5A63h, active-low controls and deterministic coordinates/status. | ✓ VERIFIED | `sio.gun.guncon` passed; hardware protocol is documented in `docs/hardware/sio.md`. |
| 11 | Gun transactions preserve byte completion, ACK, abort and terminal-byte timing. | ✓ VERIFIED | `sio.gun.guncon` passed its byte and aborted-poll assertions; selected transport tests passed in the earlier phase suite. |
| 12 | Valid visible aim schedules a bounded displayed-pixel sample at its emulated beam timestamp. | ✓ VERIFIED | `gpu.gun.sample` and `gpu.gun.geometry` passed, including split/whole run consistency and mode-zero sensor scheduling. |
| 13 | Brightness and coordinates honor display geometry and pixel encoding; invalid/offscreen aim does not sample. | ✓ VERIFIED | `gpu.gun.geometry` and `gpu.gun.sample` passed. |
| 14 | Direct-port Justifier reports ID 5A31h with documented buttons. | ✓ VERIFIED | `sio.gun.justifier` passed. |
| 15 | Only an enabled bright sensor raises IRQ10 after Timer 0/1 expose the shot timestamp. | ✓ VERIFIED | `gpu.gun.irq_order` passed; the test checks timer values at the IRQ beam tick and dark/offscreen/reload suppression. |
| 16 | GunCon and Justifier use official identifiers and poll through LIGHTGUN base IDs. | ✓ VERIFIED | `libretro.input.descriptors` and `.lightgun` passed; `docs/controllers.md` records official identifiers. |
| 17 | The adapter submits one frame snapshot and unsupported hosts cannot register a hit. | ✓ VERIFIED | `libretro.input.lightgun`, `.gun_unavailable`, and `sio.gun.latch` passed. |
| 18 | POINTER hosts provide aim, pressed trigger and offscreen state through the documented fallback. | ✓ VERIFIED | `libretro.input.pointer` passed. |
| 19 | Reload, missing capability, frame changes and tap selections cannot fabricate controls or hits. | ✓ VERIFIED | `libretro.input.pointer`, `.gun_unavailable`, `.multitap`, and `sio.gun.latch` passed. |
| 20 | Save/load preserves in-flight serial and scheduled sensor behavior with fixed-width state. | ✓ VERIFIED | `sio.gun.state` and `gpu.gun.state` passed; exact state bytes were measured. |
| 21 | Pending host input is excluded and malformed sensor state is rejected atomically. | ✓ VERIFIED | `sio.gun.state`, `gpu.gun.state`, and `core.state.rejects` passed; review-fix tests reject checksum-correct guns in tap endpoints. |
| 22 | Compatibility records only exercised synthetic gun predicates and public docs match mappings. | ✓ VERIFIED | Compatibility preflight/controls/ratchet, `policy.compat_doc_fresh`, and `sh scripts/compat-doc.sh tests/compat/ledger.csv docs/compatibility.md --check` passed. |
| 23 | All registered gun cases are runnable and final reviewed sources pass the complete required workflow with measured quality evidence. | ✓ VERIFIED | The exact-head full Release CI run [45923155197](https://github.com/szTheory/schwaystation/actions/runs/45923155197) passed all required release-test and publication checks; shipped source PR CI [45913678576](https://github.com/szTheory/schwaystation/actions/runs/45913678576) also passed. |

**Score:** 23/23 truths verified (0 present, behavior-unverified)

### Required Artifacts

| Artifact | Expected | Status | Details |
|---|---|---|---|
| `include/schwaystation.h`, `src/frame.c`, `tests/api/v9.c`, `tests/api/v10.c` | Append API v10 while preserving released input prefixes | ✓ VERIFIED | Substantive code and passing v9/v10 callers. |
| `src/pad.c`, `src/pad.h`, `tests/sio/pad_tests.c` | Frame-latched direct-port gun state and serial protocol | ✓ VERIFIED | Passing latch, GunCon, Justifier and continuation tests. |
| `src/gpu.c`, `src/gpu.h`, `tests/gpu/beam.c` | Bounded beam sampling, geometry and timer-visible IRQ | ✓ VERIFIED | Passing sample, geometry, IRQ-order and sensor-state tests. |
| `libretro/libretro.c`, `tests/libretro/host.c` | Official LIGHTGUN/POINTER mapping with fail-closed host checks | ✓ VERIFIED | Passing descriptor, lightgun, pointer, unavailable-host and multitap tests. |
| `tests/compat/features.csv`, `scripts/compat-doc.sh`, `docs/compatibility.md` | Evidence tied to tested synthetic predicates | ✓ VERIFIED | Ratchet, preflight and generated-document checks pass. |
| `quality.json` | Exact measured quality bounds | ✓ VERIFIED | Exact Cachegrind, WASI size and state-byte measurements compare successfully. |

### Key Link Verification

| From | To | Via | Status | Details |
|---|---|---|---|---|
| `tests/abi/tests.cmake` | `tests/api/v10.c` | `abi.api_v10` target | ✓ WIRED | The test target compiles the caller and `abi.api_v10` passed. |
| `tests/sio/tests.cmake` | `tests/sio/pad_tests.c` → `src/pad.c` | `schwaystation-test-sio-pad` | ✓ WIRED | CMake target links the production library; GunCon/Justifier/latch/state selectors passed. |
| `tests/gpu/tests.cmake` | `tests/gpu/beam.c` → `src/gpu.c` | `schwaystation-test-gpu-beam` | ✓ WIRED | CMake target links the production library; beam, geometry, IRQ and state selectors passed. |
| `tests/libretro/tests.cmake` | `tests/libretro/host.c` → `libretro/libretro.c` | libretro test module/host | ✓ WIRED | Integration host loads the built module; descriptor and gun capability selectors passed. |
| `tests/compat/features.csv` | `docs/compatibility.md` | `scripts/compat-doc.sh --check` | ✓ WIRED | Generator output is current and the compatibility checks passed. |

The built-in key-link helper reports literal-name misses for cross-file C calls and generated test wiring. Manual source/CMake tracing confirmed the links above; exact-head full Release CI then exercised the registered integration suite.

### Data-Flow Trace (Level 4)

| Artifact | Data variable | Source | Produces real data | Status |
|---|---|---|---|---|
| `libretro/libretro.c` | `schway_input.guns[]` | Host `input_state` callbacks gated by LIGHTGUN/POINTER availability | Yes | ✓ FLOWING |
| `src/frame.c` / `src/pad.c` | latched gun snapshot and protocol state | Public frame input and SIO transaction | Yes | ✓ FLOWING |
| `src/gpu.c` | gun sensor result | Emulated beam timing and VRAM pixel contents | Yes | ✓ FLOWING |
| `tests/compat/features.csv` | synthetic feature evidence | Named passing selectors | Yes | ✓ FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|---|---|---|---|
| Current release-head gun, controller-input and save-state regression | Full Release CI run [45923155197](https://github.com/szTheory/schwaystation/actions/runs/45923155197) on `a2f37b46eab24f53e30f99c6b3faff6eedc8f2f8` | Exit success; current source plus all required release lanes passed. Source PR CI [45913678576](https://github.com/szTheory/schwaystation/actions/runs/45913678576) also passed. | ✓ PASS |
| Review regressions and serial/state behavior | `ctest --test-dir build/ci -R '^(built[.]openbios[.]shell|gpu[.]gun[.]sample|core[.]state[.]rejects|sio[.]gun[.]latch|sio[.]gun[.]guncon|sio[.]gun[.]justifier|sio[.]gun[.]state)$' --output-on-failure` | 7/7 passed in 1.58s; OpenBIOS hash is `df2704d7cf3f01ff70ab4534a443729bba77bf73e3488c836513f9f8b86a9372`. | ✓ PASS |
| GPU geometry, timer IRQ, sensor state and libretro host mapping | `ctest --test-dir build/ci -R '^(gpu[.]gun[.]geometry|gpu[.]gun[.]irq_order|gpu[.]gun[.]state|libretro[.]input[.]lightgun|libretro[.]input[.]pointer|libretro[.]input[.]gun_unavailable|libretro[.]input[.]multitap)$' --output-on-failure` | 7/7 passed in 0.12s. | ✓ PASS |
| Released API caller and appended API input | `ctest --test-dir build/ci -R '^(abi[.]api_v9|abi[.]api_v10)$' --output-on-failure` | 2/2 passed. | ✓ PASS |
| Official mapping across delivered controller classes | `ctest --test-dir build/ci -R '^libretro[.]input[.]descriptors$' --output-on-failure` | 1/1 passed. | ✓ PASS |
| Synthetic compatibility evidence and documentation | `ctest --test-dir build/ci -R '^(compat[.]features[.]preflight|compat[.]features[.]controls|compat[.]features[.]ratchet|compat[.]ratchet[.]rows|policy[.]compat_doc_fresh|docs[.]api_names|docs[.]budgets)$' --output-on-failure` | 11/11 passed in 7.59s, including four required fixture tests. | ✓ PASS |
| Stable serialized state bytes | `build/ci/tests/schwaystation-test-state state.stable_bytes` then `cmake -DQUALITY=quality.json -DMETRIC=state.bytes -DMEASURED=3757754 -P tests/cmake/quality.cmake` | Producer reported `state_bytes=3757754`; exact bound passed. | ✓ PASS |
| Cheap preflight | `git diff --check`; `LC_ALL=C sort -cu` on affected inventories; `cmake -DROOT=. -P tests/cmake/budgets.cmake`; `cmake --preset ci`; exact `ctest --show-only=json-v1` selector check | All exited 0; 27 selector entries, no missing or duplicate names. `docs/testing.md` is 61432/61440 bytes, within budget. | ✓ PASS |
| Exact quality comparisons | `quality.cmake` with testcard 1649488, gpu_motion 138450971, WASI 957144 and state.bytes 3757754 | All four exact comparisons exited 0. | ✓ PASS |
| Linux no-FP gates | Direct GCC 15.2.0 configure/build/CTest with `SCHWAYSTATION_NOFP=ON`, inventory `nofp`, on Ubuntu 26.04 arm64 and x86_64 | Build and CTest passed 23/23 on both architectures, per the orchestrator's recorded runs. | ✓ PASS |
| Format 21 state pins | `ctest --preset ci --no-tests=error -R '^fixtures[.]gpu_motion[.]state[.](150|180)$' --output-on-failure` | Exit 0; 2/2 passed in 0.67s with exact measured hashes and unchanged rendered-frame pins. | ✓ PASS |
| Rebuilt libretro controls test | `cmake --build --preset ci --target schwaystation-test-libretro-host` then `ctest --preset ci --no-tests=error -R '^libretro[.]input[.]controls$' --output-on-failure` | Target build exit 0; selector exit 0, 1/1 passed in 1.33s. | ✓ PASS |
| Full native and release workflow | `time cmake --workflow --preset ci` | Exit 0 on code-under-test head `97606163409f6be78c08b8a2249b85bb68930619`; 717/717 CTest cases passed in 71s, then 8/8 release checks passed in 1.54s; total wall 74.50s. | ✓ PASS |

The remaining rows preserve the original phase-local command results; the first row is the current exact-release-head evidence.

### Probe Execution

No phase-declared or conventional `scripts/*/tests/probe-*.sh` probes were found. The phase uses named CTest cases; relevant current-head behavior checks are recorded above.

### Advisory (New Scope, Unevidenced)

| # | Finding | Category | Why Advisory |
|---|---|---|---|
| — | None | — | No unevidenced new-scope concerns were found during re-verification. |

### Hosted Gates

The exact release head passed full Release CI [45923155197](https://github.com/szTheory/schwaystation/actions/runs/45923155197); its full release-test lane is the current hosted evidence for Windows/MSVC, WASI/BE32, RetroArch, hash and publication integration. The release-commit Main CI route [45923155084](https://github.com/szTheory/schwaystation/actions/runs/45923155084) was intentionally metadata-only (53-second hygiene job, expensive matrix skipped); it is not used as full test evidence. Source PR CI [45913678576](https://github.com/szTheory/schwaystation/actions/runs/45913678576) also passed.

Efficiency review: source PR CI took 96m06s with BE32 at 5,695s; full Release CI took 76m38s with `release-ci / be32` at 4,110s. BE32 remains the critical-path bottleneck. The proposed shard remains pending exact hosted selector/JUnit/fixture/hash coverage and before/after proof in [todo 052](../../todos/pending/052-be32-movie-test-shard.md); no unmeasured split or quality reduction is recommended. No workflow changed in this report refresh, so another `act` run would add no coverage. No local tests, Docker containers or task-owned processes were started.

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|---|---|---|---|---|
| GUN-01 | 15-02 through 15-06, 15-09 | GunCon/Justifier protocols, beam samples, threshold and IRQ timing | ✓ SATISFIED | Exact-head full Release CI passed with the registered API, SIO, GPU, save-state and OpenBIOS regressions. |
| GUN-02 | 15-02, 15-07, 15-08, 15-10, 15-12 | LIGHTGUN/POINTER mapping and fail-closed unsupported hosts | ✓ SATISFIED | Exact-head full Release CI passed with descriptor, LIGHTGUN, POINTER, unavailable-host, multitap and unsupported-device controls. |
| SIO-05 | 15-01 through 15-10 | Official identifiers, public mapping and frame-latched input | ✓ SATISFIED | Exact-head full Release CI passed with API/latch/libretro tests and generated documentation checks. |

No orphaned Phase 15 requirements were found.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|---|---:|---|---|---|
| None | — | No debt markers or stub implementations found in phase code. | — | The scanned `return NULL` branches are input/allocation error paths, not empty feature implementations. |

### Human Verification Required

None. The project’s settled verification policy is automation-first, and the phase does not require a physical light gun or owner hardware capture.

### Gaps Summary

No Phase 15 verification gaps remain. All three roadmap criteria and 23/23 truths are present and wired; required API, SIO, GPU, host mapping and state tests are covered by the successful exact-head full Release CI run. The latest Phase 15 code-review artifact is clean. Synthetic fixtures establish the documented behavior but do not claim retail-game compatibility or physical-gun calibration.

---

_Verified: 2026-11-27T04:16:27Z_
_Verifier: the agent (gsd-verifier)_
