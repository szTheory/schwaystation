---
phase: 06-discs-and-fmv
reviewed: 2026-11-16T14:03:43Z
depth: standard
files_reviewed: 5
files_reviewed_list:
  - tests/mdec/reference.c
  - tests/mdec/model.c
  - tests/mdec/tests.cmake
  - tests/mdec/inventory.txt
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

**Reviewed:** 2026-11-16T14:03:43Z

**Depth:** standard

**Files Reviewed:** 5

**Status:** issues_found

## Summary

The direct DMA1 caller contract is represented truthfully: the pinned helper writes its caller buffer pointer to DMA1 MADR without a CPU copy. The comparator reports the pinned hardware log address and this emulator run's DMA1 start separately, then checks the pinned bytes at the emulated start; their address difference is not treated as staging. The documentation distinguishes the matching 4-bit bytes from the unresolved 8-bit arithmetic mismatch. The focused `mdec.reference.4bit` test passed after this change (reported); no tests were run during this review. The prior status-oracle warning remains visible as a deferred tooling seed.

## Narrative Findings (AI reviewer)

## Warnings

### WR-01: Status guest oracle discards event identity and status-read count

**File:** `tests/mdec/reference.c:686-710`
**Severity:** WARNING
**Disposition:** Deferred tooling seed.
**Issue:** `collect_status_events` stores only event kinds and collapses consecutive events of the same kind. It accepts any status-read count of at least 100, and `status_order_matches` compares only the collapsed kind sequence. As a result, the pinned status selector can accept a run with missing or extra status reads, or different status/input/output values, whenever the remaining event-kind transitions match. This establishes event-class order, not an ordered match to the pinned records.
**Fix:** Preserve one parsed entry per event and compare exact event counts and order against the pinned log. Where TTY lines contain format placeholders, compare their expected status/data payloads against the guest's traced MDEC bus accesses rather than dropping payload identity.

---

_Reviewed: 2026-11-16T14:03:43Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
