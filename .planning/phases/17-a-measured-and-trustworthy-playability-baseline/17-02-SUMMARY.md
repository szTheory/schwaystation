---
phase: 17-a-measured-and-trustworthy-playability-baseline
plan: "02"
subsystem: ci-reliability
tags:
  - github-actions
  - ci
  - timeouts
requires:
  - phase: 17-01
    provides: Run collection bounds and source audit
provides:
  - Recorded exact-attempt Actions cohort and workload matching dispositions
  - Classified current failure evidence and timeout observations
  - Audited Windows DLL consumer and runner/checkpoint repetition evidence
affects:
  - Phase 17 CI scoring and improvement selection
  - Phase 17 BE32 and timeout review
actuals:
  tokens: 9687
  tasks: 2
  commits: 2
tech-stack:
  added: []
  patterns:
    - Historical timings are grouped only when the event, route, selected workload inputs, and runner match.
key-files:
  created:
    - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-CI-EFFICIENCY.md
    - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-RELIABILITY.md
  modified:
    - .planning/todos/pending/022-windows-dll-crt-consumer-flake.md
    - .planning/todos/completed/049-review-runner-checkpoint-repeats.md
key-decisions:
  - "Keep all workflow and timeout values unchanged; the cohort has no repeated full-CI or Nightly workload fingerprint."
  - "Keep TODO-022 pending because the latest ten Windows jobs contain only seven clean DLL consumer results."
  - "Complete TODO-049 as an audit because every repeated output serves a separate assertion or consumer."
patterns-established:
  - "Report exact source job URLs and distinguish runner occupancy from CPU, JUnit occupancy, and billing."
requirements-completed: []
coverage:
  - id: P17-02-A
    description: "A bounded exact-attempt cohort records matching groups, critical-path evidence, and available timing limits."
    requirement: TUNE-07
    verification:
      kind: other
      ref: "17-CI-EFFICIENCY.md; exact 40-run API query (exit 0, 40); 40/40 job payload validator (exit 0); git diff --check origin/main (exit 0)"
      status: pass
    human_judgment: false
  - id: P17-02-B
    description: "Timeout, failure, Windows DLL, and repeat-work decisions are traceable to exact jobs and leave unsupported changes deferred."
    requirement: TUNE-08
    verification:
      kind: other
      ref: "17-RELIABILITY.md; four failed-run logs and ten Windows job logs (exit 0); exact Plan 2 Task 2 check (exit 0)"
      status: pass
    human_judgment: false
duration: 45m
completed: 2026-11-27
status: complete
---

# Phase 17 Plan 02 Summary

**Captured a bounded Actions baseline and left unmatched timing, timeout, and flake decisions unchanged.**

## Performance

- **Duration:** About 45 minutes, including the complete 40-run API collection and job-log review.
- **Started:** 2026-11-28T01:33:00Z (approximate; task collection files were first written at this time).
- **Completed:** 2026-11-28T02:18:10Z.
- **Runtime/model:** Codex/OpenGSD 1.23.0; primary model ID unavailable.
- **Tasks:** 2.
- **Task commits:** 2.

## Accomplishments

- Collected all 40 latest workflow run identities and all 556 exact-attempt job records. Every attempt was 1, every job payload matched its run metadata, and the report separates PR events without merge SHAs from matching workload groups.
- Recorded one exact repeated Release metadata job group and the single full-release critical path. The full-release BE32 duration is retained as a separate observation; no timeout or speedup change is inferred from it.
- Classified all four failed runs. The Windows DLL consumer passed in seven of the latest ten Windows-x64 job records, leaving TODO-022 pending with a three-pass shortfall.
- Completed TODO-049 after finding no safe repeated-output reuse. TODO-051 remains unchanged.

## Task Commits

1. **Task 1: Collect and compare the bounded Actions cohort** - `23b1f6f` (`docs(17-02): capture CI cohort efficiency baseline`).
2. **Task 2: Derive reliability dispositions and audit repeat work** - `769f244` (`docs(17-02): record CI reliability and repeat audit`).

## Verification

- The GitHub Actions run-list API query with per_page=40 returned 40; exit 0.
- The required git diff check passed; exit 0. The required nonempty reliability-report check passed; exit 0.
- The 40/40 payload validator confirmed 556 unique job records, exact run/attempt/source-head identities, and no attempt above 1 — exit 0.
- Logs for the four unsuccessful runs and the latest ten Windows-x64 job records were fetched read-only. Seven logs showed the named consumer passing; three did not provide a clean result.
- No builds or tests were run while authoring this plan. TODO-051 was unchanged.

## Remaining evidence

TODO-022 remains pending until its original silent exit cause is explained or ten clean comparable Windows DLL consumer runs are available. Phase requirements TUNE-07 and TUNE-08 remain pending until the full tune-up and phase verification complete.

## Cleanup

All collection JSON, job logs, analysis scratch, and the isolated GitHub CLI cache were kept under `/private/tmp/schwaystation-plan17-02`; task-owned temporary data will be removed before the next plan.

---
*Phase: 17-a-measured-and-trustworthy-playability-baseline*
*Plan: 02*
*Completed: 2026-11-27*
