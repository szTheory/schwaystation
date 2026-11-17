---
phase: 06-discs-and-fmv
plan: "26"
subsystem: save-state
tags:
  - c
  - cdrom
  - mdec
  - save-state
  - fixtures
requires:
  - phase: 06-discs-and-fmv
    provides: CD-ROM and MDEC instance state and chip-owned visitors from Plan 03
  - phase: 06-discs-and-fmv
    provides: Host-owned disc callback and copied TOC interface from Plan 02
provides:
  - Validated CD-ROM and MDEC machine fields in state format 6
  - MMIO ownership checks and an SIO1 fixture for the remaining unbuilt-device diagnostic
affects:
  - 06-04
  - 06-11
  - 06-21
actuals:
  tokens: 3303
  tasks: 2
  commits: 4
commits: 4
plan_head_before: 815297fc0c7e4939a9cee560735ce3e2dfbd55cb
plan_head_after: a45acb09d70fa550db657dafa0d6166eea1f2f3b
tech-stack:
  added: []
  patterns:
    - Chip-owned visitors append deterministic device bytes without serializing host callbacks.
    - State validation covers bounded FIFO indices, phases, sector counts and deadlines before apply.
key-files:
  created:
    - .planning/phases/06-discs-and-fmv/06-26-SUMMARY.md
  modified:
    - src/state.c
    - src/internal.h
    - tests/core/test_state.c
    - tests/core/bus_unbuilt.c
    - tests/fixtures/src/unbuilt.asm
    - tests/fixtures/tests.cmake
    - tests/fixtures/manifest.txt
key-decisions:
  - Append CD-ROM and MDEC visitors after SPU to preserve earlier field order while advancing the format once to 6.
  - Keep host callback pointers and the external audio drain ring outside serialized machine state.
  - Move the unbuilt fixture probe to SIO1; CD-ROM and MDEC are built MMIO owners.
requirements-completed:
  - DISC-01
  - DISC-03
coverage:
  - id: D1
    description: CD-ROM and MDEC fields round-trip deterministically, ignore callback addresses, reset cleanly, and reject corrupt bounds atomically.
    requirement: DISC-01
    verification:
      - kind: unit
        ref: cmake --preset ci && cmake --build --preset ci --target schwaystation-test-state && ctest --test-dir build/ci -R '^core\.state\.' --output-junit build/ci/06-26-task1-green-rerun.xml --no-tests=error --output-on-failure
        status: pass
    human_judgment: false
  - id: D2
    description: The MMIO sweep treats CD-ROM and MDEC as owned, reports only remaining unbuilt SIO1 accesses, and retains gap-write faults.
    requirement: DISC-03
    verification:
      - kind: integration
        ref: cmake --preset ci && cmake --build --preset ci --target schwaystation-test-bus-unbuilt schwaystation-fixtures schwaystation-test-boot && ctest --test-dir build/ci -R '^core\.bus_unbuilt$|^fixtures\.unbuilt_footer$|^fixtures\.gap_write$' --output-junit build/ci/06-26-task2-green.xml --no-tests=error --output-on-failure
        status: pass
    human_judgment: false
decisions:
  - CD-ROM and MDEC visitors were appended after SPU so existing field offsets remain stable within the new format.
  - Host-owned callbacks and the host audio drain ring remain excluded from serialized state.
metrics:
  duration: 20min
  completed: 2026-11-15
status: complete
---

# Phase 6 Plan 26: CD/MDEC State Serialization Summary

**Validated CD-ROM and MDEC state now round-trips under format revision 6, and unbuilt diagnostics probe SIO1.**

## Performance

- **Duration:** 20 minutes
- **Started:** 2026-11-15T18:42:29Z
- **Completed:** 2026-11-15T19:02:06Z
- **Tasks:** 2
- **Files modified:** 7
- **Runtime/model:** Codex, OpenGSD 1.20.0; executor gpt-6-luna xhigh

## Accomplishments

- Added the CD-ROM and MDEC chip visitors to the state walk and advanced behavior/state-format revision 5 to 6 once.
- Tested byte-identical device round trips with different host callback addresses, callback preservation on load, POWER reset, equivalent split budgets, and failure-atomic rejection of invalid FIFO indices, phases, sector counts and deadlines.
- Updated the MMIO sweep to classify the CD-ROM and MDEC windows as owned, moved the guest unbuilt probe to SIO1, and kept the unmapped gap-write fault case.
- Added the authored fixture source and its digest to the fixture manifest.

## Verification Evidence

- `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-state && ctest --test-dir build/ci -R '^core\.state\.' --output-junit build/ci/06-26-task1-green-rerun.xml --no-tests=error --output-on-failure` — exit 0; 8/8 state tests passed.
- `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-bus-unbuilt schwaystation-fixtures schwaystation-test-boot && ctest --test-dir build/ci -R '^core\.bus_unbuilt$|^fixtures\.unbuilt_footer$|^fixtures\.gap_write$' --output-junit build/ci/06-26-task2-green.xml --no-tests=error --output-on-failure` — exit 0; 3/3 selected bus and fixture tests passed.
- TDD RED evidence was captured for both tasks and accepted by `gsd_run check tdd-red-evidence`: the state test failed on the 24 absent visitor fields, and the fixture assertion failed because the old guest no longer produced an MDEC-unbuilt diagnostic. Both RED CTest runs exited 8 intentionally; their target assertions failed for the planned behavior.
- The authored fixture SHA-256 matched its manifest row. All checks ran locally from the checkout root; no hosted URL applies.

## Task Commits

1. **Task 1 RED: CD/MDEC state assertions** — `4db9d80` (test)
2. **Task 1 GREEN: serialize device state and revision 6** — `aa1080b` (feat)
3. **Task 2 RED: owned-window and SIO1 expectations** — `8934338` (test)
4. **Task 2 GREEN: migrate the unbuilt fixture to SIO1** — `a45acb0` (feat)

## Files Created/Modified

- `src/state.c` — appends CD-ROM and MDEC state visitors and updates the body description.
- `src/internal.h` — advances behavior and state-format revision constants to 6.
- `tests/core/test_state.c` — checks serialization, validation, reset, callbacks and budget splitting.
- `tests/core/bus_unbuilt.c` — treats CD-ROM and MDEC MMIO as owned.
- `tests/fixtures/src/unbuilt.asm` — probes SIO1 instead of built MDEC.
- `tests/fixtures/tests.cmake` — expects the SIO1 unbuilt diagnostic and preserves the gap test.
- `tests/fixtures/manifest.txt` — lists the fixture source digest.

## Decisions Made

- Appended the new device visitors after SPU so the previous fields retain their order inside the new format.
- Kept callback pointers and the host audio drain ring outside serialized machine state; the state is independent of callback addresses.
- Used SIO1 as the remaining unbuilt fixture target after Plan 03 routed CD-ROM and MDEC through their owners.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Test bug] Corrected the seeded MDEC deadline to stay within the modeled 1,000-cycle bound.**
- **Found during:** Task 1 verification
- **Issue:** The initial test seed used a 3,000-cycle deadline, which the MDEC visitor correctly rejected.
- **Fix:** Changed the valid round-trip seed to 900 cycles; the invalid-state case remains at 1,001 cycles.
- **Files modified:** `tests/core/test_state.c`
- **Verification:** The focused state suite passed 8/8 after correction.
- **Committed in:** `aa1080b` (part of Task 1 GREEN)

**Total deviations:** 1 auto-fixed (Rule 1)
**Impact on plan:** Test input corrected; no production scope changed.

## Issues Encountered

- The first state GREEN run exposed only the invalid test seed described above. No other implementation or verification issues remain.

## User Setup Required

None — no external configuration is needed.

## Next Phase Readiness

The state integration and fixture migration are complete. The next sequential Phase 6 plan remains 06-04; this out-of-order wave plan does not advance that pointer. Phase-level CD-ROM and MDEC behavior acceptance remains for the other Phase 6 plans.

## Self-Check: PASSED

- The summary and seven plan artifacts exist.
- All four task commits are ancestors of HEAD.
