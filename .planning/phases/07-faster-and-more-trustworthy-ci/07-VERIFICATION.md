---
phase: 07-faster-and-more-trustworthy-ci
verified: 2026-11-19T04:06:07Z
status: passed
score: 5/5 pre-ship implementation truths verified
covered_files:
  - .github/workflows/ci.yml
  - .github/workflows/nightly.yml
  - .github/workflows/release.yml
  - .github/workflows/triage.yml
  - .planning/phases/07-faster-and-more-trustworthy-ci/07-01-PLAN.md
  - .planning/phases/07-faster-and-more-trustworthy-ci/07-01-SUMMARY.md
  - .planning/phases/07-faster-and-more-trustworthy-ci/07-02-PLAN.md
  - .planning/phases/07-faster-and-more-trustworthy-ci/07-02-SUMMARY.md
  - .planning/phases/07-faster-and-more-trustworthy-ci/07-03-PLAN.md
  - .planning/phases/07-faster-and-more-trustworthy-ci/07-03-SUMMARY.md
  - .planning/phases/07-faster-and-more-trustworthy-ci/07-04-PLAN.md
  - .planning/phases/07-faster-and-more-trustworthy-ci/07-04-SUMMARY.md
  - .planning/phases/07-faster-and-more-trustworthy-ci/07-05-PLAN.md
  - .planning/phases/07-faster-and-more-trustworthy-ci/07-05-SUMMARY.md
  - .planning/phases/07-faster-and-more-trustworthy-ci/07-06-PLAN.md
  - .planning/phases/07-faster-and-more-trustworthy-ci/07-06-SUMMARY.md
  - .planning/phases/07-faster-and-more-trustworthy-ci/07-07-PLAN.md
  - .planning/phases/07-faster-and-more-trustworthy-ci/07-07-SUMMARY.md
  - .planning/phases/07-faster-and-more-trustworthy-ci/07-08-PLAN.md
  - .planning/phases/07-faster-and-more-trustworthy-ci/07-08-SUMMARY.md
  - .planning/phases/07-faster-and-more-trustworthy-ci/07-09-PLAN.md
  - .planning/phases/07-faster-and-more-trustworthy-ci/07-09-SUMMARY.md
  - .planning/phases/07-faster-and-more-trustworthy-ci/07-10-PLAN.md
  - .planning/phases/07-faster-and-more-trustworthy-ci/07-10-SUMMARY.md
  - .planning/phases/07-faster-and-more-trustworthy-ci/07-BASELINES.md
  - .planning/phases/07-faster-and-more-trustworthy-ci/07-CI-EFFICIENCY.md
  - .planning/phases/07-faster-and-more-trustworthy-ci/07-CONTEXT.md
  - .planning/phases/07-faster-and-more-trustworthy-ci/07-DELIVERY-EVIDENCE.md
  - .planning/phases/07-faster-and-more-trustworthy-ci/07-QUALITY-REVIEW.md
  - .planning/phases/07-faster-and-more-trustworthy-ci/07-RESEARCH.md
  - .planning/phases/07-faster-and-more-trustworthy-ci/07-REVIEW.md
  - .planning/phases/07-faster-and-more-trustworthy-ci/07-UPGRADE-AUDIT.md
  - .planning/preparation/ARCHITECTURE.md
  - .planning/preparation/ENGINEERING.md
  - .planning/preparation/PROMPT-COVERAGE.md
  - README.md
  - docs/maintaining.md
  - docs/testing.md
  - quality.json
  - scripts/ci-critical-path.sh
  - scripts/classify-changes.sh
  - scripts/triage-report.sh
  - tests/cmake/ci_route_controls.cmake
  - tests/cmake/lessons_unique.cmake
  - tests/cmake/quality.cmake
  - tests/cmake/quality_controls.cmake
  - tests/cmake/triage_controls.cmake
  - tests/core/test_state.c
  - tests/hygiene/tests.cmake
  - tests/policy/inventory.txt
  - tests/policy/tests.cmake
covered_digest: "v4:sha256:0bc6682b5185a9ea9b7e3f6c5f555344c37d98aa3e9b00b0b5cc6afd7f39fb19"
behavior_unverified: 0
overrides_applied: 0
decision_coverage:
  honored: 0
  total: 0
  not_honored: []
human_verification: []
ship_evidence_pending:
  - "Dispatch default-branch triage; record the run URL and inspect the uploaded seven-day artifact."
  - "Run CI on the exact final PR head; record comparable timing and the generated release-App comparison with required-check parity."
  - "Measure the linked Release WASI module on Ubuntu 26 and set a bound only from that byte count."
  - "After merge, record green main CI, the automatic Release workflow, published release, and install-line check."
---

# Phase 7: Faster and more trustworthy CI — Verification

**Phase Goal:** Maintainers can see where CI time goes, find work that needs attention, and verify that the pipeline and published guidance meet measured quality targets.

**Verified:** 2026-11-19T04:06:07Z
**Status:** passed
**Re-verification:** No — initial phase verification

## Goal Achievement

### Observable Truths

| # | Roadmap truth | Status | Evidence |
|---|---|---|---|
| 1 | The weekly workflow produces a sanitized triage artifact for open issues and current run status, with read-only project access and no messages. | ✓ VERIFIED | The scheduled/manual workflow grants only read permissions, calls the production collector, and uploads only its sanitized JSON output. `ci.triage` exercises the collector's production transform, privacy/schema rules, and API-failure behavior. This verifies the pre-ship implementation and data path; hosted upload/download remains pending ship evidence and has not run. |
| 2 | The CI report compares up to 40 runs, identifies the measured path, duplication, core use and caching, ties timeouts to matching runs, and makes no unsupported speed claim. | ✓ VERIFIED | The report records 40 run rows, 573 job records, 25 comparable completed runs across 20 workload groups, critical paths, duplicated work, core/cache evidence, flaky-check dispositions, timeout arithmetic and run links. It marks the release-App after comparison pending and claims no speedup. Workflow timeout controls passed. |
| 3 | The seven-dimension review selects the weakest dimension and verifies one bounded improvement. | ✓ VERIFIED | The review scores all seven dimensions before selection, explains the documentation/README tie-break, and records the single README correction. The obsolete claim is absent; the README aligns with the runner guide and fixture-scoped compatibility ledger. |
| 4 | The quality registry and CI cover the required metrics, enforce measured applicable bounds, and represent unmeasured or incomparable values without inventing a bound. | ✓ VERIFIED | `quality.json` records measured Cachegrind, state-byte, and historical push-timing bounds; the production comparator and focused controls pass. The linked WASI producer validates and measures the actual module when run, while `size.wasm` stays explicitly unbounded until its supported Ubuntu 26 measurement exists. The current PR route reports unbaselined until the exact hosted attempt establishes comparability. These are the plan's ship measurement boundary, not claims that a bound or hosted result already exists. Lesson uniqueness rejects duplicate IDs and normalized text. |
| 5 | Maintainers can trace settings, pins, runner and device/documentation budgets, install guidance, roadmap and prompt coverage to evidence. | ✓ VERIFIED | The upgrade audit, metric baselines, maintainer/testing docs, prompt coverage, product roadmap and README provide the evidence trail. Focused pin, timeout, 3,900-line repository-budget and 300-line RetroArch-budget checks passed. |

**Score:** 5/5 pre-ship implementation truths verified (0 behavior-unverified). Hosted outcomes remain pending as listed under Ship Evidence Pending.

### Required Artifacts

All plan-frontmatter artifact checks passed: 27/27 artifacts across Plans 07-01 through 07-10 exist and are substantive.

| Artifact | Expected | Status | Details |
|---|---|---|---|
| .github/workflows/triage.yml and scripts/triage-report.sh | Scheduled/manual workflow and bounded sanitizer | ✓ VERIFIED | Workflow wiring and permissions are present; ci.triage exercises the real transform and failure behavior. |
| .planning/phases/07-faster-and-more-trustworthy-ci/07-CI-EFFICIENCY.md | Matched-run CI report | ✓ VERIFIED | Contains captured runs/jobs, comparable cohorts, DAG results, timeout evidence and explicit unknowns. |
| quality.json, tests/cmake/quality.cmake, .github/workflows/ci.yml | Registry, comparator and measurement producers | ✓ VERIFIED; SHIP MEASUREMENTS PENDING | Comparator and producers are wired and fixture-tested. Explicit unbounded/unbaselined states prevent guessed values; actual linked WASI bytes and exact-head PR timing remain ship evidence. |
| .planning/phases/07-faster-and-more-trustworthy-ci/07-QUALITY-REVIEW.md | Seven scores and selected improvement | ✓ VERIFIED | Contains all dimensions, evidence, selection rationale and verified README correction. |
| docs/maintaining.md, docs/testing.md, README.md | Current maintainer and user guidance | ✓ VERIFIED | The README no longer makes the false BIOS/disc claim; detailed operational guidance stays in docs. |

**Artifacts:** 27/27 plan artifacts passed existence/substance checks.

### Key Link Verification

The generic path-string key-link query returned lexical false negatives for several indirect CMake and documentation links. Manual traces confirmed the production connections below; the Plan 04 CI-analysis link was conditional on choosing a CI improvement and does not apply because the evidence-selected change was the README correction.

| From | To | Via | Status | Details |
|---|---|---|---|---|
| tests/policy/tests.cmake | tests/cmake/triage_controls.cmake → scripts/triage-report.sh | ci.triage registered CTest control | ✓ WIRED | The control invokes the production script with successful and malformed/API-failure fixtures. |
| tests/policy/tests.cmake | tests/cmake/quality_controls.cmake → tests/cmake/quality.cmake → quality.json | policy.quality_metrics and QUALITY_FILE | ✓ WIRED | The helper executes production measurement blocks and comparator calls; matching, over-bound and unbaselined cases remain. |
| tests/policy/tests.cmake | tests/cmake/lessons_unique.cmake | policy.lessons_unique | ✓ WIRED | The registered test checks the real ledger; the self-test rejects duplicate IDs and normalized text. |
| .github/workflows/ci.yml | scripts/ci-critical-path.sh | Current-attempt jobs API and PR head SHA | ✓ WIRED | Required CI reads the paginated attempt endpoint and passes PR head separately from merge SHA; release-input route is fingerprinted. |
| .github/workflows/ci.yml | tests/cmake/quality.cmake | Cachegrind, state-byte and WASI measurement blocks | ✓ WIRED | The workflow connects measured outputs to the shared comparator; WASI remains unbounded until measured. |
| tests/hygiene/tests.cmake | tests/cmake/line_budget.cmake | Repository/E2E budget selectors | ✓ WIRED | Focused CTest controls enforce 3,900 and 300 lines with over-limit negative controls. |
| 07-BASELINES.md | quality.json | Measured bounds and source evidence | ✓ WIRED | Baseline instructions identify the registry and withhold bounds where evidence is absent. |
| 07-DELIVERY-EVIDENCE.md | 07-CI-EFFICIENCY.md | Before/after and ship evidence locator | ✓ WIRED | The delivery matrix links to the report and lists release-App comparison pending. |

### Data-Flow Trace

| Artifact | Data variable | Source | Produces real data | Status |
|---|---|---|---|---|
| scripts/triage-report.sh | issue and workflow records | GitHub Issues and Actions APIs via gh api | Yes; response schemas are checked and scrubbed before atomic publish | ✓ FLOWING; hosted artifact pending |
| .github/workflows/ci.yml | instruction totals | Cachegrind output from two registered workloads | Yes; comparator consumes parsed integer counts | ✓ FLOWING |
| .github/workflows/ci.yml | serialized state bytes | state.stable_bytes output from the C test binary | Yes; exactly one positive integer is required | ✓ FLOWING |
| .github/workflows/ci.yml | WASI module bytes | Linked build/wasm32/schwaystation-run file | Yes; the producer validates the module and reads its byte count when run | ✓ FLOWING; hosted byte count and bound pending |
| .github/workflows/ci.yml | CI path duration | Current attempt job timestamps and runner labels | Yes; merge SHA and PR head SHA are checked independently | ✓ FLOWING; exact-head hosted measurement pending; current route explicitly unbaselined |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|---|---|---|---|
| Classifier, triage transform and lesson identity/text checks | ctest --preset ci --no-tests=error -R '^(ci[.]classify_policy|ci[.]triage|policy[.]lessons_unique|policy[.]lessons_unique[.]self_test)$' --output-on-failure --parallel 1 | Exit 0; 4/4 passed in 0.51s | ✓ PASS |
| Production quality measurement/control path | ctest --preset ci --no-tests=error -R '^policy[.]quality_metrics$' --output-on-failure --parallel 1 | Exit 0; 1/1 passed in 37.22s after review fixes | ✓ PASS |
| No-skipped-tests and failure-direction control | ctest --preset ci --no-tests=error -R '^policy[.]no_skip([.]self_test)?$' --output-on-failure --parallel 1 | Exit 0; 2/2 passed in 0.22s | ✓ PASS |
| Budget, pin and timeout controls after refreshing stale generated config | cmake --preset hygiene, then focused CTest for gate budget, E2E budget, action pins, and timeout comments | Configure exit 0; CTest exit 0; 4/4 passed in 0.03s | ✓ PASS |

These were targeted checks, not a broad suite. An initial hygiene run used stale generated configuration with a 3,890-line registration and failed immediately. After cmake --preset hygiene refreshed it from the tracked 3,900-line limit, the same four selectors passed.

### Probe Execution

No probe scripts are declared in the Phase 7 plans or summaries.

### Requirements Coverage

| Requirement | Status | Evidence |
|---|---|---|
| TUNE-01 | Implementation verified; hosted artifact pending at ship | Workflow wiring, read-only scopes, production collector, and privacy/schema/failure controls pass; hosted upload/download is not claimed. |
| TUNE-02 | Verified; exact-head release-App comparison pending at ship | Historical report is complete and makes no unsupported speed claim; post-change comparison and required-check parity remain pending. |
| TUNE-03 | Verified; release-App comparison pending at ship | Sampled timeout formulas and flaky-signal dispositions are documented with run IDs; post-change release-App comparison remains pending. |
| TUNE-04 | Satisfied | Seven-score review and one README correction are verified. |
| TUNE-05 | Pipeline verified; hosted measurements pending at ship | Cachegrind/state bounds, duplicate checks, the WASI producer, and exact-head timing route are wired and locally controlled. WASI size remains unbounded and current PR timing unbaselined until matching hosted evidence is available. |
| TUNE-06 | Satisfied | Settings/pins/runner audits, documentation, and 3,900/300 line checks are verified. |

### Test Quality Audit

| Test File | Linked Req | Active checks | Skipped | Circular | Assertion level | Verdict |
|---|---|---:|---:|---|---|---|
| tests/cmake/triage_controls.cmake | TUNE-01 | 1 registered; passed | 0 | No | Behavioral schema, scrub and failure-output assertions | PASS |
| tests/cmake/ci_route_controls.cmake | TUNE-02/03 | 1 registered; passed | 0 | No | Production classifier routes and exact expected outputs | PASS |
| tests/cmake/quality_controls.cmake | TUNE-05 | 1 registered; passed | 0 | No | Production blocks, exact fixture bounds, missing/over-bound and route assertions | PASS |
| tests/cmake/lessons_unique.cmake | TUNE-05 | 2 registered; passed | 0 | No | Real ledger plus known duplicate-ID/text failures | PASS |
| tests/hygiene/tests.cmake | TUNE-05/06 | 4 selected; passed | 0 | No | Exact cap boundaries and timeout/pin checks with negative controls | PASS |

Disabled tests on requirements: 0
Circular patterns detected: 0
Insufficient assertions: 0

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|---|---:|---|---|---|
| scripts/triage-report.sh:22,42 | Temporary-file templates contain XXXXXX, the six-character secure mktemp suffix, not an XXX debt marker. | Info | No phase debt marker. |
| docs/testing.md:69 | The word placeholder describes CD-ROM-only test registrations, not an unimplemented Phase 7 artifact. | Info | No phase stub or missing implementation. |

No unreferenced TBD, FIXME or XXX debt marker was found in changed implementation files. The no-skip scan and policy.no_skip controls passed.

### Human Verification Required

N/A — infrastructure/CI phase with no user-facing flow and no behavior-unverified invariants. No owner UAT is needed.

### Hosted Ship Evidence Pending (Not Run)

These are machine-checkable ship results, not human verification. They remain pending and none are claimed as passed:

1. Dispatch the weekly triage workflow on the default branch; record its successful run URL and inspect the uploaded seven-day artifact.
2. Confirm CI required on the exact final PR head, record comparable timing, and compare the generated release-App PR with its baseline and required-check set.
3. Measure the actual linked Release WASI module on Ubuntu 26 and add a bound only from the observed byte count.
4. After merge, confirm green main CI, the automatic Release workflow, the published release, and the install-line check.

### Gaps Summary

No pre-ship source, wiring, or local test gap remains. Phase verification is passed at the implementation and data-flow level; hosted artifact delivery, the actual WASI byte count, exact-head timing, release-App comparison, and post-merge release checks remain explicit ship evidence. Do not infer that any hosted outcome ran. Keep Phase 8 stopped until ship evidence is recorded and Phase 7 is released.

---

**Verification approach:** Goal-backward from ROADMAP success criteria, cross-checked with all 10 plans and their artifact links.
**Must-haves source:** Phase 7 ROADMAP success criteria plus Plans 07-01 through 07-10.
**Automated checks:** 13 focused CTest checks passed after targeted reconfiguration; all 10 plan artifact queries passed (27/27 artifacts).
**Human checks required:** None; this is an infrastructure phase. Hosted ship evidence remains pending.
**Total verification time:** approximately 12 minutes of verification and focused checks.

---
Verified: 2026-11-19T04:06:07Z
