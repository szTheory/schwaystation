---
phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
fixed_at: 2026-11-10T12:00:00Z
review_path: .planning/phases/03-the-gte-matches-the-hardware-cases-dma-and-timers/03-REVIEW.md
iteration: 1
findings_in_scope: 2
fixed: 2
skipped: 0
status: all_fixed
---

# Phase 3: Code Review Fix Report

**Fixed at:** 2026-11-10
**Source review:** .planning/phases/03-the-gte-matches-the-hardware-cases-dma-and-timers/03-REVIEW.md
**Iteration:** 1

**Summary:**
- Findings in scope: 2 (CR-01, WR-01; IN-01 to IN-07 are out of scope and become seeds)
- Fixed: 2
- Skipped: 0

Both fixes ran in the main checkout (`workflow.use_worktrees` is `false`), on the phase branch, one commit each.

## Fixed Issues

### CR-01: A DMA register read can leave `busy_until` beyond the state's own bound, so the core's saved state is refused on load

**Files modified:** `src/dma.c`, `src/dma.h`, `docs/hardware/dma.md`, `tests/dma/dma_tests.c`, `tests/dma/inventory.txt`,
`tests/dma/tests.cmake`
**Commit:** 15ae4d4
**Regression test:** `dma.read.chain_saves` (case `read_chain_saves`)
**Status:** fixed: requires human verification (engine ordering logic)

**Applied fix:** The review's second option. The register read still finishes the running unit as of its end tick, so the
program reads the completed state, but it no longer starts the next waiting channel there. The event loop was split into a
static `dma_run` with a `start_next` flag (the read passes 0, `schway__dma_event` passes 1). `schway__dma_next` now returns
the finished unit's end tick when nothing runs and a channel waits, so the normal event starts the next unit at that tick.
The timeline is unchanged (the next unit starts at the same tick it did before and ends at the same tick), only the
engine state between the read and that tick differs, and it is inside the bound. The bound stays at one unit: doubling it
would not close the fault, because a loaded state already at the doubled bound would chain a third unit on the next read.
`docs/hardware/dma.md` and the `dma.h` comments say so.

The test arms channels 3 and 5 at `BCR` 0 and `CHCR` 11000000h, reads a register while the first runs, and checks that the
read sees the first unit complete, that `busy_until` stays within `SCHWAY__DMA_BUSY_MAX`, that the next deadline is the
first unit's end, that save, load and save gives identical bytes (before and after the first unit ends), and that both
transfers finish with their flags. On the unfixed code it fails (the load returns `SCHWAY_STATUS_ERR_STATE_FIELD`, 10, and
the saved bytes differ); it passes with the fix.

### WR-01: A DMA stall does not advance `cpu.cycle`, so GTE and multiply stamps do not decay during it

**Files modified:** `src/cpu.c`, `docs/hardware/cpu.md`, `tests/dma/dma_tests.c`, `tests/dma/inventory.txt`,
`tests/dma/tests.cmake`
**Commit:** 9c3aac8
**Regression test:** `dma.stall.engines_decay` (case `stall_decays_engines`)
**Status:** fixed: requires human verification (timing model)

**Applied fix:** Not the review's literal `inst->cpu.cycle += wait;`. That change was built and run first, and it broke
three existing phase 3 scheduler tests (`core.sched.stall_freeze`, `core.sched.stall_carries`, `core.sched.stall_frame_end`),
which pin that the CPU cycle count does not move during a freeze (as `docs/hardware/cpu.md` states), and it changed the
saved-state hash of the `dma_chain` fixture. The instruction was to report rather than edit an expectation, so that
change was reverted before any commit. The equivalent that keeps the documented contract: the stall branch of
`schway__cpu_run` moves `cpu.muldiv_ready` and `gte.ready` back by the cycles the freeze holds, saturating at 0 (0 is
"no stamp"). A read of the result then waits `ready - (cycle + k)` exactly as it would have with the cycle advanced. Both
stamps stay inside the bounds a state load enforces (`gte.ready <= cycle + 44` only gets looser), and a freeze cut into
runs of any size subtracts the same total, so split equivalence holds (`core.run.split_devices` and `core.sched.*` pass).

The test runs a program that enables the GTE, issues NCDT (or a DIV), starts OTC, loads from the I/O page (which freezes
the CPU for the 4,352-cycle unit) and then runs MFC2 (or MFLO). It measures that read's cost in ticks and requires it to
equal the cost of the same read after a nop in place of the command (1 cycle). Without the fix the costs are 32 and 24;
with it, 1 and 1.

## Verification

All run from the repository root, in the main checkout (no worktree), after both commits:

- `cmake --workflow --preset ci`: exit 0, 337 tests passed.
- `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake`: exit 0 (337 tests ran once each).
- `cmake --workflow --preset hygiene`: exit 0.
- `ctest --test-dir build/ci -L '^dma$' --no-tests=error`: exit 0, 35 tests passed.
- `build/ci/hashes.txt` against a copy saved before the first change: identical, all 37 lines, so every older fixture
  digest and the new device fixtures (`gte_vectors`, `dma_chain`, `timers_lines`) are unchanged.

---

_Fixed: 2026-11-10_
_Fixer: Claude (gsd-code-fixer)_
_Iteration: 1_
