---
phase: 05-the-sound-matches-the-console
plan: "16"
subsystem: state-serialization
tags: [spu, save-state, cmake, ctest]
requires:
  - phase: 05-the-sound-matches-the-console
    provides: SPU machine storage and private device-state seams from plan 05-02
provides:
  - Complete little-endian SPU machine-state save/load and sample-phase continuation
  - Format-5 state compatibility with malformed SPU state rejection before apply
affects: [spu, state-serialization, embedding-docs, phase-05-verification]
actuals:
  tokens: 7612
  tasks: 2
  commits: 4
plan_head_before: 0d36382ab38d88f10b3c74e993d83002c79e85e4
plan_head_after: 03306b2dca36b545faca673b2063d8c90bb6ef4e
tech-stack:
  added: []
  patterns: [little-endian field visitor, validate-before-apply state loading]
key-files:
  created: []
  modified: [src/spu.c, src/spu.h, src/state.c, src/internal.h, tests/core/test_state.c, docs/embedding.md]
key-decisions:
  - Pass the saved scheduler tick value into the private SPU visitor to validate sample phase before applying fields.
  - Advance behavior revision and state format together to 5 while preserving the public API and ABI.
patterns-established:
  - SPU state uses the shared little-endian field visitor for every machine field and RAM byte.
  - State validation checks cross-field ranges before applying decoded values.
requirements-completed: [SOUND-01, SOUND-02]
coverage:
  - id: D1
    description: SPU machine storage and sample phase resume through save/load while the host audio delivery queue clears.
    requirement: SOUND-01
    verification:
      - kind: unit
        ref: "cmake --preset ci && cmake --build --preset ci --target schwaystation-test-state schwaystation-test-spu && ctest --test-dir build/ci -R '^core\\.state\\.|^spu\\.model$' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: Malformed SPU state is rejected atomically and format-4 state is refused under state format 5.
    requirement: SOUND-01
    verification:
      - kind: unit
        ref: "cmake --preset ci && cmake --build --preset ci --target schwaystation-test-state schwaystation-test-spu schwaystation-test-api-v1 && ctest --test-dir build/ci -R '^core\\.state\\.|^spu\\.model$|^abi\\.api_v1$' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
  - id: D3
    description: Public API and ABI compatibility remain valid for the frozen API v1 caller.
    requirement: SOUND-02
    verification:
      - kind: unit
        ref: "abi.api_v1 in the Task 2 focused CTest selection"
        status: pass
    human_judgment: false
metrics:
  duration: 26min
  completed: 2026-11-13
status: complete
---

# Phase 05 Plan 16: SPU State Continuation Summary

**SPU machine state now round-trips in format 5, restoring sample timing and future PCM while keeping the host audio queue outside saved state.**

## Performance

- **Duration:** 26 minutes
- **Started:** 2026-11-13T23:00:27Z
- **Completed:** 2026-11-13T23:26:23Z
- **Tasks:** 2
- **Files modified:** 6

## Accomplishments

- Added little-endian state visitor coverage for SPU RAM, registers, voices, transfer FIFO, capture, reverb, FIR history, and the next sample deadline.
- Restored the saved SPU phase and future PCM after load, rebuilt derived deadlines, and verified the host-delivery audio queue is cleared.
- Bumped the behavior revision and state format to 5, rejected old format-4 and malformed state before mutation, and documented compatibility without changing public API or ABI.

## Task Commits

1. **Task 1 RED:** `a9feb6e` — failing SPU save/load continuation proof.
2. **Task 1 GREEN:** `331db7f` — SPU state serialization and sample-phase restore.
3. **Task 2 RED:** `b5f0512` — revision-5 compatibility and malformed-state proofs.
4. **Task 2 GREEN:** `03306b2` — format-5 compatibility and save-state documentation.

## Files Modified

- `src/spu.c`, `src/spu.h` — SPU fields use the state visitor; load validates phase against saved scheduler ticks and restores the machine state.
- `src/state.c` — dispatches state serialization through the SPU visitor.
- `src/internal.h` — reports behavior revision 5 and state format 5.
- `tests/core/test_state.c` — continuation, invalid-state atomicity, format compatibility, and 4 MiB ceiling checks.
- `docs/embedding.md` — describes SPU state contents, format-4 refusal, sample-phase restoration, and audio queue exclusion.

## TDD Gate Compliance

| Task | RED evidence | GREEN verification | Result |
| --- | --- | --- | --- |
| 1 | `core.state.device_fields` failed with exit 8 because all 31 expected SPU records were absent and the saved sample deadline was not restored. `gsd_run check tdd-red-evidence` returned `RED_EVIDENCE_OK`; semantic inspection confirmed the planned assertions failed. | Planned focused CMake/CTest command passed all 9 state and SPU tests. | RED → GREEN committed in order; no refactor was needed. |
| 2 | `core.state.rejects` failed with exit 8 because format 4 was accepted and changed the destination; malformed SPU controls passed. Evidence returned `RED_EVIDENCE_OK`; semantic inspection confirmed the compatibility assertion failure. | Focused state, SPU, and frozen API v1 CTest command passed all 10 tests. | RED → GREEN committed in order; no refactor was needed. |

## Verification

- Task 1 command: `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-state schwaystation-test-spu && ctest --test-dir build/ci -R '^core\.state\.|^spu\.model$' --no-tests=error --output-on-failure` — exit 0, 9/9 tests passed.
- Task 2 command: `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-state schwaystation-test-spu schwaystation-test-api-v1 && ctest --test-dir build/ci -R '^core\.state\.|^spu\.model$|^abi\.api_v1$' --no-tests=error --output-on-failure` — exit 0, 10/10 tests passed.
- RED evidence records: `build/ci/05-16-task1-red-evidence.json` and `build/ci/05-16-task2-red-evidence.json`; each was classified `RED_EVIDENCE_OK` against its JUnit report.

## Decisions Made

- The private SPU state visitor receives the saved scheduler tick so it can validate the SPU sample deadline before any decoded fields are applied.
- Format 5 rejects released format-4 states; the public API and ABI remain unchanged, and the host audio delivery queue is not serialized.

## Deviations from Plan

### Necessary private interface adjustment

The plan listed `src/spu.c` but not `src/spu.h`; the private visitor signature in `src/spu.h` was extended to receive saved scheduler ticks. This lets validation reject an inconsistent sample phase before APPLY. It does not change a public interface and is part of the planned state validation.

**Total deviations:** 1 narrow private interface adjustment. **Impact:** no scope expansion.

## Issues Encountered

The pre-existing run-bounds state test manually changes scheduler ticks. Its fixture was updated to set a valid SPU sample deadline alongside those ticks so it continues to model a valid state after phase validation was added.

## User Setup Required

None — no external service configuration is required.

## Next Phase Readiness

This plan’s SPU save/load path and format policy are in place. Continue the remaining Phase 5 plans and phase verification; preserve the stop before shipping Phase 5 or starting Phase 6.

---
*Phase: 05-the-sound-matches-the-console*
*Completed: 2026-11-13*

## Self-Check: PASSED

The summary exists, all four plan commits are ancestors of HEAD, and the measured plan commit count is 4.
