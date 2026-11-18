---
phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
plan: 22
subsystem: testing
tags:
  - citations
  - comments
  - tests
  - fixtures

requires:
  - phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
    provides: the docs/hardware pages and headings the comments now name (plans 03-13 to 03-19, 03-21)
provides:
  - no test source and no fixture source cites PSX-SYSTEM sections 2 to 6 or PSX-GPU-AND-GTE section 5 in any form
  - each re-pointed comment names a docs/hardware page and a heading that exists on it
affects:
  - plan 03-23, which shrinks the preparation sections to pointers with nothing in tests/ pointing back

actuals:
  tokens: 6500
  tasks: 2
  commits: 2

plan_head_before: 0554ef52abbe9a48e45d205da8d2688697fc2147
plan_head_after: 19da8d444575dceb6f0b40b933726a15a9959b01
commits: 2

tech-stack:
  added: []
  patterns:
    - "a test comment cites docs/hardware/<page>.md and a heading that exists on that page; the file-header references to a preparation file name the pages the file's tests rely on"

key-files:
  created: []
  modified:
    - tests/bus/bus_tests.c
    - tests/bus/cache_tests.c
    - tests/core/bus_unbuilt.c
    - tests/core/test_state.c
    - tests/cpu/cop0_tests.c
    - tests/cpu/cop2_tests.c
    - tests/cpu/delay_tests.c
    - tests/cpu/exception_tests.c
    - tests/cpu/timing_tests.c
    - tests/fixtures/src/stub/stub.asm

key-decisions:
  - "The old subsection names were mapped to the page headings that now hold the fact (Load delay to Delay latches, MULTU/MULT and DIV/DIVU to Multiply and divide, Vectors and EPC to Entry, ExcCodes to CAUSE, Cache isolation and write queue to Instruction cache and Write queue, D-cache to The scratchpad), and every heading named was checked against the page"
  - "stub.asm keeps PSX-SYSTEM sections 7 and 9, which stay citable, and moves section 3 to the bus page's Memory control heading"

requirements-completed: []

coverage:
  - id: D1
    description: "No test or fixture source cites PSX-SYSTEM sections 2 to 6 or PSX-GPU-AND-GTE section 5 in any form (numbered, wrapped, or the unnumbered preparation-file headers of cop0_tests.c, delay_tests.c and exception_tests.c); each names a docs/hardware page and heading"
    requirement: GTE-01
    verification:
      - kind: other
        ref: "test -z \"$(grep -rnoE '(PSX-SYSTEM|PSX-GPU-AND-GTE)( sections? [0-9]+)?' tests | grep -vE '(PSX-SYSTEM sections? (1|7|8|9|10)|PSX-GPU-AND-GTE sections? [1-46-9])$')\", exit 0 (62 mentions at HEAD, 0 after)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The change is comment-only: every test passes, the stub's assembled bytes equal the committed array and the JUnit inventory check passes"
    requirement: GTE-02
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci, exit 0"
        status: pass
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^fixtures\\.(stub_drift|stub\\.window)$' --no-tests=error, exit 0"
        status: pass
      - kind: integration
        ref: "cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake, exit 0 (335 tests ran once each)"
        status: pass
      - kind: other
        ref: "cmake --workflow --preset hygiene, exit 0"
        status: pass
    human_judgment: false

duration: 4min
completed: 2026-11-10
status: complete
---

# Phase 3 Plan 22: Re-point the test and fixture citations Summary

**62 citations of PSX-SYSTEM sections 2, 3 and 6 in nine test sources and the stub kernel source re-pointed to docs/hardware pages and headings, comment-only, with the stub's assembled bytes unchanged.**

## Performance

- **Duration:** 4 min
- **Started:** 2026-11-10T08:11:00Z
- **Completed:** 2026-11-10T08:15:00Z
- **Tasks:** 2
- **Files modified:** 10

## Accomplishments

- The citation gate listed 62 mentions under `tests/` at HEAD (the count the plan gave, so plans 03-08 to 03-19 added none in the files this plan lists; the 03-21 note about newer citations found nothing extra here) and lists none now. By file: cop0_tests.c 15, exception_tests.c 20, timing_tests.c 11, delay_tests.c 7, bus_tests.c 3, cache_tests.c 1, cop2_tests.c 1, bus_unbuilt.c 1, test_state.c 1, stub.asm 2, counted per matching line.
- The three unnumbered file headers (cop0_tests.c, delay_tests.c, exception_tests.c) that named `.planning/preparation/PSX-SYSTEM.md` now name the pages the file's tests rely on.
- Comment-only, checked by script: with block comments (and `;` comments for the stub) stripped, each of the ten changed files is identical to HEAD. `cmake --workflow --preset ci` exits 0, `fixtures.stub_drift` and `fixtures.stub.window` pass, the JUnit inventory check reports 335 tests ran once each, and `cmake --workflow --preset hygiene` exits 0.
- Every `docs/hardware/<page>.md, <heading>` named in the new comments exists as a heading on that page.
- gte_tests.c, dma_tests.c, timer_tests.c, sched_tests.c and the other listed files carried no such citation and are unchanged.

## Task Commits

1. **Task 1: tracer, the stub kernel source re-pointed** - `1a4e91d` (docs)
2. **Task 2: the test sources re-pointed** - `19da8d4` (docs)

**Plan metadata:** the commit that follows this summary (docs: complete plan).

## Files Created/Modified

- `tests/fixtures/src/stub/stub.asm` - Memory control citations move to the bus page; sections 7 and 9 kept
- `tests/bus/bus_tests.c`, `tests/bus/cache_tests.c`, `tests/core/bus_unbuilt.c`, `tests/core/test_state.c`, `tests/cpu/cop0_tests.c`, `tests/cpu/cop2_tests.c`, `tests/cpu/delay_tests.c`, `tests/cpu/exception_tests.c`, `tests/cpu/timing_tests.c` - comment citations only

## Decisions Made

- Old subsection names were mapped to the heading that now holds the fact; where the page splits a fact across headings (for example "SR bits and Address and bus errors" for a coprocessor-unusable case) the comment names the heading that states it (SR and Coprocessor unusable).
- Facts that came from the preparation section's own wording and have no heading on a page of their own (Priority, "GTE in a delay slot of an interrupt") cite the page's Interrupts heading, which holds them in its list of entry rules.

## Deviations from Plan

None - plan executed exactly as written. The `requirements:` frontmatter of the plan lists GTE-01 to GTE-03 while its output spec asks for an empty `requirements-completed:`; the output spec was followed because the plan changes no behaviour, so no requirement is marked complete here and REQUIREMENTS.md is untouched.

## Issues Encountered

None.

## Not run locally

`asan` and `be32` need clang-21, which this host lacks; hosted CI runs them. The change is comments only (comment-stripped sources are identical to HEAD), so neither can see it.

## Known Stubs

None.

## Threat Flags

None. No code, endpoint or schema changed.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Plan 03-23 can shrink PSX-SYSTEM sections 2 to 6 and PSX-GPU-AND-GTE section 5 to pointers: nothing in `src/`, `docs/hardware/` or `tests/` cites them.

## Self-Check: PASSED

Created and modified files exist; commits `1a4e91d` and `19da8d4` are ancestors of HEAD; the citation gate exits 0.
