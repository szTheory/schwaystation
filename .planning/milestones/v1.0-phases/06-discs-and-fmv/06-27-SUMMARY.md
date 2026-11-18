---
phase: 06-discs-and-fmv
plan: "27"
subsystem: CD-ROM DMA3
tags: [cdrom, dma3, timing]
dependency_graph:
  requires:
    - 06-05
  provides:
    - DMA3 sector transfer bytes and documented transfer rates
  affects:
    - 06-28
    - 06-06
tech_stack:
  added: []
  patterns:
    - CD-ROM readiness is separate from DMA DREQ lines
    - CPU RDDATA and DMA3 share the sector cursor
key_files:
  created: []
  modified:
    - src/dma.c
    - src/dma.h
    - src/cdrom.c
    - src/cdrom.h
    - src/cdrom_sector.c
    - tests/cdrom/sectors.c
    - tests/dma/dma_tests.c
    - docs/hardware/dma.md
decisions:
  - DMA3 starts only when CHCR is triggered, BFRD is enabled, and sector data is buffered; CD-ROM does not assert DREQ.
  - The chopped-window readiness edge is isolated in sequential Plan 06-28.
metrics:
  duration: "41m 08s"
  completed_date: "2026-11-15"
status: complete
requirements-completed: []
coverage:
  - id: DISC-03
    description: DMA3 transfers ready callback sector bytes through the shared RDDATA cursor and selects 24/40-cycle rates.
    requirement: DISC-03
    verification:
      kind: integration
      ref: "cmake --preset ci && cmake --build --preset ci --target schwaystation-test-cdrom schwaystation-test-dma && ctest --test-dir build/ci -R '^cdrom\\.(sectors|delivery)$|^dma\\.' --no-tests=error --output-on-failure; exit 0; 42 tests passed"
      status: partial
    human_judgment: false
actuals:
  tokens: 6579.5
  tasks: 1
  commits: 4
plan_head_before: 51649b19b061b44cd5a112c66df26a8346981988
plan_head_after: ff5922cfa4a675ecd08f32dd07e5ae94a998db36
---

# Phase 06 Plan 27: DMA3 sector transfers and rates

DMA3 now transfers callback-provided sector bytes from the shared CD RDDATA cursor, waits for initial BFRD/data readiness without using DREQ, and charges the documented DEV5-selected 24/40-cycle rate.

## TDD Gate Compliance

- **RED:** `cmake --build --preset ci --target schwaystation-test-cdrom schwaystation-test-dma && ctest --test-dir build/ci -R "^(cdrom\\.sectors|dma\\.cdrom\\.costs)$" --no-tests=error --output-junit build/plan27-red-junit.xml --output-on-failure` exited 8. The recorded failures showed zero DMA sector data/cursor movement and the unimplemented 40-cycle setting taking 1,540 rather than 2,564 cycles. `gsd_run check tdd-red-evidence build/plan27-red-evidence.json --raw` accepted the JUnit evidence as `RED_EVIDENCE_OK`.
- **GREEN:** `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-cdrom schwaystation-test-dma && ctest --test-dir build/ci -R '^cdrom\\.(sectors|delivery)$|^dma\\.' --no-tests=error --output-on-failure` exited 0 with all 42 selected tests passing.
- **REFAC:** The tail assertion address was corrected from `0x4FFC` to `0x47FC` after RAM layout inspection; no production behavior was changed for that correction.

## Deviations from Plan

### Auto-fixed issues

1. **[Rule 1 — readiness bug]** The first implementation allowed triggered DMA3 to complete with zeros when BFRD was off or no sector was buffered. The implementation now keeps DMA3 pending until both conditions are ready, and it wakes the engine after a sector arrives or BFRD is enabled. Tests also seed ready CD data in timing/state fixtures and keep the generic zero-source test on MDEC output. Files: `src/dma.c`, `src/dma.h`, `src/cdrom.c`, `src/cdrom.h`, `src/cdrom_sector.c`, `tests/cdrom/sectors.c`, `tests/dma/dma_tests.c`.
2. **[Rule 1 — test fixture]** The first full lane exposed an empty-buffer assumption in `dma.read.chain_saves`; seeding its CD buffer restored the intended channel-chain and state-round-trip coverage.

### Review-driven split

The spec review found that the `DMA_PH_NEXT` early return can bypass readiness if BFRD is disabled or the buffer is reset during a chopped-transfer CPU gap. Plan 06-28 now owns this focused regression and fix. Wave 7 runs 06-27, then 06-28, then the pinned Getloc caller in 06-06. No Plan 27 behavior or acceptance was removed.

## Review and Verification

- Code review found no remaining Plan 27 gaps after the initial-readiness correction. Reviewers confirmed the callback data, shared cursor, tail/reset behavior, DEV5 rates, and initial wait/wake behavior; the chopped-window edge is assigned to Plan 06-28.
- `src/cdrom.c` is 1,199 lines, below the project cap. `git diff --check` passed.
- Final configure/build/focused CTest command is recorded above. It selected all 40 DMA tests and both CD sector tests. No hosted run applies; no fetched executable or binary reference artifact was committed.
- The original 20–30 minute forecast was exceeded: wall time from the recorded 23:09:01 UTC start to this closeout was at least 41 minutes. The review-driven readiness work and required split account for the extension. Executor runtime/model: Codex/OpenGSD 1.20.0, `gpt-6-luna xhigh`.

## Remaining Aggregate Acceptance

DISC-03 remains pending for the later MDEC/FMV caller plans. Plan 06-28 must establish chopped-window readiness before Plan 06-06's pinned Getloc guest is rerun.

## Commits

- `d444f92` — add failing DMA3 transfer tests
- `34f5c0d` — cover DMA3 tail and reset
- `556e4c4` — connect DMA3 to CD sector data
- `ff5922c` — wait for ready CD data

## Self-Check: PASSED

The summary exists, all four measured task commits are ancestors of HEAD, and `git diff --check` passed.
