# Phase 7: Faster and more trustworthy CI - Context

**Gathered:** 2026-11-18
**Status:** Ready for planning

<domain>
## Phase Boundary

Tune the repository's CI, quality baselines, and maintainer guidance before v2.0 feature work begins. Deliver a weekly read-only triage artifact, an evidence-backed CI efficiency report, a seven-dimension quality review with one improvement to its lowest score, measured `quality.json` bounds and a lesson-uniqueness check, and current settings/dependency/pin/docs evidence. Keep all existing required checks and coverage. Do not change emulator behavior or make the report claim title compatibility beyond the evidence already collected.

</domain>

<decisions>
## Implementation Decisions

### CI evidence and efficiency
- Compare at most 40 recent runs only when workflow, matrix leg, and workload fingerprint match. Show sample size, evidence links, critical path, queue/wall times where available, retry/flaky signals, runner-core use, and cache evidence. State when data cannot support a comparison; never infer a speedup from unlike runs.
- Derive each timeout from twice the slowest matching successful or failed execution duration, retaining the established setup floor and naming the source run ID beside the value. Fix each evidenced flake or identify the stronger retained signal that makes it redundant; do not mask or drop required coverage.
- Treat BE32's 65-72 minute runs as the observed current critical path. Inspect whether the same workload is needlessly repeated across PR, main, and release events before proposing changes. Retain the required post-merge main CI and release rehearsal, all eight relevant lanes, and the existing planning-only fast path. No broad matrix trimming, blind cache, or concurrency increase.
- Run the report and focused local preflights first; use one changed-head hosted run for coherent workflow changes. A scoped `act --dryrun` validates wiring only and is not evidence that jobs/tests pass.

### Weekly triage artifact
- Produce a short-retention artifact from scheduled and manually dispatched runs. It contains a fixed schema and only useful open-issue metadata plus the latest workflow/run status; omit bodies, logs, authors, branches, commit SHAs, and any message/posting action.
- Filter pull requests returned by the issue API. Bound and sanitize untrusted titles/labels, removing control/ANSI text, e-mail addresses, and private filesystem paths before JSON encoding. Use the smallest job-level read permissions needed and no write permissions; use the repository's existing SHA-pinned artifact action and a seven-day retention period.
- Prefer a small built-in shell/JQ implementation over adding a runtime dependency or general-purpose report service. Add focused positive and hostile-input controls.

### Quality, docs, and dependencies
- Keep one checked-in `quality.json` with the required measured ceilings and floors for instructions per frame, Wasm bytes, serialized state bytes, and comparable CI critical-path seconds. Record the reason for the repository-check cap adjustment in `reasons[]`; `policy.gate_budget` remains the only gate for that count. Ratchet measurements, reuse CMake `string(JSON)` and negative controls, and keep the existing dependencies.
- Check lesson uniqueness by both identity and normalized lesson text, with a negative control. Verify each new gate prevents a named recurring fault. Apply the dated repository-check ceiling decision below and retain the 300-line RetroArch ceiling.
- Score all seven dimensions with direct evidence links, then implement and verify exactly one evidence-backed improvement in the lowest-scoring dimension. Do not select the improvement before scoring.
- Check repository settings with the existing `scripts/apply-repo-settings.sh --check`, current Dependabot major updates, runner images, and named tool pins. Add a minimal grouped Dependabot configuration for GitHub Actions if evidence confirms the absent config is the weakest upgrade-path item; do not enable auto-merge.
- Refresh only documentation shown stale by code/settings changes, including README install guidance, device/size budgets, roadmap, and prompt-coverage records. Keep detailed evidence in phase artifacts and user-facing README concise.

### Execution and verification
- Before long builds, test suites, guest runs, or hosted workflows, run the applicable cheap inventory/count/selector/budget check. Reuse current CMake checks and existing presets instead of adding an overlapping gate.
- Repository-check code remains CMake `-P` or POSIX shell, within the existing phase budget. Keep code review before final verification, then ship and release this phase before Phase 8.

### Repository-check cap adjustment on 2026-11-18

- Plan 01 measured 3,133 counted lines under the original 3,200 ceiling, leaving 67. Independent quality-gate review estimated 90–135 additional counted lines for the fully tested triage, release routing, and measured quality work. The estimated final total is 3,223–3,268, or 23–68 lines above the original ceiling. [Completed Plan 01 evidence](07-01-SUMMARY.md#verification)
- Set the outer repository-check ceiling to 3,300, allowing 167 additional lines and leaving 32 above the upper estimate. Plan 03 updates the existing gate before Plans 05–08 add counted work. At ship ratchet the enforced limit to the actual total. Keep the inventory scope, the required production-path and negative controls, and the separate 300-line RetroArch cap.
- Plan 07 records a dated `quality.json` `reasons[]` entry with the prior cap, the 3,133 measured lines, the 90–135 net estimate, the revised cap, and the ship ratchet. This is rationale metadata for the process budget.

### Superseding repository-check cap adjustment on 2026-11-18

- Preflight after Plan 06 measured 3,299/3,300 lines. Plans 05–06 added 166 counted lines after Plan 01, already 31 above the prior 135-line upper forecast. A read-only plan review estimates a further 70–100 lines for Plan 07-07 and 300–400 lines for Plan 07-08; Plans 09–10 add none to the counted globs. Keeping the cap would require removing or relocating 370–500 lines of required production-path controls, or weakening existing assertions.
- Correct the temporary ceiling to 3,900: the current 3,299 plus the 500-line upper estimate plus 100 lines of headroom. Preserve every existing check and all counted-file globs. This is a correction to a demonstrably stale estimate, not authorization for unrelated gate growth. Plan 10 still ratchets the ceiling to the actual shipping total.
- The focused WASI module is not present in the linked prior run artifacts; only its hash manifest was uploaded. Keep the Wasm bound absent until the focused Ubuntu 26 WASI Release build produces the actual module size. Do not infer a value from the hash artifact or another build.

</decisions>

<code_context>
## Existing Code Insights

- `.github/workflows/ci.yml` already uses a `changes` job for planning-only changes without trigger-level path filters; its required aggregator remains active. Preserve this behavior.
- `.github/workflows/ci.yml` already has timeout comments and pins actions by SHA. Existing workflow, budget, and hygiene checks include negative controls; extend only where a named Phase 7 requirement needs it.
- The BE32 `portable` leg is the observed long pole (about 65-72 minutes in recent comparable runs). CMake presets already use the processor-default test concurrency for BE32/native tests; there is no evidence to blindly increase it.
- `docs/testing.md` documents scoped local `act`; the installed `act --dryrun` passed for the `changes` job. It checks workflow wiring, not guest test results or hosted runner behavior.
- `scripts/apply-repo-settings.sh --check` already reports repository settings, including workflow permissions and Dependabot alert settings. `.github/dependabot.yml` is currently absent.
- Quality checks cover docs byte budgets, repository-gate lines, RetroArch lines, workflow timeout comments, and pinned action SHAs. The gate count was 2,700 before Phase 7 and 3,133 after Plan 01.
- Plan 01 added `policy.lessons_unique` and the tested CMake comparator. Plan 07 creates the root `quality.json` and wires the live measurements.
- `.planning/config.json` has a pre-existing user edit. Preserve it and stage only files explicitly assigned to each commit.

</code_context>

<specifics>
## Specific Ideas

- Source the weekly artifact from the GitHub Issues and Actions metadata APIs; no issue body or run log is needed to satisfy triage.
- Include run URLs and run IDs in the CI efficiency report as attributable evidence. Report retries and `suite-flake` signals only where run history supports them; distinguish flaky failures from source regressions.
- The current main-branch planning-only path is already a measured improvement; document and retain it rather than counting it as a new change in this phase.
- Revisit actions/runner update automation as a bounded grouped weekly PR flow; updates must remain reviewable and must not auto-merge.

</specifics>

<deferred>
## Deferred Ideas

- Do not add owner hardware captures, full emulator accuracy campaigns, new CI service dependencies, general dashboards, or a separate report-generation platform in this tune-up.
- Keep the usual requirement for broad hosted CI on behavior/workflow changes. One-off documentation and planning edits use attributable existing evidence and do not justify another full suite.
- If BE32 remains the critical path after the redundancy analysis and no measured safe shortening is available, record that outcome and its constraint instead of removing the lane or claiming an unmeasured optimization.

</deferred>
