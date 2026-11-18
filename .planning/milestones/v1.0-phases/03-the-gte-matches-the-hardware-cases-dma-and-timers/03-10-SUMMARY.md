---
phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
plan: 10
subsystem: dma
tags:
  - dma
  - sync-modes
  - device-ends
  - dreq
  - costs

requires:
  - phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
    provides: the unit engine, arbitration, the stall hook and OTC (plan 08); the DMA state struct with remain, dreq, words and crc (plan 01); the fault-free RAM word path (plan 04)
provides:
  - sync mode 0 bursts for the six non-OTC channels in either direction and step, with MADR wrapping at 24 bits
  - sync mode 1 request blocks, one block per raised request line, with a block counter and no CPU hold while the line is low
  - one switch per direction for the device ends, each case a stand-in that counts words and keeps a CRC-32 per channel in the state, the source reading 0
  - per-channel word costs and the RAM row, and the sync 1 block overhead
  - schway__dma_dreq starting a waiting channel when the engine is idle
  - the DMA page's sync mode, device end and cost sections
affects:
  - 03-14
  - 03-15
  - phase-4 GPU, phase-5 SPU and MDEC, phase-6 CD-ROM device ends (each replaces two cases)

actuals:
  tokens: 6600
  tasks: 3
  commits: 3

plan_head_before: b60dbdc1cea7b0cabf797000ec92c08e8413590e
plan_head_after: bc437ad3c48d6075bf558fd41ebe03c5c7655feb
commits: 3

tech-stack:
  added: []
  patterns:
    - "a device end is two cases of one switch each in dma.c; a stand-in case calls a shared sink or source and a built device replaces the case"
    - "a sync 1 block takes its channel's request bit with it; the device raises the line again, so the CPU window between blocks is the time the line is low"
    - "a unit costs n * rate + ceil(n / 16), which is ceil(n * 17 / 16) at one cycle per word, so OTC's cost is the same function"

key-files:
  created: []
  modified:
    - src/dma.c
    - src/dma.h
    - tests/dma/dma_tests.c
    - tests/dma/tests.cmake
    - tests/dma/inventory.txt
    - docs/hardware/dma.md

key-decisions:
  - "The stub sink's word count and CRC-32 live in the existing per-channel words and crc arrays of struct schway__dma, which the state walk already saves; they run from power-on and a power reset clears them, so a test reads them after one transfer on a fresh instance"
  - "A sync 1 block clears the channel's request bit when it starts; schway__dma_dreq raises it again, and a raise with the engine idle starts the channel at once"
  - "A unit costs n * rate + ceil(n / 16): the RAM row is one cycle per 16 words added to the device rate, so 24 for the CD-ROM gives 1,540 cycles for 64 words and 1,572,864 + 4,096 for 10000h, inside the engine's 2,691,328 bound"
  - "The sync 1 block overhead is 11 cycles, labelled derived; the linked-list and reserved modes are not pending, so a channel written with them stays busy until plans 03-14 and 03-15"
  - "A CHCR write to a channel in any phase but idle keeps bit 24 set, not only the channel whose unit runs, because a sync 1 channel between blocks is mid-transfer"
  - "remain in the state walk is now bounded at 10000h (it holds a block count), so a hostile state cannot name billions of blocks"

patterns-established:
  - "an arbitration-only test holds the engine with a fake running unit so a DPCR write cannot start the channels it ranks"

requirements-completed: []

coverage:
  - id: D1
    description: "A sync 0 burst of 256 words from RAM to the GPU channel reaches the stand-in sink, which counts 256 words and checksums the 1,024 bytes, advances MADR by 1,024, ends 272 cycles after the start and raises the flag and the interrupt"
    requirement: "GTE-02"
    verification:
      - kind: unit
        ref: "tests/dma/dma_tests.c#sync0_gpu_sink (dma.sync0.gpu_sink)"
        status: pass
    human_judgment: false
  - id: D2
    description: "A sync 0 read from the CD-ROM channel writes zeros to RAM, steps MADR backward with CHCR bit 1, leaves the words around it alone and costs 24 clocks per word"
    requirement: "GTE-02"
    verification:
      - kind: unit
        ref: "tests/dma/dma_tests.c#sync0_from_device (dma.sync0.from_device)"
        status: pass
    human_judgment: false
  - id: D3
    description: "Sync 1 moves four blocks of 16 words to the SPU channel, one block per raise of the request line at 76 cycles a block, keeps the channel busy with no flag between blocks, and sets the flag and the interrupt after the last"
    requirement: "GTE-02"
    verification:
      - kind: unit
        ref: "tests/dma/dma_tests.c#sync1_dreq_blocks (dma.sync1.dreq_blocks)"
        status: pass
    human_judgment: false
  - id: D4
    description: "A request-mode channel with its line low stays busy and never stalls the CPU: an EXE loop reading RAM counts the same passes as with no transfer started"
    requirement: "GTE-02"
    verification:
      - kind: unit
        ref: "tests/dma/dma_tests.c#sync1_no_dreq_no_stall (dma.sync1.no_dreq_no_stall)"
        status: pass
    human_judgment: false
  - id: D5
    description: "Each channel's 64-word burst ends at 64 * rate + 4 cycles: 1 for the MDEC channels and the GPU, 24 for the CD-ROM, 4 for the SPU, 20 for PIO; a word count of 0 on the CD-ROM stays inside the bound on a unit"
    requirement: "GTE-02"
    verification:
      - kind: unit
        ref: "tests/dma/dma_tests.c#costs_per_channel (dma.costs.per_channel)"
        status: pass
    human_judgment: false
  - id: D6
    description: "A DPCR write while another unit runs decides the next unit at its boundary: raising channel 5 over channel 4 makes 5 go next, and without the write 4 does"
    requirement: "GTE-02"
    verification:
      - kind: unit
        ref: "tests/dma/dma_tests.c#dpcr_rearbitrate (dma.dpcr.rearbitrate)"
        status: pass
    human_judgment: false
  - id: D7
    description: "docs/hardware/dma.md (15,611 bytes) documents the two block modes, the request line, the device-end interface and the cost table with each row labelled doc or derived, and the CD-ROM 24-versus-40 deferral, with no planning identifier"
    requirement: "GTE-02"
    verification:
      - kind: other
        ref: "ctest -R '^docs\\.budgets$' inside cmake --workflow --preset ci (exit 0); grep -qE 'D-[0-9]{2}|GTE-0[0-9]|PX\\.[0-9]|DEC\\.[0-9]|XFER-' docs/hardware/dma.md src/dma.c src/dma.h tests/dma/dma_tests.c (exit 1, no match)"
        status: pass
    human_judgment: false

duration: 7min
completed: 2026-11-10
status: complete
---

# Phase 3 Plan 10: Block modes, device ends and per-channel costs Summary

**Sync mode 0 bursts and sync mode 1 request blocks for the six non-OTC channels through one-switch device ends with a counting, checksumming stand-in, per-channel word costs (CD-ROM at the documented 24, the 40 recorded as a deferral) and DPCR re-arbitration at the unit boundary, with six new `dma.*` tests and the page's block-mode, device-end and cost sections.**

## Performance

- **Duration:** 7 min (by the wall clock between the previous plan's state write and this plan's last commit)
- **Completed:** 2026-11-10T06:39:14Z
- **Tasks:** 3
- **Files modified:** 6 outside `.planning`

## Accomplishments

- `dma_dev_write` and `dma_dev_read` are one `switch (ch)` each with a case per channel 0-5; every case calls the stand-in, so a device replaces exactly two cases. The sink adds one to `words[ch]` and folds the word's four little-endian bytes into `crc[ch]` with `schway__crc32_update`; the source writes 0. No callback table, no function pointer in the state.
- Sync mode 0 moves BCR's low half words (0 means 10000h) as one unit in either direction with MADR stepping by 4 or -4 and wrapping at 24 bits; the unit ends `n * rate + ceil(n / 16)` cycles after the start.
- Sync mode 1 moves one block per raised request line, counts blocks down in `remain`, clears the channel's request bit when the block starts, and leaves the channel busy between blocks with no unit, so the CPU is never held while the line is low. The flag and the interrupt follow only the last block.
- `schway__dma_dreq` starts a waiting channel when the engine is idle and otherwise leaves it to the next unit boundary.
- Costs per word are 1 (MDEC, GPU, OTC), 24 (CD-ROM), 4 (SPU), 20 (PIO) plus one cycle per 16 words for the RAM row; OTC now uses the same cost function, and its 4,352 cycles for 4,096 words are unchanged.
- Mutation checks of `src/dma.c` (sync 1 ignoring the request line, a block that keeps the request bit, the CD-ROM at 40, a tie broken the other way, no block counting, no backward step) each fail at least one `dma.*` test.

## Task Commits

1. **Task 1: Tracer - a sync 0 burst from RAM to the GPU channel reaches the stub sink, which counts and checksums it** - `52dc8e9` (feat)
2. **Task 2: Sync 1 with DREQ, both directions, the per-device costs and re-arbitration** - `ef9860f` (feat)
3. **Task 3: The DMA page's block modes, device ends and costs** - `bc437ad` (docs)

**Plan metadata:** the docs commit that carries this summary, STATE.md and ROADMAP.md.

The tracer gate ran in the default end-of-phase mode with an automated-only verify: `cmake --workflow --preset ci` and `ctest -L '^dma$'` were re-run, passed, and the expansion task went ahead.

## Files Created/Modified

- `src/dma.c` - the device-end switches and stand-ins, the cost function, the burst and block units, the block finish, the request-line start, the pending rule for sync mode 1
- `src/dma.h` - the comment on the state fields that the new modes use
- `tests/dma/dma_tests.c` - six cases and the pattern helpers; the arbitration case holds the engine with a fake running unit
- `tests/dma/tests.cmake`, `tests/dma/inventory.txt` - six registrations, 22 in total
- `docs/hardware/dma.md` - three new sections and the edited state, arbitration and not-built text

## Decisions Made

See `key-decisions` above. The stand-in's word count and CRC-32 sit in the existing `dma.words` and `dma.crc` arrays of the DMA state, already saved by the state walk (the plan left the place to this plan).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug in an earlier test, exposed by this change] The arbitration test relied on channels 0-5 never starting**
- **Found during:** Task 1
- **Issue:** `dma.dpcr.priority` wrote CHCR for two sync 0 channels and then moved DPCR; with units for those channels a DPCR write now starts them, so the test would have measured the engine, not the arbitration.
- **Fix:** the test starts with a fake running unit (the helper the DICR cases already use), so no write can start a channel and every pick is the arbitration alone; every assertion is unchanged.
- **Files modified:** tests/dma/dma_tests.c
- **Verification:** `ctest -R '^dma\.dpcr\.priority$'` exit 0; a mutant that breaks ties the other way fails it.
- **Committed in:** 52dc8e9

**2. [Rule 2 - Missing critical bound] The state walk accepted any block count**
- **Found during:** Task 2
- **Issue:** `remain` was bounded at 32 bits; now that it holds a block count, a hostile state could name billions of blocks.
- **Fix:** bound `remain` at 10000h in the state walk, the most blocks a transfer can have.
- **Files modified:** src/dma.c
- **Verification:** `cmake --workflow --preset ci` exit 0, including the state and `core.state.*` cases.
- **Committed in:** ef9860f

**3. [Rule 3 - Blocking] The block request is consumed by the block**
- **Found during:** Task 2
- **Issue:** the plan says one block moves per `schway__dma_dreq(inst, ch, 1)` and the line is a latched level; with the level left high every block would move on one raise.
- **Fix:** starting a block clears the channel's request bit and the device raises it again; the page says so. A device that keeps the line high by calling `schway__dma_dreq` again after each block still gets back-to-back blocks.
- **Files modified:** src/dma.c, docs/hardware/dma.md
- **Verification:** `dma.sync1.dreq_blocks` exit 0; the mutant that keeps the bit fails it.
- **Committed in:** ef9860f

---

**Total deviations:** 3 auto-fixed (1 test bug exposed by the change, 1 missing bound, 1 blocking interpretation)
**Impact on plan:** none on the plan's tests, names or files.

## Issues Encountered

- The plan's "17/16 RAM row" for rates above 1 is read as one extra cycle per 16 words added to the device rate (`n * rate + ceil(n / 16)`), which equals the documented `ceil(17 * n / 16)` at rate 1 and matches the engine's own 2,691,072-cycle bound for the CD-ROM at 40; the documented table (psx-spx) lists 24 and 40 without the row, so the row's addition at those rates is the model's choice, stated on the page.
- The first draft of two expected values in my own tests (MADR after a backward burst, the DICR value with the SPU flag) were wrong by one word and one flag bit; the code was right and the tests were corrected.

## Not runnable locally

`asan`, `be32` and the `wasm32` lanes need clang-21 or wasmtime that this host lacks; hosted CI runs them. `cmake --workflow --preset ci` passes locally (304 tests).

## Known Stubs

The device ends are stand-ins by design: the sink counts and checksums words and the source reads 0 for channels 0-5. Each is replaced by the device's own case when its phase builds the device (GPU phase 4, SPU and MDEC phase 5, CD-ROM phase 6, PIO unbuilt). The CD-ROM's 40-cycle cost is a recorded deferral: the phase-6 device end selects 40 from the delay setting. Channels written with the linked-list or reserved mode stay busy until plans 03-14 and 03-15.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Ready for 03-14 (chopping and linked lists): `dma_pending` returns 0 for sync 2 and 3 on non-OTC channels, the unit start has one branch per mode, and `remain` and `nodes_done` are bounded in the state walk.
- Ready for 03-15 (the sync-mode fault): mode 3 on a non-OTC channel is the case that does nothing today.

---
*Phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers*
*Completed: 2026-11-10*

## Self-Check: PASSED
