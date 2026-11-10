---
phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
plan: 03
subsystem: core
tags:
  - scheduler
  - block-clock
  - cpu-stall
  - vblank
  - timing

requires:
  - phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
    provides: sched.h with the derived deadline, the dma and timers entry points as stubs, state format 3 with the sched recalculation after a load
provides:
  - an always-on block clock (blk_tick, blk_cycle) so every device access and first fault can name its own tick inside a block
  - the deadline limit, the event runner and the freeze-until stall in the CPU run loop
  - the fifth queued store reaching the DMA bus hook, with the hook still a stub
  - the vertical blank pushed to the timers at the frame end, and due events run on the no-program path
  - tests/core/sched_tests.c with seven cases and the CPU page section on the spine
affects:
  - 03-04
  - 03-06
  - 03-07
  - 03-08
  - phase-3 plans that fill the DMA engine and the root counters

actuals:
  tokens: 8300
  tasks: 3
  commits: 3

plan_head_before: 7e747b98c665b243e0cc08e72709dfdb87618e27
plan_head_after: ca7a21c9bd41e79dccf50eeb49e5bdcc72e6631a
commits: 3

tech-stack:
  added: []
  patterns:
    - "the block clock is two non-state CPU fields set at every block start; a device reads now = blk_tick + (cycle - blk_cycle) + (cycles - 1), and a change of tick or cycle outside a block (power reset, state load, a no-program run, the one-instruction entry) restarts it with schway__sched_sync"
    - "the run loop offers due events once per pass, limits each block to the next deadline, and treats a deadline still due after its event as a one-instruction limit, so a misbehaving device cannot stall the loop"
    - "a stall is a freeze in cpu_run: ticks advance, cpu.cycle does not, within the budget and frame room and up to the next deadline; the remainder stays in the state"

key-files:
  created:
    - tests/core/sched_tests.c
  modified:
    - src/cpu.h
    - src/cpu.c
    - src/cpu_exec.inc
    - src/sched.c
    - src/frame.c
    - src/instance.c
    - src/internal.h
    - src/state.c
    - tests/core/tests.cmake
    - tests/core/inventory.txt
    - docs/hardware/cpu.md

key-decisions:
  - "A deadline that is still due after its event limits the next block to one instruction instead of underflowing next - ticks, so the loop always advances and the event is offered again at the next boundary"
  - "The no-program path moves ticks in steps that stop at the next deadline and runs only events that are already due; with nothing armed it is one compare and the Ir/frame moved by 121"
  - "schway__sched_recalc sets block_end EVENT every time it runs, also at a power reset and a state load; the flag is not state and cpu_block clears it at every block start"

requirements-completed: []

coverage:
  - id: D1
    description: "The block clock is always on: blk_tick and blk_cycle are set at every block start in both executor copies, schway__sched_now returns blk_tick + (cycle - blk_cycle) + (cycles - 1), and the first fault of a long block carries the tick its own instruction began on"
    requirement: "GTE-03"
    verification:
      - kind: unit
        ref: "tests/core/sched_tests.c#fault_ticks (core.sched.fault_ticks: a fault 40 no-ops into a block equals the tick a one-cycle-at-a-time reference gives, and is at least 40 cycles past the block's entry)"
        status: pass
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^core\\.(trace\\.|trace_hash_equal|run\\.)' (exit 0, the trace record cycles did not move)"
        status: pass
    human_judgment: false
  - id: D2
    description: "cpu_run limits each block to the next device deadline and runs the due events at the first instruction boundary at or after it, with the runner recomputing the deadline; a state load recomputes it and restarts the block clock"
    requirement: "GTE-03"
    verification:
      - kind: unit
        ref: "tests/core/sched_tests.c#event_dispatch (core.sched.event_dispatch: a deadline 37 cycles ahead inside 100 straight-line no-ops is run and recomputed in a 200-cycle budget; one 100000 cycles ahead is left alone)"
        status: pass
      - kind: unit
        ref: "tests/core/sched_tests.c#state_next_derived (core.sched.state_next_derived: a loaded state recomputes a deadline set to 1)"
        status: pass
    human_judgment: false
  - id: D3
    description: "A pending DMA stall freezes the CPU: ticks advance within the budget with no instruction run, the rest of a stall past the budget carries in the state to the next call, and the fifth queued store reaches the DMA bus hook while scratchpad stores and the first four stores do not"
    requirement: "GTE-02"
    verification:
      - kind: unit
        ref: "tests/core/sched_tests.c#stall_freeze (core.sched.stall_freeze: 500 stall cycles in a 1000-cycle budget leave cpu.cycle advanced by the other 500, to within one instruction)"
        status: pass
      - kind: unit
        ref: "tests/core/sched_tests.c#stall_carries (core.sched.stall_carries: a 5000-cycle stall uses two 1000-cycle budgets with cpu.cycle and pc unchanged and the stamp kept, then the third call runs the rest)"
        status: pass
      - kind: other
        ref: "awk '/^static void cpu_wq_push/,/^}/' src/cpu_exec.inc | grep -q 'schway__dma_bus_access' (exit 0, baseline at HEAD: exit 1); the end-to-end fifth-store rule is tested by the DMA plan once the hook is filled"
        status: pass
    human_judgment: false
  - id: D4
    description: "A stall that crosses the frame end ends the frame at the frame's end tick with the rest of the stall still in the state; with no program loaded, an event that is already due runs at its own deadline inside a run and no counter is stepped"
    requirement: "GTE-03"
    verification:
      - kind: unit
        ref: "tests/core/sched_tests.c#stall_frame_end (core.sched.stall_frame_end: run_frame returns STOP_FRAME at the frame's end tick, cpu.cycle unchanged, stall stamp kept, and the next frame runs exactly the cycles after the stall)"
        status: pass
      - kind: unit
        ref: "tests/core/sched_tests.c#no_exe_events (core.sched.no_exe_events: a deadline 100 ahead is recomputed and ticks reach 500; one 100000 ahead is left alone)"
        status: pass
    human_judgment: false
  - id: D5
    description: "With a PS-EXE loaded the frame end drives the vertical blank high then low to the timers beside the I_STAT bit 0 raise; the timers' vblank entry point is still an empty stub, so only the call is checked here and the effect is tested by the timers plan"
    requirement: "GTE-03"
    verification:
      - kind: other
        ref: "grep -q 'schway__timers_vblank' src/frame.c (exit 0); cmake --workflow --preset ci (exit 0, 248 tests); every phase-2 fixture and hash test unchanged"
        status: pass
    human_judgment: false
  - id: D6
    description: "docs/hardware/cpu.md has the section Devices, the block clock and the stall (now formula, deadlines, events at the first boundary, the freeze-until stall and why it is not a charge, the vblank push) and no planning identifier"
    requirement: "GTE-03"
    verification:
      - kind: other
        ref: "grep -qE 'D-[0-9]{2}|GTE-0[0-9]|PX\\.[0-9]|DEC\\.[0-9]|XFER-' docs/hardware/cpu.md (exit 1, no match); sh scripts/hygiene.sh (exit 0)"
        status: pass
    human_judgment: false

duration: 11min
completed: 2026-11-10
status: complete
---

# Phase 3 Plan 03: The scheduler spine Summary

**An always-on block clock, a deadline that cuts every CPU block, an event runner, a freeze-until DMA stall and the frame-end vertical blank push, with seven `core.sched` tests and a CPU-page section, and every phase-2 hash and assertion unchanged.**

## Performance

- **Duration:** 11 min (the wait for the hosted CI run included)
- **Started:** 2026-11-10T05:00:15Z
- **Completed:** 2026-11-10T05:11:35Z
- **Tasks:** 3
- **Files modified:** 12 outside `.planning`

## Accomplishments

- `struct schway__cpu` gains non-state `blk_tick` and `blk_cycle`, set at the start of every block in both executor copies; `schway__sched_now` is the formula the plan names and the trace ring uses the same fields (its `t0` and `c0` are gone). A first fault now carries the tick of its own instruction.
- `schway__cpu_run` runs due events at the top of each pass, cuts each block at `min(budget left, sched.next - ticks)` and, while `dma.stall_until` is ahead of the tick, advances the tick without executing, up to the budget, the frame room and the next deadline. The fast path is unchanged: the existing `ran >= limit` compare ends the block at the deadline.
- `cpu_wq_push` takes the instance and calls `schway__dma_bus_access` on its full-queue branch only; the scratchpad return stays first.
- `schway__sched_recalc` and `schway__sched_run` are real: the runner offers each due device one event per call and recomputes the deadline, and `schway__sched_sync` restarts the block clock after a power reset, a state load, a no-program run and the one-instruction entry.
- `advance()` pushes `schway__timers_vblank` high then low at the frame end with a program loaded; with none loaded it steps ticks to the next deadline and runs only events already due.
- Ir/frame of the test card: **1,620,545**, against the phase-1 control of 1,620,424 (+121, +0.0075 %). Measured once on this commit with the hosted CI run `https://github.com/szTheory/schwaystation/actions/runs/42760163838` (started by `gh workflow run ci.yml --ref phase/03-the-gte-matches-the-hardware-cases-dma-and-timers`, exit 0; the run and its perf job concluded success): the perf job's line is `perf control testcard runs=1620545,1620545,1620545 spread=0`. `valgrind` is not on this host, so the local Cachegrind method was not run.

## Task Commits

1. **Task 1: Tracer - the block clock: a fault inside a long block carries its own instruction's tick** - `850c4e1` (feat)
2. **Task 2: The deadline limit, the event runner and the freeze-until stall in cpu_run** - `5a7afd1` (feat)
3. **Task 3: The frame end pushes the vertical blank; the no-EXE path runs due events; the stall crosses the frame end; the CPU page** - `ca7a21c` (feat)

**Plan metadata:** the docs commit that carries this summary, STATE.md and ROADMAP.md.

The tracer gate ran in the default end-of-phase mode with an automated-only verify: `cmake --workflow --preset ci` and the `core.sched.fault_ticks` / trace / run ctest filter were re-run after Task 1, passed, and the expansion tasks went ahead.

## Files Created/Modified

- `src/cpu.h`, `src/internal.h`, `src/cpu.c`, `src/cpu_exec.inc` - the block clock fields, the trace ring without `t0` and `c0`, the run loop, `cpu_wq_push` with the instance and the DMA hook
- `src/sched.c` - `schway__sched_now`, `_sync`, `_recalc` (with the block-end flag) and `_run`
- `src/instance.c`, `src/state.c`, `src/frame.c` - the fault stamp from the block clock, the sync and recalc at power-on and after a load, the vblank push and the no-program event stepping
- `tests/core/sched_tests.c` (new), `tests/core/tests.cmake`, `tests/core/inventory.txt` - target `schwaystation-test-sched` and `core.sched.{fault_ticks,stall_freeze,stall_carries,event_dispatch,state_next_derived,stall_frame_end,no_exe_events}`
- `docs/hardware/cpu.md` - the new section; the first-fault sentence of the run loop section now names the block clock

## Decisions Made

See `key-decisions`. The stale sentence in the run loop section saying the CPU takes no interrupts is left for the docs plan, as the plan says.

## Triage

None. No phase-2 assertion, hash or fixture moved. `core.run.fault_latched` pins `fault.ticks` at 1000 after a no-program run; it still passes unchanged because the no-program path restarts the block clock at the new tick, so the D-24 exception the plan allows was not needed.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] The state-load sync and the no-program sync moved from Tasks 2 and 3 into Task 1**
- **Found during:** Task 1
- **Issue:** with `fault.ticks` read from the block clock, `core.run.fault_latched` (a fault latched after a no-program run of 1000 cycles) and any fault latched after a state load would have read a stale clock, and the plan assigned those two syncs to later tasks.
- **Fix:** `schway__sched_sync` is called after APPLY in `src/state.c`, after the tick moves in the no-program path of `advance()`, in the power-on reset, and at the start of `schway__cpu_step_one`. The later tasks then built on them.
- **Files modified:** src/state.c, src/frame.c, src/instance.c, src/cpu.c
- **Verification:** `core.run.fault_latched` and every phase-2 test passed unchanged in the Task 1 ci run.
- **Committed in:** 850c4e1

**2. [Rule 2 - Missing Critical] A deadline still due after its event could underflow the block limit**
- **Found during:** Task 2
- **Issue:** `sched.next - ticks` wraps to a huge limit when a device's event leaves its deadline at or before the tick, which would let a block run unbounded by the deadline instead of re-offering the event.
- **Fix:** such a deadline limits the next block to one instruction; the stall step ignores a due deadline. The loop therefore advances every pass.
- **Files modified:** src/cpu.c
- **Verification:** compiles into the ci run; no stub device leaves a due deadline, so this is not yet reachable by a test (the timers plan's zero-progress guard covers the device side).
- **Committed in:** 5a7afd1

---

**Total deviations:** 2 auto-fixed (1 blocking, 1 missing critical)
**Impact on plan:** both are inside the files the plan names; no scope creep.

## Issues Encountered

- The `asan` and `be32` lanes need `clang-21`, which this host lacks, so they were not run locally; the hosted run `42760163838` ran them, with `asan`, `be32`, `wasm32`, `nofp` and `hash-equality` all successful.
- The first `core.sched.fault_ticks` draft assumed the program starts at the first cycle; the stub kernel boots first (about 5,000 cycles), so the case now notes the entry tick from the one-cycle-at-a-time reference and runs a 20,000-cycle budget.
- Mutations run by hand: stamping the fault with `inst->ticks` fails `fault_ticks`; dropping the deadline from the block limit fails `event_dispatch`; not counting the stall in `ran` fails `stall_freeze`.
- The broken-windows ledger `.planning/WINDOWS.md` does not exist, and this plan leaves no stub, so nothing was appended.
- The vertical blank call has no observable effect until the timers plan fills `schway__timers_vblank`; this plan checks the call by `grep` and the timers plan tests the effect.

## Known Stubs

None added. The DMA bus hook (`schway__dma_bus_access`) and the timer entry points are still the 03-01 stubs; the DMA and timers plans fill them.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Plan 03-04 can hook reads and fetches on the bus side; 03-06 takes the cop2 dispatch and the interrupt hook fix; the DMA plan fills `schway__dma_bus_access` and tests the fifth-store rule end to end; the timers plan tests the vertical blank push. The branch is pushed to `origin` and carries no pull request.

## Self-Check: PASSED

- Created file exists: `tests/core/sched_tests.c`.
- Commits `850c4e1`, `5a7afd1` and `ca7a21c` are ancestors of HEAD; the plan measured 3 commits from the recorded base `7e747b9`.
- Every task's acceptance commands exited as the plan says; `cmake --workflow --preset ci` (248 tests) and the ci JUnit check (248 tests ran once each) pass; `sh scripts/hygiene.sh` exits 0.

---
*Phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers*
*Completed: 2026-11-10*
