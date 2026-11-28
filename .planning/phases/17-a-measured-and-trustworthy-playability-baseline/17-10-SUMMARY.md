---
phase: 17-a-measured-and-trustworthy-playability-baseline
plan: "10"
subsystem: testing-and-documentation
tags: [quality-baselines, WASI, CI-evidence]
requires:
  - phase: 17-a-measured-and-trustworthy-playability-baseline
    provides: existing quality registry, comparator controls, and Phase 17 measurement baseline
provides:
  - exact bounded linked-WASI byte output through the existing comparator
  - positive and negative evidence in the existing registered quality control
  - draft pull-request run identity and honest pending-measurement handoff
affects: [phase-17-verification, 17-11]
actuals:
  tokens: 5161
  tasks: 2
  commits: 3
tech-stack:
  added: []
  patterns: [report exact producer value after artifact and bound validation, preserve one registered quality control]
key-files:
  created:
    - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-MEASUREMENT.md
    - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-10-SUMMARY.md
  modified:
    - tests/cmake/quality.cmake
    - tests/cmake/quality_controls.cmake
key-decisions:
  - "Keep quality.json unchanged until the matching hosted producers finish and exact results are available."
  - "Keep the draft PR run untouched while active; record its title-gate failure and pending producer state for the orchestrator."
requirements-completed: []
coverage:
  - id: D1
    description: "The existing bounded linked-WASI comparator reports the exact validated byte count, and the registered control checks the actual 20-byte fixture."
    requirement: TUNE-10
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^policy[.]quality_metrics$'"
        status: pass
    human_judgment: false
  - id: D2
    description: "The reviewed draft PR has a matching pull_request CI run on its exact head; producer completion and the title gate failure are recorded as pending/open."
    requirement: TUNE-10
    verification:
      - kind: integration
        ref: "https://github.com/szTheory/schwaystation/actions/runs/46165917340"
        status: pass
    human_judgment: false
duration: 25min
completed: 2026-11-28
status: complete
commits: 3
plan_head_before: fb35303f760ac617b5991416832e2f15f5132e5d
plan_head_after: 48161b72944627550b9c1cf7e51354551240d34c
---

# Phase 17 Plan 10: Exact linked WASI size handoff summary

**The linked-WASI comparator now reports its exact bounded byte count through the existing CI control, and the reviewed draft PR has started the supported producer run.**

## Performance

- **Duration:** 26 minutes
- **Started:** 2026-11-28T16:19:39Z
  - **Completed:** 2026-11-28T16:45:08Z
- **Tasks:** 2
- **Files modified:** 4
- **Runtime/model:** Codex / OpenGSD executor, GPT-6 Luna xhigh

## Accomplishments

- Added exact successful measurement output after the existing WebAssembly magic, integer, and bound checks. The unbounded disposition and all artifact rejection paths remain unchanged.
- Added a positive control that reads the actual 20-byte fixture and requires the exact `20` value with a successful comparator exit. The existing missing, malformed, over-ceiling, and artifact/manual-measurement rejection controls remain registered through `policy.quality_metrics`.
- Created a draft PR to start the full pull-request producer early. The exact head, run, attempt, merge revision, and pending state are recorded in `17-MEASUREMENT.md`.
- Kept `quality.json`, test registration, and inventory files unchanged. The repository-check total remains 4,694 lines; the RetroArch test total remains 300 lines.

## Task Commits

1. **Task 1 RED: assert the bounded measurement is observable** — `fe549286` (`test(17-10)`)
2. **Task 1 GREEN: report validated linked-WASI bytes** — `558f207c` (`feat(17-10)`)
3. **Task 2: record draft PR run state** — `48161b72` (`docs(17-10)`)

**Plan metadata:** summary commit is recorded in the executor handoff. Task commit count is measured from the plan ledger and excludes this separate metadata commit.

## Files Created/Modified

- `tests/cmake/quality.cmake` — reports exact validated measurement after successful bound checks.
- `tests/cmake/quality_controls.cmake` — requires the 20-byte artifact value and keeps existing negative controls.
- `.planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-MEASUREMENT.md` — records TDD evidence, review, focused checks, and hosted run identity/status.
- `.planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-10-SUMMARY.md` — this plan summary.

## Verification Evidence

- **RED:** `ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^policy[.]quality_metrics$' --output-junit <task-temp>/red.xml` — exit 8 on the planned successful-output assertion. `gsd_run check tdd-red-evidence .gsd/tdd-evidence/17-10/red.json --raw` returned `RED_EVIDENCE_OK` for the executed target test.
- **GREEN:** `ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^policy[.]quality_metrics$'` — exit 0; 1/1 registered control passed.
- **Focused preflight and tracer re-run:** the plan's Task 1 `<automated>` command — exit 0. It passed the quality control and registry, 4,694-line repository-check cap, 300-line RetroArch cap, 61 pinned action references, 26 timeout comments, and `git diff --check origin/main`.
- **Scope:** `git diff --exit-code origin/main -- tests/policy/tests.cmake tests/policy/inventory.txt quality.json` — exit 0.
- **Act routing:** `act pull_request --dryrun` with a task-owned PR payload, temporary action cache, and runner-label mappings — exit 0. It checked the pull-request trigger and initial jobs; it did not simulate dynamic heavy-job selection or hosted services.
- **Review:** focused manual review found no active source or control findings.
- **Hosted run:** [PR #50](https://github.com/szTheory/schwaystation/pull/50) remains a draft on head `558f207c1229848cd3454243e0a0084ee4a042e3`. [CI run 46165917340](https://github.com/szTheory/schwaystation/actions/runs/46165917340) is attempt 1 for `pull_request`, with merge revision `716ba97760db83b881cd374b9f9f2e24255eca1d`. At the 16:40:41Z status query, the run was `in_progress`; WASI and several required lanes were still running, while `title` had failed. The exact WASI value and a successful comparable timing value are not claimed.

## Issues Encountered

The PR title was created as `feat(17): expose exact linked WASI module size`; the repository title gate requires a lowercase scope. The local title checker rejected that title (exit 1) and accepted `feat(phase17): expose exact linked WASI module size` (exit 0). The current hosted run is active, so PR metadata has not been edited and the run has not been canceled. The orchestrator should inspect the completed run before correcting the title; the current `title` failure means no successful timing claim is available from the run unless the required roll-up completes successfully.

The first line-budget check measured 4,704 lines, and the next measured 4,695. Ten blank lines were removed from the touched CMake control file as the plan directs; the unchanged 4,694-line limit then passed exactly, with all assertions preserved.

The measurement report and this summary were committed locally after the PR run started and were not pushed. The remote PR head remains `558f207c1229848cd3454243e0a0084ee4a042e3`; do not push the two local documentation commits while the run is active.

## Decisions Made

- No quality bound or fingerprint changed because exact hosted producer values are still pending.
- The PR run is handed to the orchestrator while pending. Plan 17-11 must wait for its completion and reconcile only the results the supported producers actually emit.

## Deviations from Plan

None in the comparator implementation. The PR title-gate failure is recorded above; editing the active PR would risk canceling the authorized measurement, so correction is deferred until the orchestrator observes the run complete.

## Next Phase Readiness

Plan 17-10's local implementation, review, and focused controls are complete. The orchestrator owns asynchronous monitoring of CI run 46165917340. Plan 17-11 is not ready until that run completes; do not start it or push the local documentation commits beforehand. No owner action is needed.

## Self-Check: PASSED

All listed source/report files exist, and RED `fe549286`, GREEN `558f207c`, and Task 2 `48161b72` are ancestors of HEAD. `git diff --check origin/main` exited 0; `summary-extract` parsed the required metadata.
