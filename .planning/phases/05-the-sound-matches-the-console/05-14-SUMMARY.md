---
phase: 05-the-sound-matches-the-console
plan: "14"
subsystem: SPU state
tags:
  - save-state
  - PCM
  - validation
requires:
  - phase: 05-the-sound-matches-the-console
    provides:
      - Active BRR voice, capture and reverb paths
      - FIFO/DMA4 state and revision-5 state format
provides:
  - Byte-identical PCM continuation through fixture voice and noise state load
  - Validation of every active queued SPU register address before state apply
  - Atomic malformed-state refusal including preservation of queued host audio
affects:
  - Phase 5 verification
  - SPU save-state loading
actuals:
  tasks: 2
  commits: 2
  plan_head_before: f37860fc452640b87ef90a949a645a79b90e97b9
  plan_head_after: 6ff4ae13770ec10c581198ef5d2c8e77adf00915
tech-stack:
  added: []
  patterns:
    - Compare fixture-backed PCM frame by frame across save/load continuation
    - Validate queued guest register offsets from the state wire before applying state
key-decisions:
  - Permit only aligned queued writes that runtime register writes can queue; reject out-of-image, FIFO, STATX and current-volume offsets.
  - Keep host audio outside serialized state, clearing it after valid load and preserving it after refused loads.
  - Leave state layout, revision 5 and interim state goldens unchanged because this change adds validation and tests without changing serialized fields.
requirements-completed: []
coverage:
  - id: D1
    description: A live stub-booted BRR voice with a pending noise-source write and active capture resumes to byte-identical future PCM after save and load.
    requirement: SOUND-01
    verification:
      kind: integration
      ref: "cmake --preset ci (0); cmake --build --preset ci --target schwaystation-test-state schwaystation-test-spu (0); ctest --test-dir build/ci -R '^core\\.state\\.|^spu\\.model$' --no-tests=error --output-on-failure (9/9 passed)"
      status: pass
    human_judgment: false
  - id: D2
    description: Active reverb feedback and partial DMA4 state continue through the modeled SPU request gap, with matching PCM and saved machine state.
    requirement: SOUND-01
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^core\\.state\\.|^spu\\.(model|reverb)$' --no-tests=error --output-on-failure (10/10 passed)"
      status: pass
    human_judgment: false
  - id: D3
    description: Malformed active queued addresses (range, alignment, FIFO, STATX, current-volume and later queue slots) reject without changing machine state or host audio; revision-5 refusal and the 4 MiB ceiling remain covered.
    requirement: SOUND-02
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^core\\.state\\.|^abi\\.api_v1$|^spu\\.(model|reverb)$' --no-tests=error --output-on-failure (11/11 passed; state cases include old-format refusal and the 4 MiB ceiling, plus frozen API v1)"
      status: pass
    human_judgment: false
  - id: D4
    description: State fixture hashes and the public state format remain unchanged because no serialized field or order changed.
    requirement: SOUND-02
    verification:
      kind: manual_procedural
      ref: "Reviewed src/spu.c state field walk and tests/fixtures/tests.cmake; this plan only tightens pre-apply validation and adds continuations, so no state golden update is applicable."
      status: pass
    human_judgment: false
duration: "about 20 minutes wall from the persisted Plan 05-12 closeout; exact active time across restart is unavailable"
completed: 2026-11-14
status: complete
---

# Phase 05 Plan 14: Active SPU state continuation Summary

**Active SPU state now resumes through save/load with matching PCM, while malformed queued register writes are rejected before any instance data is applied.**

## Performance

- **Duration:** About 20 minutes wall from the preceding persisted plan closeout; exact active time across restart is unavailable.
- **Started:** 2026-11-14T09:50:40Z (Plan 05-12 closeout baseline)
- **Completed:** 2026-11-14T10:10:52Z
- **Tasks:** 2
- **Files modified:** 3
- **Runtime/model:** Codex, OpenGSD 1.21.0; GPT-6 session (exact model ID not exposed)

## Accomplishments

- Added a fixture-backed save/load continuation across 90 frames, comparing complete PCM bytes for every frame after a live BRR voice, noise source, and capture cursor had advanced.
- Confirmed a valid load clears queued host samples, while refused loads preserve the full host audio ring and every serialized machine field.
- Closed the saved pending-register address bounds: every active offset must be aligned, inside the SPU register image, runtime-writable, and not FIFO, STATX, or current volume.
- Updated the DMA4 state continuation to exercise the modeled wait until the next SPU request before the next block begins; existing reverb feedback continuation also passes.
- Preserved state revision 5, the old-format refusal, frozen API v1 caller, and 4 MiB ceiling. No serialized fields changed, so no state hash goldens were updated.

## Task Commits

1. **Task 1 RED:** b5d4806 — added active PCM, capture, valid upper register-offset, and malformed queued-address continuation cases; the focused state test rejected no malformed pending address yet.
2. **Task 2 GREEN:** 6ff4ae1 — validate queued offsets directly from saved bytes before applying state; focused state, model and reverb checks passed.

## Files Created/Modified

- `src/spu.c` — validate active pending SPU register offsets before apply.
- `tests/core/test_state.c` — exercise malformed address refusal, atomic audio-ring preservation, valid queued boundary offsets, and partial DMA4 continuation through the request gap.
- `tests/spu/model.c` — compare the live fixture's future PCM bytes across valid save/load with active voice, noise, and capture state.

## Decisions and Deviations

- The state test still referenced a bus-owned ATTR mirror removed by Plan 05-23. Updated its observation helper to read the SPU's pending and committed ATTR values, and adjusted the DMA continuation to follow the actual SPU request deadline.
- State field order, serialized size, and revision remain unchanged. Interim GPU state pins therefore remain valid.
- SOUND-01 and SOUND-02 remain pending for phase-level verification.
