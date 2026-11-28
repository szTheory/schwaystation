---
phase: 17-a-measured-and-trustworthy-playability-baseline
plan: "05"
subsystem: testing
tags:
  - BE32
  - CTest
  - JUnit
  - GitHub Actions
dependency-graph:
  requires:
    - 17-02
  provides:
    - Current BE32 selector and preservation review
    - Updated TODO-052 evidence
  affects:
    - TUNE-07
    - TUNE-08
    - CI production-gate disposition
tech-stack:
  added: []
  patterns:
    - Read-only CTest selector enumeration
    - Exact JUnit report union proof
key-files:
  created:
    - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-BE32-REVIEW.md
  modified:
    - .planning/todos/pending/052-be32-movie-test-shard.md
decisions:
  - Keep the monolithic BE32 production job until a comparable hosted shard run proves a strict required-DAG improvement.
metrics:
  duration: 23m
  completed: 2026-11-28
  status: complete
  commits: 2
  plan_head_before: bcd55c66a9d2e4a8171127ce9714ee727f5b16ea
  plan_head_after: cbd7ed46a4b364de6378841ee0477221c9aaf737
actuals:
  tokens: 5201
  tasks: 2
  commits: 2
requirements-completed:
  - TUNE-07
  - TUNE-08
coverage:
  - id: current-selector-partition
    description: The exact audio, movie, and broad portable selectors form a complete 694-name partition.
    requirement: TUNE-07
    verification:
      kind: other
      ref: ctest --preset ci --show-only=json-v1 plus per-directory inventory and label comparison
      status: pass
    human_judgment: false
  - id: junit-union-proof
    description: The one-off report procedure accepts the full exact-name union and rejects six malformed cases.
    requirement: TUNE-07
    verification:
      kind: other
      ref: Disposable synthetic JUnit union proof using current CTest names
      status: pass
    human_judgment: false
  - id: hosted-be32-disposition
    description: Latest BE32 timings and the required CI path are recorded without claiming an unmatched shard speedup.
    requirement: TUNE-08
    verification:
      kind: integration
      ref: GitHub Actions run 45949888885, BE32 job 140343426896, and current 40-run inventory
      status: pass
    human_judgment: false
---

# Phase 17 Plan 05: BE32 shard evidence summary

**Proved the current 694-test BE32 partition and retained the production gate pending comparable hosted shard evidence.**

## Performance

- **Duration:** 23 minutes
- **Started:** 2026-11-28T03:17:24Z
- **Completed:** 2026-11-28
- **Runtime/model:** Codex with OpenGSD 1.23.0; executor model `gpt-6-luna` (primary model ID unavailable)
- **Tasks:** 2
- **Files changed:** 3 (two task artifacts and this summary)

## Accomplishments

- Recomputed the exact portable set from the configured CI graph: broad 692, audio 1, movie 1; the partitions are disjoint and equal the current per-directory inventory union.
- Exercised a one-off strict JUnit union procedure with a complete 694-name positive report set and missing, duplicate, extra, wrong-shard, failure, and skipped negatives. The positive case passed and all six negatives were rejected.
- Reviewed the latest full Main CI observation, fixture and hash ownership, repeated setup, and required CI DAG. No comparable hosted candidate run exists, so the BE32 gate and `hashes-be32` route remain unchanged and TODO-052 stays pending.

## Task Commits

1. **Task 1: Prove the current three-way BE32 selector and preservation contract** — `f7c856e` (`docs`)
2. **Task 2: Evaluate hosted critical-path evidence and retain only what is proven** — `cbd7ed4` (`docs`)

## Files Created/Modified

- [17-BE32-REVIEW.md](./17-BE32-REVIEW.md) — selector proof, candidate fixture/hash contract, latest hosted observation, and exact before/after acceptance criteria.
- [TODO-052](../../../todos/pending/052-be32-movie-test-shard.md) — current long-test timings, setup estimate, CI dependency path, and pending evidence.

## Decisions Made

- Preserve the monolithic production gate: selector correctness and synthetic report proof do not substitute for a hosted shard candidate and comparable critical-path evidence.
- Plan 17-06's conditional CI/CD routes do not select the full BE32 rewrite; if a later score selects it, handle the added shard scope and missing hosted proof separately.

## Deviations from Plan

None. The only validation used the existing configured graph, the requested disposable synthetic reports, and read-only hosted evidence; no source, test, workflow, timeout, or gate changed.

## Issues Encountered

The anonymous Actions job-log endpoint returned 403. The existing authenticated `gh` session retrieved the same read-only job log successfully with a task-local cache. The default sandbox also denied writing the required plan ledger under `.git`; automatic review allowed the two authorized commits on the phase branch. No blocker remains.

## User Setup Required

None.

## Next Phase Readiness

The exact local partition and preservation contract are reviewable. A future workflow change must provide the strict three-report JUnit union and a comparable hosted run before changing the production route. Phase orchestration owns state and roadmap updates.

## Self-Check: PASSED

All three declared output files exist, both task commits are ancestors of HEAD, and `git diff --check origin/main` passed.

---
*Phase: 17-a-measured-and-trustworthy-playability-baseline*
*Completed: 2026-11-28*
