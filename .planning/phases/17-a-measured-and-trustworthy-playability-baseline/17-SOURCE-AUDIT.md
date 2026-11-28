# Phase 17 source coverage and execution contract

Planning runtime: Codex / OpenGSD 1.23.0. Primary model identifier was not exposed to this agent. Estimate calibration returned factor 1, zero samples and low confidence. No builds, application runs or tests were performed during planning. The plan schema, structure, shell syntax, path and dependency checks are planning checks, not implementation evidence.

Discovery: the phase research and existing project-owned CMake, policy, quality, installer and API patterns provide the required current discovery. Official Dependabot configuration/restrictions pages were opened during planning; no new library/API integration or package installation is selected. Project agent skills are empty. Graphify and security enforcement are disabled by current configuration. No active project skill directory is required by this phase.

## Dependency and ownership map

| Plan | Wave | Needs | Creates / concern | Estimate |
| --- | ---: | --- | --- | ---: |
| 17-01 | 1 | Shipped CMake/inventory/install path | Current skeleton, real install tracer, claim/drift audit | 25 min |
| 17-02 | 2 | 17-01 | Bounded CI cohort, reliability and repeat-work dispositions | 25 min |
| 17-03 | 2 | 17-01 | Existing-producer measurements and exact quality ratchets | 25 min |
| 17-04 | 2 | 17-01 | Read-only settings/runner/pin audit and policy reconciliation | 20 min |
| 17-05 | 3 | 17-02 | Both-test BE32 equivalence and hosted-proof disposition | 25 min |
| 17-06 | 4 | 17-02/03/04/05 | Seven scores and one exact selected-change contract | 20 min |
| 17-07 | 5 | 17-06 | One bounded selected improvement and its proof | 25 min |
| 17-08 | 6 | 17-07 | Only demonstrated public-doc/hardware/compatibility freshness | 25 min |
| 17-09 | 7 | 17-08 | Direction, lessons and complete review/verification handoff | 25 min |

Wave 2 source files, reports and todo ownership are disjoint. Each worktree uses its own task-named scratch/build resources; hosted dispatch is coordinated by the parent to avoid duplicated suites. Plan 03 deliberately does not consume the concurrent Plan 02 report; Plan 09 reconciles its timing row later. Inventory/header changes are serial: Plan 01 establishes their existing exact skeleton, and any chosen Plan 07 registration change happens later. No plan writes STATE.md; the orchestrator records the runtime/model and progress.

Plan 01 reuses the already-complete CMake skeleton and per-directory inventories. Correct registrations/rows stay unchanged; the first task records their exact active identities and exercises the existing installer end to end. Only a reproduced wiring fault warrants an inventory modification. A new unused test scaffold or prose-report validator would consume budget without preventing a named fault.

Plan 07 has an explicit conditional file union, approved by the orchestrator to preserve D-06's measurement-before-selection rule and a fixed executable plan inventory. Its implementation must touch at most five exact selected files, from only one candidate branch. Unselected branches stay untouched. This is the sole task-file-count exception: the declaration lists plausible ownership, not simultaneous implementation. No task creates/edits a downstream PLAN during execution. The score contract records a real blocker if no bounded candidate can improve the lowest-scoring dimension; it must not inflate a score, choose the next-best dimension or call the audit itself an improvement.

Hosted workflows may outlast an individual local plan. Estimates bound active local work; required long hosted runs are asynchronous under the orchestrator's existing phase verification, with frequent status updates. Their evidence is never represented as passed before it exists. No new timeout or campaign resource limit is inferred from these estimates.

## Multi-source audit

| SOURCE | ID / item | Plan | Status | Contract |
| --- | --- | --- | --- | --- |
| GOAL | Current install guidance, scoped compatibility and measured limits before playability | 01–09 | COVERED | Tracer/audits → score/change → docs/direction → review/verification |
| GOAL | Success criterion 1: bounded linked comparable CI evidence | 02, 05, 09 | COVERED | Exact workflow/matrix/workload groups and exclusions |
| GOAL | Success criterion 2: traceable timeout and flaky-check decisions | 02, 07, 09 | COVERED | Doubled slowest comparable job with source run; retained gaps |
| GOAL | Success criterion 3: one verified lowest-dimension improvement | 06, 07, 09 | COVERED | All seven scores before selection; explicit blocked contract if unavailable |
| GOAL | Success criterion 4: measured bounds and device assumptions | 03, 08, 09 | COVERED | Exact producer units/configurations, no widening to pass |
| GOAL | Success criterion 5: maintenance audit, guidance, roadmap and lessons | 01, 04, 08, 09 | COVERED | Read-only live policy and actual consumers; source-scoped edits |
| REQ | TUNE-07 | 02, 05, 09 | COVERED | Bounded exact-attempt cohort, critical path/duplicates/cores/flake signals |
| REQ | TUNE-08 | 02, 05, 07, 09 | COVERED | Job/source/failure evidence and conditional exact timeout change |
| REQ | TUNE-09 | 01, 06, 07, 09 | COVERED | Evidence-first score, tie-break and one verified outcome |
| REQ | TUNE-10 | 03, 08, 09 | COVERED | All existing five producers, exact ratchets and unknown device rates |
| REQ | TUNE-11 | 01, 04, 06, 07, 08, 09 | COVERED | Settings, update policy, active images/pins, guidance/direction/lessons |
| RESEARCH | Responsibility map: hosted APIs → reports; existing producers → registry; ledger → claims | 01–09 | COVERED | No application/core-layer reassignment |
| RESEARCH | At most forty recent run identities with complete exact-attempt pagination | 02 | COVERED | Repository-wide cap; incomplete payloads cannot become comparable evidence |
| RESEARCH | Fingerprint workflow/event/route/matrix/tool/fixture/test/runner configuration | 02, 05, 09 | COVERED | PR merge identity and historical DAG limitations retained |
| RESEARCH | Script accepts only full uncalled PR context | 02, 09 | COVERED | No forced main/release invocation or false push-to-PR baseline |
| RESEARCH | Queue, occupancy, capacity, raw runner time and billing are distinct | 02, 05 | COVERED | Unavailable quantities explicit; no invented utilization/cost savings |
| RESEARCH | Existing Cachegrind windows/relative limit, linked Release WASI bytes and complete state bytes | 03, 09 | COVERED | No surrogate metrics or incompatible local toolchain results |
| RESEARCH | Current producer/toolchain unavailable locally | 03, 07, 09 | COVERED | Existing hosted producer; retain bound and show pending evidence |
| RESEARCH | Old quality figures/links in public guidance require evidence-scoped refresh | 01, 08, 09 | COVERED | Canonical links preferred to stale duplicate values |
| RESEARCH | Low-power rate assumptions; Wasmtime determinism is not browser speed | 03, 08, 09 | COVERED | Named-device evidence required for a measured rate |
| RESEARCH | Seven current scores; Phase 7 precedent is not default selection | 06, 07 | COVERED | One minimum/tied-minimum selection from observed impact |
| RESEARCH | Exact title/configuration/predicate claims and required experimental matrix | 01, 06, 08, 09 | COVERED | No new playability data or metadata promotion |
| RESEARCH | Current runner primary sources and actual pin consumers | 04 | COVERED | No retired-image introduction or stale wasi-sdk consumer inference |
| RESEARCH | Dependabot restrictions and later no-auto-merge policy | 04, 06, 07 | COVERED | Conditional Actions-only grouped configuration |
| RESEARCH | Ten-job Windows DLL evidence still needs collecting | 02 | COVERED | Ten eligible jobs within bounded cohort or pending sample shortfall |
| RESEARCH | Both long BE32 tests, exact selector/JUnit/fixture/hash and hosted critical path | 05 | COVERED | Candidate evaluated; production remains intact absent every proof |
| RESEARCH | Current full cohort/producer values are open execution evidence questions | 02, 03, 09 | COVERED | No invented measurements and no owner decision manufactured |
| RESEARCH | Existing tools suffice; no package installation | 01–09 | COVERED | Concrete unmet need required before scope revision |
| CONTEXT | D-01 bounded exact comparisons | 02, 05, 09 | COVERED | At most forty run identities; exclusions explicit |
| CONTEXT | D-02 doubled slowest comparable job timeout; flake cause/stronger signal | 02, 07, 09 | COVERED | Integer seconds and ceiling to minutes, source run adjacent |
| CONTEXT | D-03 both-test BE32 full equivalence and hosted gain | 05 | COVERED | Current gates retained without exact complete proof |
| CONTEXT | D-04 repeat-work audit; deferred Nightly route | 02, 07 | COVERED | Only proved equivalent removal; no Nightly implementation |
| CONTEXT | D-05 current ten-job DLL condition | 02 | COVERED | Clean ten-job sample or explicitly pending |
| CONTEXT | D-06 seven scores, observed-impact tie-break, exactly one improvement | 06, 07, 09 | COVERED | No preselection, relabeling or substitution |
| CONTEXT | D-07 existing producers and exact measured bounds | 03, 09 | COVERED | Missing comparison retains bound; out-of-bound result investigated |
| CONTEXT | D-08 device assumptions and Wasmtime/browser distinction | 03, 08, 09 | COVERED | Claims preserve measurement units and limits |
| CONTEXT | D-09 narrow compatibility and required experimental matrix | 01, 06, 08, 09 | COVERED | No integrated-playability inference from protocol/inventory |
| CONTEXT | D-10 read-only policy audit and current actual consumers | 04 | COVERED | No unrelated pin/cache changes |
| CONTEXT | D-11 later no-auto-merge policy; conditional TODO-010 | 04, 06, 07 | COVERED | TODO-011/012 remain deferred |
| CONTEXT | D-12 demonstrated guidance drift; direction/prompt and applicable lessons | 01, 04, 08, 09 | COVERED | Concise README and existing budgets |
| CONTEXT | D-13 small flat dependency tree | 01, 04, 07 | COVERED | Existing tools and no package installs |

Exclusions, not gaps: Phase 18/19 implementation, broader compatibility inference, owner hardware/device benchmarking, browser-speed claims, unmeasured cache/concurrency changes, unrelated pin refreshes, TODO-011/012 auto-merge and TODO-051 routing. TODO-052 is a measured review with a retain-current-gates default, not authorization for an unproven split. This audit never converts absence of measurements into a speedup, quality success or low-power device result.

## Flagged assumptions: exact unresolved edge probe

The canonical serialized planning input is embedded below. All seven input items retain unresolved status, null verification/resolution/reason, and the exact original prompt; none is auto-resolved or dismissed. It is durable input to the downstream verifier, not an execution log.

| Requirement | Category | Unresolved flag / practical handling |
| --- | --- | --- |
| TUNE-07 | unclassified | unclassified — review manually; review the actual report/comparability claims during phase verification |
| TUNE-08 | empty | Empty/single/null generic data prompt remains unresolved; the concrete absence-of-comparable-evidence policy comes from D-02, not a new parser product requirement |
| TUNE-08 | encoding | Byte/code-point/grapheme/normalization prompt remains unresolved; no text-length API is invented from it |
| TUNE-09 | unclassified | unclassified — review manually; verify the score/tie-break/one-improvement contract against live outcomes |
| TUNE-10 | boundary | Boundary/one-step-outside prompt remains unresolved; existing producer comparator controls are reused without declaring the generic probe resolved |
| TUNE-10 | precision | Exact rounding/overflow prompt remains unresolved; timeout ceiling division and integer producer values follow their explicit source contracts |
| TUNE-11 | unclassified | unclassified — review manually; inspect maintenance/docs/direction/lessons coverage rather than infer a generic input shape |

No SPEC exists. These flags must remain visible in execution and verification handoffs. They are not owner UAT or newly invented product requirements.

## Spec-less prohibition recall and precision

Applied the recall question separately to TUNE-07 through TUNE-11: what could the feature silently become that the owner would reject but the source does not forbid? Considered misleading speed/cost claims, failure masking, score manipulation, unsafe scope expansion, undisclosed estimates, auto-merge and unrelated upgrades, as well as routine input/arithmetic/file-handling concerns.

Precision outcome: no additional bespoke prohibition is retained. The meaningful product/transparency constraints are already explicit D-01 through D-13 and therefore covered above; routine parser, arithmetic, link and cleanup correctness belongs to existing controls and review, not newly minted product intent. Generic security/secret-handling concerns are canon referred to the existing hygiene/security review, without minting a new prohibition. The empty items list and zero coverage in the serialized input below are intentional. Each plan carries must_haves.prohibitions: [] using the flat projection contract; no nested check descriptor, unsupported test-tier claim or fabricated fail-first evidence is emitted.

## Verification and scope notes

- Existing evidence commands are grounded in current tracked paths; intended new report/contract paths are named explicitly. No absolute checkout path is embedded in task commands.
- Every automated element is immediately followed by a concrete fails_when direction. Count/text-grep gates are not authored; existing project controls own budgets, inventory and claims.
- Current per-phase gate ceiling remains 4,694 counted lines and RetroArch remains 300; no guessed adjustment. Ship ratchets the actual measured shipping count.
- Plan 07's actual chosen code behavior requires a failing concrete regression followed by a passing check registered in existing CI. Configuration/documentation-only branches use appropriate existing guards and one-off agent evidence.
- Audit artifacts can be complete with explicit unsupported-evidence dispositions where locked decisions allow retaining existing state. Missing required remeasurement or selected-improvement proof stays pending and cannot be disguised as the phase's successful completion.
- Code review/fixes precede final phase verification. Required builds/tests/hosted checks remain required; no build/test ran during this planning step.
- No manual approval checkpoint is introduced for a settled reversible choice. Real unresolved selected-change scope or identity-bound authority remains a concrete blocker for the orchestrator, not a ritual acknowledgment.

## Serialized probe input

```json
{"edge_probe":{"items":[{"requirement_id":"TUNE-07","category":"unclassified","status":"unresolved","verification":null,"resolution":null,"reason":null,"probe":"unclassified — review manually"},{"requirement_id":"TUNE-08","category":"empty","status":"unresolved","verification":null,"resolution":null,"reason":null,"probe":"What is the result for empty, single-element, or null input?"},{"requirement_id":"TUNE-08","category":"encoding","status":"unresolved","verification":null,"resolution":null,"reason":null,"probe":"Whose definition of length/equality applies — bytes, code points, grapheme clusters, or normalized form?"},{"requirement_id":"TUNE-09","category":"unclassified","status":"unresolved","verification":null,"resolution":null,"reason":null,"probe":"unclassified — review manually"},{"requirement_id":"TUNE-10","category":"boundary","status":"unresolved","verification":null,"resolution":null,"reason":null,"probe":"What happens exactly at each min/max/threshold — and one step either side?"},{"requirement_id":"TUNE-10","category":"precision","status":"unresolved","verification":null,"resolution":null,"reason":null,"probe":"Where can precision loss, overflow, or rounding/tie-breaking occur — and what is the exact contract (e.g. half-up vs half-to-even, ceil/floor/truncate)?"},{"requirement_id":"TUNE-11","category":"unclassified","status":"unresolved","verification":null,"resolution":null,"reason":null,"probe":"unclassified — review manually"}],"coverage":{"applicable":7,"resolved":0,"unresolved":7,"unclassified":3,"byVerification":{"explicit":0,"backstop":0}}},"prohibition_probe":{"items":[],"coverage":{"applicable":0,"resolved":0,"unresolved":0,"byVerification":{"test":0,"judgment":0}}}}
```
