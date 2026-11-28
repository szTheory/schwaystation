# Phase 17 exact producer handoff

## Linked WASI size output

The existing `quality.cmake` artifact path now prints the exact validated integer after its magic and bound checks. The registered `policy.quality_metrics` control requires the 20-byte Wasm fixture to report `20` while also exiting successfully. The unbounded message and rejection cases remain intact; `quality.json` is unchanged.

### Red/green evidence

- Red command: `ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^policy[.]quality_metrics$' --output-junit <task-temp>/red.xml` — exit 8. The registered target failed on the new positive assertion because the bounded comparator returned 0 without reporting `20`. `gsd_run check tdd-red-evidence .gsd/tdd-evidence/17-10/red.json --raw` returned `RED_EVIDENCE_OK`; the intended assertion failed, with no fixture or setup failure.
- Green command: `ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^policy[.]quality_metrics$'` — exit 0; the registered quality controls passed.
- Focused preflight command: the plan's Task 1 `<automated>` command — exit 0. It includes the artifact controls, registry check, repository-check budget (4,694 lines), RetroArch budget (300 lines), 61 workflow pins, 26 documented timeouts, and `git diff --check origin/main`.
- Inventory disposition: `tests/policy/tests.cmake` and `tests/policy/inventory.txt` are unchanged; the existing `policy.quality_metrics` registration remains the only test entry for these controls.

## Hosted measurement handoff

Both exact producers now completed on the authorized draft PR. Run 1 exposed the exact WASI size early; its title gate failed, which prevented the timing step from running. After that run finished, the PR title was corrected and one current-head full run completed successfully. No manual dispatch, merge, or release was used.

### Review and routing

- Focused review of `tests/cmake/quality.cmake` and `tests/cmake/quality_controls.cmake` found no active findings. The value is read from the validated artifact size, reported only after the bound checks, and tested through the existing registered control. Existing malformed, missing, manual-measurement, and over-ceiling cases remain in place; `quality.json` and the test inventory are unchanged.
- `act pull_request --dryrun` with a task-owned event payload and action cache exited 0. It checked the pull-request trigger and initial workflow wiring. Dry-run did not evaluate the dynamic heavy-job output or establish GitHub-hosted runner, artifact-transfer, or permission behavior. No containers were created.
- The PR title check initially failed on numeric scope `17`; `sh scripts/check-title.sh 'feat(17): expose exact linked WASI module size' </dev/null` exited 1, while the corrected `feat(phase17): expose exact linked WASI module size` exited 0. `gh pr edit 50 --title 'feat(phase17): expose exact linked WASI module size'` exited 0 after run 1 was terminal.

### Draft pull request and exact attempt identities

- Draft PR: [#50](https://github.com/szTheory/schwaystation/pull/50), base `main`, branch `phase/17-a-measured-and-trustworthy-playability-baseline`. Corrected title: `feat(phase17): expose exact linked WASI module size`.
- Both runs used PR head `558f207c1229848cd3454243e0a0084ee4a042e3`, base `7185b7ad9e6078070b8bbcecd9763fc0a497b894`, and pull-request merge revision `716ba97760db83b881cd374b9f9f2e24255eca1d`. The run API, PR API, and paginated job records agree on the head, event, and attempt identity.
- Run 1: [run 46165917340](https://github.com/szTheory/schwaystation/actions/runs/46165917340), attempt 1, event `pull_request`, created/started `2026-11-28T16:34:56Z`, completed `2026-11-28T17:20:24Z`, conclusion `failure`; exact PR head unchanged. Its WASI job passed; the title job failed because scope `17` is not an allowed lowercase scope. `CI required` failed its rollup and skipped `Compare current-attempt critical path`, so this attempt has no timing result. Job records: 22 total; 18 success, 2 failure, 2 skipped. The failed jobs were `title` and `CI required`; the skipped jobs were `rehearsal` and `cleanup`.
- Run 2: [run 46170139230](https://github.com/szTheory/schwaystation/actions/runs/46170139230), attempt 1, event `pull_request`, created/started `2026-11-28T17:22:33Z`, completed `2026-11-28T18:08:53Z`, conclusion `success`; exact PR head, base, and merge revision match run 1. Paginated Actions API returned all 22 jobs for attempt 1, each with the exact PR head. `title` and `CI required` passed; 20 jobs succeeded and only `rehearsal` and `cleanup` were skipped because release inputs were false.
- Source identity commands: `gh api repos/szTheory/schwaystation/actions/runs/46165917340`, `gh api --paginate --jq '.jobs[]' 'repos/szTheory/schwaystation/actions/runs/46165917340/jobs?per_page=100'`, `gh api repos/szTheory/schwaystation/pulls/50`, and the matching run/job commands for run 46170139230 all exited 0. Run 2's producer and required job are [WASI job 141056653305](https://github.com/szTheory/schwaystation/actions/runs/46170139230/job/141056653305) and [CI required job 141069205674](https://github.com/szTheory/schwaystation/actions/runs/46170139230/job/141069205674).

### Exact linked WASI bytes

The existing hosted producer configured `cmake --preset wasm32-chd`, built with `cmake --build --preset wasm32-chd`, ran `ctest --preset wasm32-chd --output-on-failure`, then ran the existing bounded artifact comparator. The preset is Release with tests and CHD enabled and the 256 KiB stack configuration. The job log identifies runner label `ubuntu-26.04`, Clang 21.1.8, `libclang-rt-21-dev-wasm32` 1:21.1.8-6ubuntu1, `wasi-libc` 0.0~git20250726.3f7eb4c-4, and configured `lld-21`. The job log does not expose its runner-image patch or an exact CMake version; no version is inferred here.

Exact artifact command from both WASI jobs: `cmake "-DQUALITY=${QUALITY_FILE:-quality.json}" -DMETRIC=size.wasm "-DARTIFACT=${WASM_MODULE:-build/wasm32/schwaystation-run}" -P tests/cmake/quality.cmake` — exit 0 in each job.

| Run / attempt | Job | Exact output | Result |
| --- | --- | --- | --- |
| [46165917340 / 1](https://github.com/szTheory/schwaystation/actions/runs/46165917340/job/141043516845) | `wasm32`, `ubuntu-26.04` | `quality: size.wasm: measured 1014665; within configured bounds` | Job exit 0; workflow later failed only the title rollup. |
| [46170139230 / 1](https://github.com/szTheory/schwaystation/actions/runs/46170139230/job/141056653305) | `wasm32`, `ubuntu-26.04` | `quality: size.wasm: measured 1014665; within configured bounds` | Job exit 0; successful exact-head PR workflow. |

These are two repeated exact outputs from the same current hosted producer configuration. The prior local value 1,014,617 bytes used CMake 4.2.3 in an x86_64 Ubuntu 26.04 task container; it remains a distinct local toolchain observation. The previous hosted result was 1,014,665 bytes, so the active hosted result is reproducible and supports the exact hosted ratchet. No browser-speed or device-performance result is inferred from module size or Wasmtime tests.

### Exact PR critical-path timing

Run 2's `changes` output recorded `heavy=true`, `release_inputs=false`; the `CI required` log recorded `CI_HEAVY=true`, `CI_CALLED=false`, `CI_RELEASE_INPUTS=false`, and PR head `558f207c1229848cd3454243e0a0084ee4a042e3`. This is the full, uncalled, non-main pull-request route consumed by the timing producer. Its exact-attempt runner labels were:

| Job group | Runner labels |
| --- | --- |
| `changes`, `title`, `hygiene`, `gpu-reference`, `openbios`, `perf`, `nofp (ubuntu-26.04)`, `be32`, `wasm32`, `hash-equality`, `CI required`, skipped `rehearsal` and `cleanup` | `ubuntu-26.04` |
| `nofp (ubuntu-26.04-arm)`, `build (linux-arm64)` | `ubuntu-26.04-arm` |
| `build (windows-arm64)` | `windows-11-arm` |
| `build (macos-arm64)`, `retroarch` | `macos-15` |
| `build (windows-x64)` | `windows-2025` |
| `build (macos-x64)` | `macos-15-intel` |

The exact producer command was `CI_JOBS_FILE="$jobs_file" scripts/ci-critical-path.sh`, from the successful [CI required job](https://github.com/szTheory/schwaystation/actions/runs/46170139230/job/141069205674). It exited 0 and printed:

```text
ci timing: measured=2761 s fingerprint=453c9901d914abd0a6a9b8410ece574ec75f0f835423bdb2662e76d03a3a14de
ci timing: unbaselined workload (recorded=316f86729c5129fc618ae0acf71014a5b18784d9e7a4f6c03ff067d0b4326ed8); comparison deferred until the fingerprint is refreshed
```

The exact source DAG and job records identify the critical chain: `changes` 8s → `openbios` 30s → `be32` 2,711s → `hash-equality` 12s = 2,761s. This is the producer's result, not a replacement timing calculation. The current interval before reconciliation was 2,249–3,293 seconds. The measured value is inside it; the old fingerprint represented a push/main route, so the script correctly reported the current PR route as unbaselined. Plan 17-11 updates the fingerprint to the exact current PR route and keeps the interval unchanged.

The full workflow wall intervals were 2,728s for run 1 and 2,780s for run 2. Run 1 used 8,281 aggregate runner-seconds (8,270 successful and 11 failed); run 2 used 7,507 successful runner-seconds. Across both attempts this is 15,777 successful runner-seconds plus 11 seconds from the two failed gates. These are unweighted runner-time totals, not billing estimates. The run API shows creation and workflow start at the same second; the first `changes` job began two seconds later. Per-job queued duration, CPU utilization, and billing-weighted cost are not exposed by these records.

Run 2 was a full workflow repeat required because run 1's title gate made `CI required` fail and skipped the exact timing producer. After the completed run, one title correction triggered one full current-head run. No additional full suite was run for bookkeeping. Within the successful route, the six platform builds cover distinct targets and OpenBIOS is supplied as a shared artifact; no other duplicate full-suite job was identified. The title correction and the two successful WASI measurements are recorded above; no bound was loosened and no workflow was manually dispatched.

## Plan 17-11 reconciliation and local checks

The linked WASI producer returned the same exact integer twice under the same logged ubuntu-26.04 wasm32-chd toolchain/configuration. `quality.json` now tightens `size.wasm` to floor=ceiling=1,014,665. The 1,014,617 local CMake 4.2.3 output remains documented separately because it is a distinct local toolchain observation. The exact CI result 2,761 seconds falls within the existing 2,249–3,293 interval; only the stored workload fingerprint changed, from the historical push/main value to the printed full, uncalled, non-main PR value. No speedup or timeout change is claimed.

| Command | Exit | Result |
| --- | ---: | --- |
| `cmake -DQUALITY=quality.json -DMETRIC=size.wasm -DMEASURED=1014665 -P tests/cmake/quality.cmake` | 0 | Exact hosted WASI result passes the refreshed point bound. |
| `cmake -DQUALITY=quality.json -DMETRIC=ci.critical_path_s -DMEASURED=2761 -P tests/cmake/quality.cmake` | 0 | Exact successful PR timing passes the retained interval. |
| `cmake -DQUALITY=quality.json -P tests/cmake/quality.cmake` | 0 | Registry schema and active bound definitions pass. |
| `cmake -DROOT=$PWD -DBIN=<task-temp>/quality -P tests/cmake/quality_controls.cmake` with the refreshed registry | 1 | Existing `checked_pr_baseline` fixture expected the current synthetic route to be unbaselined. It now matches the refreshed producer fingerprint and correctly rejects that fixture's 11-second measurement below the 2,249-second floor. No control source was changed because it is part of the workload fingerprint. |
| Same registered quality controls with a task-temporary copy of `quality.json` carrying the prior fingerprint `316f8672…` | 0 | Existing boundary, malformed-input, integer, artifact/state producer-wiring and timing-route controls pass. The temporary registry was restored and the checked-in fingerprint rechecked as `453c9901…`. |
| `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=4694 -P tests/cmake/line_budget.cmake` | 0 | 4,694 counted lines at the existing limit. |
| `cmake -DROOT=. '-DPATTERNS=tests/retroarch/*' -DLIMIT=300 -P tests/cmake/line_budget.cmake` | 0 | 300 counted lines at the existing limit. |
| `git diff --check origin/main` | 0 | No whitespace errors. |

The one failing quality-control assertion is now a stale test expectation exposed by the authorized fingerprint refresh, not a producer mismatch: the control still expects a full current route to be unbaselined. Editing that control would itself change a workload input and make this hosted timing observation no longer match the checked-in fingerprint without another full hosted measurement. The direct exact timing comparator, registry validation, remaining quality controls, and both budget checks pass. The focused control discrepancy is carried to the orchestrator for review before TUNE-10 is closed.
