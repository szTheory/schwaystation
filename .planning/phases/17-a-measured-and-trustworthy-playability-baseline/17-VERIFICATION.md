---
phase: 17-a-measured-and-trustworthy-playability-baseline
verified: 2026-11-28T07:59:32Z
status: gaps_found
score: 19/20 truths verified
re_verification:
  previous_status: gaps_found
  previous_score: 19/20
  gaps_closed: []
  gaps_remaining:
    - "TUNE-10 current hosted WASI byte count and comparable ci.critical_path_s evidence remain unavailable."
  regressions: []
covered_files:
  - .github/workflows/ci.yml
  - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-01-PLAN.md
  - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-01-SUMMARY.md
  - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-02-PLAN.md
  - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-02-SUMMARY.md
  - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-03-PLAN.md
  - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-03-SUMMARY.md
  - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-04-PLAN.md
  - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-04-SUMMARY.md
  - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-05-PLAN.md
  - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-05-SUMMARY.md
  - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-06-PLAN.md
  - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-06-SUMMARY.md
  - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-07-PLAN.md
  - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-07-SUMMARY.md
  - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-08-PLAN.md
  - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-08-SUMMARY.md
  - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-09-PLAN.md
  - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-09-SUMMARY.md
  - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-BASELINES.md
  - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-BE32-REVIEW.md
  - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-CI-EFFICIENCY.md
  - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-DELIVERY-EVIDENCE.md
  - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-DOC-AUDIT.md
  - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-IMPROVEMENT-CONTRACT.md
  - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-IMPROVEMENT-EVIDENCE.md
  - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-LESSONS-EXPORT.md
  - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-QUALITY-REVIEW.md
  - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-RELIABILITY.md
  - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-REVIEW.md
  - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-SKELETON.md
  - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-UPGRADE-AUDIT.md
  - .planning/preparation/DECISIONS.md
  - .planning/preparation/PROMPT-COVERAGE.md
  - .planning/todos/completed/049-review-runner-checkpoint-repeats.md
  - .planning/todos/pending/022-windows-dll-crt-consumer-flake.md
  - .planning/todos/pending/052-be32-movie-test-shard.md
  - docs/compatibility.md
  - docs/testing.md
  - quality.json
  - scripts/ci-critical-path.sh
  - tests/cmake/quality.cmake
  - tests/compat/ledger.csv
  - tests/compat/tests.cmake
  - tests/hygiene/tests.cmake
covered_digest: "v4:sha256:cf0d355ad7d4cc4701f6308281b97d9e462644fd5156e5b60af7a6b009979d13"
behavior_unverified: 0
overrides_applied: 0
gaps:
  - truth: "Published quality bounds match remeasured existing producers, and a maintainer can trace each current quality metric to its exact producer, configuration, and measured output."
    status: partial
    reason: "Exact Cachegrind and state results are present, but the final report explicitly leaves the current hosted WASI integer unobservable and ci.critical_path_s pending. Hosted run 46112240653 passed the WASI bound check and required jobs, but its workflow_dispatch event/workload fingerprint does not match the stored push/main bound and it did not produce a current comparable CI metric."
    artifacts:
      - path: ".planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-BASELINES.md"
        issue: "Records the current hosted WASI byte count and current ci.critical_path_s as pending."
      - path: ".planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-DELIVERY-EVIDENCE.md"
        issue: "Confirms hosted run 46112240653 is a workflow_dispatch event, not comparable to the stored push/main fingerprint, and does not expose the exact hosted WASI byte count."
      - path: "quality.json"
        issue: "The existing quality check passes, but its retained CI interval is historical for a different fingerprint; the final dispatch did not evaluate it as a comparable current measurement."
    missing:
      - "An exact output from the current hosted WASI producer, or an equivalent exact current producer result whose configuration is documented."
      - "A current ci.critical_path_s result tied to a supported matching workload fingerprint before describing the stored interval as current."
---

# Phase 17: A measured and trustworthy playability baseline — Verification Report

**Phase Goal:** Players and maintainers can rely on current installation guidance, scoped compatibility claims and measured performance and CI limits before playability work begins.
**Verified:** 2026-11-28T07:59:32Z
**Status:** gaps_found
**Re-verification:** Yes — after the final delivery-evidence update.

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---:|---|---|---|
| 1 | Roadmap SC1 / Plan 17-02 T1: a maintainer can inspect up to 40 recent runs, matching fingerprints, critical path, duplicate work, failures/flakes and runner occupancy, with no speedup claim for unmatched runs. | ✓ VERIFIED | `17-CI-EFFICIENCY.md` inventories 40 runs and 556 exact-attempt jobs; it identifies the only repeated exact group, distinguishes runner occupancy from CPU/billing, and excludes PR runs with missing merge SHAs. It makes no full-CI speedup claim. |
| 2 | Roadmap SC2 / Plan 17-02 T2: changed timeouts and flaky-check changes have traceable evidence, or remain unchanged where the cohort cannot support them. | ✓ VERIFIED | `17-RELIABILITY.md` records the two matching release-please durations (9s, 11s), calculates `ceil(2 × 11 / 60) = 1` minute, and retains the 5-minute floor. It classifies the four failures and changes no timeout or test. |
| 3 | Plan 17-02 T3: maintainers can see whether the Windows DLL and runner/checkpoint repeat-work claims are supported. | ✓ VERIFIED | The report lists ten Windows-x64 jobs, seven explicit clean consumer results, and three not-clean signals; TODO-022 remains pending. TODO-049 records why repeated outputs serve independent assertions. |
| 4 | Plan 17-01 T1: maintainers can identify exact current test registrations and inventories. | ✓ VERIFIED | `17-SKELETON.md` records 24 hygiene and 12 policy rows, sorted names/labels, and the CMake inventory registration path. `tests/tests.cmake` includes `tests/policy/tests.cmake`; the existing deferred inventory check enforces the set. |
| 5 | Plan 17-01 T2: a user can follow the current README installer into an isolated home and obtain the released core and metadata. | ✓ VERIFIED | The recorded `install_line.cmake` run installed the v0.16.0 dylib and `.info` under an isolated HOME. The flipped-byte control rejected the modified archive and installed nothing. The README command is the exact line exercised by that CMake script. |
| 6 | Roadmap SC5 / Plan 17-01 T3, 17-04 T1–T3, and 17-08 T1/T3: installation, compatibility and maintenance claims have source-backed dispositions; current docs, repository policy, pins, unknown combinations, and update behavior remain scoped. | ✓ VERIFIED | `17-DOC-AUDIT.md` has dispositions for each inspected claim, corrects four archived links, and preserves 13,407 unknown title entries, 27 unknown required-profile rows, and experimental metadata. `17-UPGRADE-AUDIT.md` maps active runners and consumed pins; the read-only settings check exited 0, and no unrelated pins/caches changed. `DECISIONS.md` and the source confirm Dependabot auto-merge stays off. |
| 7 | Roadmap SC4 / Plan 17-03 T1: published bounds match remeasured existing producers, and each current quality metric has exact producer/configuration/output evidence. | ✗ FAILED | Exact Cachegrind outputs (1,620,381 and 138,406,446 Ir/frame) and stable state bytes (3,757,754) are traceable. The local exact linked-Wasm result is 1,014,617 bytes; final hosted run 46112240653 passed the WASI bound check but does not expose its exact module bytes. Its `workflow_dispatch` event is not a matching current producer for `ci.critical_path_s`, whose current output/fingerprint remains pending. See the remaining gap. |
| 8 | Plan 17-03 T2 / Plan 17-08 T2: users can distinguish measured producer values from device/browser assumptions. | ✓ VERIFIED | `17-BASELINES.md`, `docs/testing.md`, `.planning/PRODUCT-ROADMAP.md`, and `PROMPT-COVERAGE.md` label low-power throughput as assumed, browser speed as unmeasured, and Wasmtime as determinism coverage only. |
| 9 | Plan 17-03 T3: no quality limit changes merely to hide a failure or compensate for unavailable evidence. | ✓ VERIFIED | `quality.json` is unchanged; the existing integer/bound comparator passes. The WASI interval and CI fingerprint remain unchanged rather than being widened or relabeled from the final dispatch. |
| 10 | Plan 17-04 T1: maintainers can inspect read-only repository-policy drift and upstream evidence for active runners and tools. | ✓ VERIFIED | `17-UPGRADE-AUDIT.md` records the `--check` result, active runner labels, actual tool consumers, pin identities, official evidence, and keep/change-needed/unavailable dispositions. `scripts/apply-repo-settings.sh --check` exited 0. |
| 11 | Plan 17-04 T2: update PRs remain reviewable and Dependabot auto-merge remains disabled. | ✓ VERIFIED | `.github/dependabot.yml` is absent, no Dependabot auto-merge workflow was added, and the later settled policy is explicitly reconciled in `DECISIONS.md`. |
| 12 | Plan 17-05 T1: the exact portable-test partition includes both long libretro tests. | ✓ VERIFIED | `17-BE32-REVIEW.md` records a disjoint 692 + 1 + 1 partition equal to the current 694-name inventory, with `libretro.host` and `libretro.input.movies` present once each. |
| 13 | Plan 17-05 T2: a maintainer can inspect whether a shard candidate preserves every test, fixture, report, artifact, and hash consumer. | ✓ VERIFIED | The BE32 review records the exact JUnit-union positive and six malformed negative controls plus fixture, `hashes-be32`, hash-equality, and required-DAG ownership. The hosted shard candidate itself is explicitly unmeasured. |
| 14 | Plan 17-05 T3: BE32 production gates remain unchanged when comparable hosted gain is unproven. | ✓ VERIFIED | `.github/workflows/ci.yml` retains the monolithic BE32 job and required `hashes-be32` route; TODO-052 remains pending because no comparable hosted shard run exists. |
| 15 | Roadmap SC3 / Plan 17-06 T1–T2 / Plan 17-07 T1: the weakest dimension is scored from current evidence and receives exactly one verified improvement. | ✓ VERIFIED | `17-QUALITY-REVIEW.md` scores all seven dimensions, selects compatibility breadth from the score-2 tie by observed impact, and contracts one frame-120 fixture result. `17-IMPROVEMENT-EVIDENCE.md` and the final local suite verify the result without broadening playability claims. |
| 16 | Plan 17-06 T3: Dependabot configuration is selected only if upgrade path wins the lowest-score review. | ✓ VERIFIED | Compatibility breadth won the score-2 tie; TODO-010 remains pending, `.github/dependabot.yml` was not added, and auto-merge remains off. |
| 17 | Plan 17-07 T2: every changed source file belongs to the selected contract and its focused proof. | ✓ VERIFIED | The source diff changes the compatibility test producer and ledger only for the selected frame-120 predicate; the other candidate routes (cheats, timeout, workflow, install, updater) were not implemented. |
| 18 | Plan 17-07 T3: the phase remains blocked if the selected improvement cannot be proven within scope. | ✓ VERIFIED | The change has the pinned expected hash, before/after gap reproduction, wrong/missing-result controls, and exact row scope. The improvement is proven; the separate TUNE-10 gap remains visible. |
| 19 | Plan 17-09 T1–T2: rolling roadmap and prompt coverage reflect current outcomes, and only new reproduced lessons are exported. | ✓ VERIFIED | `.planning/PRODUCT-ROADMAP.md` and `PROMPT-COVERAGE.md` preserve Phase 18/19 scope and state unmeasured limits. `17-LESSONS-EXPORT.md` records evidence-backed none; `.planning/LESSONS.md` is unchanged. |
| 20 | Plan 17-09 T3: phase handoff identifies every proven result and concrete remaining blocker without treating pending hosted checks as passes. | ✓ VERIFIED | `17-DELIVERY-EVIDENCE.md` lists TUNE-07–11, command/status/run evidence, the exact WASI/CI limitations, seven unresolved source-audit items, and no phase-18/19 implementation. Final local and hosted checks are recorded below. |

**Score:** 19/20 truths verified (one failed).

### Deferred Items

None. Roadmap Phases 18 and 19 cover game playability and a retail title, not the remaining Phase 17 metric evidence gap.

### Required Artifacts

| Artifact | Expected | Status | Details |
|---|---|---|---|
| `17-SKELETON.md` | CMake wiring, exact inventories, install evidence | ✓ VERIFIED | Substantive report with commands and installer outcome; referenced by the documentation audit. |
| `17-DOC-AUDIT.md` | Source-to-claim drift and correction record | ✓ VERIFIED | Dispositions cover installation, compatibility, hardware, metrics, and maintainer claims; later updated with final output identities. |
| `17-CI-EFFICIENCY.md` | Run membership, exact attempts, comparisons, timing, and occupancy | ✓ VERIFIED | 40 runs, 556 jobs, fingerprints, critical path and explicit measurement limits. |
| `17-RELIABILITY.md` | Timeout/flakiness dispositions and DLL evidence | ✓ VERIFIED | Exact matching sample, formulas, failure classes, ten Windows jobs, and pending shortfall. |
| `17-BASELINES.md` | Five metric producer/config/output records and bounds | ✓ VERIFIED | All five keys are documented; unavailable values remain explicitly pending. Completeness truth is separately failed. |
| `quality.json` | Producer-supported metric registry | ✓ VERIFIED | The corrected 17-03 must-have path exists and passes the artifact query. `tests/cmake/quality.cmake` reads the registry through `-DQUALITY=quality.json`; the local comparator exits 0 and CI uses the same registry. |
| `17-UPGRADE-AUDIT.md` | Settings, runner, pin, consumer and update-policy audit | ✓ VERIFIED | Read-only status, consumers, sources, and bounded dispositions are present. |
| `.planning/preparation/DECISIONS.md` | Reconcile older Dependabot wording | ✓ VERIFIED | DEC.94 history is retained and superseding no-auto-merge policy is linked. |
| `17-BE32-REVIEW.md` | Selector, JUnit, fixture, hash, and hosted disposition | ✓ VERIFIED | Exact set proof and preservation conditions are recorded; no unsupported hosted speedup claim. |
| `052-be32-movie-test-shard.md` | Current candidate evidence and pending status | ✓ VERIFIED | Todo remains pending with both long tests and missing hosted proof identified. |
| `17-QUALITY-REVIEW.md` | Seven scores and impact-based weakest-dimension selection | ✓ VERIFIED | Complete seven-dimension table and tie-break. |
| `17-IMPROVEMENT-CONTRACT.md` | One selected path, exact before/after and proof contract | ✓ VERIFIED | Selects one fixture predicate and claim boundary. |
| `17-IMPROVEMENT-EVIDENCE.md` | Actual outcome and focused proof | ✓ VERIFIED | Direct hash output and active positive/negative controls recorded. |
| `docs/testing.md` | Current producer guidance and evidence links | ✓ VERIFIED | Links canonical `quality.json` and Phase 17 evidence; budgets pass. |
| `docs/maintaining.md` | Current scoped maintenance guidance | ✓ VERIFIED | Existing guidance was audited and remained current; file exists and links to CI procedures. |
| `17-DOC-AUDIT.md` | Final dispositions for original drift items | ✓ VERIFIED | Each audited row has a retained/corrected/blocked disposition. |
| `.planning/PRODUCT-ROADMAP.md` | Current tune-up outcomes and unchanged playability boundary | ✓ VERIFIED | Links evidence and keeps later phase scope intact. |
| `.planning/preparation/PROMPT-COVERAGE.md` | Current measured/unknown prompt coverage | ✓ VERIFIED | Updated rows describe current evidence and limitations. |
| `17-LESSONS-EXPORT.md` | New lessons or evidence-backed none | ✓ VERIFIED | Explicit no-new-lesson result with supporting evidence. |
| `17-DELIVERY-EVIDENCE.md` | Requirement evidence and final handoff | ✓ VERIFIED | Exact local/hosted evidence and open limitations included. |
| `docs/compatibility.md` | Generated compatibility output | ✓ VERIFIED | Freshness check passes; frame-120 row is sourced from the live ledger and retains unknowns/experimental status. |

### Key Link Verification

| From | To | Via | Status | Details |
|---|---|---|---|---|
| `README.md` | `tests/cmake/install_line.cmake` | Extract and execute the verbatim installer | WIRED | The CMake control reads the README line, substitutes the download base, and verifies install/tamper behavior. |
| `tests/tests.cmake` | `tests/policy/inventory.txt` | Include and deferred CMake inventory check | WIRED | Policy tests are included; inventory names/labels are listed and the registration check enforces equality. |
| `.github/workflows/ci.yml` | `scripts/ci-critical-path.sh` | Exact-attempt DAG producer | WIRED | CI required passes the selected complete job payload to the script. |
| `tests/install/tests.cmake` | `TODO-022` | Consumer failure evidence | WIRED (evidence) | The test and exact Windows job observations are tied in `17-RELIABILITY.md`; the link is audit evidence, not a runtime import. |
| `.github/workflows/ci.yml` | `quality.json` | Existing metric producers | WIRED | CI invokes quality checks for Cachegrind, linked WASI, state and the exact-attempt path where its route applies. |
| `tests/cmake/quality.cmake` | `quality.json` | Integer bound validation | WIRED | The checker reads the registry; current local invocation passes. |
| `scripts/apply-repo-settings.sh` | `.github/rulesets/main.json` | Read-only `--check` | WIRED | Script reads the configured ruleset and live settings check exited 0. |
| `pins.txt` | `.github/workflows/ci.yml` | Actual pin consumers | WIRED | CI calls the pin-fetch path, which reads `pins.txt`; direct download/tool consumers are mapped in the audit. |
| `tests/libretro/inventory.txt` | `.github/workflows/ci.yml` | Portable registration | WIRED | CI configures portable CTest labels and runs both named serial tests in the BE32 graph. |
| `tests/cmake/check_junit.cmake` | `tests/inventory/ci.txt` | Exact registered test set | WIRED | The CI workflow passes the inventory path to the JUnit checker. |
| `.github/workflows/ci.yml` | `scripts/ci-critical-path.sh` | Required DAG and hashes artifact | WIRED | `be32` → `hash-equality` → `CI required` dependencies remain required. |
| `17-BASELINES.md` | `17-QUALITY-REVIEW.md` | Evidence for scoring | WIRED (document) | The review cites the baseline report, measured values, and limitations. |
| `17-UPGRADE-AUDIT.md` | `17-IMPROVEMENT-CONTRACT.md` | Conditional update route | WIRED (document) | The contract records why TODO-010 remains unselected. |
| `17-IMPROVEMENT-CONTRACT.md` | `17-IMPROVEMENT-EVIDENCE.md` | Selected route proof | WIRED (document) | Exact selected row and test evidence match the contract. |
| `tests/runner/cheats.c` | `host/cheats.c` | Conditional architecture candidate | N/A | Architecture was not selected; neither file changed, so this conditional branch is not required. |
| `tests/compat/tests.cmake` | `tests/compat/ledger.csv` | Fresh measured frame row | WIRED | Live compatibility producer runs frames 60 and 120, selects the row predicate, and compares the fresh value. |
| `quality.json` | `docs/testing.md` | Canonical current bounds | WIRED | The testing guide links the root registry and Phase 17 producer report. |
| `tests/compat/ledger.csv` | `17-DOC-AUDIT.md` | Source-to-output disposition | WIRED (evidence) | The audit identifies the added row and its generator-owned compatibility output. |
| `17-QUALITY-REVIEW.md` | `.planning/PRODUCT-ROADMAP.md` | Selected outcome and remaining gaps | WIRED (document) | Product roadmap links the one selected improvement and keeps remaining quality gaps. |
| `17-BASELINES.md` | `quality.json` | Producer results and retained bounds | WIRED | The report enumerates the exact five root metric keys and their current bound dispositions. |
| `tests/compat/ledger.csv` | `docs/compatibility.md` | Existing generator and freshness check | WIRED | `scripts/compat-doc.sh ... --check` exits 0; output is generated from ledger/features/capability inputs. |

### Data-Flow Trace (Level 4)

| Artifact | Data variable | Source | Produces real data | Status |
|---|---|---|---|---|
| `quality.json` | `ir_per_frame.testcard` | Cachegrind frames 60/360 from the runner | Yes; exact `1,620,381` Ir/frame | ✓ FLOWING |
| `quality.json` | `ir_per_frame.gpu_motion` | Cachegrind frames 10/60 on the pinned generated fixture | Yes; exact `138,406,446` Ir/frame | ✓ FLOWING |
| `quality.json` | `size.wasm` | Linked `build/wasm32/schwaystation-run` passed to `quality.cmake` | Local exact `1,014,617` exists; final hosted job passed its range check but does not expose the exact integer | ⚠️ STATIC (hosted value unavailable) |
| `quality.json` | `state.bytes` | Stable save/load/save producer on Linux x64 | Yes; exact `3,757,754` bytes | ✓ FLOWING |
| `quality.json` | `ci.critical_path_s` | Exact Actions attempt job payload → `scripts/ci-critical-path.sh` | Historical exact interval exists; no current comparable PR fingerprint. Final workflow_dispatch raw timing is a separate route. | ⚠️ STATIC (current comparable value pending) |
| `docs/compatibility.md` | fixture evidence rows | Runner output → `compat.ratchet.rows` → `tests/compat/ledger.csv` → `scripts/compat-doc.sh` | Yes; fresh frame-120 row uses the pinned fixture predicate and generator check passes | ✓ FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|---|---|---|---|
| Existing and new `gpu_motion` frame predicates agree with their pinned hashes | `ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^fixtures[.]gpu_motion[.]hash[.](60|120|self_test)$'` | Exit 0; 3/3 passed, per `17-IMPROVEMENT-EVIDENCE.md`. | ✓ PASS |
| Live ledger rows, transitions, protected wrong/missing controls, and doc freshness | `ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^compat[.]ratchet[.](rows|transitions|protected_controls)$|^policy[.]compat_doc_fresh$'` | Exit 0; 9/9 passed, per `17-IMPROVEMENT-EVIDENCE.md`. | ✓ PASS |
| Final local configured CI build, packaging and registered test set | `cmake --workflow --preset ci` | Exit 0; 273 build targets and 742/742 CI tests passed, per final orchestrator evidence. | ✓ PASS |
| Final hosted required CI workflow | [Run 46112240653](https://github.com/szTheory/schwaystation/actions/runs/46112240653) | GitHub API confirms `workflow_dispatch`, head `a61d25944125fe42a0f6f59065812c399379fd18`, 18 success jobs, 4 event-inapplicable skips, and successful `CI required`. BE32 passed 694/694 in 5,033s. | ✓ PASS (event-scoped) |

The hosted dispatch confirms required checks for that event. It is not a comparable PR/full-Main timing sample and does not expose the exact WASI module bytes; it is not used to claim a speedup or validate a current `ci.critical_path_s` interval.

### Probe Execution

| Probe | Command | Result | Status |
|---|---|---|---|
| Phase-declared or conventional probe scripts | `find scripts -path '*/tests/probe-*.sh' -type f` and plan/summary probe search | No probe scripts or declared probe paths found; Phase 17 is not a migration/tooling phase. | SKIPPED (none applicable) |

### Test Quality Audit

| Test file | Linked requirement | Active | Skipped | Circular | Assertion level | Verdict |
|---|---|---:|---:|---|---|---|
| `tests/compat/tests.cmake` | TUNE-09 | Yes | 0 | No | Behavioral and value | PASS; fresh runner values are compared with the ledger and protected controls reject wrong/missing frame-120 values. |
| `tests/fixtures/tests.cmake` | TUNE-09 | Yes | 0 | No | Value | PASS; frame-120 expectation is pinned independently in the fixture test. |
| `tests/cmake/quality.cmake` and `quality_controls.cmake` | TUNE-10 | Yes | 0 | No | Bound/value and malformed-input | PARTIAL; comparator and control tests pass, but the exact current hosted WASI value and comparable CI path value remain unavailable. |
| `tests/cmake/check_junit.cmake` | TUNE-07, TUNE-08 | Yes | 0 | No | Exact set/status | PASS; existing checker is wired to the inventory; BE32's one-off cross-report control rejected missing, duplicate, extra, wrong-shard, failure, and skipped cases. |

**Disabled tests on requirement-linked files:** 0. **Circular expected-value patterns:** 0. **Insufficient assertions:** 0 for verified fixture/compatibility claims; TUNE-10 remains partial because producer values are missing, not because an assertion is weak.

### Decision Coverage

The configured decision-coverage gate checked all 13 trackable `17-CONTEXT.md` decisions: 13 honored, 0 not honored. It is non-blocking and does not alter the phase status.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|---|---:|---|---|---|
| — | — | No unreferenced debt markers, placeholder output, empty implementation, or console-only implementation in changed source/test files | None | No blocker found. |

### Unresolved Source-Audit Inputs

The seven probe items serialized in `17-DELIVERY-EVIDENCE.md` remain unresolved as specified by Plan 17-09: TUNE-07 and TUNE-09/TUNE-11 unclassified manual-review flags; TUNE-08 empty-input and text-encoding assumptions; TUNE-10 generic boundary and precision assumptions. They were not silently marked resolved or promoted to new product requirements. The bespoke prohibition probe was empty; no prohibition was inferred as passed from that empty probe.

## Requirements Coverage

| Requirement | Source plans | Status | Evidence |
|---|---|---|---|
| TUNE-07 | 17-02, 17-05, 17-09 | ✓ SATISFIED | Bounded 40-run matching/exclusion analysis, exact critical-path/job evidence, no unmatched speedup claim; BE32 selector and preservation review. |
| TUNE-08 | 17-02, 17-05, 17-07, 17-09 | ✓ SATISFIED | Timeout derivation uses the slowest exact matching job; no timeout or test was weakened. Failure review and TODO-022's three-sample shortfall are explicit; no flaky product-test removal/change is claimed. |
| TUNE-09 | 17-01, 17-06, 17-07, 17-09 | ✓ SATISFIED | Seven linked scores selected compatibility breadth from the minimum tie; the existing fixture's frame-120 result is freshly produced, tested and documented. |
| TUNE-10 | 17-03, 17-08, 17-09 | ✗ BLOCKED | Exact results and assumptions are documented, but the current hosted WASI integer is hidden and no comparable current `ci.critical_path_s` producer result exists. `quality.json` remains unchanged; passing checks do not establish the missing measurements. |
| TUNE-11 | 17-01, 17-04, 17-06, 17-07, 17-08, 17-09 | ✓ SATISFIED | Read-only settings/pin audit, update-policy reconciliation, scoped guidance, roadmap/prompt coverage, compatibility freshness and evidence-backed lesson disposition. |

All five Phase 17 requirement IDs appear in at least one plan; no Phase 17 requirement is orphaned. No later milestone phase addresses the TUNE-10 metric evidence gap.

### Advisory (New Scope, Unevidenced)

None. This re-verification found no new-scope concern that lacked deterministic evidence.

## Human Verification Required

N/A — no owner-machine, BIOS/disc, visual, audio, or interactive playability check is part of Phase 17. Installation was exercised under isolated HOME, compatibility output has a machine-checked generator, and the final hosted run is observable through GitHub's job API.

## Gaps Summary

Plan 17-03's corrected repository-root `quality.json` artifact path remains verified. The remaining TUNE-10 gap is current evidence: the latest hosted `workflow_dispatch` run confirms only that the hidden WASI module size falls within the retained interval, while its event/workload fingerprint cannot establish a current comparable `ci.critical_path_s` result. No speedup or current-bound claim is inferred from that run. The delivery handoff says owner direction is needed before adding gap-closure scope or crossing the ship boundary; no gap plan is started here.

---

_Verified: 2026-11-28T07:59:32Z_
_Verifier: the agent (gsd-verifier)_
