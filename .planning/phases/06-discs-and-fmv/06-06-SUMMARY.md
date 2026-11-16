---
phase: 06-discs-and-fmv
plan: "06"
subsystem: cdrom
tags: [cdrom, timing, getloc, nightly-reference]
requires:
  - phase: 06-04
    provides: Deterministic CD-ROM command and response model
  - phase: 06-05
    provides: Sector delivery and controller buffer
  - phase: 06-27
    provides: DMA3 sector-word transfer path
  - phase: 06-28
    provides: CD readiness across chopped DMA windows
  - phase: 06-29
    provides: ReadN status, SeekError recovery, and pinned DMA caller
  - phase: 06-30
    provides: Validated raw Q records and exact GetlocP rows
provides:
  - Pinned timing and Getloc callers run through the production CD host interface
  - Reference timing comparisons retain eligible outliers and state their limits
affects: [cdrom, nightly, phase-6-verification]
actuals:
  tokens: 24905
  tasks: 2
  commits: 16
  plan_head_before: 51649b19b061b44cd5a112c66df26a8346981988
  plan_head_after: 2d93859a3213f3f75025beb615f4f6dff6837f50
tech-stack:
  added: []
  patterns:
    - Verify pinned guest and log digests before execution; keep fetched bytes in build storage.
    - Compare timing only for eligible consecutive-LBA callbacks and reject sample-buffer overflow.
key-files:
  created:
    - cmake/CDReferences.cmake
    - tests/cdrom/reference.c
  modified:
    - .github/workflows/nightly.yml
    - docs/hardware/cdrom.md
    - pins.txt
    - src/cdrom.c
    - src/cdrom.h
    - src/cdrom_sector.c
    - tests/cdrom/model.c
    - tests/nightly/inventory.txt
    - tests/nightly/tests.cmake
key-decisions:
  - "Keep fixed 451,584/225,792-cycle schedule assertions separate from the pinned drive timing bands."
  - "Treat positive same-speed, read-active intervals between consecutive LBA callbacks as timing samples; exclude nonconsecutive LBA gaps and reject buffer overflow."
  - "Keep DISC-01 pending until the remaining Phase 6 media, fuzz, and integration acceptance is complete."
patterns-established:
  - "The CD-only nightly configuration reuses verified pins while leaving the default nightly lane intact."
  - "Getloc guest order/data controls and the timing comparator run without parsing the guest's literal `%u` placeholders as measurements."
requirements-completed: []
coverage:
  - id: D1
    description: "The pinned timing and Getloc callers pass with ordered rows, exact host-observed status/data checks, and timing-model limits stated separately from fixed cycle assertions."
    requirement: DISC-01
    verification:
      - kind: integration
        ref: "cmake -S . -B build/disc-reference -G Ninja -DSCHWAYSTATION_INVENTORY=nightly -DSCHWAYSTATION_MEDIA_REFERENCE_ONLY=cdrom && cmake --build build/disc-reference --target schwaystation-test-cdrom-reference && ctest --test-dir build/disc-reference -R '^cdrom\\.reference\\.' --no-tests=error --output-on-failure (exit 0; 3/3)"
        status: pass
    human_judgment: false
  - id: D2
    description: "Comparator controls reject a consecutive-LBA timing outlier and overflow, and ignore a nonconsecutive callback gap."
    requirement: DISC-01
    verification:
      - kind: integration
        ref: "cmake --build build/disc-reference --target schwaystation-test-cdrom-reference && ctest --test-dir build/disc-reference -R '^cdrom\\.reference\\.(timing|controls)$' --no-tests=error --output-on-failure (exit 0; 2/2)"
        status: pass
    human_judgment: false
metrics:
  duration: ">=2h35m20s wall lower bound from Plan 06-29's 2026-11-16T00:43:26Z marker through Plan 06 closeout preparation; original start and active time unavailable"
  completed: 2026-11-16
  commits: 16
status: complete
---

# Phase 6 Plan 06: Pinned CD Timing and Getloc Callers Summary

**The pinned CD timing and Getloc guests pass the focused CD nightly lane with exact ordered results and explicit timing limits.**

## Performance

- **Duration:** At least 2h35m20s wall time from the earliest durable marker in dependent Plan 06-29 through closeout preparation. Plan 06 began earlier; its exact start and active time were not captured.
- **Started:** Not captured. The execution was already active before Plans 06-27 through 06-30 were split out as required dependent work.
- **Completed:** 2026-11-16
- **Tasks:** 2/2
- **Files modified:** 11
- **Runtime/model:** Codex / OpenGSD 1.20.0; executor gpt-6-luna xhigh.

## Accomplishments

- Ran the pinned timing and Getloc guests against the production CD interface. The final timing, Getloc, and control tests passed 3/3; fetched executable bytes and reference artifacts remain under `build/disc-reference`.
- Kept exact Getloc status/order/data controls separate from the literal TTY templates, whose `%u` fields contain no measurements. Fixed 1x/2x cycle assertions remain separate from the pinned timing bands.
- Made timing samples explicit: positive intervals are retained for same-speed, read-active consecutive LBA callbacks; nonconsecutive LBA gaps are excluded and a full sample buffer fails the comparator. The documented boundary includes adjacent-LBA restarts because the host callback supplies no read-generation marker.
- The focused nightly configuration selects only the CD reference lane; the normal nightly configuration remains in place.

## Task Commits

1. **Task 1: Fetch and execute the real CD reference guests** — `0fb2868` (the pinned harness/pins were integrated with the dependent caller work).
2. **Task 2: Make reference comparison limits and failure direction explicit** — `2d93859` (timing outlier, nonconsecutive-gap, and overflow controls).

The measured Plan 06 commit range is 16 commits from `51649b1` through `2d93859`; it includes the sequential dependent Plan 06-27 through Plan 06-30 commits, each of which has its own summary and ledger. This Plan's metadata commit is separate.

## Verification

The exact final Plan 06 gate, run from the repository root, exited 0:

```sh
cmake -S . -B build/disc-reference -G Ninja -DSCHWAYSTATION_INVENTORY=nightly -DSCHWAYSTATION_MEDIA_REFERENCE_ONLY=cdrom && cmake --build build/disc-reference --target schwaystation-test-cdrom-reference && ctest --test-dir build/disc-reference -R '^cdrom\.reference\.' --no-tests=error --output-on-failure
```

CTest passed `cdrom.reference.timing`, `cdrom.reference.getloc`, and `cdrom.reference.controls` (3/3). The focused timing/control rerun passed 2/2. Static rereview of the comparator and pin/fetch logic found no blocker. The full gate is local; no hosted run URL applies.

An earlier run exited 8 when a 5,232,758-cycle gap between repeated LBA 16 callbacks was treated as a sector interval. The final comparator restricts timing samples to consecutive LBA callback pairs without dropping large eligible deltas. Controls prove that a 999,999-cycle `100→101` interval is rejected, a nonconsecutive `100→16` gap is excluded, and sample-buffer overflow is rejected. An adjacent-LBA restart can still include seek delay; the docs state this measurement boundary rather than claiming a general seek curve.

## Decisions Made

- Keep observed hardware timing bands separate from deterministic 451,584/225,792-cycle model values.
- Define timing eligibility by observable consecutive LBA callback pairs because the host callback interface does not carry a read-generation identifier.
- Keep aggregate DISC-01 pending until the remaining Phase 6 media, fuzz, and integration work is complete.

## Deviations from Plan

### Evidence-driven scope split

The pinned caller exposed dependencies beyond the original timing/Getloc harness. The work was split into Plans 06-27 (DMA3), 06-28 (chopped-window readiness), 06-29 (ReadN/HSTS and caller data), and 06-30 (validated raw Q rows), then returned here for the full gate. No acceptance was removed. The measured wall lower bound includes this sequential dependency chain and exceeds the original 30-minute estimate.

### Auto-fixed comparator bug

**1. [Rule 1 - Bug] Preserve and reject out-of-band timing intervals**
- **Found during:** Task 2 final gate
- **Issue:** The comparator discarded intervals above twice the maximum pinned band and silently stopped recording when its fixed buffer filled, allowing eligible bad data to escape validation.
- **Fix:** Record all eligible positive intervals, fail if the interval buffer overflows, and add controls for eligible outliers and excluded nonconsecutive gaps.
- **Files modified:** `tests/cdrom/reference.c`, `docs/hardware/cdrom.md`
- **Verification:** Focused timing/control tests passed 2/2; final reference gate passed 3/3.
- **Committed in:** `2d93859`.

## Issues Encountered

The first corrected full gate passed Getloc and controls but failed timing on the repeated-LBA restart gap. LBA continuity made the timing definition observable without hiding large consecutive-LBA measurements; a stricter current-position check yielded no samples because valid Q state intentionally preserves the last good position. The final contract is documented and was rereviewed before the passing gate.

## User Setup Required

None.

## Next Phase Readiness

Plan 06-06 is complete. DISC-01 remains pending aggregate Phase 6 acceptance. Continue with Wave 8 Plan 06-09; do not run phase verification or ship until all remaining Phase 6 plans are complete.

## Self-Check: PASSED

- Summary path is `.planning/phases/06-discs-and-fmv/06-06-SUMMARY.md`.
- Task commits `0fb2868` and `2d93859` are ancestors of HEAD.
- Measured range is 16 commits from the saved Plan 06 base `51649b19b061b44cd5a112c66df26a8346981988` through `2d93859a3213f3f75025beb615f4f6dff6837f50`.

---
*Phase: 06-discs-and-fmv*
*Completed: 2026-11-16*
