---
phase: 04-games-play-in-retroarch
plan: "24"
subsystem: testing
tags: [compatibility, cmake, ctest, posix-shell]

requires:
  - phase: "04-37"
    provides: "Ten-column compatibility ledger and measured fixture rows"
  - phase: "04-40"
    provides: "Checked OpenBIOS shell frame result"
  - phase: "04-13"
    provides: "Fixture hash tests and runner result capture"
  - phase: "04-01"
    provides: "CTest registration and per-directory test inventories"
provides:
  - "Ten-column compatibility ratchet with serial+MD5+predicate keys and optional prior ledger"
  - "Fresh fixture-result comparison in both directions"
  - "Generated compatibility documentation and stale-output test"
affects: ["GAME-06", "04-41", "compatibility"]

actuals:
  tokens: 8689
  tasks: 2
  commits: 11
  plan_head_before: 4bb984b4f5049a15c74b1c3e5bcdc701d04083a3
  plan_head_after: 4b19188d0a0c79774e8c6d296410b4a858876b28

tech-stack:
  added: []
  patterns: ["CMake -P", "POSIX shell", "CTest fixture dependencies"]
key-files:
  created:
    - "docs/compatibility.md"
    - "scripts/compat-doc.sh"
    - "scripts/compat-ratchet.sh"
    - "tests/cmake/compat-controls.cmake"
    - "tests/cmake/compat-ratchet.cmake"
    - "tests/compat/allow.txt"
  modified:
    - ".planning/preparation/CONFORMANCE.md"
    - "tests/compat/compat_tests.c"
    - "tests/compat/inventory.txt"
    - "tests/compat/ledger.csv"
    - "tests/compat/tests.cmake"
key-decisions:
  - "Use serial+MD5+predicate as the row key and compare fresh fixture keys and statuses in both directions."
  - "Keep protected PR-base lookup and artifact capture in 04-41; expose the ratchet with a prior-ledger argument."
  - "Require behavior-revision 4 allowances with an inclusive validity window of at most 60 days."

patterns-established:
  - "The compatibility row test depends on existing fixture producers and consumes their same-run outputs."
  - "The docs generator validates the ledger through the same ratchet schema before rendering."

requirements-completed: []
coverage:
  - id: D1
    description: "Ten-column compatibility transitions are judged against fresh fixture results."
    requirement: "GAME-06"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^compat\\.ratchet\\.rows$' --no-tests=error --output-on-failure"
        status: pass
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^compat\\.ratchet\\.transitions$' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: "Generated compatibility docs show stable counts, confidence, evidence and measurement dates."
    requirement: "GAME-06"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^policy\\.compat_doc_fresh$' --no-tests=error --output-on-failure"
        status: pass
      - kind: other
        ref: "cmake --workflow --preset ci"
        status: pass
    human_judgment: false

duration: 35m
completed: 2026-11-12
status: complete
---

# Phase 04 Plan 24: Compatibility ledger and generated docs Summary

**Measured fixture compatibility results now use a ten-column ratchet, with generated docs that expose stable counts, confidence, evidence and dates.**

## Performance

- **Duration:** 35 minutes for the resumed implementation; the earlier schema slice was checkpointed.
- **Started:** 2026-11-12T03:10:11Z
- **Completed:** 2026-11-12T03:45:11Z
- **Runtime/model:** Codex, OpenGSD 1.20.0; sequential executor in the shared phase checkout; gpt-6-luna xhigh.
- **Tasks:** 2
- **Plan implementation files:** 11

## Accomplishments

- Added a CMake ratchet for schema validity, duplicate keys, the `unknown`/`fail`/`pass` transitions, behavior-revision 4 allowances, and exact candidate-to-fresh matching in both directions.
- Connected the live compatibility row test to fresh GPU runner output, pad/card fixture captures, and the checked OpenBIOS shell tuple.
- Generated sorted status/region/media counts and title rows with confidence, evidence and measurement dates; registered stale-document and invalid-field controls.
- Updated the conformance reference from the obsolete nine-column example to the ten-column ledger and derived key.

## Task Commits

1. **Task 1: Judge ten-column compatibility rows against real fixture results** — `59ad2c5` (RED), `2b8c94c` (schema/data GREEN), and `4b19188` (completed judge, fresh comparison and docs).
2. **Task 2: Expand and document the proven path** — `4b19188` (expansion controls landed with the remaining Task 1 implementation).

The measured commit interval from `4bb984b` to `4b19188` contains 11 commits because plan 04-24 paused while plan 04-27 and the approved budget update were committed. The task implementation commits are listed above.

## Evidence

- `cmake --preset ci` — exit 0.
- `cmake --build --preset ci --target schwaystation-test-compat schwaystation-run schwaystation-fixtures` — exit 0.
- `ctest --test-dir build/ci -R '^(compat\\.ratchet\\.rows|compat\\.ratchet\\.transitions|policy\\.compat_doc_fresh)$' --no-tests=error --output-on-failure` — exit 0; the four fixture setup tests and all three compatibility tests passed.
- `cmake --workflow --preset ci` on HEAD `4b19188` — exit 0, including 482 CI tests, packaging and 5 release checks.
- `SCHWAYSTATION_TIMEOUT_SCALE=4 cmake --workflow --preset hygiene` — exit 0; all 23 hygiene checks passed. The task commit's active pre-commit hook also completed successfully.

## Decisions Made

- The row key is `(serial, md5, predicate)`. A changed passing frame hash therefore requires an allowance for the prior key.
- The ratchet accepts the prior ledger as an argument so the later workflow integration can load the actual PR base. Workflow artifact extraction remains in 04-41.
- Allowances identify behavior revision 4 and expire no later than 60 days after their start date.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Preserved semicolons in evidence while assembling fresh rows**
- **Found during:** Task 1, focused compatibility verification.
- **Issue:** CMake list parsing split semicolon-bearing evidence into extra fields and rejected a valid live ledger.
- **Fix:** Escaped semicolons before list expansion and rejected collision with the reserved parser token.
- **Files modified:** `tests/compat/tests.cmake`.
- **Verification:** The focused compatibility CTest selection and full CI both passed.
- **Committed in:** `4b19188`.

### Task-order deviation

The resumed implementation completed the remaining Task 1 judge/docs work and Task 2 expansion controls in the same commit, `4b19188`. The earlier RED and schema/data GREEN commits remain intact. No second RED was claimed for regression tests around already-implemented behavior. This is a recorded task-order deviation; it does not leave implementation or verification work outstanding.

The deviation is also recorded as waived in `.planning/WINDOWS.md`; its source and validation work are complete.

**Total deviations:** 1 auto-fixed bug and 1 task-order deviation.
**Impact on plan:** The behavior, registered controls, documentation and verification criteria are complete. The two resumed tasks share the final implementation commit.

## Issues Encountered

The first live-row control run exposed the semicolon parser issue above. The corrected reader passed the focused checks and full CI.

## User Setup Required

None.

## Next Phase Readiness

The ratchet accepts an optional prior ledger for 04-41 to supply from the protected PR base. The workflow's base capture and artifact plumbing remain for 04-41. GAME-06 remains partial until its later compatibility-library work is complete.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-12*

## Self-Check: PASSED

Summary and implementation files exist, and commits `59ad2c5`, `2b8c94c`, and `4b19188` are ancestors of HEAD.
