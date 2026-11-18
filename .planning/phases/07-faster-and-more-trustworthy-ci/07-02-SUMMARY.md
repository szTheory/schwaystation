---
phase: 07-faster-and-more-trustworthy-ci
plan: "02"
subsystem: infra
tags: [github-actions, ci, workload-fingerprints, timeouts]
requires: []
provides:
  - Bounded exact-head CI history and critical-path baseline
  - Evidence-linked timeout formulas, failure classifications and flake disposition
affects: [ci-routing, measured-quality, release-efficiency]
tech-stack:
  added: []
  patterns: [exact historical workload fingerprints, setup-floor timeout derivation]
key-files:
  created:
    - .planning/phases/07-faster-and-more-trustworthy-ci/07-CI-EFFICIENCY.md
  modified: []
key-decisions:
  - Exclude pull-request timing rows when the exact merge revision cannot be proven; retain the release-App PR as a supporting metadata anchor only.
  - Include every CTest inventory and the called CI workflow in the fingerprint and Release DAG.
  - Keep single-sample timeout values provisional, and make no workflow or test changes in the evidence plan.
requirements-completed: []
coverage:
  - id: D1
    description: Captured up to 40 Actions runs and identified exact comparable workload groups with linked critical paths.
    requirement: TUNE-02
    verification:
      - kind: other
        ref: python3 build/phase07/analyze_history.py; python3 build/phase07/derive_metrics.py
        status: pass
    human_judgment: false
  - id: D2
    description: Derived timeout floors and two-times-slowest values for all configured jobs, with run and job evidence or an explicit zero-sample reason.
    requirement: TUNE-03
    verification:
      - kind: other
        ref: cmake -DREPORT=.planning/phases/07-faster-and-more-trustworthy-ci/07-CI-EFFICIENCY.md -P build/phase07/check-report.cmake
        status: pass
    human_judgment: false
  - id: D3
    description: Reviewed sampled failure logs, separated source and portability failures from setup and timeout observations, and found no same-workload flaky test.
    requirement: TUNE-03
    verification:
      - kind: other
        ref: gh run view --log-failed for the seven linked captured attempts in 07-CI-EFFICIENCY.md
        status: pass
    human_judgment: false
actuals:
  tokens: 10971
  tasks: 2
  commits: 2
commits: 2
plan_head_before: 5a1a7d3
plan_head_after: 0119cbb
duration: 55min
completed: 2026-11-18
status: complete
---

# Phase 07 Plan 02 — CI efficiency baseline

**Exact-head CI report across 40 run samples, with the reusable Release DAG and evidence-linked timeout formulas.**

## Performance

- **Duration:** 55 minutes wall elapsed; active time was not measured.
- **Started:** 2026-11-18T19:12:35Z (phase planning commit, immediately before Plan 07-02 preflight).
- **Completed:** 2026-11-18T20:07:41Z (summary and state finalized).
- **Tasks:** 2.
- **Files modified:** 4, including the report, summary, state and roadmap.
- **Runtime/model:** Codex/OpenGSD CLI 1.20.0; inherited GPT-6 session model (exact deployed ID unavailable). The delegated executor inherited the session model at xhigh effort.

## Accomplishments

- Captured 40 CI, Release and Nightly runs once, fetched all 40 job-attempt payloads once, and analyzed 573 job records. Exact-head filtering retained 25 completed attempts in 20 exact workload groups.
- Reconstructed the nested Release-to-CI critical path. The longest full Release observation is 80:47 on run 44262491280, with `release-ci / be32` at 73:23. The repeated full-CI group measured 37:29 and 54:53, with BE32 as its slowest job.
- Recorded formulas and evidence for all 24 timeout settings. Seven sampled failure or cancelled-run logs distinguish dependency/setup issues, test timeouts, source portability failures and cross-platform hash mismatches. No same-fingerprint test failed and passed on a repeat; no confirmed flake was found.
- The metadata-only release PR run 44253419074 remains a supporting 60:21 BE32 anchor because its execution merge SHA is unavailable. The actual after measurement and timeout application remain with later plans.

## Checks run

| Command | Exit | Result |
| --- | ---: | --- |
| Actions run-list request with `per_page=40` | 0 | Captured the bounded 40-run sample. |
| Paginated job request for each selected run attempt | 0 (40/40) | Retained all 573 job records. |
| python3 build/phase07/analyze_history.py | 0 | Fingerprinted exact local heads, including all test inventory and manifest files. |
| python3 build/phase07/derive_metrics.py | 0 | Recomputed exact-group DAG paths, Release reusable jobs and timeout arithmetic from retained payloads. |
| `XDG_CACHE_HOME=build/phase07/gh-cache gh run view <id> --log-failed` for the seven IDs in the report | 0 (7/7) | Classified captured failures without rerunning a workflow. |
| python3 build/phase07/write_report.py | 0 | Wrote the reproducible run, cohort, critical-path and timeout tables. |
| cmake -DREPORT=.planning/phases/07-faster-and-more-trustworthy-ci/07-CI-EFFICIENCY.md -P build/phase07/check-report.cmake | 0 | Validated 20 fingerprints, 20 linked critical paths and 24 timeout formulas; rejected a missing section and incorrect arithmetic. |

No workflow or test suite ran. The detailed data and one-shot scripts remain ignored under `build/phase07`; the committed report contains the reproducible measurements and linked run evidence.

## Task commits

1. **Tasks 1–2: capture comparison, timeout and flake evidence** — `6b7d50d` (`docs(07-02): capture bounded CI baselines`)
2. **Evidence correction: align provisional timeout guidance with Plan 06** — `0119cbb` (`docs(07-02): align timeout evidence with application plan`)

## Decisions and issues

- PR workflows expose the PR head without its executed merge-ref SHA. The report excludes all such PR attempts from exact workload timing rather than approximating with the current or nearby revision.
- The initial local projection omitted suite `inventory.txt` files; the cheap inventory check caught this, and fingerprints were recomputed from the already-retained 40-run capture with no extra GitHub requests.
- The first generic job mapping omitted reusable `release-ci` child jobs. The local DAG calculation now includes the called `ci.yml` dependencies; unmapped children make a path invalid.
- GitHub CLI's default run-log cache was outside the writable workspace. Setting `XDG_CACHE_HOME` to ignored phase scratch allowed the seven targeted log reads to complete without copying credentials or repeating workflow runs.
- TUNE-02 and TUNE-03 remain phase-level pending: the report has no before/after optimization yet, and the workflow timeout edits belong to Plan 06.

## Next phase readiness

Plan 07-03 is runnable and owns the 3,300-line budget update before later gate additions. It will measure quality inputs and audit settings, runner images and pins. No owner action is needed.

---
*Phase: 07-faster-and-more-trustworthy-ci*
*Completed: 2026-11-18*

## Self-Check: PASSED

The summary, report, state and roadmap exist; the two report commits are reachable; the report validator and both negative controls passed.
