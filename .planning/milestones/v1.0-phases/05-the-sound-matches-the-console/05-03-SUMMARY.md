---
phase: 05-the-sound-matches-the-console
plan: "03"
subsystem: sound
tags: [SPU, MMIO, C, TDD]
requires:
  - phase: 05-01/05-02/05-16
    provides: "Bounded SPU tick, instance-owned chip state, and state serialization"
provides:
  - "CPU byte, halfword, and word writes reach the chip-owned SPU register model"
  - "Register writes latch at a scheduled SPU tick and register peeks remain pure"
affects: [SPU FIFO, DMA4, voices, state]
actuals:
  tokens: 5766
  tasks: 1
  commits: 2
  plan_head_before: eae6e2756c19cc353c8a6e58e1a0e98ec9b6b30d
  plan_head_after: b22d3ff1b0f59641b2fc245ae0278a246d2151ad
tech-stack:
  added: []
  patterns:
    - "Chip-owned register writes are queued and applied at the next emulated SPU tick"
key-files:
  created:
    - .planning/phases/05-the-sound-matches-the-console/05-20-PLAN.md
  modified:
    - .planning/phases/05-the-sound-matches-the-console/05-03-PLAN.md
    - .planning/ROADMAP.md
    - src/bus.c
    - src/spu.c
    - src/spu.h
    - tests/spu/model.c
    - tests/bus/bus_tests.c
    - tests/core/bus_unbuilt.c
    - tests/core/test_state.c
key-decisions:
  - "The bus updates the existing ATTR mirror alongside chip-owned writes until the DMA4 caller moves to chip state."
  - "FIFO, capture, and IRQA transfer behavior moves into plan 05-20; downstream SPU plans depend on its completion."
requirements-completed: []
coverage:
  - id: D1
    description: "Guest MMIO writes route to chip-owned SPU registers with defined widths and tick latching."
    requirement: SOUND-01
    verification:
      - kind: unit
        ref: "tests/spu/model.c#check_bus_width_and_control_tick"
        status: pass
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^(spu.model|core.bus_unbuilt|bus.spu_attr|core.state.(device_fields|round_trip|record_sweep))$'"
        status: pass
    human_judgment: false
  - id: D2
    description: "SPU register peeks leave the scheduler, pending writes, FIFO positions, and IRQ latch unchanged."
    requirement: SOUND-01
    verification:
      - kind: unit
        ref: "tests/spu/model.c#check_register_peek_is_pure"
        status: pass
    human_judgment: false
metrics:
  duration: 40min
  completed: 2026-11-14
status: complete
---

# Phase 5 Plan 03: Guest SPU Register MMIO Summary

**CPU-width SPU writes now reach chip-owned registers and latch at scheduled sample ticks.**

## Performance

- **Duration:** 40 minutes
- **Started:** 2026-11-13T23:31:36Z
- **Completed:** 2026-11-14T00:11:17Z
- **Tasks:** 1
- **Files modified by the task:** 8
- **Executor runtime/model:** Codex/OpenGSD 1.20.0; gpt-6-luna xhigh

## Accomplishments

- Routed supported SPU registers through the chip model for byte, halfword, and ordered word accesses.
- Applied queued register writes at the scheduled 44.1 kHz SPU tick and kept peeks free of state changes.
- Updated directly affected ATTR bus, unbuilt-range, and save-state regressions.

## TDD Gate Compliance

- **RED:** `bf3b97e` — `spu.model` failed on assertions that guest voice/control writes were not reaching the SPU model. The evidence classifier accepted the intended failing target.
- **GREEN:** `b22d3ff` — routed MMIO through the chip register model and passed the focused SPU, bus, unbuilt-access, and state checks.
- **Tracer feedback gate:** the planned `spu.model` command was rerun after the implementation commit and passed.

Verification commands and results:

- RED: `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-spu && ctest --test-dir build/ci -R '^spu\.model$' --no-tests=error --output-on-failure` — CTest exit 8 on the intended assertions; `gsd_run check tdd-red-evidence` returned `RED_EVIDENCE_OK`.
- GREEN: `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-spu schwaystation-test-bus-unbuilt schwaystation-test-bus schwaystation-test-state && ctest --test-dir build/ci -R '^(spu\.model|core\.bus_unbuilt|bus\.spu_attr|core\.state\.(device_fields|round_trip|record_sweep))$' --no-tests=error --output-on-failure` — exit 0, 6 tests passed.
- Tracer rerun: `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-spu && ctest --test-dir build/ci -R '^spu\.model$' --no-tests=error --output-on-failure` — exit 0.

## Task Commits

1. **Task 1 RED:** `bf3b97e` — add failing guest SPU register checks.
2. **Task 1 GREEN:** `b22d3ff` — route guest MMIO through the SPU model.

**Plan metadata:** to be committed after the summary and planning-state checks.

## Files Modified

- `src/bus.c`, `src/spu.c`, `src/spu.h` — chip-owned register dispatch, delayed writes, register peeks, and STATX/IRQ integration.
- `tests/spu/model.c` — CPU-width, tick-latch, IRQ disable, and pure-peek checks.
- `tests/bus/bus_tests.c`, `tests/core/bus_unbuilt.c`, `tests/core/test_state.c` — existing regressions updated for supported SPU routing and the expanded register image.

## Decisions Made

- Keep the current ATTR mirror synchronized from the bus write path until the DMA4 reader is migrated.
- Split FIFO/manual-transfer/capture/IRQA behavior into plan 05-20 to preserve its full RED/GREEN and documentation scope. The phase plan index now reports 05-03 as the only ready plan, with 05-20 and its consumers gated behind it.

## Deviations from Plan

**1. Moved manual FIFO, capture, IRQA, and hardware documentation work to plan 05-20.**

- **Found during:** Task 2 scope assessment.
- **Issue:** The remaining behavior and docs could not be completed within the plan's 30-minute execution target without rushing.
- **Fix:** Narrowed 05-03 to the completed MMIO tracer and created 05-20 for the remaining behavior. Added 05-20 as a dependency to direct consumers and shifted later waves to preserve ordering.
- **Files modified:** phase 5 plan files and `.planning/ROADMAP.md`.
- **Verification:** `gsd_run phase-plan-index 05` showed only 05-03 ready; 05-20 is wave 5 and directly blocks all listed consumers.

**2. Added affected existing regression files to the 05-03 manifest.**

- **Found during:** Task 1.
- **Issue:** Existing ATTR, unbuilt-range, and save-state expectations directly changed when SPU dispatch became supported.
- **Fix:** Updated those checks and listed their paths in `files_modified`.
- **Files modified:** `tests/bus/bus_tests.c`, `tests/core/bus_unbuilt.c`, `tests/core/test_state.c`, and `src/spu.h`.
- **Verification:** Focused CTest passed 6/6.

## Next Phase Readiness

Plan 05-20 is the next ready plan and owns the outstanding FIFO, capture, IRQA, and register documentation work. Global SOUND-01 and SOUND-02 remain pending until Phase 5 verification establishes full coverage. Do not ship Phase 5 or start Phase 6 here.

---
*Phase: 05-the-sound-matches-the-console*
*Completed: 2026-11-14*

## Self-Check: PASSED

- Summary file exists and its frontmatter passes `gsd_run verify-summary`.
- Task commits `bf3b97e` and `b22d3ff` are ancestors of the current branch.
- Before summary closeout, the plan index reported 05-03 as the only ready plan. After closeout, it reports 05-20 as the only ready plan; its direct consumers remain gated.
