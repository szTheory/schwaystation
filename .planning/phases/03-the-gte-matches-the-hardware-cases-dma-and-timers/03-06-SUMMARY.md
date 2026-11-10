---
phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
plan: 06
subsystem: core
tags:
  - cpu
  - cop2
  - gte
  - interrupt-entry
  - timing

requires:
  - phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
    provides: gte.h with the register file, read, write and exec entry points and the 22-command cycle table (03-01, 03-05), the block clock and the interrupt entry spine (03-03), schway__bus_ram_word (03-04)
provides:
  - cpu_cop2 reaching the GTE with every COP2 form, no access counted as unbuilt
  - the GTE busy stamp with the multiply convention, the wait on MFC2, CFC2, SWC2 and a new command, clamped to 44
  - an interrupt entry that peeks the interrupted instruction at pc without side effects and executes a pending GTE command through the hook
  - tests/cpu/cop2_tests.c, cpu.timing.gte_cycles, cpu.timing.gte_stall_rules and three interrupt-entry cases
  - the CPU page section on COP2 and the GTE, and the COP0 page hook paragraph
affects:
  - 03-26
  - phase-3 plans that compute the GTE commands
  - phase 4 CPU-side GTE timing questions

actuals:
  tokens: 14200
  tasks: 3
  commits: 3

plan_head_before: dd745aa8be64b46a8db9c7e52f7592737e283bb2
plan_head_after: ff05731cb4bd39f62131d951be241d1a6fde41ed
commits: 3

tech-stack:
  added: []
  patterns:
    - "the GTE busy stamp is gte.ready = cycle + (cycles - 1) + n, set when a command issues; an instruction that waits adds min(ready - issue cycle, 44) to its own cycles before it reads or issues"
    - "the interrupt entry peeks the interrupted word with a side-effect-free reader (cache line, then RAM, then BIOS, 0 under a replay log) and hands it to the one COP0 hook, which executes a GTE command only when ExcCode is 0 and SR.CU2 is set"

key-files:
  created:
    - tests/cpu/cop2_tests.c
  modified:
    - src/cpu_exec.inc
    - src/cop0.c
    - src/cpu.h
    - tests/cpu/exception_tests.c
    - tests/cpu/timing_tests.c
    - tests/cpu/tests.cmake
    - tests/cpu/inventory.txt
    - docs/hardware/cpu.md
    - docs/hardware/cop0.md
    - docs/hardware/gte.md
  deleted:
    - tests/cpu/cop2_prephase.c

key-decisions:
  - "The hook returns 1 only when it executed a command, not when the word merely is one; with SR.CU2 clear or a non-interrupt ExcCode it returns 0 and touches nothing, because that instruction would raise CpU and never reach the GTE"
  - "The entry's command sets gte.ready from cpu.cycle at the entry and does not wait for a command still running; the wait applies to the second run after the return, which issues through cpu_cop2"
  - "The peek returns 0 for KSEG2 and under a replay log, uses the cache only for KUSEG and KSEG0 outside the BIOS window, and aligns the address down as a fetch does"
  - "The two-instruction write-to-use latency of MTC2, CTC2 and LWC2 stays unmodelled and is a documented deviation on the CPU page"

requirements-completed: []

coverage:
  - id: D1
    description: "A GTE command issued by the CPU keeps the engine busy for its documented time and an MFC2 right after it waits for the rest: the pair costs the busy time on the read, for all 22 commands"
    requirement: "GTE-01"
    verification:
      - kind: unit
        ref: "tests/cpu/timing_tests.c#gte_cycles (cpu.timing.gte_cycles: for each of the 22 commands the stamp is the busy time after issue and the following MFC2 takes exactly the busy time)"
        status: pass
    human_judgment: false
  - id: D2
    description: "MFC2, CFC2, SWC2 and a second command wait for the running command, MTC2, CTC2 and LWC2 do not, each intervening no-operation shortens the wait by one, and a stamp set far ahead waits exactly 44"
    requirement: "GTE-01"
    verification:
      - kind: unit
        ref: "tests/cpu/timing_tests.c#gte_stall_rules (cpu.timing.gte_stall_rules: waits of 43, 40 and 37 after an NCDT at 0, 3 and 6 no-operations, the quiet forms unchanged, a far stamp 44, a past stamp 0)"
        status: pass
    human_judgment: false
  - id: D3
    description: "With SR.CU2 clear every COP2 form raises CpU with CE 2 and latches no fault; with it set the moves go through the load delay, LWC2 and SWC2 use the real bus, and none of it is counted as an unbuilt access; the pre-GTE test file is gone"
    requirement: "GTE-01"
    verification:
      - kind: unit
        ref: "tests/cpu/cop2_tests.c#cu2_clear, #moves, #no_unbuilt (cpu.cop2.cu2_clear, cpu.cop2.moves, cpu.cop2.no_unbuilt)"
        status: pass
      - kind: other
        ref: "test ! -e tests/cpu/cop2_prephase.c && ! grep -q cop2_prephase tests/cpu/inventory.txt tests/cpu/tests.cmake (exit 0, baseline at HEAD: exit 1); grep -q SCHWAY__DEV_GTE src/cpu_exec.inc (exit 1)"
        status: pass
    human_judgment: false
  - id: D4
    description: "An interrupt due before a GTE command executes it at the entry, before the handler's first instruction, and again after the return; in a branch delay slot the slot's command is the one run, a plain slot word runs nothing, and with SR.CU2 clear nothing runs"
    requirement: "GTE-01"
    verification:
      - kind: unit
        ref: "tests/cpu/exception_tests.c#irq_gte_double_execute, #irq_gte_delay_slot (cpu.exception.irq_gte_double_execute, cpu.exception.irq_gte_delay_slot)"
        status: pass
    human_judgment: false
  - id: D5
    description: "The entry reads the pending word without a bus fetch: a command held in the instruction cache runs after the RAM behind it was overwritten with a no-operation, a command in uncached RAM runs with the cache empty, and neither read changes the unbuilt counts, the last-access record or the fault record (the review finding about the word at EPC is fixed)"
    requirement: "GTE-01"
    verification:
      - kind: unit
        ref: "tests/cpu/exception_tests.c#irq_entry_peek (cpu.exception.irq_entry_peek)"
        status: pass
    human_judgment: false
  - id: D6
    description: "The CPU page has the COP2 and the GTE section (the moves, the wait and which forms wait, the clamp, the interrupt entry, and the unmodelled write-to-use latency as a documented deviation) and the COP0 page describes the hook; neither carries a planning identifier"
    requirement: "GTE-01"
    verification:
      - kind: other
        ref: "grep -qE 'D-[0-9]{2}|GTE-0[0-9]|IN-0[0-9]|PX\\.[0-9]|DEC\\.[0-9]' src/cpu_exec.inc src/cop0.c docs/hardware/cpu.md docs/hardware/cop0.md (exit 1, no match); sh scripts/hygiene.sh (exit 0)"
        status: pass
    human_judgment: false

duration: 9min
completed: 2026-11-10
status: complete
---

# Phase 3 Plan 06: COP2 reaches the GTE and the interrupt entry runs the pending command Summary

**Every COP2 form now goes through the GTE with a measured busy stamp and wait rule, and the interrupt entry peeks the interrupted instruction at pc without side effects and executes a pending command at the entry, so it runs again after the return, in and out of a delay slot.**

## Performance

- **Duration:** 9 min
- **Started:** 2026-11-10T05:28:02Z
- **Completed:** 2026-11-10T05:37:00Z
- **Tasks:** 3
- **Files modified:** 12 outside `.planning` (11 changed, 1 created, 1 deleted)

## Accomplishments

- `cpu_gte_wait` follows the multiply unit's convention on `gte.ready` and is clamped to `SCHWAY__GTE_STALL_MAX`; a command executes at issue and stamps `cycle + (cycles - 1) + n`; MFC2, CFC2, SWC2 and a new command wait, MTC2, CTC2 and LWC2 do not. The wait decays by one per intervening cycle and a loaded stamp cannot make it longer than 44.
- `cpu_cop2` has no `schway__bus_unbuilt` call left: LWC2 does the real bus read into the data register, SWC2 the real bus write of it, the moves go through `schway__gte_read` and `schway__gte_write`, BC2F and BC2T keep their behaviour.
- `cpu_irq_enter` no longer fetches from the bus. `cpu_peek_word` reads the instruction at pc (the delay slot's instruction, not the branch EPC names) from the cache line when it holds it, else RAM, else the BIOS image, else 0, with no fault, count or cost. This fixes the phase-2 review finding IN-01 before the hook executes anything: Task 3's first edit was the peek, and the delay-slot test fails if the peek is pointed back at EPC (mutation run by hand).
- `schway__cop0_irq_entry_cop2` executes the command when ExcCode is 0, SR.CU2 is set and `word >> 25 == 25h`, and stamps the engine from the entry's cycle.
- Tests: `cpu.timing.gte_cycles` (22 busy times), `cpu.timing.gte_stall_rules`, `cpu.cop2.{cu2_clear,moves,no_unbuilt}`, `cpu.exception.{irq_gte_double_execute,irq_gte_delay_slot,irq_entry_peek}`; `cop2_prephase.c` and `irq_entry_cop2_hook` are gone. The full ci workflow runs 256 tests.
- Mutations run by hand and caught: a stamp from `cycle + 1` instead of `cycle + (cycles - 1)` fails `gte_cycles`; dropping SWC2's wait fails `gte_stall_rules`; peeking at EPC fails `irq_gte_delay_slot`; skipping the cache in the peek fails `irq_entry_peek`.

## Task Commits

1. **Task 1: Tracer - a GTE command issued by the CPU stalls the next MFC2 by its busy time** - `15e17c2` (feat)
2. **Task 2: Every COP2 form through the GTE, the stall rule, and the pre-GTE tests replaced** - `b98e1e3` (feat)
3. **Task 3: The interrupt entry peeks the pending command and runs it twice; the CPU and COP0 pages** - `ff05731` (feat)

**Plan metadata:** the docs commit that carries this summary, STATE.md and ROADMAP.md.

The tracer gate ran in the default end-of-phase mode with an automated-only verify: `cmake --workflow --preset ci` and the `cpu.timing.*` ctest filter were re-run after Task 1, passed, and the expansion tasks went ahead.

## Files Created/Modified

- `src/cpu_exec.inc` - `cpu_gte_wait`, `cpu_peek_word`, the new `cpu_cop2` and `cpu_irq_enter`
- `src/cop0.c`, `src/cpu.h` - the hook executes the pending command; its declaration comment
- `tests/cpu/cop2_tests.c` (new, replaces `cop2_prephase.c`), `tests/cpu/timing_tests.c`, `tests/cpu/exception_tests.c`, `tests/cpu/tests.cmake`, `tests/cpu/inventory.txt` - the tests above
- `docs/hardware/cpu.md` - "COP2 and the GTE" replaces "COP2 before the GTE", with the deviation; `docs/hardware/cop0.md` - the hook paragraph; `docs/hardware/gte.md` - one sentence that pointed at the removed heading

## Decisions Made

See `key-decisions`. The plan names the CU2 condition on the hook as discretionary; the hook returns 0 and does nothing without it.

## Triage

None. No phase-2 assertion, hash or fixture moved: `exc_matrix`, the r3000 vectors (13 tests) and every hash test pass unchanged. The only phase-2 assertions touched are the ones the plan replaces: `cpu.cop2_prephase.cu2_clear` moved into `cpu.cop2.cu2_clear` unchanged in content, `cu2_set` is replaced by `moves` and `no_unbuilt`, and `irq_entry_cop2_hook` by the three interrupt cases.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] The old hook test's final unbuilt-count assertion changed in Task 2**
- **Found during:** Task 2
- **Issue:** `cpu.exception.irq_entry_cop2_hook` ended by asserting the command run after the return counted one unbuilt GTE access; once `cpu_cop2` stopped counting, `cmake --workflow --preset ci` (Task 2's verify) failed on that line, and the test is only replaced in Task 3.
- **Fix:** the assertion now expects 0, for the one commit until Task 3 deleted the case.
- **Files modified:** tests/cpu/exception_tests.c
- **Verification:** the Task 2 ci run passed (254 tests).
- **Committed in:** b98e1e3

**2. [Rule 1 - Bug] A stale pointer in the GTE page**
- **Found during:** Task 3
- **Issue:** `docs/hardware/gte.md` told the reader that the CPU's COP2 forms still take the path in "COP2 before the GTE", a section this plan removes and a statement that is no longer true. The file is not in the plan's file list.
- **Fix:** one sentence rewritten to point at "COP2 and the GTE".
- **Files modified:** docs/hardware/gte.md
- **Verification:** `sh scripts/hygiene.sh` exit 0; no planning identifier in the page.
- **Committed in:** ff05731

---

**Total deviations:** 2 auto-fixed (1 blocking, 1 bug)
**Impact on plan:** both are one-line edits inside the plan's subject; no scope creep.

## Issues Encountered

- The first `moves` draft ran each word as soon as it was written and the instruction cache still held the line from the previous fetch, so the next word read as a no-operation. The test's `put` now empties the instruction cache, as the exception tests' does.
- The `asan` and `be32` lanes need `clang-21`, which this host lacks, so they were not run locally; hosted CI runs them. The perf job's Ir/frame was not re-measured: the change adds a branch-free compare in `cpu_cop2` and the interrupt entry only.
- The broken-windows ledger `.planning/WINDOWS.md` does not exist and this plan leaves no stub, so nothing was appended.

## Known Stubs

None added. `schway__gte_exec` still computes nothing (the seed plan 03-26 fills it); this plan uses only its busy-cycle return.

## Threat Flags

None. No new input surface: the peek reads instance memory only, bounded by the RAM and BIOS windows, and returns 0 under a replay log.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

The CPU side of the GTE spine is complete: once the seed plan fills the commands, a program that issues them, reads the results and takes an interrupt in between sees the hardware's wait rule and the double execution. Not modelled and documented: the two-instruction write-to-use latency after MTC2, CTC2 and LWC2.

## Self-Check: PASSED

- Created file exists: `tests/cpu/cop2_tests.c`; deleted file is absent: `tests/cpu/cop2_prephase.c`.
- Commits `15e17c2`, `b98e1e3` and `ff05731` are ancestors of HEAD; the plan measured 3 commits from the recorded base `dd745aa`.
- Every task's acceptance commands exited as the plan says; `cmake --workflow --preset ci` (256 tests), `ctest -L '^cpu$'`, the ci JUnit check (256 tests ran once each) and `sh scripts/hygiene.sh` all exit 0.

---
*Phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers*
*Completed: 2026-11-10*
