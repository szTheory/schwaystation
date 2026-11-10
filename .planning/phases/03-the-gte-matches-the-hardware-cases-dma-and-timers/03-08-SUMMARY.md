---
phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
plan: 08
subsystem: dma
tags:
  - dma
  - otc
  - dpcr
  - dicr
  - scheduler
  - cpu-stall

requires:
  - phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
    provides: the DMA state struct, reset and bounded state walk (plan 01); the block clock, the deadline, the event runner, the freeze-until stall in cpu_run and the fifth-store hook (plan 03); schway__bus_ram_word and the DMA bus hook on reads and fetches (plan 04)
provides:
  - the DMA register file at 1F801080h-10FFh at every width, with MADR, BCR, CHCR, DPCR and DICR rules
  - DPCR arbitration (lower value wins, tie to the higher channel number, CPU field inert, master enables) as schway__dma_arbitrate
  - DICR with the derived bit 31, write-1-to-clear flags set only under their enables and bit 23, and the I_STAT bit 3 edge
  - the data-atomic, time-evented unit engine and its step contract (schway__dma_event never leaves schway__dma_next at or before now)
  - OTC end to end with its readback rule, wrap, dropped words and 17/16 cost
  - the CPU hold: schway__dma_bus_access sets stall_until to the running unit's end
  - tests/dma/dma_tests.c with 16 cases and docs/hardware/dma.md
affects:
  - 03-10
  - 03-14
  - 03-15
  - phase-3 plans that add sync modes, device ends, chopping, linked lists and the dma_chain fixture

actuals:
  tokens: 16000
  tasks: 3
  commits: 3

plan_head_before: 012dec7530406f2af8926aa18ffb2ddb52b4fbe7
plan_head_after: 54a5d54229568306dbd3d8cbc9859b7406972a68
commits: 3

tech-stack:
  added: []
  patterns:
    - "a unit moves its words at once through the fault-free RAM word path and sets busy_until; one event call finishes the due unit and starts the next at the same tick, so every return leaves the deadline after now"
    - "arbitration is a function of a candidate mask: the engine passes the channels that have a unit (OTC only for now) and the test passes all seven, so the priority rules are testable before the other channels have units"
    - "a register read that finds a unit running finishes it as of its end tick, because the bus hook has already frozen the CPU until then"

key-files:
  created:
    - tests/dma/dma_tests.c
    - docs/hardware/dma.md
  modified:
    - src/dma.c
    - src/dma.h
    - tests/dma/tests.cmake
    - tests/dma/inventory.txt

key-decisions:
  - "Unit enum: a phase per channel, 0 idle and 1 unit running; cur names the channel and busy_until its end. remain, nodes_done, words and crc stay unused until the block modes, lists and device stubs land"
  - "DMA_UNIT_CHANNELS (bit 6 only) is the mask the engine passes to schway__dma_arbitrate, so channels 0-5 keep registers and wait; plan 03-10 widens the mask"
  - "OTC CHCR is stored as (value & 51000000h), bit 1 is OR-ed on read and never stored, bit 28 is cleared at the unit's start and bit 24 at its end; a write of 70770703h with bit 24 clear reads 50000002h (ps1-tests dma/otc-test)"
  - "DICR bit 31 is bit 15 OR (bit 23 AND any flag): the per-channel enables gate only whether a flag is set, which the nugget 84800000h reading requires (mask 0, flag set, bit 31 set)"
  - "A CHCR write to the channel whose unit is running keeps bit 24 set; the hardware stop by writing 0 is unmodelled"
  - "A DMA register read finishes a running unit as of its end tick before returning; a write only catches the engine up to its own tick"
  - "The interrupt entry reads the word at EPC by peek (plan 03-06), not through the bus, so it never sets a stall; the I-cache refill words reach the hook through a throwaway cycle sink and set the same stamp as the first word"

patterns-established:
  - "OTC cost is ceil(words * 17 / 16): 4,096 words end 4,352 cycles after the CHCR write, 10000h words 69,632"

requirements-completed: []

coverage:
  - id: D1
    description: "A CPU-started OTC of 4,096 words writes the ordering table (each word the address of the one below, the last FFFFFFh), ends 4,352 cycles after the start, reads back 00000002h, and sets the DICR flag and I_STAT bit 3"
    requirement: "GTE-02"
    verification:
      - kind: unit
        ref: "tests/dma/dma_tests.c#otc_clear (dma.otc.clear)"
        status: pass
    human_judgment: false
  - id: D2
    description: "DPCR resets to 07654321h and arbitrates: the lower value wins, a tie goes to the higher channel number, the CPU field changes nothing, a clear enable does not start, and a DPCR write starts a waiting channel"
    requirement: "GTE-02"
    verification:
      - kind: unit
        ref: "tests/dma/dma_tests.c#dpcr_reset,dpcr_priority,dpcr_enable (dma.dpcr.reset, dma.dpcr.priority, dma.dpcr.enable)"
        status: pass
    human_judgment: false
  - id: D3
    description: "DICR derives bit 31 and never stores it, sets a flag only under its enable and bit 23, clears flags by writing 1, reproduces the nugget readings 84840000h, 84800000h, 04000000h and 84008000h, and raises I_STAT bit 3 only on a 0 to 1 change of bit 31"
    requirement: "GTE-02"
    verification:
      - kind: unit
        ref: "tests/dma/dma_tests.c#dicr_reads,dicr_irq_edge (dma.dicr.reads, dma.dicr.irq_edge)"
        status: pass
    human_judgment: false
  - id: D4
    description: "OTC reads back 00000002h or 50000002h as the hardware tests show, starts only with bits 24 and 28, ignores the direction, step, sync and chopping bits, wraps at FFFFFCh dropping words past the RAM window with no fault, and transfers 10000h words for a BCR of 0 in 69,632 cycles"
    requirement: "GTE-02"
    verification:
      - kind: unit
        ref: "tests/dma/dma_tests.c#otc_readback,otc_madr_wrap,otc_needs_trigger,otc_ignores_bits,otc_bcr_zero (dma.otc.readback, dma.otc.madr_wrap, dma.otc.needs_trigger, dma.otc.ignores_bits, dma.otc.bcr_zero)"
        status: pass
    human_judgment: false
  - id: D5
    description: "The event call finishes nothing one tick before busy_until, leaves schway__dma_next equal to busy_until and above now, finishes the unit when called at or past its end and then leaves UINT64_MAX; a CHCR write cannot stop the running unit"
    requirement: "GTE-02"
    verification:
      - kind: unit
        ref: "tests/dma/dma_tests.c#step_contract (dma.engine.step_contract)"
        status: pass
    human_judgment: false
  - id: D6
    description: "A running unit freezes the CPU: a RAM read, a CHCR read and the fifth queued store hold the next instruction until the unit's end, the first four stores and every scratchpad access never wait, and a CHCR read right after the start returns the completed value"
    requirement: "GTE-02"
    verification:
      - kind: unit
        ref: "tests/dma/dma_tests.c#stall_cpu_access,stall_not_scratch,stall_write_queue,otc_read_sees_completion (dma.stall.cpu_access, dma.stall.not_scratch, dma.stall.write_queue, dma.otc.read_sees_completion)"
        status: pass
    human_judgment: false
  - id: D7
    description: "docs/hardware/dma.md (11,755 bytes) covers the registers, arbitration, DICR and the edge, the unit model and step, OTC and its ignored bits, the hold, the dropped word and the sources, with no planning identifier"
    requirement: "GTE-02"
    verification:
      - kind: other
        ref: "grep -qE 'D-[0-9]{2}|GTE-0[0-9]|PX\\.[0-9]|DEC\\.[0-9]|XFER-' docs/hardware/dma.md src/dma.c tests/dma/dma_tests.c (exit 1, no match); ctest -R '^docs\\.' inside cmake --workflow --preset ci (exit 0)"
        status: pass
    human_judgment: false

duration: 15min
completed: 2026-11-10
status: complete
---

# Phase 3 Plan 08: DMA registers, arbitration, the unit engine and OTC Summary

**DPCR arbitration, DICR with the derived master flag and its interrupt edge, a data-atomic and time-evented unit engine, OTC end to end (4,096 words end 4,352 cycles after the start) and a freeze-until CPU hold, with 16 `dma.*` tests and the first DMA page.**

## Performance

- **Duration:** 15 min
- **Started:** 2026-11-10T05:46:00Z (approximate; the start time was not captured)
- **Completed:** 2026-11-10T06:01:25Z
- **Tasks:** 3
- **Files modified:** 6 outside `.planning`

## Accomplishments

- The register window answers at every byte, halfword and word with the stored-bit masks of the state walk; OTC's CHCR keeps bits 24, 28 and 30 and always shows bit 1.
- `schway__dma_arbitrate(inst, mask)` implements the measured priority rules; the engine passes only the channels that have a unit, the test passes all seven.
- `schway__dma_event` finishes due units at their own end ticks and starts the next channel at the same tick, and never leaves the next deadline at or before the tick it was given.
- OTC moves its words through `schway__bus_ram_word` (wrap at FFFFFCh, dropped words past the window, no fault) and costs `ceil(words * 17 / 16)`.
- A read that waits on the bus while a unit transfers freezes the CPU until the unit's end; a DMA register read finishes the unit as of that end, so a CHCR read right after the start already shows completion.
- Mutation checks of `src/dma.c` (no stall, cost rounded down, tie broken the other way, no edge test, a flag set without its channel enable, a CHCR write that clears the running unit's bit 24, no read-completes-unit) each fail at least one `dma.*` test.

## Task Commits

1. **Task 1: Tracer - a CPU-started OTC clears an ordering table in RAM, completes on its event and raises the DMA interrupt** - `4135078` (feat)
2. **Task 2: DPCR arbitration, DICR rules, OTC edges and the dropped word** - `0517851` (test)
3. **Task 3: The CPU stall, the read that sees completion, and the DMA page** - `54a5d54` (test, with the page)

**Plan metadata:** the docs commit that carries this summary, STATE.md and ROADMAP.md.

The tracer gate ran in the default end-of-phase mode with an automated-only verify: `cmake --workflow --preset ci` and `ctest -R '^dma\.otc\.clear$'` were re-run, passed, and the expansion tasks went ahead.

## Files Created/Modified

- `src/dma.c` - register read and write rules, arbitration, DICR and its edge, OTC unit, start and finish, the event step, the bus hold
- `src/dma.h` - the `schway__dma_arbitrate` declaration and the contract comments on the event and the hold
- `tests/dma/dma_tests.c` - the `schwaystation-test-dma` executable with 16 cases
- `tests/dma/tests.cmake`, `tests/dma/inventory.txt` - the target and the 16 registrations
- `docs/hardware/dma.md` - the DMA page

## Decisions Made

See `key-decisions` above. The unit enum chosen is a per-channel phase of 0 (idle) or 1 (unit running).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] The engine, arbitration and stall hook landed in the Task 1 commit**
- **Found during:** Task 1
- **Issue:** The tracer needs registers, start, the event, the flag and the interrupt, which is nearly all of `src/dma.c`; arbitration, the DICR rules and the hold are in the same few functions, so splitting `src/dma.c` across three commits would have meant three artificial intermediate files.
- **Fix:** `src/dma.c` and `src/dma.h` are complete in commit `4135078`; the tests of Tasks 2 and 3 landed in their own commits against it, with the test file and registrations staged per task.
- **Files modified:** src/dma.c, src/dma.h
- **Verification:** `cmake --workflow --preset ci` exit 0 at each of the three commits (259, 270 and 274 tests).
- **Committed in:** 4135078

**2. [Rule 1 - Bug, found in review of my own tests] Test-side corrections before the first commit**
- **Found during:** mutation checks after Task 1's code was written
- **Issue:** the first build of two mutants failed with `-Werror` and left the good binary in place, which looked like surviving mutants; three expected DICR constants and the OTC cost test (all multiples of 16) did not discriminate floor from ceiling, a missing master-only case let a flag set without its channel enable pass, and nothing pinned that a CHCR write cannot stop the running unit.
- **Fix:** added a 4-word cost check (5 cycles), the master-enable-alone case in `dicr_reads`, and the running-unit CHCR write in `step_contract`; corrected two expected DICR values.
- **Files modified:** tests/dma/dma_tests.c
- **Verification:** each mutant above now fails a test.
- **Committed in:** 4135078 and 0517851 (the tests were first committed after these fixes)

**3. [Rule 3 - Blocking] The acceptance grep wants the literal `schway__irq_raise(inst, 3u)`**
- **Found during:** Task 1 acceptance criteria
- **Issue:** an `DMA_IRQ` macro made the criterion's grep fail.
- **Fix:** the call uses `3u` with a comment naming I_STAT bit 3.
- **Files modified:** src/dma.c
- **Committed in:** 4135078

---

**Total deviations:** 3 auto-fixed (2 blocking, 1 bug in my own draft tests)
**Impact on plan:** none on behaviour; the plan's tests, names and files are as written.

## Issues Encountered

- `DICR` bit 31 in the plan text reads "any enabled flag"; psx-spx and the nugget reading 84800000h (mask 0, flag set, bit 31 set) both say the per-channel enables do not enter the read, only the setting of a flag. The code follows the sources and the page says so.
- The five-store case needed the loop's code already cached and `cpu.wq_count` set to 0 at the unit's start, as the plan said; the second pass through the loop would otherwise start with a full queue.

## Not runnable locally

`asan`, `be32` and the `wasm32` lanes need clang-21 or wasmtime that this host lacks; hosted CI runs them. `cmake --workflow --preset ci` passes locally (274 tests, `check_junit` exit 0).

## Known Stubs

Channels 0 to 5 start no transfer: a channel written with bit 24 set stays busy. This is intentional and bound to plan 03-10 (sync modes and device ends); the DMA page says so.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Ready for 03-10 (block modes and device ends): widen `DMA_UNIT_CHANNELS`, add the per-channel `switch (ch)` ends, and keep the event step contract; 03-14 adds chopping and linked lists with the 1000h-node yield; 03-15 adds the sync-mode fault.
- `remain`, `nodes_done`, `words` and `crc` in the state are untouched and ready.

---
*Phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers*
*Completed: 2026-11-10*

## Self-Check: PASSED
