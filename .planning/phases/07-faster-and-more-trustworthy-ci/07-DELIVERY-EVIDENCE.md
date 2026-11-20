# Phase 7 delivery evidence and ship handoff

Updated 2026-11-19. Runtime/model: Codex/OpenGSD CLI 1.20.0; executor gpt-6-luna at xhigh effort. Plan 07-10 is
the final phase execution plan. The original handoff and intermediate ship sections below preserve what was
known at each checkpoint. The final ship section supersedes their pending statements and records the completed
PR, hosted, merge, release, and artifact evidence without treating canceled attempts as passes.

## Focused local evidence

The initial preflight confirmed all expected selectors were registered, workflow and producer paths were present,
and the current inputs were internally consistent. Before the ratchet, the repository-check inventory counted
3,890 lines against the then-current 3,900 ceiling; the RetroArch inventory counted 300 against its separate 300
ceiling. At that handoff, `quality.json` recorded current testcard, gpu_motion, state and CI values while
`size.wasm` remained unbounded. The workflow source linked the Cachegrind, state, WASI module and current-attempt
CI outputs to the shared comparator. The triage workflow and its fixture-based privacy/schema controls were present.

At the Plan 10 handoff, the hygiene `policy.gate_budget` registration was ratcheted to 3,890 lines. The
independent review fixes added required regression coverage; the count reached 3,900 and the registration was
carried forward as Plan 10 directs. A later Windows run exposed CRLF output from native `jq`; a one-line
normalization in the timing producer adds one counted line. The current exact budget is therefore 3,901. The
separate `policy.e2e_budget` limit remains 300. No case or assertion was removed or weakened.

| Command | Exit | Result |
| --- | ---: | --- |
| `cmake --preset ci` | 0 | CI tree configured before the focused selector run. |
| `ctest --preset ci --no-tests=error -R '^(ci[.](classify_policy|triage)|policy[.](quality_metrics|lessons_unique([.]self_test)?))$' --output-on-failure` | 8 | Preset-parallel selector: 4/5 passed; `policy.quality_metrics` hit its registered 30-second timeout at 30.01 seconds. |
| `cmake -DROOT="$PWD" -DBIN="$PWD/build/ci/policy/quality" -P tests/cmake/quality_controls.cmake` | 0 | From the repository root, the producer/comparator and roll-up integration script completed successfully in about 25 seconds. |
| `ctest --preset ci --parallel 1 --no-tests=error -R '^(ci[.](classify_policy|triage)|policy[.](quality_metrics|lessons_unique([.]self_test)?))$' --output-on-failure` | 0 | Before timeout adjustment: 5/5 passed in 30.21 seconds; `policy.quality_metrics` took 29.69 seconds. |
| `cmake --preset ci` | 0 | Reconfigured after the evidence-based timeout adjustment. |
| `ctest --preset ci --parallel 1 --no-tests=error -R '^(ci[.](classify_policy|triage)|policy[.](quality_metrics|lessons_unique([.]self_test)?))$' --output-on-failure` | 0 | After changing only `policy.quality_metrics` TIMEOUT from 30 to 60 seconds: 5/5 passed in 39.17 seconds; the metric integration test took 38.61 seconds. This is timeout headroom, not a speedup. |
| `cmake --preset ci` | 0 | Reconfigured the existing CI build tree after refining the `policy.quality_metrics` timeout to 80 seconds. |
| `ctest --preset ci --parallel 1 --no-tests=error -R '^policy[.]quality_metrics$' --output-on-failure` | 0 | Isolated serialized rerun passed 1/1 in 28.26 seconds; CTest reports 28.25 seconds for the test. |
| `cmake --preset hygiene` | 0 | Hygiene tree configured. |
| `ctest --preset hygiene --no-tests=error -R '^(ci[.]classify|policy[.](gate_budget|e2e_budget|action_pins)|hygiene[.]timeout_comments)$' --output-on-failure` | 0 | 5/5 passed, including the ratcheted gate and action-pin controls. |
| `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3890 -P tests/cmake/line_budget.cmake` | 0 | Repository-check total is 3,890/3,890 lines. |
| `cmake -DROOT=. '-DPATTERNS=tests/retroarch/*' -DLIMIT=300 -P tests/cmake/line_budget.cmake` | 0 | RetroArch test total is 300/300 lines. |
| `ctest --preset ci --no-tests=error -R '^policy[.]quality_metrics$' --output-on-failure --parallel 1` | 0 | Post-review selector passed 1/1 in 37.22 seconds with the checked-in push baseline preserved and PR timing explicitly unbaselined. |
| `cmake --preset hygiene`, then focused CTest for gate budget, E2E budget, action pins, and timeout comments | 0 | Refreshed the stale generated 3,890-line registration; the four selected checks passed, confirming 3,900/3,900 and 300/300 before the Windows timing fix. |
| `sh scripts/check-title.sh 'feat: publish the weekly triage artifact' < build/phase07/pr-body.md` | 0 | Proposed title/body pass the local Conventional Commit and no-CI-skip checks. The body file is ignored build output. |
| `jq -e '.packages["."].["release-type"] == "simple"' release-please-config.json` | 0 | The unscoped `feat:` title uses the configured release-please simple strategy and is release-affecting. |
| `git diff --check HEAD` | 0 | No whitespace errors in the scoped work. |
| `jq -e . quality.json >/dev/null && cmake -DQUALITY=quality.json -P tests/cmake/quality.cmake` | 0 | The measured WASI bound parses and passes the shared quality registry contract. |
| `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` | 0 | Corrected repository-check inventory is 3,901/3,901 lines. |
| `cmake -DROOT=. '-DPATTERNS=tests/retroarch/*' -DLIMIT=300 -P tests/cmake/line_budget.cmake` | 0 | RetroArch test inventory remains 300/300 lines. |
| `cmake -DMODE=timeouts -DDIR=.github/workflows -P tests/cmake/workflows.cmake` | 0 | All 25 workflow timeout comments name a run ID or provisional evidence. |
| `cmake --preset ci` and `cmake --preset hygiene` | 0 | Both existing build trees reconfigured successfully before focused tests. |
| `ctest --test-dir build/ci --no-tests=error -R '^policy[.]quality_metrics$' --output-on-failure --parallel 1` | 0 | 1/1 passed in 30.27 seconds with the bounded WASI registry and production PR timing control. |
| `ctest --test-dir build/hygiene --no-tests=error -R '^(policy[.](gate_budget|e2e_budget|action_pins)|hygiene[.]timeout_comments)$' --output-on-failure --parallel 1` | 0 | 4/4 passed in 0.03 seconds, including the 3,901-line and timeout-comment checks. |
| `git diff --check` | 0 | No whitespace errors in the corrected diff. |

The initial parallel timeout was diagnosed from the test registration and exact `quality_controls.cmake` command.
Serialized measurements were 29.69 seconds and 38.61 seconds in the five-test selector; after the timeout was
refined to 80 seconds, the isolated serialized rerun passed in 28.25 seconds. These are timeout headroom
adjustments, not speedups. The focused selectors retain all cases and assertions.

No full CI preset, full test suite, code review, or hosted workflow was run as part of this handoff. Plan 10
commits the measured test-timeout headroom and gate ratchet; phase-level independent code review and final
verification must happen before ship.

## Quality review outcome

The seven ratings remain in [07-QUALITY-REVIEW.md](07-QUALITY-REVIEW.md). Plan 04's single selected improvement
remains the README status correction, verified against the previously false BIOS/disc claim; Plan 10 found no
scope conflict and selected no second change. The current CI/CD score remains 3: production selectors pass, but
the real generated release-App PR's before/after timing and required-check parity are still pending. Upgrade path
remains 2: `.github/dependabot.yml` is absent, so there is no grouped Actions update configuration to parse and no
auto-merge workflow is active. No updater, auto-merge, pin, cache or runner change was added.

## Independent review and verification follow-up

The first independent review found a PR head/merge SHA mismatch, stale or incomplete workload fingerprints, and
quality-registry floor validation gaps. The fixes preserve existing production gate cases and add focused
regressions. A second review found that the push/main timing range must not be applied to the different PR
route; the final review is clean after that range was restored to its original push fingerprint. The production
PR timing result remains explicitly unbaselined until ship provides matching hosted measurements. The synthetic
controls prove the production route wiring and comparator behavior, not real PR timing comparability.

The final local `policy.quality_metrics` selector passed 1/1 in 37.22 seconds. After `cmake --preset hygiene`
refreshed its stale generated test registration, the four focused hygiene checks passed, with the repository
gate at exactly 3,900 lines and the RetroArch budget at 300. The verifier also confirmed the plan artifacts,
workflow markers, selector inventory and locally automatable behavior. Its report records hosted acceptance
items as pending ship evidence, not as passed or as owner UAT.

## Ship follow-up: first hosted attempt and corrections

PR [#28](https://github.com/szTheory/schwaystation/pull/28) first ran at head `8a3a502ab4dd3a47fe906e7976a33680a71b8584`.
The successful WASI job measured the linked module at 595,820 bytes on Ubuntu 26.04.1 image 20261111.196.1,
with `libclang-rt-21-dev-wasm32` 1:21.1.8-6ubuntu1 and `wasi-libc` 0.0~git20250726.3f7eb4c-4. The exact
measurement is recorded in `quality.json` and [07-BASELINES.md](07-BASELINES.md).

Both Windows build jobs reached the test inventory but failed the same `policy.quality_metrics` timing control:
Windows-native `jq` emitted CRLF, so the elapsed-seconds parser rejected the trailing carriage return. The same
test failed on both Windows runners while their other 617 tests passed. A one-line CRLF normalization now follows
the successful `jq` write, preserving its failure status. This real Windows test failure is the regression case;
the corrected test must pass on both Windows lanes in the next exact-head CI run.

The separate PR-triggered Nightly fuzz job [run 44460238938](https://github.com/szTheory/schwaystation/actions/runs/44460238938)
was canceled at its 5-minute job timeout. Successful comparable native fuzz job [run 44190163638](https://github.com/szTheory/schwaystation/actions/runs/44190163638)
took 7m03. Its doubled duration is 14m06, rounded up to 15 minutes; the workflow timeout now follows that
evidence. The incomplete first PR CI run [44460238941](https://github.com/szTheory/schwaystation/actions/runs/44460238941)
was canceled after the required Windows failures were confirmed and its completed WASI log yielded the size.

These corrections have passed read-only code review and focused local checks, including the quality metric
integration and the four relevant hygiene tests. A new exact-head hosted run remains pending; it must confirm the
Windows fix, checked-in WASI bound, all required CI lanes, and revised fuzz timeout before Phase 7 can ship.

## Second Windows failure and correction

The next exact-head CI run [44464772297](https://github.com/szTheory/schwaystation/actions/runs/44464772297) used
commit `4d389aef0a58ceadc8e010a15559852bbbe237f4`. Its x64 Windows lane cleared the CRLF parser failure but
exposed a second portability issue: CMake supplied `D:/a/.../quality.json`, while the timing script treated only
`/*` as absolute and incorrectly prefixed the repository root. The arm64 lane passed; the x64 registry read
failed, and this known-failing run was canceled. The aggregate conclusion is not a pass.

Independent follow-up review confirmed the corrected `case` accepts Windows drive-rooted and POSIX absolute
paths and continues to root relative paths, with the `quality` value quoted. Before another hosted run, these
cheap checks passed:

| Command | Exit | Result |
| --- | ---: | --- |
| `cmake --preset hygiene && cmake --preset ci` | 0 | Both existing trees configured before the focused selectors. |
| `sh -n scripts/ci-critical-path.sh && git diff --check` | 0 | Script syntax and patch whitespace are valid. |
| `ctest --test-dir build/ci --no-tests=error -R '^policy[.]quality_metrics$' --output-on-failure --parallel 1` | 0 | 1/1 passed in 35.45 seconds after the Windows path correction. |
| `ctest --test-dir build/hygiene --no-tests=error -R '^(policy[.](gate_budget|e2e_budget|action_pins)|hygiene[.]timeout_comments)$' --output-on-failure --parallel 1` | 0 | 4/4 passed in 0.03 seconds; repository checks remain 3,901/3,901 and RetroArch remains 300/300. |

PR Nightly run [44464210494](https://github.com/szTheory/schwaystation/actions/runs/44464210494) completed
successfully on the preceding head, including all six fuzz campaigns under the measured 15-minute timeout; the
corrected code head must run Nightly again before ship. Do not edit PR metadata while checks run: this workflow
treats PR body edits as CI-triggering `edited` events and cancels/restarts in-progress work.

On corrected-path head `57501d46bf5b102e448e0436e7f28fc888cbe136`, the Windows x64 lane passed
`policy.quality_metrics` in 38.51 seconds. Windows ARM64 ran the same registered check but CTest stopped it at
the existing 80-second limit; its other 617/618 tests passed. The CI run [44466369143](https://github.com/szTheory/schwaystation/actions/runs/44466369143)
was canceled after that known failure, so its aggregate is not green. The matching Nightly run
[44466369199](https://github.com/szTheory/schwaystation/actions/runs/44466369199) completed successfully,
including full vectors, all three native benchmark legs, WASI and fuzz.

The focused quality control invokes many separate CMake checks. A read-only follow-up review approved increasing
its CTest timeout from 80 to 180 seconds as a provisional cross-platform allowance; the ARM64 completion time is
not known from the timed-out attempt. All assertions and checks remain enabled. After `cmake --preset ci`, the
local serialized selector passed 1/1 in 29.36 seconds. The next exact-head Windows ARM64 run must establish the
real duration before this allowance can be described as measured headroom.

| Command | Exit | Result |
| --- | ---: | --- |
| `git diff --check && cmake --preset ci` | 0 | The timeout change is whitespace-clean and the CI selector was regenerated before running it. |
| `ctest --test-dir build/ci --no-tests=error -R '^policy[.]quality_metrics$' --output-on-failure --parallel 1` | 0 | 1/1 passed in 29.36 seconds with the provisional 180-second cap. |

## Requirement status at pre-ship handoff (historical)

| Requirement | Local or historical result | Remaining evidence and locator |
| --- | --- | --- |
| TUNE-01 | The `ci.triage` production selector passes 1/1 inside the serialized CI selector; it checks bounded sanitized output and failure behavior. | Verify one default-branch scheduled/manual artifact after merge. Expected manual command: `gh workflow run triage.yml --ref main`; inspect that run and download `weekly-triage`. Record the successful Actions URL and artifact result. No dispatch was started during this plan. |
| TUNE-02 | The [CI report](07-CI-EFFICIENCY.md) captures 40 workflow runs, 573 job records, comparable workload groups and the original critical path. Production release-route and exact-attempt timing controls pass locally. | Compare the real generated release-App PR against the explicitly qualified baseline below; include required-check parity, the matching final PR head, path list, slowest job, critical path and run/job URLs. No speedup is claimed before that comparison. |
| TUNE-03 | The report links sampled timeout formulas to their extremal run IDs and reviewed all captured failures; it found no confirmed same-workload flaky test. | Complete the post-change release-App comparison at ship and preserve the current timeout provenance. The baseline sample remains small and its unsampled jobs remain provisional. |
| TUNE-04 | Plan 04's selected README correction passed its obsolete-claim and diff checks; see [Plan 04 summary](07-04-SUMMARY.md) and [quality review](07-QUALITY-REVIEW.md). | No additional change is selected in Plan 10. Phase readiness and final requirement audit still occur after independent review. |
| TUNE-05 | `policy.quality_metrics`, `policy.lessons_unique`, and its duplicate negative control pass in focused CI. The registry records both Ir/frame fixtures, 3,755,020 state bytes, the measured 595,820-byte WASI module, and the historical push/main CI range bound to its original fingerprint. The production PR route is intentionally unbaselined. | Verify the checked-in WASI size bound on the corrected final head. Measure matching final-head PR timing attempts before setting a PR-route range; do not reuse the push/main interval without evidence. |
| TUNE-06 | The [upgrade audit](07-UPGRADE-AUDIT.md), docs byte budget, settings/pin checks and prior install-line evidence record unchanged items. The README invocation still matches `tests/cmake/install_line.cmake` and the successful install-line-latest check in [release run 44262491280](https://github.com/szTheory/schwaystation/actions/runs/44262491280). | Confirm the remaining phase evidence after the reviewed final head and real release. Hardware pages stay unchanged because this phase changes CI policy and documentation rather than emulator behavior. |

## Release-App comparison row at pre-ship handoff (historical)

| Cohort | Before | After |
| --- | --- | --- |
| Release App PR #23, generated metadata only | [run 44253419074](https://github.com/szTheory/schwaystation/actions/runs/44253419074); exact files were `.release-please-manifest.json`, `CHANGELOG.md`, and `version.txt`; `be32` took 60:21. The merge-ref SHA was not retained, so this is not an exact workload-fingerprint or timeout comparison. | **Pending ship.** Use the automatic App PR after the Plan 06 route. Record its head and merge SHA, stable complete file list, required-check parity and allowed skips, slowest job, critical path, and run/job URLs. Compare only if the generated workload matches. |

## Ship sequence recorded before ship (historical)

After independent code review and final local verification, the authorized ship operator must:

1. Run the installed GSD ship flow for the phase, then apply the verified final title `feat: publish the weekly triage artifact` once and wait for successful `CI required` on the exact final SHA.
2. Capture a broad CI run on that changed head with all required jobs and only established conditional skips. It should report the current PR workload as unbaselined against the distinct push/main fingerprint; record the PR head SHA, measured path, fingerprint and run/job URL. Gather matching PR attempts before setting a PR timing range, then confirm the comparator on the updated final head.
3. Merge only after those checks, then record green main CI and the generated release-App PR's successful checks. Verify the real Release workflow, published release, and `install-line-latest` result.
4. Complete the release-App comparison row above and record one successful default-branch triage artifact with its run URL.
5. Confirm the exact 595,820-byte bound on the supported Ubuntu 26 lane; keep all unmeasured or incomparable values explicitly pending.

At this checkpoint, the final PR SHA, generated release PR, merged-main run, release URL, and post-merge triage
URL had not yet been observed. The following final ship record supersedes this checkpoint.

## Final ship evidence and requirement closeout

Phase 7 shipped through [PR #28](https://github.com/szTheory/schwaystation/pull/28), whose exact final head
`9bda0bd6aea82381732247e79f04fd5c64b6877e` passed required CI in
[run 44468099352](https://github.com/szTheory/schwaystation/actions/runs/44468099352) and Nightly in
[run 44468099361](https://github.com/szTheory/schwaystation/actions/runs/44468099361). The Ubuntu 26 WASI job
measured the linked module at 595,820 bytes; the checked-in bound was exercised on the final head. Windows ARM64
ran `policy.quality_metrics` in 116.03 seconds under its 180-second cap, validating the cap with 63.97 seconds
of observed headroom.

The automatic release PR [#29](https://github.com/szTheory/schwaystation/pull/29) contained exactly
`.release-please-manifest.json`, `CHANGELOG.md`, and `version.txt`. Its required check passed in
[run 44476990492](https://github.com/szTheory/schwaystation/actions/runs/44476990492) in about 70 seconds; the
metadata-only route retained `CI required` and skipped only the measured heavy matrix. The prior release PR
baseline [run 44253419074](https://github.com/szTheory/schwaystation/actions/runs/44253419074) had the same three
paths and a 60:21 BE32 job, but its merge-ref SHA is unavailable. Therefore the report records the route and
required-check parity but makes no exact fingerprint comparison or speedup claim.

Release PR #29 merged as `af64484f388eddf67c615673b98edd6ccb5dae4c`. Main CI on that exact commit passed on its
targeted second attempt in [run 44477155668](https://github.com/szTheory/schwaystation/actions/runs/44477155668).
The first attempt's macOS x64 job was an anomalously slow hosted run: three tests timed out after the same tests
passed on the release workflow for the identical commit. Only the failed macOS path was rerun; its tests,
hash-equality check, RetroArch check, and final `CI required` all passed. The separate automatic Release workflow
passed in [run 44477155670](https://github.com/szTheory/schwaystation/actions/runs/44477155670), published
[v0.7.0](https://github.com/szTheory/schwaystation/releases/tag/v0.7.0) with 19 assets, and passed
`install-line-latest`.

The default-branch weekly triage workflow passed in
[run 44486437761](https://github.com/szTheory/schwaystation/actions/runs/44486437761). Its downloaded
`weekly-triage` artifact passed schema validation, reported zero open issues without truncation, and listed the
current CI, release, and Nightly runs as successful.

| Requirement | Final evidence | Status |
| --- | --- | --- |
| TUNE-01 | Sanitized default-branch artifact uploaded and downloaded from run 44486437761; schema valid, zero open issues, no truncation. | Complete |
| TUNE-02 | The report compares the exact generated file list, route, and required-check parity; baseline merge SHA is missing, so it explicitly makes no speedup claim. | Complete |
| TUNE-03 | Timeout derivations and flaky-check dispositions retain run evidence; the ARM64 quality control completed in 116.03s under its 180s cap. The macOS retry followed same-SHA evidence and was limited to the failed path. | Complete |
| TUNE-04 | Seven-dimension review and selected README correction remain verified in [07-QUALITY-REVIEW.md](07-QUALITY-REVIEW.md). | Complete |
| TUNE-05 | Exact Ubuntu 26 measurement is 595,820 WASI bytes; final PR CI exercised the checked-in bound. The PR timing route remains intentionally unbaselined against the distinct push/main fingerprint. | Complete |
| TUNE-06 | Upgrade/settings/pin/docs audits and installation guidance are documented; the published v0.7.0 run passed `install-line-latest`. | Complete |

Two full workflow matrices ran concurrently on the same release commit: main push CI and Release's reusable CI.
That duplicates runner minutes but does not establish a wall-clock delay because the workflows overlapped. No
required coverage was removed. The larger artifact-reuse question is recorded in
[`050-release-ci-artifact-reuse.md`](../../todos/pending/050-release-ci-artifact-reuse.md), bound to a future
Phase 9 plan that first edits the CI/release workflows.

Phase 7 ship evidence is complete. Phase 8 may proceed from the verified main commit above.
