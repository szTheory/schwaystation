---
phase: 02-the-cpu-matches-the-public-vectors
plan: 07
subsystem: bus
tags: [bus, ram-mirrors, memory-control, scratchpad, bios-window, unbuilt-devices, irq, cause-bit]

requires:
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: cpu and bus state structs, schway__fault_latch, schway__tty_push, state format 2 (plans 02-01, 02-02)
provides:
  - segment-aware bus decode (schway__bus_segment, one switch on address class, one on bits 4-11 of the I/O page)
  - RAM_SIZE bank decode with mirrors and bus errors past the banks
  - memory-control registers with a derived per-port cost table rebuilt on a delay write and on a state load
  - scratchpad, BIOS window over the host image, EXP1/EXP2/EXP3 regions and the debug ports behind the profile flag
  - schway__bus_unbuilt for the seven documented-but-unbuilt devices, with a saturating count and the last access
  - I_STAT/I_MASK with edge rules and CAUSE bit 10 updated at each change
  - docs/hardware/bus.md
affects: [02-08, 02-09, 02-10, 02-12, 02-14, 02-16, 02-19, 02-25]

actuals:
  tokens: 20300
  tasks: 3
  commits: 3

plan_head_before: 2ee40687383a9384f85800f5772865ef0018e625
plan_head_after: 900f01f1fab581e09693e622ecb16660e72c0d7f
commits: 3

tech-stack:
  added: []
  patterns:
    - "the CPU passes the virtual address and the bus decodes the segment, so segment-dependent rules (scratchpad gap in KSEG1) live in one place"
    - "I/O-page registers use lane helpers (reg_get, reg_put) so byte and halfword accesses reach their own bytes"
    - "one switch case per unbuilt device, each calling schway__bus_unbuilt; no function-pointer tables, no host-endian casts"

key-files:
  created:
    - tests/bus/bus_tests.c
    - tests/core/bus_unbuilt.c
    - docs/hardware/bus.md
  modified:
    - src/bus.c
    - src/bus.h
    - src/irq.c
    - src/cpu_exec.inc
    - tests/bus/tests.cmake
    - tests/bus/inventory.txt
    - tests/core/tests.cmake
    - tests/core/inventory.txt
    - tests/core/test_state.c

key-decisions:
  - "The CPU hands the bus the virtual address (cpu_physical removed from src/cpu_exec.inc); the bus decodes the segment, which the scratchpad's KSEG1 gap needs"
  - "RAM_SIZE bank rule is the psx-spx DRAM_CTRL one: /RAS0 = 1 MiB << (bit9*2 + bit11), /RAS1 likewise from bit8 and bit11 when bit10 enables it; a bank larger than the 2 MiB RAM mirrors it, a second bank maps onto the same RAM"
  - "Cost formula source is the psx-spx Memory Control formula (PSX-SYSTEM section 3, SYS.03), used as published: COM0, COM2 and COM3 only, COM1 (hold) adds nothing; reproduces the 29-cycle BIOS word"
  - "Writes to the EXP1 and EXP3 regions are dropped with no fault; only their reads and fetches are bus errors"
  - "Debug ports act only for byte writes with the profile flag set; anything else at 1F802080h and 1F802082h answers and does nothing, and no fault is latched"
  - "Registers with no delay register of their own (memory control, RAM_SIZE, I_STAT/I_MASK, SIO, DMA, timers, GPU, MDEC) charge one cycle, marked approximate"

patterns-established:
  - "Unaligned RAM accesses are located byte by byte; aligned ones locate once"
  - "A bus error latches the first fault inside the bus, naming the device (UNMAPPED for holes), with opcode 0 until the exception logic supplies it"

requirements-completed: [CPU-03]

coverage:
  - id: D1
    description: "Segment classification and KUSEG/KSEG0/KSEG1 aliasing onto addr & 1FFFFFFFh; KSEG2 answers only FFFE0130h by word access and every other KSEG2 address is a bus error with a first fault"
    requirement: CPU-03
    verification:
      - kind: unit
        ref: "tests/bus/bus_tests.c#kseg2 (ctest bus.kseg2)"
        status: pass
    human_judgment: false
  - id: D2
    description: "RAM as 2 MiB mirrored across 8 MiB as RAM_SIZE selects, bus error and first fault past the banks, and a write through one mirror read through the other three"
    requirement: CPU-03
    verification:
      - kind: unit
        ref: "tests/bus/bus_tests.c#ram_mirrors (ctest bus.ram_mirrors)"
        status: pass
    human_judgment: false
  - id: D3
    description: "Memory-control registers read back within their documented bits and every delay-register write rebuilds that port's six costs from the published formula; the same write twice gives the same table"
    requirement: CPU-03
    verification:
      - kind: unit
        ref: "tests/bus/bus_tests.c#memctl_costs (ctest bus.memctl_costs)"
        status: pass
    human_judgment: false
  - id: D4
    description: "Scratchpad is data in KUSEG and KSEG0, a gap in KSEG1, and a fetch from it is a bus error"
    requirement: CPU-03
    verification:
      - kind: unit
        ref: "tests/bus/bus_tests.c#scratchpad (ctest bus.scratchpad)"
        status: pass
    human_judgment: false
  - id: D5
    description: "BIOS window reads the host image, is a gap while none is mapped or past its end, drops ROM writes, keeps the image across a power reset, and costs what DEV2 says"
    requirement: CPU-03
    verification:
      - kind: unit
        ref: "tests/bus/bus_tests.c#bios_window (ctest bus.bios_window)"
        status: pass
    human_judgment: false
  - id: D6
    description: "EXP1 and EXP3 reads are bus errors naming them, EXP2 answers only 1F802080h and 1F802082h, and those ports act only with the debug-port flag set"
    requirement: CPU-03
    verification:
      - kind: unit
        ref: "tests/bus/bus_tests.c#exp_regions (ctest bus.exp_regions)"
        status: pass
    human_judgment: false
  - id: D7
    description: "Every I/O-page address at widths 1, 2 and 4, read and write, is owned, unbuilt or a gap, none in two classes or none, including range edges; unbuilt devices read 0, drop writes, count and keep the last access"
    requirement: CPU-03
    verification:
      - kind: unit
        ref: "tests/core/bus_unbuilt.c (ctest core.bus_unbuilt)"
        status: pass
    human_judgment: false
  - id: D8
    description: "I_STAT sets on a 0-to-1 edge through schway__irq_raise, a written 0 clears and 1 leaves, I_MASK keeps bits 0-10, and CAUSE bit 10 equals (I_STAT & I_MASK) != 0 right after each change"
    requirement: CPU-03
    verification:
      - kind: unit
        ref: "tests/bus/bus_tests.c#irq_edge, irq_line, irq_cause_bit (ctest bus.irq_edge, bus.irq_line, bus.irq_cause_bit)"
        status: pass
    human_judgment: false
  - id: D9
    description: "docs/hardware/bus.md documents the physical map, mirrors, scratchpad, BIOS window, memory control with the cost formula and a worked example, unbuilt devices, gaps, debug ports and the interrupt registers, with no planning identifiers"
    requirement: CPU-03
    verification:
      - kind: other
        ref: "ctest -R '^docs\\.budgets$' (exit 0) and grep for planning identifiers in docs/hardware/bus.md (no match)"
        status: pass
    human_judgment: false

duration: 14min
completed: 2026-11-09
status: complete
---

# Phase 2 Plan 7: The register-level bus Summary

**Segment-aware bus with RAM_SIZE bank mirrors, a delay-register cost table, scratchpad, BIOS window, counted unbuilt devices, faulting gaps, gated EXP2 debug ports, and I_STAT/I_MASK driving CAUSE bit 10**

## Performance

- **Duration:** 14 min
- **Started:** 2026-11-09T19:28:00Z
- **Completed:** 2026-11-09T19:42:00Z
- **Tasks:** 3
- **Files modified:** 12 (3 created, 9 modified)

## Accomplishments

- `src/bus.c` decodes the virtual address by segment and then by one switch on address class (RAM, scratchpad, I/O page, EXP2, EXP1, BIOS, EXP3, gap); inside the I/O page one switch on bits 4-11 reaches memory control, `RAM_SIZE`, the interrupt registers and the seven unbuilt devices.
- `RAM_SIZE` (power-on 0B88h) selects the DRAM banks: an 8 MiB bank mirrors the 2 MiB four times, 0888h is one 2 MiB bank, and an access past the banks is a bus error naming RAM.
- Every DEVn delay write, and a COM_DELAY write, rebuilds the six-entry cost table of the port(s) from the psx-spx formula; a state load rebuilds it too, since the table is derived and not saved.
- Gaps return a bus error and latch the first fault with the device named; unbuilt devices read 0, drop writes and count in `unbuilt_n` with the last access kept.
- `irq.c` implements the edge rule and I_MASK bounds and sets CAUSE bit 10 to `(I_STAT & I_MASK) != 0` at each change.
- `core.bus_unbuilt` sweep counts, per width-address pair for reads: owned=144 unbuilt=3756 gap=8388 (writes give the same counts).
- Cost formula source: psx-spx Memory Control, as cited by PSX-SYSTEM section 3 (SYS.03). It reproduces PSX-SYSTEM's 29-cycle BIOS word read (8 + 7 + 7 + 7) and the maximum-delay power-on table (24, 47, 93).

## Task Commits

Each task was committed atomically:

1. **Task 1: Tracer - a word written through KSEG1 RAM reads back through every RAM mirror** - `51aae6b` (feat)
2. **Task 2: Memory control, scratchpad, BIOS window, unbuilt devices, gaps, debug ports, I_STAT/I_MASK and CAUSE bit 10** - `e2d160c` (feat)
3. **Task 3: The bus page** - `900f01f` (docs)

**Plan metadata:** recorded in the docs(02-07) completion commit that carries this file.

## Files Created/Modified

- `src/bus.c` - physical decode, mirrors, scratchpad, BIOS window, memory control and costs, unbuilt devices, gaps, debug ports
- `src/bus.h` - `schway__bus_segment`, port enum and cost table, `schway__bus_unbuilt`, interrupt write entries, power-on RAM_SIZE and debug-port flag
- `src/irq.c` - I_STAT edges, I_MASK, the line and CAUSE bit 10
- `src/cpu_exec.inc` - the CPU now passes the virtual address to the bus
- `tests/bus/bus_tests.c`, `tests/bus/tests.cmake`, `tests/bus/inventory.txt` - nine `bus.*` cases
- `tests/core/bus_unbuilt.c`, `tests/core/tests.cmake`, `tests/core/inventory.txt` - the I/O-page sweep
- `tests/core/test_state.c` - bus-error addresses moved to the 8 MiB window; the fault record is cleared before the first-fault check
- `docs/hardware/bus.md` - the bus page

## Decisions Made

See `key-decisions` above. The ones a later plan will lean on: the bus takes virtual addresses; EXP1/EXP3 writes are silent; debug ports are byte-write only and never fault; non-port I/O registers cost one cycle (approximate).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] The CPU passed the bus a segment-stripped address**
- **Found during:** Task 1
- **Issue:** `cpu_physical` in `src/cpu_exec.inc` dropped the segment bits of KSEG0 and KSEG1 before the bus call, so the bus could not tell the scratchpad's KSEG1 gap from KSEG0 data, nor decode KSEG2. The plan lists no CPU file.
- **Fix:** removed `cpu_physical`; the three access front-ends pass the virtual address and the bus decodes the segment. The replay-log path is unchanged.
- **Files modified:** `src/cpu_exec.inc`, `src/bus.h` (parameter documented as virtual)
- **Verification:** `vectors.r3000.sample` still passes 2,100 cases; `bus.kseg2`, `bus.scratchpad`
- **Committed in:** `51aae6b`

**2. [Rule 1 - Bug] The state test assumed a 2 MiB RAM and an untouched fault record**
- **Found during:** Task 1
- **Issue:** `core.state.cpu_fields` read 200000h and a word at 1FFFFEh expecting bus errors; with the 0B88h power-on RAM_SIZE the plan mandates they are mirrors, and the bus errors latch a first fault that the test then expected to be empty.
- **Fix:** the test reads 7FFFFEh and 800000h for the errors and clears the fault record before its own latch.
- **Files modified:** `tests/core/test_state.c`
- **Verification:** `core.state.cpu_fields` passes
- **Committed in:** `51aae6b`

**3. [Rule 3 - Blocking] The sweep restores the small state instead of running a full power reset per access**
- **Found during:** Task 2
- **Issue:** the plan asks for a power reset between accesses; 24,576 resets each clear 3 MiB of RAM and VRAM, which risks the 10 s test timeout on slow lanes.
- **Fix:** `core.bus_unbuilt` puts back every field an I/O-page access can touch (memory control, costs, RAM_SIZE, cache control, unbuilt counters, fault record, interrupt registers, CAUSE) from a power-on snapshot before each access; RAM and VRAM are never reached by that page.
- **Files modified:** `tests/core/bus_unbuilt.c`
- **Verification:** `core.bus_unbuilt` passes in well under a second
- **Committed in:** `e2d160c`

**4. [Rule 1 - Correctness] Cost formula omits the COM1 hold period**
- **Found during:** Task 2
- **Issue:** the plan says "COM0-3 when bits 8-11 select them"; the published formula uses COM0, COM2 and COM3 only, and COM1 appears nowhere in it.
- **Fix:** the formula is implemented exactly as published and the page says COM1 adds nothing, so every value stays traceable to a document and to PSX-SYSTEM's 29-cycle BIOS word.
- **Files modified:** `src/bus.c`, `docs/hardware/bus.md`
- **Verification:** `bus.memctl_costs` hand-worked values (24/47/93, 8/15/29, 22/22/43, 7/13/25)
- **Committed in:** `e2d160c`

---

**Total deviations:** 4 auto-fixed (2 Rule 1, 2 Rule 3)
**Impact on plan:** None changes behaviour the plan's truths name. The CPU-side edit was required by the scratchpad and KSEG2 truths; plan 02-14 keeps working from the virtual address it already receives in `cpu_fetch`.

## Issues Encountered

None beyond the deviations above.

## Known Stubs

None. The seven unbuilt devices are the documented intermediate state (they read 0, drop writes and count); each later phase replaces its own case.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- The bus interface the CPU calls (`schway__bus_read`, `_write`, `_fetch` with virtual addresses and a cycle charge) is complete for this phase's register-level map.
- `bus.bios` and `bus.bios_size` are NULL/0 until plan 02-16 maps the stub, so BIOS accesses are gaps until then.
- Plan 02-14 adds the instruction cache and write queue on top of `cpu_fetch`; the KSEG2 cache-control word is stored in `bus.cache_ctrl` but has no meaning yet.
- The frame driver does not raise I_STAT bit 0 yet; `schway__irq_raise` is the internal entry for it.

## Self-Check: PASSED

- Files created and modified all exist (`src/bus.c`, `src/bus.h`, `src/irq.c`, `tests/bus/bus_tests.c`, `tests/core/bus_unbuilt.c`, `docs/hardware/bus.md`, the registration pairs).
- Commits `51aae6b`, `e2d160c`, `900f01f` are ancestors of HEAD.
- `cmake --workflow --preset ci` exits 0 (104 tests); `ctest -L bus` passes 10 of 10; acceptance greps hold (`schway__bus_unbuilt(` count 8, no host-endian pointer casts, no planning identifiers in the page).

---
*Phase: 02-the-cpu-matches-the-public-vectors*
*Completed: 2026-11-09*
