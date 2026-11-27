---
phase: 07-faster-and-more-trustworthy-ci
verified: 2026-11-27T04:18:44Z
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
covered_digest: "v4:sha256:475033a985681c1349c90be51401c70a5181f801b7fd2c5681c317d0bf0878d9"
behavior_unverified: 0
overrides_applied: 0
decision_coverage:
  honored: 0
  total: 0
  not_honored: []
re_verification:
  previous_status: gaps_found
  previous_score: 10/11 must-haves verified
  gaps_closed:
    - "docs/testing.md now identifies 3,901 as the historical Phase 7 ratchet, 4,694 as the current Phase 16-approved ceiling, and uses -DLIMIT=4694 in its current preflight command."
  gaps_remaining: []
  regressions: []
advisory: []
---

# Phase 7: Faster and more trustworthy CI — Verification Report

**Phase Goal:** Maintainers can see where CI time goes, find work that needs attention, and verify that the pipeline and published guidance meet measured quality targets.

**Verified:** 2026-11-27T04:18:44Z
**Status:** passed
**Re-verification:** Yes — refreshed the fingerprint against exact release head `a2f37b46eab24f53e30f99c6b3faff6eedc8f2f8` and rechecked the prior budget-documentation gap after its repair.

## Goal Achievement

This exact-head re-verification reused the supplied successful hosted runs: [source PR CI](https://github.com/szTheory/schwaystation/actions/runs/45913678576), [full release CI](https://github.com/szTheory/schwaystation/actions/runs/45923155197), and [post-merge metadata-route Main CI](https://github.com/szTheory/schwaystation/actions/runs/45923155084). I inspected the current workflow, budget, quality registry and maintainer guidance without rerunning the suite or code review. The repaired preflight agrees with the live gate, its focused line-count and documentation-budget checks passed, and the hosted CI and release runs remain green.

The five roadmap truths remain the phase contract. Plan truths that add implementation or delivery detail are included below, with direct restatements merged into the roadmap rows.

### Observable Truths

| # | Truth | Status | Evidence |
|---|---|---|---|
| 1 | A scheduled weekly workflow produces a sanitized triage artifact for open issues and current run status, with read-only project access and no messages. | VERIFIED | Current triage workflow schedules Saturday and supports manual dispatch; its job grants only Actions, contents and issues read permissions, calls the production collector, and uploads one seven-day JSON artifact. The collector only GETs issue and workflow metadata and writes the sanitized schema. The earlier source-head CI run 45442543076 passed the production controls; the supplied exact-closeout-head full release CI run 45923155197 also succeeded. Historical default-branch run 44486437761 completed successfully and uploaded the artifact; its seven-day artifact has expired. |
| 2 | CTest exercises the production triage transform and checks privacy, schema bounds, stable output and API-failure behavior. | VERIFIED | tests/policy/tests.cmake registers ci.triage against tests/cmake/triage_controls.cmake, which invokes scripts/triage-report.sh through a stub gh client with fixed fixture inputs and checks output and failures. The source-head controls passed in Main CI 45442543076; the supplied exact-closeout-head full release CI run 45923155197 succeeded. |
| 3 | A CI report compares up to 40 runs, identifies the measured path, duplicate work, core use and caching, links timeout calculations and flaky-check dispositions, and avoids unsupported speed claims. | VERIFIED | 07-CI-EFFICIENCY.md contains 40 run rows, 573 job records, 25 completed comparable runs across 20 workload groups, critical-path/DAG and duplicate-work analysis, cache/core limits, run-linked timeout arithmetic and failure dispositions. The captured release-App comparison explicitly disclaims a speedup because the baseline merge SHA is missing. Its Phase 14 timing sample is historical and correctly emitted “unbaselined workload” because the workload fingerprint differed; the exact-head CI run results supplied for this re-verification establish pass status, not a new comparable timing claim. |
| 4 | The exact release-App generated-metadata PR route is the only path that skips heavy CI; uncertain lists, other changes, renames and release inputs fail closed to heavy checks. | VERIFIED | ci.yml verifies the PR metadata against the GitHub API before and after enumerating files, authenticates the release App bot, and requires exactly the three generated files. The production classifier routes other/invalid paths to heavy checks. The source-head classifier controls passed in Main CI 45442543076; the exact-closeout-head metadata-route Main CI run 45923155084 succeeded. |
| 5 | The seven-dimension quality review identifies the weakest dimension and verifies one bounded improvement. | VERIFIED | 07-QUALITY-REVIEW.md scores all seven dimensions, records the documentation/README tie-break and selects only the false BIOS/disc README claim. Current README no longer contains that claim and states the fixture-scoped compatibility limit. |
| 6 | quality.json covers the measured performance, Wasm, state and comparable CI metrics; CI enforces applicable bounds, and lesson uniqueness rejects duplicate IDs and duplicate normalized text. | VERIFIED | Current quality.json carries Cachegrind bounds 1,620,101 and 138,319,540 Ir/frame, linked WASI size 947,969 bytes, state size 3,757,638 bytes, and the explicitly fingerprinted 2,249–3,293s CI range. ci.yml measures live producers and passes them to tests/cmake/quality.cmake. The earlier source-head run passed the perf, wasm32, state, quality and lesson controls; the supplied exact-closeout-head full release CI run 45923155197 succeeded. Its provided status contains no new timing measurement, so this re-verification makes no new CI-speed comparison claim. |
| 7 | Phase 7 measured and ratcheted its repository-check budget to the 3,901-line shipping inventory, with a separate 300-line RetroArch budget; a later owner-approved Phase 16 change raised the current shared gate ceiling to 4,694. | VERIFIED | Phase 7 baseline and delivery evidence record the 3,300-line initial cap, the justified 3,900-line working cap, and the final 3,901-line Phase 7 ratchet. Current tests/hygiene/tests.cmake registers the separately approved 4,694-line ceiling, and the independent RetroArch cap remains 300. docs/testing.md now distinguishes historical from current limits and its preflight uses 4,694. The exact current preflight measured 4,694/4,694 and exited 0. |
| 8 | The relative performance regression gate and native/portable test coverage remain connected to the workflow. | VERIFIED | ci.yml retains the head/base Cachegrind comparison and the 1% regression guard. The supplied exact-head release CI completed successfully across its required lanes; its pass result is not used to assert an unreported performance delta. |
| 9 | Maintainers can trace current settings, pin and runner checks, device and documentation budgets, install instructions, and roadmap and prompt coverage to evidence. | VERIFIED | docs/testing.md distinguishes the Phase 7 historical 3,901-line ratchet from the current Phase 16-approved 4,694-line limit, and its preflight command uses `-DLIMIT=4694`, matching tests/hygiene/tests.cmake. The focused line-count preflight exited 0 at 4,694/4,694; the documentation-budget check also exited 0 with the file below its strict limit. Other listed Phase 7 evidence remains present. |
| 10 | Every Phase 7 requirement has a focused result or an explicit ship-only evidence locator. | VERIFIED | TUNE-01 through TUNE-06 are all mapped in the requirements table below. 07-DELIVERY-EVIDENCE.md distinguishes local checks, hosted evidence and ship-only outcomes rather than treating pending checks as completed. |
| 11 | The reviewed Phase 7 PR, generated release PR, post-merge CI and release provide the promised ship evidence. | VERIFIED | Historic Phase 7 PR, generated release PR, post-merge CI/Release and triage runs remain linked in 07-DELIVERY-EVIDENCE.md. Exact-head source PR CI 45913678576, full release CI 45923155197 and post-merge metadata-route Main CI 45923155084 all succeeded on the v2.0 closeout head. |

**Score:** 11/11 truths verified (0 present, behavior-unverified; 0 failed).

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
| 07-09 | docs/maintaining.md; docs/testing.md; PROMPT-COVERAGE.md; PRODUCT-ROADMAP.md | 4/4 current-guidance checks verified | The roadmap and prompt-coverage evidence remain accurate. `docs/testing.md` now labels 3,901 as the historical Phase 7 measurement, states the current approved 4,694-line ceiling, and runs the matching `-DLIMIT=4694` preflight. The documentation byte-budget check passes. |
| 07-10 | 07-DELIVERY-EVIDENCE.md; 07-QUALITY-REVIEW.md | 2/2 verified | Focused outcomes and hosted shipment locators are recorded. |

The plan artifact query found 27/27 artifacts present with no structural stub issues. The documentation artifact is substantive, and its current budget statement and preflight match the active gate.

### Key Link Verification

The automated key-link query found lexical false negatives for several indirect CMake, workflow-marker and documentation links. Manual source traces confirmed these connections:

| From | To | Via | Status | Details |
|---|---|---|---|---|
| .github/workflows/triage.yml | scripts/triage-report.sh | scheduled/manual collector step | WIRED | Read-only scope, collector invocation and artifact upload verified. |
| tests/policy/tests.cmake | triage_controls.cmake; quality_controls.cmake; lessons_unique.cmake; ci_route_controls.cmake | registered CTest cases | WIRED | Current run logs show the linked behavior tests passed. |
| .github/workflows/ci.yml | scripts/classify-changes.sh | metadata validation and route classification | WIRED | Exact PR identity and path set are validated before the classifier opt-in; failure path is heavy. |
| .github/workflows/ci.yml | tests/cmake/quality.cmake | real Cachegrind, state, Wasm and CI timing producers | WIRED | Current CI job calls are present and the real producers passed. |
| scripts/ci-critical-path.sh | quality.json via tests/cmake/quality.cmake | current attempt jobs, DAG seconds and workload hash | WIRED | Current PR run records 3,921s as unbaselined because the workload hash differs. |
| .github/workflows/release.yml | .github/workflows/ci.yml | release-please reusable CI, then publish | WIRED | Historic Release run 45455115400 passed its reusable CI, publish and install-line-latest jobs; the exact-head full release CI run 45923155197 also passed. |
| docs/testing.md | quality.json; tests/hygiene/tests.cmake | documented producers and budget preflight | WIRED | Documented quality producers remain connected; budget guidance distinguishes the historical 3,901-line Phase 7 ratchet from the current 4,694-line registration, and the documented command uses `-DLIMIT=4694`. The strict documentation byte budget is checked by current CI. |
| .planning/phases/07-faster-and-more-trustworthy-ci/07-DELIVERY-EVIDENCE.md | .planning/phases/07-faster-and-more-trustworthy-ci/07-CI-EFFICIENCY.md | linked before/after and ship evidence | WIRED | The report records comparable limits, actual route outcomes and the missing baseline SHA. |

### Data-Flow Trace

| Artifact | Data variable | Source | Produces real data | Status |
|---|---|---|---|---|
| scripts/triage-report.sh | issue and workflow records | GitHub Issues and Actions APIs through gh api | Yes; schema checks and scrub rules produce bounded JSON | FLOWING; current CTest controls pass; historical upload run 44486437761 succeeded (artifact expired after seven days). |
| .github/workflows/ci.yml | instructions per frame | Cachegrind output from real runner workloads | Yes; integer counts flow through the quality comparator | FLOWING; current perf job passed. |
| .github/workflows/ci.yml | serialized state bytes | state.stable_bytes from the test binary and public serialization path | Yes; exactly one positive integer is checked | FLOWING; the earlier source-head run passed at the recorded 3,757,638-byte value; exact-closeout-head full release CI also succeeded. |
| .github/workflows/ci.yml | linked WASI module bytes | build/wasm32/schwaystation-run | Yes; module is validated and sized after linking | FLOWING; the earlier source-head WASI job passed at 947,969 bytes; exact-closeout-head full release CI also succeeded. |
| .github/workflows/ci.yml | current CI critical path | paginated current-attempt Actions jobs with start/end timestamps | Yes; helper verifies job identities and computes the dependency path | FLOWING; the Phase 14 sample was 3,921s and correctly reported unbaselined, not compared to the old fingerprint. The exact-closeout-head status supplied here does not contain a timing value. |

### Behavioral Spot-Checks

These results reuse read-only GitHub Actions evidence; no local suite was rerun.

| Behavior | Command/evidence | Result | Status |
|---|---|---|---|
| Full source CI on exact closeout head `a2f37b46eab24f53e30f99c6b3faff6eedc8f2f8` | [Source PR CI run 45913678576](https://github.com/szTheory/schwaystation/actions/runs/45913678576) | Passed. | PASS |
| Full release CI, including required cross-platform, quality and test lanes | [Release CI run 45923155197](https://github.com/szTheory/schwaystation/actions/runs/45923155197) | Passed on exact closeout source. | PASS |
| Post-merge generated-metadata route | [Main CI run 45923155084](https://github.com/szTheory/schwaystation/actions/runs/45923155084) | Passed; this is route evidence and is not counted as a rerun of heavy CI. | PASS |
| CI measurement claim | Exact-head run summaries supplied for this verification | The runs establish pass status; no timing value was supplied, so no new wall-clock or cost comparison is asserted. | PASS |
| Repository-check budget preflight | `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=4694 -P tests/cmake/line_budget.cmake` | Exit 0; measured 4,694/4,694, matching the current approved limit and documented command. | PASS |
| Documentation-size preflight | `cmake -DROOT=. -P tests/cmake/budgets.cmake` | Exit 0; docs/testing.md measured 61,237 bytes, below the strict 61,440-byte maximum. | PASS |
| Patch whitespace validation | `git diff --check` | Exit 0. | PASS |

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
| TUNE-06 | 07-03, 07-09, 07-10 | Satisfied | Maintainer guidance preserves the historical 3,901-line Phase 7 result, names the current approved 4,694-line limit, and runs a matching preflight. The exact count and documentation-size checks both passed. |

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

The CI and release mechanisms remain connected and the supplied exact-head source PR, full release and metadata-route Main runs succeeded. Phase 7's historical 3,901-line measurement and the Phase 16 owner-approved 4,694-line current ceiling are both documented accurately; the active preflight command matches the live registration. The targeted line-count, documentation-size and whitespace checks passed. No remaining gap or source implementation issue was found.

---

_Verified: 2026-11-27T04:18:44Z_
_Verifier: the agent (gsd-verifier)_
