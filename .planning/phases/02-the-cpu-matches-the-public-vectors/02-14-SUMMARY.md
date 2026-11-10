---
phase: 02-the-cpu-matches-the-public-vectors
plan: 14
subsystem: cpu
tags: [cpu, icache, cache-isolation, write-queue, bus, state]

requires:
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: the data access front-ends, the load latch and the cycle counter in cpu_step (plan 02-12)
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: the bus with segment decode, the cache-control word, the first-fault record and read costs (plan 02-07)
provides:
  - schway__cpu_icache_fetch, the 256-line direct-mapped I-cache with per-word valid bits that every KUSEG and KSEG0 fetch outside the BIOS window goes through
  - SR.Isc cache isolation with the TAG and IS1 tag-write rule, so the BIOS FlushCache sequence leaves every line invalid
  - wq_count, the 4-deep write queue as a count, drained by any KSEG1 read, write or fetch
  - cpu.icache_tag, cpu.icache_word, cpu.icache_valid and cpu.wq_count in reset and the state walk, with load-time bounds
  - bus.icache_fill, bus.icache_valid_bits, bus.icache_isolation, bus.write_queue_depth, bus.write_queue_drain, bus.write_queue_exception and bus.write_queue_state
  - the Instruction cache and Write queue sections of docs/hardware/bus.md and the Instruction fetch section of docs/hardware/cpu.md
affects: [02-15, 02-17, 02-19, 02-25]

actuals:
  tokens: 10700
  tasks: 3
  commits: 3

plan_head_before: 8c374a8ca514d8dd45fa002331cc3211bacd69bb
plan_head_after: b74ffd6d6741c8095c1578fb06cb23c0644a0a0c
commits: 3

tech-stack:
  added: []
  patterns:
    - "the cache and the queue live in the CPU, in front of the bus: the bus has no idea they exist and the vector replay path bypasses both by taking the log branch first in cpu_fetch, cpu_bus_read and cpu_bus_write"
    - "the write queue is a count, not a list of stores: every store reaches the bus at once and lands in order, and only the cost of waiting is modelled, so a state needs one number and an exception cannot drop or reorder anything"
    - "a cache fill that fails on a word after the one asked for puts the first-fault record back, so the record only ever names an access the CPU made"

key-files:
  created:
    - tests/bus/cache_tests.c
  modified:
    - src/cpu.h
    - src/cpu.c
    - src/cpu_exec.inc
    - src/internal.h
    - tests/bus/tests.cmake
    - tests/bus/inventory.txt
    - tests/cpu/cop0_tests.c
    - docs/hardware/bus.md
    - docs/hardware/cpu.md

key-decisions:
  - "schway__cpu_icache_fetch takes the virtual address of the word, not the physical one the plan names: the bus decodes the segment from the virtual address, and a bus error on a KUSEG fetch must record the KUSEG address as the first fault. The physical index and tag are derived inside"
  - "A tag hit on a word that is not valid refills from that word to the line's end and keeps the words already valid, one mechanism for every miss. PSX-SYSTEM says the whole line is refilled; the two differ only in the valid bits of earlier words, and the page says so"
  - "A fill costs the bus charge of the missed word plus one cycle per further word, the 4-plus-words burst rate of RAM; a hit adds nothing. Both are approximations documented as such, and only the fill's shape is tested"
  - "The write queue has no retirement by elapsed time: it empties only by a fifth store (one drain), a KSEG1 access (all of it) or a power reset. A rule that retires entries with time would be a timing behaviour with no measurement behind it and no test that may pin it, so the model stays pessimistic and says so; the drain is an estimated 4 cycles per store"
  - "A store to the scratchpad does not join the queue and a store that is a bus error does not either; a KSEG1 store drains the queue and then joins it"
  - "Isolation sends KUSEG, KSEG0 and KSEG1 data accesses to the I-cache arrays and leaves the KSEG2 cache-control word on the bus, so the FlushCache restore of cache control works while Isc is set. The scratchpad alternative the sources mention for TAG clear is not modelled and the page says so"
  - "The I-cache is always on: cache control's enable bits are not modelled, only TAG and IS1 matter, and only under Isc"

requirements-completed: [CPU-03]

coverage:
  - id: D1
    description: "KUSEG and KSEG0 fetches outside the BIOS window go through a direct-mapped I-cache of 256 lines of 4 words (index bits 11:4, tag physical bits 31:12, a valid bit per word); a miss fills from the missed word to the line end, a hit costs nothing extra, KSEG1 fetches bypass it, a store never changes a hit"
    requirement: "CPU-03"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^bus\\.icache_fill$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: "A tag hit on an invalid word refills from that word on and keeps valid words, another tag takes the line over, IBLKSZ does not shorten a fill, a bus error on the missed word leaves the line alone and one on a later word ends the fill with no fault recorded"
    requirement: "CPU-03"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^bus\\.icache_valid_bits$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D3
    description: "With SR.Isc set, loads and stores go to the cache and never the bus; a load returns the line's word whatever the tag; with cache-control TAG and IS1 set a store writes the tag and the valid bits, otherwise the word; KSEG2 still reaches the bus; the FlushCache sequence leaves every line invalid"
    requirement: "CPU-03"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^bus\\.icache_isolation$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D4
    description: "The write queue holds 4 stores, a fifth costs more and leaves the count at 4, scratchpad and failed stores do not join it, any KSEG1 read, write or fetch drains it first, a cached read does not, an empty queue adds no drain cycles, and stores land in order; only depth, order and drain are pinned, not the cycles of a wait"
    requirement: "CPU-03"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^bus\\.write_queue_(depth|drain)$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D5
    description: "The vector replay path bypasses the I-cache and the queue: with a replay log attached a store is emitted at its issue point, wq_count and the cache do not change, and all 5,500 sample cases still pass"
    requirement: "CPU-03"
    verification:
      - kind: unit
        ref: "tests/bus/cache_tests.c#write_queue_depth (replay log block, bus.write_queue_depth exit 0)"
        status: pass
      - kind: integration
        ref: "ctest --test-dir build/ci -L vectors --no-tests=error (exit 0, 13 tests)"
        status: pass
    human_judgment: false
  - id: D6
    description: "An exception taken with stores queued neither drops, reorders nor drains them, and a state saved with wq_count 3 and a filled cache restores both, and RAM, byte for byte after a power reset; wq_count and the tags are bounded on load"
    requirement: "CPU-03"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^bus\\.write_queue_(exception|state)$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D7
    description: "docs/hardware/bus.md has Instruction cache and Write queue sections and docs/hardware/cpu.md an Instruction fetch section, each fact cited, the timing marked approximate, the replay bypass stated, and no planning ID"
    requirement: "CPU-03"
    verification:
      - kind: other
        ref: "grep -q 'Instruction cache' docs/hardware/bus.md && grep -q 'Write queue' docs/hardware/bus.md (exit 0)"
        status: pass
      - kind: other
        ref: "grep -qE 'D-[0-9]{2}|CPU-0[0-9]|PX\\.|DEC\\.[0-9]' docs/hardware/bus.md docs/hardware/cpu.md (exit 1, no match)"
        status: pass
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^docs\\.' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false

duration: 11min
completed: 2026-11-09
status: complete
---

# Phase 2 Plan 14: The I-cache, cache isolation and the write queue Summary

**A 256-line direct-mapped I-cache with per-word valid bits on every cached fetch, SR.Isc isolation with the TAG and IS1 tag-write rule that makes FlushCache work, and a 4-deep write queue kept as a count that KSEG1 accesses drain, all in the saved state and bypassed by the vector replay path so the 5,500 cases still pass.**

## Performance

- **Duration:** 11 min
- **Started:** 2026-11-09T21:14:08Z (the previous plan's last commit; the start time was not recorded)
- **Completed:** 2026-11-09T21:26:00Z
- **Tasks:** 3
- **Files modified:** 10 (1 created, 9 modified)

## Accomplishments

- `schway__cpu_icache_fetch` in `src/cpu.c` is the cached fetch: a hit is free, a miss or a tag hit on an invalid word fills from the missed word to the line end, a later-word bus error ends the fill and puts the first-fault record back. `cpu_fetch` routes KUSEG and KSEG0 outside the BIOS window to it, drains the queue for KSEG1 and replays the log with neither.
- SR.Isc isolation in `cpu_bus_read` and `cpu_bus_write`: data accesses in KUSEG, KSEG0 and KSEG1 hit the cache arrays; TAG with IS1 writes tag and valid bits. A test runs the 256-store FlushCache sequence through the CPU and checks every line invalid and a formerly cached address missing afterwards.
- `wq_count` with `SCHWAY__WQ_DEPTH` 4 and `SCHWAY__WQ_DRAIN_CYCLES` 4: a fifth store waits one drain, any KSEG1 access drains all, an empty queue costs nothing, scratchpad and failed stores do not queue. The cache arrays and the count are in reset and the state walk with bounds (count 0 to 4, tag under 100000h, valid under 10h).
- Seven `bus.*` cases in `tests/bus/cache_tests.c` (one executable, 380 lines). Three mutations were run by hand and each fails its test: no fifth-store wait, no KSEG1 read drain, no valid-bit check on a tag hit.
- The bus and CPU pages say what the model does and where it differs from the sources (refill from the missed word, IBLKSZ ignored, scratchpad alternative not modelled, no retirement by time).

## Task Commits

1. **Task 1 (tracer): a KSEG0 fetch misses, fills to the line end, then hits** - `12980f2` (feat)
2. **Task 2: valid bits, isolation, tag writes and the write queue** - `9910f2e` (feat)
3. **Task 3: the cache sections of the bus and CPU pages** - `b74ffd6` (docs)

**Plan metadata:** the docs commit that carries this summary, STATE.md and ROADMAP.md.

The tracer gate ran at end-of-phase mode with an automated-only verify: `cmake --workflow --preset ci` and `ctest -R '^(bus\.icache_fill|vectors\.r3000\.sample)$'` exited 0 before Task 2 started.

## Files Created/Modified

- `src/cpu.h` - SR.Isc, the cache and queue constants, the new cpu fields, the `schway__cpu_icache_fetch` declaration
- `src/cpu.c` - the cache fetch, reset and the four new state fields
- `src/cpu_exec.inc` - `cpu_fetch` routing, isolation helpers, `cpu_wq_drain`, `cpu_wq_push`, and the two data front-ends
- `src/internal.h` - the layout comment only; the `exc_repeat` assertion is unchanged at 312
- `tests/bus/cache_tests.c`, `tests/bus/tests.cmake`, `tests/bus/inventory.txt` - the seven cases
- `tests/cpu/cop0_tests.c` - `put` empties the I-cache
- `docs/hardware/bus.md`, `docs/hardware/cpu.md` - the new sections

## Decisions Made

See `key-decisions`. The plan that adds the run loop (02-17) inherits the cached fetch with no change; the timing plan (02-19) can refine the queue's retirement and the fill cost with a measurement, and the interrupt case for the queue is that plan's.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] tests/cpu/cop0_tests.c rewrote code in place, which a cache makes stale**
- **Found during:** Task 1
- **Issue:** nine `cpu.cop0.*` tests put a different word at the same KSEG0 program counter before each step; with a cached fetch the second step ran the first word. The file is not in the plan's list.
- **Fix:** the test helper `put` empties the valid bits, as a program rewriting code would flush the cache. No behaviour of the core changed for it.
- **Files modified:** `tests/cpu/cop0_tests.c`
- **Verification:** `cmake --workflow --preset ci` exit 0 (153 tests)
- **Committed in:** `12980f2`

**2. [Rule 3 - Blocking] src/internal.h comment about the cpu layout**
- **Found during:** Task 2
- **Issue:** the comment said the state part of the cpu ends at `exc_repeat`; the write count and cache arrays now follow it before the log pointer. Not in the plan's list.
- **Fix:** the comment says what follows; the assertion is unchanged.
- **Files modified:** `src/internal.h`
- **Committed in:** `9910f2e`

**3. [Rule 1 - Bug avoided] the fetch function takes the virtual address**
- **Found during:** Task 1
- **Issue:** the plan's signature names a physical address, but the bus decodes the segment from the virtual address and the first-fault record must carry the address the CPU used.
- **Fix:** `schway__cpu_icache_fetch(schway *inst, uint32_t vaddr, ...)`; the symbol, the call site and every test name are as the plan has them.
- **Files modified:** `src/cpu.h`, `src/cpu.c`
- **Committed in:** `12980f2`

---

**Total deviations:** 3 (2 blocking, 1 signature adjustment). **Impact on plan:** all local; every artifact, symbol and test the plan names exists.

## Issues Encountered

- A first mutation check was inconclusive because the mutated source did not compile under the project's warnings and the old binary ran; it was redone with a compiling mutation. The one-off `cc -fsanitize=address,undefined` build of `tests/bus/cache_tests.c` with `src/*.c` ran all seven cases with exit 0 (the asan and nofp presets cannot configure on this host, as the previous plan recorded).

## Known Stubs

None. The queue's retirement by time and the fill cost for devices other than RAM are documented approximations, not stubs; the timing plan may replace them.

## Threat Flags

None - CPU-internal behaviour and tests. The new state fields are bounded on load (`wq_count` 0 to 4, tags under 100000h, valid bits under 10h), so a hostile state cannot index outside the cache or make a drain unbounded.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 02-17's run loop calls `cpu_step` and gets the cached fetch and the queue with no change; a self-modifying guest must flush the cache, as on the console.
- Plan 02-19 pins the delay and timing rules and the interrupt case of the write queue; the queue's drain figure, the fill cost and the missing retirement by time are the open timing choices, listed on the bus page.
- Not covered by a test: a cached fetch from the I/O page beyond the one gap case, and isolation with a user-mode address; both are reachable through the same two helpers.

## Self-Check: PASSED

- FOUND: tests/bus/cache_tests.c, src/cpu.c, src/cpu_exec.inc, docs/hardware/bus.md, docs/hardware/cpu.md
- FOUND commits: 12980f2, 9910f2e, b74ffd6 (ancestors of HEAD)
- `cmake --workflow --preset ci` exit 0 (153 tests); `cmake --workflow --preset hygiene` exit 0; `ctest -L bus` exit 0 (16 tests); `ctest -L vectors` exit 0 (13 tests)

---
*Phase: 02-the-cpu-matches-the-public-vectors*
*Completed: 2026-11-09*
