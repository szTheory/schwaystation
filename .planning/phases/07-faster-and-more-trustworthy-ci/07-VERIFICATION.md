---
phase: 07-faster-and-more-trustworthy-ci
verified: 2026-11-25T01:57:35Z
status: passed
score: 11/11 must-haves verified
covered_files:
  - .github/workflows/ci.yml
  - .github/workflows/nightly.yml
  - .github/workflows/release.yml
  - .github/workflows/triage.yml
  - .planning/PRODUCT-ROADMAP.md
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
  - .planning/phases/07-faster-and-more-trustworthy-ci/07-DELIVERY-EVIDENCE.md
  - .planning/phases/07-faster-and-more-trustworthy-ci/07-QUALITY-REVIEW.md
  - .planning/phases/07-faster-and-more-trustworthy-ci/07-UPGRADE-AUDIT.md
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
covered_digest: "v4:sha256:e65dbf9f566c3f75b8a2b9cc30c8a4627f1eedf51b9456f026fb0015d522c576"
behavior_unverified: 0
overrides_applied: 0
decision_coverage:
  honored: 0
  total: 0
  not_honored: []
re_verification:
  previous_status: gaps_found
  previous_score: 10/11
  gaps_closed:
    - "Maintainers can trace current settings, pin and runner checks, device and documentation budgets, install instructions, and roadmap and prompt coverage to evidence."
  gaps_remaining: []
  regressions: []
advisory: []
---

# Phase 7: Faster and more trustworthy CI — Verification Report

**Phase Goal:** Maintainers can see where CI time goes, find work that needs attention, and verify that the pipeline and published guidance meet measured quality targets.

**Verified:** 2026-11-25T01:57:35Z
**Status:** passed
**Re-verification:** Yes — after closing the prior documentation and coverage gap. The existing must-haves were retained.

## Goal Achievement

The five roadmap truths remain the phase contract. Plan truths that add implementation or delivery detail are included below, with direct restatements merged into the roadmap rows.

### Observable Truths

| # | Truth | Status | Evidence |
|---|---|---|---|
| 1 | A scheduled weekly workflow produces a sanitized triage artifact for open issues and current run status, with read-only project access and no messages. | VERIFIED | Current triage workflow schedules Saturday and supports manual dispatch; its job grants only Actions, contents and issues read permissions, calls the production collector, and uploads one seven-day JSON artifact. The collector only GETs issue and workflow metadata and writes the sanitized schema. Current exact-head CI run 45442543076 passed ci.triage against trusted, hostile, empty, capped and API-failure fixtures. Historical default-branch run 44486437761 completed successfully and uploaded the artifact; its seven-day artifact has expired. |
| 2 | CTest exercises the production triage transform and checks privacy, schema bounds, stable output and API-failure behavior. | VERIFIED | tests/policy/tests.cmake registers ci.triage against tests/cmake/triage_controls.cmake, which invokes scripts/triage-report.sh through a stub gh client with fixed fixture inputs and checks output and failures. The exact-head Main CI log records ci.triage passed in 1.12s. |
| 3 | A CI report compares up to 40 runs, identifies the measured path, duplicate work, core use and caching, links timeout calculations and flaky-check dispositions, and avoids unsupported speed claims. | VERIFIED | 07-CI-EFFICIENCY.md contains 40 run rows, 573 job records, 25 completed comparable runs across 20 workload groups, critical-path/DAG and duplicate-work analysis, cache/core limits, run-linked timeout arithmetic and failure dispositions. The captured release-App comparison explicitly disclaims a speedup because the baseline merge SHA is missing. The current Phase 14 PR CI run measured 3,921s but emitted “unbaselined workload” because fingerprint f0dd7d5c8854fb539e0528ff4fdec70b799d9214d2cdf0c6a2870039686c6c55 differs from the recorded 316f86729c5129fc618ae0acf71014a5b18784d9e7a4f6c03ff067d0b4326ed8; the gate did not claim a comparison. |
| 4 | The exact release-App generated-metadata PR route is the only path that skips heavy CI; uncertain lists, other changes, renames and release inputs fail closed to heavy checks. | VERIFIED | ci.yml verifies the PR metadata against the GitHub API before and after enumerating files, authenticates the release App bot, and requires exactly the three generated files. The production classifier routes other/invalid paths to heavy checks. ci.classify_policy passed in the exact-head Main CI run; hygiene route tests also passed. |
| 5 | The seven-dimension quality review identifies the weakest dimension and verifies one bounded improvement. | VERIFIED | 07-QUALITY-REVIEW.md scores all seven dimensions, records the documentation/README tie-break and selects only the false BIOS/disc README claim. Current README no longer contains that claim and states the fixture-scoped compatibility limit. |
| 6 | quality.json covers the measured performance, Wasm, state and comparable CI metrics; CI enforces applicable bounds, and lesson uniqueness rejects duplicate IDs and duplicate normalized text. | VERIFIED | Current quality.json carries Cachegrind bounds 1,620,101 and 138,319,540 Ir/frame, linked WASI size 947,969 bytes, state size 3,757,638 bytes, and the explicitly fingerprinted 2,249–3,293s CI range. ci.yml measures live producers and passes them to tests/cmake/quality.cmake. The current exact-head run passed perf, wasm32, core.state.stable_bytes, policy.quality_metrics, policy.lessons_unique and its negative control. Its CI timing sample was correctly marked unbaselined on fingerprint mismatch. |
| 7 | The repository-check budget’s progression is measured and documented, the final gate matches the measured 3,901-line shipping inventory, and the RetroArch budget remains 300 lines. | VERIFIED | Phase 7 baseline and delivery evidence record the 3,300-line initial cap, the later justified 3,900-line working cap, and the final 3,901-line ratchet. Current tests/hygiene/tests.cmake registers 3,901, the independent RetroArch cap is 300, and both current CI hygiene checks passed. |
| 8 | The relative performance regression gate and native/portable test coverage remain connected to the workflow. | VERIFIED | ci.yml retains the head/base Cachegrind comparison and the 1% regression guard. The exact-head run’s perf job passed, and its Linux build reported all 705 CI tests passing; the WASI lane also completed its portable test inventory. |
| 9 | Maintainers can trace current settings, pin and runner checks, device and documentation budgets, install instructions, and roadmap and prompt coverage to evidence. | VERIFIED | `docs/testing.md` now documents the measured 3,901-line cap and command; the Phase 7 product-roadmap section records completion and 947,969-byte WASI evidence; prompt coverage records measured WASI and release-App route evidence while preserving the missing merge-ref SHA limitation. The docs size budget passes at 61,432 bytes (<61,440); repository-check and RetroArch inventories pass at 3,901/3,901 and 300/300. |
| 10 | Every Phase 7 requirement has a focused result or an explicit ship-only evidence locator. | VERIFIED | TUNE-01 through TUNE-06 are all mapped in the requirements table below. 07-DELIVERY-EVIDENCE.md distinguishes local checks, hosted evidence and ship-only outcomes rather than treating pending checks as completed. |
| 11 | The reviewed Phase 7 PR, generated release PR, post-merge CI and release provide the promised ship evidence. | VERIFIED | GitHub reports success for Phase PR CI/Nightly 44468099352/44468099361, generated release PR CI 44476990492, post-merge Main CI/Release 44477155668/44477155670, and triage run 44486437761. Current-source Phase 14 PR CI 45442543076 passed all heavy lanes; post-merge Main CI 45455115354 and Release 45455115400 also succeeded on f08fc08d40a1ebcb42708e0a31be3111e3eed711. |

**Score:** 11/11 must-haves verified (0 present, behavior-unverified). The prior documentation and coverage drift gap is closed.

### Required Artifacts

| Plan | Declared artifacts | Result | Details |
|---|---|---|---|
| 07-01 | tests/policy/tests.cmake; tests/policy/inventory.txt; tests/cmake/quality.cmake; tests/cmake/lessons_unique.cmake; scripts/triage-report.sh | 5/5 verified | All exist and are substantive; current CI tests exercise their production wiring. |
| 07-02 | 07-CI-EFFICIENCY.md | 1/1 verified | Contains captured runs, comparable cohorts, DAG, timeouts, failure dispositions and explicit unknowns. |
| 07-03 | tests/hygiene/tests.cmake; 07-BASELINES.md; 07-UPGRADE-AUDIT.md | 3/3 verified | Present and substantive; the budget history and audit evidence are traceable. |
| 07-04 | 07-QUALITY-REVIEW.md | 1/1 verified | Seven scores, tie-break, selected change and measurable outcome are present. |
| 07-05 | .github/workflows/triage.yml; scripts/triage-report.sh | 2/2 verified | Workflow-to-collector link is direct; current CI passes the production transform controls. |
| 07-06 | scripts/classify-changes.sh; .github/workflows/ci.yml; tests/hygiene/tests.cmake | 3/3 verified | Workflow-authenticated route calls the classifier; production route controls pass. |
| 07-07 | quality.json; .github/workflows/ci.yml; tests/core/test_state.c | 3/3 verified | Current producer values match the registry; test and quality controls pass on hosted CI. |
| 07-08 | scripts/ci-critical-path.sh; .github/workflows/ci.yml; quality.json | 3/3 verified | Exact-attempt job data, workload fingerprint and comparator are connected. |
| 07-09 | docs/maintaining.md; docs/testing.md; PROMPT-COVERAGE.md; PRODUCT-ROADMAP.md | 4/4 verified | Maintainer guidance matches the 3,901-line gate; the product roadmap records measured WASI size and completed Phase 7; prompt coverage records current CI and release-route evidence, including the limitation on exact speedup comparison. |
| 07-10 | 07-DELIVERY-EVIDENCE.md; 07-QUALITY-REVIEW.md | 2/2 verified | Focused outcomes and hosted shipment locators are recorded. |

The plan artifact query found 27/27 artifacts present with no structural stub issues. The two artifact checks marked “current” under Plan 07-09 fail on content accuracy, despite passing existence and substance checks.

### Key Link Verification

The automated key-link query found lexical false negatives for several indirect CMake, workflow-marker and documentation links. Manual source traces confirmed these connections:

| From | To | Via | Status | Details |
|---|---|---|---|---|
| .github/workflows/triage.yml | scripts/triage-report.sh | scheduled/manual collector step | WIRED | Read-only scope, collector invocation and artifact upload verified. |
| tests/policy/tests.cmake | triage_controls.cmake; quality_controls.cmake; lessons_unique.cmake; ci_route_controls.cmake | registered CTest cases | WIRED | Current run logs show the linked behavior tests passed. |
| .github/workflows/ci.yml | scripts/classify-changes.sh | metadata validation and route classification | WIRED | Exact PR identity and path set are validated before the classifier opt-in; failure path is heavy. |
| .github/workflows/ci.yml | tests/cmake/quality.cmake | real Cachegrind, state, Wasm and CI timing producers | WIRED | Current CI job calls are present and the real producers passed. |
| scripts/ci-critical-path.sh | quality.json via tests/cmake/quality.cmake | current attempt jobs, DAG seconds and workload hash | WIRED | Current PR run records 3,921s as unbaselined because the workload hash differs. |
| .github/workflows/release.yml | .github/workflows/ci.yml | release-please reusable CI, then publish | WIRED | Release run 45455115400 passed its reusable CI, publish and install-line-latest jobs. |
| docs/testing.md | quality.json; tests/hygiene/tests.cmake | documented producers and budget preflight | WIRED | Current quality values and the measured 3,901-line gate match their producers; the strict documentation byte budget also passes. |
| .planning/phases/07-faster-and-more-trustworthy-ci/07-DELIVERY-EVIDENCE.md | .planning/phases/07-faster-and-more-trustworthy-ci/07-CI-EFFICIENCY.md | linked before/after and ship evidence | WIRED | The report records comparable limits, actual route outcomes and the missing baseline SHA. |

### Data-Flow Trace

| Artifact | Data variable | Source | Produces real data | Status |
|---|---|---|---|---|
| scripts/triage-report.sh | issue and workflow records | GitHub Issues and Actions APIs through gh api | Yes; schema checks and scrub rules produce bounded JSON | FLOWING; current CTest controls pass; historical upload run 44486437761 succeeded (artifact expired after seven days). |
| .github/workflows/ci.yml | instructions per frame | Cachegrind output from real runner workloads | Yes; integer counts flow through the quality comparator | FLOWING; current perf job passed. |
| .github/workflows/ci.yml | serialized state bytes | state.stable_bytes from the test binary and public serialization path | Yes; exactly one positive integer is checked | FLOWING; current test and quality jobs passed at 3,757,638 bytes. |
| .github/workflows/ci.yml | linked WASI module bytes | build/wasm32/schwaystation-run | Yes; module is validated and sized after linking | FLOWING; current WASI job passed at 947,969 bytes. |
| .github/workflows/ci.yml | current CI critical path | paginated current-attempt Actions jobs with start/end timestamps | Yes; helper verifies job identities and computes the dependency path | FLOWING; current PR sample was 3,921s and correctly reported unbaselined, not compared to the old fingerprint. |

### Behavioral Spot-Checks

These results reuse read-only GitHub Actions evidence; no local suite was rerun.

| Behavior | Command/evidence | Result | Status |
|---|---|---|---|
| Current Phase 14 final-head CI on 75beb849f4b9e8a9a3bd023cc5c80632e4c74d56 | [Main CI run 45442543076](https://github.com/szTheory/schwaystation/actions/runs/45442543076) | All heavy platform, perf, hygiene, hash, RetroArch and required jobs succeeded; Linux reported 705/705 CI tests. | PASS |
| Triage, classifier, quality and lesson production controls | Main CI run 45442543076 Linux build logs | ci.triage, ci.classify_policy, policy.quality_metrics, policy.lessons_unique and its self-test passed. | PASS |
| Repository and E2E budget, route and timeout controls | Main CI run 45442543076 hygiene job | policy.gate_budget, policy.e2e_budget, ci.classify, and hygiene.timeout_comments including self-test passed. | PASS |
| Current attempt timing policy | Main CI run 45442543076 CI required job | Measured 3,921s; fingerprint mismatch emitted the explicit unbaselined result and deferred comparison. | PASS |
| Post-merge Main CI on f08fc08d40a1ebcb42708e0a31be3111e3eed711 | [Main CI run 45455115354](https://github.com/szTheory/schwaystation/actions/runs/45455115354) | Succeeded; planning/release-metadata-only route skipped heavy jobs as allowed. | PASS |
| Published Release path on f08fc08d40a1ebcb42708e0a31be3111e3eed711 | [Release run 45455115400](https://github.com/szTheory/schwaystation/actions/runs/45455115400) | Reusable release CI, hash-equality, publication and install-line-latest succeeded. | PASS |

### Probe Execution

No probes are declared in the Phase 7 plans or summaries, and no conventional scripts/*/tests/probe-*.sh file applies to this CI-tooling phase.

### Requirements Coverage

| Requirement | Source plan | Status | Evidence |
|---|---|---|---|
| TUNE-01 | 07-01, 07-05, 07-10 | Satisfied | Read-only scheduled/manual workflow, sanitizer and fixture controls, historical uploaded artifact. |
| TUNE-02 | 07-01, 07-02, 07-06, 07-08, 07-10 | Satisfied | 40-run report, path classifier, exact-attempt timing helper and correct unbaselined behavior. |
| TUNE-03 | 07-02, 07-06, 07-10 | Satisfied | Run-linked timeout arithmetic and flaky-check dispositions; production CI route and required roll-up controls pass. |
| TUNE-04 | 07-04, 07-09, 07-10 | Satisfied | Seven-dimension review and the selected README correction are verified. |
| TUNE-05 | 07-01, 07-03, 07-07, 07-08, 07-10 | Satisfied | Current quality producers and bounds pass; lesson duplicate identity/text negative controls pass. |
| TUNE-06 | 07-03, 07-09, 07-10 | SATISFIED | Current maintainer guidance, product roadmap and prompt coverage agree with the measured budget and shipped evidence; release run 45455115400 passed install-line-latest. |

All six requirement IDs declared by plans are covered by the roadmap requirement list; no orphaned requirement was found.

### Test Quality Audit

| Test file | Linked requirement | Active | Skipped | Circular | Assertion level | Verdict |
|---|---|---:|---:|---|---|---|
| tests/cmake/triage_controls.cmake | TUNE-01 | Yes | 0 | No | Behavioral privacy, schema, bounds, determinism and API-failure assertions | PASS |
| tests/cmake/ci_route_controls.cmake | TUNE-02/03 | Yes | 0 | No | Production routing cases with exact expected outputs and fail-closed cases | PASS |
| tests/cmake/quality_controls.cmake | TUNE-05 | Yes | 0 | No | Production measurement/comparison blocks with in-bound, over-bound and missing-data controls | PASS |
| tests/cmake/lessons_unique.cmake | TUNE-05 | Yes | 0 | No | Live ledger plus duplicate-ID and normalized-text negative controls | PASS |
| tests/hygiene/tests.cmake | TUNE-05/06 | Yes | 0 | No | Exact budget boundaries, timeout evidence and route failure controls | PASS |
| tests/core/test_state.c | TUNE-05 | Yes | 0 | No | Reads actual serialized byte count and asserts state format/length | PASS |

The linked tests contain no disabled/skipped test patterns. Fixture outputs are fixed synthetic inputs; no test generates its expected values by running the system under test. The current exact-head hosted run passed the linked cases.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|---|---:|---|---|---|
| scripts/triage-report.sh | 22, 42 | XXXXXX is the secure mktemp suffix, not an XXX debt marker. | Info | No unresolved debt marker. |
| docs/testing.md | 76 | “placeholder registrations” describes CD-ROM-only test registration, not an unfinished Phase 7 feature. | Info | No Phase 7 stub. |
| tests/hygiene/tests.cmake | 535 | “placeholder zips” refers to synthetic release archive fixtures. | Info | No production placeholder. |

No unreferenced TBD, FIXME or XXX debt marker was found in the Phase 7 implementation files. No empty implementation or console-log-only implementation was found.

### Human Verification Required

N/A — this is an infrastructure/CI phase with no user-facing flow to manually test. Its observable outcomes are covered by gating checks and exact hosted run evidence.

### Gaps Summary

The Phase 7 CI and release mechanisms remain connected and pass on current source. The final-head Main CI run passed its heavy jobs and correctly declined to compare a 3,921-second path against the Phase 7 baseline after the workload fingerprint changed. Post-merge Main CI and Release also succeeded. The previously reported documentation and coverage mismatch is resolved: maintainer guidance matches the 3,901-line gate, and the roadmap and prompt coverage now reflect completed measured evidence without claiming an exact release-App speedup. Cheap current budget checks and the documentation byte-budget check pass.

---

_Verified: 2026-11-25T01:57:35Z_
_Verifier: the agent (gsd-verifier)_
