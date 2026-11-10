---
phase: 02-the-cpu-matches-the-public-vectors
plan: 01
subsystem: testing
tags: [cmake, ctest, inventory, test-skeleton]

requires:
  - phase: 01-foundation
    provides: per-directory inventory machinery, ci and wasm32 selections, JUnit exact-inventory check
provides:
  - empty inventory.txt and tests.cmake for tests/cpu, bus, vectors, fuzz, exe and fixtures
  - the SCHWAYSTATION_FIXTURES option gating the fixtures directory
  - ci and wasm32 selections that include all six directories
affects: [02-02, 02-03, 02-04, 02-05, 02-06, 02-07]

actuals:
  tokens: 1500
  tasks: 2
  commits: 2

plan_head_before: 8b29d8aa3c0e21d3a9db84df5c2655b86ab1c687
plan_head_after: 8a83d12b1aded5dbe01cb8a325ff230c3e9605cd
commits: 2

tech-stack:
  added: []
  patterns:
    - "one inventory.txt and tests.cmake pair per test directory, selected by name in tests/inventory/*.txt"

key-files:
  created:
    - tests/cpu/inventory.txt
    - tests/cpu/tests.cmake
    - tests/bus/inventory.txt
    - tests/bus/tests.cmake
    - tests/vectors/inventory.txt
    - tests/vectors/tests.cmake
    - tests/fuzz/inventory.txt
    - tests/fuzz/tests.cmake
    - tests/exe/inventory.txt
    - tests/exe/tests.cmake
    - tests/fixtures/inventory.txt
    - tests/fixtures/tests.cmake
  modified:
    - tests/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "Selection files stay in byte order, so each new directory name sits at its sorted position rather than at the end"
  - "SCHWAYSTATION_FIXTURES is declared in tests/tests.cmake immediately before the fixtures include, after the hygiene and nofp early returns"

patterns-established:
  - "Later plans append only to their own directory's inventory.txt and tests.cmake; the ci and wasm32 selections are already final for this phase"

requirements-completed: [CPU-01, CPU-02, CPU-03, CPU-04, CPU-05]

coverage:
  - id: D1
    description: "Six test directories (cpu, bus, vectors, fuzz, exe, fixtures) each have an empty inventory.txt and a tests.cmake included from tests/tests.cmake"
    requirement: "CPU-01"
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci (exit 0)"
        status: pass
      - kind: integration
        ref: "cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake (exit 0, 79 tests ran once each)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The ci and wasm32 selections name all six new directories"
    requirement: "CPU-03"
    verification:
      - kind: other
        ref: "for d in cpu bus vectors fuzz exe fixtures; do test -f tests/$d/inventory.txt && test -f tests/$d/tests.cmake && grep -qx $d tests/inventory/ci.txt && grep -qx $d tests/inventory/wasm32.txt || exit 1; done (exit 0)"
        status: pass
    human_judgment: false
  - id: D3
    description: "SCHWAYSTATION_FIXTURES option (default ON) gates inclusion of tests/fixtures/tests.cmake"
    requirement: "CPU-04"
    verification:
      - kind: other
        ref: "grep -q 'option(SCHWAYSTATION_FIXTURES' tests/tests.cmake (exit 0)"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci (exit 0)"
        status: pass
    human_judgment: false

duration: 2min
completed: 2026-11-09
status: complete
---

# Phase 2 Plan 1: Test skeleton Summary

**Six empty per-directory test inventories (cpu, bus, vectors, fuzz, exe, fixtures) selected on ci and wasm32, with a SCHWAYSTATION_FIXTURES option gating the fixtures directory.**

## Performance

- **Duration:** 2 min
- **Started:** 2026-11-09T18:46:10Z
- **Completed:** 2026-11-09T18:47:00Z
- **Tasks:** 2
- **Files modified:** 15 (12 created, 3 modified)

## Accomplishments
- `tests/vectors` landed first as a tracer: selected, included and passing the exact-inventory check on its own.
- `tests/cpu`, `bus`, `fuzz`, `exe` and `fixtures` followed the same shape, so every later plan touches only its own directory pair.
- `SCHWAYSTATION_FIXTURES` (default ON) lets a lane without armips drop the fixtures directory.

## Task Commits

1. **Task 1: Tracer - vectors directory selected, included and checked** - `21b8ebe` (test)
2. **Task 2: The other five directories and the fixtures option** - `8a83d12` (test)

**Plan metadata:** recorded in the docs commit that follows this summary.

## Files Created/Modified
- `tests/{cpu,bus,vectors,fuzz,exe,fixtures}/inventory.txt` - header comment only, no entries yet
- `tests/{cpu,bus,vectors,fuzz,exe,fixtures}/tests.cmake` - one-line comment naming what each will register
- `tests/tests.cmake` - includes for the six directories and the `SCHWAYSTATION_FIXTURES` option
- `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt` - the six names, in byte order

## Decisions Made
- Kept both selection files sorted in byte order (the new names are interleaved, not appended).
- Placed the `vectors` include in the same sorted run as the other directories rather than literally after `core`; include order has no effect on the inventory check.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None. Tracer gate re-run: the Task 1 verify command passed end to end before Task 2 began.

## Known Stubs

None. The empty inventories are intentional placeholders for later plans in this phase, not stubs that reach a user.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Every later plan in the phase can register tests by appending to its own `inventory.txt` and `tests.cmake`.
- `tests/fixtures/tests.cmake` is still comment-only; the armips pin and fixture assembly arrive with their own plan.

## Self-Check: PASSED

- Created files present on disk (all twelve `tests/{cpu,bus,vectors,fuzz,exe,fixtures}/{inventory.txt,tests.cmake}`).
- Commits `21b8ebe` and `8a83d12` are ancestors of HEAD.
- Acceptance commands for both tasks exit 0; `cmake --workflow --preset ci` exit 0; JUnit check exit 0; `sh scripts/hygiene.sh` exit 0.

---
*Phase: 02-the-cpu-matches-the-public-vectors*
*Completed: 2026-11-09*
