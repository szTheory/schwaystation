# Phase 17 exact producer handoff

## Linked WASI size output

The existing `quality.cmake` artifact path now prints the exact validated integer after its magic and bound checks. The registered `policy.quality_metrics` control requires the 20-byte Wasm fixture to report `20` while also exiting successfully. The unbounded message and rejection cases remain intact; `quality.json` is unchanged.

### Red/green evidence

- Red command: `ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^policy[.]quality_metrics$' --output-junit <task-temp>/red.xml` — exit 8. The registered target failed on the new positive assertion because the bounded comparator returned 0 without reporting `20`. `gsd_run check tdd-red-evidence .gsd/tdd-evidence/17-10/red.json --raw` returned `RED_EVIDENCE_OK`; the intended assertion failed, with no fixture or setup failure.
- Green command: `ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^policy[.]quality_metrics$'` — exit 0; the registered quality controls passed.
- Focused preflight command: the plan's Task 1 `<automated>` command — exit 0. It includes the artifact controls, registry check, repository-check budget (4,694 lines), RetroArch budget (300 lines), 61 workflow pins, 26 documented timeouts, and `git diff --check origin/main`.
- Inventory disposition: `tests/policy/tests.cmake` and `tests/policy/inventory.txt` are unchanged; the existing `policy.quality_metrics` registration remains the only test entry for these controls.

## Hosted measurement handoff

The exact linked module bytes and comparable pull-request timing are not yet available. The supported full CI producer will run on the reviewed draft pull request after Task 2. Record its PR URL, exact head, `pull_request` run ID/URL, merge SHA, attempt, and pending/completed state here before Plan 17-11 consumes it. Do not infer a new bound from a range pass or from an unfinished run. Keep all `quality.json` values unchanged until the exact producers finish.

### Review and routing

- Focused review of `tests/cmake/quality.cmake` and `tests/cmake/quality_controls.cmake` found no active findings. The value is read from the validated artifact size, reported only after the bound checks, and tested through the existing registered control. Existing malformed, missing, manual-measurement, and over-ceiling cases remain in place; `quality.json` and the test inventory are unchanged.
- `act pull_request --dryrun` with a task-owned event payload and action cache exited 0. It checked the pull-request trigger and initial workflow wiring. Dry-run did not evaluate the dynamic heavy-job output or establish GitHub-hosted runner, artifact-transfer, or permission behavior. No containers were created.

### Draft pull request and hosted run

- Draft PR: [#50](https://github.com/szTheory/schwaystation/pull/50), base `main`, branch `phase/17-a-measured-and-trustworthy-playability-baseline`.
- Reviewed PR head: `558f207c1229848cd3454243e0a0084ee4a042e3`.
- Pull-request merge revision at run start: `716ba97760db83b881cd374b9f9f2e24255eca1d`.
- Full CI run: [46165917340](https://github.com/szTheory/schwaystation/actions/runs/46165917340), event `pull_request`, attempt 1, exact head `558f207c1229848cd3454243e0a0084ee4a042e3`. Created and started at `2026-11-28T16:34:56Z`; the initial status was `queued` and it began immediately.
- Source checks: `gh pr view 50 --json isDraft,baseRefName,headRefName,headRefOid,url,mergeCommit`; `gh api repos/szTheory/schwaystation/pulls/50`; `gh run list --workflow ci.yml --event pull_request --branch phase/17-a-measured-and-trustworthy-playability-baseline --limit 10 --json databaseId,headSha,event,status,url`; `gh run view 46165917340 --json attempt,status,event,headSha,url,createdAt,startedAt,jobs`.
- Latest queried state at `2026-11-28T16:40:41Z`: the run is `in_progress`; `changes`, `hygiene`, `gpu-reference`, `openbios`, `perf`, and both `nofp` jobs succeeded. WASI, sanitizer, BE32, and multiple platform builds were still running. The `title` job failed.
- Title diagnosis: `sh scripts/check-title.sh 'feat(17): expose exact linked WASI module size' </dev/null` exited 1 because the checker requires a lowercase scope; `sh scripts/check-title.sh 'feat(phase17): expose exact linked WASI module size' </dev/null` exited 0. The hosted log is not yet available while the run is active. The title has not been edited, because editing PR metadata during this run can cancel the measurement.
- The WASI job has not finished, and the timing producer has not reported a result. No exact bytes, timing, successful full-CI claim, or bound change is recorded. No manual workflow was dispatched.
