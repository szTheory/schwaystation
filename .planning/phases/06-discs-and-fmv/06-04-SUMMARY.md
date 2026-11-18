---
phase: 06-discs-and-fmv
plan: "04"
subsystem: cdrom
tags: [cdrom, mmio, irq2, command-timing, region]
requires:
  - phase: 06-01
    provides: Disc host API, profile model and test target skeleton
  - phase: 06-03
    provides: Instance-owned CD state, scheduler and IRQ2 wiring
  - phase: 06-07
    provides: CD-ROM implementation inventory and preparation
  - phase: 06-26
    provides: CD-ROM state visitor and serialization support
provides:
  - Indexed CD register banks, FIFOs, queued interrupts and deterministic command responses
  - CD command, region, licence, lid, pause, seek and lock behavior with focused integration tests
  - CD hardware behavior and timing documentation
affects: [06-05, 06-11, 06-25, 06-26, disc, cdrom]
actuals:
  tokens: 31735
  tasks: 2
  commits: 5
tech-stack:
  added: []
  patterns:
    - Real MMIO writes and scheduler budgets drive CD command integration tests
    - CD state reset and serialization are split into a dedicated state module
key-files:
  created:
    - src/cdrom_state.c
    - docs/hardware/cdrom.md
    - tests/cdrom/model.c
    - tests/cdrom/tests.cmake
  modified:
    - src/cdrom.c
    - src/cdrom.h
    - tests/cdrom/inventory.txt
    - tests/tests.cmake
    - include/schwaystation.h
    - docs/embedding.md
key-decisions:
  - "Use the AGENTS.md-mandated cdrom.* test selectors and inventory names."
  - "Keep sector acquisition and INT1 delivery in Plan 06-05; this plan models command and response behavior."
  - "Represent CD unlock/lock and licensed-profile behavior in instance state and serialize it with the CD device."
patterns-established:
  - "Command behavior tests cross the guest MMIO, scheduler and IRQ2 paths."
  - "CD reset and state serialization stay in cdrom_state.c to keep functions within project limits."
requirements-completed: []
coverage:
  - id: D1
    description: "CD command/status model with FIFO boundaries, scheduled IRQ responses, region/licence/lid behavior and documented timings."
    requirement: DISC-01
    verification:
      - kind: integration
        ref: "cmake --preset ci && cmake --build --preset ci --target schwaystation-test-cdrom schwaystation-test-state schwaystation-test-api-v5 && ctest --test-dir build/ci -R '^(cdrom\\.(command|timing|region|bounds)|core\\.state\\..*|abi\\.api_v5)$' --no-tests=error --output-on-failure --output-junit build/ci/06-04-final-tests.xml (exit 0, 13/13 passed; local run, no hosted URL)"
        status: pass
    human_judgment: false
duration: 107min
completed: 2026-11-15
status: complete
plan_head_before: 805e6b0a1048739d83a9d93790ea2da5aafd4d43
plan_head_after: 336b82a0d4499a7a7beef41157900b6cdb582fcc
commits: 5
---

# Phase 6 Plan 04: CD Command Model Summary

**Guest CD commands now traverse indexed MMIO registers, deterministic scheduler deadlines and IRQ2, with region/licence responses and timing behavior documented and covered by integration tests.**

## Performance

- **Duration:** 107 minutes
- **Started:** 2026-11-15T14:18:10-05:00 (first task commit; work began before this timestamp)
- **Completed:** 2026-11-15T16:04:47-05:00
- **Tasks:** 2
- **Files modified:** 20
- **Runtime/model:** Codex, OpenGSD 1.20.0; executor gpt-6-luna xhigh

## Accomplishments

- Added the CD register banks, parameter/result FIFOs, queued interrupt values, command scheduling, and Nop response through the instance scheduler and IRQ2.
- Implemented the planned command/status matrix, profile and licence checks, lid interrupt behavior, region and firmware differences, lock/unlock transitions, and deterministic command timing.
- Added registered CD command, timing, region and bounds integration tests plus hardware behavior and timing documentation.
- Extended CD state serialization for command state and profile flags; the focused state and public API v5 checks continue to pass.

## Task Commits

1. **Task 1: Run a Nop through MMIO, scheduler and IRQ2** - `4c9e6e8` (RED), `cbeeceb` (GREEN).
2. **Task 2: Expand the documented command/region table** - `a8ed9d4`, `2b43fe4` (RED), `336b82a` (GREEN).

## Files Created/Modified

- `src/cdrom.c`, `src/cdrom_state.c`, `src/cdrom.h` - CD register, command, timing and state behavior.
- `tests/cdrom/model.c`, `tests/cdrom/tests.cmake`, `tests/cdrom/inventory.txt`, `tests/tests.cmake` - real-path CD behavior coverage and registration.
- `docs/hardware/cdrom.md` - command matrix, selected timing baselines, hardware timing spread and seek confidence.
- `include/schwaystation.h`, `src/disc.c`, `src/instance.c`, `src/internal.h`, `src/state.c`, `cmake/DiscTargets.cmake`, `CMakeLists.txt`, `docs/embedding.md`, `tests/api/v5.c`, `tests/core/test_state.c`, `.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md`, `.planning/phases/06-discs-and-fmv/06-04-PLAN.md` - profile/state wiring, registration, tests and plan selector corrections.

## Decisions Made

- Follow the repository's required `cdrom.*` selector naming and inventory convention. This keeps the new lane consistent with the CMake test naming rule.
- Keep sector acquisition and INT1 delivery in Plan 06-05. The sector event currently clears its deadline; later work will deliver the sector to the host-facing data path.
- Store lock/unlock and licensed-profile state in the CD instance and include it in the versioned state format.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing critical functionality] Added explicit persistent licence and unlock profile state.**
- **Found during:** Task 2 (Expand the documented command/region table)
- **Issue:** The command/profile behavior needed to distinguish licensed and unlocked systems across reset and save-state operations.
- **Fix:** Added profile flags and serialized command state; advanced the behavior/state format revisions and updated embedding documentation.
- **Files modified:** `include/schwaystation.h`, `src/cdrom.c`, `src/cdrom_state.c`, `src/disc.c`, `src/instance.c`, `src/internal.h`, `src/state.c`, `docs/embedding.md`, `tests/api/v5.c`, `tests/core/test_state.c`
- **Verification:** Final state suite and API v5 integration check passed in the focused command.
- **Committed in:** `336b82a`.

**2. [Rule 1 - Bug] Corrected licence-sector offset and CD lock/read response behavior.**
- **Found during:** Task 2 (Expand the documented command/region table)
- **Issue:** Review and tests exposed an off-by-12 licence offset and missing status transitions for incomplete unlock, SecretLock and unlicensed reads.
- **Fix:** Read the licence marker at raw sector offset `0x38`; implement the documented lock status and reject unlicensed ReadN/ReadS with INT5 `(03h,40h)`.
- **Files modified:** `src/cdrom.c`, `tests/cdrom/model.c`, `docs/hardware/cdrom.md`
- **Verification:** Focused command/region tests passed after review.
- **Committed in:** `336b82a`.

**3. [AGENTS.md adjustment] Used required CD test selectors and split state helpers to meet source limits.**
- **Found during:** Task 1 and Task 2
- **Issue:** The plan called the selectors `disc.*`, while repository conventions require subsystem-prefixed names. The added state helpers also needed to stay under the project function-size limit.
- **Fix:** Renamed selectors and inventory entries to `cdrom.*`, updated plan acceptance commands, and moved CD reset/serialization code to `src/cdrom_state.c`.
- **Files modified:** `.planning/phases/06-discs-and-fmv/06-04-PLAN.md`, `src/cdrom.c`, `src/cdrom_state.c`, `CMakeLists.txt`, `cmake/DiscTargets.cmake`, `tests/cdrom/inventory.txt`, `tests/cdrom/tests.cmake`, `tests/tests.cmake`
- **Verification:** Focused build and selected CTest cases passed; changed functions remain below 60 lines and `src/cdrom.c` remains below its 1200-line cap.
- **Committed in:** `336b82a`.

**Total deviations:** 3 auto-fixed or policy-driven adjustments. **Impact on plan:** Required for correct stateful behavior, project naming/source limits and executable acceptance commands; no sector-delivery scope was pulled forward.

## Issues Encountered

No unresolved implementation issues. The initial default sandbox could not write `.git`; the task commits were completed through the authorized Git operation with the repository's noreply identity and hooks enabled.

## User Setup Required

None - no external service configuration required.

## Known Stubs

- `src/cdrom.c:1083` - sector events currently clear the scheduled deadline; sector acquisition and INT1 delivery are intentionally assigned to Plan 06-05.

## Next Phase Readiness

The command/status and deterministic timing layer is ready for Plan 06-05 to connect sector acquisition and INT1 delivery. Global DISC-01 acceptance remains pending until the cross-plan sector and disc behavior is complete. No owner verification is needed for this plan's automated checks.

## Self-Check: PASSED

- Summary exists at the plan output path.
- All five task commit hashes are ancestors of the current plan commit.
- The final focused build/test command exited 0 with 13/13 selected cases passing.

---
*Phase: 06-discs-and-fmv*
*Completed: 2026-11-15*
