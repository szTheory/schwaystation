# Phase 7 delivery evidence and ship handoff

Updated 2026-11-19. Runtime/model: Codex/OpenGSD CLI 1.20.0; executor gpt-6-luna at xhigh effort. Plan 07-10 is
the final phase execution plan. This document separates local evidence from hosted results that only exist after
the reviewed phase branch is shipped. No push, PR, merge, hosted workflow or triage dispatch was run here.

## Focused local evidence

The initial preflight confirmed all expected selectors were registered, workflow and producer paths were present,
and the current inputs were internally consistent. Before the ratchet, the repository-check inventory counted
3,890 lines against the corrected 3,900 ceiling; the RetroArch inventory counted 300 against its separate 300
ceiling. `quality.json` parsed and recorded current testcard, gpu_motion, state and CI values; `size.wasm` remains
explicitly unbounded. The workflow source links the Cachegrind, state, WASI module and current-attempt CI outputs
to the shared comparator. The triage workflow and its fixture-based privacy/schema controls are present.

The hygiene `policy.gate_budget` registration is now ratcheted to the measured 3,890 lines. The separate
`policy.e2e_budget` limit remains 300. No case or assertion was removed or weakened.

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
| `sh scripts/check-title.sh 'feat: publish the weekly triage artifact' < build/phase07/pr-body.md` | 0 | Proposed title/body pass the local Conventional Commit and no-CI-skip checks. The body file is ignored build output. |
| `jq -e '.packages["."].["release-type"] == "simple"' release-please-config.json` | 0 | The unscoped `feat:` title uses the configured release-please simple strategy and is release-affecting. |
| `git diff --check HEAD` | 0 | No whitespace errors in the scoped work. |

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

## Requirement status and ship-only evidence

| Requirement | Local or historical result | Remaining evidence and locator |
| --- | --- | --- |
| TUNE-01 | The `ci.triage` production selector passes 1/1 inside the serialized CI selector; it checks bounded sanitized output and failure behavior. | Verify one default-branch scheduled/manual artifact after merge. Expected manual command: `gh workflow run triage.yml --ref main`; inspect that run and download `weekly-triage`. Record the successful Actions URL and artifact result. No dispatch was started during this plan. |
| TUNE-02 | The [CI report](07-CI-EFFICIENCY.md) captures 40 workflow runs, 573 job records, comparable workload groups and the original critical path. Production release-route and exact-attempt timing controls pass locally. | Compare the real generated release-App PR against the explicitly qualified baseline below; include required-check parity, the matching final PR head, path list, slowest job, critical path and run/job URLs. No speedup is claimed before that comparison. |
| TUNE-03 | The report links sampled timeout formulas to their extremal run IDs and reviewed all captured failures; it found no confirmed same-workload flaky test. | Complete the post-change release-App comparison at ship and preserve the current timeout provenance. The baseline sample remains small and its unsampled jobs remain provisional. |
| TUNE-04 | Plan 04's selected README correction passed its obsolete-claim and diff checks; see [Plan 04 summary](07-04-SUMMARY.md) and [quality review](07-QUALITY-REVIEW.md). | No additional change is selected in Plan 10. Phase readiness and final requirement audit still occur after independent review. |
| TUNE-05 | `policy.quality_metrics`, `policy.lessons_unique`, and its duplicate negative control pass in the focused CI selector. The current registry records both Ir/frame fixtures, 3,755,020 state bytes, and the historical comparable CI range. | Measure the actual Release `build/wasm32/schwaystation-run` module on Ubuntu 26 and set `size.wasm` only from those bytes. The linked WASI module size remains unmeasured and unbounded. A matching hosted PR critical-path measurement also remains for ship. |
| TUNE-06 | The [upgrade audit](07-UPGRADE-AUDIT.md), docs byte budget, settings/pin checks and prior install-line evidence record unchanged items. The README invocation still matches `tests/cmake/install_line.cmake` and the successful install-line-latest check in [release run 44262491280](https://github.com/szTheory/schwaystation/actions/runs/44262491280). | Confirm the remaining phase evidence after the reviewed final head and real release. Hardware pages stay unchanged because this phase changes CI policy and documentation rather than emulator behavior. |

## Release-App comparison row

| Cohort | Before | After |
| --- | --- | --- |
| Release App PR #23, generated metadata only | [run 44253419074](https://github.com/szTheory/schwaystation/actions/runs/44253419074); exact files were `.release-please-manifest.json`, `CHANGELOG.md`, and `version.txt`; `be32` took 60:21. The merge-ref SHA was not retained, so this is not an exact workload-fingerprint or timeout comparison. | **Pending ship.** Use the automatic App PR after the Plan 06 route. Record its head and merge SHA, stable complete file list, required-check parity and allowed skips, slowest job, critical path, and run/job URLs. Compare only if the generated workload matches. |

## Ship sequence and evidence still to collect

After independent code review and final local verification, the authorized ship operator must:

1. Run the installed GSD ship flow for the phase, then apply the verified final title `feat: publish the weekly triage artifact` once and wait for successful `CI required` on the exact final SHA.
2. Capture one broad CI run on that changed head with all required jobs and only established conditional skips; record the PR head SHA and run URL.
3. Merge only after those checks, then record green main CI and the generated release-App PR's successful checks. Verify the real Release workflow, published release, and `install-line-latest` result.
4. Complete the release-App comparison row above and record one successful default-branch triage artifact with its run URL.
5. Measure the actual linked WASI module on the supported Ubuntu 26 lane before adding a size bound; keep all unmeasured or incomparable values explicitly pending.

There is no final PR SHA, generated release PR, merged-main run, release URL, or post-merge triage URL yet. They are
ship evidence, not inferred from the current branch or older runs. Phase 8 does not start until that release
evidence is complete.
