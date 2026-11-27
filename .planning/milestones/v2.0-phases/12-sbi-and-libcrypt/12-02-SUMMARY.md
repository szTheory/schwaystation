---
phase: 12-sbi-and-libcrypt
plan: "02"
subsystem: disc
tags: [sbi, parser, bounds, fuzz-preparation]
requires:
  - phase: 12-01
    provides: registered host SBI parser and independent format-1 tracer
provides:
  - SBI format-1/2/3 mapping with checked real absolute MSF conversion
  - atomic malformed-input rejection, explicit local bounds and deterministic duplicate handling
affects: [SBI provider binding, fuzz replay]
actuals:
  tokens: 2318
  tasks: 2
  commits: 1
tech-stack:
  added: []
  patterns: [bounded multi-format records, sorted unique bad-Q LBAs]
key-files:
  created: []
  modified: [host/disc_sbi.c, host/disc_sbi.h, tests/disc/sbi.c]
key-decisions:
  - "Formats 2 and 3 use the real absolute MSF field and ignore their deliberately corrupted payload bytes."
  - "Accept duplicate sectors idempotently, sort the published LBA list, and cap files at 65,536 bytes and records at 8,192."
requirements-completed: [CRYPT-01]
coverage:
  - id: D1
    description: "All documented SBI formats map the real absolute MSF; malformed, invalid-BCD, pre-lead-in and out-of-map records fail without publishing partial output."
    requirement: CRYPT-01
    verification:
      - kind: unit
        ref: "disc.image.sbi"
        status: pass
    human_judgment: false
  - id: D2
    description: "Duplicate records are deterministic and idempotent, and oversized files or record counts are rejected within explicit implementation bounds."
    requirement: CRYPT-01
    verification:
      - kind: unit
        ref: "disc.image.sbi"
        status: pass
    human_judgment: false
duration: 3min30s
completed: 2026-11-23
status: complete
---

# Phase 12 Plan 02: SBI formats and bounds Summary

**The SBI parser now accepts formats 1–3, validates only the real absolute MSF, and publishes a bounded sorted set of unique bad-Q LBAs.**

## Performance

- **Duration:** 3m 30s wall time
- **Started:** 2026-11-23T05:24:09Z
- **Completed:** 2026-11-23T05:27:39Z
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments

- Added format-2 and format-3 mappings; both use the record's real absolute address and ignore corrupted payload fields.
- Added vectors for invalid BCD, invalid seconds/frames, addresses before the MSF lead-in, out-of-map sectors, unknown formats, and each partial record length.
- Added file-size and record-count caps, atomic failure checks, and sorted de-duplication.

## Task Commits

1. **Accept SBI formats 2/3 and bound malformed input** - `4082215` (`feat(12): validate SBI record variants and bounds`)

## Files Created/Modified

- `host/disc_sbi.c` - format dispatch, bounds, address validation and sorted unique outputs.
- `host/disc_sbi.h` - explicit implementation limits.
- `tests/disc/sbi.c` - positive vectors and malformed/boundary controls.

## Decisions Made

- The file and record caps are defensive implementation policy, not SBI format limits.
- Duplicate real sectors are harmless and collapse to one sorted LBA.

## Deviations from Plan

None.

## Issues Encountered

- The count allocation uses the smallest supported record size as its upper bound, then checks the actual record count while parsing. This avoids rejecting a valid all-format-1 file near the file-size cap.
- Sorted inventory, 3,901-line budget and diff checks passed. The focused parser CTest and its two required fixture setup checks passed (3/3, exit 0, 0.12s).

## User Setup Required

None.

## Next Phase Readiness

Plan 12-03 can attach the parser output to each concrete image provider and its M3U child without changing the core API.

---
*Phase: 12-sbi-and-libcrypt*
*Completed: 2026-11-23*
