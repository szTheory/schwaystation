---
phase: 02-the-cpu-matches-the-public-vectors
plan: 02
subsystem: core
tags: [cpu, cop0, bus, irq, state-format, fault-record, tty, cmake]

requires:
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: per-directory test inventories (plan 02-01)
provides:
  - OBJECT libraries schwaystation_cpu and schwaystation_bus in the static archive
  - struct schway__cpu, struct schway__cop0, struct schway__bus, struct schway__irq with power-on values
  - first-fault record, enum schway__dev, SCHWAY__FAULT_* kinds, schway__fault_latch
  - 4 KiB TTY ring and exit-port fields outside the state, schway__tty_push
  - state format 2 and behaviour revision 2, with u32 and u8 array walk helpers
  - schway__bus_read, schway__bus_write and schway__bus_fetch over main RAM, schway__irq_raise, schway__irq_line
affects: [02-03, 02-04, 02-05, 02-06, 02-07]

actuals:
  tokens: 10200
  tasks: 3
  commits: 3

plan_head_before: f73222d93fb394d9ec69f55cf2a0273bd39a3117
plan_head_after: 9fb1db71231a39111bae0467f96205dd853548ee
commits: 3

tech-stack:
  added: []
  patterns:
    - "module state structs live in the module header and the instance embeds them, so a later plan edits only its own source file"
    - "one walker per module (schway__cpu_state_io, schway__bus_state_io) called from body_io, every field bounded for VALIDATE"
    - "test programs size state buffers from schway_state_size, never from a constant"

key-files:
  created:
    - src/cpu.h
    - src/cpu.c
    - src/cop0.c
    - src/bus.h
    - src/bus.c
    - src/irq.c
  modified:
    - CMakeLists.txt
    - src/internal.h
    - src/instance.c
    - src/state.h
    - src/state.c
    - tests/core/test_state.c
    - tests/core/test_core.c
    - tests/core/tests.cmake
    - tests/core/inventory.txt
    - tests/embed/CMakeLists.txt

key-decisions:
  - "schway__irq_raise takes an interrupt number 0 to 10, not a mask; any other number is ignored"
  - "I_STAT and I_MASK are bounded to bits 0 to 10 in the state, so a state with a higher bit is refused"
  - "The profile and its flags survive a power reset; exe_loaded, the fault record, the TTY ring and the exit fields do not"
  - "The bus calls leave the cycles argument unchanged until the timing model lands; RAM reads and writes cost 0 here"
  - "last_unbuilt is walked as four named scalars so the device and width carry their own bounds"

requirements-completed: [CPU-01, CPU-03]

coverage:
  - id: D1
    description: "The cpu and bus modules exist as OBJECT libraries inside the static archive with the core options, and the embed check names them"
    requirement: "CPU-01"
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci (exit 0)"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset dev (exit 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: "Power-on sets PC BFC00000h, SR 00400000h, PRID 2, BPCM and BDAM FFFFFFFFh, zero GPRs, HI, LO, CAUSE, RAM, scratchpad, I_STAT and I_MASK"
    requirement: "CPU-01"
    verification:
      - kind: unit
        ref: "tests/core/test_state.c#state.cpu_fields (ctest -R core.state.cpu_fields)"
        status: pass
    human_judgment: false
  - id: D3
    description: "State format 2 saves a GPR, a RAM byte, scratchpad, memory control, an unbuilt counter, interrupt registers and the first-fault record through save, power reset and load byte for byte, with every field bounded and the test card hashes unchanged"
    requirement: "CPU-03"
    verification:
      - kind: unit
        ref: "tests/core/test_state.c#state.cpu_fields (ctest -R core.state.cpu_fields)"
        status: pass
      - kind: unit
        ref: "ctest -R core.state.record_sweep and core.state.rejects (exit 0)"
        status: pass
      - kind: integration
        ref: "ctest -R runner.hash (exit 0)"
        status: pass
    human_judgment: false
  - id: D4
    description: "The first fault is latched only while none is held, the TTY ring keeps 4096 bytes and counts drops, and bus reads and writes assemble little-endian bytes in RAM and refuse addresses past 2 MiB"
    requirement: "CPU-03"
    verification:
      - kind: unit
        ref: "tests/core/test_state.c#state.cpu_fields (ctest -R core.state.cpu_fields)"
        status: pass
    human_judgment: false
  - id: D5
    description: "The core test programs size their state buffers from schway_state_size, so states of format 2 are not refused"
    requirement: "CPU-03"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -L core --no-tests=error (exit 0, 24 tests)"
        status: pass
    human_judgment: false
  - id: D6
    description: "The core options define _FORTIFY_SOURCE=0, so the dev preset passes abi.undefined on macOS and the negative control still fails its fixture"
    requirement: "CPU-03"
    verification:
      - kind: integration
        ref: "cmake --workflow --preset dev (exit 0)"
        status: pass
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^abi\\.undefined(\\.self_test)?$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false

duration: 4min
completed: 2026-11-09
status: complete
---

# Phase 2 Plan 2: CPU and bus modules, state format 2 Summary

**Two new core object libraries (cpu with COP0, bus with irq) holding the register, RAM, interrupt and first-fault state, a 4 KiB TTY ring, and state format 2 that saves all of it with bounded fields.**

## Performance

- **Duration:** 4 min
- **Started:** 2026-11-09T18:49:15Z
- **Completed:** 2026-11-09T18:52:37Z
- **Tasks:** 3
- **Files modified:** 16 (6 created, 10 modified)

## Accomplishments
- `schwaystation_cpu` (`src/cpu.c`, `src/cop0.c`) and `schwaystation_bus` (`src/bus.c`, `src/irq.c`) are OBJECT libraries in the archive, so the CPU, bus, loader and stub plans each own one module file. The state structs sit in `src/cpu.h` and `src/bus.h`.
- Power-on values are the documented ones (PC BFC00000h, SR 00400000h, PRID 2, BPCM and BDAM FFFFFFFFh, everything else zero); `core.state.cpu_fields` checks them and shows a GPR, HI, PC, EPC, a RAM byte, scratchpad, memory control, an unbuilt counter, I_STAT, I_MASK and a latched fault surviving save, power reset and load byte for byte.
- The instance holds the first-fault record (`schway__fault_latch`, first fault wins), `enum schway__dev`, the nine fault kinds, `profile_flags` and `exe_loaded`; the 4 KiB TTY ring with `schway__tty_push` and the exit-port fields sit outside the state.
- State format 2 and behaviour revision 2: 46 recorded fields, each with VALIDATE bounds (kind 0 to 9, device below the device count, width 0 to 4, I_STAT and I_MASK to bit 10, flags 0 to 1); the test card hashes are unchanged (`runner.hash` passes).
- `core.state.*` and the core test program no longer assume the phase-1 state size; both size heap buffers from `schway_state_size`.
- `-D_FORTIFY_SOURCE=0` in the core options: the dev preset now passes `abi.undefined` on macOS and `abi.undefined.self_test` still fails its fixture. This closes todo 019; plan 02-27 alone moves the todo file.

## Task Commits

1. **Task 1: Tracer - a CPU register survives save, reset and load through the cpu module** - `abdf048` (feat)
2. **Task 2: The bus and irq modules, the fault record, the TTY ring and the rest of state format 2** - `a690948` (feat)
3. **Task 3: Fortify off in the core options (todo 019)** - `9fb1db7` (build)

**Plan metadata:** recorded in the docs commit that follows this summary.

## Files Created/Modified
- `src/cpu.h`, `src/cpu.c`, `src/cop0.c` - CPU and COP0 state, power-on, state walk
- `src/bus.h`, `src/bus.c`, `src/irq.c` - device enum, RAM, scratchpad, memory control, I_STAT and I_MASK, little-endian RAM access, state walk
- `src/internal.h` - includes both headers, fault and TTY structs, new instance members, version bumps, layout asserts, declarations
- `src/instance.c` - `schway__fault_latch`, `schway__tty_push`, power-on resets
- `src/state.h`, `src/state.c` - `schway__state_u32_array` and `schway__state_u8_array`, the new fields in the body walk
- `CMakeLists.txt` - the two object libraries, the archive, the options loop, fortify
- `tests/core/test_state.c`, `tests/core/test_core.c` - heap state buffers, `state.cpu_fields`
- `tests/core/tests.cmake`, `tests/core/inventory.txt` - register `core.state.cpu_fields`
- `tests/embed/CMakeLists.txt` - the embed check's list of library targets

## Decisions Made
- `schway__irq_raise` takes an interrupt number (0 to 10), the reading that matches `schway__irq_line` and the 11-bit registers; plan 02-07 calls it for the edge rule.
- The profile flags are kept by a power reset like the region; everything the guest can change is reset.
- RAM reads and writes charge no cycles in this plan (the plan's cost 0); plan 02-07 adds the costs.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] The embed test lists the library's targets**
- **Found during:** Task 1 (the ci workflow runs the embed project)
- **Issue:** `tests/embed/CMakeLists.txt` fails configure unless the library defines exactly the four known targets; two new object libraries break it, and the file is not in the plan's file list.
- **Fix:** Added `schwaystation_cpu` in Task 1 and `schwaystation_bus` in Task 2 to the expected list.
- **Files modified:** `tests/embed/CMakeLists.txt`
- **Verification:** `cmake --workflow --preset ci` exit 0 (embed test passes).
- **Committed in:** `abdf048`, `a690948`

**2. [Rule 2 - Missing Critical] The last unbuilt access is walked as four bounded scalars**
- **Found during:** Task 2
- **Issue:** One four-word array has one bound for all words, so a loaded state could carry a device number past the device count or a width past 4, which a later footer print would index.
- **Fix:** The walk emits `bus.last_unbuilt.dev` (below the device count), `.addr`, `.width` (0 to 4) and `.pc`; the instance field stays the four-word array the plan names.
- **Files modified:** `src/bus.c`
- **Verification:** `core.state.record_sweep` refuses each out-of-range value (46 fields recorded, 16 body values refused).
- **Committed in:** `a690948`

---

**Total deviations:** 2 auto-fixed (1 blocking, 1 missing critical)
**Impact on plan:** Both are small and local; no scope change.

## Issues Encountered

None. Tracer gate: the Task 1 verify command passed end to end before Task 2 began.

## Known Stubs

None. The bus answers main RAM only and returns a bus error elsewhere, as the plan states; plan 02-07 widens the decode. The instance's `bios` pointer is set by no code yet; it is an interface for plan 02-07.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Plans 02-03 onward can edit `src/cpu.c`, `src/bus.c` and their headers independently: the state structs, the bus calls, `schway__fault_latch` and `schway__tty_push` exist.
- The next-PC model (the branch-delay latch) is added by plan 02-06 to `struct schway__cpu` and its walker.

---
*Phase: 02-the-cpu-matches-the-public-vectors*
*Completed: 2026-11-09*

## Self-Check: PASSED

All created files exist (`src/cpu.h`, `src/cpu.c`, `src/cop0.c`, `src/bus.h`, `src/bus.c`, `src/irq.c`) and commits `abdf048`, `a690948` and `9fb1db7` are ancestors of HEAD.
