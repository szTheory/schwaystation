---
phase: 04-games-play-in-retroarch
plan: "41"
subsystem: testing
tags: [compatibility, github-actions, cmake, posix-sh, tdd]
requires:
  - phase: "04-24"
    provides: [compatibility ledger, shell ratchet judge]
provides:
  - Exact pull-request base checkout and protected compatibility judge capture
  - Fail-closed baseline selection and registered protected-control tests
affects: [compatibility CI, phase-04 verification]
actuals:
  tokens: 4334
  tasks: 2
  commits: 3
plan_head_before: 44e37975acaec42641516b00a0b8aed4ba463f3c
plan_head_after: f22aa2e2694c903abaac4803054b94cff006cb25
tech-stack:
  added: []
  patterns: [capture exact pull-request base before head checkout, preserve protected checkout outside workspace cleanup]
key-files:
  created:
    - .planning/phases/04-games-play-in-retroarch/deferred-items.md
  modified:
    - .github/workflows/ci.yml
    - scripts/compat-ratchet.sh
    - tests/compat/tests.cmake
    - tests/compat/inventory.txt
    - docs/testing.md
    - .planning/preparation/CONFORMANCE.md
key-decisions:
  - "Only a base with both the ledger and shell judge absent selects first-introduction mode; malformed or mismatched protected bases fail closed."
  - "Copy the exact sparse base checkout to RUNNER_TEMP before the root head checkout, preserving its Git metadata and protected judge dependencies."
patterns-established:
  - "Protected baseline pattern: validate the supplied base SHA, copy its shell judge, CMake judge and ledger, then execute that copied judge."
requirements-completed: []
coverage:
  - id: D1
    description: "Compatibility comparisons use the exact pull-request base judge; only an absent-both baseline selects introduction."
    requirement: GAME-06
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^compat\\.ratchet\\.baseline$' --no-tests=error --output-on-failure"
        status: pass
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^compat\\.ratchet\\.protected_controls$' --no-tests=error --output-on-failure"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset hygiene"
        status: pass
    human_judgment: false
duration: 23min
completed: 2026-11-12
status: complete
---

# Phase 4 Plan 41: Protected Compatibility Baseline Summary

**The compatibility ratchet now captures and executes the judge from the exact pull-request base, with tests proving malformed baselines and head-judge deletion cannot weaken later comparisons.**

## Performance

- **Duration:** 23 minutes
- **Started:** 2026-11-12T04:04:15Z
- **Completed:** 2026-11-12T04:27:19Z
- **Tasks:** 2
- **Files modified:** 7

## Accomplishments

- Added a registered regression test that proves a protected prior pass is still rejected when the head judge accepts every result.
- Captured the pull request's exact `base.sha` before head checkout, preserved the checkout in `RUNNER_TEMP`, verified its Git `HEAD`, and copied the base shell judge, CMake judge dependency and ledger before execution.
- Registered and documented fail-closed controls for absent-both introduction, each one-missing case, missing judge dependency, SHA mismatch and deleted head helper.

## Evidence

- Task 1 RED: registered `compat.ratchet.baseline` failed when run against the deliberately accepting head judge. The accepted RED evidence was produced through the repository's supported unittest reporter after the first JUnit-format evidence attempt was rejected as unsupported.
- Task 1 GREEN: `cmake --preset ci`, `cmake --build --preset ci --target schwaystation-test-compat schwaystation-run schwaystation-fixtures`, and `ctest --test-dir build/ci -R '^compat\\.ratchet\\.baseline$' --no-tests=error --output-on-failure` — exit 0.
- Task 2: the same configure/build command and `ctest --test-dir build/ci -R '^compat\\.ratchet\\.protected_controls$' --no-tests=error --output-on-failure` — exit 0.
- Final `cmake --workflow --preset ci` — exit 0; all 486 CI tests and all 5 release checks passed.
- Final `cmake --workflow --preset hygiene` — exit 0; all 23 hygiene checks passed.
- `git diff --check`, byte-sorted inventory validation and `actionlint -ignore 'label "ubuntu-26.04(-arm)?" is unknown' .github/workflows/ci.yml` — exit 0. The ignore is limited to pre-existing runner labels; the workflow edit itself passed actionlint.
- The measured repository-check implementation count is 2292/2300 lines. No policy logic was moved out of counted paths or weakened.
- No hosted PR workflow run was made from this executor. The hosted event requires the root-owned exact-head workflow process; no hosted success is claimed here.

## Task Commits

1. **Task 1 RED: protected baseline regression test** — `febbea3` (`test`)
2. **Task 1 GREEN: protect compatibility ratchet with PR base judge** — `20a91c3` (`feat`)
3. **Task 2: protected baseline fail-closed cases and workflow capture** — `f22aa2e` (`test`)

The separate plan metadata commit contains this summary and the state/roadmap updates; its hash is reported by the executor after commit.

## Files Created/Modified

- `.github/workflows/ci.yml` — captures the exact PR base before head checkout and preserves it outside workspace cleanup.
- `scripts/compat-ratchet.sh` — verifies the base SHA and invokes a copy of the base shell judge, CMake judge and ledger.
- `tests/compat/tests.cmake` and `tests/compat/inventory.txt` — register the baseline regression and protected-control tests.
- `docs/testing.md` and `.planning/preparation/CONFORMANCE.md` — document the capture and protected-judge contract.
- `.planning/phases/04-games-play-in-retroarch/deferred-items.md` — records the local actionlint runner-label limitation.

## Decisions Made

- The base commit alone selects first introduction; it does so only when both the ledger and shell judge are absent.
- The sparse checkout is copied to `RUNNER_TEMP` before root head checkout so workspace cleanup cannot discard the protected base files or Git metadata.
- `GAME-06` remains partial at the phase level, so this plan records coverage but does not mark the requirement complete.

## Deviations from Plan

### Execution bookkeeping

- The plan commit ledger was initialized after the first two task commits. The original parent was verified as `44e37975acaec42641516b00a0b8aed4ba463f3c` (`febbea3^`), and the persisted ledger was initialized from that verified commit before Task 2. The measured task-commit count is 3; no history was rewritten.
- The plan text contains an older 1850-line ceiling. Execution followed the current owner-approved 2300-line ceiling and finished at 2292 lines, without moving or shortening counted gate code.
- Workflow inspection showed that the nested workspace base checkout could be removed by the subsequent head checkout. The workflow now copies it to `RUNNER_TEMP` before checkout and verifies the copied Git `HEAD` against the exact base SHA.

### Auto-fixed issues

- The local `actionlint` runner catalog does not recognize the repository's existing `ubuntu-26.04` and `ubuntu-26.04-arm` labels. Those unrelated labels were left intact, recorded in `deferred-items.md`, and ignored narrowly for validation of this workflow change.

**Total deviations:** 4 bookkeeping/correctness adjustments; no scope expansion. The implementation and controls remain within the approved gate ceiling.

## Issues Encountered

- The first RED evidence reporter did not support the emitted JUnit format. That evidence was discarded; the registered failing test was rerun through the supported unittest reporter, which accepted the failure as RED before implementation.
- The actual hosted pull-request event remains unrun here. Root-owned exact-head hosted validation can confirm workflow behavior after the branch is pushed; no user action is needed for this local plan closeout.

## User Setup Required

None.

## Next Phase Readiness

The compatibility baseline code and local controls are ready for the scheduled independent code review. That review must happen before final phase verification in plan 04-32. The hosted PR workflow result is still needed before making a hosted-CI claim.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-12*

## Self-Check: PASSED

- Summary and implementation files exist.
- Task commits `febbea3`, `20a91c3` and `f22aa2e` are ancestors of the current plan head.
- Recorded plan head and measured task-commit count match the persisted plan ledger.
