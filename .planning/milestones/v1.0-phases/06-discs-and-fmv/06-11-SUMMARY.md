---
phase: 06-discs-and-fmv
plan: "11"
subsystem: mdec
tags: [mdec, dma, integer-idct]
requires:
  - phase: 06-03
    provides: "Instance-owned MDEC state, bus and scheduler integration"
  - phase: 06-06
    provides: "Phase 6 acceptance ordering before MDEC callers"
provides:
  - "Monochrome MDEC command, table, block and DMA0/1 baseline"
affects: [mdec, dma, state, phase-06-plan-31]
actuals:
  tokens: 7161
  tasks: 2
  commits: 2
plan_head_before: 97b1a66b42fad9a8bb2f0b549da0a4d37090cf31
plan_head_after: e155b2906f723520a021f8562362ca45fea9da2c
commits: 2
tech-stack:
  added: []
  patterns: ["Bounded instance-owned MDEC FIFOs and scheduled block deadlines"]
key-files:
  created: [tests/mdec/model.c, tests/mdec/tests.cmake, tests/mdec/inventory.txt]
  modified: [src/mdec.c, src/mdec.h, src/dma.c, tests/tests.cmake]
key-decisions:
  - "Use 1,000 cycles as the selected deterministic block-availability baseline; the status log has no cycle stamps."
  - "Move the remaining monochrome stream, FIFO and documentation controls to dependent Plan 06-31 before pinned guests."
patterns-established:
  - "DMA0 and DMA1 use the existing device request line and transfer engine."
requirements-completed: []
coverage:
  - id: MDEC-MONO-DMA-BASELINE
    description: "One-block monochrome 4/8-bit decode reaches RAM through DMA0 and DMA1 after the selected deadline."
    requirement: DISC-03
    verification:
      - kind: integration
        ref: "cmake --preset ci && cmake --build --preset ci --target schwaystation-test-mdec schwaystation-test-dma && ctest --test-dir build/ci -R '^mdec\\.(block|status|bounds)$|^dma\\.' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
duration: 39m 33s
completed: 2026-11-16
status: complete
---

# Phase 6 Plan 11: Monochrome MDEC DMA Summary

**Monochrome MDEC command decoding now drives DMA0 input and DMA1 output with bounded 4/8-bit block availability.**

## Performance

- **Duration:** 39m 33s (measured from the 04:03:11Z start to this closeout record)
- **Started:** 2026-11-16T04:03:11Z
- **Completed:** 2026-11-16T04:42:44Z
- **Tasks:** 2
- **Files modified:** 7
- **Executor:** Codex/OpenGSD 1.20.0, gpt-6-luna xhigh

## Accomplishments

- Implemented bounded MDEC command, quant/scale table, RLE, integer IDCT, output packing and status behavior.
- Routed DMA0 writes and DMA1 reads through the existing DREQ-controlled DMA engine.
- Registered and passed the MDEC block/status/bounds tests and the generic DMA regression set.

## Task Commits

1. **Task 1: Decode one block through command ports and DMA0/1** — `c9c87f2` (test), `e155b29` (feat)
2. **Task 2: Register the first MDEC caller and baseline status checks** — included with `c9c87f2` and `e155b29`

**Plan metadata:** `2711e9c` (docs: complete monochrome MDEC baseline plan)

## Verification Evidence

After focused code review, the exact Plan 06-11 command passed with exit 0:

```text
cmake --preset ci && cmake --build --preset ci --target schwaystation-test-mdec schwaystation-test-dma && ctest --test-dir build/ci -R '^mdec\.(block|status|bounds)$|^dma\.' --no-tests=error --output-on-failure
```

CTest passed all 43 selected tests: 40 DMA tests and 3 MDEC tests. No hosted run URL applies to this local gate.

## Decisions Made

- The 1,000-cycle block deadline is a selected deterministic implementation baseline, not a cycle measurement from the pinned status log.
- Follow-up acceptance for preserving low-half EOB data, output FIFO backpressure, nonzero raw-output/DMA1 layout, and hardware documentation is assigned to Plan 06-31. Plan 06-12 now depends on that follow-up.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Test correction] Corrected the color quant-table test command length**
- **Found during:** Task 1
- **Issue:** The initial test sent the 16-word luma-only command while asserting all 128 quant bytes were loaded.
- **Fix:** Changed the test to select the two-table command and send its specified 32 words.
- **Files modified:** `tests/mdec/model.c`
- **Verification:** The block model and exact 43-test plan gate passed.
- **Committed in:** `e155b29`

### Scope split

The executor crossed the 30-minute forecast while completing the first MDEC caller and diagnosing its coverage. The focused review found that a low-half EOB could discard the next block's high-half DC and a nearly full output FIFO could drop a completed block. These are preserved as blocking follow-up work in Plan 06-31, which also adds raw nonzero-output, FIFO boundary and documentation evidence. No Phase 6 requirement was removed; DISC-03 remains pending.

**Total deviations:** 1 auto-fixed test issue and 1 evidence-driven plan split.
**Impact on plan:** The basic single-block DMA path is complete; stream continuity and FIFO backpressure must pass in Plan 06-31 before Plan 06-12 starts.

## Issues Encountered

The initial sandboxed Git commit could not create `.git/index.lock`. The authorized escalation path committed through the repository hook; the noreply identity and `.githooks` configuration were active.

## User Setup Required

None.

## Next Phase Readiness

Plan 06-31 is now the dependent Wave 8 follow-up, and Plan 06-12 depends on it. DISC-03 is not complete: the pinned block/status guests and later XA/SPU/display integration remain outstanding.

---
*Phase: 06-discs-and-fmv*
*Completed: 2026-11-16*

## Self-Check: PASSED

- Summary file exists at the required path.
- Task commits `c9c87f2` and `e155b29` are ancestors of HEAD.
- Plan metadata commit `2711e9c` is an ancestor of HEAD.
