---
phase: 05-the-sound-matches-the-console
plan: "20"
subsystem: audio
tags: [spu, fifo, manual-transfer, irqa, state]
requires:
  - phase: 05-the-sound-matches-the-console
    provides: "SPU instance RAM and scheduled MMIO writes from plans 05-03, 05-04, and 05-16"
provides:
  - "Bounded SPU FIFO and sample-tick manual RAM writes with RAM_CTRL replication"
  - "RAM_CTRL-gated zero-input capture, STATX request/status bits, and SPU-enabled IRQA"
affects:
  - "Phase 05 SPU DMA, voice, reverb, and state integration"
actuals:
  tokens: 6526
  tasks: 1
  commits: 2
commits: 2
tech-stack:
  added: []
  patterns:
    - "FIFO data-port accesses update the queue at bus-access time; register writes remain tick-scheduled"
    - "SPU RAM writes use the little-endian helper that checks IRQA on every access"
key-files:
  created:
    - docs/hardware/spu.md
  modified:
    - src/spu.c
    - src/spu.h
    - tests/spu/model.c
    - tests/core/test_state.c
    - docs/hardware/bus.md
key-decisions:
  - "ManualWrite moves one halfword per 44.1 kHz tick and RAM_CTRL values 4, 6, 8, and A repeat it 1, 2, 4, and 8 times."
  - "Capture writes the four planned zero-valued inputs only when RAM_CTRL bits 2–3 are nonzero; no unmeasured sub-tick order is claimed."
  - "IRQ9 requires both SPU Enable and IRQ9 Enable; clearing IRQ9 Enable acknowledges the latch."
patterns-established:
  - "Persist and validate the internal SPU RAM transfer pointer alongside FIFO contents."
requirements-completed: []
coverage:
  - id: D1
    description: "FIFO, manual upload, RAM_CTRL replication, capture gating, STATX, and SPU-enabled IRQA behavior"
    requirement: SOUND-01
    verification:
      - kind: unit
        ref: "tests/spu/model.c#spu.model"
        status: pass
    human_judgment: false
  - id: D2
    description: "Transfer pointer round-trips in save states and malformed range/alignment values are refused"
    requirement: SOUND-01
    verification:
      - kind: unit
        ref: "tests/core/test_state.c#core.state.device_fields, core.state.round_trip, core.state.record_sweep"
        status: pass
    human_judgment: false
  - id: D3
    description: "SPU transfer and register behavior documented with sample-tick and measurement limits"
    requirement: SOUND-01
    verification:
      - kind: other
        ref: "docs/hardware/spu.md and docs/hardware/bus.md reviewed against src/spu.c and preparation §§1.1, 1.5–1.6"
        status: pass
    human_judgment: false
plan_head_before: ade0a0a8c3cead0183f0e0d5d65f5da150633de6
plan_head_after: cff598330f318569c7682e536e12af8db5676154
duration: 20 min
completed: 2026-11-14
status: complete
---

# Phase 5 Plan 20: SPU manual transfers and capture Summary

**32-halfword SPU FIFO, tick-bounded manual RAM writes, capture, STATX and gated IRQ9.**

## Performance

- **Duration:** 20 min
- **Started:** 2026-11-14T00:18:40Z
- **Completed:** 2026-11-14T00:39:16Z
- **Tasks:** 1
- **Files modified:** 6
- **Runtime/model:** Codex/OpenGSD 1.20.0; gpt-6-luna xhigh

## Accomplishments

- Added FIFO push, pop, full-write rejection, empty-read zero, and non-consuming pure peek behavior.
- Implemented bounded one-halfword-per-tick ManualWrite transfers, TSA byte-address wrap, and RAM_CTRL replication.
- Added gated zero-input capture, STATX transfer/DMA/capture status, SPU Enable plus IRQ9 Enable gating, and save-state transfer-pointer validation.
- Replaced the bus page's obsolete ATTR-only description with a focused SPU hardware page that states the model's timing and evidence limits.

## Task Commits

1. **Task 1 RED: failing FIFO, transfer, capture, and IRQ assertions** - `b1c6f48` (`test`)
2. **Task 1 GREEN: manual transfers, capture, STATX, IRQA, and state support** - `cff5983` (`feat`)

## TDD Cycle

- **RED command:** `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-spu schwaystation-test-state && ctest --test-dir build/ci -R '^(spu\.model|core\.state\.(device_fields|round_trip|record_sweep))$' --no-tests=error --output-on-failure --output-junit "$PWD/build/ci/05-20-red.xml"`
- **RED result:** exit 8; the target `spu.model` ran and failed planned assertions for FIFO state, RAM transfer, capture, STATX, and IRQ gating. All three selected state tests passed. `gsd_run check tdd-red-evidence` returned `RED_EVIDENCE_OK` for the JUnit report.
- **RED semantic assessment:** the named test executed and failed on assertions in `tests/spu/model.c` for the intended missing behavior; no compile, discovery, load, or fixture failure occurred.
- **GREEN command:** `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-spu schwaystation-test-state && ctest --test-dir build/ci -R '^(spu\.model|core\.state\.(device_fields|round_trip|record_sweep))$' --no-tests=error --output-on-failure`
- **GREEN result:** exit 0; all four selected tests passed.
- **Commit hygiene:** `scripts/hygiene.sh --staged` exited 0 for the implementation commit.

## Files Created/Modified

- `src/spu.c` and `src/spu.h` - FIFO operations, transfer/capture tick steps, status and IRQ gating, and serialized transfer pointer.
- `tests/spu/model.c` - RED assertions for FIFO behavior, manual upload, replication, capture, status, and IRQ gate.
- `tests/core/test_state.c` - transfer-pointer round-trip and invalid range/alignment checks.
- `docs/hardware/spu.md` - register behavior, little-endian RAM, sample granularity, and unmeasured limits.
- `docs/hardware/bus.md` - links the modeled SPU window to its hardware page.

## Deviations from Plan

None. The earlier IRQ assertions were updated to enable the SPU, as required by the plan. Global SOUND-01 and SOUND-02 remain Pending until Phase 5 verification.

## Issues Encountered

The broad default hygiene sweep was stopped while still running; the staged-file hygiene check used by the commit hook passed.

## User Setup Required

None - no external configuration is required.

## Next Phase Readiness

Plan 05-20 is complete. Global SOUND-01/SOUND-02 remain Pending; finish the other Phase 5 plans and verify the phase before marking them complete.

---
*Phase: 05-the-sound-matches-the-console*
*Completed: 2026-11-14*

## Self-Check: PASSED

- Summary and all six plan files exist.
- RED commit `b1c6f48` and GREEN commit `cff5983` are ancestors of the current branch head.
- The measured plan ledger records two task commits from `ade0a0a8c3cead0183f0e0d5d65f5da150633de6` through `cff598330f318569c7682e536e12af8db5676154`.
