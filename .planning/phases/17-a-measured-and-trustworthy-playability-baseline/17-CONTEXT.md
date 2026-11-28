# Phase 17: A measured and trustworthy playability baseline - Context

**Gathered:** 2026-11-27
**Status:** Ready for planning

<domain>
## Phase Boundary

Before v3.0 playability work, this phase refreshes current installation guidance, evidence-scoped compatibility claims, performance and CI measurements, repository-maintenance evidence, and the rolling roadmap. It delivers the TUNE-07 through TUNE-11 outcomes in ROADMAP.md and REQUIREMENTS.md. It does not implement the homebrew session or retail-game proof assigned to Phases 18 and 19, claim broad compatibility from a single title, or change emulator behavior without a separately scoped evidence-backed need.

</domain>

<decisions>
## Implementation Decisions

### CI evidence and reliability
- **D-01:** Compare no more than the latest 40 runs, and calculate timing or speedup only for runs with matching workflow, matrix, and workload fingerprints. Show which runs match, their sample size, run URLs, critical path, job durations, queue time when available, duplicate work, retries, flake signals, and runner-core use. Unmatched, cancelled, metadata-only, or otherwise different workloads may be described but must not be pooled into a speedup claim.
- **D-02:** Derive a changed timeout from twice the slowest comparable run for that job and record the source run ID beside it. If the comparable sample is absent, keep the existing timeout and report the evidence gap. Fix a reproduced flaky cause or name the stronger retained signal; do not mask a failure, drop coverage, or treat an unreliable harness assertion as a product failure.
- **D-03:** Re-evaluate the Phase 16 BE32 candidate with both long tests, libretro.host and libretro.input.movies. Keep current gates unless private same-configuration workspaces, disjoint selectors, complete test-inventory and JUnit unions, fixture/artifact/hash preservation, and a comparable hosted run prove a real critical-path reduction after duplicated setup. Runner time alone is not a billing-cost claim. TODO-052 is in scope for this measured review.
- **D-04:** Audit duplicate runner/checkpoint work through TODO-049, but remove a command only if new evidence proves the result is redundant and an independent check remains. Keep TODO-051 deferred: the recorded safe Nightly route exceeded the gate-code budget, and it may be revisited only after a measured refactor fits all controls without weakening the full scheduled/manual workload or conservative unknown-path fallback.
- **D-05:** Recheck TODO-022 against the current ten-job Windows-x64 sample and record a no-reproduction disposition if that sample remains clean. Do not change the test or weaken its signal solely because an old one-off failure was recorded.

### Quality, performance, and public claims
- **D-06:** Score all seven dimensions from TUNE-09 on a 1-5 scale with direct evidence links before choosing the improvement. Make exactly one verified improvement to the lowest-scoring dimension. If scores tie, use the observed user or maintainer impact and explain the tie-break; the improvement is selected after measurement, not in advance.
- **D-07:** Remeasure only the existing quality.json producers on their defined, matching configurations. Update each floor or ceiling only from that producer's exact result; leave a metric unchanged and name the missing evidence when a comparable producer is unavailable. Do not guess or widen a limit to make a run pass.
- **D-08:** Keep low-power device rates explicitly labeled as assumptions until measured on the named hardware. The wasm32 hash lane proves determinism under Wasmtime, not browser speed. Keep measured values separate from estimates in user and maintainer guidance.
- **D-09:** Compatibility claims remain limited to the title, configuration, and predicates actually measured. Inventory counts and protocol/profile tests are not playability evidence; libretro remains experimental until every required capability row passes. Phase 19's one retail title cannot imply broad library support.

### Repository, upgrades, and documentation
- **D-10:** Use scripts/apply-repo-settings.sh --check as the read-only repository-settings audit. Check active runner labels/images and actual tool consumers against current primary upstream release evidence. Keep unrelated pins and caches unchanged absent a specific security, compatibility, or measured-performance reason.
- **D-11:** Reconcile the older DEC.94 wording against the later Phase 7 decision and evidence: do not enable Dependabot auto-merge. Fold TODO-010 only as a conditional evaluation: add a minimal grouped GitHub Actions update configuration only if the new quality score selects upgrade path as the weakest dimension (or selects it after an evidence-based tied-lowest review). Keep TODO-011 and TODO-012 deferred because their auto-merge behavior conflicts with the later Phase 7 decision.
- **D-12:** Refresh README, install, compatibility, hardware, and maintainer guidance only where the audit shows drift. Preserve the concise user README and existing byte budgets. Update PRODUCT-ROADMAP.md and PROMPT-COVERAGE.md from measured outcomes and export only new, applicable lessons with evidence.
- **D-13:** Keep the dependency tree small. Prefer existing project tools or a small, purpose-built copy when they solve the need; add a dependency only when the phase has a concrete requirement the current tools cannot meet.

### the agent's Discretion
- Choose the exact run cohort, report layout, score evidence, tie-break rationale, and single improvement after inspecting current evidence.
- Use current official GitHub documentation and APIs for facts that can change; prefer project-owned run records, producers, and pinned references for behavior and measurements.
- Run cheap selectors, inventory, artifact, and budget checks before any long build or hosted run. Add a permanent preflight only when it prevents a recurring fault already named by a requirement.
- No owner choice is needed before planning: the project's autonomous discussion rules authorize following these evidence-backed recommendations, and no unresolved decision changes scope, authority, risk, cost, or a phase boundary.

### Folded Todos
- **TODO-052 — Evaluate BE32 shards for long libretro host tests:** update the candidate using both long tests and its exact selector, fixture, JUnit, artifact, and hosted timing contract; do not split without a measured improvement.
- **TODO-049 — Review runner and checkpoint repeats:** include the current duplication evidence in the CI report; retain independent deterministic checks unless new evidence supports a safe reduction.
- **TODO-022 — Triage the Windows DLL consumer signal:** review the later Windows-x64 sample and record whether its ten-run no-reproduction condition is met; do not add an unsupported retry or hide the exit status.
- **TODO-010 — Add grouped Dependabot configuration:** evaluate only after the seven-dimension score. Implement the minimal Actions-only config only if upgrade path is selected as weakest; never enable auto-merge.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Scope and settled decisions
- .planning/ROADMAP.md §Phase 17 and milestone constraints — phase goal, success criteria, absolute gate and real-host limits.
- .planning/REQUIREMENTS.md TUNE-07 through TUNE-11 — required outcomes and traceability.
- .planning/seeds/SEED-007-milestone-tune-up.md — recurring tune-up scope and intended evidence.
- .planning/PRODUCT-ROADMAP.md — current playability milestone and prior tune-up evidence.
- .planning/LESSONS.md — CI, quality measurement, source confidence, and GSD lessons; read before planning.
- .planning/AUTONOMOUS.md — standing fan-out, evidence, and decision rules for phase steps.
- AGENTS.md — repository constraints, release policy, local verification, and CI-efficiency rules.
- .planning/preparation/README.md — index to the project reference pack.
- .planning/preparation/DECISIONS.md — settled choices; check the older Dependabot row against the later Phase 7 record before changing it.
- .planning/preparation/ENGINEERING.md §§4, 7-8, 11-12, 14 — CI, performance, quality, docs, recurring maintenance, and low-power limits.
- .planning/preparation/CONFORMANCE.md — compatibility ledger, evidence rules, and CI tiers.
- .planning/preparation/HOSTS-AND-INTEGRATION.md — installation and host-facing claims.

### Prior tune-up evidence and carried backlog
- .planning/milestones/v2.0-phases/07-faster-and-more-trustworthy-ci/07-CONTEXT.md — previous tune-up choices and scope boundaries.
- .planning/milestones/v2.0-phases/07-faster-and-more-trustworthy-ci/07-QUALITY-REVIEW.md — prior seven-dimension scores and tie-break method.
- .planning/milestones/v2.0-phases/07-faster-and-more-trustworthy-ci/07-DELIVERY-EVIDENCE.md — prior disposition, including the no-auto-merge decision.
- .planning/milestones/v2.0-phases/07-faster-and-more-trustworthy-ci/07-BASELINES.md — prior quality producer inputs and bounds.
- .planning/milestones/v2.0-phases/07-faster-and-more-trustworthy-ci/07-CI-EFFICIENCY.md — prior timing comparison and CI review.
- .planning/milestones/v2.0-phases/07-faster-and-more-trustworthy-ci/07-UPGRADE-AUDIT.md — prior settings, Dependabot, runner, and pin evidence.
- .planning/milestones/v2.0-phases/16-patches-cheats-and-compatibility-evidence/16-CONTEXT.md — Phase 16's TODO-052 fold and BE32 evidence.
- .planning/todos/pending/010-dependabot-config.md, 011-dependabot-automerge-workflow.md, and 012-dependabot-gate.md — pending Dependabot scope and earlier bindings.
- .planning/todos/pending/022-windows-dll-crt-consumer-flake.md, 049-review-runner-checkpoint-repeats.md, 051-nightly-pr-job-routing.md, and 052-be32-movie-test-shard.md — reviewed reliability and CI candidates.

### Existing producers and user-facing surfaces
- quality.json — current exact metric bounds and producer provenance.
- scripts/ci-critical-path.sh and .github/workflows/ci.yml — CI timing producer, required job graph, and quality producers.
- tests/cmake/quality.cmake — metric validation and ratchet rules.
- scripts/apply-repo-settings.sh and pins.txt — repository-policy audit and pinned downloads.
- README.md, docs/compatibility.md, docs/maintaining.md, and docs/testing.md — installation, scoped claims, maintenance, and verification guidance.
- tests/compat/ledger.csv — exact title/configuration/predicate evidence behind the generated compatibility report.
- https://docs.github.com/en/code-security/how-tos/secure-your-supply-chain/secure-your-dependencies/configure-version-updates — current Dependabot version-update configuration reference.
- https://docs.github.com/en/code-security/reference/supply-chain-security/dependabot-on-actions — current restrictions for Dependabot-triggered workflows.

No separate phase SPEC exists; the locked requirements and roadmap define the boundary.
</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- scripts/ci-critical-path.sh validates the current exact-attempt jobs and computes the CI critical path from the declared workflow graph.
- tests/cmake/quality.cmake and quality.json already enforce measured floors and ceilings for Cachegrind, Wasm size, serialized state, and fingerprinted CI timing.
- scripts/apply-repo-settings.sh --check reports settings drift without writing repository settings.
- docs/testing.md and docs/maintaining.md already describe cheap preflights, hosted evidence, and how to review CI cost and release behavior.

### Established Patterns
- Measurements are tied to the producer, run, workload fingerprint, and configuration; unmatched runs do not establish a speedup.
- Required platform coverage, test cases, assertions, release checks, and hash artifacts remain intact unless an exact replacement contract proves equivalent coverage.
- Documentation and planning reports record commands, exit status, and run URLs; they do not commit logs or private local paths.
- The project prefers existing tools and small flat dependency trees; new checks must prevent a named recurring fault.

### Integration Points
- Main CI and Release CI use separate required evidence paths; any changed CI route must preserve the required roll-up and exact source/artifact identity.
- quality.json's producers are wired through .github/workflows/ci.yml; local reproduction is preferred when its pinned host/toolchain exists.
- README install instructions connect the released package to RetroArch and the runner; docs/compatibility.md is generated from the evidence ledger and keeps claims scoped.
- The upgrade audit connects repository settings, Dependabot configuration, hosted runner labels, pins.txt, and their actual consumers.

</code_context>

<specifics>
## Specific Ideas

- Treat the current Phase 7 review as precedent, not as a substitute for rescoring current evidence. It selected the README correction when documentation and upgrade path tied at 2, and left Dependabot auto-merge disabled.
- The present quality.json values are measurement starting points, not permission to change a bound without rerunning its defined producer.
- Keep all user-facing playability and installation claims tied to the current docs and required capability matrix. One owned retail title belongs to Phase 19.
- The advisor pass used focused CI, quality/product, and maintainer/security lenses. They found no owner-only decision before planning; the main trade-off is whether new evidence justifies the one improvement or a BE32 split.

</specifics>

<deferred>
## Deferred Ideas

### Reviewed Todos (not folded)
- **TODO-011 and TODO-012 — Dependabot auto-merge workflow and guard:** defer because the later Phase 7 context and evidence explicitly keep update PRs reviewable and do not authorize auto-merge.
- **TODO-051 — Route Nightly pull-request jobs by changed work:** defer until a measured refactor fits the complete required controls inside the existing gate budget; preserve full scheduled/manual runs and the conservative fallback.

### Other deferred ideas
- No unmeasured cache, concurrency increase, broad matrix reduction, dependency, or pin refresh.
- No browser-speed or named-device performance claim without the corresponding run; no broader compatibility claim from inventory counts, fixture protocol tests, or one retail title.
- No Phase 18 or Phase 19 playability implementation in this tune-up.

</deferred>

---

*Phase: 17-a-measured-and-trustworthy-playability-baseline*
*Context gathered: 2026-11-27*
