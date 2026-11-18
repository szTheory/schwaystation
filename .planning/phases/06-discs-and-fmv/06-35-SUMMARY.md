---
phase: 06-discs-and-fmv
plan: "35"
subsystem: mdec
tags: [mdec, dma0, parser, pinned-reference]
requires:
  - phase: 06-discs-and-fmv
    provides: Pinned MDEC callers and registered model/reference tests
provides:
  - Bounded acceptance for a complete 64-coefficient block ending exactly at the pinned command boundary
  - Captured DMA0 source words linked to the pinned 4-bit guest digest
affects: [DISC-03, 06-36, 06-13, 06-14]
actuals:
  tokens: 13595
  tasks: 2
  commits: 3
plan_head_before: ce770edc89f2b4cd79a959529beecb00b2170f00
plan_head_after: 8c829fd
tech-stack:
  added: []
  patterns: [Reference-captured host RAM input reproduced through a focused model regression]
key-files:
  created: [.planning/phases/06-discs-and-fmv/06-36-PLAN.md]
  modified: [src/mdec.c, tests/mdec/model.c, tests/mdec/reference.c, docs/hardware/mdec.md, .planning/ROADMAP.md, .planning/STATE.md]
key-decisions:
  - "Publish a completed block only for the pinned 32-word command ending at coefficient 63; retain the malformed and incomplete-stream aborts."
  - "Keep exact 4/8-bit byte matching and status-guest completion as gating work in dependent Plan 06-36."
patterns-established:
  - "A complete 64-coefficient input may use the observed command boundary as its terminator only for the pinned bounded caller case."
requirements-completed: []
coverage:
  - id: D1
    description: "The captured pinned 32-word input publishes one complete block at the command boundary, while MDEC, DMA and state regressions pass."
    requirement: DISC-03
    verification:
      - kind: unit
        ref: "cmake --preset ci && cmake --build --preset ci --target schwaystation-test-mdec schwaystation-test-dma schwaystation-test-state && ctest --test-dir build/ci -R '^mdec\\.|^dma\\.|^core\\.state\\.' --no-tests=error --output-on-failure (exit 0; 56 tests passed)"
        status: pass
    human_judgment: false
  - id: D2
    description: "Pinned 4/8-bit exact bytes and ordered status completion remain registered and gating; output bytes still mismatch and the status guest stops before Done."
    requirement: DISC-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/mdec-reference -R '^mdec\\.reference\\.4bit$' --no-tests=error --output-on-failure (exit 8; first differing byte observed after command-boundary repair)"
        status: fail
      - kind: integration
        ref: "ctest --test-dir build/mdec-reference -R '^mdec\\.reference\\.8bit$' --no-tests=error --output-on-failure (exit 8; pinned RAM bytes differ)"
        status: fail
      - kind: integration
        ref: "ctest --test-dir build/mdec-reference -R '^mdec\\.reference\\.status$' --no-tests=error --output-on-failure (exit 8; guest stops before Done with command 0x000d0012)"
        status: fail
    human_judgment: false
  - id: D3
    description: "Pinned output-byte and ordered-status comparators reject their mutation controls."
    requirement: DISC-03
    verification:
      - kind: unit
        ref: "ctest --test-dir build/mdec-reference -R '^mdec\\.reference\\.controls$' --no-tests=error --output-on-failure (Plan 06-12; exit 0)"
        status: pass
    human_judgment: false
metrics:
  duration: "at least 30m55s wall"
  completed: 2026-11-16
status: complete
---

# Phase 6 Plan 35: Complete the pinned MDEC input block Summary

**The exact pinned 32-word command now publishes its full 64-coefficient block, with the broader output-byte and status callers explicitly left for Plan 06-36.**

## Performance

- **Runtime/model:** Codex/OpenGSD 1.20.0; gpt-6-luna xhigh.
- **Duration:** At least 30m55s wall from the durable start marker at 2026-11-16T09:39:27Z through the 2026-11-16T10:10:22Z gate snapshot; closeout continued afterward.
- **Tasks:** 2/2 complete for this bounded parser slice.
- **Files modified:** 24 tracked files across the parser, regression, split plan graph, and closeout.
- **Hosted run:** None; local focused checks only.

## Completed work

- Added the pinned guest's exact DMA0 input as a model regression. Before the fix, the input was consumed with no pending block or output readiness; afterward it publishes one pending block through the normal deadline.
- Completed a block only for the observed 32-word command that reaches coefficient 63 at command end. EOB behavior, malformed-run rejection, truncated-input aborts, cursor handling, and output backpressure remain in place.
- Kept a bounded failure-only diagnostic that dumps the captured 32 DMA0 source words for the pinned 4/8-bit command. Removed temporary full-byte and DMA-trace dumps.
- Created Plan 06-36 for the still-failing exact guest output bytes and ordered-status completion. Shifted later plans and added the dependency before MDEC color and owned-disc consumers.

## Gate evidence

The final bounded model command was:

```text
cmake --preset ci && cmake --build --preset ci --target schwaystation-test-mdec schwaystation-test-dma schwaystation-test-state && ctest --test-dir build/ci -R '^mdec\.|^dma\.|^core\.state\.' --no-tests=error --output-on-failure
```

It exited 0 with all 56 selected MDEC, DMA, and state tests passing, including `mdec.guest_input`. The registered real guests remain gating: the 4-bit and 8-bit cases reach DMA1 but their output bytes still differ from the pinned expected bytes; the status guest still ends before `Done` at command `0x000d0012`. Its log is unstamped and is used only for ordered events. Plan 06-36 owns those unresolved results.

## Task commits

- `3e7317f` — `test(06-35): reproduce pinned MDEC DMA0 input failure`
- `f2af568` — split 06-35 from the exact-byte/status follow-up and update the Phase 6 wave graph
- `8c829fd` — `fix(06-35): accept complete command-boundary MDEC block`

## Decisions made

- A missing EOB is accepted only at the command boundary for the exact full-block caller case observed in the pinned guest; this is not a general relaxation of the MDEC bitstream parser.
- Keep DISC-03 incomplete. Exact 4/8-bit output bytes and ordered status completion remain gating in Plan 06-36.
- Sync-mode-1 BCR high-half zero represents 65,536 blocks. Plan 06-36 must trace that actual request and the caller wait, not treat the field as a zero-block transfer.

## Deviations from plan

The original 06-35 acceptance bundled parser completion with exact output bytes and status-guest completion. The real caller passed the parser boundary but exposed separate output and status failures. Before continuing diagnosis, the remaining acceptance was split into dependent Plan 06-36; no selectors or expected bytes were removed.

## TDD gate compliance

- **RED:** `mdec.guest_input` failed before the parser correction because no completed block/output readiness was produced.
- **GREEN:** The focused model case and the 56-test MDEC/DMA/state gate pass after the correction.
- The pinned real guests remain enabled and fail visibly on the unresolved byte/status mismatches; those are not self-skipped.

## Self-Check: PASSED

- Summary, Plan 36, and updated ROADMAP/STATE files exist.
- Task and split commits `3e7317f`, `f2af568`, and `8c829fd` are ancestors of HEAD.
