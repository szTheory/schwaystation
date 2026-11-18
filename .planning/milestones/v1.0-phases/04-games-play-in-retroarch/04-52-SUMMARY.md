---
phase: 04-games-play-in-retroarch
plan: "52"
subsystem: hardware
tags: [dma4, spu-attr, saved-state, deterministic-testing]
requires:
  - phase: "04-51"
    provides:
      - "Instance-owned exact-halfword ATTR readback saved in bus state"
provides:
  - "Derived DMA4 RAM-to-device request readiness from saved ATTR mode bits"
  - "Bounded multiblock continuation through the existing deterministic sink"
affects: [phase-04-games-play-in-retroarch, openbios, dma, spu-attr, saved-state]
tech-stack:
  added: []
  patterns:
    - "Device readiness can be derived from existing saved state without a duplicated serialized producer field"
    - "MMIO readiness changes catch the engine up at the access tick before changing the latch"
key-files:
  created: []
  modified:
    - src/dma.c
    - src/dma.h
    - src/bus.c
    - tests/dma/dma_tests.c
    - tests/dma/tests.cmake
    - tests/dma/inventory.txt
    - tests/core/test_state.c
    - docs/hardware/dma.md
    - docs/hardware/bus.md
key-decisions:
  - "Derive DMA4 RAM-to-device readiness from saved ATTR bits 5-4 equal to mode 2; keep manual DREQ credits independent."
  - "Keep the existing deterministic sink, unit cost, serialized fields, and state format revision; do not model FIFO latency, STATX, SPU RAM, or audio."
requirements-completed: []
coverage:
  - id: D1
    description: "ATTR-driven DMA4 request blocks transfer RAM words through the deterministic sink with the existing count, cost, CRC, and completion behavior."
    requirement: GAME-05
    verification:
      - kind: unit
        ref: "dma.spu.request_blocks"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci"
        status: pass
    human_judgment: false
  - id: D2
    description: "DMA4 mode, stop, deadline, instance isolation, and saved-state controls preserve manual request and existing DMA2 behavior."
    requirement: GAME-05
    verification:
      - kind: unit
        ref: "dma.spu.request_controls; core.state.device_fields; core.state.rejects; core.state.round_trip"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci"
        status: pass
    human_judgment: false
actuals:
  tokens: 5086
  tasks: 2
  commits: 3
plan_head_before: c6dc165655311093ac0e3d853e408fb0be1949bf
plan_head_after: 3fbc91104d0ad85fbd0322ee0b988122d5eef208
duration: 18m 38s
completed: 2026-11-11
status: complete
---

# Phase 4 Plan 52: DMA4 ATTR Readiness Summary

**ATTR mode now drives bounded DMA4 request blocks through the deterministic sink while preserving credit, deadline, cost, and saved-state behavior.**

## Accomplishments

- Added persistent, derived readiness for sync-mode-1 DMA4 RAM-to-device blocks when ATTR bits 5-4 select mode 2. Manual DREQ credits and DMA2 readiness remain independent.
- Made ATTR writes catch DMA up at the MMIO tick before updating the latch. Atomic units finish, future read-ahead deadlines remain in force, and the next block is reconsidered at its boundary.
- Added three-block and 757-block known-answer regressions, mode/stop/direction/DPCR controls, instance isolation, split-run equivalence, and saved-state checks. The 757-block run transfers 12,112 words in 57,532 modeled cycles.
- Documented the deterministic unbuilt-sink policy and its limits. No new state field or revision was needed.

## Files Created/Modified

- `src/dma.c` — Derived DMA4 readiness, ATTR wake, and the future-deadline start guard.
- `src/dma.h` — Clarified explicit DREQ credits and declared the ATTR wake entry point.
- `src/bus.c` — Catches the engine up before storing ATTR and recalculates scheduling at the same tick.
- `tests/dma/dma_tests.c` — Added request-block and control/state regressions.
- `tests/dma/tests.cmake` — Registered the two DMA4 tests.
- `tests/dma/inventory.txt` — Added byte-sorted inventory rows.
- `tests/core/test_state.c` — Verifies ATTR restoration over a different destination value and continuation without a serialized readiness field.
- `docs/hardware/dma.md` — Describes the modeled request policy and block boundaries.
- `docs/hardware/bus.md` — Documents the first ATTR-driven DMA4 caller.

## Task Commits

1. **Task 1 RED:** `b2df5ab` — Added the bus-driven failing DMA4 request regression; its named CTest assertion failed because ATTR did not start DMA4.
2. **Task 1 GREEN:** `5c82787` — Implemented ATTR-derived readiness, timestamped wake, state coverage, and hardware documentation.
3. **Task 2:** `3fbc911` — Added stop, deadline, mode, 757-block, split-run, and instance controls.

## Decisions Made

- ATTR bits 5-4 select a derived ready condition only for channel 4, sync mode 1, and RAM-to-device direction. The condition does not set or consume the explicit DREQ credit field.
- Preserve the existing deterministic sink and modeled 76-cycle cost per 16-word block. This does not model SPU FIFO timing, STATX, sample RAM, or audio.
- Keep state format revision 4 unchanged because the saved ATTR latch and existing DMA engine fields fully determine readiness.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Test Bug] Corrected DMA4 direction, address, and access-tick assertions**
- **Found during:** Tasks 1 and 2
- **Issue:** A device-to-RAM block does not increment the device-write sink counter; the state test's two-block final address was one block short; and the timed bus helper initially mapped the requested access one tick early.
- **Fix:** Asserted MADR and zero-filled RAM for reverse direction, corrected the final address, and matched the helper to the scheduler's cycle indexing.
- **Files modified:** `tests/dma/dma_tests.c`, `tests/core/test_state.c`
- **Verification:** Focused DMA/state checks and full CI passed.
- **Committed in:** `3fbc911` (the state-address correction is included in `5c82787`).

**Total deviations:** 1 auto-fixed test issue (Rule 1)
**Impact on plan:** Test expectations and test timing were corrected; no product scope was added.

## Evidence

- RED: `ctest --test-dir build/ci -R '^dma\.spu\.request_blocks$' --no-tests=error --output-on-failure --output-junit build/ci/04-52-red.xml` exited 8 on the named behavioral assertions. `gsd-tools check tdd-red-evidence build/ci/04-52-red-evidence.json --raw` exited 0 with `RED_EVIDENCE_OK`.
- Tracer GREEN: `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-dma schwaystation-test-state && ctest --test-dir build/ci -R '^(dma\.(spu\.request_blocks|sync1\.(dreq_blocks|no_dreq_no_stall)|gpu\..*)|core\.state\.device_fields)$' --no-tests=error --output-on-failure` exited 0; all 5 selected cases passed.
- Control/state checks: the focused `dma.spu.request_controls` and core state round-trip/rejection run exited 0; all 4 selected cases passed.
- `cmake --workflow --preset ci` exited 0; all 475 tests passed, including release archive and firmware-scan checks.
- `cmake --workflow --preset hygiene` exited 0; all 23 checks passed.

## Performance

- **Duration:** 18m 38s
- **Started:** 2026-11-11T22:22:30Z
- **Completed:** 2026-11-11T22:41:08Z
- **Tasks:** 2
- **Files modified:** 9

## Issues Encountered

The implementation and focused controls passed after correcting test expectations noted above. No source issue remains.

## User Setup Required

None.

## Next Phase Readiness

The DMA4 upload request path now progresses through the deterministic sink. Full GAME-05 remains pending: 04-40 still owns actual shell inspection and same-run portable acceptance. This plan accepts no shell frame or hash; no owner action is needed for this plan.

## Self-Check: PASSED

- All nine plan files and this summary exist.
- RED, GREEN, and control-test commits are ancestors of the current branch head.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-11*
