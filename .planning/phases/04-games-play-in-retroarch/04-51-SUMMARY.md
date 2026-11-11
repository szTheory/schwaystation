---
phase: 04-games-play-in-retroarch
plan: "51"
subsystem: bus
tags: [mmio, spu, state, ctest]
requires:
  - phase: "04-23"
    provides: Runner BIOS loading
  - phase: "04-39"
    provides: Pinned OpenBIOS build and handoff
  - phase: "04-50"
    provides: OpenBIOS POST byte compatibility
provides:
  - Exact halfword ATTR control readback at physical 1F801DAA
  - Instance-owned ATTR state with save, load and emulator-reset coverage
affects: [04-40 boot acceptance, SPU implementation]
actuals:
  tokens: 4281
  tasks: 2
  commits: 2
  plan_head_before: 6b5903e36ac3ccf42e51801eeb01040be5eca670
  plan_head_after: 081a0c8b5a2a8c3a7d42e88366f6b8293530e30a
tech-stack:
  added: []
  patterns:
    - Exact MMIO control readback uses one typed instance field and the existing state visitor.
key-files:
  created:
    - .planning/phases/04-games-play-in-retroarch/04-51-SUMMARY.md
  modified:
    - src/bus.h
    - src/bus.c
    - tests/bus/bus_tests.c
    - tests/bus/tests.cmake
    - tests/bus/inventory.txt
    - tests/core/test_state.c
    - tests/core/bus_unbuilt.c
    - tests/fixtures/tests.cmake
    - docs/hardware/bus.md
    - docs/embedding.md
key-decisions:
  - Model raw 16-bit ATTR readback only for exact halfword accesses; keep all other SPU accesses unbuilt.
  - Keep API, state and behavior revisions at 4; the interim v4 state hash changes only because the new field is serialized.
patterns-established:
  - Bus control latches are instance-owned, reset by the emulator policy and serialized through the existing typed visitor.
requirements-completed: []
coverage:
  - id: D1
    description: Exact halfword ATTR readback, including the 0020h transfer-mode poll, with aliases, isolation, unchanged DEV4 cost and preserved unbuilt boundaries.
    requirement: GAME-05
    verification:
      - kind: integration
        ref: ctest bus.spu_attr and core.bus_unbuilt
        status: pass
    human_judgment: false
  - id: D2
    description: The ATTR latch survives same-region save/load and clears on emulator reset; only its attributable interim-v4 content-state pins changed.
    requirement: GAME-05
    verification:
      - kind: unit
        ref: core.state.device_fields and core.state.round_trip
        status: pass
      - kind: integration
        ref: fixtures.gpu_motion.state.150 and fixtures.gpu_motion.state.180
        status: pass
    human_judgment: false
duration: 24min
completed: 2026-11-11
status: complete
---

# Phase 04 Plan 51: SPU ATTR Readback Summary

**The bus now reads back the exact 16-bit ATTR value needed by the traced OpenBIOS poll, while preserving per-instance save/reset ownership and all other unbuilt SPU accesses.**

## Performance

- **Duration:** 24 min
- **Started:** 2026-11-11T21:26:53Z
- **Completed:** 2026-11-11T21:51:09Z
- **Tasks:** 2
- **Files modified:** 10

## Accomplishments

- Added exact physical `1F801DAAh`, width-2 control readback, retaining all 16 bits and the existing DEV4 port cost without unbuilt accounting.
- Added state round-trip, same-region load, independent-instance and deterministic emulator-reset coverage. Byte/word accesses, neighboring SPU addresses and STATX remain unbuilt.
- Updated only the two interim-v4 motion state hashes affected by the serialized field; API, state and behavior revisions remain 4.

## Task Commits

1. **Task 1 RED: add failing ATTR readback regression** — `855d907` (`test`).
2. **Task 1 GREEN: implement ATTR readback and state ownership** — `081a0c8` (`feat`).
3. **Task 2: confirm the control-only model and retain real boot acceptance** — documentation review found no further edits; its documentation was already included in the GREEN commit.

The plan metadata commit follows the SUMMARY and STATE/ROADMAP updates.

## Files Created/Modified

Created:

- `.planning/phases/04-games-play-in-retroarch/04-51-SUMMARY.md` — plan outcome, evidence and coverage.

Modified:

- `src/bus.h` — instance-owned 16-bit ATTR field.
- `src/bus.c` — exact decode, deterministic reset and state-visitor field.
- `tests/bus/bus_tests.c` — readback, aliases, isolation, costs and unsupported-access controls.
- `tests/bus/tests.cmake` — registered `bus.spu_attr` integration test.
- `tests/bus/inventory.txt` — byte-sorted test inventory row.
- `tests/core/test_state.c` — saved value, cross-instance load and reset assertions.
- `tests/core/bus_unbuilt.c` — independent ownership classification for only ATTR/halfword.
- `tests/fixtures/tests.cmake` — updated state hashes for frames 150 and 180.
- `docs/hardware/bus.md` — exact readback scope, reset policy and unbuilt boundaries.
- `docs/embedding.md` — saved-latch contract.

## TDD Gate Compliance

- **RED:** The registered `bus.spu_attr` case executed and failed because the halfword read after writing `0020h` returned `0000h`. The JUnit report was accepted by `gsd_run check tdd-red-evidence` as `RED_EVIDENCE_OK`; semantic review confirmed the target assertion failed for the planned behavior.
- **GREEN:** Commit `081a0c8` made the focused bus, state, ownership-sweep and state-hash cases pass. No refactor was needed.

## Decisions Made

- Store the raw control halfword only at physical ATTR `1F801DAAh` with width 2; add no transfer, audio, status-mirroring or timing behavior.
- Keep the latch in `struct schway__bus`, include it in the existing visitor and reset it to zero as the emulator's deterministic reset model, without claiming a hardware power-on value.
- Preserve revision 4 because it is unreleased; update only state hashes attributable to the added serialized field.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Test bug] Corrected the expected unbuilt-count progression**

- **Found during:** Task 1 GREEN verification.
- **Issue:** The new loop expected one cumulative unbuilt access per unsupported read, but each preceding unsupported write also increments the counter.
- **Fix:** Assert the count after each read as `2*i + 1` and after each write as `2*(i + 1)`; implementation behavior was unchanged.
- **Files modified:** `tests/bus/bus_tests.c`.
- **Verification:** The focused bus test and final six-case tracer verification passed.
- **Committed in:** `081a0c8` (part of the GREEN commit).

**Total deviations:** 1 auto-fixed test issue (Rule 1). **Impact:** No scope change.

## Issues Encountered

An initial RED command used an unsupported CTest option and did not run the test. The corrected command produced the valid RED report before implementation.

## User Setup Required

None.

## Evidence

- RED: `ctest --test-dir build/ci -R '^bus\.spu_attr$' --no-tests=error --output-on-failure --output-junit tdd-red.xml` exited 8 on the intended ATTR assertion; the classifier command exited 0 with `RED_EVIDENCE_OK`.
- GREEN/tracer: the focused configure/build plus six selected tests exited 0, including `bus.spu_attr`, `core.state.device_fields`, `core.state.round_trip`, `core.bus_unbuilt`, and both motion state pins.
- State pin 150: `build/ci/schwaystation-run --exe build/ci/fixtures/gpu_motion.exe --frames 150 --hash-state` exited 0; state SHA-256 `c8b2b8d9b6d51f60f3cb37fe9e8329e41031dcf8d78f7da2239b7ba2345df6e1`.
- State pin 180: `build/ci/schwaystation-run --exe build/ci/fixtures/gpu_motion.exe --frames 180 --hash-state` exited 0; state SHA-256 `b00b85c90e610e888e3553ffa492204f99a2d1faefb03ce9bd5f9a3045cf1a4c`.
- Full CI: `cmake --workflow --preset ci` exited 0; all 473 tests and all 5 release-package checks passed.
- Hygiene: `cmake --workflow --preset hygiene` exited 0; all 23 checks passed.
- Documentation whitespace check: `git diff --check HEAD` exited 0.

## Next Phase Readiness

Plan 04-40 remains responsible for rerunning the actual BIOS trace, inspecting a recognizable shell and accepting the same-run portable frame tuple. This plan claims no shell success or shell hash.

## Self-Check: PASSED

The SUMMARY and all ten modified project files exist; both task commits are ancestors of HEAD; focused verification, full CI and hygiene passed.
