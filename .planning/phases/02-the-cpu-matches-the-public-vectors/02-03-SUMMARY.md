---
phase: 02-the-cpu-matches-the-public-vectors
plan: 03
subsystem: testing
tags: [cmake, ctest, expect_fail, gate-budget, gitattributes, requirements, docs]

requires:
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: per-directory test inventories (plan 02-01)
provides:
  - schway_add_test PASS_REGEX runs through expect_fail.cmake with CODE 0, so a non-zero exit fails even when the output matches
  - policy.pass_regex_status.self_test proving matching text with exit 1 is refused
  - policy.gate_budget limit 1520 and a docs/*/*.md budget of 61,440 bytes in docs.budgets
  - REQUIREMENTS paragraph naming the phase-2 gate budget and the gates the phase adds, and CPU-05's fuzz clause reworded
  - .gitattributes lines for fixture sources (LF) and the vector sample (binary)
  - docs/testing.md with separate rehearsal and cleanup rows, the six phase labels and the tests/diag data directory
affects: [02-04, 02-05, 02-06, 02-07, 02-27]

actuals:
  tokens: 4200
  tasks: 3
  commits: 3

plan_head_before: 74f035a18eb091d27438c2998b4d257799064277
plan_head_after: 13e44a8e2ec94cd65b6cad997c388798477cba09
commits: 3

tech-stack:
  added: []
  patterns:
    - "PASS_REGEX callers keep their signature; the helper wraps the command, replacing semicolons with a generator-expression separator"
    - "a nested negative control writes a small driver script at configure and includes expect_fail.cmake from it, because a second wrapper level cannot carry the escaped command list"

key-files:
  created: []
  modified:
    - cmake/SchwaystationTests.cmake
    - tests/policy/tests.cmake
    - tests/policy/inventory.txt
    - tests/hygiene/tests.cmake
    - tests/cmake/budgets.cmake
    - .gitattributes
    - .planning/REQUIREMENTS.md
    - docs/testing.md

key-decisions:
  - "The docs/*/*.md glob joins the existing docs/*.md glob in one file(GLOB) call, adding no line to the counted gate code"
  - "policy.pass_regex_status.self_test runs the inner run through a configure-time driver script rather than a second nested -DCMD, since the escaped list does not survive two generator-expression evaluations"

requirements-completed: [CPU-01, CPU-02, CPU-03, CPU-04, CPU-05]

coverage:
  - id: D1
    description: "schway_add_test PASS_REGEX commands run through expect_fail.cmake with CODE 0, and a command that prints the matching text but exits 1 is refused"
    requirement: "CPU-05"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R policy.pass_regex_status.self_test (exit 0)"
        status: pass
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^(runner.hash|runner.version)$' (exit 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: "policy.gate_budget holds repository-check code at or under 1520 lines (measured 1392) and docs.budgets limits docs/*/*.md pages to under 61,440 bytes"
    requirement: "CPU-05"
    verification:
      - kind: integration
        ref: "cmake --workflow --preset hygiene (exit 0) and check_junit.cmake on build/hygiene/junit.xml (exit 0)"
        status: pass
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^docs\\.budgets$' (exit 0)"
        status: pass
    human_judgment: false
  - id: D3
    description: "REQUIREMENTS names the 1,520-line phase budget and the gates the phase adds, keeps FRAME-05's 1,392 sentence, and words CPU-05 so fuzz.regress.exe gates on every leg while the nightly campaign is report-only"
    requirement: "CPU-05"
    verification:
      - kind: other
        ref: "grep -q '1,520' .planning/REQUIREMENTS.md and the 1,392, docs.api_names and D-33 amended at plan time greps (exit 0)"
        status: pass
    human_judgment: false
  - id: D4
    description: "Fixture sources keep LF line ends and the vector sample is named binary"
    requirement: "CPU-01"
    verification:
      - kind: other
        ref: "git check-attr text -- tests/fixtures/src/stub/stub.asm prints text: set; git check-attr binary -- tests/vectors/r3000/sst-sample.bin prints binary: set (exit 0)"
        status: pass
    human_judgment: false
  - id: D5
    description: "docs/testing.md has separate rehearsal and cleanup rows and lists the cpu, bus, vectors, fuzz, exe and fixtures labels, the PASS_REGEX exit-status rule and the tests/diag data directory"
    requirement: "CPU-05"
    verification:
      - kind: other
        ref: "grep -qE '^\\| `rehearsal` \\|' docs/testing.md and the cleanup and fixtures row greps (exit 0)"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci (exit 0, 81 tests)"
        status: pass
    human_judgment: false

duration: 6min
completed: 2026-11-09
status: complete
---

# Phase 2 Plan 3: Test helper exit status, 1520 gate budget, docs rules Summary

**PASS_REGEX tests now fail on a non-zero exit even when their output matches (through expect_fail.cmake with CODE 0), with the 1520-line gate budget, the docs/*/*.md byte budget, the phase's gate names in REQUIREMENTS, fixture attributes and the testing guide's new rows all in place.**

## Performance

- **Duration:** about 6 min
- **Started:** 2026-11-09T18:51:00Z (approximate)
- **Completed:** 2026-11-09T18:57:00Z
- **Tasks:** 3
- **Files modified:** 8

## Accomplishments
- `schway_add_test(... PASS_REGEX re)` registers `cmake -DCMD=... -DCODE=0 -DREGEX=... -P tests/cmake/expect_fail.cmake`; the five existing callers (runner hashes, runner version, install consumers, embed) keep passing and the `PASS_REGULAR_EXPRESSION` property is gone from the helper.
- `policy.pass_regex_status.self_test` (labels `policy portable`) runs a command that prints `pass-regex-match` and exits 1 under the CODE 0 wrapper, and expects the wrapper's `expected exit 0` refusal.
- `policy.gate_budget` limit is 1520 (measured total today is 1392, no lines added); `docs.budgets` also globs `docs/*/*.md` at 61,440 bytes.
- REQUIREMENTS records the phase budget and the seven gate names after CPU-05, with CPU-05's fuzz clause edited per the plan-time D-33 amendment; no checkbox changed and FRAME-05's 1,392 line is untouched.
- `.gitattributes` keeps `/tests/fixtures/src/**` at LF and names `/tests/vectors/r3000/sst-sample.bin` binary.
- `docs/testing.md` gains separate `rehearsal` and `cleanup` rows, six label rows, a directory note including `tests/diag/`, and the PASS_REGEX exit-status sentence.
- This plan closes todos 023 (test-helper exit status) and 020 (rehearsal and cleanup rows); plan 02-27 alone moves the todo files, so they stay in `.planning/todos/pending/` for now.

## Task Commits

1. **Task 1: Tracer - PASS_REGEX with a non-zero exit is refused** - `c90fac8` (test)
2. **Task 2: The 1520 budget, the docs glob, the attributes and the REQUIREMENTS paragraph** - `7259c26` (chore)
3. **Task 3: The testing guide's rows and the new directories** - `13e44a8` (docs)

**Plan metadata:** the docs(02-03) commit following this summary.

## Files Created/Modified
- `cmake/SchwaystationTests.cmake` - PASS_REGEX wraps the command in expect_fail.cmake with CODE 0
- `tests/policy/tests.cmake` - `policy.pass_regex_status.self_test` and its configure-time scripts
- `tests/policy/inventory.txt` - the new test's line
- `tests/hygiene/tests.cmake` - gate budget limit 1520
- `tests/cmake/budgets.cmake` - `docs/*/*.md` glob
- `.gitattributes` - fixture source LF line ends, vector sample binary
- `.planning/REQUIREMENTS.md` - phase-2 budget and gate paragraph, CPU-05 fuzz wording
- `docs/testing.md` - CI rows, labels, directories, PASS_REGEX note, docs glob note

## Decisions Made
- The `docs/*/*.md` glob joins the existing glob in one `file(GLOB)` call, so the counted gate code stays at 1392 lines.
- The self test nests through a configure-time driver script (`policy/pass_regex_driver.cmake`) instead of a second `-DCMD`, since the escaped command list does not survive two generator-expression evaluations.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Nested negative control cannot carry its command list through two wrappers**
- **Found during:** Task 1
- **Issue:** the plan's shape (outer `expect_fail.cmake` running an inner `expect_fail.cmake` with its own `CMD`) needs the inner list's semicolons escaped twice; generator expressions evaluate once, so the outer wrapper would receive a flattened list.
- **Fix:** write `exit_one.cmake` and a small driver `pass_regex_driver.cmake` at configure; the driver sets `CMD`, `CODE` 0 and `REGEX` and includes `expect_fail.cmake`, and the outer wrapper (CODE 1, REGEX `expected exit 0`) runs the driver. The same CODE 0 comparison the helper uses is what gets exercised.
- **Files modified:** tests/policy/tests.cmake
- **Verification:** `ctest -R policy.pass_regex_status.self_test` passes; the verbose log shows the outer wrapper matching `expected exit 0`.
- **Committed in:** c90fac8

---

**Total deviations:** 1 auto-fixed (1 blocking)
**Impact on plan:** None on scope; same behaviour proven, mechanism differs in one configure-time file.

## Issues Encountered
None

## Known Stubs
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- The helper, the 1520 ceiling and the gate names are in place for plans 02-04 onward; later plans may add gate lines up to 1520 and must name each gate's fault.
- `tests/diag/` is listed in the testing guide but does not exist yet; the plan that creates it should confirm the description.

## Self-Check: PASSED

- Commits `c90fac8`, `7259c26`, `13e44a8` are ancestors of HEAD.
- Modified files exist; `cmake --workflow --preset ci` (exit 0) and `cmake --workflow --preset hygiene` with the JUnit check (exit 0) pass.

---
*Phase: 02-the-cpu-matches-the-public-vectors*
*Completed: 2026-11-09*
