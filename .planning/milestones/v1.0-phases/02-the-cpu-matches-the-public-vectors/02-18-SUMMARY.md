---
phase: 02-the-cpu-matches-the-public-vectors
plan: 18
subsystem: cpu
tags: [tty, putchar, debug-port, stub-kernel, public-api, fixtures]

requires:
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: the stub kernel and its committed array (02-16), the run loop and hello.exe boot (02-17), the TTY ring in the instance (02-02) and the debug-port decode behind the profile flag (02-07)
provides:
  - the stub's A(3Ch) and B(3Dh) putchar entries as real MIPS in ROM, with the three kernel call slots at 800000A0h-800000CFh written by the reset code
  - schway_tty and schway_get_tty, the public pull ring that drains TTY bytes in write order with a drop count
  - hello.exe printing one line through the port and one through putchar
  - fixtures.tty.hello, fixtures.tty.ring and fixtures.tty.flag_off
  - the guide's TTY section and the CPU page's kernel call paragraph
affects: [02-19, 02-20, 02-21, 02-22, 02-23, 02-24, 02-25]

actuals:
  tokens: 7500
  tasks: 3
  commits: 3

plan_head_before: ea6b707e115b2f1368376390d01ef5fe6fade406
plan_head_after: ac716a0655fc8ff030ba4db477ba903a88c48473
commits: 3

tech-stack:
  added: []
  patterns:
    - "a kernel call entry is a four-word RAM slot that jumps to a ROM routine using only k0 and k1, so the caller's registers survive"
    - "a boundary struct that carries a caller buffer follows the audio layout: size, reserved0, pointer, capacity, written, dropped"

key-files:
  created: []
  modified:
    - tests/fixtures/src/stub/stub.asm
    - src/stub_kernel.c
    - tests/fixtures/manifest.txt
    - PROVENANCE.md
    - tests/fixtures/src/hello.asm
    - include/schwaystation.h
    - src/internal.h
    - src/instance.c
    - tests/api/v2.c
    - tests/fixtures/boot_tests.c
    - tests/fixtures/tests.cmake
    - tests/fixtures/inventory.txt
    - docs/embedding.md
    - docs/hardware/cpu.md
    - docs/hardware/bus.md

key-decisions:
  - "the stub is 620 bytes: the reset code copies a 48-byte table of three four-word entry slots with a loop, and the A and B routines share one return"
  - "a drain with capacity 0 reports and clears the drop count and leaves the bytes queued; bytes may be NULL only then"
  - "the drop count saturates at the largest 32-bit value and a load leaves the ring alone, as the ring is outside the state"

patterns-established:
  - "a fixture that calls the kernel writes `jal 0x800000A0` with the function number in the delay slot; armips encodes the target in the caller's 256 MB region"

requirements-completed: [CPU-04]

coverage:
  - id: D1
    description: "The stub implements the putchar entries as MIPS: A(3Ch) and B(3Dh) store the low byte of a0 to 1F802080h and return, other function numbers and C0h return at once, and no C hook watches the entries"
    requirement: "CPU-04"
    verification:
      - kind: integration
        ref: "tests/fixtures/boot_tests.c#tty_hello, run as fixtures.tty.hello (exit 0), whose second line travels only through jal 800000A0h"
        status: pass
      - kind: unit
        ref: "tests/fixtures/boot_tests.c#stub_drift, run as fixtures.stub_drift (exit 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: "schway_tty and schway_get_tty drain up to capacity bytes of the 4 KiB ring in write order, report and clear the bytes dropped since the last drain, and refuse a NULL buffer with capacity, a NULL argument and a size below the first released size"
    requirement: "CPU-04"
    verification:
      - kind: unit
        ref: "tests/api/v2.c, run as abi.api_v2 (exit 0)"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci (exit 0, 167 of 167 tests)"
        status: pass
    human_judgment: false
  - id: D3
    description: "hello.exe with the debug-port flag drains exactly hello port and hello putchar, each ending in a newline, in that order, and a second drain finds nothing"
    requirement: "CPU-04"
    verification:
      - kind: integration
        ref: "tests/fixtures/boot_tests.c#tty_hello, run as fixtures.tty.hello (exit 0)"
        status: pass
    human_judgment: false
  - id: D4
    description: "5,000 port bytes in one frame leave 4,096 in write order, the first drain reports 904 dropped and the next reports 0, and a partial drain keeps the order of the rest"
    requirement: "CPU-04"
    verification:
      - kind: integration
        ref: "tests/fixtures/boot_tests.c#tty_ring, run as fixtures.tty.ring (exit 0)"
        status: pass
    human_judgment: false
  - id: D5
    description: "With the profile flag clear the ring stays empty, hello runs to its end with no fault, no unbuilt-device count and no pending exit"
    requirement: "CPU-04"
    verification:
      - kind: integration
        ref: "tests/fixtures/boot_tests.c#tty_flag_off, run as fixtures.tty.flag_off (exit 0)"
        status: pass
    human_judgment: false
  - id: D6
    description: "The committed stub array, the manifest digest of src/stub_kernel.c and the image digest in PROVENANCE follow the regenerated stub, and the guide names every new public name"
    requirement: "CPU-04"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^(fixtures\\.stub_drift|docs\\.)' --no-tests=error (exit 0); grep of the stub.bin sha256 in PROVENANCE.md (exit 0)"
        status: pass
    human_judgment: false

duration: 3min
completed: 2026-11-09
status: complete
---

# Phase 2 Plan 18: TTY text through the core Summary

**The stub implements putchar as real MIPS entries at A0h and B0h that store to the 1F802080h debug port, and `schway_get_tty` drains that 4 KiB drop-counting ring in write order, with hello.exe printing a line through each source.**

## Performance

- **Duration:** 3 min
- **Started:** 2026-11-09T22:01:19Z
- **Completed:** 2026-11-09T22:04:00Z
- **Tasks:** 3
- **Files modified:** 15

## Accomplishments

- The reset code copies three four-word slots to `800000A0h`, `800000B0h` and `800000C0h`. A and B jump to ROM routines that use only `k0` and `k1`, and for t1 = 3Ch (A) or 3Dh (B) store a0's low byte to the debug port and return; any other number, and C, return at once. The image grew from 524 to 620 bytes.
- `schway_tty` and `schway_get_tty` are pinned like `schway_audio` with `SCHWAY__TTY_SIZE_FIRST`. The ring and its drop counter were already in the instance outside the state; this plan added the drain.
- hello.exe prints `hello port` through the port and `hello putchar` through `jal 800000A0h`; `fixtures.tty.hello` drains the 25 bytes in order.
- `fixtures.tty.ring` checks overflow (4,096 kept in order, 904 dropped once, a partial drain) and `fixtures.tty.flag_off` the retail case.
- The guide has a TTY section; the CPU page documents the kernel calls.

## Task Commits

1. **Task 1: Tracer - hello prints through the port and putchar, and a host drains both lines** - `6eb6235` (feat)
2. **Task 2: Overflow and the retail flag** - `0660d15` (test)
3. **Task 3: The guide's TTY section** - `ac716a0` (docs)

**Plan metadata:** recorded in the docs commit that carries this file.

The tracer was verified end to end before expansion: the full ci workflow and the five named tests passed.

## Files Created/Modified

- `tests/fixtures/src/stub/stub.asm`, `src/stub_kernel.c` - the entry slots, the putchar routines and the regenerated array
- `tests/fixtures/manifest.txt`, `PROVENANCE.md` - the array digest and the 620-byte image digest
- `tests/fixtures/src/hello.asm` - two printed lines
- `include/schwaystation.h`, `src/internal.h`, `src/instance.c` - `schway_tty`, `schway_get_tty` and the layout pins
- `tests/api/v2.c` - the drain's fields and refusals
- `tests/fixtures/boot_tests.c`, `tests/fixtures/tests.cmake`, `tests/fixtures/inventory.txt` - the three `fixtures.tty.*` cases
- `docs/embedding.md`, `docs/hardware/cpu.md`, `docs/hardware/bus.md` - the TTY section, the kernel call paragraph and the new image size

## Decisions Made

- The entry slots are copied by a loop over a 48-byte table rather than by straight-line stores, to keep the reset code short.
- A drain with `capacity` 0 reports and clears `dropped` and leaves the bytes queued; this differs from audio only in that the count is reported, and the guide says so.
- The ring stays outside the state and every hash; a load leaves it alone and a power reset empties it.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] The tracer's verify runs the whole ci workflow, which includes `docs.api_names`**
- **Found during:** Task 1
- **Issue:** with the header changed and no guide text, `docs.api_names` failed, so the tracer's `cmake --workflow --preset ci` could not pass.
- **Fix:** the tracer commit carries a short TTY section; Task 3 replaced it with the full one.
- **Files modified:** docs/embedding.md
- **Verification:** `cmake --workflow --preset ci` exit 0 before the tracer commit
- **Committed in:** 6eb6235 and ac716a0

**2. [Rule 1 - Bug] Three documents still gave the old stub facts**
- **Found during:** Task 1
- **Issue:** the CPU page said the stub does not write the call entries, and it and the bus page and PROVENANCE gave 524 bytes.
- **Fix:** the table row, a kernel calls paragraph and the new size, 620 bytes.
- **Files modified:** docs/hardware/cpu.md, docs/hardware/bus.md, PROVENANCE.md
- **Committed in:** 6eb6235

**3. [Rule 3 - Blocking] `jal 0xA0` is written `jal 0x800000A0` in hello.asm**
- **Found during:** Task 1
- **Issue:** the plan's spelling names an address outside the caller's 256 MB region, which armips can refuse; the encoding is the same.
- **Fix:** the full KSEG0 address. The call lands on the same entry.
- **Files modified:** tests/fixtures/src/hello.asm
- **Committed in:** 6eb6235

---

**Total deviations:** 3 auto-fixed (1 bug, 2 blocking)
**Impact on plan:** no scope growth; each was needed for a passing workflow or for text that stated the old stub.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 02-20 can wire the runner's TTY flags to `schway_get_tty`.
- The stub now has 620 bytes; later plans that add kernel calls extend the slot table and its digests the same way.

## Self-Check: PASSED

- Files: stub.asm, stub_kernel.c, hello.asm, schwaystation.h, instance.c, boot_tests.c, embedding.md present.
- Commits 6eb6235, 0660d15 and ac716a0 are ancestors of HEAD.
- `cmake --workflow --preset ci` exit 0 (167 of 167 tests); the acceptance greps and the stub.bin digest check exit 0.

---
*Phase: 02-the-cpu-matches-the-public-vectors*
*Completed: 2026-11-09*
