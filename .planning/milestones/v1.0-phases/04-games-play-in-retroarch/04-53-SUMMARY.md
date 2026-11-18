---
phase: 04-games-play-in-retroarch
plan: "53"
subsystem: gpu
tags: [GPUSTAT, beam, savestate, v4]
requires:
  - phase: "04-52"
    provides: [scheduled GPU beam and typed state visitor]
provides:
  - Saved GPU field-status phase and scheduled GPUSTAT bits 13 and 31
  - Regression coverage for profile, mode, IRQ, boundary, and state behavior
affects: [04-40, GPU status, interim-v4 savestates]
actuals:
  tokens: 8759
  tasks: 2
  commits: 3
commits: 3
plan_head_before: 86863464577798d3b44bfbdaa7fd0e327a603fda
plan_head_after: cef488e7d4dd79b28c9250c3449061d5c93b885d
tech-stack:
  added: []
  patterns: [per-instance saved GPU status phase, typed and range-checked state field]
key-files:
  created: []
  modified:
    - src/gpu.c
    - src/gpu.h
    - tests/gpu/beam.c
    - tests/gpu/tests.cmake
    - tests/gpu/inventory.txt
    - tests/fixtures/tests.cmake
    - docs/hardware/gpu.md
    - .planning/preparation/PSX-GPU-AND-GTE.md
key-decisions:
  - "Store one v4 field-status latch because disabling IRS retains bit 13 across later progressive fields."
  - "Keep fixed 263/314-line beam timing; document the model boundary instead of claiming exact alternating physical field timing."
patterns-established:
  - "GPUSTAT status is composed from saved phase, current mode, beam line, and display range without read-side transitions."
requirements-completed: []
coverage:
  - id: D1
    description: "Scheduled GPUSTAT bit 13 and bit 31 behavior across display, mode, and GPU revision controls."
    requirement: GAME-05
    verification:
      - kind: unit
        ref: 'ctest --test-dir build/ci --output-on-failure -R "^gpu\\.beam\\.status_controls$"'
        status: pass
      - kind: unit
        ref: 'ctest --test-dir build/ci --output-on-failure -R "^gpu\\.beam\\.status_fields$"'
        status: pass
    human_judgment: false
  - id: D2
    description: "Saved phase restores deterministically and the fixed-beam timing limits are documented with primary evidence."
    requirement: GAME-05
    verification:
      - kind: unit
        ref: 'ctest --test-dir build/ci --output-on-failure -R "^gpu\\.beam\\.(status_controls|reject_malformed_state|field_edges)$"'
        status: pass
      - kind: other
        ref: 'cmake --workflow --preset ci (docs and state-hash checks included)'
        status: pass
    human_judgment: false
duration: 45min
completed: 2026-11-11
status: complete
---

# Phase 04 Plan 53: GPUSTAT Field Status Summary

**GPUSTAT field phase now follows scheduled beam edges, retains bit 13 through progressive mode, and round-trips in interim v4 state.**

## Performance

- **Duration:** 45 min
- **Started:** 2026-11-11T23:16:01Z
- **Completed:** 2026-11-12T00:01:15Z
- **Tasks:** 2
- **Files modified:** 8

## Accomplishments

- Added one instance-owned saved phase, exposing v2 bit 13 while v0 masks that bit and preserving the existing beam clock.
- Composed bit 31 from the active vertical range, saved phase in interlaced display, or beam-line parity in effective 240-line modes.
- Covered scheduled edges, read/peek purity, IRQ independence, mode retention, state validation and restoration across NTSC-J, NTSC-U, PAL, and GPU v0/v2 profiles.
- Updated only the attributable interim-v4 state-hash pins and documented the primary evidence and fixed-beam timing limits.

## Task Commits

1. **Task 1: Expose status parity through real scheduled beam and GP1 reads** — `461c6a0` test RED, then `09c8e62` implementation GREEN.
2. **Task 2: Expand status coverage across modes, state, and timing controls** — `cef488e` regression tests.

## Files Created/Modified

- `src/gpu.c`, `src/gpu.h` — saved phase, scheduled transition, GPUSTAT composition, and typed state field.
- `tests/gpu/beam.c`, `tests/gpu/tests.cmake`, `tests/gpu/inventory.txt` — RED/GREEN status assertions and expanded controls.
- `tests/fixtures/tests.cmake` — two interim-v4 state-hash pins.
- `docs/hardware/gpu.md`, `.planning/preparation/PSX-GPU-AND-GTE.md` — implemented status semantics, source provenance, and clock-model limits.

## Decisions Made

- A saved boolean is required because bit 13 retains its last value after IRS is disabled; progressive mode cannot derive it from the live mode or frame parity.
- The existing integer beam and field lengths remain unchanged. The implementation follows the published normal active-display sequence and documents unmodeled alternating physical field lengths and early-vblank entry behavior.

## Deviations from Plan

None — plan executed as written. Task 2 expanded regression coverage without inventing a second RED cycle for already implemented behavior.

## Issues Encountered

- Corrected test setup during implementation: the beam helper needed to stop within one modeled frame, I_STAT clears written zero bits, and the new test name needed its sorted inventory entry. These were test-only corrections; no production behavior or plan scope changed.

## Evidence

- TDD RED: the dedicated `gpu.beam.status_fields` run exited 8 after build/configure succeeded; its assertion failures were the intended missing bit-13/31 behavior. `gsd_run check tdd-red-evidence` classified the captured report as `RED_EVIDENCE_OK`.
- Focused GREEN: `gpu.beam.status_fields`, `gpu.beam.status_controls`, `gpu.beam.reject_malformed_state`, and `gpu.beam.field_edges` passed. The final post-boundary-change focused command exited 0.
- Interim-v4 state pins: `build/ci/schwaystation-run --exe build/ci/fixtures/gpu_motion.exe --frames 150 --hash-state` exited 0 with `ca90ec77bf7b84ad79f64471fae24ab436ccb5c8df03e5827e0af9e4638470a`; the equivalent `--frames 180` command exited 0 with `1634e1a6f1904cc3244a68e420bb90111a6e1d67a2c259661dee613aebbee949`.
- Inventory: `LC_ALL=C sort -c tests/gpu/inventory.txt` exited 0.
- Full CI: `cmake --workflow --preset ci` exited 0; all 477 tests and all 5 release archive checks passed.
- Hygiene: `SCHWAYSTATION_TIMEOUT_SCALE=4 cmake --workflow --preset hygiene` exited 0; all 23 checks passed.

## User Setup Required

None — no external service configuration required.

## Next Phase Readiness

Plan 04-53 is complete. GAME-05 remains open pending the actual recognizable-shell and same-run portable acceptance owned by Plan 04-40. This plan establishes GPU status semantics only; it does not claim shell success or alter the independent readiness oracle.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-11*

## Self-Check: PASSED

- Summary exists and the validator found both modified-file references.
- Task commits `461c6a0`, `09c8e62`, and `cef488e` are present in HEAD history.
