---
phase: 06-discs-and-fmv
plan: "28"
subsystem: CD-ROM DMA3
tags: [cdrom, dma3, chopping, readiness]
dependency_graph:
  requires:
    - 06-27
  provides:
    - CD readiness is rechecked at every chopped DMA3 window
  affects:
    - 06-06
tech_stack:
  added: []
  patterns:
    - Gate DMA_PH_NEXT on CD readiness before arbitration
key_files:
  created: []
  modified:
    - src/dma.c
    - tests/cdrom/sectors.c
    - docs/hardware/dma.md
decisions:
  - Reuse the CD BFRD and buffered-sector predicate at both initial start and chopped-window boundaries.
metrics:
  duration: "6m 00s"
  completed_date: "2026-11-15"
status: complete
requirements-completed: []
coverage:
  - id: DISC-03
    description: Chopped DMA3 waits during a readiness gap without overwriting RAM and resumes at the shared sector cursor.
    requirement: DISC-03
    verification:
      kind: integration
      ref: "cmake --build --preset ci --target schwaystation-test-cdrom schwaystation-test-dma && ctest --test-dir build/ci -R '^cdrom\\.sectors$|^dma\\.' --no-tests=error --output-on-failure; exit 0; 41 tests passed"
      status: partial
    human_judgment: false
actuals:
  tokens: 1012.5
  tasks: 1
  commits: 2
plan_head_before: c3175fe8a5a381fd87068ffc2058d81eada195a1
plan_head_after: 20894035cc3162358f4ca3bbd05420d8efc7780e
---

# Phase 06 Plan 28: chopped DMA3 readiness

DMA3 now rechecks BFRD and buffered-sector readiness before starting each chopped sync-0 window, leaving unavailable data untouched and resuming at the shared cursor when ready again.

## TDD Gate Compliance

- **RED:** `cmake --build --preset ci --target schwaystation-test-cdrom && ctest --test-dir build/ci -R '^cdrom\.sectors$' --no-tests=error --output-junit build/plan28-red.junit.xml --output-on-failure` exited 8. The failure showed DMA3 started the second window with BFRD disabled and overwrote the sentinel RAM byte. `gsd_run check tdd-red-evidence build/plan28-red-evidence.json --raw` returned `RED_EVIDENCE_OK`.
- **GREEN:** `cmake --build --preset ci --target schwaystation-test-cdrom schwaystation-test-dma && ctest --test-dir build/ci -R '^cdrom\.sectors$|^dma\.' --no-tests=error --output-on-failure` exited 0; all 41 selected tests passed.
- **REFAC:** No separate refactor was needed.

## Review and Verification

- Spec review passed: the readiness check now precedes the `DMA_PH_NEXT` early return and is limited to sync-0 DMA3. The regression covers callback data, the first word, waiting without a RAM write, and resumption at the next cursor position.
- Standards review found no style, source-limit, or citation issue. The test calls the internal DMA event to hit the CPU-gap boundary exactly; the sector itself is supplied through the normal host callback.
- `git diff --check` passed. No hosted run applies; generated test evidence remains in build storage.
- Runtime/model: Codex/OpenGSD 1.20.0, executor `gpt-6-luna xhigh`; wall time from 23:53:19 UTC start to summary creation was 6 minutes.

## Remaining Aggregate Acceptance

DISC-03 remains pending for the downstream MDEC/FMV caller plans. Plan 06-06 is next and can now run after both DMA3 readiness plans.

## Commits

- `cbd7850` — add chopped DMA3 readiness regression
- `2089403` — gate chopped DMA3 windows on readiness

## Self-Check: PASSED

The summary exists, both measured task commits are ancestors of HEAD, and `git diff --check` passed.
