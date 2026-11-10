---
phase: 04-games-play-in-retroarch
plan: "04"
subsystem: gpu
tags: [gpu, vram, transfer, cache, save-state]

requires:
  - phase: 04-games-play-in-retroarch
    plan: "01"
    provides: GPU test registration and inventory seams.
  - phase: 04-games-play-in-retroarch
    plan: "03"
    provides: Shared GPU decoder, upload/readback cursors, and validated GPU state.
provides:
  - Ordered GP0(80h-9Fh) VRAM copies with independent wrapping and E6 mask behavior.
  - Instance-owned texture and CLUT lookup caches, invalidation, and serialized cache state.
  - Bus-level transfer and cache regression tests for the next GPU rendering plan.
affects: [gpu, rendering, vram, save-state]

actuals:
  tokens: 8348
  tasks: 2
  commits: 3
plan_head_before: 4eb1396eaae10c21137cb8baab497113864d242e
plan_head_after: b1e2f7df8534c97386a1698e2b0615f88779fdf6

tech-stack:
  added: []
  patterns:
    - Read and write each copy pixel in row-major order so overlap behavior is explicit.
    - Keep cache tags and contents per GPU instance and include them in validate-before-apply state.

key-files:
  created: []
  modified:
    - src/gpu.c
    - src/gpu.h
    - tests/gpu/transfers.c
    - tests/gpu/gpu_tests.c
    - tests/gpu/tests.cmake
    - tests/gpu/inventory.txt

key-decisions:
  - Apply E6 skip-mask to the old destination before force-mask is added to copy and upload values; GP0 fill continues to ignore E6.
  - Keep texture and CLUT cache data instance-owned and serialized, with GP0(01h) clearing texture entries and GP1(01h) invalidating the CLUT row.
  - Treat cache behavior as a model contract for the renderer, not a hardware-measured claim.

requirements-completed: []
coverage:
  - id: D1
    description: A0 upload and C0 readback preserve independent wrapping, odd-word padding, reset, saved cursor, and malformed-cursor rejection behavior.
    requirement: GAME-01
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci (exit 0; 354 tests and 2 release archive checks)"
        status: pass
      - kind: integration
        ref: "ctest --test-dir build/ci -L '^gpu$' --no-tests=error --output-on-failure (exit 0; 16 tests)"
        status: pass
    human_judgment: false
  - id: D2
    description: VRAM copies read and write pixels in observable row-major order across horizontal, vertical, and diagonal overlaps, apply E6 masks, and wrap X and Y independently.
    requirement: GAME-01
    verification:
      - kind: integration
        ref: "gpu.transfer.copy_order, gpu.transfer.copy_masks, gpu.transfer.copy_wrap in cmake --workflow --preset ci (exit 0)"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci (exit 0; 354 tests and 2 release archive checks)"
        status: pass
    human_judgment: false
  - id: D3
    description: Per-instance texture and CLUT caches expose stale-hit, address-change, GP0/GP1 invalidation, 256-entry wrapping, and save/load behavior; malformed cache state is rejected atomically.
    requirement: GAME-01
    verification:
      - kind: unit
        ref: "gpu.transfer.cache_contract and gpu.state.cache_roundtrip in cmake --workflow --preset ci (exit 0)"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci (exit 0; 354 tests and 2 release archive checks)"
        status: pass
    human_judgment: false

duration: 34min
completed: 2026-11-10
status: complete
---

# Phase 4 Plan 4: GPU transfer and cache semantics summary

**Ordered overlapping VRAM copies and instance-owned texture/CLUT caches now extend the bus-backed GPU transfer contract.**

## Performance

- **Duration:** 34 min
- **Started:** 2026-11-10T19:12:17Z
- **Completed:** 2026-11-10T19:46:29Z
- **Tasks:** 2
- **Files modified:** 6

## Accomplishments

- Reused the A0/C0 decoder and cursor implementation from 04-03, adding bus-level contract tests for odd halfwords, independent axis wrapping, partial state, reset, and atomic rejection of invalid saved cursors.
- Added GP0(80h-9Fh) VRAM copies that read and write in row-major order, apply skip-mask to the prior destination before forcing bit 15, and do not use draw-area clipping.
- Added fixed-size per-instance texture and CLUT lookup storage, explicit GP0/GP1 invalidation, stale-hit and different-address tests, and cache save/load validation.

## Task Commits

1. **Task 1: Trace CPU upload to VRAM-to-CPU readback** — e3bc809 (test; existing transfer implementation passed the new contract checks).
2. **Task 2: Trace overlapping copy and stale CLUT reads** — 13653c4 (RED tests), b1e2f7d (copy and cache implementation with GREEN coverage).

## Files Created/Modified

- src/gpu.c and src/gpu.h add ordered copy operations, mask helpers, and cache data/lookup/invalidation behavior.
- tests/gpu/transfers.c covers bus transfers, overlap directions, wrapping, copy/upload masks, fill mask exceptions, and stale cache reads.
- tests/gpu/gpu_tests.c, tests/gpu/tests.cmake, and tests/gpu/inventory.txt cover cache state restoration, atomic rejection, and sorted test registration.

## Decisions Made

- Copies process each source and destination coordinate in row-major order, so overlapping reads observe earlier writes in the same packet.
- Cache behavior is a deterministic contract for the next rendering plan; no hardware-measured claim is made.

## TDD RED Evidence

- Task 1's A0/C0 implementation was already present from 04-03. New bus tests passed the existing behavior; no artificial failing test was created.
- Task 2's gpu.transfer.copy_order test failed on the planned destination VRAM assertions because GP0(80h) copies were not yet dispatched. The RED classifier returned RED_EVIDENCE_OK for build/ci/tdd/04-04-copy-red.json, whose fresh JUnit report is build/ci/build/ci/tdd/04-04-copy-red.xml.
- The copy, cache, and state tests passed after implementation. No separate refactor was needed.

## Verification

- cmake --workflow --preset ci — exit 0; all 354 CI tests and both release archive checks passed.
- ctest --test-dir build/ci -L '^gpu$' --no-tests=error --output-on-failure — exit 0; all 16 GPU tests passed before the final mask assertions; those assertions also passed in the final CI run.
- cmake --workflow --preset hygiene — exit 0; all 23 checks passed.
- No hosted CI run URL was produced.

## Deviations from Plan

The plan's A0/C0 behavior was already implemented by its dependency, 04-03. This plan verified and locked it with the requested contract tests instead of duplicating the decoder. The remaining copy and cache behavior was implemented as planned.

## Issues Encountered

A reset test initially used a GP0 word that decoded as a command packet. It was replaced with a NOP word, and the corrected test passed in the focused GPU suite and final CI run.

## User Setup Required

None.

## Next Phase Readiness

Plan 04-05 can use the per-instance texture and CLUT lookup helpers. Cache behavior remains a modeled contract until the later pinned GPU oracle plan verifies it.

## Self-Check: PASSED

- Summary file exists at the required phase path.
- Task commits e3bc809, 13653c4, and b1e2f7d are ancestors of the current branch head.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-10*
