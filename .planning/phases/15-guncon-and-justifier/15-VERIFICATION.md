---
phase: 15-guncon-and-justifier
verified: 2026-11-25T18:31:40Z
status: passed
score: 23/23 must-haves verified
covered_files:
  - .planning/AUTONOMOUS.md
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
  - .planning/phases/15-guncon-and-justifier/15-REVIEW-FIX.md
  - .planning/phases/15-guncon-and-justifier/15-REVIEW.md
  - .planning/phases/15-guncon-and-justifier/deferred-items.md
  - AGENTS.md
  - docs/compatibility.md
  - docs/controllers.md
  - docs/embedding.md
  - docs/hardware/gpu.md
  - docs/hardware/sio.md
  - docs/libretro.md
  - docs/saves.md
  - include/schwaystation.h
  - libretro/libretro.c
  - quality.json
  - scripts/compat-doc.sh
  - src/frame.c
  - src/gpu.c
  - src/gpu.h
  - src/internal.h
  - src/pad.c
  - src/pad.h
  - tests/abi/inventory.txt
  - tests/abi/tests.cmake
  - tests/api/v10.c
  - tests/api/v9.c
  - tests/compat/features.csv
  - tests/compat/tests.cmake
  - tests/core/test_core.c
  - tests/core/test_state.c
  - tests/fixtures/tests.cmake
  - tests/gpu/beam.c
  - tests/gpu/inventory.txt
  - tests/gpu/tests.cmake
  - tests/libretro/host.c
  - tests/libretro/inventory.txt
  - tests/libretro/tests.cmake
  - tests/sio/inventory.txt
  - tests/sio/pad_tests.c
  - tests/sio/tests.cmake
covered_digest: "v4:sha256:7c26c1f5a6d5d93517755cc302c622fda008af187164e385f0aedddb8734fc84"
behavior_unverified: 0
overrides_applied: 0
re_verification:
  previous_status: gaps_found
  previous_score: 22/23
  gaps_closed:
    - "GCC 15.2 no-FP warnings-as-errors in GPU sensor state I/O."
    - "Exact Cachegrind and WASI bounds no longer matching current-source measurements."
    - "Format 21 gpu_motion state hashes at frames 150 and 180 now match exact runner outputs; rendered-frame pins are unchanged."
    - "libretro.input.controls now uses an unsupported device in its invalid-selection control, and the changed test executable was rebuilt before the focused pass."
  gaps_remaining: []
  regressions: []
advisory: []
gaps: []
---

# Phase 15: GunCon and Justifier Verification Report

**Phase Goal:** GunCon and Justifier trigger, aim and reload behave deterministically using the emulated beam and available host input.
**Verified:** 2026-11-25T18:31:40Z
**Status:** passed
**Re-verification:** Yes — after no-FP and quality-gap closure

## Goal Achievement

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
| 23 | All registered gun cases are runnable and final reviewed sources pass the complete required local workflow with measured quality evidence; hosted Main/Release checks remain ship gates. | ✓ VERIFIED | The exact Format 21 state pins, `libretro.input.controls`, the full 717-test native workflow and 8 release checks passed after exact-head review; hosted gates remain required during ship. |

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

### Probe Execution

No phase-declared or conventional `scripts/*/tests/probe-*.sh` probes were found. The phase uses named CTest cases; relevant current-head behavior checks are recorded above.

### Advisory (New Scope, Unevidenced)

| # | Finding | Category | Why Advisory |
|---|---|---|---|
| — | None | — | No unevidenced new-scope concerns were found during re-verification. |

### Hosted Gates

No GitHub workflow files changed, and the complete local workflow already covered the reproducible test and release checks, so `act` would add no job-step coverage. Windows/MSVC, hosted WASI and BE32 integration, RetroArch, required CI timing, and release publication checks remain required for ship.

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|---|---|---|---|---|
| GUN-01 | 15-02 through 15-06, 15-09 | GunCon/Justifier protocols, beam samples, threshold and IRQ timing | ✓ SATISFIED | Passing API, SIO, GPU, save-state and OpenBIOS regression checks. |
| GUN-02 | 15-02, 15-07, 15-08, 15-10, 15-12 | LIGHTGUN/POINTER mapping and fail-closed unsupported hosts | ✓ SATISFIED | Passing descriptor, LIGHTGUN, POINTER, unavailable-host, multitap and unsupported-device controls. |
| SIO-05 | 15-01 through 15-10 | Official identifiers, public mapping and frame-latched input | ✓ SATISFIED | Passing API/latch/libretro tests and generated documentation checks. |

No orphaned Phase 15 requirements were found.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|---|---:|---|---|---|
| None | — | No debt markers or stub implementations found in phase code. | — | The scanned `return NULL` branches are input/allocation error paths, not empty feature implementations. |

### Human Verification Required

None. The project’s settled verification policy is automation-first, and the phase does not require a physical light gun or owner hardware capture.

### Gaps Summary

No local Phase 15 verification gaps remain. The Format 21 state pins match the current runner output, the invalid-device libretro control now uses an unsupported device and passes after rebuilding its test target, and the complete local workflow passes with all 717 test cases and 8 release checks. The latest exact-head review is clean. Hosted Main CI and Release checks remain mandatory during ship; no retail-game compatibility or physical-gun calibration is inferred from the synthetic fixtures.

---

_Verified: 2026-11-25T18:31:40Z_
_Verifier: the agent (gsd-verifier)_
