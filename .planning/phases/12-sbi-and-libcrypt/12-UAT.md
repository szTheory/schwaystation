---
status: complete
phase: 12-sbi-and-libcrypt
source:
  - 12-01-SUMMARY.md
  - 12-02-SUMMARY.md
  - 12-03-SUMMARY.md
  - 12-04-SUMMARY.md
  - 12-05-SUMMARY.md
  - 12-06-SUMMARY.md
  - 12-07-SUMMARY.md
started: 2026-11-23T08:39:00Z
updated: 2026-11-23T08:39:00Z
---

## Current Test

[testing complete]

## Tests

### 1. Bind SBI records to the selected disc provider
expected: A matching SBI sidecar affects only the selected provider's addressable sectors; malformed or mismatched sidecars cannot mark another disc's sectors as bad Q, including after playlist selection changes.
result: pass
evidence: `cmake --workflow --preset ci` at the verified phase head exited 0; all 674 tests passed. The phase verification traced `host/disc_image.c`, `tests/disc/sbi.c`, and `tests/cdrom/sbi.c` through the selected-provider integration path.

### 2. Preserve data and the previous good Q position for bad-Q sectors
expected: Reading a listed bad-Q sector still returns its data, while GetlocP reports the previous valid position after a read or seek.
result: pass
evidence: `cmake --workflow --preset ci` at the verified phase head exited 0; all 674 tests passed, including the selected-provider CD command sequence and read/seek regressions documented in the phase summaries and verification report.

### 3. Replay malformed SBI inputs with deterministic bounded results
expected: The parser rejects malformed inputs within its bounds, and the named seed corpus replays expected results without changing the output on failure.
result: pass
evidence: `cmake --workflow --preset ci` at the verified phase head exited 0; all 674 tests passed, including `fuzz.regress.sbi`. The sanitizer replay also passed 1/1. The mutation campaign is a separate hosted Ubuntu Nightly check and is still pending ship.

### 4. Reuse regional PCM without dropping libretro comparisons
expected: Each region has one verified runner PCM capture reused for both callback modes, retaining all six comparisons and the existing size, alignment, hash and status assertions.
result: pass
evidence: `ctest --preset ci --no-tests=error -R '^libretro[.]host$' --output-on-failure` exited 0; 1/1 passed in 131.00s. The full CI workflow also passed 674/674 tests and the Release checks 8/8.

## Summary

total: 4
passed: 4
issues: 0
pending: 0
skipped: 0
blocked: 0

## Gaps

None. The hosted Nightly mutation campaign remains a required ship check and is not claimed as completed here.
