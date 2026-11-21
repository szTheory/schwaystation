---
phase: 09-chd-disc-images
plan: 10
subsystem: infra
tags:
  - CI
  - GitHub Actions
  - release-artifacts
requires:
  - phase: 09-01
    provides: Existing CI matrix, required-check roll-up, and release archive validation
provides:
  - Exact-SHA job timings, runner-minute totals, matrix identity, and artifact digests for Main and Release
  - Evidence-backed decision to retain independent Main and Release CI gates
affects:
  - Phase 09 Plan 09-11 workflow integration
  - Release CI efficiency
actuals:
  tokens: 3668
  tasks: 2
  commits: 2
tech-stack:
  added: []
  patterns:
    - Measure workflow jobs and artifacts from exact-SHA Actions run and artifact metadata
    - Compare downloaded product archives with published release asset digests before considering reuse
key-files:
  created:
    - .planning/phases/09-chd-disc-images/09-CI-REUSE.md
  modified:
    - .planning/todos/pending/050-release-ci-artifact-reuse.md
key-decisions:
  - Retain independent Main and Release CI gates; current evidence proves duplicate runner cost, not a wall-clock saving
  - Same source SHA does not establish archive identity when all eighteen Main and Release package digests differ
requirements-completed:
  - CHD-01
coverage:
  - id: CI-REUSE-DISPOSITION
    description: Exact-SHA workflow costs, required checks, matrices, and package identities were measured and the safe reuse disposition was recorded.
    requirement: CHD-01
    verification:
      kind: other
      ref: "GitHub Actions run/job/artifact queries and archive digest comparison exited 0; `git diff --check origin/main...HEAD` and `git diff --check HEAD` exited 0. See `09-CI-REUSE.md`."
      status: pass
    human_judgment: false
plan_head_before: e99da4fe4a3f1d9b13d1e0d5b970547238c6b38d
plan_head_after: f12fbe7b72f54ed2fe707ea7cea99fd8d2ab7407
commits: 2
metrics:
  duration: 23m 09s
  completed: 2026-11-20
status: complete
---

# Phase 9 Plan 10: CI artifact reuse Summary

**Measured duplicate CI cost on the exact same source SHA and retained separate release gates because archive identity and a shorter critical path were not established.**

## Performance

- **Duration:** 23m 09s wall time from the previous saved STATE checkpoint; active time was not measured.
- **Started:** 2026-11-20T15:44:44Z
- **Completed:** 2026-11-20T16:07:53Z
- **Tasks:** 2
- **Files modified:** 2
- **Runtime/model:** Codex runtime; executor model gpt-6-luna.

## Accomplishments

- Measured 139.13 Main CI runner-minutes and 117.53 minutes for Release's repeated CI jobs from exact-SHA Actions data; the complete Release run finished before Main's BE32-dominated required path.
- Verified both runs used the same six build lanes and source SHA. All six artifacts in each run recorded that SHA and contained the expected eighteen product ZIPs.
- Compared all eighteen Main and Release product ZIP digests with the published v0.8.0 release. The Main and Release ZIPs differ in every pair; the Release ZIP digests match the published assets.
- Recorded the active ruleset's required `CI required` check, the release workflow dependency chain, the reuse failure cases that would need tests, and a retain disposition in todo 050.

## Verification and efficiency

- Queried both run records, complete job pages, artifact metadata, check runs, the active repository ruleset, and published release assets. Run and nested CI checks succeeded; the expected legacy branch-protection endpoint returned 404 because the branch uses a repository ruleset.
- Downloaded the six `archives-*` artifacts from each run. All twelve `commit.txt` files matched the exact source SHA; the release artifact ZIP hashes matched all eighteen published v0.8.0 asset digests. The one-off `jq` job-time aggregation and archive comparison exited 0.
- `git diff --check origin/main...HEAD` and `git diff --check HEAD` passed for both task commits.
- No build, test suite, `act`, or hosted run was needed for this read-only efficiency investigation. No permanent reporting helper was added.

## Task Commits

1. **Task 1: Measure one exact-SHA Main/Release pair** — `3847aa3` (`docs(09-10): record exact-SHA CI reuse measurements`)
2. **Task 2: Choose and record the smallest safe disposition** — `f12fbe7` (`docs(09-10): retain separate Main and Release CI gates`)

## Files Created/Modified

- `.planning/phases/09-chd-disc-images/09-CI-REUSE.md` — Job, runner-time, required-check, artifact and archive digest evidence, with the reuse decision.
- `.planning/todos/pending/050-release-ci-artifact-reuse.md` — Measured retain decision and the remaining Plan 11 workflow-invariant check.

## Decisions Made

- Keep Main and Release CI independent. Requiring Main's completed check before publishing would project about 88m31s for Release, approximately 24m25s longer than the measured run, while using the existing parallel checks finishes sooner.
- Do not substitute Main-run ZIPs into Release: their eighteen digests differ from Release-produced archives, so the artifact identity and delivery equivalence are unproven.
- Plan 11 should preserve all six package jobs, both CI-required checks, and exact-release-SHA checksum and attestation validation, then close todo 050 after checking its changed workflow.

## Deviations from Plan

None. The sandbox denied direct `.git` writes; a narrow reviewed escalation created the required commit ledger and allowed the two authorized task commits.

## User Setup Required

None.

## Next Plan Readiness

Plan 09-09 is next by dependency order. Plan 11 can use this measured disposition after its other dependencies complete. No owner verification or other user action is needed.

---
*Phase: 09-chd-disc-images*
*Completed: 2026-11-20*

## Self-Check: PASSED

The summary exists and task commits `3847aa3` and `f12fbe7` are ancestors of HEAD.
