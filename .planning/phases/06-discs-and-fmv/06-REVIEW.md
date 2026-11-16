---
phase: 06-discs-and-fmv
reviewed: 2026-11-16T13:03:44Z
depth: standard
files_reviewed: 4
files_reviewed_list:
  - src/mdec.c
  - tests/mdec/model.c
  - tests/mdec/reference.c
  - docs/hardware/mdec.md
findings:
  critical: 0
  warning: 1
  info: 0
  total: 1
  necessity: 0
status: issues_found
---

# Phase 06: Code Review Report

**Reviewed:** 2026-11-16T13:03:44Z

**Depth:** standard

**Files Reviewed:** 4

**Status:** issues_found

## Summary

The changed conversion and expected-byte paths were checked against the pinned 4-bit and 8-bit log bytes. One documentation mismatch remains: the hardware page describes a DMA1 destination comparison, while the reference comparator now checks the guest buffer address parsed from the digest-verified log.

## Narrative Findings (AI reviewer)

## Warnings

### WR-01: Documentation names the wrong RAM buffer checked by the guest comparator

**File:** `docs/hardware/mdec.md:81-83`
**Issue:** The page says the reference test compares the "DMA1 destination RAM" with the logged bytes. In `tests/mdec/reference.c:561-566`, the comparator instead parses the `mdec_readDecodedDma(addr=...)` address from the pinned log and checks RAM at that address; the DMA1 start captured at `tests/mdec/reference.c:585-588` is diagnostic only. The recorded DMA1 starts (`0x001ffeb8`/`0x001ffe98`) differ from the logged guest buffer targets (`0x001ffe0c`/`0x001ffdec`), so the documentation currently overstates what address the test validates.
**Fix:** Describe the check as comparing the guest buffer at the digest-verified `mdec_readDecodedDma` target, and state separately whether DMA1's direct destination is asserted or only reported diagnostically.

---

_Reviewed: 2026-11-16T13:03:44Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
