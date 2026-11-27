---
status: complete
phase: 11-ecm-and-clonecd-images
source:
  - 11-01-SUMMARY.md
  - 11-02-SUMMARY.md
  - 11-03-SUMMARY.md
  - 11-04-SUMMARY.md
  - 11-05-SUMMARY.md
  - 11-06-SUMMARY.md
  - 11-07-SUMMARY.md
  - 11-08-SUMMARY.md
  - 11-09-SUMMARY.md
  - 11-10-SUMMARY.md
  - 11-11-SUMMARY.md
  - 11-12-SUMMARY.md
  - 11-13-SUMMARY.md
  - 11-14-SUMMARY.md
  - 11-15-SUMMARY.md
started: 2026-11-22T20:59:24Z
updated: 2026-11-22T21:00:15.135Z
---

## Current Test

[testing complete]

## Tests

### 1. Open supported ECM images
expected: Supported ECM streams return deterministic sector bytes on repeated and out-of-order reads; malformed streams fail within the documented bounds.
result: pass
evidence: `cmake --workflow --preset ci` at 961964a exited 0; all 670 CI tests passed. The phase verification traces the production reader, source-count preflight and regression cases.

### 2. Read CloneCD tracks and subcode
expected: Supported CCD/IMG/SUB images preserve the documented modes, indexes, pregaps, boundary sectors and subcode; escaping companion paths are rejected before opening.
result: pass
evidence: `cmake --workflow --preset ci` at 961964a exited 0; all 670 CI tests passed. The phase verification traces the reader, confinement tests and known-answer SUB transpose.

### 3. Load images through the runner and libretro core
expected: Consumers load and probe supported images through the shared reader, report unsupported formats clearly, and preserve active content when a replacement fails.
result: pass
evidence: `cmake --workflow --preset ci` at 961964a exited 0; all 670 CI tests passed. Phase plans also record provider-specific guest and consumer checks.

### 4. Package and advertise the supported formats
expected: CHD-off and CHD-on packages include the provider and notices, libretro metadata names ECM and CloneCD, and the optional installed SDL2 example compiles against the staged library.
result: pass
evidence: Phase 11 Plans 11-14 and 11-15 record the CHD-off package/archive checks, CHD-on consumers, metadata guard and installed SDL2 example build; the exact-head CI workflow passed release checks 7/7.

## Summary

total: 4
passed: 4
issues: 0
pending: 0
skipped: 0
blocked: 0

## Gaps
