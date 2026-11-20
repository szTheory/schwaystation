---
phase: 09-chd-disc-images
plan: 08
subsystem: testing
tags:
  - compatibility
  - feature-ledger
  - CMake
  - POSIX-shell
requires:
  - phase: 09-01
    provides: Existing compatibility ledger, CTest controls, and protected-base judge
provides:
  - Sorted feature evidence ledger with schema and bounded preflight validation
  - Protected feature transitions tied to fresh results and dated allowances
  - Generated compatibility documentation for title and feature evidence
affects:
  - Phase 9 future provider evidence plans
  - Phase 9 Plan 11 protected checkout workflow
actuals:
  tokens: 13596
  tasks: 2
  commits: 3
plan_head_before: 43ede68822c80c38a1f1d4a096c76f0883985bb3
plan_head_after: 45b78672f61c3f8fdc94461416762eb10d7e8eff
commits: 3
tech-stack:
  added: []
  patterns:
    - Schema-driven CMake ledger validation shares title and feature parsing.
    - Feature tuple sorting uses a separator lower than every permitted key character.
    - Protected delegation adds a kind argument only for feature-capable judges.
key-files:
  created:
    - tests/compat/features.csv
    - scripts/compat-controls.sh
    - tests/compat/compat-control-cases.tsv
  modified:
    - scripts/compat-ratchet.sh
    - scripts/compat-doc.sh
    - tests/cmake/compat-ratchet.cmake
    - tests/cmake/compat-controls.cmake
    - tests/compat/tests.cmake
    - tests/compat/inventory.txt
    - docs/compatibility.md
key-decisions:
  - Feature identity is provider, profile, protection, peripheral, and predicate.
  - Production allowance checks use the current UTC date; controls inject fixed dates for reproducible boundaries.
  - Preserve the five-argument title judge interface; append the kind argument only for feature-capable protected judges.
requirements-completed:
  - COMPAT-02
coverage:
  - id: FEATURE-PREFLIGHT
    description: The feature ledger rejects malformed, duplicate, missing, unsorted, and over-limit rows before broad compatibility tests.
    requirement: COMPAT-02
    verification:
      kind: unit
      ref: "ctest --preset ci --no-tests=error -R '^compat[.]features[.]' --output-on-failure; 2/2 passed, exit 0."
      status: pass
    human_judgment: false
  - id: PROTECTED-FEATURE-RATCHET
    description: Feature results must match independent fresh evidence, and protected status changes require an allowed transition or matching dated allowance while legacy title delegation remains compatible.
    requirement: COMPAT-02
    verification:
      kind: integration
      ref: "The feature/ratchet selector passed 5/5 with the actual origin/main base; COMPAT_BASE_DIR and COMPAT_BASE_SHA with selector '^compat[.]ratchet[.]rows$' passed 6/6 including required fixture setup, exit 0."
      status: pass
    human_judgment: false
  - id: COMPATIBILITY-DOCUMENTATION
    description: Generated compatibility documentation includes ordered title results and feature evidence and fails freshness checks when stale.
    requirement: COMPAT-02
    verification:
      kind: unit
      ref: "ctest --preset ci --no-tests=error -R '^policy[.]compat_doc_fresh$' --output-on-failure; 1/1 passed, exit 0."
      status: pass
    human_judgment: false
duration: 2h 05m
completed: 2026-11-20
status: complete
---

# Phase 9 Plan 09-08 Summary

**A sorted feature evidence ledger now has a cheap schema preflight, protected fresh-result ratchet, legacy title compatibility, and generated documentation.**

## Performance

- **Duration:** 2 hours 5 minutes from the saved Plan 09-07 state checkpoint at 2026-11-20T12:51:00Z to closeout.
- **Started:** 2026-11-20T12:51:00Z
- **Completed:** 2026-11-20T14:55:33Z
- **Tasks:** 2
- **Files changed:** 10
- **Runtime/model:** Codex runtime; executor model gpt-6-luna.

## Accomplishments

- Added the unknown CHD feature row and stable inventory entry. The key covers provider, profile, protection, peripheral, and predicate.
- Added feature schema, full-identity, tuple-order, row-count, date, and allowance checks. The production path uses the current UTC date, and controls inject a fixed date.
- Kept title ratchet calls at five arguments for older protected judges. Feature mode receives the sixth kind argument only when the protected base supports it.
- Compared the checked-in feature ledger with an independent expected-unknown fixture so changing the ledger and regenerating docs cannot certify itself.
- Preserved title controls and added feature transition cases for missing evidence, regressions, removals, allowance errors, row-count bounds, and tuple-prefix sorting.

## Verification and Efficiency

- LC_ALL=C sort -cu tests/compat/inventory.txt passed.
- The shipping-helper line budget passed at 3901/3901 lines after review correction. sh -n passed for the changed shell helpers, and cmake --preset ci configured successfully.
- The feature selector passed 2/2. The feature/ratchet selector passed 5/5 with COMPAT_BASE_DIR set to a shared clone of origin/main at 830bf4cfe74aca92930e34fdf9667c5c1b24c370.
- The direct compat.ratchet.rows check against that same five-argument judge passed 6/6 including CTest's five required fixture setups. The documentation freshness check passed 1/1.
- Code review found that protection and peripheral were absent from feature identity. The ratchet and allowances now bind all five feature fields; a changed prior combination and candidate/fresh context mismatch both have negative controls. The post-fix selector for feature ratchet, title transitions and generated-document freshness passed 4/4 in 1.91s.
- `COMPAT_TODAY=2026-11-20 sh scripts/compat-doc.sh tests/compat/ledger.csv docs/compatibility.md --check` passed. The Plan 09-07 and 09-08 summary EOF blanks were removed in commit 9224eac; both branch and HEAD diff checks pass.
- The scenario runner moved from the large CMake control file into a counted POSIX shell helper, with a small CMake adapter and fixture-only TSV. This retained all title and feature assertions within the hard line budget. No full suite or hosted workflow run was needed because this plan changed no workflow.

## Task Commits

1. **Task 1 RED probes** — aa18277 (test(09-08): add feature ledger schema probes)
2. **Task 1 implementation** — d40e18b (feat(09-08): add protected feature compatibility ratchet)
3. **Task 2 controls and docs** — 45b7867 (test(09-08): cover protected feature ledger transitions)

## Deviations from Plan

The control scenarios remain CTest-registered through tests/cmake/compat-controls.cmake, but the runner implementation moved to POSIX shell so every scenario stays inside the counted helper budget. The plan's cases and assertions were retained.

The sparse-checkout workflow entry for tests/compat/features.csv is carried forward to Plan 09-11, which owns .github/workflows/ci.yml. This plan leaves the workflow unchanged.

## Issues Encountered

The empty-key negative control initially expected a different CMake diagnostic; its regex now accepts the parser's row-width rejection, preserving the required failure behavior. The first legacy-base invocation used a relative path from CTest's build directory; rerunning with the absolute clone path passed. No implementation issue remains.

## Next Plan Readiness

COMPAT-02 feature evidence is ready for later providers. Plan 09-11 must add tests/compat/features.csv to the protected-base sparse-checkout list. No work from Plan 09-09 was started.

---
*Phase: 09-chd-disc-images*
*Completed: 2026-11-20*

## Self-Check: PASSED
