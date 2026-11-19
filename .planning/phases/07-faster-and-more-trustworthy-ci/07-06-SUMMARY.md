---
phase: 07-faster-and-more-trustworthy-ci
plan: "06"
subsystem: ci
tags: [github-actions, release-please, workflow-routing, timeout-evidence]

requires:
  - phase: "07-01"
    provides:
      - registered CI classifier and route-control checks
      - 3,300-line repository-check budget
  - phase: "07-02"
    provides:
      - exact-workload timeout evidence and linked run IDs
  - phase: "07-05"
    provides:
      - timeout-comment and action-pin gates
provides:
  - fail-closed fast route for exact release-please App metadata PRs
  - workflow timeouts derived from linked measured runs
affects:
  - Phase 7 ship verification
  - CI maintenance

actuals:
  tokens: 6010
  tasks: 2
  commits: 3
commits: 3
plan_head_before: 8f1dfce6c2dbd0527f3003281d42f36a96df6995
plan_head_after: 0c9c2783b4596e4c34bb4a334d0a3b420daa7c3e

tech-stack:
  added: []
  patterns:
    - Fresh REST identity and paginated file metadata must agree before generated-only PRs skip heavy jobs
    - Sampled timeout allowances retain setup floors and link the extremal run
key-files:
  created: []
  modified:
    - tests/cmake/ci_route_controls.cmake
    - scripts/classify-changes.sh
    - .github/workflows/ci.yml
    - .github/workflows/nightly.yml
    - .github/workflows/release.yml
    - tests/hygiene/tests.cmake
key-decisions:
  - Compare the literal release App bot login from the PR event against fresh REST metadata and require stable, complete paginated file identity.
  - Keep unsampled job timeout values provisional with their existing setup floors, and leave TUNE-02/03 pending for the post-change hosted release PR evidence.
patterns-established:
  - Route only exact generated metadata PRs through the fast path; stale, incomplete, malformed, or unexpected metadata stays heavy.
requirements-completed: []
coverage:
  - id: D1
    description: Exact release App metadata PR classification is covered by fail-closed route controls.
    requirement: TUNE-02
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^ci[.]classify_policy$' --output-on-failure"
        status: pass
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^ci[.]classify$' --output-on-failure"
        status: pass
      - kind: integration
        ref: "ctest --preset hygiene --no-tests=error -R '^(ci[.]classify|policy[.]gate_budget)$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: Sampled timeout changes and provisional unsampled values pass the timeout-comment and workflow-pin controls.
    requirement: TUNE-03
    verification:
      - kind: integration
        ref: "ctest --preset hygiene --no-tests=error -R '^(hygiene[.]timeout_comments([.]self_test)?|policy[.]action_pins|policy[.]gate_budget)$' --output-on-failure"
        status: pass
      - kind: other
        ref: "XDG_CACHE_HOME=/private/tmp/schway-act-cache act workflow_dispatch --dryrun -W .github/workflows/ci.yml -j changes -P ubuntu-26.04=catthehacker/ubuntu:act-latest --container-architecture linux/amd64"
        status: pass
    human_judgment: false

duration: 40min
completed: 2026-11-18
status: complete
---

# Phase 7 Plan 6: Exact release metadata routing and measured workflow timeouts

**The exact release-please App metadata PR takes a fail-closed fast route, while workflow timeouts follow linked run evidence.**

## Performance

- **Duration:** 40 min
- **Started:** 2026-11-18T22:16:08Z
- **Completed:** 2026-11-18T22:55:41Z
- **Tasks:** 2
- **Files modified:** 6

## Accomplishments

- Added exact release App identity, same-repository refs and SHAs, stable paginated file metadata, and exact three-file set checks before the PR fast route can skip heavy jobs. Incomplete or untrusted API data stays on the heavy route.
- Preserved ordinary, fork, release-input, planning-only, main-push, and required-check routing; source Release validation remains unchanged.
- Applied evidence-linked timeout values to sampled CI, Release, and Nightly jobs. Unsampled values remain provisional with setup floors; no test timeout or bounded case changed.
- Kept the repository-check gate at 3,299/3,300 lines.

## TDD RED/GREEN Evidence

- **RED:** The new production-block route controls failed before the route implementation because the exact qualifying metadata PR was still classified heavy. The evidence validator accepted the focused failure as RED; its record is in ignored `build/ci` output.
- **GREEN:** The exact-set route implementation passed the focused classifier and production-block controls. No refactor commit was needed.

## Checks

- `ctest --preset ci --no-tests=error -R '^ci[.]classify_policy$' --output-on-failure` — exit 0; 1/1 passed.
- `ctest --preset ci --no-tests=error -R '^ci[.]classify$' --output-on-failure` — exit 0; 1/1 passed.
- `ctest --preset hygiene --no-tests=error -R '^(ci[.]classify|hygiene[.]timeout_comments([.]self_test)?|policy[.]action_pins|policy[.]gate_budget)$' --output-on-failure` — exit 0; 5/5 passed.
- `cmake -DROOT="$PWD" -DLIMIT=3300 '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -P tests/cmake/line_budget.cmake` — exit 0; 3,299/3,300 counted lines.
- The shell syntax check for scripts/classify-changes.sh, YAML parsing of the changed workflow files, and `git diff --check` — exit 0.
- `XDG_CACHE_HOME=/private/tmp/schway-act-cache act workflow_dispatch --dryrun -W .github/workflows/ci.yml -j changes -P ubuntu-26.04=catthehacker/ubuntu:act-latest --container-architecture linux/amd64` — exit 0; dry-run only, no actions or runners executed.
- No hosted CI was started. The post-change automatic App PR route, required-check parity, and elapsed comparison against release PR #23 are intentionally reserved for ship. The baseline metadata PR is [run 44253419074](https://github.com/szTheory/schwaystation/actions/runs/44253419074); no speedup is claimed here.
- No same-workload flaky test was confirmed in Plan 02; no flake fix or weakened signal was needed.

## Task Commits

1. **Task 1 RED controls** — `c90ca5e` (`test(07-06): add release metadata route controls`)
2. **Task 1 GREEN route** — `063301f` (`feat(07-06): fast-route exact release metadata PRs`)
3. **Task 2 timeouts** — `0c9c278` (`ci(07-06): derive workflow timeouts from measured runs`)

## Files Created/Modified

- `scripts/classify-changes.sh` — fail-closed classification of the exact generated metadata PR set.
- `.github/workflows/ci.yml` — fresh PR metadata validation and measured CI timeout values.
- `.github/workflows/nightly.yml` — evidence-linked sampled timeouts and provisional unsampled values.
- `.github/workflows/release.yml` — evidence-linked sampled timeouts; Release validation is unchanged.
- `tests/hygiene/tests.cmake` — table-driven route controls and production marked-block harness coverage.
- `tests/cmake/ci_route_controls.cmake` — normal-CI classifier route controls.

## Decisions Made

- The release App login is compared as the literal `schwaystation-release[bot]` representation recorded in the pull request event and confirmed by PR #23 REST metadata; the bot type and fresh REST identity must also match.
- Timeouts use the linked Plan 02 exact-workload samples with setup floors retained. Zero-sample jobs remain provisional and preserve their prior setup-aware values.
- TUNE-02 and TUNE-03 remain phase-level pending until the real post-change App release PR supplies hosted route, required-check, and elapsed evidence.

## Deviations from Plan

None — the implementation followed the plan. Hosted comparison was not run because the plan reserves that evidence for ship.

## Issues Encountered

- The repository write sandbox denied direct git index writes during task commits; the parent completed the scoped commits through the approved repository-write path. All three task commits are present and verified.

## Known Stubs

None.

## User Setup Required

None — no external service configuration is required for these local checks.

## Next Phase Readiness

- Plan 07-06 implementation and local verification are complete. TUNE-02 and TUNE-03 remain Pending in `REQUIREMENTS.md` pending the phase/ship hosted release PR comparison; no owner action is needed now. Plan 07-07 is next.
- The next phase work may proceed. At ship, compare the next automatic App PR with run 44253419074, check required-check parity and record the hosted run URL.

## Self-Check: PASSED

- Summary exists and passes `gsd-tools verify-summary` with no missing file or commit claims.
- Representative created and modified files exist; RED, GREEN, and timeout commits are ancestors of the current branch head.
- Plan commit count is measured from the persisted ledger: 3 implementation commits from `8f1dfce6c2dbd0527f3003281d42f36a96df6995` through `0c9c2783b4596e4c34bb4a334d0a3b420daa7c3e`.

---
*Phase: 07-faster-and-more-trustworthy-ci*
*Completed: 2026-11-18*
