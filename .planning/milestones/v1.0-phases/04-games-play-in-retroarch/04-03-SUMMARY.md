---
phase: 04-games-play-in-retroarch
plan: "03"
subsystem: gpu
tags: [gpu, gp0, vram, save-state, api-v4]

requires:
  - phase: 04-games-play-in-retroarch
    plan: "01"
    provides: GPU test registration and inventory seams.
  - phase: 04-games-play-in-retroarch
    plan: "02"
    provides: CPU and bus behavior needed by GPU MMIO callers.
provides:
  - Bounded per-instance GPU command state routed from CPU MMIO.
  - GP0 fill, register queries, IRQ acknowledgement, VRAM transfer state, and atomic state validation.
  - API v4 read-only access to the complete native VRAM span.
affects: [gpu, display, save-state, api]

actuals:
  tokens: 13871
  tasks: 2
  commits: 6
plan_head_before: 7314824b7aa062ffc9a4f41e106a379088e51424
plan_head_after: 844e7592393a449957ea0dbdaecc719e05d65f0e

tech-stack:
  added: []
  patterns:
    - One bounded GPU word-entry path serves bus writes and future DMA callers.
    - GPU state owns VRAM and parser fields in the existing validate-then-apply state walk.

key-files:
  created:
    - src/gpu.c
    - src/gpu.h
    - tests/api/v4.c
    - tests/gpu/gpu_tests.c
  modified:
    - CMakeLists.txt
    - docs/embedding.md
    - include/schwaystation.h
    - src/bus.c
    - src/instance.c
    - src/internal.h
    - src/state.c
    - tests/abi/inventory.txt
    - tests/abi/tests.cmake
    - tests/bus/bus_tests.c
    - tests/core/bus_unbuilt.c
    - tests/core/test_run.c
    - tests/core/test_state.c
    - tests/embed/CMakeLists.txt
    - tests/gpu/inventory.txt
    - tests/gpu/tests.cmake

key-decisions:
  - Preserve one bounded GPU word-entry path for CPU MMIO and later DMA callers.
  - Keep API v4 to a read-only span over native VRAM and leave frozen API v1-v3 callers unchanged.
  - Validate a saved partial packet using its candidate FIFO command before applying any state.

patterns-established:
  - State validation checks GPU packet, transfer phase, dimensions, and coordinates before apply.
  - The public VRAM pointer stays instance-owned and stable for the instance lifetime.

requirements-completed: []
coverage:
  - id: D1
    description: CPU GPU MMIO uses a bounded decoder for fill, environment queries, reset, IRQ, and VRAM transfers.
    requirement: GAME-01
    verification:
      - kind: unit
        ref: gpu.decoder.fill_mmio, gpu.decoder.partial_packet_reset, gpu.decoder.register_images via cmake --workflow --preset ci
        status: pass
      - kind: integration
        ref: cmake --workflow --preset ci
        status: pass
    human_judgment: false
  - id: D2
    description: GPU partial packets resume after save/load, while malformed FIFO, phase, and coordinate values are rejected without changing the target instance.
    requirement: GAME-01
    verification:
      - kind: unit
        ref: gpu.state.partial_fill via ctest --test-dir build/ci -R gpu.state.partial_fill
        status: pass
      - kind: unit
        ref: core.state.rejects, core.state.visit, and core.state.record_sweep via cmake --workflow --preset ci
        status: pass
    human_judgment: false
  - id: D3
    description: API v4 exposes the stable read-only 1024x512 native VRAM span and preserves existing API callers.
    requirement: GAME-01
    verification:
      - kind: unit
        ref: abi.api_v1, abi.api_v2, abi.api_v3, abi.api_v4, and abi.api_version via cmake --workflow --preset ci
        status: pass
      - kind: integration
        ref: docs.api_names via cmake --workflow --preset ci
        status: pass
    human_judgment: false

duration: 47min
completed: "2026-11-10"
status: complete
---

# Phase 04 Plan 03: GPU Decoder, State, and VRAM API Summary

**A bounded per-instance GPU command path now fills VRAM through MMIO, resumes partial packets safely, and exposes native VRAM through append-only API v4.**

## Performance

- **Duration:** 47 min
- **Started:** 2026-11-10T18:22:43Z
- **Completed:** 2026-11-10T19:09:24Z
- **Tasks:** 2
- **Files modified:** 20

## Accomplishments

- Added a fixed-capacity GPU packet decoder and per-instance register/transfer state. GP0 fills, environment commands, GP1 reset/query, IRQ acknowledgement, upload/readback, and side-effect-free peeks now use the real bus path.
- Moved VRAM and GPU parser fields into one state visitor walk. Partial packets resume after load, and invalid FIFO, phase, or coordinate state is rejected before any field is applied.
- Added API v4 `schway_span` and `schway_get_vram` for a stable, read-only view of the full 1024 by 512 native VRAM. API v1-v3 tests remain frozen and pass.

## Task Commits

1. **Trace GP0 fill from CPU MMIO to native VRAM** — `360892c` (RED), `3ad4f63` (RED), `d501e88` (implementation).
2. **Expose VRAM and round-trip every parser field under API4** — `5cd1095` (RED), `bcbfd55` (implementation), `844e759` (hygiene fix).

## Files Created/Modified

- `src/gpu.c`, `src/gpu.h`, and `src/bus.c` add the GPU module and connect its bounded word path to MMIO.
- `src/state.c`, `src/internal.h`, `src/instance.c`, and `include/schwaystation.h` own and expose GPU state, state format 4, behavior revision 4, and the API v4 VRAM span.
- `tests/gpu/`, `tests/api/v4.c`, ABI registration, and the existing state tests cover the new bus, serialization, and public API behavior.
- `docs/embedding.md` describes the new public VRAM API.

## Decisions Made

- Commands without a renderer remain consumed without pretending to draw; this plan implements the first real VRAM primitive, GP0 fill.
- API v4 adds only a read-only native VRAM span. Existing size-tagged structures and API v1-v3 callers are unchanged.
- GPU state validation checks the saved FIFO command from the candidate buffer, so loading a partial packet into a fresh instance does not depend on the target's previous FIFO contents.

## TDD RED Evidence

- `gpu.decoder.fill_mmio` failed because the expected red pixels at VRAM `(7,16)`, `(7,47)`, and `(8,16)` remained zero. Evidence: `build/ci/tdd/04-03-task1-red.json` and `build/ci/build/ci/tdd/04-03-task1-red.xml`.
- `gpu.decoder.register_images` failed on the expected GPUSTAT, environment query, mask, and IRQ images. Evidence: `build/ci/tdd/04-03-task1-reg-red.json` and `build/ci/build/ci/tdd/04-03-task1-reg-red.xml`.
- `abi.api_v4` failed because the public macro was still version 3. `gpu.state.partial_fill` failed because the fill did not resume and GPU parser fields were not in the state visitor. Evidence records: `build/ci/tdd/04-03-task2-red-api.json`, `build/ci/tdd/04-03-task2-red-gpu.json`; reporter: `build/ci/build/ci/tdd/04-03-task2-red.xml`. Each named target failed on its planned assertion; no passing control was reported as RED.

## Verification

- `cmake --workflow --preset ci` — exit 0; all 344 tests and both release archive checks passed.
- `cmake --workflow --preset hygiene` — exit 0; all 23 checks passed.
- Focused API, GPU-state, and existing state selection — exit 0; all 11 selected tests passed.
- The tracer feedback gate reran the full CI workflow and focused fill case after Task 1 — exit 0.
- No hosted CI run URL was produced.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Updated existing MMIO ownership expectations**
- **Found during:** Task 1 CI integration.
- **Issue:** GPU ports are now owned, so prior tests and embedding inventory still treated them as unbuilt.
- **Fix:** Kept the width-sensitive queue test on an unbuilt MDEC port, updated the unbuilt bus map, and added the GPU object to the embedding target allowlist.
- **Files modified:** `tests/bus/bus_tests.c`, `tests/core/bus_unbuilt.c`, `tests/embed/CMakeLists.txt`.
- **Verification:** Full CI passed.
- **Committed in:** `d501e88`.

**2. [Rule 1 - Bug] Validated the candidate FIFO command during state loading**
- **Found during:** Task 2 review.
- **Issue:** The validation pass could otherwise compare a saved partial packet against the target instance's previous FIFO word.
- **Fix:** Read the candidate command from the state buffer before checking packet length.
- **Files modified:** `src/gpu.c`.
- **Verification:** Partial-packet restore and malformed-state tests passed.
- **Committed in:** `bcbfd55`.

**3. [Rule 3 - Blocking] Aligned state tests with the expanded visitor**
- **Found during:** Task 2 CI integration.
- **Issue:** GPU state raised the visitor field count to 140, exceeding existing test buffers capped at 128; revision checks still expected 3.
- **Fix:** Raised both existing state-test capacities to 256 and updated the explicit state and behavior revision assertions to 4.
- **Files modified:** `tests/core/test_state.c`, `tests/core/test_run.c`.
- **Verification:** All 11 focused API/GPU/state tests and full CI passed.
- **Committed in:** `bcbfd55`.

**4. [Rule 3 - Blocking] Documented the API v4 names**
- **Found during:** Task 2 full CI.
- **Issue:** `docs.api_names` requires each public header name in the embedding guide.
- **Fix:** Added the VRAM span contract and API v4 version note.
- **Files modified:** `docs/embedding.md`.
- **Verification:** `docs.api_names` and full CI passed.
- **Committed in:** `bcbfd55`.

**5. [Rule 3 - Blocking] Reworded the GPU source citation for hygiene**
- **Found during:** Plan hygiene verification.
- **Issue:** The floating-point scanner interpreted decimal section numbers in a comment as numeric syntax.
- **Fix:** Spelled the section numbers in words.
- **Files modified:** `src/gpu.c`.
- **Verification:** `hygiene.no_float` and the full hygiene workflow passed.
- **Committed in:** `844e759`.

**Total deviations:** 5 auto-fixed (1 Rule 1 bug, 4 Rule 3 blocking integration issues).
**Impact on plan:** These fixes were required for saved-state correctness, existing test compatibility, API documentation, and the repository hygiene gate. No dependencies or unrelated behavior were added.

## Issues Encountered

The first full Task 2 CI run reported state-test capacity/revision assumptions and the missing API documentation; those were corrected before the passing run. The first hygiene run found the numeric source citation; the exact failing check passed after rewording and the full workflow then passed. No issues remain.

## User Setup Required

None.

## Next Phase Readiness

The first real GPU fill now reaches native VRAM through CPU MMIO, and the public/state contracts can support later renderer and display work. Broader primitive rendering and display scan-out remain for later Phase 4 plans. No user action is needed.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-10*

## Self-Check: PASSED

All 20 changed files and six task commits are present; the commits are ancestors of HEAD and `git diff --check 7314824..HEAD` passed.
