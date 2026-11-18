---
phase: quick-261118-bvm
plan: "01"
subsystem: ci
tags: [github-actions, cmake, ctest, shell]
requires: []
provides:
  - Safe heavy-job classification for complete planning-only direct pushes to main
  - Local integration coverage for the actual CI classifier shell
affects: [CI, hygiene, testing-docs]
actuals:
  tokens: 3986
  tasks: 2
  commits: 3
plan_head_before: 01c42a87b6ec47ef4a2711dcffb7eb2c07ff084d
plan_head_after: 11c9789a5e13c7851acf5030bcb28710f6cf90f8
tech-stack:
  added: []
  patterns:
    - Extract the marked workflow shell into the existing hygiene test during CMake configuration
    - Fail closed to the heavy matrix for incomplete or uncertain push diffs
key-files:
  created:
    - .planning/quick/261118-bvm-skip-heavyweight-main-push-ci-for-planni/261118-bvm-SUMMARY.md
  modified:
    - .github/workflows/ci.yml
    - tests/hygiene/tests.cmake
    - docs/testing.md
    - .planning/quick/261118-bvm-skip-heavyweight-main-push-ci-for-planni/261118-bvm-PLAN.md
    - .planning/STATE.md
key-decisions:
  - Validate the exact push endpoints and diff both trees with rename detection disabled and Git path quoting enabled.
  - Keep PR behavior intact and route manual runs, release calls, and uncertain inputs to the full matrix.
  - Reuse the existing classifier and hygiene test without a new dependency or counted gate file.
requirements-completed:
  - FRAME-05
  - FRAME-06
coverage:
  - id: D1
    description: Complete planning-only main pushes can skip heavy jobs while required hygiene and roll-up jobs remain.
    requirement: FRAME-05
    verification:
      - kind: integration
        ref: "ctest --test-dir build/hygiene -R '^ci[.]classify$' --no-tests=error --output-on-failure"
        status: pass
      - kind: other
        ref: "cmake --preset hygiene && ctest --test-dir build/hygiene -R '^ci[.]classify$|^policy[.]gate_budget$' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: PR classification stays intact and release calls, manual runs, release inputs, and uncertain diffs retain full coverage.
    requirement: FRAME-06
    verification:
      - kind: integration
        ref: "ci.classify local Git fixtures: PR, release-call, manual, release-input, rename, malformed/fetch/diff failures"
        status: pass
      - kind: other
        ref: "actionlint exact project command; exit 0"
        status: pass
    human_judgment: false
duration: 33min
completed: 2026-11-18
status: complete
runtime-model: Codex / OpenGSD 1.21.0; primary GPT-6 submodel not exposed
---

# Quick Task 261118-bvm: CI push classifier summary

**Main-push CI now skips the heavy matrix only for a verified, complete, non-empty planning-only diff; uncertain inputs keep full coverage.**

## Performance

- **Duration:** about 33 minutes
- **Started:** 2026-11-18T13:44:14Z
- **Completed:** 2026-11-18 (local verification finished before the pending hosted PR run)
- **Tasks:** 2
- **Implementation files modified:** 3
- **Runtime/model:** Codex / OpenGSD 1.21.0; primary GPT-6 submodel not exposed

## Accomplishments

- Direct pushes to `main` use the exact `before` and `after` commit IDs. The router verifies both IDs, confirms the checkout matches `after`, fetches the exact `before` commit when needed, and compares complete trees with rename detection disabled and path quoting enabled.
- Any missing, malformed, zero, unavailable, mismatched, empty, failed, or oversized comparison selects `heavy=true` and `release_inputs=true`. Normal PR classification stays on its API path; manual runs and workflow calls remain conservative.
- A failed PR file-list API request now marks the release-input list unknown as well, so the release rehearsal cannot be skipped on incomplete evidence.
- The existing `ci.classify` test extracts and runs the actual workflow shell against local Git fixtures. It covers planning-only pushes, source/release changes, both rename directions, unusual names, fetch/diff errors, invalid endpoints, overflow, PRs, release calls and manual dispatch.
- Testing docs now describe the focused CTest and gate-budget preflight, workflow lint, hygiene workflow, and optional scoped `act` dry run.

## Task Commits

1. **Task 1 TDD red:** `71be4ef` — extract and exercise the actual workflow classifier shell; the planning-only main-push case failed as expected before the router change.
2. **Task 1 green:** `a8553f4` — implement fail-closed direct-push classification and complete the local route controls.
3. **Task 2:** `6f60661` — document the classifier and cheap feedback path.
4. **Review fix:** `11c9789` — keep release rehearsal enabled when the PR file-list API fails.

The four task commits were measured from `plan_head_before` to `plan_head_after`. The attempt to persist the optional GSD commit ledger under `.git` was denied by the filesystem sandbox, so the recorded count uses the captured start SHA.

## Files Created/Modified

- `.github/workflows/ci.yml` — routes only direct main pushes through the complete endpoint diff check.
- `tests/hygiene/tests.cmake` — executes the marked workflow shell with local Git remotes and failure controls.
- `docs/testing.md` — documents routing and local checks.
- `.planning/quick/261118-bvm-skip-heavyweight-main-push-ci-for-planni/261118-bvm-PLAN.md` — replaced two absolute GSD reference paths with a generic workflow/template description after the hygiene tree scan found a home-specific path.
- `.planning/STATE.md` — appended the quick-task row through the GSD helper and updated Last activity.

## Verification

| Command | Result |
|---|---|
| `git diff --check` | Exit 0 |
| `cmake --preset hygiene` | Exit 0 |
| `ctest --test-dir build/hygiene -R '^ci[.]classify$|^policy[.]gate_budget$' --no-tests=error --output-on-failure` | Exit 0; both tests passed |
| `actionlint -shellcheck= -pyflakes= -ignore 'label "ubuntu-26\.04(-arm)?" is unknown' -ignore 'unexpected key "queue" for "concurrency" section' .github/workflows/ci.yml .github/workflows/release.yml .github/workflows/nightly.yml` | Exit 0 |
| `cmake --preset hygiene` then `ctest --test-dir build/hygiene -R '^ci[.]classify$|^policy[.]gate_budget$|^hygiene[.]tree$' --no-tests=error --output-on-failure` | Exit 0; all three passed after fail-closed PR listing correction |
| `shellcheck -s bash build/hygiene/hygiene/classifier-step.sh && bash -n build/hygiene/hygiene/classifier-step.sh` | Exit 0 |
| `act --dryrun --container-architecture linux/arm64 --action-cache-path /private/tmp/schwaystation-act-cache -P ubuntu-26.04=catthehacker/ubuntu:act-latest -j changes -W .github/workflows/ci.yml` | Exit 0; workflow wiring parsed only |
| `cmake --workflow --preset hygiene` | Exit 8; build passed and 22/23 tests passed. `hygiene.tree` found an absolute home path in the untracked quick plan artifact. The artifact was sanitized, then the targeted tree test passed. |
| `ctest --test-dir build/hygiene -R '^hygiene[.]tree$' --no-tests=error --output-on-failure` | Exit 0 after plan sanitation |
| `cmake --preset hygiene` then `ctest --test-dir build/hygiene -R '^ci[.]classify$|^policy[.]gate_budget$|^hygiene[.]tree$' --no-tests=error --output-on-failure` | Exit 0; all three passed after the fail-closed PR listing correction |
| `ctest --test-dir build/hygiene -R '^ci[.]classify$|^policy[.]gate_budget$' --no-tests=error --output-on-failure` | Exit 0 after final fixture-path quoting and PR API failure control |
| `act --dryrun --container-architecture linux/arm64 --action-cache-path /private/tmp/schwaystation-act-cache -P ubuntu-26.04=catthehacker/ubuntu:act-latest -j changes -W .github/workflows/ci.yml` | Exit 0 after the final workflow edit; structure only |

No hosted run URL exists yet: the orchestrator owns opening the PR and confirming the required PR, `main`, and release checks. The `act` dry run does not execute the tests or substitute for hosted matrix, artifact, or RetroArch evidence.

## Decisions Made

The existing path classifier remains the single authority for whether a diff is planning-only. The workflow supplies it an exact, complete, bounded list; every uncertainty takes the established conservative route. No package, new counted check file, trigger-level filter, or broad local emulator matrix was added.

## Deviations from Plan

- Parent-directed hygiene repair: the untracked plan artifact contained machine-specific GSD paths, so those references were generalized. The subsequent targeted hygiene tree check passed. No implementation scope changed.
- The optional `.git`-directory plan ledger could not be written under the sandbox. Commit count and before/after heads were measured from the captured `origin/main` start SHA and Git history instead.

## Issues Encountered

- Initial local fixture runs exposed a missing parent directory for a newline-path case and shallow-clone setup that needed to fetch each checkout target. Those fixture issues were corrected before the final focused test.
- Full hygiene's only failure was the untracked plan's personal path. After sanitizing it, the targeted `hygiene.tree` rerun passed; the full hygiene workflow was not repeated.
- Review found that an unavailable PR file-list response had set `heavy=true` but left `release_inputs=false`; the route now sets both outputs conservatively. Its local fixture passes.

## Next Step

The implementation is ready for the orchestrator to open the planned Conventional Commit CI PR, arm squash auto-merge, and verify the hosted PR, `main`, and release runs.

---
*Quick task: 261118-bvm*
*Completed: 2026-11-18*

## Self-Check: PASSED

- Summary file exists and contains the required requirements and coverage records.
- All three task commits are ancestors of `HEAD`; measured plan commit count is 3.
- `git diff --check` passes, and the quick-plan directory contains no home-directory paths.
