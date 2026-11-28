# Phase 17 delivery evidence

This is the review handoff for TUNE-07 through TUNE-11. Results are limited to the named producer, run, fixture,
configuration, or policy check. This document does not claim final phase verification, code review, publication, or
a retail playability result.

## Requirement outcomes

| Requirement | Evidence and exact producer | Outcome | Remaining evidence or blocker |
| --- | --- | --- | --- |
| TUNE-07 | `sh /private/tmp/schwaystation-plan17-02/collect.sh` — exit 0; the exact-attempt validation and run/job integrity checks — exit 0. See [`17-CI-EFFICIENCY.md`](17-CI-EFFICIENCY.md), including [Main run 45949888885](https://github.com/szTheory/schwaystation/actions/runs/45949888885). | The bounded 40-run cohort, routes, fingerprints, job times, queue observations, duplicate-work review, failures, retries, and runner labels are recorded. The latest full Main route reached `CI required` 4,199 seconds after run creation; BE32 took 4,113 seconds. | No repeated exact full-CI workload exists in the cohort; PR execution SHAs are unavailable. Do not claim a current CI speedup or use the old `ci.critical_path_s` interval as a current changed-route result. |
| TUNE-08 | `gh api 'repos/szTheory/schwaystation/actions/runs?per_page=40' --jq '.workflow_runs | length'` — exit 0; read-only failed-job log review for the four failed runs and Windows DLL job review — exit 0. Exact cases and URLs are in [`17-RELIABILITY.md`](17-RELIABILITY.md). BE32 selector and preservation analysis are in [`17-BE32-REVIEW.md`](17-BE32-REVIEW.md). | No reproduced flaky product test was found. Existing timeouts remain. TODO-022 has seven clean DLL-consumer signals out of ten. Local BE32 selector and JUnit-union controls passed; the production BE32 job stays monolithic. | TODO-022 is short by three clean signals; the original silent Windows DLL-consumer failure remains unexplained. TODO-051 and TODO-052 remain pending. No hosted shard candidate or comparable critical-path reduction exists. |
| TUNE-09 | The seven linked dimension scores and selection rationale are in [`17-QUALITY-REVIEW.md`](17-QUALITY-REVIEW.md). `build/ci/schwaystation-run --exe build/ci/fixtures/gpu_motion.exe --frames 120 --hash` — exit 0; frame-120 hash `f92191242105428b4345519f237979aaf0b50b33431ce945f209f03b1d47257d`. `ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^fixtures[.]gpu_motion[.]hash[.](60|120|self_test)$'` — exit 0, 3/3; the compatibility rows/transitions/protected-controls and doc-freshness selector — exit 0, 9/9. | The selected compatibility-breadth improvement is one fresh `gpu_motion` NTSC-U EXE frame-120 predicate. Its source identity, before/after reproduction, scope, and limits are in [`17-IMPROVEMENT-EVIDENCE.md`](17-IMPROVEMENT-EVIDENCE.md). It is distinct from mandatory documentation freshness and metric reconciliation. No hosted run was required for this fixture and ledger change. | It is not boot-to-play, audio, input-driven control, a complete session, a retail-title result, another profile, or broad library compatibility. Source review and final phase verification remain for the orchestrator. |
| TUNE-10 | Final `cmake -DQUALITY=quality.json -P tests/cmake/quality.cmake` — exit 0. Exact Cachegrind values: [run 45902643250, job 140191914798](https://github.com/szTheory/schwaystation/actions/runs/45902643250/job/140191914798). Exact state bytes: [run 45923003995, job 140258044481](https://github.com/szTheory/schwaystation/actions/runs/45923003995/job/140258044481). Latest successful hosted WASI job: [run 45949888885, job 140343426889](https://github.com/szTheory/schwaystation/actions/runs/45949888885/job/140343426889); its exact module size is not exposed. Producer and device/browser limits are in [`17-BASELINES.md`](17-BASELINES.md). | Exact `testcard`, `gpu_motion`, and stable-state values match their existing bounds. `quality.json` is unchanged under D-07. The local linked-Wasm value is 1,014,617 bytes and the retained interval is 1,014,617–1,014,665. | Current hosted WASI byte count is unobservable. The 40-run cohort has no comparable exact current PR producer/fingerprint for `ci.critical_path_s`; its historic interval and fingerprint remain unchanged and pending. Low-power rates remain assumptions; browser speed and named-device performance were not measured. |
| TUNE-11 | `sh scripts/apply-repo-settings.sh --check` — exit 0; vulnerability alerts enabled, zero open Dependabot alerts, and `.github/dependabot.yml` absent. Exact action, runner-image, pin, and policy sources are in [`17-UPGRADE-AUDIT.md`](17-UPGRADE-AUDIT.md). Final compatibility freshness check `sh scripts/compat-doc.sh tests/compat/ledger.csv docs/compatibility.md --check` — exit 0. Roadmap and prompt coverage were refreshed. | Existing install, maintenance, testing, and compatibility claim limits are reconciled. No unrelated pin, cache, workflow, or auto-merge policy changed. [`17-LESSONS-EXPORT.md`](17-LESSONS-EXPORT.md) records evidence-backed none; `.planning/LESSONS.md` is unchanged. | Wasmtime v48.0.10 remains the audit's bounded pin-update candidate for a later authorized `ci:` step. No Phase 18/19 implementation or release claim is made here. |

## Compatibility source-to-output identity

`sh scripts/compat-doc.sh tests/compat/ledger.csv docs/compatibility.md --check` exited 0; the generated report was
already current, so regeneration was unnecessary. Current Git blobs match the Plan 17-08 audit snapshot:

| Input/output | Git blob |
| --- | --- |
| `tests/compat/ledger.csv` | `a8ce81781888fe2c7dedb80ee2f61e85a2ff5ccf` |
| `tests/compat/features.csv` | `fc35b20972c1239c9d88622aa9c7bc2362773ce2` |
| `tests/compat/required-capabilities.csv` | `5efa0ebf0b8a96653be7b2d5aa2b77d37354fab8` |
| `scripts/compat-doc.sh` | `5c046e1d938669d353fc93682e4b86b9b6e9a891` |
| `docs/compatibility.md` | `493da6834801a42e64948e79c43c3967a5966f75` |

The output remains five measured fixture predicates, 13,407 unknown Redump entries, 27 unknown required profile
rows, and experimental libretro metadata. No generated claim, ledger row, feature row, capability result, or source
identity changed in this plan.

## Preserved unresolved edge assumptions and prohibition flags

All seven serialized source-audit items remain unresolved with their original category and handling:

| Requirement | Category | Still unresolved; handling carried to review/verification |
| --- | --- | --- |
| TUNE-07 | unclassified | Manually review the actual report and comparability claims. |
| TUNE-08 | empty | The generic empty/single/null-input probe remains unresolved. The concrete no-comparable-evidence policy comes from D-02; no parser product requirement is inferred. |
| TUNE-08 | encoding | Byte/code-point/grapheme/normalization semantics remain unresolved; no text-length API is inferred. |
| TUNE-09 | unclassified | Manually check the scores, tie-break, and one-improvement contract against the measured outcome. |
| TUNE-10 | boundary | The generic min/max/threshold-and-neighbor probe remains unresolved. Existing producer comparator controls are reused without claiming the generic probe is resolved. |
| TUNE-10 | precision | The generic precision/overflow/rounding probe remains unresolved. Only explicit source contracts are applied, including integer producer values and timeout ceiling arithmetic. |
| TUNE-11 | unclassified | Manually inspect maintenance, documentation, direction, and lesson coverage. |

The original serialized input is preserved here without resolving or dismissing any item:

```json
{"edge_probe":{"items":[{"requirement_id":"TUNE-07","category":"unclassified","status":"unresolved","verification":null,"resolution":null,"reason":null,"probe":"unclassified — review manually"},{"requirement_id":"TUNE-08","category":"empty","status":"unresolved","verification":null,"resolution":null,"reason":null,"probe":"What is the result for empty, single-element, or null input?"},{"requirement_id":"TUNE-08","category":"encoding","status":"unresolved","verification":null,"resolution":null,"reason":null,"probe":"Whose definition of length/equality applies — bytes, code points, grapheme clusters, or normalized form?"},{"requirement_id":"TUNE-09","category":"unclassified","status":"unresolved","verification":null,"resolution":null,"reason":null,"probe":"unclassified — review manually"},{"requirement_id":"TUNE-10","category":"boundary","status":"unresolved","verification":null,"resolution":null,"reason":null,"probe":"What happens exactly at each min/max/threshold — and one step either side?"},{"requirement_id":"TUNE-10","category":"precision","status":"unresolved","verification":null,"resolution":null,"reason":null,"probe":"Where can precision loss, overflow, or rounding/tie-breaking occur — and what is the exact contract (e.g. half-up vs half-to-even, ceil/floor/truncate)?"},{"requirement_id":"TUNE-11","category":"unclassified","status":"unresolved","verification":null,"resolution":null,"reason":null,"probe":"unclassified — review manually"}],"coverage":{"applicable":7,"resolved":0,"unresolved":7,"unclassified":3,"byVerification":{"explicit":0,"backstop":0}}},"prohibition_probe":{"items":[],"coverage":{"applicable":0,"resolved":0,"unresolved":0,"byVerification":{"test":0,"judgment":0}}}}
```

The source audit retained no additional bespoke prohibitions: `prohibition_probe.items` is empty with zero coverage.
This plan adds no prohibition flag and does not treat the empty probe as a resolved test result.

## Final light checks and handoff

All commands ran from the repository root; each exited 0. The two line budgets are exactly at their existing
ceilings. These are local checks and have no run URL.

| Command | Exit | Result |
| --- | ---: | --- |
| `cmake -DQUALITY=quality.json -P tests/cmake/quality.cmake` | 0 | Quality registry and active bounds pass; no bound or fingerprint changed. |
| `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=4694 -P tests/cmake/line_budget.cmake` | 0 | 4,694 counted lines against the 4,694-line ceiling. |
| `cmake -DROOT=. '-DPATTERNS=tests/retroarch/*' -DLIMIT=300 -P tests/cmake/line_budget.cmake` | 0 | 300 counted lines against the 300-line ceiling. |
| `sh scripts/compat-doc.sh tests/compat/ledger.csv docs/compatibility.md --check` | 0 | Generated compatibility report is fresh; no regeneration was needed. |
| `git diff --check origin/main` | 0 | No whitespace errors. |

No full build, broad test suite, hosted workflow, or code review ran in this plan.

Code review is next, before final Phase 17 verification. The phase orchestrator owns review/fixes, the final required
local and hosted workflow, STATE/ROADMAP updates, and the next phase boundary. Phase 18 homebrew and Phase 19
owner-selected retail scope remain approved and unstarted. No user action is needed for this handoff.
