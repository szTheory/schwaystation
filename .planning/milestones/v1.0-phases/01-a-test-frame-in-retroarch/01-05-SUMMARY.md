---
phase: 01-a-test-frame-in-retroarch
plan: 05
subsystem: testing, abi
tags: [cmake, ctest, nm, abi, header, c17, cxx17, nofp, api-version]

requires:
  - phase: 01-01
    provides: "schway_add_test, expect_fail.cmake, per-directory inventories, schway_core_options, the nofp option"
provides:
  - "tests/cmake/abi.cmake with modes symbols, globals, undefined and clock, run on $<TARGET_FILE:schwaystation>"
  - "abi.symbols, abi.globals, abi.undefined and core.no_host_clock, each with a .self_test on a fixture built with the core's options"
  - "abi.header.c17 and abi.header.cxx17 (size-tag layout, status size, API version 1, every signature)"
  - "tests/api/v1.c (abi.api_v1) and tests/cmake/api_version.cmake (abi.api_version and its self-test)"
  - "abi.nofp_fixture in tests/nofp, registered only under SCHWAYSTATION_NOFP"
affects: [01-07, 01-08, 01-10, 01-14]

actuals:
  tokens: 7505
  tasks: 3
  commits: 3
plan_head_before: 421f1ec347e6df47e060f20cbdaea06611bb2a79
plan_head_after: 56fefeea421f0e6f8bbd61ed8dde20f092d38df2

tech-stack:
  added: []
  patterns:
    - "One abi.cmake run per mode; each negative control is a static library built with get_target_property(schwaystation_instance COMPILE_OPTIONS) and checked through expect_fail.cmake with CODE 1 and a tolerant regex"
    - "Per-system allowlists are rows of 'system regex:names'; each row's capture is saved before if(MATCHES) overwrites CMAKE_MATCH_n"

key-files:
  created:
    - tests/cmake/abi.cmake
    - tests/cmake/api_version.cmake
    - tests/abi/symbol_fixture.c
    - tests/abi/global_fixture.c
    - tests/abi/libc_fixture.c
    - tests/abi/clock_fixture.c
    - tests/nofp/nofp_fixture.c
    - tests/header/header.c
    - tests/header/header.cpp
    - tests/api/v1.c
  modified:
    - tests/abi/tests.cmake
    - tests/abi/inventory.txt
    - tests/nofp/tests.cmake
    - tests/nofp/inventory.txt

key-decisions:
  - "The Darwin helper row allows memset_pattern16 beside bzero: Apple clang lowers the test card's repeating 16-byte fill to it (nm -u on the Release core). It is a compiler idiom like bzero, not a libc call in src/"
  - "The Windows helper row allows __ImageBase (MSVC x64 and ARM64 address switch tables from the linker-defined image base; schway_status_string is a switch). The plan's allowlist names it; the seed's did not"
  - "The clock mode matches an optional __imp_ prefix and leading underscore, and time32/time64, because MSVC's time() resolves to _time64"
  - "abi.api_version keeps the seed's reach: functions, types, constants and struct fields, and the generated version header's include name. v1.c includes schwaystation_version.h and checks the library version fields against it"
  - "No injected real-time clock: the PS1 has no RTC, so ENGINEERING section 2's injected-RTC form does not apply; core.no_host_clock is the whole host-clock proof"
  - "abi.header.c17, abi.header.cxx17 and abi.api_v1 are compile and run checks, not gates: each fails by a compile error or a non-zero exit against the real header, so they have no .self_test. The gates with negative controls are abi.symbols, abi.globals, abi.undefined, abi.api_version and core.no_host_clock"

requirements-completed: [FRAME-04, FRAME-08, FRAME-09]

coverage:
  - id: D1
    description: "abi.symbols passes on the Release core archive and its self-test fails with exit 1 naming the export outside the schway_ namespace"
    requirement: FRAME-04
    verification:
      - kind: integration
        ref: "tests/abi/tests.cmake#abi.symbols"
        status: pass
      - kind: integration
        ref: "tests/abi/tests.cmake#abi.symbols.self_test"
        status: pass
      - kind: other
        ref: "nm -g build/ci/libschwaystation.a, every defined external name starts with _schway (awk filter exit 0, no other name)"
        status: pass
    human_judgment: false
  - id: D2
    description: "abi.globals finds no writable data symbol in the core and its self-test fails with exit 1 naming the fixture's counter"
    requirement: FRAME-04
    verification:
      - kind: integration
        ref: "tests/abi/tests.cmake#abi.globals"
        status: pass
      - kind: integration
        ref: "tests/abi/tests.cmake#abi.globals.self_test"
        status: pass
    human_judgment: false
  - id: D3
    description: "abi.undefined allows only memcpy, memmove, memset, memcmp and the per-system helpers, and its self-test fails with exit 1 naming abort"
    requirement: FRAME-04
    verification:
      - kind: integration
        ref: "tests/abi/tests.cmake#abi.undefined"
        status: pass
      - kind: integration
        ref: "tests/abi/tests.cmake#abi.undefined.self_test"
        status: pass
    human_judgment: false
  - id: D4
    description: "core.no_host_clock finds no undefined host clock symbol in the core and its self-test fails with exit 1 on the fixture calling time(NULL)"
    requirement: FRAME-09
    verification:
      - kind: integration
        ref: "tests/abi/tests.cmake#core.no_host_clock"
        status: pass
      - kind: integration
        ref: "tests/abi/tests.cmake#core.no_host_clock.self_test"
        status: pass
    human_judgment: false
  - id: D5
    description: "The public header compiles alone as C17 and C++17 with -Wall -Wextra -Wpedantic -Werror, asserting each struct's leading uint32_t size at offset 0, a 4-byte status, API version 1 and every function and visitor signature"
    requirement: FRAME-04
    verification:
      - kind: integration
        ref: "tests/abi/tests.cmake#abi.header.c17"
        status: pass
      - kind: integration
        ref: "tests/abi/tests.cmake#abi.header.cxx17"
        status: pass
    human_judgment: false
  - id: D6
    description: "tests/api/v1.c calls every public function, assigns every field and checks every constant, and abi.api_version fails naming any header name v1.c lacks; its self-test fails on a header copy with one extra function"
    requirement: FRAME-08
    verification:
      - kind: unit
        ref: "tests/abi/tests.cmake#abi.api_v1"
        status: pass
      - kind: integration
        ref: "tests/abi/tests.cmake#abi.api_version"
        status: pass
      - kind: integration
        ref: "tests/abi/tests.cmake#abi.api_version.self_test"
        status: pass
    human_judgment: false
  - id: D7
    description: "abi.nofp_fixture lives in tests/nofp and is registered only by the nofp preset, where a double division must fail to build under -mgeneral-regs-only"
    requirement: FRAME-04
    verification:
      - kind: other
        ref: "grep -c abi.nofp_fixture tests/nofp/inventory.txt printed 1; grep -c nofp_fixture tests/abi/tests.cmake printed 0"
        status: pass
      - kind: integration
        ref: "tests/nofp/tests.cmake#abi.nofp_fixture"
        status: not_run
    human_judgment: false

duration: 8min
completed: 2026-10-20
status: complete
---

# Phase 1 Plan 05: Binary-shape and header gates Summary

**The Release core archive is now checked by nm for its exported names, writable data, imports and host clocks. Each of those four gates has a fixture that makes it fail with its named message. The header compiles alone as C17 and C++17 with its size-tag layout and signatures asserted. `tests/api/v1.c` touches all 86 header names, and `abi.api_version` fails on any name it lacks.**

## Performance

- **Duration:** 8 min (wall time, 2026-10-20T03:31:08Z to 03:39Z, SUMMARY included)
- **Tasks:** 3
- **Files modified:** 14

## Accomplishments

- `tests/cmake/abi.cmake` was ported from the seed with the schway_/schway__ renames. It has four modes. `symbols` checks both ways between the header and the archive and rejects any other external name. `globals` rejects nm types B b D d C. `undefined` allows the memory functions plus a per-system helper row. `clock` covers time, clock, clock_gettime, gettimeofday, timespec_get, mach_absolute_time, mach_continuous_time, QueryPerformanceCounter, GetTickCount64 and GetSystemTimeAsFileTime, with an optional import prefix and underscore.
- The four fixtures are `symbol`, `global`, `libc` and `clock`. Each is a static library built with `schwaystation_instance`'s compile options. `abi.symbols.self_test` and `core.no_host_clock.self_test` are new; the seed had neither.
- The header checks, `tests/api/v1.c` and `api_version.cmake` were rewritten for this header's 14 functions, 7 size-tagged structs, the allocator and the state visitor.
- `abi.nofp_fixture` moved to `tests/nofp/`. `tests/inventory/nofp.txt` (abi plus nofp) now holds the 13 abi tests plus the fixture on every lane that runs it.
- No injected RTC: the PS1 has no real-time clock, so ENGINEERING section 2's injected-RTC form does not apply (research Pattern 8). `core.no_host_clock` is the whole host-clock proof.

## Task Commits

1. **Task 1: Tracer - `abi.symbols` on the built archive with its new negative control** - `9088777` (test)
2. **Task 2: Globals, imports and host clocks with their negative controls** - `a84d440` (test)
3. **Task 3: Header compiles, v1 name coverage and the nofp fixture** - `56fefee` (test)

## Evidence (command, exit status; run from the repository root)

- `cmake --workflow --preset ci`: exit 0, 52 of 52 tests passed (final run after Task 3).
- `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake`: exit 0 (52 tests ran once each).
- `ctest --test-dir build/ci -R '^abi\.symbols' --output-on-failure`: exit 0. `ctest --test-dir build/ci -N -R '^abi\.symbols' | grep -c 'Test #'`: printed 2.
- Tracer gate: the Task 1 `<verify>` re-run passed before any Task 2 work.
- `nm -g build/ci/libschwaystation.a | awk '$2 ~ /^[A-TV-Z]$/ && $3 !~ /^_?schway/ {bad++} END {exit bad>0}'`: exit 0. The 23 defined external names are 9 `_schway__*` internals and the 14 header functions.
- `ctest --test-dir build/ci -R '^(abi\.(globals|undefined)|core\.no_host_clock)' --output-on-failure`: exit 0. The same filter with `-N | grep -c 'Test #'`: printed 6. `grep -c clock_gettime tests/cmake/abi.cmake`: printed 1.
- `ctest --test-dir build/ci -N -L abi | grep -cE 'Test +#'`: printed 13. The plan's literal `grep -c 'Test #'` undercounts single-digit test numbers, which ctest pads as `Test  #1`. That is why the tolerant form is used.
- `ctest --test-dir build/ci -L abi`: exit 0, 13 of 13. `abi.api_version` reported "86 header names covered at API version 1".
- `cmake --build build/ci --target schwaystation_header_c17 schwaystation_header_cxx17 -v` after touching both sources: exit 0. The compile lines carry `-std=c17` and `-std=c++17` with `-Wall -Wextra -Wpedantic -Werror`.
- `grep -c 'abi.nofp_fixture' tests/nofp/inventory.txt`: printed 1. `grep -c 'nofp_fixture' tests/abi/tests.cmake`: printed 0.
- Gate-line ceiling: `test "$(cat tests/cmake/abi.cmake tests/cmake/api_version.cmake | wc -l)" -le 150`: exit 0 (150 lines: 140 commands, 10 comment lines, 0 blank). `wc -l scripts/*.sh tests/cmake/*.cmake .githooks/* | tail -1`: 1049 total.
- Cruft grep (plan 01-01's pathspecs): a=1, b=1, `test "$a" -eq 1 && test "$b" -eq 1` exit 0.
- Planning-ID scan: `git grep --untracked -nE -e 'D-[0-9]{2}' -e 'FRAME-[0-9]' -e 'XFER-[0-9]' -e 'DEC\.[0-9]' -- tests/cmake/abi.cmake tests/cmake/api_version.cmake tests/abi tests/nofp tests/header tests/api; test $? -eq 1`: exit 0.
- `sh scripts/hygiene.sh --tree`: exit 0.
- The `nofp` preset cannot run on this host because there is no GCC. Plan 01-14's hosted job runs it first. `be32`, `wasm32`, `asan` and `ci-msvc` were not run either. The MSVC rows (`__ImageBase`, `__isa_available`, the COMDAT names, `_time64`) are first exercised there.

## Files Created/Modified

- `tests/cmake/abi.cmake`: nm modes symbols, globals, undefined and clock.
- `tests/cmake/api_version.cmake`: the header-name coverage check.
- `tests/abi/{symbol,global,libc,clock}_fixture.c`: negative controls.
- `tests/abi/tests.cmake`, `tests/abi/inventory.txt`: 13 abi tests registered and listed.
- `tests/header/header.c`, `tests/header/header.cpp`: the C17 and C++17 header compiles.
- `tests/api/v1.c`: the frozen API v1 program.
- `tests/nofp/nofp_fixture.c`, `tests/nofp/tests.cmake`, `tests/nofp/inventory.txt`: `abi.nofp_fixture`.

## Decisions Made

See `key-decisions` in the frontmatter. In short: Darwin also allows `memset_pattern16` and Windows `__ImageBase`, the clock regex covers MSVC's `_time64`, `abi.api_version` keeps the seed's field coverage, and the PS1 needs no injected RTC.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Darwin `abi.undefined` failed on `memset_pattern16`**
- **Found during:** Task 2
- **Issue:** Apple clang lowers the test card's repeating fill in `src/testcard.c` to libSystem's `memset_pattern16`, so `abi.undefined` failed on the Release archive. The seed's core never had this pattern.
- **Fix:** Added the name to the Darwin helper row, next to `bzero`. This is a compiler idiom, not a call in `src/`, so no defect goes to plan 01-10.
- **Files modified:** tests/cmake/abi.cmake
- **Commit:** a84d440

**2. [Rule 1 - Bug] The generated-name regex captured an empty alternative**
- **Found during:** Task 3, while trimming comments
- **Issue:** The Task 1 port read `CMAKE_MATCH_2` after `if(SYSTEM MATCHES ...)`, which overwrites it. On Windows this appended an empty `|` alternative, so `globals` and `symbols` would have skipped every name. The bug was reproduced: the HEAD-at-Task-2 script with `-DSYSTEM=Windows/AMD64` passed the global fixture (exit 0).
- **Fix:** The row's capture is now saved before the system match, as the seed did. The same command now reports `counter`.
- **Files modified:** tests/cmake/abi.cmake
- **Commit:** 56fefee

**3. [Plan ceiling] Comments trimmed to fit the 150-line gate ceiling**
- After the port the two scripts had 179 lines. Only comment and blank lines were cut, down to one usage line, a fault-prevented line, and a one-line source note per allowlist table. No commands were removed.

## Issues Encountered

None beyond the deviations above.

## Defects for plan 01-10

None. No gate found a defect in `src/`.

## Known Stubs

None.

## Requirement Status

`requirements-completed` lists this plan's share of FRAME-04, FRAME-08 and FRAME-09. The REQUIREMENTS.md checkboxes stay open, as after earlier plans. Plans 01-06, 01-07, 01-08, 01-09, 01-12, 01-13 and 01-14 still declare these requirements: `hygiene.no_float`, `docs.budgets`, the install and embed consumers, `--bench`, Cachegrind, and the hosted nofp and MSVC runs.

## Flagged-assumption classification

`abi.header.c17`, `abi.header.cxx17` and `abi.api_v1` are compile and run checks. The gates with negative controls that fail with their named message and exit 1 are `abi.symbols`, `abi.globals`, `abi.undefined`, `abi.api_version` and `core.no_host_clock`.

## Next Phase Readiness

- Plan 01-14's hosted run is the first to exercise `abi.nofp_fixture` (nofp preset) and the MSVC allowlist rows (ci-msvc).
- Plan 01-08's C++ consumer can reuse `tests/header/header.cpp`'s pattern. A later API change needs `tests/api/v2.c` and `SCHWAY_API_VERSION 2`.

## Self-Check: PASSED

- Files: every file listed under key-files is present.
- Commits: 9088777, a84d440 and 56fefee are ancestors of HEAD.
