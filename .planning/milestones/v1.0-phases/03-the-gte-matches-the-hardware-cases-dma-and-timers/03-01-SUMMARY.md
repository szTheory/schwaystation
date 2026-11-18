---
phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
plan: 01
subsystem: core
tags:
  - gte
  - dma
  - timers
  - scheduler
  - state-format
  - skeleton

requires:
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: the state walk and its field helpers, the bus I/O page with unbuilt-device counting, the CPU cycle counter and the multiply stamp convention
provides:
  - four object libraries (gte, dma, timers, sched) with headers declaring every cross-module entry point later plans implement
  - struct schway embedding gte, dma and timers in the state part and a derived, never-saved sched deadline
  - state format 3 and behaviour revision 3, raised once, with the gte, dma and timers fields walked and bounded
  - thin bus dispatch of DMA and timer registers to their modules, behaviour unchanged
  - empty tests/gte, tests/dma and tests/timers directories selected on ci, asan and wasm32
affects:
  - 03-02
  - 03-03
  - phase-3 plans that implement the GTE, the DMA engine, the root counters and the scheduler

actuals:
  tokens: 10400
  tasks: 3
  commits: 3

plan_head_before: 02f6574afd337119435e8a81c700a75008af2417
plan_head_after: 693df674351d40eb781a3912a70d420249a9ae41
commits: 3

tech-stack:
  added: []
  patterns:
    - "one object library per module, listed in the static library and in the core-options loop, and in the embed test's exact target list"
    - "a clock stamp in the state is walked with the saturating sum of the loaded clock and its own maximum, so the recorded maximum is the bound the rejection sweep probes at max + 1"
    - "a stored-bit mask is a cross-field check beside a full-range field, since the field helpers bound only by range"

key-files:
  created:
    - src/gte.h
    - src/gte.c
    - src/dma.h
    - src/dma.c
    - src/timers.h
    - src/timers.c
    - src/sched.h
    - src/sched.c
    - tests/gte/tests.cmake
    - tests/gte/inventory.txt
    - tests/dma/tests.cmake
    - tests/dma/inventory.txt
    - tests/timers/tests.cmake
    - tests/timers/inventory.txt
  modified:
    - CMakeLists.txt
    - src/internal.h
    - src/state.c
    - src/instance.c
    - src/bus.c
    - src/cpu.c
    - src/cpu.h
    - tests/core/test_state.c
    - tests/core/test_run.c
    - tests/core/tests.cmake
    - tests/core/inventory.txt
    - tests/embed/CMakeLists.txt
    - tests/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/asan.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "SCHWAY__DMA_BUSY_MAX is 2,691,328 cycles: 65,536 words at the CD's 40 cycles per word, plus 17 cycles per 16 words on the RAM side (2,691,072), plus 256 cycles of per-unit overhead (a chopping window's setup and the longest CPU window of 128 cycles)"
  - "Field names chosen: gte {ready, d, c}; dma {busy_until, stall_until, madr, bcr, chcr, dpcr, dicr, cur, phase, remain, nodes_done, dreq, words, crc}; timer {value, target, mode, hold, reached, irq_line, oneshot_done, paused, free_after}; timers {t, base}; sched {next}; state names are dotted (gte.ready, dma.chcr, timers.value)"
  - "CHCR is walked one channel at a time (seven records named dma.chcr) so the loaded values are at hand for the stored-bit mask and the current-channel start-bit check; the other DMA arrays are single array fields"
  - "Stored-bit masks: CHCR 71770703h, DICR 7FFF803Fh (bit 31 derived, not stored), MADR 24 bits; phase 0-7, nodes_done at most 1000h, dreq at most 7Fh"
  - "The timers' base tick is set to the loaded ticks after APPLY and is never walked; the timers' irq_line resets to 0 as the plan says, and the timers plan decides how mode bit 10 reads from it"
  - "An opcode outside the GTE table returns 5 cycles as a no-op placeholder, the shortest documented time, with a comment saying it is not a hardware claim"

patterns-established:
  - "Plan 1 of a phase wires every new module through CMake, the instance, the state walk and the test inventories, so later plans touch only their own module and test directory"

requirements-completed: []

coverage:
  - id: D1
    description: "The GTE module exists with its register file, busy stamp and 22 documented command cycle counts, is embedded in the instance, reset at power-on, and saved and reloaded in the state with the ready stamp clamped to the walked cycle count; state format and behaviour revision are 3"
    requirement: "GTE-01"
    verification:
      - kind: unit
        ref: "tests/core/test_state.c#state.cpu_fields (core.state.cpu_fields pins format 3 and revision 3)"
        status: pass
      - kind: unit
        ref: "tests/core/test_state.c#state.record_sweep (core.state.record_sweep: gte.ready records a maximum below UINT64_MAX and max + 1 is refused)"
        status: pass
      - kind: other
        ref: "cmake --workflow --preset ci && ctest --test-dir build/ci -R '^core\\.state\\.' --no-tests=error --output-on-failure (exit 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: "DMA and timers have their state structs, reset values and bounded state walks: DMA stamps bounded by the loaded clock plus the busy maximum, MADR within 24 bits, CHCR and DICR holding only stored bits, a current channel at most 7 and started; timers serialize the normalized view with no base tick"
    requirement: "GTE-02"
    verification:
      - kind: unit
        ref: "tests/core/test_state.c#state.device_fields (core.state.device_fields: round trip, base tick not in the bytes, unstored CHCR and DICR bits refused, an unstarted current channel refused, a started one loads)"
        status: pass
      - kind: unit
        ref: "tests/core/test_state.c#state.record_sweep (core.state.record_sweep: dma.busy_until and dma.stall_until record maxima below UINT64_MAX)"
        status: pass
    human_judgment: false
  - id: D3
    description: "The bus dispatches 1F801080h-10FFh to the DMA module and 1F801100h-112Fh to the timers module through thin cases; both still count the access as unbuilt, so every phase-2 test, hash and footer is unchanged"
    requirement: "GTE-01"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R 'core.bus_unbuilt|fixtures.unbuilt_footer' (exit 0, both pass unchanged)"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci then cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake (exit 0, 241 tests ran once each)"
        status: pass
    human_judgment: false
  - id: D4
    description: "tests/gte, tests/dma and tests/timers each have an empty inventory and a tests.cmake, included from tests/tests.cmake and selected by the ci, asan and wasm32 inventories, and not by nofp, hygiene or nightly"
    requirement: "GTE-03"
    verification:
      - kind: other
        ref: "for d in gte dma timers; do test -f tests/$d/inventory.txt && test -f tests/$d/tests.cmake && grep -qx $d tests/inventory/ci.txt && grep -qx $d tests/inventory/asan.txt && grep -qx $d tests/inventory/wasm32.txt || exit 1; done (exit 0, baseline at HEAD: exit 1)"
        status: pass
      - kind: other
        ref: "grep -qx gte tests/inventory/nofp.txt returns 1, as required"
        status: pass
    human_judgment: false
  - id: D5
    description: "The embedding test's exact target list names the four new object libraries, so an embedder still gets the library and the object libraries only"
    requirement: "GTE-01"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^embed\\.' --no-tests=error (exit 0 inside the 241-test ci run)"
        status: pass
    human_judgment: false

duration: 25min
completed: 2026-11-10
status: complete
---

# Phase 3 Plan 01: The phase-3 skeleton Summary

**Four core modules (gte, dma, timers, sched) wired through CMake, the instance and a bounded little-endian state walk at format 3, with thin bus dispatch that leaves every phase-2 result unchanged, and three empty test directories on the ci, asan and wasm32 lanes.**

## Performance

- **Duration:** about 25 min (the start time was not captured; estimated from the commit times)
- **Completed:** 2026-11-10
- **Tasks:** 3
- **Files modified:** 30 outside `.planning`

## Accomplishments

- GTE state (`ready`, `d[32]`, `c[32]`) saved and reloaded right after the CPU; `schway__cpu_state_io` now returns the cycle value it read, and `gte.ready` is walked with maximum `cycle + 44`, saturating, so the rejection sweep probes it at max + 1.
- DMA struct of arrays and the timers' normalized view walked after the bus and irq fields, with the plan's bounds; the derived scheduler deadline is never saved and is recomputed after a power reset and a load.
- `SCHWAY__STATE_FORMAT` and `SCHWAY__BEHAVIOUR_REVISION` raised to 3 once; the version pins in `core.state.cpu_fields` follow.
- The bus hands DMA and timer registers to `schway__dma_io` and `schway__timers_io`, which still count the access as unbuilt for their device, so `core.bus_unbuilt` and `fixtures.unbuilt_footer` pass unchanged.
- `tests/gte`, `tests/dma` and `tests/timers` registered on ci, asan and wasm32.

## Task Commits

1. **Task 1: Tracer - the GTE module through the instance to a saved and reloaded state** - `713cbd8` (feat)
2. **Task 2: DMA, timers and the scheduler spine declared, embedded, reset and walked; thin bus dispatch** - `20bc8fd` (feat)
3. **Task 3: The three test directories, included and selected** - `693df67` (test)

**Plan metadata:** the docs commit that carries this summary, STATE.md and ROADMAP.md.

The tracer gate ran in the default end-of-phase mode with an automated-only verify: `cmake --workflow --preset ci` and `ctest -R '^core\.state\.'` were re-run, passed, and the expansion tasks went ahead.

## Files Created/Modified

- `src/gte.h`, `src/gte.c` - GTE struct, reset, state walk, raw register access, and the 22 documented command cycle counts (no arithmetic yet)
- `src/dma.h`, `src/dma.c` - DMA struct of arrays, constants and masks, reset to DPCR 07654321h, bounded state walk, unbuilt register window, latched request levels
- `src/timers.h`, `src/timers.c` - the counter struct, reset, nine-field walk per counter, unbuilt register window, empty event entry points
- `src/sched.h`, `src/sched.c` - the cached deadline and the minimum over the device deadlines
- `src/internal.h`, `src/state.c`, `src/instance.c`, `src/bus.c`, `src/cpu.c`, `src/cpu.h` - instance members, version bump, walks, resets, dispatch, the cycle return
- `CMakeLists.txt` - four object libraries in the static library and the core-options loop
- `tests/core/test_state.c`, `tests/core/test_run.c`, `tests/core/tests.cmake`, `tests/core/inventory.txt` - version pins, stamp bound checks, the new `core.state.device_fields` case, a larger field cap
- `tests/embed/CMakeLists.txt` - the exact target list
- `tests/{gte,dma,timers}/` and the three inventory selections - the empty directories

## Decisions Made

See `key-decisions` above. `SCHWAY__DMA_BUSY_MAX` is 2,691,328 cycles (65,536 x 40 + 65,536 x 17 / 16 + 256).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] The state tests capped the field list at 64 and the plan adds about 49 fields**
- **Found during:** Task 1 verification (the baseline had 62 recorded fields)
- **Issue:** `MAX_FIELDS` was 64 in `tests/core/test_state.c` and in `tests/core/test_run.c`; `core.run.budget_resume` failed with `CHECK(nf <= MAX_FIELDS)` once the GTE fields landed.
- **Fix:** both caps raised to 128, which holds the plan's full set (about 111 records) with room for the later plans.
- **Files modified:** tests/core/test_state.c, tests/core/test_run.c
- **Verification:** `cmake --workflow --preset ci` passes, 241 of 241 tests.
- **Committed in:** 713cbd8

**2. [Rule 3 - Blocking] The embed test lists the exact object-library targets**
- **Found during:** Task 1 (a grep for the other places that name `schwaystation_bus`)
- **Issue:** `tests/embed/CMakeLists.txt` compares the sorted target list with a literal string; adding the four object libraries would have failed the embed test.
- **Fix:** the literal now names gte in Task 1 and dma, sched and timers in Task 2.
- **Files modified:** tests/embed/CMakeLists.txt
- **Verification:** the embed tests pass inside the ci run.
- **Committed in:** 713cbd8 and 20bc8fd

**3. [Rule 2 - Missing Critical] No test exercised the new refusals**
- **Found during:** Task 2
- **Issue:** the plan's VALIDATE rules for CHCR and DICR stored bits and for a started current channel are cross-field checks the max + 1 sweep does not reach (the fields record full range), so nothing would have caught their removal.
- **Fix:** a new case `core.state.device_fields` (registered in `tests/core/tests.cmake` and `tests/core/inventory.txt`) round-trips the device fields, shows the timers' base tick is not in the bytes, refuses each unstored bit and an unstarted current channel with the instance unchanged, and loads the started channel.
- **Files modified:** tests/core/test_state.c, tests/core/tests.cmake, tests/core/inventory.txt
- **Verification:** `ctest --test-dir build/ci -R core.state.device_fields` passes; the ci JUnit check counts 241 tests once each.
- **Committed in:** 20bc8fd

---

**Total deviations:** 3 auto-fixed (2 blocking, 1 missing critical)
**Impact on plan:** all three are test-side or inventory edits the plan's own acceptance runs required; no scope creep in `src/`.

## Issues Encountered

- The `asan` and `be32` workflow presets need compilers this host does not have (`clang-21` is not on the path), so those lanes were not run locally; the hosted CI runs them. The new code uses only fixed-width integers, no floating point and no signed shifts.
- The broken-windows ledger `.planning/WINDOWS.md` does not exist, so the stubs below were not appended to it.

## Known Stubs

These are intentional skeleton stubs; later plans of this phase replace each one.

| File | Stub | Resolved by |
|------|------|-------------|
| src/gte.c | `schway__gte_exec` returns the documented cycle count and computes nothing; `schway__gte_read` and `schway__gte_write` store and return the raw word | the GTE plans of this phase |
| src/dma.c | `schway__dma_io` counts the access as unbuilt and reads 0; `_next` returns UINT64_MAX; `_event` and `_bus_access` do nothing | the DMA plan of this phase |
| src/timers.c | `schway__timers_io` counts the access as unbuilt and reads 0; `_next` returns UINT64_MAX; the event, sync and signal entry points do nothing | the timers plan of this phase |
| src/sched.c | `schway__sched_now` returns the instance tick; `_sync` does nothing; `_run` only recomputes the deadline | the scheduler spine plan of this phase |

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Plan 03-02 (manifest rows, notices, budget ratchet) shares no file with this plan. The scheduler spine plan can edit `cpu.c`, `cpu_exec.inc` and `frame.c` against the declared `sched.h`; the GTE, DMA and timers plans each own one module and one test directory.

## Self-Check: PASSED

- Created files exist: `src/gte.h`, `src/gte.c`, `src/dma.h`, `src/dma.c`, `src/timers.h`, `src/timers.c`, `src/sched.h`, `src/sched.c` and the six files under `tests/gte`, `tests/dma`, `tests/timers`.
- Commits `713cbd8`, `20bc8fd` and `693df67` are ancestors of HEAD; the plan measured 3 commits from the recorded base `02f6574`.
- Every task's acceptance commands exited as the plan says; `cmake --workflow --preset ci` and the ci JUnit check pass (241 tests ran once each); `sh scripts/hygiene.sh` exits 0.

---
*Phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers*
*Completed: 2026-11-10*
