---
phase: 07-faster-and-more-trustworthy-ci
plan: "08"
subsystem: ci
tags: [critical-path, GitHub Actions, jq, quality.json, POSIX-shell]
requires:
  - phase: "07-02"
    provides: Historical per-head workload fingerprints and CI timing contract
  - phase: "07-07"
    provides: Shared quality comparator and checked-in CI critical-path bound
provides:
  - Current-attempt CI critical-path measurement from complete Actions job timestamps
  - Workload fingerprint matching that excludes product source bytes and includes workflow/build inputs
  - Required-job enforcement for comparable full heavy pull-request CI runs
affects: [Phase 7 ship evidence]
actuals:
  tokens: 6600
  tasks: 2
  commits: 2
commits: 3
plan_head_before: 102558fe7fa07dd52c2ef54c5f97f3a6c32deab2
plan_head_after: 577ef3d95c543feb01adbbb947cd0db80700500d
range_commits: 3
tech-stack:
  added: []
  patterns:
    - Measure the CI dependency DAG from exact current-attempt job rows
    - Report changed workload fingerprints as unbaselined without applying the old bound
    - Extract and execute marked workflow shell blocks in the counted production-path controls
key-files:
  created:
    - scripts/ci-critical-path.sh
    - .planning/phases/07-faster-and-more-trustworthy-ci/07-08-SUMMARY.md
  modified:
    - .github/workflows/ci.yml
    - .github/workflows/release.yml
    - tests/cmake/quality_controls.cmake
    - .planning/ROADMAP.md
    - .planning/STATE.md
key-decisions:
  - A workload fingerprint change reports fresh measured seconds as unbaselined and skips the old ceiling comparison.
  - The in-progress CI required job is validated but excluded from the measured DAG because it has no completion timestamp until its own check finishes.
  - The matching hosted Phase 7 PR measurement remains a ship-time check; no hosted measurement is claimed here.
requirements-completed: []
coverage:
  - id: C1
    description: The timing helper validates complete exact-attempt rows, unique job identities, current commit SHA, runner labels, and timestamp-derived dependency paths.
    requirement: TUNE-02
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^(policy[.]quality_metrics|ci[.]classify_policy)$' --output-on-failure (exit 0, 2/2 passed)"
        status: pass
      - kind: other
        ref: "python3 build/phase07/analyze_history.py (exit 0; 40 runs and 573 job records replayed)"
        status: pass
    human_judgment: false
  - id: C2
    description: CI required checks all predecessors and compares only full heavy uncalled PR attempts; release, main, planning-only, and nested release routes keep their existing skip behavior.
    requirement: TUNE-05
    verification:
      - kind: integration
        ref: "policy.quality_metrics production roll-up controls: matching, over-bound, changed fingerprint, missing/malformed/stale data, duplicate ID, failed predecessor, and permitted skips (exit 0)"
        status: pass
      - kind: integration
        ref: "ctest --test-dir build/hygiene --no-tests=error -R '^(policy[.](action_pins|gate_budget)|hygiene[.]timeout_comments)$' --output-on-failure (exit 0, 3/3 passed)"
        status: pass
    human_judgment: false
  - id: C3
    description: Ship still needs a matching hosted Phase 7 PR timing run and the actual Ubuntu 26 WASI module size before the final quality bound is set.
    requirement: TUNE-05
    verification:
      - kind: other
        ref: "Hosted matching Phase 7 PR critical-path measurement and run URL (pending ship; no hosted run claimed)"
        status: unknown
      - kind: other
        ref: "Ubuntu 26 WASI Release module size and checked-in size.wasm bound (pending ship)"
        status: unknown
    human_judgment: false
metrics:
  duration: 58min
  completed: 2026-11-19
status: complete
---

# Phase 7 Plan 8: Enforce measured CI critical-path time Summary

**Full heavy pull requests now compare the measured CI dependency path only when the current attempt and workload match the checked-in baseline.**

## Performance

- **Duration:** about 58 minutes
- **Started:** 2026-11-19T00:17:25Z
- **Completed:** 2026-11-19T01:15:27Z
- **Tasks:** 2
- **Files modified:** 4 implementation and test files
- **Runtime/model:** Codex/OpenGSD CLI 1.20.0; executor gpt-6-luna at xhigh effort

## Accomplishments

- Added a POSIX-shell and jq helper that validates paginated current-attempt job data against the run ID, attempt, branch, checked-out SHA, complete job inventory, unique IDs and names, successful predecessor statuses, and actual runner labels. It calculates the longest serial path from UTC job timestamps across native builds, nofp, BE32, WASM32, hash comparison, RetroArch, perf, hygiene/title, and rehearsal cleanup.
- Fingerprinted the CI workflow, CMake/build inputs, test inventories and fixture manifests, pins, scripts, and actual runner labels. Product source bytes do not affect the fingerprint. Matching fingerprints reach the shared `quality.cmake` ceiling; changed fingerprints log measured seconds as unbaselined and do not apply the old bound.
- Wired the comparison into `CI required` for full heavy uncalled pull requests. The job checks out `github.sha` with a pinned checkout and no persisted credentials, requests the exact run attempt with Actions read permission, and retries incomplete pagination once after two seconds. The reusable release caller adds only `actions: read`.
- Kept the counted repository-check code at 3,890 of the temporary 3,900-line limit.

## Verification

- `cmake --preset ci` — exit 0.
- `ctest --preset ci --no-tests=error -R '^(policy[.]quality_metrics|ci[.]classify_policy)$' --output-on-failure` — exit 0; 2/2 passed, including production-path matching, over-bound, changed fingerprint, missing/malformed/stale data, duplicate IDs, failed predecessors, and allowed skips.
- `cmake --preset hygiene` — exit 0.
- `ctest --preset hygiene --no-tests=error -R '^(policy[.](action_pins|gate_budget)|hygiene[.]timeout_comments)$' --output-on-failure` — exit 0; 3/3 passed.
- `ctest --test-dir build/ci --no-tests=error -R '^policy[.]quality_metrics$' --output-on-failure` — exit 0; 1/1 passed after the duplicate-job control was added (24.30 seconds).
- `cmake -DROOT=$PWD '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3900 -P tests/cmake/line_budget.cmake` — exit 0; 3,890/3,900 lines.
- `sh -n scripts/ci-critical-path.sh` and `git diff --check` — exit 0.
- `python3 build/phase07/analyze_history.py` — exit 0; the retained projection replay produced 40 run rows and 573 job records.

## TDD Evidence

Commit `931e055` added the production-path controls before the helper implementation. During development, the focused test was observed failing while the helper was absent. No durable RED-evidence artifact, complete report, or exit-status record was retained, so this summary does not claim a classifier result or invent an exit status. Commit `577ef3d` implemented the helper and workflow path; the focused GREEN checks passed as recorded above.

## Hosted Measurement Pending

The final reviewed matching Phase 7 PR must run the new required timing step and provide the measured critical path and run URL. The Ubuntu 26 WASI Release lane must also produce the linked module size before `size.wasm` gets a bound. Both items remain pending for ship; no hosted measurement is claimed in this summary.

## Decisions and Deviations

- The current `CI required` row must be present and in progress, but its unfinished duration is excluded from the DAG calculation. Including it would measure a partial duration of the check that is performing the comparison.
- The test controls were kept under the existing 30-second focused-test timeout and 3,900-line repository-check limit. The source-only exclusion is checked alongside the matching case; the changed-pin case exercises the workload-input change behavior.
- Release, main, planning-only, and nested reusable-workflow paths retain the existing result roll-up and do not collect an incomparable product timing sample.

## Task Commits

- `931e055` — `test(07-08): cover measured CI critical path`
- `577ef3d` — `feat(07-08): enforce measured CI critical path`

The execution-start range from `102558f` to `577ef3d` contains three commits. `bf84ae2` (`feat(07-07): align measured quality gate budget`) is the prior Plan 07-07 closeout; only the two commits above are attributed to Plan 07-08.

## Self-Check: PASSED

- The summary file exists at `.planning/phases/07-faster-and-more-trustworthy-ci/07-08-SUMMARY.md`.
- Both Plan 07-08 commits are ancestors of `577ef3d`; the range count and prior Plan 07-07 closeout are identified above.
- The focused CI and hygiene selectors, shell syntax check, diff check, historical replay, and repository-check budget passed.
