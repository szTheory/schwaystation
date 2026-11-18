---
phase: 01-a-test-frame-in-retroarch
plan: 07
subsystem: testing, policy, docs
tags: [cmake, ctest, perf, cachegrind, budgets, no-skip, docs]

requires:
  - phase: 01-01
    provides: "schway_add_test, expect_fail.cmake, check_junit.cmake and the per-directory inventories for perf and policy"
  - phase: 01-06
    provides: "policy.gate_budget at 1500 lines over scripts/*.sh, tests/cmake/*.cmake and .githooks/*"
provides:
  - "tests/cmake/perf_compare.cmake: integer Ir per frame comparison, mode from SCHWAYSTATION_PERF_MODE (report | gate)"
  - "seven perf.gate.* cases on synthetic measurements; tests/perf/allow.txt with no exceptions"
  - "docs.budgets (README 16384, header 40960, docs/*.md 61440, strictly below), policy.no_skip and policy.no_skip.self_test"
  - "docs/testing.md, with an empty CI jobs heading for plan 01-12"
affects: [01-12, 01-14, 01-15]

actuals:
  tokens: 5700
  tasks: 3
  commits: 4
plan_head_before: ce7f3296261ff3522c490a373fa74c0434e67d6d
plan_head_after: 247a04b4a01fdbfd7939a42e23945dd8374a75a1

tech-stack:
  added: []
  patterns:
    - "The perf mode is an environment variable; the test helper sets ENV{SCHWAYSTATION_PERF_MODE} before expect_fail.cmake runs the script, so the child inherits it"
    - "Two-digit date fields are read with math(EXPR \"1${field} - 100\"), which avoids both octal reading and a string(REGEX) call that would clobber CMAKE_MATCH_n"

key-files:
  created:
    - tests/cmake/perf_compare.cmake
    - tests/cmake/budgets.cmake
    - tests/cmake/no_skip.cmake
    - tests/perf/allow.txt
    - docs/testing.md
  modified:
    - tests/perf/tests.cmake
    - tests/perf/inventory.txt
    - tests/policy/tests.cmake
    - tests/policy/inventory.txt

key-decisions:
  - "perf_compare.cmake reads its mode only from SCHWAYSTATION_PERF_MODE, not from -DMODE; an unset mode is an input error, and perf.gate.missing covers it"
  - "The seed's ROM parameter and allow-file column are named FIXTURE and <fixture>: a disc-based console has no ROM"
  - "budgets.cmake resolves ROOT with file(REAL_PATH) before globbing, because -DROOT=. silently matched no docs page"

requirements-completed: [FRAME-01, FRAME-04, FRAME-05, FRAME-09]

coverage:
  - id: D1
    description: "perf_compare.cmake computes the Ir per frame delta in basis points with integer math; exactly +1.00 % passes and +1.01 % fails in gate mode, while report mode exits 0 on FAIL"
    requirement: FRAME-09
    verification:
      - kind: integration
        ref: "tests/perf/tests.cmake#perf.gate.plus_1_00"
        status: pass
      - kind: integration
        ref: "tests/perf/tests.cmake#perf.gate.plus_1_01"
        status: pass
      - kind: integration
        ref: "tests/perf/tests.cmake#perf.gate.identical"
        status: pass
    human_judgment: false
  - id: D2
    description: "A current allow line raises the limit and an expired one does not; unequal bases give UNSTABLE; no base gives NO_BASE, failing only in gate mode; a missing measurement or unset mode is an input error in either mode"
    requirement: FRAME-09
    verification:
      - kind: integration
        ref: "tests/perf/tests.cmake#perf.gate.allowed"
        status: pass
      - kind: integration
        ref: "tests/perf/tests.cmake#perf.gate.unstable"
        status: pass
      - kind: integration
        ref: "tests/perf/tests.cmake#perf.gate.no_base"
        status: pass
      - kind: integration
        ref: "tests/perf/tests.cmake#perf.gate.missing"
        status: pass
    human_judgment: false
  - id: D3
    description: "docs.budgets keeps README.md under 16384 bytes, include/schwaystation.h under 40960 and each docs/*.md under 61440, naming each file at or over; an empty docs glob passes"
    requirement: FRAME-04
    verification:
      - kind: integration
        ref: "tests/policy/tests.cmake#docs.budgets"
        status: pass
      - kind: other
        ref: "cmake -DROOT=. -P tests/cmake/budgets.cmake exit 0; ROOT at a scratch copy of the header and README only exit 0; the same with a 61440-byte docs page exit 1, with a 61439-byte page exit 0"
        status: pass
    human_judgment: false
  - id: D4
    description: "policy.no_skip fails naming file and line when a test file sets a skip or disable property; policy.no_skip.self_test proves it with exit 1 and the named line"
    requirement: FRAME-05
    verification:
      - kind: integration
        ref: "tests/policy/tests.cmake#policy.no_skip"
        status: pass
      - kind: integration
        ref: "tests/policy/tests.cmake#policy.no_skip.self_test"
        status: pass
    human_judgment: false
  - id: D5
    description: "docs/testing.md covers the presets, the inventory model, adding a test, negative controls, the gate-line and end-to-end budgets, reconfiguring after commits, CI-only lanes and the perf gate, under 61440 bytes and with no planning IDs"
    requirement: FRAME-01
    verification:
      - kind: other
        ref: "test -f docs/testing.md && test \"$(wc -c < docs/testing.md)\" -lt 61440 && cmake --workflow --preset ci exit 0; grep -c -i inventory docs/testing.md prints 10; grep -E -c 'D-[0-9]|FRAME-|XFER-|DEC\\.' docs/testing.md prints 0"
        status: pass
    human_judgment: false
  - id: D6
    description: "The three budgeted scripts total at or under 160 lines and carry no planning ID"
    requirement: FRAME-05
    verification:
      - kind: other
        ref: "test \"$(cat tests/cmake/no_skip.cmake tests/cmake/perf_compare.cmake tests/cmake/budgets.cmake | wc -l)\" -le 160 exit 0 (160); planning-ID git grep over the plan's paths, test $? -eq 1 exit 0"
        status: pass
    human_judgment: false

duration: 6min
completed: 2026-10-20
status: complete
---

# Phase 1 Plan 07: Perf gate logic, document budgets and the testing guide Summary

**`perf_compare.cmake` compares head and base Ir per frame in integer basis points. `SCHWAYSTATION_PERF_MODE` picks report or gate, and seven synthetic cases pin the +1.00 % boundary. `docs.budgets` and `policy.no_skip` (with its self test) now run in `ci`, and `docs/testing.md` explains the presets, inventories, controls and budgets to maintainers.**

## Performance

- **Duration:** 6 min (wall time, 2026-10-20T03:51:16Z to 03:57:28Z before the SUMMARY)
- **Tasks:** 3
- **Files modified:** 9

## Accomplishments

- The perf gate stays report-only in this phase. The CI job that runs Valgrind arrives in plan 01-12, and arming the gate is the phase-1 `ci:` tail step.
- The seven `perf.gate.*` cases run on every lane without Valgrind. `missing` also checks that an unset mode is an error and is never read as report.
- The three scripts this plan budgets total exactly 160 lines (perf_compare 102, budgets 26, no_skip 32). The gate-line total is 1144 of 1500.
- `docs.api_names`, `policy.notices` and `release.single_version_source` were not ported, as the plan says.

## Task Commits

1. **Task 1: Tracer - perf_compare.cmake through the seven synthetic gate cases** - `a0a7c79` (test)
2. **Task 2: docs.budgets and the no-skip policy** - `12a72c2` (test)
3. **Task 3: docs/testing.md** - `0ea639a` (docs)
4. **Task 2 follow-up: trim perf_compare.cmake to the ceiling** - `247a04b` (refactor). The Task 2 trim of this file was tested but left out of commit `12a72c2`; it was found by `git status` before the SUMMARY commit.

## Evidence (command, exit status; run from the repository root)

- `cmake --workflow --preset ci && ctest --test-dir build/ci -L perf --output-on-failure`: exit 0. Tracer gate: the re-run passed before Task 2.
- `grep -c '^perf\.gate\.' tests/perf/inventory.txt`: printed 7. `grep -c -E 'float|\.[0-9]+e' tests/cmake/perf_compare.cmake`: printed 0.
- `ctest --test-dir build/ci -R '^(docs\.budgets|policy\.no_skip(\.self_test)?)$' --output-on-failure`: exit 0.
- `cmake -DROOT=. -P tests/cmake/budgets.cmake`: exit 0, listing three files (README, header, docs/embedding.md).
- The same script with ROOT at a scratch directory holding only the header and README: exit 0. With a 61440-byte `docs/big.md` added: exit 1. With 61439 bytes: exit 0.
- `grep -c 'policy.no_skip' tests/policy/inventory.txt`: printed 2. `ctest --test-dir build/ci -R '^policy\.no_skip' --output-on-failure`: exit 0.
- `grep -c -E 'api_names|single_version|policy\.notices' tests/policy/tests.cmake`: printed 0.
- Task 3 verify (`test -f docs/testing.md && test "$(wc -c < docs/testing.md)" -lt 61440 && cmake --workflow --preset ci`): exit 0. The page is 10525 bytes.
- Cruft grep (the plan's pathspecs): a=1, b=1, `test "$a" -eq 1 && test "$b" -eq 1` exit 0.
- `grep -c -i 'inventory' docs/testing.md`: printed 10. `grep -E -c 'D-[0-9]|FRAME-|XFER-|DEC\.' docs/testing.md`: printed 0.
- `test "$(cat tests/cmake/no_skip.cmake tests/cmake/perf_compare.cmake tests/cmake/budgets.cmake | wc -l)" -le 160`: exit 0.
- `wc -l scripts/*.sh tests/cmake/*.cmake .githooks/* | tail -1`: 1144 total.
- Planning-ID scan over `tests/cmake/{perf_compare,budgets,no_skip}.cmake tests/perf tests/policy`: `test $? -eq 1` exit 0.
- Final, after `247a04b`: `cmake --workflow --preset ci`: exit 0. `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake`: exit 0. `cmake --workflow --preset hygiene`: exit 0.

## Files Created/Modified

- `tests/cmake/perf_compare.cmake`: the Ir per frame comparison.
- `tests/perf/tests.cmake`, `tests/perf/inventory.txt`, `tests/perf/allow.txt`: the seven cases and the empty exceptions file.
- `tests/cmake/budgets.cmake`, `tests/cmake/no_skip.cmake`: the document budgets and the self-skip scan.
- `tests/policy/tests.cmake`, `tests/policy/inventory.txt`: `docs.budgets`, `policy.no_skip` and `policy.no_skip.self_test`.
- `docs/testing.md`: the maintainer testing guide.

## Decisions Made

See `key-decisions` in the frontmatter.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Date parsing lost the day field during the header trim**
- **Found during:** Task 1
- **Issue:** The first trim passed `CMAKE_MATCH_3` straight to a `string(REGEX REPLACE)` that ran after another one. The first call overwrote the match variables, so every expiry date was parsed wrong and `perf.gate.allowed` accepted an expired line.
- **Fix:** `a0a7c79` restored the seed's copy-first form. The trim in `247a04b` then reads the fields with `math(EXPR "1${CMAKE_MATCH_n} - 100")`, which leaves the match variables alone and never reads a field as octal.
- **Files modified:** tests/cmake/perf_compare.cmake
- **Commits:** a0a7c79, 247a04b

**2. [Rule 1 - Bug] A relative ROOT made docs.budgets skip every docs page**
- **Found during:** Task 2 (running the plan's `-DROOT=.` acceptance command)
- **Issue:** With `ROOT=.`, the seed's `file(GLOB ... RELATIVE)` matched no `docs/*.md`, so the check passed while checking only two files.
- **Fix:** `file(REAL_PATH "${ROOT}" ROOT)` runs before the glob. A redundant `set(over "")` was dropped to stay within 160 lines.
- **Files modified:** tests/cmake/budgets.cmake
- **Commit:** 12a72c2

**3. [Plan reading] The perf mode comes from the environment, not -DMODE**
- The plan names the environment variable `SCHWAYSTATION_PERF_MODE`. The seed took `-DMODE`, so the script now reads only the variable. The test helper sets it for each run, and an unset value is tested as an error.
- **Commit:** a0a7c79

**4. [Process] The perf_compare.cmake trim landed in its own commit**
- The Task 2 trim (115 to 102 lines) was tested in the working tree but not staged with `12a72c2`. It was committed as `247a04b` before the SUMMARY commit, and `ci`, the JUnit check and `hygiene` were rerun after it: exit 0 each.

**Total deviations:** 4 (2 bugs, 1 plan reading, 1 process). **Impact:** none on scope. Both bugs were caught by the plan's own checks before commit.

## Flagged assumptions

- The plan gives the ceiling as both 165 (must-haves) and 160 (acceptance). The tighter 160 was met exactly.
- `docs.budgets` has no `.self_test` CTest because the plan's inventory lists none. The boundary (61440 fails, 61439 passes) was run once and recorded above.
- `docs/testing.md` describes `build.revision_matches_head` (plan 01-09), `policy.e2e_budget` (plan 01-12) and the package step behind `ctest --preset ci-release` before they land, as the plan asks. The `## CI jobs` heading holds only a comment until plan 01-12 fills it.

## Issues Encountered

None beyond the deviations above.

## Known Stubs

- `docs/testing.md` `## CI jobs`: an intentional placeholder that plan 01-12 fills, as the plan specifies.

## Requirement Status

`requirements-completed` lists this plan's share of FRAME-01, FRAME-04, FRAME-05 and FRAME-09. The REQUIREMENTS.md checkboxes stay open, as after earlier plans, because plans 01-09 and 01-11 through 01-15 still declare them.

## Next Phase Readiness

- Plan 01-12's perf job passes HEAD, BASE1, BASE2, ALLOW=tests/perf/allow.txt and TODAY, sets `SCHWAYSTATION_PERF_MODE: report`, and fills the `## CI jobs` section of `docs/testing.md`.

## Self-Check: PASSED

- Files: tests/cmake/perf_compare.cmake, tests/cmake/budgets.cmake, tests/cmake/no_skip.cmake, tests/perf/allow.txt and docs/testing.md exist.
- Commits: a0a7c79, 12a72c2, 0ea639a and 247a04b are ancestors of HEAD.
