---
phase: 06-discs-and-fmv
plan: "12"
subsystem: testing
tags: [mdec, references, nightly, ctest]
requires:
  - phase: 06-discs-and-fmv
    provides: MDEC command/DMA caller and registered model-test infrastructure
provides:
  - Pinned MDEC block/status guests, digest-checked nightly registrations, and comparator mutation controls
  - Reproducible failing guest selectors assigned to dependent Plan 06-35
affects: [mdec, nightly, phase-6-verification]
actuals:
  tokens: 15498
  tasks: 2
  commits: 4
plan_head_before: ace70a1c498901c20a3347409efcfaade9926b09
plan_head_after: f974070b3d48677febf93b2b5bdddde4b13cac90
tech-stack:
  added: []
  patterns: [digest-verified public guest references, host-observed RAM comparison, comparator mutation controls]
key-files:
  created: [cmake/MDECReferences.cmake, tests/mdec/reference.c]
  modified: [pins.txt, tests/nightly/tests.cmake, tests/nightly/inventory.txt, docs/hardware/mdec.md, .planning/ROADMAP.md, .planning/STATE.md, .planning/phases/06-discs-and-fmv/06-12-PLAN.md, .planning/phases/06-discs-and-fmv/06-13-PLAN.md, .planning/phases/06-discs-and-fmv/06-14-PLAN.md, .planning/phases/06-discs-and-fmv/06-35-PLAN.md]
key-decisions:
  - "Keep the guest selectors gating and assign the production DMA0 input/decode mismatch to dependent Plan 06-35."
  - "Use host-observed RAM for exact block bytes because guest TTY formatting contains literal placeholders."
  - "Do not infer cycle timing from the unstamped status log."
patterns-established:
  - "Nightly reference cases remain registered and fail visibly when the real guest does not complete."
requirements-completed: []
coverage:
  - id: D1
    description: "Pinned MDEC assets and four nightly selectors are registered; the 4-bit, 8-bit, and status guests expose the reproduced production mismatch."
    requirement: DISC-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/mdec-reference -R '^mdec\\.reference\\.4bit$' --no-tests=error --output-on-failure (exit 8; guest incomplete)"
        status: fail
      - kind: integration
        ref: "ctest --test-dir build/mdec-reference -R '^mdec\\.reference\\.8bit$' --no-tests=error --output-on-failure (exit 8; guest incomplete)"
        status: fail
      - kind: integration
        ref: "ctest --test-dir build/mdec-reference -R '^mdec\\.reference\\.status$' --no-tests=error --output-on-failure (exit 8; guest incomplete before order comparison)"
        status: fail
    human_judgment: false
  - id: D2
    description: "The block-byte and ordered-status comparators reject mutations, and the exact inventory remains sorted."
    requirement: DISC-03
    verification:
      - kind: unit
        ref: "cmake -S . -B build/mdec-reference -G Ninja -DSCHWAYSTATION_INVENTORY=nightly -DSCHWAYSTATION_MEDIA_REFERENCE_ONLY=mdec && cmake --build build/mdec-reference --target schwaystation-test-mdec-reference && ctest --test-dir build/mdec-reference -R '^mdec\\.reference\\.controls$' --no-tests=error --output-on-failure (exit 0)"
        status: pass
      - kind: other
        ref: "LC_ALL=C sort -c tests/nightly/inventory.txt (exit 0)"
        status: pass
    human_judgment: false

# Metrics
duration: "at least 1h26m55s wall; active time unavailable"
completed: 2026-11-16
status: complete
---

# Phase 6 Plan 12: Pinned MDEC Guest Oracle Harness Summary

**Digest-checked MDEC block and status guests are registered with mutation controls; guest completion and DISC-03 acceptance remain pending the dependent DMA0 repair.**

## Performance

- **Duration:** At least 1h26m55s wall from the durable 2026-11-16T08:10:43Z start marker through summary preparation at 2026-11-16T09:37:38Z; exact active time is unavailable.
- **Started:** 2026-11-16T08:10:43Z
- **Completed:** 2026-11-16T09:37:38Z (bounded oracle/control slice; full guest gate resumes after Plan 06-35)
- **Tasks:** 2
- **Files modified:** 12 across implementation and split metadata
- **Model:** Codex/OpenGSD 1.20.0; gpt-6-luna xhigh

## Accomplishments

- Added public-pin/member digest checks, nightly target wiring, and registered 4-bit, 8-bit, status-order, and comparator-control selectors. Reference executables and logs remain under `build/`.
- Verified the negative controls reject byte corruption and reordered/dropped status rows; the inventory sort check passes.
- Preserved the three failing real guest selectors and recorded the DMA0 input/decode trace for Plan 06-35. The status guest also fails before Done, so there is not yet an actual completed trace for the order comparator.

## Selector Evidence

| Selector | Exact command result | Evidence |
| --- | --- | --- |
| `mdec.reference.4bit` | `ctest --test-dir build/mdec-reference -R '^mdec\.reference\.4bit$' --no-tests=error --output-on-failure` — exit 8 | Guest did not reach Done within the bounded run; DMA0 transferred but MDEC had no output and DMA1 was never armed. |
| `mdec.reference.8bit` | `ctest --test-dir build/mdec-reference -R '^mdec\.reference\.8bit$' --no-tests=error --output-on-failure` — exit 8 | Guest did not reach Done; MDEC remained empty with no DMA1 transfer. |
| `mdec.reference.status` | `ctest --test-dir build/mdec-reference -R '^mdec\.reference\.status$' --no-tests=error --output-on-failure` — exit 8 | Guest stopped before Done with 12,333 TTY bytes; no complete event trace exists to compare ordering. |
| `mdec.reference.controls` | `ctest --test-dir build/mdec-reference -R '^mdec\.reference\.controls$' --no-tests=error --output-on-failure` — exit 0 | Byte mutation and reordered/dropped status mutations are rejected. |

The controls selector was run after the exact configure/build command in the plan. The guest log prints literal format placeholders, so block expectations are compared against host-observed RAM using independently pinned expected bytes. The status guest log is unstamped and provides no cycle measurement.

## Task Commits

1. **Pin and register first-caller block and status reference guests** - `f974070` (`test(06-12): register pinned MDEC guest oracles`)

The split and graph changes were committed before this bounded implementation slice: `befc53a`, `90a4709`, and `70f4646`.

## Files Created/Modified

- `cmake/MDECReferences.cmake` and `tests/mdec/reference.c` - pinned asset validation, real guest execution, output/status comparators, diagnostics, and mutation controls.
- `pins.txt`, `tests/nightly/tests.cmake`, `tests/nightly/inventory.txt`, and `docs/hardware/mdec.md` - pin ledger, nightly registrations, sorted inventory, and behavior/evidence documentation.
- `.planning/phases/06-discs-and-fmv/06-12-PLAN.md`, `06-35-PLAN.md`, `06-13-PLAN.md`, `06-14-PLAN.md`, `.planning/ROADMAP.md`, and `.planning/STATE.md` - bounded split and dependency bookkeeping.

## Decisions Made

- Keep the 4-bit, 8-bit, and status selectors gating; do not self-skip the observed guest failure.
- Assign the production input/decode repair and green full oracle gate to dependent Plan 06-35.
- Keep the status guest comparator active, but report its order result as unobserved until the guest completes.

## Deviations from Plan

The original plan crossed its 30-minute bound before the production input/decode mismatch could be repaired and validated. The plan was split: this bounded slice contains the pin, harness, and comparator controls; Plan 06-35 owns the source repair and passing full guest gate. No selectors or acceptance criteria were removed.

## Issues Encountered

The 4-bit guest submitted command `0x20000020` using DMA0 MADR `0x12000`, BCR `0x00010020`, and CHCR `0x01000201`; after the transfer, MDEC was idle with empty input/output and DMA1 had not been armed while the guest polled `0x80040000`. The 8-bit and status callers also fail before Done. Plan 06-35 records the observed state and will determine whether the status caller shares the decode defect or needs a separate minimal repair.

## User Setup Required

None.

## Next Phase Readiness

The pinned oracle harness and negative controls are reproducible. Plan 06-35 is next and owns the production repair; afterward, rerun all four selectors as the exact Plan 06-12 full gate. DISC-03 remains incomplete until that gate passes.

---
*Phase: 06-discs-and-fmv*
*Completed: 2026-11-16*

## Self-Check: PASSED

- Created reference harness files exist.
- Split/harness commits `befc53a`, `90a4709`, `70f4646`, and `f974070` are ancestors of HEAD.
- `gsd_run verify-summary .planning/phases/06-discs-and-fmv/06-12-SUMMARY.md` passed.
