---
phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
plan: 15
subsystem: dma
tags:
  - dma
  - first-fault
  - public-api
  - api-version-3
  - runner

requires:
  - phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
    provides: the CHCR write path, the sync modes, and the register sweep that left the reserved-mode word out (plans 08, 10, 14)
provides:
  - SCHWAY_FAULT_DMA (10), a first-fault kind for a DMA setup the model does not run, and SCHWAY_API_VERSION 3
  - the reserved sync mode 3 on a channel other than OTC no longer starts and latches the register, the value and the writing instruction
  - tests/api/v3.c and abi.api_v3, the frozen consumer of the new name
  - the runner's kind name dma
affects:
  - 03-16 (the dma_chain fixture completes the requirement)
  - phase-4 GPU, phase-5 SPU and MDEC, phase-6 CD-ROM device ends (a mode a device cannot use is reported the same way)

actuals:
  tokens: 11000
  tasks: 2
  commits: 2

plan_head_before: 56037e85c3808c93fe528a4711ff3c88a2afbf26
plan_head_after: 8a5384bdef32ce1b2bd3b853e6ea768181d79e01
commits: 2

tech-stack:
  added: []
  patterns:
    - "a frozen API consumer reads a new constant back through the public record from a short program that causes it, not only as a number"
    - "a register sweep names the one access that may latch a fault and fails on any other"

key-files:
  created:
    - tests/api/v3.c
  modified:
    - include/schwaystation.h
    - src/internal.h
    - src/state.c
    - src/dma.c
    - runner/trace.c
    - tests/runner/test_trace.c
    - tests/dma/dma_tests.c
    - tests/dma/tests.cmake
    - tests/dma/inventory.txt
    - tests/abi/tests.cmake
    - tests/abi/inventory.txt
    - docs/embedding.md
    - docs/runner.md
    - docs/hardware/dma.md

key-decisions:
  - "A write that leaves a non-OTC channel idle with bit 24 set and the mode field at 3 does not start it: bits 24 and 28 are dropped and every other bit is kept, so the channel is not busy and a later legal write starts it as usual"
  - "The fault carries the access's own width (the plan names 4; a word write gives 4, a narrower write that completes the same state gives its own width), the CHCR address aligned down to the register, and the value written in the opcode field"
  - "A write to a channel that is running is not modelled and latches nothing, as before"
  - "No DMA trace event: trace_push stays static in cpu.c and the runner's footer keeps its unbuilt line for the GPU, SPU, CD-ROM and MDEC ends"

patterns-established:
  - "an unusable device setup is a first fault with its own kind, never a silent stall"

requirements-completed: []

coverage:
  - id: D1
    description: "A CHCR write with the start bits and sync mode 3 on a channel other than OTC does not start it and latches a first fault of kind SCHWAY_FAULT_DMA naming the DMA device, the CHCR address, the value written and the writing instruction's program counter; nothing moves, the channel is not busy, no flag or interrupt follows, a second hostile write leaves the first record, mode 3 without the start bit and OTC with the same bits latch nothing, and a later write of a usable mode moves the words and finishes with the flag and the interrupt"
    requirement: "GTE-02"
    verification:
      - kind: unit
        ref: "tests/dma/dma_tests.c#fault_sync3 (dma.fault.sync3)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The register sweep over every DMA offset, three widths and four edge values accepts the DMA fault only for the word FFFFFFFFh into a CHCR below OTC's and no other fault, with the same CRC on two passes"
    requirement: "GTE-02"
    verification:
      - kind: unit
        ref: "tests/dma/dma_tests.c#regs_sweep (dma.regs.sweep)"
        status: pass
    human_judgment: false
  - id: D3
    description: "The public change is additive and versioned: SCHWAY_API_VERSION is 3, the internal enum and its static assert match the public value, the state's fault-kind range admits 10, and the runner names the kind dma and still reports 11 as unknown"
    requirement: "GTE-02"
    verification:
      - kind: unit
        ref: "tests/runner/test_trace.c (runner.trace.*), cmake --workflow --preset ci (core.state.*)"
        status: pass
      - kind: unit
        ref: "abi.api_version in the ci workflow"
        status: pass
    human_judgment: false
  - id: D4
    description: "tests/api/v3.c is the frozen version 3 consumer: it checks the constant and reads the fault back through schway_get_fault from a PS-EXE that writes 11000601h to 1F8010A8h, giving kind 10, the DMA device, width 4, address 1F8010A8h, opcode 11000601h and pc 8001000Ch; abi.api_v3 builds and runs it and tests/api/v1.c and v2.c are untouched"
    requirement: "GTE-02"
    verification:
      - kind: integration
        ref: "ctest -R '^abi\\.api_v[123]$' (abi.api_v3)"
        status: pass
      - kind: other
        ref: "git diff --quiet HEAD -- tests/api/v1.c tests/api/v2.c (exit 0)"
        status: pass
    human_judgment: false
  - id: D5
    description: "docs/embedding.md names SCHWAY_FAULT_DMA in the fault table and the versions paragraph, docs/runner.md has the dma row, and docs/hardware/dma.md states the rule, with no planning identifier"
    requirement: "GTE-02"
    verification:
      - kind: other
        ref: "ctest -R '^docs\\.(api_names|budgets)$' (exit 0); grep -nE 'D-[0-9]{2}|GTE-0[0-9]|PX\\.[0-9]|DEC\\.[0-9]|XFER-' over the edited docs, src/dma.c and the tests (exit 1, no match)"
        status: pass
    human_judgment: false

duration: 16min
completed: 2026-11-10
status: complete
---

# Phase 3 Plan 15: The DMA first fault and API version 3 Summary

**A DMA channel written with the reserved sync mode 3 no longer waits for ever: it does not start and latches a first fault of the new public kind `SCHWAY_FAULT_DMA` (10, API version 3) with the register, the value and the writing instruction, consumed by a frozen `tests/api/v3.c` that reads it back from a real program, named `dma` by the runner and documented.**

## Performance

- **Duration:** 16 min
- **Completed:** 2026-11-10T07:31:00Z
- **Tasks:** 2
- **Files modified:** 15 outside `.planning` (one created)

## Accomplishments

- `SCHWAY_FAULT_DMA` is 10 and `SCHWAY_API_VERSION` is 3. The internal enum, its `_Static_assert`, the state's fault-kind range, the runner's kind table and the trace test's kind list follow; 11 is still `unknown`.
- In `dma.c` a CHCR write that leaves a non-OTC channel idle with bit 24 set and the mode field at 3 drops bits 24 and 28 and latches the fault at the register's address with the value written and the CPU's program counter. A channel in flight is not touched, as before.
- `dma.fault.sync3` checks the whole record, that nothing moves and no flag or interrupt follows, that a second hostile write leaves the first record, that mode 3 without the start bit and OTC with mode bits latch nothing, and that a later legal write of the same channel moves its 64 words with the matching checksum and finishes with the flag and the interrupt. Two mutants (the start bits not dropped, the value left out of the record) each fail it.
- `tests/api/v3.c` checks the constant and runs a six-instruction PS-EXE that writes 11000601h to the channel 2 control register through `schway_run` with stop-on-fault, then reads the record back through `schway_get_fault`. `abi.api_v3` runs it; `abi.api_version` covers every header name at version 3.
- The register sweep now runs the word FFFFFFFFh into the channels below OTC instead of skipping it, and accepts the DMA fault only there.
- The embedding guide's fault table and versions paragraph, the runner's kind table and the DMA page (the rule, the sweep sentence, the state and not-built sections) describe the fault.

## Task Commits

1. **Task 1: Tracer - sync mode 3 on a non-OTC channel latches a DMA first fault that the runner names** - `fd0ff69` (feat)
2. **Task 2: The API version 3 consumer, its abi target, and the docs** - `8a5384b` (feat)

**Plan metadata:** the docs commit that carries this summary, STATE.md and ROADMAP.md.

The tracer gate ran in the default end-of-phase mode with an automated-only verify: `cmake --workflow --preset ci` and the named `ctest -R` set were run, passed, and the expansion task went ahead.

## Files Created/Modified

- `include/schwaystation.h` - `SCHWAY_FAULT_DMA` and version 3
- `src/internal.h`, `src/state.c` - the enum value, its static assert and the state range
- `src/dma.c` - the reserved-mode rule in the CHCR write
- `runner/trace.c`, `tests/runner/test_trace.c` - the `dma` kind name and the table check
- `tests/dma/dma_tests.c`, `tests/dma/tests.cmake`, `tests/dma/inventory.txt` - `fault_sync3`, its registration and the sweep change
- `tests/api/v3.c`, `tests/abi/tests.cmake`, `tests/abi/inventory.txt` - the frozen consumer and its target
- `docs/embedding.md`, `docs/runner.md`, `docs/hardware/dma.md` - the fault described

## Decisions Made

See `key-decisions` above.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] `tests/api/v3.c` and the embedding table row land in Task 1**
- **Found during:** Task 1
- **Issue:** raising `SCHWAY_API_VERSION` to 3 makes `abi.api_version` fail without a `tests/api/v3.c`, and a header name makes `docs.api_names` fail without a page naming it, so Task 1's own verify (`cmake --workflow --preset ci`) could not pass with only the files the plan lists for it.
- **Fix:** Task 1 carries a constant-only `v3.c` and the one fault-table row; Task 2 replaces `v3.c` with the program-reading form and adds the abi target and the rest of the docs.
- **Files modified:** tests/api/v3.c, docs/embedding.md
- **Verification:** `cmake --workflow --preset ci` exit 0 before each commit.
- **Committed in:** fd0ff69

**2. [Rule 1 - Bug] The sweep's skip became an accept-only-here rule that has to cover unaligned words**
- **Found during:** Task 1
- **Issue:** the sweep skipped the word FFFFFFFFh at CHCR offsets `(off & 0xC) == 8`, which includes the unaligned offsets +1 to +3. A word written at +1 shifts into the mode and start bits and latches the fault too (at +2 and +3 it does not), so a rule that named only the aligned offset failed six cases.
- **Fix:** the sweep accepts no fault or the DMA fault, and the DMA fault only for that word at any byte offset within a CHCR below OTC's.
- **Files modified:** tests/dma/dma_tests.c
- **Verification:** `ctest -R '^dma\.regs\.sweep$'` exit 0 inside the ci workflow.
- **Committed in:** fd0ff69

**3. [Interpretation] The fault's width is the access's width**
- **Found during:** Task 1
- **Issue:** the plan's call passes 4 as the width. A narrower write that completes the same CHCR state (the byte at +3 over a stored mode 3) is a one-byte access.
- **Fix:** the record carries the access's own width; the word write the plan names gives 4, and the tests and the frozen consumer assert 4 for it.
- **Files modified:** src/dma.c
- **Committed in:** fd0ff69

---

**Total deviations:** 3 (1 blocking ordering, 1 test-rule bug, 1 interpretation)
**Impact on plan:** none on the plan's names, files or tests.

## Issues Encountered

- None beyond the deviations above.

## Not runnable locally

The `asan`, `be32` and `wasm32` lanes need clang-21 or wasmtime that this host lacks; hosted CI runs them. `cmake --workflow --preset ci` passes locally (330 tests registered, `check_junit` inside the workflow, exit 0). No sanitizer run was made for this plan.

## Known Stubs

None added. The device ends stay the stand-ins plan 03-10 recorded; each device replaces its two cases in its own phase.

## Threat Flags

None. The change reports a hostile DMA setup instead of acting on it; the only new surface is a fault-record value.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Ready for 03-16: the `dma_chain` fixture completes GTE-02 (`requirements-completed` stays empty here for that reason). A fixture program that writes the reserved mode will end with `SCHWAY_FAULT_DMA` as its first fault, which the fixture can pin.
- Plan 03-24's extended N + M split check is untouched.

---
*Phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers*
*Completed: 2026-11-10*

## Self-Check: PASSED
