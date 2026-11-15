---
phase: 05-the-sound-matches-the-console
plan: "21"
subsystem: audio
tags: [dma, spu, dev4, timing, guest]
requires:
  - phase: 05-the-sound-matches-the-console
    provides: "SPU DMA4 read/write and scheduled transfer state from plans 05-05 and 05-20"
provides:
  - "Timed DMA4 request blocks, guest-verified sync-mode-0 completion, and DEV4 reads"
  - "Bounded full memory-transfer guest comparator with all seven hardware cases in order"
affects:
  - "Phase 05 SPU ATTR ownership and nightly transfer registration in plan 05-23"
tech-stack:
  added: []
  patterns:
    - "SPU DMA4 request blocks resume at the next SPU sample deadline"
    - "Full guest output is checked through Done with every failure line rejected"
key-files:
  created: []
  modified:
    - src/dma.c
    - src/spu.c
    - tests/dma/dma_tests.c
    - tests/dma/tests.cmake
    - tests/dma/inventory.txt
    - tests/spu/model.c
    - tests/spu/reference.c
key-decisions:
  - "DMA4 sync-mode-0 starts from CHCR bit 24 alone, matching the pinned guest's register writes; other DMA channels retain their bit-28 trigger requirement."
metrics:
  duration: 46 min
  completed: 2026-11-14
  tasks: 1
  files: 7
actuals:
  tokens: 7428
  tasks: 1
  commits: 2
requirements-completed: []
coverage:
  - id: DMA4-REQUEST-BLOCKS
    description: "DMA4 request blocks preserve data, CPU progress, sample-deadline timing, and completion."
    requirement: SOUND-01
    verification:
      - kind: unit
        ref: "tests/dma/inventory.txt: dma.spu.request_blocks, dma.spu.request_controls"
        status: pass
    human_judgment: false
  - id: DMA4-SYNC0
    description: "Both DMA4 sync-mode-0 directions start from bit 24 alone, transfer data, and complete."
    requirement: SOUND-01
    verification:
      - kind: unit
        ref: "tests/dma/inventory.txt: dma.spu.sync0_start"
        status: pass
    human_judgment: false
  - id: DEV4-READ
    description: "DEV4 control suppresses the documented FFFFh insertion, while the default path reproduces it."
    requirement: SOUND-01
    verification:
      - kind: unit
        ref: "tests/spu/model.c: spu.model"
        status: pass
    human_judgment: false
  - id: TRANSFER-GUEST
    description: "The pinned full transfer guest passes its seven ordered hardware cases and reaches Done."
    requirement: SOUND-01
    verification:
      - kind: e2e
        ref: "schwaystation-test-spu-reference memory-transfer --full --assets build/ci/spu-reference"
        status: pass
    human_judgment: false
commits: 2
plan_head_before: e13c0b380fc23348e441e949fd26047ef2008ffe
plan_head_after: 810e84db4df7af5d1cf796c24a72ea294e70ef81
duration: 46 min
completed: 2026-11-14
status: complete
---

# Phase 5 Plan 21: DMA4 timing, DEV4 and the full transfer guest Summary

**Bidirectional DMA4 timing and DEV4 behavior pass focused checks; the full pinned transfer guest completes all seven hardware cases.**

## Performance

- **Duration:** 46 minutes
- **Started:** 2026-11-14T02:37:44Z
- **Completed:** 2026-11-14T03:23:27Z
- **Tasks:** 1
- **Files modified:** 7
- **Runtime/model:** Codex/OpenGSD 1.20.0; GPT-6 session (exact model ID not exposed)

## Accomplishments

- Timed subsequent DMA4 request blocks at SPU sample deadlines, preserving CPU execution between blocks.
- Matched the pinned transfer guest's sync-mode-0 CHCR behavior in both directions and verified data and completion.
- Implemented DEV4-controlled suppression of the periodic FFFFh insertion and a default-path regression check.
- Extended the bounded guest check through `Done.` while requiring all seven pinned hardware pass lines in order and rejecting every guest failure.
- Re-ran the exact tracer verification after implementation; all focused checks passed.

## Task Commits

1. **Task 1 RED: failing DEV4 and full guest checks** - `c64f0f6` (`test`)
2. **Task 1 GREEN: timed DMA4, sync-mode-0 completion, and DEV4 behavior** - `810e84d` (`feat`)

## TDD Cycle

- **RED evidence:** The new DEV4 model assertion failed on the missing FFFFh insertion; the full guest exposed immediate DMA timing and sync-mode-0 completion failures. `gsd_run check tdd-red-evidence` classified the targeted JUnit assertion as genuine RED evidence.
- **GREEN command:** `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-spu schwaystation-test-spu-reference schwaystation-test-dma && ctest --test-dir build/ci -R '^dma\.spu\.|^spu\.(model|reference\.controls|reference\.first_upload)$' --no-tests=error --output-on-failure && build/ci/tests/schwaystation-test-spu-reference memory-transfer --full --assets build/ci/spu-reference`
- **GREEN result:** exit 0. All six selected CTests passed, and the guest matched all seven cases in order through `Done.`
- **Tracer feedback:** Automated end-of-phase verification was rerun after the tracer commit and passed; no human visual check was required.

## Deviations from Plan

- Added a separately inventoried `dma.spu.sync0_start` unit test after the pinned guest showed DMA4 sync-mode-0 starts with bit 24 alone. This keeps the regression in the normal CTest run.
- Moved the original ATTR mirror and nightly-registration task to dependency-gated plan 05-23 after Task 1 exceeded the 30-minute target. Plan 05-21 was narrowed to the completed GREEN task; no RED-only work is reported complete.

## Verification

- `cmake --preset ci` - exit 0.
- `cmake --build --preset ci --target schwaystation-test-spu schwaystation-test-spu-reference schwaystation-test-dma` - exit 0.
- Focused CTest selection for DMA4/SPU model and reference checks - exit 0, 6 of 6 passed.
- Full pinned guest command - exit 0, seven ordered hardware cases and `Done.` observed.
- `git diff --check` - exit 0 before the implementation commit.

## Deferred Work

The duplicate bus ATTR field/state path and nightly registration are assigned to [05-23-PLAN.md](05-23-PLAN.md), which depends on this plan. SOUND-01 and SOUND-02 remain Pending until Phase-5 verification.

## User Setup Required

None. All checks were deterministic and agent-run.

---
*Phase: 05-the-sound-matches-the-console*
*Completed: 2026-11-14*

## Self-Check: PASSED

- Summary exists and both task commits are ancestors of the plan head.
- Commit count is measured from `e13c0b380fc23348e441e949fd26047ef2008ffe` through `810e84d`.
