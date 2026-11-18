---
phase: 06-discs-and-fmv
plan: "13"
reviewed: 2026-11-16
depth: standard
files_reviewed: 11
files_reviewed_list:
  - src/mdec.c
  - src/mdec.h
  - src/internal.h
  - tests/mdec/model.c
  - tests/mdec/color_reference.c
  - tests/mdec/reference.c
  - tests/mdec/tests.cmake
  - tests/mdec/inventory.txt
  - cmake/MDECReferences.cmake
  - tests/gpu/reference_png.py
  - .planning/phases/06-discs-and-fmv/06-13-PLAN.md
findings:
  critical: 0
  warning: 2
  info: 0
  total: 2
status: issues_found
---

# Plan 06-13: Code Review

## Summary

The MDEC FIFO state, chroma staging, 15/24-bit packing, and DMA backpressure changes are internally consistent with the synthetic model cases. The new exhausted-input check distinguishes an empty, completed decode command from a slow decode and makes the pinned guest failures fast and explicit. The pinned image references cannot currently validate the full-frame implementation because the guest requests more macroblocks than its embedded streams contain. Plan acceptance remains open.

## Warnings

### WR-01: Pinned color guests run out of compressed macroblocks

**Severity:** WARNING — acceptance blocker
**Files:** `tests/mdec/color_reference.c`, pinned frame/movie guest assets
**Evidence:** The pinned frame stream contains 296 complete macroblocks for a 320x240 caller that reads 300. The first pinned movie frame contains 162 for a 256x240 caller that reads 240. All four 15/24-bit CTest cases then wait on an empty MDEC output FIFO; the focused run fails in under a second total.
**Recommendation:** Replace these inputs with a pinned, complete frame and matching reference captures, or revise the plan around a separately verified complete oracle. Do not accept a partial display or synthesize missing decoder output.

### WR-02: Real guest raw-output hash is not recorded

**Severity:** WARNING
**Files:** `tests/mdec/color_reference.c`, `tests/mdec/model.c`
**Issue:** The model test hashes controlled raw FIFO words before DMA1 and compares them with the DMA1 RAM copy. The real guest reference test compares full VRAM and native display pixels, but does not hash or compare that guest's raw MDEC words. The plan's reference action asks for this distinction on the real caller path.
**Recommendation:** When a complete input/reference pair is available, capture the guest raw FIFO domain before DMA1 and compare it independently from VRAM and display interpretation.

## Resolved During Review Follow-up

The IDCT passes use floor division after adding `0x8000`, matching the pinned ares arithmetic shift and restoring the pinned monochrome guest vectors. The second pass wraps to signed 9-bit range with defined signed modulo before signed 8-bit saturation. The color/display model checks pass with that behavior, including the `+256`/negative extreme vectors. The bounds case now uses a genuinely truncated coefficient block instead of treating a run that reaches coefficient 64 as malformed.

The exact 8-bit guest-vector mismatch remains an existing open gate from Plans 06-36 through 06-39. It reproduced with the phase-start `src/mdec.c`; expected bytes remain unchanged. The Plan13 targeted color/display, FIFO, bounds, vector and 4-bit tests pass, while `mdec.guest_input` still reports the previously recorded 12 exact-word differences.

## Review Notes

No production memory-bound or FIFO-capacity issue was found in the reviewed color output path. The existing phase-level warning about the monochrome status-log comparator remains recorded in `06-REVIEW.md`.
