---
phase: 07-faster-and-more-trustworthy-ci
reviewed: 2026-11-19T03:34:54Z
depth: standard
files_reviewed: 6
files_reviewed_list:
  - .github/workflows/ci.yml
  - quality.json
  - scripts/ci-critical-path.sh
  - tests/cmake/quality.cmake
  - tests/cmake/quality_controls.cmake
  - tests/hygiene/tests.cmake
findings:
  critical: 0
  warning: 1
  info: 0
  total: 1
  necessity: 0
status: issues_found
---

# Phase 7: Code Review Report

**Reviewed:** 2026-11-19T03:34:54Z
**Depth:** standard
**Files Reviewed:** 6
**Status:** issues_found

## Summary

The post-review changes fix the PR head SHA comparison, include the release-input route in the fingerprint, preserve CMake bracket data, require both bounds for bounded metrics, and add regression controls for those cases. One baseline issue remains: the updated PR-route fingerprint now enables the existing hard range even though that range was measured on a different route.

## Warnings

### WR-01: PR route fingerprint enables a push-only timing range

**Severity:** WARNING
**File:** quality.json:14-16
**Issue:** The stored 2,249–3,293 second range is documented as measured from two matching ci.yml push runs in .planning/phases/07-faster-and-more-trustworthy-ci/07-BASELINES.md:15,85-90. This change replaces the old push/main fingerprint with the production pull_request/non-main projection, so scripts/ci-critical-path.sh now applies those same numeric bounds to ordinary PR attempts when the new hash matches. The new checked_pr_baseline control uses a synthetic 2,706-second duration; it proves that the registry is wired to the production projection, but does not show that PR timings are comparable to the push cohort. The registry rationale also says the final-head hosted PR run will confirm comparability, so that evidence is still outstanding.
**Fix:** Before treating this range as a measured PR-route bound, record an exact current-head hosted PR attempt and confirm its workload and timing are comparable to the push cohort. If they are not, derive the PR-route interval from matching PR attempts; otherwise keep the PR metric deferred until the evidence is available. Record the command/run URL and the resulting bound rationale.

---

_Reviewed: 2026-11-19T03:34:54Z_
_Reviewer: the agent (gsd-code-reviewer)_  
_Depth: standard_
