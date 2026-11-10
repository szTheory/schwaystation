---
phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
plan: 04
subsystem: core
tags:
  - bus
  - dma
  - timers
  - irq
  - fixtures

requires:
  - phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
    provides: the dma and timers stubs and declarations (plan 01), schway__sched_now with the always-on block clock and schway__sched_recalc (plan 03)
provides:
  - schway__bus_ram_word, the DMA engine's fault-free, cost-free word path into RAM through the RAM window
  - the DMA register range 1F801080h-10FFh and the timer range 1F801100h-112Fh owned, with no unbuilt count and no fault
  - a call to schway__dma_bus_access on every bus read and instruction fetch that reaches RAM, the I/O page, EXP1-3 or the BIOS, never on a store, the scratchpad or the cache-control word
  - an I_STAT read that syncs the timers at its own tick and an I_STAT write that recomputes the deadline
  - an unbuilt.exe fixture and two docs pages that no longer name DMA or the timers as unbuilt
affects:
  - 03-08
  - 03-09
  - phase-3 plans that fill the DMA engine and the root counters

actuals:
  tokens: 5100
  tasks: 3
  commits: 3

plan_head_before: 9b9137ff96cde0b41bb466493d236c2c0086df2e
plan_head_after: 132b55a4c68acdc157db5ae439046ec21610b162
commits: 3

tech-stack:
  added: []
  patterns:
    - "a device's bus hook is called from bus_access with the tick of the access (schway__sched_now with the cycles so far), before the access happens, for reads and fetches only"
    - "a device range is owned by its module even while the module is a stub: it reads 0 and drops writes with no unbuilt count, so the sweep classifies it as owned"

key-files:
  created: []
  modified:
    - src/bus.c
    - src/bus.h
    - src/irq.c
    - src/dma.c
    - src/timers.c
    - tests/bus/bus_tests.c
    - tests/bus/tests.cmake
    - tests/bus/inventory.txt
    - tests/core/bus_unbuilt.c
    - tests/fixtures/src/unbuilt.asm
    - tests/fixtures/tests.cmake
    - docs/hardware/bus.md
    - docs/runner.md

key-decisions:
  - "schway__bus_ram_word ignores the low two address bits, as MADR's are, and returns 1 with nothing moved when ram_locate maps no RAM; it takes no cycles pointer, so a charge is impossible by construction"
  - "the stall hook is called once per bus_access, before the switch, for K_READ and K_FETCH on every class except the scratchpad and a gap; the KSEG2 cache-control word returns earlier and never reaches it"
  - "the I_STAT sync applies to I_STAT only (offset 0 of the pair), not I_MASK: a mask read does not show a timer edge"
  - "unbuilt.exe MODE 0 writes the SPU main volume then the GPU GP0, so the footer line reads unbuilt dev=gpu n=1 and the docs example matches the fixture; the footer regex accepts gpu or spu"

patterns-established:
  - "an owned-range stub drops the unused-parameter warnings with (void) casts and says in its comment that the access is neither counted as unbuilt nor a fault"

requirements-completed: []

coverage:
  - id: D1
    description: "schway__bus_ram_word moves one aligned word to or from RAM through the RAM window with no fault, cost, unbuilt count or stall; a word is readable at all four mirrors through the bus, an address past the banks drops the word and returns non-zero, and DICR is untouched"
    requirement: "GTE-02"
    verification:
      - kind: unit
        ref: "tests/bus/bus_tests.c#ram_word (bus.ram_word)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The DMA and timer ranges are owned: every address at widths 1, 2 and 4 answers with no unbuilt count and no fault, and the sweep classifies them as owned with the DMA and timer state reset before each access (owned=672 unbuilt=3228 gap=8388)"
    requirement: "GTE-02"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^(core\\.bus_unbuilt|bus\\.)' --no-tests=error --output-on-failure (exit 0, 18 tests)"
        status: pass
    human_judgment: false
  - id: D3
    description: "bus_access calls the DMA stall hook for reads and instruction fetches at the tick of the access and skips stores, the scratchpad and the cache-control word; an I_STAT read syncs the timers and an I_STAT write recomputes the deadline"
    requirement: "GTE-03"
    verification:
      - kind: other
        ref: "grep -q 'schway__dma_bus_access' src/bus.c && grep -q 'schway__timers_sync' src/bus.c && grep -q 'schway__sched_recalc' src/irq.c (exit 0); the hooks are stubs until the DMA and timer plans, whose tests assert the behaviour end to end"
        status: pass
    human_judgment: false
  - id: D4
    description: "unbuilt.exe no longer touches DMA, fixtures.unbuilt_footer matches the GPU or SPU line, and docs/runner.md and docs/hardware/bus.md no longer list DMA or the timers as unbuilt"
    requirement: "GTE-02"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^fixtures\\.(unbuilt_footer|gap_write)$|^docs\\.' --no-tests=error --output-on-failure (exit 0)"
        status: pass
      - kind: other
        ref: "grep -qi 'DMA control\\|0xBF8010F0' tests/fixtures/src/unbuilt.asm (exit 1, no match); grep -q 'dev=dma' docs/runner.md (exit 1, no match)"
        status: pass
    human_judgment: false

duration: 3min
completed: 2026-11-10
status: complete
---

# Phase 3 Plan 04: The bus for DMA and the timers Summary

**The bus owns the DMA and timer ranges, gives the DMA engine a fault-free RAM word path (`schway__bus_ram_word`), reports every read and instruction fetch to the DMA stall hook at the access's own tick, and keeps the timers current on I_STAT reads and writes, with the unbuilt fixture and two pages moved off DMA.**

## Performance

- **Duration:** 3 min
- **Started:** 2026-11-10T05:13:30Z
- **Completed:** 2026-11-10T05:16:49Z
- **Tasks:** 3
- **Files modified:** 13 outside `.planning`

## Accomplishments

- `schway__bus_ram_word` sits on `ram_locate` and assembles the word a byte at a time, little-endian. The `bus.ram_word` case writes through it and reads back through `schway__bus_read` at all six KUSEG/KSEG0/KSEG1 mirror addresses, checks the byte order, the last word of the 8 MiB window, the dropped word past the window and past a 2 MiB bank, and that no fault, unbuilt count, CPU cycle or DICR bit changed.
- `schway__dma_io` and `schway__timers_io` stop calling `schway__bus_unbuilt`; the sweep flips both ranges to owned and resets both devices before each swept access. **The sweep's three counts are `owned=672 unbuilt=3228 gap=8388`** (per direction the sum is 3 widths x 4,096 offsets = 12,288, as the test asserts; reads and writes give equal counts).
- `bus_access` classifies once, and for `K_READ` and `K_FETCH` on RAM, the I/O page, EXP1-3 and the BIOS calls `schway__dma_bus_access(inst, schway__sched_now(inst, *cycles))` before the access. A store, the scratchpad, a gap and the KSEG2 cache-control word never call it; the fifth queued store is plan 03-03's call in `cpu_wq_push`.
- An I_STAT read calls `schway__timers_sync` at the access's tick; an I_STAT write calls `schway__sched_recalc` after the register changes.
- `unbuilt.exe` MODE 0 writes the SPU main volume left and the GPU's GP0. The CI-built image runs to exit 0 with `unbuilt dev=gpu n=1 last_pc=0x80010120`, which is also the example in `docs/runner.md`.
- `docs/hardware/bus.md` moves DMA and the timers out of the unbuilt list and describes the RAM word path, the hook and the I_STAT behaviour; `docs/runner.md` names devices that are still unbuilt.

## Task Commits

1. **Task 1: Tracer - the DMA RAM word path, end to end through the bus with its test** - `4660c91` (feat)
2. **Task 2: Owned DMA and timer ranges, the stall hook call and the I_STAT timer sync, with the sweep** - `6d3b9ab` (feat)
3. **Task 3: The unbuilt fixture without DMA, and the two pages** - `132b55a` (test)

**Plan metadata:** the docs commit that carries this summary, STATE.md and ROADMAP.md.

The tracer gate ran in the default end-of-phase mode with an automated-only verify: `cmake --workflow --preset ci` and `ctest -R '^bus\.ram_word$'` were re-run after Task 1, passed, and the expansion tasks went ahead.

## Files Created/Modified

- `src/bus.c`, `src/bus.h` - `schway__bus_ram_word`, the hook call, the I_STAT timer sync
- `src/irq.c` - `schway__sched_recalc` after an I_STAT write
- `src/dma.c`, `src/timers.c` - owned-range stubs that read 0 and drop writes without counting
- `tests/bus/bus_tests.c`, `tests/bus/tests.cmake`, `tests/bus/inventory.txt` - `bus.ram_word`
- `tests/core/bus_unbuilt.c` - DMA and timer ranges owned, devices reset before each access
- `tests/fixtures/src/unbuilt.asm`, `tests/fixtures/tests.cmake` - SPU and GPU writes, footer regex
- `docs/hardware/bus.md`, `docs/runner.md` - the unbuilt lists and the new bus behaviour

## Decisions Made

See `key-decisions`. Besides those, the plan's acceptance grep `grep -q 'K_WRITE' src/bus.c && grep -q 'schway__dma_bus_access(inst' src/bus.c` passes, and the "no call for a write" guard is `kind != K_WRITE` on one line above the call.

## Deviations from Plan

None - plan executed exactly as written.

The plan said the hook's tick precision and the stall itself are tested by plans 03-08 and 03-09, so this plan asserts only that the calls exist and that the bus still behaves; the hook, `schway__timers_sync` and `schway__sched_recalc` are still stubs or idempotent at this point, so no test here can observe them directly.

## Issues Encountered

None. One imprecision is recorded for plan 03-08, which owns the hook's body: two internal callers reach `bus_access` with a throwaway cycle sink and are not separate bus accesses, namely the interrupt entry's read of the word at EPC (`src/cpu_exec.inc`, only with no trace log) and the second to fourth words of an I-cache line refill. Both now call the hook. For the refill the first word's call has already set any stall, so the extra calls are harmless; for the EPC read, 03-08 should decide whether a DMA unit running at an interrupt entry may set a stall for a read the CPU never makes (the words are read for the COP2 double-execute hook).

## Known Stubs

- `src/dma.c` `schway__dma_io`, `schway__dma_next`, `schway__dma_event`, `schway__dma_bus_access` and `src/timers.c` `schway__timers_io`, `_next`, `_event`, `_sync`, `_vblank`, `_hblank`, `_dots`: registers read 0, writes are dropped, no deadline and no event. Intentional: the DMA and timers plans of this phase replace the bodies; the ranges are owned now so those plans never edit `bus.c`, `irq.c`, `tests/core/bus_unbuilt.c` or the `unbuilt` fixture. The ledger file `.planning/WINDOWS.md` does not exist, so nothing was appended.

## Verification Not Runnable Here

The asan and be32 lanes need clang-21, which this host lacks; hosted CI runs them. The new `bus.ram_word` allocates its 2 MiB RAM snapshot on the heap, not the stack, so the wasm32 and asan lanes are not stack-limited.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Plans 03-08 and 03-09 can fill the DMA engine and the timers without touching the bus, the interrupt registers, the sweep or the `unbuilt` fixture. The `unbuilt.exe` image digest changed (it is compared lane against lane in `hashes.txt`, not pinned). Evidence: `cmake --workflow --preset ci` exit 0, 249 tests; `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake` exit 0.

## Self-Check: PASSED

- `src/bus.c`, `src/bus.h`, `src/irq.c`, `src/dma.c`, `src/timers.c`, `tests/bus/bus_tests.c`, `tests/core/bus_unbuilt.c`, `tests/fixtures/src/unbuilt.asm`, `docs/hardware/bus.md`, `docs/runner.md` all exist.
- Commits `4660c91`, `6d3b9ab` and `132b55a` are ancestors of HEAD; `git rev-list --count 9b9137f..HEAD` is 3.

---
*Phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers*
*Completed: 2026-11-10*
