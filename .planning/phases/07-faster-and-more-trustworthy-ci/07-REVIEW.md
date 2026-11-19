---
phase: 07-faster-and-more-trustworthy-ci
reviewed: 2026-11-19T02:36:37Z
depth: standard
files_reviewed: 20
files_reviewed_list:
  - .github/workflows/ci.yml
  - .github/workflows/nightly.yml
  - .github/workflows/release.yml
  - .github/workflows/triage.yml
  - README.md
  - docs/maintaining.md
  - docs/testing.md
  - quality.json
  - scripts/ci-critical-path.sh
  - scripts/classify-changes.sh
  - scripts/triage-report.sh
  - tests/cmake/ci_route_controls.cmake
  - tests/cmake/lessons_unique.cmake
  - tests/cmake/quality.cmake
  - tests/cmake/quality_controls.cmake
  - tests/cmake/triage_controls.cmake
  - tests/core/test_state.c
  - tests/hygiene/tests.cmake
  - tests/policy/inventory.txt
  - tests/policy/tests.cmake
findings:
  critical: 1
  warning: 4
  info: 0
  total: 5
  necessity: 0
status: issues_found
---

# Phase 7: Code Review Report

**Reviewed:** 2026-11-19T02:36:37Z  
**Depth:** standard  
**Files Reviewed:** 20  
**Status:** issues_found

## Summary

The new CI timing path has a pull-request SHA mismatch that will fail the required check on ordinary PR runs. The checked-in timing fingerprint also does not describe the route being measured, and there are additional holes in fingerprint and quality-registry validation. No tests were run as part of this read-only review.

## Critical Issues

### CR-01: Timing gate compares PR head SHA with merge SHA

**File:** `scripts/ci-critical-path.sh:25-39`  
**Issue:** For a `pull_request` event, `GITHUB_SHA` is the synthetic merge commit (the workflow checks it out at `.github/workflows/ci.yml:1095-1100`), but the Actions jobs API records each job's `head_sha` as the PR head SHA. The phase's own CI evidence notes that Actions exposes the PR head but not the merge-ref SHA, and sampled job records show that same head SHA on every job. The predicate `.head_sha == $sha` therefore rejects a valid current attempt before the timing calculation, causing the final required `CI required` job to fail on normal heavy PRs.

**Fix:** Pass the event's PR head SHA separately and compare job records against that value; keep the `actual_head == GITHUB_SHA` check for the checked-out merge commit. Add a fixture where the PR head and merge SHAs differ so this path is covered.

## Warnings

### WR-01: Checked-in critical-path fingerprint is for a different route

**File:** `quality.json:13-16`, `scripts/ci-critical-path.sh:147-182`  
**Issue:** The checked-in hash `316f8672…` is the historical `ci.yml / push / main` fingerprint in the Phase 7 efficiency report. The production script fingerprints `ci.yml / pull_request / non-main` at line 149. Event and branch class are part of the fingerprint contract, so the hashes cannot match. On mismatch, lines 180-182 exit successfully without applying the checked-in floor or ceiling. The critical-path bound is therefore currently deferred on every ordinary PR run.

**Fix:** Establish and record a matching PR-route baseline before treating TUNE-05 as enforced. Add a control that compares the production projection against the checked-in baseline so a stale route cannot silently leave the metric unbounded.

### WR-02: Fingerprint omits the release-input route

**File:** `scripts/ci-critical-path.sh:147-149`; `.github/workflows/ci.yml:953-959,1105-1108`  
**Issue:** The fingerprint route is always recorded as `full`; it does not include `CI_RELEASE_INPUTS`. A release-input PR runs the `rehearsal` and `cleanup` jobs, while an ordinary PR skips them. Those are different workloads but can receive the same fingerprint when their file projections otherwise match. This defeats the exact-workload comparison the metric relies on.

**Fix:** Include the release-input route in the fingerprint and test both route values. Keep the existing status handling that gives skipped rehearsal jobs zero duration only when release inputs are false.

### WR-03: CMake normalization strips data inside bracket arguments

**File:** `scripts/ci-critical-path.sh:161-162`; `tests/cmake/lessons_unique.cmake:12-25`  
**Issue:** The line-based comment filter removes every CMake source line beginning with `#`, including lines inside bracket arguments. In the lesson checker, headings such as `## Imported lessons` and `## Lessons learned here` are fixture data inside `[=[...]=]` strings and affect the parser's test behavior. Changing those values does not change the fingerprint, so the checker can treat different test workloads as comparable.

**Fix:** Preserve CMake files byte-for-byte in the workload hash, or use a syntax-aware normalizer that removes comments only outside quoted and bracket arguments. Add a fingerprint control that changes a bracket-string line beginning with `#` and expects the digest to change.

### WR-04: Missing quality floors are accepted for bounded metrics

**File:** `tests/cmake/quality.cmake:25-35`; `tests/cmake/quality_controls.cmake:75-77`  
**Issue:** `read_bound()` treats any missing `floor` as valid and passes an empty bound to the comparator for every metric, not only the explicitly unbounded Wasm metric. The control test explicitly expects a registry with the `state.bytes.floor` removed to accept a measurement of `1`. A later accidental floor deletion therefore disables that side of a checked-in metric without failing registry validation, despite TUNE-05 requiring ceilings and floors.

**Fix:** Require `floor` for each bounded metric and retain the existing special handling only for `size.wasm` when it has `unbounded: true`. Change the negative control to assert that a missing floor is rejected.

---

_Reviewed: 2026-11-19T02:36:37Z_  
_Reviewer: the agent (gsd-code-reviewer)_  
_Depth: standard_
