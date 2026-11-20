---
phase: 09-chd-disc-images
reviewed: 2026-11-20T17:44:59Z
depth: standard
files_reviewed: 11
files_reviewed_list:
  - libretro/CMakeLists.txt
  - libretro/schwaystation_libretro.info.in
  - scripts/compat-ratchet.sh
  - tests/cmake/compat-ratchet.cmake
  - tests/cmake/date_days.cmake
  - tests/cmake/info_claims.cmake
  - tests/cmake/info_claims_self_test.cmake
  - tests/cmake/perf_compare.cmake
  - tests/compat/tests.cmake
  - THIRD-PARTY-NOTICES.md
findings:
  critical: 1
  warning: 0
  info: 0
  total: 1
  necessity: 0
status: resolved
---

# Phase 9: Code Review Report

**Reviewed:** 2026-11-20T17:44:59Z
**Depth:** standard
**Files Reviewed:** 11
**Status:** resolved

## Summary

The metadata projection, shared date helper, protected-base transport and CHD notice additions were reviewed. The CHD metadata self-test contains an invalid regular expression, which prevents its new extension mutation control from reliably identifying the enabled configuration. No test suite was run during this review.

## Critical Issues

### CR-01: BLOCKER — Unbalanced CHD extension regular expression

**File:** `tests/cmake/info_claims_self_test.cmake:16`
**Severity:** BLOCKER
**Issue:** The pattern `(^|\\|)chd($|\\|` opens a second capture group but never closes it. CMake's regular-expression engine rejects the malformed pattern or fails to match the intended CHD token, so the self-test cannot select the correct inverse extension set for the CHD-on configuration. This leaves the claimed false-negative/false-positive CHD mutation control broken and may make the selector fail before its remaining controls run.
**Fix:** Close the group (or match the known suffix directly), for example:

```cmake
if(expected_extensions MATCHES "\\|chd$")
```

**Resolution:** Replaced the malformed expression with the terminal-suffix match above. A follow-up read-only review confirmed it matches the canonical enabled extension list and removes only `|chd`.

**Post-review verification:** The focused metadata self-test passed with CHD enabled and disabled (2/2 each).

---

_Reviewed: 2026-11-20T17:44:59Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
