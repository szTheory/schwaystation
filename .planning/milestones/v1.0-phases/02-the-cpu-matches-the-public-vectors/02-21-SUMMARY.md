---
phase: 02-the-cpu-matches-the-public-vectors
plan: 21
subsystem: cpu
tags: [trace, ring, dual-compile, schway_get_trace, first-fault, api-v2]

requires:
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: the run loop with blocks and block_end (plan 02-17), interrupts into the CPU and the retirement stamp (02-19), the first-fault record and the run API (02-18)
provides:
  - cpu_exec.inc compiled twice (SCHWAY_TRACE 0 and 1) with the traced copy chosen once per schway__cpu_run call from the budget's SCHWAY_RUN_TRACE flag
  - a 4,096-record ring of 32-byte records in the instance (131,096 bytes), outside the state, the visitor and every hash
  - schway_trace, SCHWAY_RUN_TRACE, SCHWAY_TRACE_EXEC, SCHWAY_TRACE_RD, SCHWAY_TRACE_WR, SCHWAY_TRACE_IRQ, SCHWAY_TRACE_FAULT and schway_get_trace
  - schway__bus_device, the device decode a rd or wr record names
  - seven core.trace tests and the trace sections of the embedding guide and the CPU page
affects: [02-24, 02-25, phase-4-gpu-beam]

actuals:
  tokens: 18100
  tasks: 3
  commits: 3

plan_head_before: d46b729599ea2dea306f94d4df0b1c53cddcce5f
plan_head_after: 6230ed1fa4145f3cc2d04f26fe66e954806226a1
commits: 3

tech-stack:
  added: []
  patterns:
    - "one include compiled twice: the helpers that write no record sit in an untraced-only block, the functions that reach a bus access or an exception entry get a _traced name by macro, and TRACE(...) is empty in the untraced copy"
    - "the traced block loop emits the first fault's record itself, from inst->fault, after the step that latched it, so no cold path outside the loop knows about the ring"
    - "a record keeps the frame as its low 32 bits and the event, width, device and a 16-bit note in one packed word, so 32 bytes carry every event; schway_get_trace widens and unpacks"

key-files:
  created:
    - tests/core/test_trace.c
  modified:
    - src/cpu.c
    - src/cpu_exec.inc
    - src/internal.h
    - src/instance.c
    - src/frame.c
    - src/bus.c
    - src/bus.h
    - include/schwaystation.h
    - tests/api/v2.c
    - tests/core/test_run.c
    - tests/core/tests.cmake
    - tests/core/inventory.txt
    - docs/embedding.md
    - docs/hardware/cpu.md

key-decisions:
  - "the ring is a member of struct schway (struct schway__trace_ring, 131,096 bytes: 4,096 records of 32 bytes, head, count and the block's start tick and cycle count), allocated with the instance, zeroed by a power reset and never touched by create, load or the state walk"
  - "the ring is not cleared per run call: a runner that calls schway_run once per frame keeps one rolling tail, and only a power reset empties it"
  - "schway_trace is 64 bytes with no reserved field: size, event, frame u64, cycle u64, then line, dot, pc, opcode, addr, width, device, value, note and count as u32, offsets pinned by static asserts and SCHWAY__TRACE_SIZE_FIRST 64"
  - "a fault record is written by the traced block loop at the cycle its instruction began, with the fault kind in value and the loop count in note; an irq record carries the address returned to in pc, the word there in opcode and CAUSE as it was in value"
  - "rd and wr records cover accesses that fail, so a bus error shows the read and then the fault; an access with the cache isolated names device none"
  - "line and dot are derived at read time from the cycles since the record's frame began (the exact floor of frame * num / den, in 64-bit arithmetic that cannot overflow), at 11 to 7 GPU clocks per CPU cycle over 3,413 (NTSC) or 3,406 (PAL) clocks per line; dot is the clock within the line"

patterns-established:
  - "a test that must see a record placed relative to the stub kernel finds the program's first exec record by pc instead of counting from the start"

requirements-completed: [CPU-05]

coverage:
  - id: D1
    description: "cpu_exec.inc is compiled twice in cpu.c with SCHWAY_TRACE 0 and 1, the traced copy runs only when the budget sets SCHWAY_RUN_TRACE, and the untraced copy has no trace code"
    requirement: "CPU-05"
    verification:
      - kind: unit
        ref: "test \"$(grep -c 'cpu_exec.inc' src/cpu.c)\" -ge 2 (exit 0) and ctest --test-dir build/ci -R '^core\\.trace\\.exec_records$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: "A traced run leaves exec, rd and wr records in order with the device, width and value of each access, the stub kernel's memory-control and interrupt-register writes named by device, and line and dot derived from the cycles"
    requirement: "CPU-05"
    verification:
      - kind: unit
        ref: "tests/core/test_trace.c#case_exec_records, run as core.trace.exec_records (exit 0)"
        status: pass
    human_judgment: false
  - id: D3
    description: "The ring keeps exactly the last 4,096 records of a longer run across calls, a shorter run keeps all of them from the first instruction, and the frame, cycle, line and dot of a record in the next frame count from its start"
    requirement: "CPU-05"
    verification:
      - kind: unit
        ref: "tests/core/test_trace.c#case_wrap and #case_short_run, run as core.trace.wrap and core.trace.short_run (exit 0)"
        status: pass
    human_judgment: false
  - id: D4
    description: "A faulting traced run ends with a fault record taken from the fault record, after the instruction's exec and rd records, and an exception loop carries its repeat count of 1000 in note with a full 4,096-record tail"
    requirement: "CPU-05"
    verification:
      - kind: unit
        ref: "tests/core/test_trace.c#case_last_is_fault, run as core.trace.last_is_fault (exit 0)"
        status: pass
    human_judgment: false
  - id: D5
    description: "schway_get_trace answers an index past the end, an empty ring, a NULL argument and a short size tag with an error, a power reset empties the ring, and the version-2 touch test assigns every field and checks every constant"
    requirement: "CPU-05"
    verification:
      - kind: unit
        ref: "tests/core/test_trace.c#case_index_past_end, run as core.trace.index_past_end, and tests/api/v2.c, run as abi.api_v2 (exit 0)"
        status: pass
    human_judgment: false
  - id: D6
    description: "An interrupt taken in a traced run is an irq record that leads the vector's first instruction, carrying the address returned to and CAUSE with the hardware line"
    requirement: "CPU-05"
    verification:
      - kind: unit
        ref: "tests/core/test_trace.c#case_irq, run as core.trace.irq (exit 0)"
        status: pass
    human_judgment: false
  - id: D7
    description: "One program run traced and untraced for four frames gives identical state bytes, video, drained audio, fault record and run results, the state walk names no trace field, and a mutant that touched the state from the trace writer fails the test"
    requirement: "CPU-05"
    verification:
      - kind: unit
        ref: "tests/core/test_trace.c#case_hash_equal, run as core.trace_hash_equal (exit 0); mutant: inst->bus.cache_ctrl += 1u in trace_push failed it"
        status: pass
      - kind: other
        ref: "grep -qi 'trace' src/state.c exits 1 (no trace field in the state walk source)"
        status: pass
    human_judgment: false
  - id: D8
    description: "The embedding guide gives the flag, the ring size, the record fields and events, indexing from the oldest, no cost when clear and the 1.5k to 2.5k instruction history, and the CPU page gives the derived line and dot"
    requirement: "CPU-05"
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci (exit 0, 219 of 219 tests including the docs, api-name and budget checks) and cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake (exit 0, 219 tests ran once each)"
        status: pass
    human_judgment: false

duration: 15min
completed: 2026-11-09
status: complete
---

# Phase 2 Plan 21: Dual-compiled executor, trace ring and schway_get_trace Summary

**The executor compiled twice (untraced and traced, chosen once per run from SCHWAY_RUN_TRACE) writing a 131,096-byte ring of 4,096 32-byte records outside the state and every hash, read through schway_get_trace and proven hash-neutral by a four-frame comparison**

## Performance

- **Duration:** about 15 min
- **Started:** 2026-11-09T22:46:00Z
- **Completed:** 2026-11-09T22:59:00Z
- **Tasks:** 3
- **Files modified:** 15 (one created)

## Accomplishments

- `src/cpu_exec.inc` now holds one untraced-only block (the replay bus, the write queue, the fetch front end, the isolated-cache paths, the multiply and divide helpers, the arithmetic helpers) and the functions that reach a bus access or an exception entry, which `cpu.c` includes twice. The second inclusion renames each function with a `_traced` suffix by macro and turns `TRACE(...)` into a call that writes one record. In the untraced copy `TRACE(...)` is `((void)0)`, so it holds no trace code and no branch for one.
- `schway__cpu_run` picks `cpu_block` or `cpu_block_traced` once per call (a local function pointer, no table) from `SCHWAY_RUN_TRACE`. `schway_run` now accepts the flag; `schway_run_frame` never traces.
- The ring is `struct schway__trace_ring` in `struct schway`: 4,096 records of 32 bytes, 131,096 bytes with its bookkeeping, zeroed by a power reset and by `create`, written only by the traced copy, and absent from the state walk, the visitor and every hash. A full ring overwrites its oldest record; the ring is not cleared per run call.
- `exec` records are written after the fetch, `rd` and `wr` at the two bus front ends (including the failing ones and the isolated-cache paths), `irq` at the interrupt entry, and `fault` by the traced block loop after the step that latched the first fault. `schway_get_trace` copies record `index` from the oldest with `count` held and refuses an index past the end.
- `schway__bus_device` in `bus.c`, the one place guest addresses are decoded, gives each rd and wr record its device.
- `line` and `dot` are derived when a record is read: cycles since the record's frame began (the exact floor of frame * num / den), converted at 11 to 7 and split by 3,413 (NTSC) or 3,406 (PAL) clocks per line, until the GPU keeps a beam.
- Seven tests: `core.trace.exec_records`, `wrap`, `short_run`, `last_is_fault`, `index_past_end`, `irq` and `core.trace_hash_equal`. The embedding guide and the CPU page document the trace.

## Task Commits

1. **Task 1: Tracer, a traced run of a small EXE leaves exec records readable through schway_get_trace** - `8e81dc2` (feat)
2. **Task 2: Ring wrap, order, the fault event and hash equality** - `38e9f05` (test)
3. **Task 3: Trace documentation** - `6230ed1` (docs)

**Plan metadata:** the docs(02-21) commit that adds this summary, STATE.md and ROADMAP.md.

## Files Created/Modified

- `src/cpu_exec.inc` - the untraced-only helper block, the name macros, `TRACE`, `TRACE_BEGIN`, `TRACE_START` and `TRACE_FAULT`, the trace points in the step, the two bus front ends, the interrupt entry and the block loop
- `src/cpu.c` - `trace_now` and `trace_push`, the two inclusions and the once-per-call choice in `schway__cpu_run`
- `src/internal.h` - the record and ring structs, the packed word's macros, the `schway_trace` layout asserts and `SCHWAY__TRACE_SIZE_FIRST`; the ring member of `struct schway`
- `src/instance.c` - `schway_get_trace` and the power reset's clear
- `src/frame.c` - `schway_run` accepts `SCHWAY_RUN_TRACE`
- `src/bus.c`, `src/bus.h` - `schway__bus_device`
- `include/schwaystation.h` - `SCHWAY_RUN_TRACE`, the five events, `schway_trace`, `schway_get_trace`
- `tests/core/test_trace.c`, `tests/core/tests.cmake`, `tests/core/inventory.txt` - the executable and seven registered tests
- `tests/api/v2.c` - touches every new name; `tests/core/test_run.c` - the refused flag bit is 4
- `docs/embedding.md`, `docs/hardware/cpu.md` - the trace sections

## Decisions Made

See `key-decisions` above. The ring's size is `sizeof(struct schway__trace_ring)` = 131,096 bytes (4,096 * 32 plus 24), so the instance is 3,302,264 bytes; no allocation happens after create.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] The device of a data access needs a decode, in `bus.c`**
- **Found during:** Task 1
- **Issue:** The plan's file list has no `bus.c` or `bus.h`, but an rd or wr record names a device and no function gave the device of an address; the first-fault record only learns it inside the access.
- **Fix:** Added `schway__bus_device(addr)`, a pure decode of the same map `bus_access` and the I/O page switch use, in `bus.c` (the only place guest addresses are decoded).
- **Files modified:** `src/bus.c`, `src/bus.h`
- **Verification:** `core.trace.exec_records` reads RAM, memory-control and interrupt-register devices; `core.trace.last_is_fault` reads the first expansion region.
- **Committed in:** `8e81dc2`

**2. [Rule 3 - Blocking] A guide entry has to land with the header in Task 1**
- **Found during:** Task 1
- **Issue:** The ci workflow runs the API-name check, which fails when the header names something the guide does not, so Task 1's own verify command could not pass with the guide untouched until Task 3.
- **Fix:** Task 1 added a short "The trace" section and the new flag to the Bounded runs list; Task 3 replaced it with the full section and the CPU page.
- **Files modified:** `docs/embedding.md`
- **Committed in:** `8e81dc2`, expanded in `6230ed1`

**3. [Rule 1 - Bug] An existing test treated flag bit 2 as undefined**
- **Found during:** Task 1
- **Issue:** `core.run.budget_resume` checked that `flags = 2u` is refused; the plan defines `SCHWAY_RUN_TRACE` as 2, so the check became wrong, and so did the identical line in `tests/api/v2.c`.
- **Fix:** Both now use bit 4 as the bit no release defines.
- **Files modified:** `tests/core/test_run.c`, `tests/api/v2.c`
- **Verification:** `cmake --workflow --preset ci` (exit 0, 218 of 218 at that commit)
- **Committed in:** `8e81dc2`

**4. [Rule 2 - Missing critical] The interrupt record had no test**
- **Found during:** Task 2
- **Issue:** The truths name an `irq` record, and none of the plan's six cases takes an interrupt.
- **Fix:** Added `core.trace.irq` (a vertical-blank interrupt taken in a traced run; the record leads the vector's first instruction).
- **Files modified:** `tests/core/test_trace.c`, `tests/core/tests.cmake`, `tests/core/inventory.txt`
- **Committed in:** `38e9f05`

**5. [Plan inconsistency] `schway_trace` has no `reserved0`**
- **Found during:** Task 1
- **Issue:** The plan lists a `reserved0` field and a size of 64 bytes; with the `cycle` and `frame` 64-bit fields aligned to 8, the listed fields come to 68 bytes padded to 72.
- **Fix:** Dropped `reserved0`; the struct is 64 bytes with no padding on any target and its offsets are pinned by static asserts. The size, the field names the runner's twelve-field line needs and `SCHWAY__TRACE_SIZE_FIRST` 64 all hold.
- **Files modified:** `include/schwaystation.h`, `src/internal.h`
- **Committed in:** `8e81dc2`

**6. [Plan order] The trace points and the full case set landed with the tracer**
- **Found during:** Task 1
- **Issue:** The tracer's gate (`core.trace.exec_records` plus the vectors and fixtures) is only meaningful once rd and wr records, the fault event and the ring wrap are in, and `src/cpu.h` needed no change.
- **Fix:** Task 1 holds the whole implementation and its six cases; Task 2's commit adds `core.trace.irq` and its verification (the mutant check and the acceptance commands) ran against the full set.
- **Committed in:** `8e81dc2`, `38e9f05`

---

**Total deviations:** 6 (2 blocking, 1 bug, 1 missing critical, 2 plan shape)
**Impact on plan:** One file beyond the thirteen the plan lists for source and docs (`src/bus.c` and `src/bus.h`, a pure decode) and one existing test line changed; no change to the design.

## Issues Encountered

- A 16-cycle budget from power-on holds one record: the first BIOS fetches cost 94 cycles before the stub writes the memory-control delays. The short-run test uses 1,000 cycles instead.
- No other problems; each case passed on its first full run once the budgets were right, and a mutant that wrote the state from the trace writer failed `core.trace_hash_equal`.

## Known Stubs

None.

## Threat Flags

None. `schway_get_trace` is a read-only accessor over memory the instance already owns and writes nothing outside the caller's struct; the ring is bounded and fixed.

## Next Phase Readiness

- Plan 02-24 can format `frame line dot cycle pc opcode event addr width device value note` from `schway_get_trace` and set `SCHWAY_RUN_TRACE` under `--trace first-fault`; a fault record is the last one when the run stops on the fault, and an exception loop carries 1000 in `note`.
- The traced and untraced loops' cost comparison (the Ir proof) stays a later seed, as the context says.
- No blockers.

## Self-Check: PASSED

- `src/cpu.c`, `src/cpu_exec.inc`, `src/internal.h`, `src/instance.c`, `src/frame.c`, `src/bus.c`, `src/bus.h`, `include/schwaystation.h`, `tests/api/v2.c`, `tests/core/test_trace.c`, `tests/core/tests.cmake`, `tests/core/inventory.txt`, `docs/embedding.md`, `docs/hardware/cpu.md` exist.
- Commits `8e81dc2`, `38e9f05` and `6230ed1` are ancestors of HEAD.
- `cmake --workflow --preset ci` exit 0, 219 of 219; check_junit exit 0, 219 tests ran once each.

---
*Phase: 02-the-cpu-matches-the-public-vectors*
*Completed: 2026-11-09*
