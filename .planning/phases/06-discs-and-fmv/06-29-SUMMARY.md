---
phase: 06-discs-and-fmv
plan: "29"
subsystem: cdrom
tags: [cdrom, dma3, getloc, psx-tests]
requires:
  - phase: 06-discs-and-fmv
    provides: "DMA3 transfer and chopped-window readiness from Plans 06-27 and 06-28"
provides:
  - "ReadN BUSYSTS clears at sub-CPU command acceptance while drive status remains separate"
  - "Host-observed ReadN, GetlocL, and complete DMA3 caller controls"
affects: [06-30, 06-06, disc]
actuals:
  tokens: 22718
  tasks: 2
  commits: 3
plan_head_before: b05ddb70682fdf3ab63789d9cb064ff651afc198
plan_head_after: a299e5c7b86540fa71560f5a779be4520d150438
tech-stack:
  added: []
  patterns:
    - "Command-transfer busy is distinct from the CD drive's Seek and Read status"
    - "Pinned caller results are checked through deterministic host-observed bytes"
key-files:
  created:
    - .planning/phases/06-discs-and-fmv/06-30-PLAN.md
  modified:
    - src/cdrom.c
    - src/cdrom_sector.c
    - tests/cdrom/model.c
    - tests/cdrom/reference.c
    - tests/cdrom/sectors.c
    - docs/hardware/cdrom.md
key-decisions:
  - "Clear BUSYSTS when the sub-CPU accepts a command; keep active drive state in the status byte."
  - "Move exact raw Q/GetlocP seek rows to dependent Plan 06-30 after pinned logs disproved one universal seek offset."
requirements-completed: []
coverage:
  - id: D1
    description: "Partial DISC-01 slice: ReadN status, buffer readiness, SeekError recovery, exact GetlocL results, and the pinned DMA3 caller. Aggregate DISC-01 remains open for Plan 06-30 and Plan 06-06."
    requirement: DISC-01
    verification:
      - kind: unit
        ref: "cmake --build build/ci --target schwaystation-test-cdrom && ctest --test-dir build/ci -R '^cdrom\\.(command|sectors)$' --no-tests=error --output-on-failure"
        status: pass
      - kind: integration
        ref: "cmake --build build/disc-reference --target schwaystation-test-cdrom-reference && ctest --test-dir build/disc-reference -R '^cdrom\\.reference\\.(getloc|controls)$' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
duration: ">= 1h31m23s wall lower bound from durable checkpoint"
completed: 2026-11-16
status: complete
---

# Phase 6 Plan 29: ReadN Status and DMA3 Caller Summary

**ReadN now releases the pinned caller at command acceptance, and host controls verify the full DMA3 sector transfer.**

## Performance

- **Duration:** At least 1h31m23s wall time from the earliest durable checkpoint at 2026-11-16T00:43:26Z through final gate closeout at 2026-11-16T02:14:49Z; the complete plan runtime is longer.
- **Started:** 2026-11-16T00:43:26Z (earliest durable execution marker; not the original start)
- **Completed:** 2026-11-16T02:14:49Z
- **Tasks:** 2
- **Files modified:** 14, including implementation, tests, documentation, review, and the dependent-plan graph.

## Accomplishments

- HSTS bit 7 now represents a command awaiting sub-CPU acceptance. It clears at acceptance even while the status byte reports Seek (`0x42`), allowing the pinned ReadN helper to continue.
- Model and host controls check empty-buffer BFRD readiness, exact IRQ/status/GetlocL results, all 2,340 DMA bytes, final MADR `0x3924`, and the sector cursor. SeekError tests also verify cached data survives failed seek/Getloc commands and that valid SeekL or Reset clears the error.
- The unchanged pinned guest completes its ReadN/DMA3 path. The exact Q/GetlocP seek rows moved to dependent Plan 06-30; DISC-01 remains partial until that plan and Plan 06-06's final gate pass.

## Verification

The post-review final command exited 0:

```sh
cmake --build build/ci --target schwaystation-test-cdrom && ctest --test-dir build/ci -R '^cdrom\.(command|sectors)$' --no-tests=error --output-on-failure && cmake --build build/disc-reference --target schwaystation-test-cdrom-reference && ctest --test-dir build/disc-reference -R '^cdrom\.reference\.(getloc|controls)$' --no-tests=error --output-on-failure
```

The native lane passed 2/2 tests. The pinned reference guest and host controls passed 2/2 tests. Focused code review found no actionable findings. There is no hosted run URL for these local checks.

The TDD RED evidence check returned `RED_EVIDENCE_OK` for the registered ReadN model control before implementation. The test is not skipped.

## Task Commits

1. **Task 1: Pin the ReadN HSTS and INT1 data-ready contract** — `0fb2868` (test)
2. **Task 2: Correct the ReadN ready status and verify the DMA caller** — `a299e5c` (fix)

The plan definition was committed as `9e1d1fa` before execution. The plan's measured three commits are `9e1d1fa`, `0fb2868`, and `a299e5c`; the final planning metadata commit is recorded separately.

## Decisions Made

- Keep BUSYSTS tied to command acceptance and report ongoing Seek/Read activity in the status byte.
- Keep Q-derived GetlocP behavior out of Plan 29. The three pinned seek rows differ, so Plan 06-30 now owns raw Q state, exact controls, and removal of the temporary general offset rule.

## Deviations from Plan

### Evidence-driven scope split

The plan exceeded its original 30-minute forecast while the pinned caller exposed a separate GetlocP landing mismatch. The three pinned rows show distinct Q positions, so a target-minus-five rule cannot represent them generally. Plan 06-30 was added and wired after Plan 29, before Plan 06-06. Plan 29 closes only the ReadN/HSTS/DMA3 and SeekError caller path; the original Plan 06-06 acceptance remains pending.

### Integration repairs within the focused caller path

- A successful SeekL schedules its target-sector callback so GetlocL can return the header expected immediately after seek completion. A regression covers the SeekL-to-GetlocL path.
- ReadN removes only the raw seek-header slot while preserving unrelated queued sectors. This was required by the full DMA caller and is covered by a queue-preservation test.
- The original reset assertion ran while a valid SeekL still had an asynchronous response pending. The test now asserts the SeekL INT3 at command acceptance and covers reset clearing SeekError with a separate fresh instance.

## Issues Encountered

The first valid SeekL acknowledgment failure came from advancing 67 million emulated cycles, which consumed the asynchronous interrupt before the explicit check. The invalid-seek completion now advances only its scheduled CD command event, while the valid SeekL still checks the actual INT3 at acceptance. The focused tests pass without weakening interrupt assertions or changing guest bounds.

## User Setup Required

None.

## Next Phase Readiness

Plan 06-29 is complete with partial DISC-01 coverage. Execute Plan 06-30 next to retain validated Q records, remove the universal target-minus-five rule, and pass the pinned Getloc guest's exact Q rows. Resume Plan 06-06 only after Plan 06-30; the full timing/Getloc gate and Phase 6 terminal closeout remain pending.

## Self-Check: PASSED

The summary exists, and commits `9e1d1fa`, `0fb2868`, and `a299e5c` are ancestors of the current HEAD.

---
*Phase: 06-discs-and-fmv*
*Completed: 2026-11-16*
