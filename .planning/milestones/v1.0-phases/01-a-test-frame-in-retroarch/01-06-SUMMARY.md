---
phase: 01-a-test-frame-in-retroarch
plan: 06
subsystem: testing, hygiene
tags: [cmake, ctest, sh, hygiene, gate-budget, no-float, publish, title]

requires:
  - phase: 01-01
    provides: "schway_add_test, expect_fail.cmake, check_junit.cmake, the hygiene preset and the per-directory inventories"
  - phase: 01-02
    provides: "scripts/publish.sh re-synced with the schwaystation archive names, scripts/hygiene.sh, check-title.sh, classify-changes.sh"
provides:
  - "tests/cmake/line_budget.cmake (ROOT, LIMIT, PATTERNS) with zero-match and over-budget failures; plan 01-12 reuses it for tests/retroarch"
  - "tests/cmake/no_float.cmake, the text scan for floating point in src/ and include/"
  - "15 hygiene-preset tests: hygiene.tree, hygiene.no_bios_bytes, hygiene.no_float, ci.title, ci.classify, policy.gate_budget, policy.publish_set, policy.publish_ref, policy.publish_signer and the self tests"
  - "tests/hygiene/fixtures/titles.txt"
  - "scripts/{hygiene,apply-repo-settings,fetch-pin,classify-changes,check-title}.sh trimmed to usage plus Fault prevented headers"
affects: [01-12, 01-14, 01-15]

actuals:
  tokens: 10757
  tasks: 3
  commits: 3
plan_head_before: db15cf7f269ce318b231fa74fad11e49d7500abd
plan_head_after: 4162efc08d85078bfcd96d10645717520cd269c8

tech-stack:
  added: []
  patterns:
    - "Throwaway-repository self tests: a configure-time git init directory holding a copy of scripts/hygiene.sh and planted files built from pieces, run through expect_fail.cmake with CODE 1"
    - "The executable-signature fixture is decoded at configure time from the hex on scripts/hygiene.sh's magic= line, so no tracked file holds the bytes"
    - "A multi-command sh -c body passed through expect_fail's CMD list separates commands with newlines, never ';', which would split the list"

key-files:
  created:
    - tests/cmake/line_budget.cmake
    - tests/cmake/no_float.cmake
    - tests/hygiene/fixtures/titles.txt
  modified:
    - tests/hygiene/tests.cmake
    - tests/hygiene/inventory.txt
    - scripts/hygiene.sh
    - scripts/apply-repo-settings.sh
    - scripts/fetch-pin.sh
    - scripts/classify-changes.sh
    - scripts/check-title.sh

key-decisions:
  - "line_budget.cmake counts newline bytes only, as wc -l does, so its total equals the wc -l the ceilings are written against"
  - "policy.publish_set.self_test runs prepare on a 17-archive set and on a 19-archive set (18 plus runner-linux-x86) and needs both 'missing' and 'unexpected' messages, covering both sides of exactly 18"
  - "hygiene.tree.self_test plants an address as well as the home path the seed planted, both assembled in pieces"
  - "The title fixture's perf(maria) line became perf(gpu): maria is another console's chip name"
  - "ci.title, ci.title.self_test and ci.classify carry label hygiene only; every policy.* test carries hygiene and policy"

requirements-completed: [FRAME-04, FRAME-05, FRAME-06]

coverage:
  - id: D1
    description: "policy.gate_budget counts scripts/*.sh, tests/cmake/*.cmake and .githooks/* at a 1500-line limit; its self test fails with exit 1 and the over-budget message on a 10-line fixture at LIMIT 9; LIMIT 10 passes and a pattern matching no file fails naming it"
    requirement: FRAME-05
    verification:
      - kind: integration
        ref: "tests/hygiene/tests.cmake#policy.gate_budget"
        status: pass
      - kind: integration
        ref: "tests/hygiene/tests.cmake#policy.gate_budget.self_test"
        status: pass
      - kind: other
        ref: "cmake -DROOT=build/hygiene/hygiene/line_budget_fixture -DLIMIT=10 -DPATTERNS='*' -P tests/cmake/line_budget.cmake exit 0; cmake -DROOT=. -DLIMIT=1500 '-DPATTERNS=nothing/*.sh' -P tests/cmake/line_budget.cmake exit 1 naming nothing/*.sh"
        status: pass
    human_judgment: false
  - id: D2
    description: "hygiene.tree runs the tree scan on the real tree; its self test fails with exit 1 naming the home directory path and the address planted in a throwaway repository"
    requirement: FRAME-05
    verification:
      - kind: integration
        ref: "tests/hygiene/tests.cmake#hygiene.tree"
        status: pass
      - kind: integration
        ref: "tests/hygiene/tests.cmake#hygiene.tree.self_test"
        status: pass
    human_judgment: false
  - id: D3
    description: "hygiene.no_bios_bytes runs the tree scan; its self test plants a 524288-byte NUL file and an executable-signature file generated at configure time and expects 'BIOS image by size' and 'PlayStation executable by content' in path order"
    requirement: FRAME-05
    verification:
      - kind: integration
        ref: "tests/hygiene/tests.cmake#hygiene.no_bios_bytes"
        status: pass
      - kind: integration
        ref: "tests/hygiene/tests.cmake#hygiene.no_bios_bytes.self_test"
        status: pass
    human_judgment: false
  - id: D4
    description: "hygiene.no_float scans src/ and include/ for float, double, the float headers and float literals; its self test fails with exit 1 on a fixture with six float lines"
    requirement: FRAME-04
    verification:
      - kind: integration
        ref: "tests/hygiene/tests.cmake#hygiene.no_float"
        status: pass
      - kind: integration
        ref: "tests/hygiene/tests.cmake#hygiene.no_float.self_test"
        status: pass
    human_judgment: false
  - id: D5
    description: "ci.title accepts the Conventional Commit forms and rejects Phase titles, pipeline-scoped release types and skip tokens; ci.title.self_test fails with the named message on a Phase title"
    requirement: FRAME-05
    verification:
      - kind: integration
        ref: "tests/hygiene/tests.cmake#ci.title"
        status: pass
      - kind: integration
        ref: "tests/hygiene/tests.cmake#ci.title.self_test"
        status: pass
    human_judgment: false
  - id: D6
    description: "ci.classify, policy.publish_set (18 archives in C-locale order, 17 and 19 fail through its self test), policy.publish_ref and policy.publish_signer run the scripts the rehearsal and the changes job use"
    requirement: FRAME-06
    verification:
      - kind: integration
        ref: "tests/hygiene/tests.cmake#ci.classify"
        status: pass
      - kind: integration
        ref: "tests/hygiene/tests.cmake#policy.publish_set"
        status: pass
      - kind: integration
        ref: "tests/hygiene/tests.cmake#policy.publish_set.self_test"
        status: pass
      - kind: integration
        ref: "tests/hygiene/tests.cmake#policy.publish_ref"
        status: pass
      - kind: integration
        ref: "tests/hygiene/tests.cmake#policy.publish_signer"
        status: pass
    human_judgment: false
  - id: D7
    description: "The five scripts are trimmed to comment and blank changes only; scripts/*.sh plus .githooks/* is 592 lines (ceiling 620) and line_budget.cmake plus no_float.cmake 73 (ceiling 75)"
    requirement: FRAME-05
    verification:
      - kind: other
        ref: "for f in scripts/*.sh; do sh -n and dash -n; done exit 0; git diff b7b41fb -- scripts/ shows no changed line that is not a comment or blank (grep exit 1)"
        status: pass
      - kind: other
        ref: "test \"$(cat scripts/*.sh .githooks/* | wc -l)\" -le 620 exit 0; test \"$(cat tests/cmake/line_budget.cmake tests/cmake/no_float.cmake | wc -l)\" -le 75 exit 0"
        status: pass
    human_judgment: false

duration: 8min
completed: 2026-10-20
status: complete
---

# Phase 1 Plan 06: Repository gates in the hygiene preset Summary

**The hygiene preset now runs 15 tests. `policy.gate_budget` holds gate code to 1,500 lines (984 now) and fails on a glob that matches nothing. The tree, BIOS-bytes and float scans each have a planted fixture that must fail with its named finding, and the title, classify and publish scripts are under test. The five older scripts lost 110 comment and blank lines without changing a command.**

## Performance

- **Duration:** 8 min (wall time, 2026-10-20T03:41:38Z to 03:49:14Z before the SUMMARY)
- **Tasks:** 3
- **Files modified:** 10

## Accomplishments

- `tests/cmake/line_budget.cmake` (25 lines) replaces the seed's fixed-directory end-to-end budget. It takes `ROOT`, `LIMIT` and `PATTERNS`. A glob that matches no file fails, naming the glob. A total over the limit fails, naming the total and the limit.
- `hygiene.tree.self_test` and `hygiene.no_bios_bytes.self_test` each build a throwaway git repository at configure time. The planted home path and address are assembled from pieces. The 512 KiB NUL file is written with `dd`. The executable signature is decoded from the hex in `scripts/hygiene.sh`. No tracked file holds any of them.
- `policy.publish_set.self_test` covers both sides of exactly 18 archives: 17 fails naming the missing zip, and 19 fails naming the extra one.
- `tests/cmake/no_float.cmake` was ported with a trimmed header (48 lines).
- `hygiene.format`, `hygiene.analyzer`, `policy.pins` and `policy.ci_required` were not ported, as the plan says.

## Task Commits

1. **Task 1: Tracer - `policy.gate_budget` with boundary and zero-match controls** - `22f8308` (test)
2. **Task 2: Float scan, BIOS-bytes gate, title and classify checks, publish tests** - `b7b41fb` (test)
3. **Task 3: Trim the existing scripts to the budget** - `4162efc` (refactor)

## Evidence (command, exit status; run from the repository root)

- `cmake --workflow --preset hygiene`: exit 0, 15 of 15 (final run after Task 3).
- `cmake -DJUNIT=build/hygiene/junit.xml -DINVENTORY=tests/inventory/hygiene.txt -P tests/cmake/check_junit.cmake`: exit 0 (15 tests ran once each).
- `cmake --workflow --preset ci`: exit 0, 52 of 52.
- `ctest --test-dir build/hygiene -R '^policy\.gate_budget' --output-on-failure`: exit 0. Tracer gate: re-run passed before Task 2.
- `cmake -DROOT=build/hygiene/hygiene/line_budget_fixture -DLIMIT=10 -DPATTERNS='*' -P tests/cmake/line_budget.cmake`: exit 0 (exactly at the limit).
- `cmake -DROOT=. -DLIMIT=1500 '-DPATTERNS=nothing/*.sh' -P tests/cmake/line_budget.cmake`: exit 1, naming `nothing/*.sh`.
- `wc -l scripts/*.sh tests/cmake/*.cmake .githooks/* | tail -1`: 1045 before Task 2 (Task 1 recorded), 984 after Task 3.
- `scripts/*.sh` plus `.githooks/*`: 702 lines before the trim, 592 after. `test "$(cat scripts/*.sh .githooks/* | wc -l)" -le 620`: exit 0. `test "$(cat tests/cmake/line_budget.cmake tests/cmake/no_float.cmake | wc -l)" -le 75`: exit 0 (73).
- `grep -v '^#' tests/hygiene/inventory.txt | cut -f1 | tr '\n' ' '` lists the 15 names the plan gives.
- `grep -c -E 'hygiene\.(format|analyzer)|policy\.(pins|ci_required)' tests/hygiene/tests.cmake`: printed 0.
- `sh scripts/hygiene.sh --tree`: exit 0.
- `for f in scripts/*.sh; do sh -n "$f" && dash -n "$f" || exit 1; done`: exit 0.
- BASE for the trim was `b7b41fb`. `git diff b7b41fb -- scripts/ | grep -E '^[+-][^+-]' | grep -v -E '^[+-][[:space:]]*(#|$)'`: exit 1, so no changed line is code. Comparing each script with comment-only and blank lines dropped against its BASE version also gave equal files.
- Cruft grep (plan's pathspecs): a=1, b=1, `test "$a" -eq 1 && test "$b" -eq 1` exit 0.
- Planning-ID scan over `tests/cmake/line_budget.cmake tests/cmake/no_float.cmake tests/hygiene scripts .githooks`: `test $? -eq 1` exit 0.

## Files Created/Modified

- `tests/cmake/line_budget.cmake`: the line budget check.
- `tests/cmake/no_float.cmake`: the float text scan.
- `tests/hygiene/tests.cmake`, `tests/hygiene/inventory.txt`: 15 tests registered and listed.
- `tests/hygiene/fixtures/titles.txt`: the title fixtures.
- `scripts/{hygiene,apply-repo-settings,fetch-pin,classify-changes,check-title}.sh`: comments trimmed.

## Decisions Made

See `key-decisions` in the frontmatter.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] The publish self test's `;` split expect_fail's command list**
- **Found during:** Task 2
- **Issue:** The two `prepare` runs in one `sh -c` body were joined by `;`. That split the CMD list, so sh got a broken script and exited 126.
- **Fix:** The two commands are now separated by a newline.
- **Files modified:** tests/hygiene/tests.cmake
- **Commit:** b7b41fb

**2. [Rule 2 - Coverage] publish_set.self_test covers 19 archives as well as 17**
- The must-have says that 17 or 19 archives fail. The seed tested only 17, so the self test now runs a 19-archive set as well and needs both messages.
- **Commit:** b7b41fb

**3. [Rename] Title fixture `perf(maria)` became `perf(gpu)`**
- maria is another system's chip name, and CLAUDE.md Style forbids other emulators' identifiers. The test still passes or fails each line the same way.
- **Commit:** b7b41fb

**Total deviations:** 3 (1 bug, 1 coverage, 1 rename). **Impact:** none on scope; every gate is as strict as the seed's or stricter.

## Flagged assumptions

`policy.publish_ref`, `policy.publish_signer` and `ci.classify` are behavioural tests of `scripts/publish.sh` and `scripts/classify-changes.sh`. They run the scripts on fixed inputs and check the results, as the seed registers them. They are not gates, so none has a separate `.self_test` negative control. The gates with controls are the `*.self_test` pairs: `hygiene.tree`, `hygiene.no_bios_bytes`, `hygiene.no_float`, `ci.title`, `policy.gate_budget` and `policy.publish_set`.

The "reads all input before deciding" truth is a backstop and has no test of its own. `check-title.sh` and `classify-changes.sh` read all of their input first. `hygiene.sh` ends some pipelines in `grep -q`. The pipeline's status is grep's, so an early exit cannot change a finding. Commands were out of scope for this plan's trim. Findings come in `git ls-files` path order, which `hygiene.no_bios_bytes.self_test`'s ordered regex checks.

## Issues Encountered

None beyond the deviations above.

## Known Stubs

None.

## Requirement Status

`requirements-completed` lists this plan's share of FRAME-04, FRAME-05 and FRAME-06. The REQUIREMENTS.md checkboxes stay open, as after earlier plans, because plans 01-07, 01-09, 01-11 through 01-15 still declare them.

## Next Phase Readiness

- Plan 01-12 registers `policy.e2e_budget` on `line_budget.cmake` with `PATTERNS=tests/retroarch/*` and LIMIT 300.
- Plan 01-14's hosted hygiene job runs this preset on Linux. It is the first run of these tests under dash as `/bin/sh`; locally they ran under macOS sh, with `dash -n` syntax checks only.

## Self-Check: PASSED

- Files: tests/cmake/line_budget.cmake, tests/cmake/no_float.cmake and tests/hygiene/fixtures/titles.txt exist.
- Commits: 22f8308, b7b41fb and 4162efc are ancestors of HEAD.
