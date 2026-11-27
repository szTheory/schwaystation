---
phase: 12-sbi-and-libcrypt
plan: "05"
subsystem: testing
tags: [sbi, cdrom, ccd, m3u, getlocp, compatibility]
requires:
  - phase: 12-03
    provides: selected image providers own their SBI callbacks
  - phase: 12-04
    provides: bad-Q sector delivery and previous-good CD-ROM Q state
provides:
  - Selected CCD child SBI behavior tested through CD-ROM MMIO
  - Narrow CRYPT-01 feature ledger evidence for data delivery and GetlocP
affects: [CRYPT-01, CD-ROM command path, disc fixture setup]
actuals:
  tokens: 6950
  tasks: 3
  commits: 3
commits: 3
plan_head_before: 9c3caafe4160c8fb9002c7a258c9979186472c35
plan_head_after: c0a36edce5f6ca5f2e5036cc1834827c809755d4
tech-stack:
  added: []
  patterns:
    - CTest fixture dependency for generated media
    - Bounded MMIO checks with full-buffer data comparison
key-files:
  created: []
  modified:
    - tests/cdrom/sbi.c
    - tests/cdrom/tests.cmake
    - tests/compat/features.csv
    - tests/compat/tests.cmake
    - cmake/DiscFixtures.cmake
key-decisions:
  - Reuse the generated legacy CCD/IMG/SUB fixture and patch only selected data/Q records.
  - Claim only selected-provider binding, bad-Q data availability, and previous-good GetlocP; make no retail-title claim.
  - Keep the old SBId marker out of the runtime media directory because .sbi is now a discovered sidecar name.
requirements-completed: []
coverage:
  - id: D1
    description: Selected M3U child SBI overlays deliver data for listed bad-Q sectors while GetlocP retains the prior good position across a swap.
    requirement: CRYPT-01
    verification:
      - kind: integration
        ref: ctest --preset ci --no-tests=error --output-on-failure -R '^cdrom[.]sbi$'
        status: pass
    human_judgment: false
  - id: D2
    description: The compatibility ledger records only the provider, bad-Q data, and previous-good GetlocP predicates demonstrated by the focused tests.
    requirement: CRYPT-01
    verification:
      - kind: other
        ref: ctest --preset ci --no-tests=error --output-on-failure -R '^compat[.]features[.](preflight|ratchet)$'
        status: pass
    human_judgment: false
metrics:
  duration: 37min
  completed: 2026-11-23
status: complete
---

# Phase 12 Plan 05: Selected-provider SBI command test Summary

**Selected CCD child SBI sectors stream guest data while GetlocP preserves the previous valid position across a disc swap.**

## Performance

- **Duration:** 37 minutes through execution, verification, and GSD state closeout.
- **Started:** 2026-11-23T06:08:22Z
- **Completed:** 2026-11-23T06:45:55Z
- **Tasks:** 3
- **Files modified:** 5
- **Runtime/model:** Codex GSD executor; configured `gpt-6-luna` xhigh; primary runtime model ID unavailable.

## Accomplishments

- Added a short A-to-B M3U MMIO sequence using build-owned copies of the existing 222-sector legacy CCD/IMG/SUB fixture. The test verifies child-specific bad-Q callbacks, sector data, target progression, swap reset, and all eight GetlocP bytes.
- Added cheap generated-file checks for source IMG/SUB extents and SBI header, record count, and absolute addresses before CD emulation. Existing `disc.image.sbi` vectors continue to check parser mapping; a CTest selector-presence query and sorted inventory checks passed before the broader group.
- Added one narrow CCD feature row and its independent expected row. Removed the obsolete runtime copy of the `SBId` marker that collided with automatic same-stem SBI discovery; explicit fixture tests still read the marker from its source path.

## Test Evidence

| Command | Result |
| --- | --- |
| `cmake --build --preset ci --target schwaystation-test-cdrom-sbi` | Exit 0 |
| `ctest --preset ci --no-tests=error --output-on-failure -R '^cdrom[.]sbi$'` | Exit 0; required legacy preflight and `cdrom.sbi` passed |
| `ctest --preset ci --no-tests=error --output-on-failure -R '^compat[.]features[.](preflight|ratchet)$'` | Exit 0; 2/2 passed in 0.54s |
| `ctest --preset ci --no-tests=error --output-on-failure -R '^disc[.]image[.]sbi$'` | Exit 0; legacy preflight, legacy parity, and SBI passed 3/3 in 0.12s |
| `ctest --preset ci --no-tests=error --output-on-failure -R '^(cdrom[.].*|disc[.]image[.].*)$'` | Exit 0; 39/39 passed, 0.16s wall time |
| Selector presence, sorted disc/CD-ROM inventories, `git diff --check`, and the 3,901-line budget | Exit 0; budget remained 3,901/3,901 |

No hosted CI, Act run, Docker resource, or retail title was used for this plan. The local CD-ROM/disc-image group totaled 0.47 test-seconds and 0.16 seconds wall time; it exposed no safe suite-level speed reduction. The main iteration delay was a fixture naming collision, repaired before the successful broad group.

## Task Commits

1. **Task 1: Drive CD commands through a selected image and verify bad-Q data and GetlocP** — `78475bf` (`test(12-05): exercise SBI through selected CD-ROM providers`)
2. **Task 2: Keep the feature ledger limited to demonstrated SBI behavior** — `51f8dcc` (`test(12-05): record narrow SBI evidence`)
3. **Task 3: Run cheap guest-input and selector preflights** — covered by the Task 1 runtime input preflight and the one-off selector/inventory checks; no separate file change.
4. **Blocking fixture integration repair** — `c0a36ed` (`fix(12-05): keep marker outside runtime media directory`)

## Files Modified

- `tests/cdrom/sbi.c` — generated child overlays, exact MMIO data/Q assertions, and early fixture-size/record checks.
- `tests/cdrom/tests.cmake` — selected image provider linkage and a required legacy-fixture setup edge.
- `tests/compat/features.csv` and `tests/compat/tests.cmake` — narrow evidence row and independent expected row dated 2026-11-23.
- `cmake/DiscFixtures.cmake` — stopped copying the historical marker as a malformed runtime SBI sidecar.

## Decisions Made

- Reused the generated legacy CCD/IMG/SUB fixture rather than adding another CCD generator. Only chosen IMG data/submode bytes and SUB Q packets are patched in the test-owned child copies.
- The feature evidence names generic selected-provider behavior only; it does not claim a protected retail game works.
- Compared each 2,048-byte payload as one buffer and stopped the command sequence after a failed interrupt/result guard. This keeps a bad test run bounded and readable.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Keep the legacy fixture marker out of runtime media**

- **Found during:** Task 3 selector/preflight run.
- **Issue:** `DiscFixtures.cmake` copied the historical 116-byte `SBId` test marker beside `disc_boot.cue`. The new sidecar discovery correctly rejected it as malformed, causing the legacy parity fixture gate to fail.
- **Fix:** Removed only that runtime copy. Fixture tests already pass the marker from `tests/fixtures/src/disc_boot.sbi` explicitly.
- **Files modified:** `cmake/DiscFixtures.cmake`.
- **Verification:** `disc.image.sbi` plus its legacy preflight/parity setup passed 3/3; the CD-ROM/disc-image group passed 39/39.
- **Committed in:** `c0a36ed`.

**Total deviations:** 1 auto-fixed (Rule 3 - Blocking).
**Impact on plan:** The small fixture repair restored existing media tests without changing SBI behavior or the fixed gate budget.

## Issues Encountered

The first command-test attempt used legacy Mode2 sectors whose duplicated submode byte was `0x20`, so the emulator correctly skipped them as non-data sectors. The test now marks only its selected records as data (`0x28`) and sets CD mode 0 before reading. That initial run was not valid RED evidence: it tested an invalid fixture setup, and the intended provider/core behavior was already implemented by Plans 12-03 and 12-04. No production code changed here, and no RED claim is made.

## User Setup Required

None.

## Next Phase Readiness

Plan 12-05 is complete and ready for the orchestrator to advance to Plan 12-06. No user action is needed. Phase 12 verification and ship remain outside this plan.

---
*Phase: 12-sbi-and-libcrypt*
*Completed: 2026-11-23*

## Self-Check: PASSED

The summary exists, and task commits `78475bf`, `51f8dcc`, and `c0a36ed` are ancestors of the current plan head.
