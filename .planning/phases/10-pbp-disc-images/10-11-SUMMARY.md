---
phase: 10-pbp-disc-images
plan: "11"
subsystem: ci-routing
tags: [github-actions, nightly, pull-request-routing]
requires:
  - phase: 10-pbp-disc-images
    provides:
      - bounded PBP parser seam and existing changed-path classifier
provides:
  - conservative Nightly routing for exact PBP-only, docs/planning-only and full PR changes
  - fail-open workflow behavior when PR metadata or the classifier job is unavailable
affects: [ci, nightly, maintainer-docs]
actuals:
  tasks: 2
  commits: 2
plan_head_before: ca5eeb988888ae563784955d9a50bad54d8c595d
plan_head_after: 9d74ab060ca8f9eea1dd30815749cea4d1a84b33
commits: 2
key-files:
  created: []
  modified:
    - .github/workflows/nightly.yml
    - scripts/classify-changes.sh
    - tests/cmake/ci_route_controls.cmake
    - tests/cmake/workflows.cmake
    - docs/maintaining.md
    - .planning/todos/pending/051-nightly-pr-job-routing.md
key-decisions:
  - Unknown, incomplete, mixed, renamed and failed-classifier cases run full Nightly unless cancelled.
  - Only a complete exact PBP path set skips benches/vectors while retaining fuzz; docs/planning-only lists skip all Nightly jobs.
  - Request the REST endpoint's maximum 100 changed files per page to reduce sequential calls.
requirements-completed: []
coverage:
  - id: R1
    description: The classifier routes exact PBP, docs/planning-only and conservative fallback inputs while preserving existing CI outputs.
    requirement: PBP-01
    verification:
      - kind: unit
        ref: "sh -n scripts/classify-changes.sh && cmake -DROOT=\"$PWD\" -DBIN=build/planning-route-check -P tests/cmake/ci_route_controls.cmake; exit 0"
        status: pass
    human_judgment: false
  - id: R2
    description: Workflow conditions keep scheduled/manual coverage and run full Nightly if the changes job fails; PBP-only PRs retain fuzz.
    requirement: PBP-01
    verification:
      - kind: integration
        ref: "cmake -DCASE=disc_registration -DROOT=. -P tests/cmake/workflows.cmake; exit 0; actionlint with the repository's ubuntu-26.04 label diagnostic ignored; exit 0"
        status: pass
      - kind: integration
        ref: "act pull_request --dryrun -W .github/workflows/nightly.yml -P ubuntu-26.04=catthehacker/ubuntu:act-latest -P ubuntu-26.04-arm=-self-hosted -P macos-15=-self-hosted; exit 0"
        status: pass
    human_judgment: false
  - id: R3
    description: The changes remain under the fixed repository gate and the registered classifier control executes.
    requirement: PBP-01
    verification:
      - kind: unit
        ref: "ctest --preset ci --no-tests=error -R '^ci[.]classify_policy$' --output-on-failure; exit 0, 1/1"
        status: pass
      - kind: other
        ref: "cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake; exit 0, 3900/3901"
        status: pass
    human_judgment: false
duration: "2h05m wall interval from the previous STATE checkpoint; active time not measured"
completed: 2026-11-21
status: complete
---

# Phase 10 Plan 11: Nightly PR Routing Summary

**Nightly now keeps full coverage for uncertain PR changes, while exact PBP-only and docs/planning-only changes avoid workloads they cannot affect.**

## Performance

- **Runtime/model:** Codex session; primary model ID is not exposed. A focused read-only code review and three planning/efficiency audits were used.
- **Elapsed wall interval:** About 2h05m from STATE's 07:26Z checkpoint through 09:31Z closeout; active time was not measured.
- **Tasks and commits:** Two tasks, committed as `bc4e922` and `9d74ab0`.
- **Observed bottlenecks:** The workflow test first exposed an assertion-order mistake, which was fixed before closeout. Review found that a failed `changes` dependency would skip Nightly; all dependent jobs now route full on classifier failure while respecting cancellation. PR files are requested 100 per page. No comparable hosted run exists yet, so no CI savings are claimed.

## Accomplishments

- Extended the existing classifier without changing `heavy`, `release_inputs`, or release-metadata behavior. Exact PBP-only lists retain fuzz and skip bench/vector jobs; complete docs/planning-only lists skip Nightly; every mixed, unknown, incomplete, count-mismatched or renamed list routes full.
- Marked a rename explicitly while retaining both names, so renaming between two allowlisted PBP paths cannot take the narrow route.
- Added a read-only PR file-listing job with 100 files per page. API or parse errors retain full defaults; a failed or timed-out changes job makes dependent Nightly jobs run full unless the workflow was cancelled.
- Preserved full schedule and manual workloads, existing campaign bounds, report behavior and permissions. PRs do not receive issue-writing permission.
- Documented routing and the limits of `act --dryrun`; kept TODO 051 pending for the exact-head hosted check and cost measurement.
- The fixed repository gate measures 3,900/3,901 lines.

## Review and efficiency

The focused reviewer found and cleared two issues: dependency failures needed an explicit full-work fallback, and the REST listing could reduce sequential API requests by using its supported 100-file page size. Re-review found no remaining issue. The route controls cover every former Nightly trigger pattern, non-PBP fuzz and disc paths, both rename names, a rename marker, mixed inputs, count mismatch, unknown paths, and the existing classifier cases.

`act pull_request --dryrun` exited 0 and checked the PR changes-job route. `act schedule --dryrun` also exited 0. An additional `act workflow_dispatch --dryrun` attempt unexpectedly entered local workflow steps and failed when the artifact action lacked `ACTIONS_RUNTIME_TOKEN`; it is not a hosted result, and it was not repeated. The final Phase 10 plan owns one exact-head hosted CI/Nightly run and the critical-path/raw-runner-time review. That mixed code/workflow head must take full Nightly, so no narrow-route speedup will be claimed from it.

## Verification

- `sh -n scripts/classify-changes.sh && cmake -DROOT="$PWD" -DBIN=build/planning-route-check -P tests/cmake/ci_route_controls.cmake`: exit 0.
- `ctest --preset ci --no-tests=error -R '^ci[.]classify_policy$' --output-on-failure`: exit 0, 1/1 in 0.55s.
- `cmake -DCASE=disc_registration -DROOT=. -P tests/cmake/workflows.cmake`: exit 0.
- `cmake -DMODE=pins -DDIR=.github/workflows -P tests/cmake/workflows.cmake && cmake -DMODE=timeouts -DDIR=.github/workflows -P tests/cmake/workflows.cmake`: exit 0; 61 action uses and 26 timeout comments checked.
- `actionlint -ignore 'label "ubuntu-26[.]04(-arm)?" is unknown' .github/workflows/nightly.yml`: exit 0. The ignored messages are actionlint's stale built-in runner-label list; workflow expressions and syntax otherwise pass.
- `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake`: exit 0, 3,900/3,901.
- `act pull_request --dryrun -W .github/workflows/nightly.yml -P ubuntu-26.04=catthehacker/ubuntu:act-latest -P ubuntu-26.04-arm=-self-hosted -P macos-15=-self-hosted`: exit 0.
- `act schedule --dryrun -W .github/workflows/nightly.yml --action-cache-path /private/tmp/schway-act-schedule -P ubuntu-26.04=catthehacker/ubuntu:act-latest -P ubuntu-26.04-arm=-self-hosted -P macos-15=-self-hosted`: exit 0.
- `act workflow_dispatch --dryrun -W .github/workflows/nightly.yml --action-cache-path /private/tmp/schway-act-dispatch -P ubuntu-26.04=catthehacker/ubuntu:act-latest -P ubuntu-26.04-arm=-self-hosted -P macos-15=-self-hosted`: exit 1 at local artifact upload because `ACTIONS_RUNTIME_TOKEN` is unavailable; not rerun.
- `git diff --check origin/main...HEAD && git diff --check HEAD`: exit 0.
- No hosted run URL applies yet; Plan 10-13 verifies the exact changed head. TODO 051 remains pending until then.

## Next Plan Readiness

Plan 10-03 is next. It exercises bounded raw inflate through the private wrapper, registers the focused CTest, and checks CHD-OFF and CHD-ON links. Phase-wide PBP-01 remains pending.
