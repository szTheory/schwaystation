---
phase: 07-faster-and-more-trustworthy-ci
verified: 2026-11-19T08:50:20Z
status: passed
score: 5/5 implementation truths verified; ship evidence complete
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
covered_digest: "v4:sha256:f7630d4314d410c0b6eabcaf3e19cb314242df942f2003be2572f49b3185ea59"
behavior_unverified: 0
overrides_applied: 0
decision_coverage:
  honored: 0
  total: 0
  not_honored: []
human_verification: []
ship_evidence_pending: []
---

# Phase 7: Faster and more trustworthy CI — Verification

**Phase Goal:** Maintainers can see where CI time goes, find work that needs attention, and verify that the pipeline and published guidance meet measured quality targets.

**Verified:** 2026-11-19T08:50:20Z
**Status:** passed
**Verification refresh:** Re-run after ship closeout changed the final plan summary and code review; source/test files and hosted run results are unchanged.

## Goal Achievement

### Observable Truths

| # | Roadmap truth | Status | Evidence |
|---|---|---|---|
| 1 | The weekly workflow produces a sanitized triage artifact for open issues and current run status, with read-only project access and no messages. | ✓ VERIFIED | The scheduled/manual workflow grants only read permissions, calls the production collector, and uploads only its sanitized JSON output. `ci.triage` exercises the production transform, privacy/schema rules, and API-failure behavior. Default-branch run 44486437761 uploaded an artifact that passed schema validation, reported zero open issues without truncation, and contained no untrusted issue body or log text. |
| 2 | The CI report compares up to 40 runs, identifies the measured path, duplication, core use and caching, ties timeouts to matching runs, and makes no unsupported speed claim. | ✓ VERIFIED | The report records 40 run rows, 573 job records, 25 comparable completed runs across 20 workload groups, critical paths, duplicated work, core/cache evidence, flaky-check dispositions, timeout arithmetic and run links. Release-App PR #29 retained `CI required` with the observed metadata-only route; because the baseline merge SHA is unavailable, the report makes no exact-fingerprint or speedup claim. |
| 3 | The seven-dimension review selects the weakest dimension and verifies one bounded improvement. | ✓ VERIFIED | The review scores all seven dimensions before selection, explains the documentation/README tie-break, and records the single README correction. The obsolete claim is absent; the README aligns with the runner guide and fixture-scoped compatibility ledger. |
| 4 | The quality registry and CI cover the required metrics, enforce measured applicable bounds, and represent unmeasured or incomparable values without inventing a bound. | ✓ VERIFIED | `quality.json` records measured Cachegrind, state-byte, WASI-size and historical push-timing bounds; the production comparator and focused controls pass. Ubuntu 26 measured the linked module at 595,820 bytes and final-head CI exercised that bound. The PR route remains explicitly unbaselined against the different push/main fingerprint. Lesson uniqueness rejects duplicate IDs and normalized text. |
| 5 | Maintainers can trace settings, pins, runner and device/documentation budgets, install guidance, roadmap and prompt coverage to evidence. | ✓ VERIFIED | The upgrade audit, metric baselines, maintainer/testing docs, prompt coverage, product roadmap and README provide the evidence trail. Focused pin, timeout, 3,901-line repository-budget and 300-line RetroArch-budget checks passed; the v0.7.0 release passed `install-line-latest`. |

**Score:** 5/5 implementation truths verified (0 behavior-unverified); all ship evidence is recorded below.

### Required Artifacts

All plan-frontmatter artifact checks passed: 27/27 artifacts across Plans 07-01 through 07-10 exist and are substantive.

| Artifact | Expected | Status | Details |
|---|---|---|---|
| .github/workflows/triage.yml and scripts/triage-report.sh | Scheduled/manual workflow and bounded sanitizer | ✓ VERIFIED | Workflow wiring and permissions are present; ci.triage exercises the real transform and failure behavior. |
| .planning/phases/07-faster-and-more-trustworthy-ci/07-CI-EFFICIENCY.md | Matched-run CI report | ✓ VERIFIED | Contains captured runs/jobs, comparable cohorts, DAG results, timeout evidence and explicit unknowns. |
| quality.json, tests/cmake/quality.cmake, .github/workflows/ci.yml | Registry, comparator and measurement producers | ✓ VERIFIED | Comparator and producers are wired and fixture-tested. Ubuntu 26 measured the linked module at 595,820 bytes and final-head CI passed the checked-in bound; the incomparable PR timing route remains explicitly unbaselined. |
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
| 07-DELIVERY-EVIDENCE.md | 07-CI-EFFICIENCY.md | Before/after and ship evidence locator | ✓ WIRED | The delivery record links the observed PR #29 route, required-check parity, run URL and the missing-baseline-SHA limitation. |

### Data-Flow Trace

| Artifact | Data variable | Source | Produces real data | Status |
|---|---|---|---|---|
| scripts/triage-report.sh | issue and workflow records | GitHub Issues and Actions APIs via gh api | Yes; response schemas are checked and scrubbed before atomic publish | ✓ FLOWING; uploaded artifact inspected in run 44486437761 |
| .github/workflows/ci.yml | instruction totals | Cachegrind output from two registered workloads | Yes; comparator consumes parsed integer counts | ✓ FLOWING |
| .github/workflows/ci.yml | serialized state bytes | state.stable_bytes output from the C test binary | Yes; exactly one positive integer is required | ✓ FLOWING |
| .github/workflows/ci.yml | WASI module bytes | Linked build/wasm32/schwaystation-run file | Yes; the producer validates the module and reads its byte count when run | ✓ FLOWING; 595,820-byte bound passed on final PR head |
| .github/workflows/ci.yml | CI path duration | Current attempt job timestamps and runner labels | Yes; merge SHA and PR head SHA are checked independently | ✓ FLOWING; hosted release-App route passed; exact historical fingerprint comparison is unavailable and remains unbaselined |

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
| TUNE-01 | Satisfied | Workflow wiring and read-only scopes pass; default-branch run 44486437761 uploaded a schema-valid artifact with zero open issues and no truncation. |
| TUNE-02 | Satisfied | The report records PR #29's exact three-file path list, passing `CI required`, observed route and run 44476990492; it documents the missing baseline SHA and claims no unsupported speedup. |
| TUNE-03 | Satisfied | Sampled timeout formulas and flaky-signal dispositions cite runs; Windows ARM64 completed the quality control in 116.03s under its 180s cap. |
| TUNE-04 | Satisfied | Seven-score review and one README correction are verified. |
| TUNE-05 | Satisfied | Cachegrind/state bounds and duplicate checks pass; Ubuntu 26 measured 595,820 WASI bytes and final-head CI passed the bound. PR timing stays explicitly unbaselined against the push/main fingerprint. |
| TUNE-06 | Satisfied | Settings/pins/runner audits, documentation, and 3,901/300 line checks are verified; release 44477155670 passed `install-line-latest`. |

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

### Hosted Ship Evidence

These are automated results, not owner verification:

1. [Phase PR #28](https://github.com/szTheory/schwaystation/pull/28): exact-head CI run 44468099352 and Nightly run 44468099361 passed. The Ubuntu 26 linked WASI module measured 595,820 bytes; Windows ARM64 completed `policy.quality_metrics` in 116.03s under the 180s cap.
2. [Release PR #29](https://github.com/szTheory/schwaystation/pull/29): exact generated files were `.release-please-manifest.json`, `CHANGELOG.md`, and `version.txt`; `CI required` passed in run 44476990492 in about 70s. The old baseline's merge SHA is unavailable, so no exact fingerprint or speedup claim is made.
3. Release PR #29 merged as `af64484f388eddf67c615673b98edd6ccb5dae4c`. Main CI [run 44477155668](https://github.com/szTheory/schwaystation/actions/runs/44477155668) passed on the targeted retry; automatic Release [run 44477155670](https://github.com/szTheory/schwaystation/actions/runs/44477155670) published [v0.7.0](https://github.com/szTheory/schwaystation/releases/tag/v0.7.0) with 19 assets and passed `install-line-latest`.
4. Default-branch triage [run 44486437761](https://github.com/szTheory/schwaystation/actions/runs/44486437761) uploaded a schema-valid artifact showing zero open issues and no truncation; its CI, release and Nightly entries all succeeded.

### Gaps Summary

No source, wiring, local-test, or hosted ship-evidence gap remains. The release-App baseline cannot support an exact workload fingerprint because its merge SHA was not retained; that limitation is recorded and no speedup is claimed. Phase 7 is released, main CI and the release are green, and Phase 8 may proceed.

---

**Verification approach:** Goal-backward from ROADMAP success criteria, cross-checked with all 10 plans and their artifact links.
**Must-haves source:** Phase 7 ROADMAP success criteria plus Plans 07-01 through 07-10.
**Automated checks:** 13 focused CTest checks passed after targeted reconfiguration; all 10 plan artifact queries passed (27/27 artifacts).
**Human checks required:** None; this is an infrastructure phase. Hosted ship evidence is complete.
**Total verification time:** approximately 12 minutes of initial verification and focused checks; this refresh reused unchanged code/test and hosted-run evidence and regenerated the covered-input fingerprint.

---
Verified: 2026-11-19T08:50:20Z
