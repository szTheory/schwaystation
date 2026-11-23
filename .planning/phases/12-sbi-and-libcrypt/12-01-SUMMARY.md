---
phase: 12-sbi-and-libcrypt
plan: "01"
subsystem: disc
tags: [sbi, cdrom, parser, mmio]
requires:
  - phase: 11-ecm-and-clonecd
    provides: host disc-image provider and CD-ROM MMIO test patterns
provides:
  - bounded host SBI parser seam with a format-1 known-answer vector
  - registered parser and CD-ROM MMIO baseline selectors on native and WASI inventories
affects: [SBI parser, M3U provider binding, CD-ROM Q handling, fuzz replay]
actuals:
  tokens: 4692
  tasks: 2
  commits: 1
tech-stack:
  added: []
  patterns: [atomic bounded parser output, independent MSF known-answer vector]
key-files:
  created: [host/disc_sbi.c, host/disc_sbi.h, tests/disc/sbi.c, tests/cdrom/sbi.c]
  modified: [cmake/DiscTargets.cmake, tests/disc/tests.cmake, tests/disc/inventory.txt, tests/cdrom/tests.cmake, tests/cdrom/inventory.txt]
key-decisions:
  - "The private parser takes an exclusive selected-image LBA count and leaves its empty output unchanged on failure."
  - "Keep a small MMIO GetlocP baseline beside the host parser vector so core command behavior has a registered selector from the start."
requirements-completed: [CRYPT-01]
coverage:
  - id: D1
    description: "An in-range format-1 SBI record maps its real absolute MSF to one bad-Q LBA; bad headers and truncated records publish no output."
    requirement: CRYPT-01
    verification:
      - kind: unit
        ref: "disc.image.sbi"
        status: pass
    human_judgment: false
  - id: D2
    description: "A good-Q sector read through CD-ROM MMIO is visible through GetlocP."
    requirement: CRYPT-01
    verification:
      - kind: integration
        ref: "cdrom.sbi"
        status: pass
    human_judgment: false
duration: 8min41s
completed: 2026-11-23
status: complete
---

# Phase 12 Plan 01: SBI parser and test skeleton Summary

**A private bounded SBI parser and registered CD-ROM MMIO baseline now cover the first valid format-1 record and good-Q GetlocP path.**

## Performance

- **Duration:** 8m 41s wall time
- **Started:** 2026-11-23T05:13:37Z
- **Completed:** 2026-11-23T05:22:18Z
- **Tasks:** 2
- **Files modified:** 9

## Accomplishments

- Added the private host parser interface and a format-1 record that maps absolute `00:03:00` to LBA 75.
- Added malformed signature and truncated-record atomicity checks.
- Registered `disc.image.sbi` and `cdrom.sbi`; both root CI and wasm32 inventories already select their directories.
- Added a short CD-ROM MMIO ReadN/GetlocP known-good Q control.

## Task Commits

1. **Add the host parser seam, first SBI tracer and MMIO control** - `fcdd81e` (`feat(12): add bounded SBI parser tracer`)

The test skeleton and parser behavior landed together as one buildable plan slice; the tracer first ran against a stub and failed as expected before the parser implementation replaced it.

## Files Created/Modified

- `host/disc_sbi.c`, `host/disc_sbi.h` - bounded parser API and first format-1 implementation.
- `tests/disc/sbi.c` - independent parser vector and malformed-input checks.
- `tests/cdrom/sbi.c` - MMIO ReadN/GetlocP baseline.
- `cmake/DiscTargets.cmake`, `tests/disc/tests.cmake`, `tests/cdrom/tests.cmake` - provider and test registration.
- `tests/disc/inventory.txt`, `tests/cdrom/inventory.txt` - sorted exact selectors.

## Decisions Made

- Use the selected image's exclusive LBA end as the parser range bound; check BCD MSF and subtract the 150-frame lead-in with 64-bit arithmetic.
- Do not interpret SBI's Q0–Q9 payload as a valid Q replacement.

## Deviations from Plan

None. The implementation and test skeleton were committed together because the registered parser target and tracer form one buildable slice.

## Issues Encountered

- The first MMIO test attempt consumed a response byte that GetlocP does not return. The existing command test confirmed the eight-byte response shape; the test was corrected and passed. The first parser run returned the expected failure against the temporary stub, then passed against the implementation.
- Cheap sorted-inventory, CI/wasm directory-selection, `git diff --check`, and 3,901-line budget checks passed before the focused build.
- `cmake --preset ci` and the two focused targets built successfully. `ctest --preset ci --no-tests=error -R '^disc[.]image[.]sbi$' --output-on-failure` passed with its two required legacy fixture setup checks (3/3, 0.12s); `ctest --preset ci --no-tests=error -R '^cdrom[.]sbi$' --output-on-failure` passed (1/1, 0.01s).

## User Setup Required

None.

## Next Phase Readiness

Plan 12-02 can add formats 2/3, malformed address cases, duplicate handling and explicit parser limits to the registered host tracer.

---
*Phase: 12-sbi-and-libcrypt*
*Completed: 2026-11-23*
