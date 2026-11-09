---
phase: 02-the-cpu-matches-the-public-vectors
plan: 08
subsystem: api
tags: [api-v2, ps-exe, loader, probe, validator, profile-flags, abi]

requires:
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: instance profile_flags and exe_loaded state fields, bus RAM and debug ports, state format 2 (plans 02-01, 02-02, 02-07)
provides:
  - SCHWAY_API_VERSION 2 (SOVERSION and ABI version stay 1) with every version pin accepting any version from 1
  - SCHWAY_STATUS_ERR_EXE_INVALID 11 and SCHWAY_STATUS_ERR_EXE_RANGE 12 with texts
  - schway_exe_info (56 bytes), SCHWAY_LICENCE_* and SCHWAY_EXE_LINT_* constants
  - schway_probe_exe and schway_load_exe behind one u64 validator in src/exe.c
  - schway_profile.flags with SCHWAY_PROFILE_DEBUG_PORT, unknown bits refused
  - launch block at RAM 0200h-020Fh (pc0, gp0, s_addr, s_size, little-endian)
  - tests/api/v2.c, tests/exe/exe_tests.c and ten exe/abi tests
affects: [02-09, 02-10, 02-11, 02-12, 02-13, 02-14, 02-16, 02-19, 02-25]

actuals:
  tokens: 26100
  tasks: 2
  commits: 2

plan_head_before: 49415ad288ceec26f864016269d9661a6dc83b4a
plan_head_after: 1fb5911426a4613625fbfeff70427945f49079f2
commits: 2

tech-stack:
  added: []
  patterns:
    - "one validator shared by probe and loader, all arithmetic in u64 before the first write, so a refused file changes nothing"
    - "append-only public structs: schway_profile grows by a flags word, SCHWAY__PROFILE_SIZE_FIRST stays the literal 8, a caller's shorter size tag means no flags"
    - "a later API version keeps frozen earlier test files by relaxing only the version equality to a lower bound"

key-files:
  created:
    - src/exe.c
    - tests/api/v2.c
    - tests/exe/exe_tests.c
  modified:
    - include/schwaystation.h
    - src/internal.h
    - src/instance.c
    - CMakeLists.txt
    - tests/api/v1.c
    - tests/header/header.c
    - tests/header/header.cpp
    - tests/install/tests.cmake
    - tests/abi/tests.cmake
    - tests/abi/inventory.txt
    - tests/embed/CMakeLists.txt
    - tests/exe/tests.cmake
    - tests/exe/inventory.txt

key-decisions:
  - "Status split: ERR_EXE_INVALID is a malformed file (short file, bad signature, text size not a sector multiple or past the file, misaligned t_addr, pc0 zero or misaligned); ERR_EXE_RANGE is a file whose addresses or size leave RAM (n past 0800h + 2 MiB, KSEG2 text, text or stack ending past RAM)"
  - "s_addr is checked as physical(s_addr) + s_size <= 200000h when non-zero; a range ending exactly at the end of RAM is accepted, and s_addr 0 is accepted with any s_size"
  - "An empty text (t_size 0) is accepted but its t_addr must still satisfy the same alignment, segment and end-of-RAM rules"
  - "schway_load_exe refuses with ERR_ARGUMENT when inst->frame is non-zero (a power reset zeroes it), before validating anything"
  - "A schway_profile whose size tag ends before flags sets flags to zero, whatever the bytes past the tag hold"
  - "Launch block at physical 0200h, clear of the exception vector at 80h and the call vectors at A0h, B0h and C0h"

patterns-established:
  - "Test directories that exercise a new object library add it to the embed target allow-list in tests/embed/CMakeLists.txt"
  - "abi.api_version.self_test matches the failing version number tolerantly, so it survives each version bump"

requirements-completed: [CPU-04, CPU-05]

coverage:
  - id: D1
    description: "SCHWAY_API_VERSION is 2 and every place that pinned version 1 (the v1 test, both header tests, the three install consumers and the embed project) accepts any version from 1"
    requirement: CPU-05
    verification:
      - kind: integration
        ref: "ctest -R '^(abi.api_v1|abi.api_v2|abi.api_version|abi.header.c17|abi.header.cxx17|install.consumer|install.consumer.dll_crt|install.consumer.cxx|embed.subdirectory)$'"
        status: pass
    human_judgment: false
  - id: D2
    description: "schway_probe_exe returns every header field from its own offset in little-endian order, hash_len as t_size + 0800h, and keeps a larger caller struct's tail"
    requirement: CPU-05
    verification:
      - kind: unit
        ref: "tests/exe/exe_tests.c#probe_valid (ctest exe.probe_valid)"
        status: pass
    human_judgment: false
  - id: D3
    description: "The shared validator refuses a bad signature, short or oversized file, bad text size, misaligned or unmapped text, text or stack ending past RAM, and a zero or misaligned entry point, and a refused load writes no byte and marks nothing loaded"
    requirement: CPU-04
    verification:
      - kind: unit
        ref: "tests/exe/exe_tests.c#rejects (ctest exe.rejects)"
        status: pass
    human_judgment: false
  - id: D4
    description: "Limits are exact: text ending at 200000h loads and 0800h more is refused, a header-only file loads, the largest file loads and one byte more is refused, and KUSEG, KSEG0 and KSEG1 aliases load the same bytes"
    requirement: CPU-04
    verification:
      - kind: unit
        ref: "tests/exe/exe_tests.c#boundaries (ctest exe.boundaries)"
        status: pass
    human_judgment: false
  - id: D5
    description: "Overlap of the low 64 KiB and trailing bytes are lint bits, never rejections"
    requirement: CPU-05
    verification:
      - kind: unit
        ref: "tests/exe/exe_tests.c#lint (ctest exe.lint)"
        status: pass
    human_judgment: false
  - id: D6
    description: "licence_area compares the 4Ch text with the three region strings byte for byte; one byte off, a shifted copy or no text is no region"
    requirement: CPU-05
    verification:
      - kind: unit
        ref: "tests/exe/exe_tests.c#licence (ctest exe.licence)"
        status: pass
    human_judgment: false
  - id: D7
    description: "schway_load_exe writes the text at its physical address and the 16-byte launch block at 0200h and nothing else, and a power reset erases both"
    requirement: CPU-04
    verification:
      - kind: unit
        ref: "tests/exe/exe_tests.c#load_writes (ctest exe.load_writes)"
        status: pass
    human_judgment: false
  - id: D8
    description: "A refused load leaves the saved state byte for byte unchanged, and a load after the first frame returns ERR_ARGUMENT, changes nothing and is allowed again after a power reset"
    requirement: CPU-04
    verification:
      - kind: unit
        ref: "tests/exe/exe_tests.c#error_changes_nothing and #after_first_frame (ctest exe.error_changes_nothing, exe.after_first_frame)"
        status: pass
    human_judgment: false
  - id: D9
    description: "schway_profile.flags stores the debug-port bit, refuses unknown bits, treats an older caller's shorter struct as no flags, and the bit survives a power reset and gates the TTY port"
    requirement: CPU-04
    verification:
      - kind: unit
        ref: "tests/exe/exe_tests.c#profile_flags (ctest exe.profile_flags)"
        status: pass
    human_judgment: false
  - id: D10
    description: "Every name version 2 added is touched by a frozen version-2 program"
    requirement: CPU-05
    verification:
      - kind: unit
        ref: "tests/api/v2.c (ctest abi.api_v2, abi.api_version)"
        status: pass
    human_judgment: false

duration: 25min
completed: 2026-11-09
status: complete
---

# Phase 2 Plan 08: PS-EXE loader and API version 2 Summary

**API version 2 with a PS-EXE probe and loader behind one u64 validator, a launch block at RAM 0200h, the profile flags word, and every version pin relaxed to a lower bound**

## Performance

- **Duration:** about 25 min
- **Completed:** 2026-11-09T19:48:44Z
- **Tasks:** 2
- **Files modified:** 16 (3 created, 13 modified)

## Accomplishments

- `SCHWAY_API_VERSION` is 2; the two exe statuses, the licence and lint constants and `schway_exe_info` (56 bytes, offsets pinned by `_Static_assert`) are in the public header, and `schway_probe_exe` and `schway_load_exe` are implemented in the new `src/exe.c` object library.
- One validator checks everything in u64 before any write; the loader then copies the text segment to its physical address and the launch block (pc0, gp0, s_addr, s_size) to 0200h and changes nothing on an error.
- `schway_profile` gained `flags` at offset 8 (size 12) with `SCHWAY_PROFILE_DEBUG_PORT`; `SCHWAY__PROFILE_SIZE_FIRST` stays the literal 8 and `schway_set_profile` refuses unknown bits.
- Nine `exe.*` tests plus `abi.api_v2` cover the validator's rules, limits, lint, licence, load layout, error atomicity, the first-frame rule and the profile flag.

## Task Commits

1. **Task 1: version 2 with the probe, every version pin relaxed (tracer)** - `6cee72a` (feat)
2. **Task 2: loader, profile flags, validator tests** - `1fb5911` (feat)

**Plan metadata:** the docs commit that carries this file, STATE.md and ROADMAP.md.

Tracer gate: Task 1's verify (full ci workflow plus the named tests) passed end to end before Task 2 started.

## Files Created/Modified

- `src/exe.c` - the shared validator, `schway_probe_exe`, `schway_load_exe`
- `include/schwaystation.h` - version 2, statuses, constants, `schway_exe_info`, `schway_profile.flags`, two declarations
- `src/internal.h` - exe_info and profile layout pins, `SCHWAY__EXE_INFO_SIZE_FIRST`
- `src/instance.c` - status texts, `schway_set_profile` stores and checks flags
- `CMakeLists.txt` - `schwaystation_exe` object library in the core archive
- `tests/api/v2.c`, `tests/exe/exe_tests.c` - the version-2 touch test and the white-box exe cases
- `tests/api/v1.c`, `tests/header/header.c`, `tests/header/header.cpp`, `tests/install/tests.cmake` - version pins relaxed
- `tests/abi/tests.cmake`, `tests/abi/inventory.txt`, `tests/exe/tests.cmake`, `tests/exe/inventory.txt`, `tests/embed/CMakeLists.txt` - registration and inventories

## Decisions Made

See `key-decisions`. The status split and the stack-range reading are the only places the plan left room; both follow the plan's wording and are covered by `exe.rejects` and `exe.boundaries`.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] embed.subdirectory lists the library's targets**
- **Found during:** Task 1 (first full ci run)
- **Issue:** `tests/embed/CMakeLists.txt` asserts the exact set of targets the library adds; the new `schwaystation_exe` object library made `embed.subdirectory` fail.
- **Fix:** Added `schwaystation_exe` to the expected list.
- **Files modified:** tests/embed/CMakeLists.txt
- **Verification:** `embed.subdirectory` passes in the ci workflow.
- **Committed in:** 6cee72a

**2. [Rule 3 - Blocking] abi.api_version.self_test matched version 1 literally**
- **Found during:** Task 1 (first full ci run)
- **Issue:** its expected message regex contained `K <= 1`, which the checker prints as `K <= 2` at version 2.
- **Fix:** The regex takes any number (`[0-9]+`), so it survives later bumps.
- **Files modified:** tests/abi/tests.cmake
- **Verification:** `abi.api_version.self_test` passes.
- **Committed in:** 6cee72a

**3. [Rule 2 - Missing critical] Header tests for the new names**
- **Found during:** Task 1
- **Issue:** `tests/header/header.c` and `header.cpp` pin every boundary struct's size-tag position and every function's signature; the new struct and function had none.
- **Fix:** Added `SIZE_FIRST(schway_exe_info)` and a `schway_probe_exe` signature pin to both (plan-listed files).
- **Committed in:** 6cee72a

---

**Total deviations:** 3 auto-fixed (2 blocking, 1 missing critical)
**Impact on plan:** the embed and self-test fixes are files the plan's search of seven pins did not name; both are version-bump fallout, no scope creep.

## Issues Encountered

- The asan preset could not configure here (no sanitizer-capable compiler on the PATH); only the ci preset ran locally. The nofp and wasm32 lanes are left to CI.
- One test expectation was wrong, not the code: a header-only file whose `t_addr` equals the end of RAM is valid under the plan's rule (physical address plus zero is at most 200000h); the test now uses an address past the end.

## Known Stubs

None.

## Threat Flags

None. The loader's trust boundary (a host-supplied byte buffer) is the one the plan's safety prohibition covers; `exe.rejects` and `exe.error_changes_nothing` assert that no byte is written and no state changes on a refusal.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- The boot stub (later plan) can read pc0, gp0, s_addr and s_size from physical 0200h; the address is to be documented in `docs/hardware/cpu.md`, and the loader prose in `docs/embedding.md`, by plan 02-11 with the other version-2 names.
- `schway_poke`, `schway_get_memory`, `schway_control`, `schway_load_bios` and any staging buffer were not added, as the plan prohibits.

## Self-Check: PASSED

- `src/exe.c`, `tests/api/v2.c`, `tests/exe/exe_tests.c`: found on disk.
- Commits `6cee72a` and `1fb5911`: both ancestors of HEAD.
- `cmake --workflow --preset ci`: exit 0, 114 of 114 tests passed.

---
*Phase: 02-the-cpu-matches-the-public-vectors*
*Completed: 2026-11-09*
