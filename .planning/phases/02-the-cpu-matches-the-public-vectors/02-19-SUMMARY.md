---
phase: 02-the-cpu-matches-the-public-vectors
plan: 19
subsystem: cpu
tags: [interrupts, exceptions, load-delay, mult-div-timing, cop0, retirement-stamp]

requires:
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: the run loop with blocks and block_end (plan 02-17), the write queue and instruction cache (02-14), the exception entry and COP0 (02-10), the multiply and divide ready stamp (02-12)
provides:
  - the retirement stamp irq_stamp in the CPU state, armed on the false-to-true change of the interrupt line and counted down per retired instruction
  - interrupt entry (ExcCode 0, EPC+0, pending load landed first) and the one GTE hook schway__cop0_irq_entry_cop2
  - the documented priority between an interrupt and an exception of the same instruction
  - 24 cpu.exception, 6 cpu.delay and 10 cpu.timing tests, tests/diag/allow.txt, and an Interrupts section on the COP0 page
affects: [02-20, 02-21, 02-22, 02-23, 02-24, 02-25, phase-3-gte]

actuals:
  tokens: 23800
  tasks: 3
  commits: 3

plan_head_before: fb6309eec3982f116288e1959e1acd7f7488a3dd
plan_head_after: 527b0ebf59953fc2dc4d09507189461ce3f9306a
commits: 3

tech-stack:
  added: []
  patterns:
    - "the interrupt line is a level and the stamp is armed only by a mutator that changes it (irq_sync, MTC0 to SR or CAUSE, RFE) through one helper, never by a query"
    - "the stamp counts at the end of cpu_step, so one code path serves the run loop and the one-instruction entry"
    - "multiply and divide tests prime the instruction cache so a fetch adds nothing, then read latency as the ready stamp minus the cycle counter before the instruction"

key-files:
  created:
    - tests/cpu/exception_tests.c
    - tests/cpu/delay_tests.c
    - tests/cpu/timing_tests.c
    - tests/diag/allow.txt
  modified:
    - src/cpu.h
    - src/cpu.c
    - src/cpu_exec.inc
    - src/cop0.c
    - src/irq.c
    - tests/cpu/tests.cmake
    - tests/cpu/inventory.txt
    - docs/hardware/cop0.md
    - docs/hardware/cpu.md

key-decisions:
  - "irq_stamp sits after wq_count in the CPU struct and the state walk (range 0 to 2), so the pinned offset of exc_repeat stays 312"
  - "the latency is 2 retirements for a device edge, an I_MASK write and an SR write, counting the writing instruction, and 1 for a CAUSE write and an RFE; at the last retirement the entry is taken only if the line is still true, so an acknowledged source takes the interrupt back"
  - "an interrupt due on the same instruction as an exception ranks by the documented order: SYSCALL, BREAK, RI, CpU, IBE and the fetch AdEL lose to it (it is taken at that instruction), data AdEL, AdES, DBE and overflow win and the entry's cleared IEc holds the interrupt until RFE re-evaluates"
  - "the word handed to the COP2 hook is read with schway__bus_fetch and the first-fault record put back, so the hook costs no cycles and records nothing; with a replay log it is 0"

patterns-established:
  - "Doc-row tests carry one comment naming the latency as an inference unmeasured on silicon and tests/diag/allow.txt as where a change is recorded"

requirements-completed: [CPU-02]

coverage:
  - id: D1
    description: "A device interrupt with SR and I_MASK open is taken two retirements after the edge, returns to EPC+0, shows CAUSE bit 10 at once, pushes the mode stack and records no first fault"
    requirement: "CPU-02"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^(cpu\\.exception\\.irq_latency_device|vectors\\.r3000\\.sample|fixtures\\.hello\\.boot)$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: "Every exception class named by the requirement is pinned: AdEL on LH and LW, AdES on SW, overflow, reserved instruction, CpU on COP1, SYSCALL and BREAK with and without BD, the BEV vector choice, DBE and IBE on unmapped addresses, and SR masking of the line"
    requirement: "CPU-02"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^cpu\\.exception\\.' --no-tests=error (exit 0, 24 tests)"
        status: pass
    human_judgment: false
  - id: D3
    description: "An interrupt due on the same instruction as an exception follows the documented priority, and an interrupt taken with stores queued keeps their order and leaves the queue to the handler's first uncached read"
    requirement: "CPU-02"
    verification:
      - kind: unit
        ref: "tests/cpu/exception_tests.c#priority and #irq_with_queued_stores, run as cpu.exception.priority and cpu.exception.irq_with_queued_stores (exit 0); a mutant that dropped the preemption failed priority"
        status: pass
    human_judgment: false
  - id: D4
    description: "The doc rows are pinned as documented and flagged as an inference unmeasured on silicon: CAUSE write EPC pc+4, SR write EPC pc+8, I_MASK write EPC pc+8, a CAUSE write in a delay slot taken after the slot with BD 0"
    requirement: "CPU-02"
    verification:
      - kind: unit
        ref: "tests/cpu/exception_tests.c#doc_cause_write, #doc_sr_write, #doc_imask_write and #doc_cause_in_delay_slot, run as cpu.exception.doc_* (exit 0)"
        status: pass
    human_judgment: false
  - id: D5
    description: "The interrupt entry calls the one named hook with the word at EPC; the hook reports a COP2 command word and executes nothing, the command runs once after the return, and RFE re-evaluates the line at once"
    requirement: "CPU-02"
    verification:
      - kind: unit
        ref: "tests/cpu/exception_tests.c#irq_entry_cop2_hook and #irq_rfe, run as cpu.exception.irq_entry_cop2_hook and cpu.exception.irq_rfe (exit 0); a mutant that dropped the RFE re-evaluation failed irq_rfe"
        status: pass
    human_judgment: false
  - id: D6
    description: "The load-delay queue, a write cancelling a pending load, a failed load leaving rt untouched, a pending load completing before an interrupt handler, the LWL/LWR interlock and MFC0's delay slot are pinned"
    requirement: "CPU-02"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^cpu\\.delay\\.' --no-tests=error (exit 0, 6 tests); a mutant that left the load pending into the handler failed load_before_irq"
        status: pass
    human_judgment: false
  - id: D7
    description: "MULT 6, 9 and 13 cycles by rs XOR its sign bit, MULTU on raw rs, DIV and DIVU 36, an early MFHI or MFLO stalling, a new multiply or divide replacing a running one, MTLO and MTHI setting the stamp to 0, and HI, LO and the stamp surviving an exception"
    requirement: "CPU-02"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^cpu\\.timing\\.' --no-tests=error (exit 0, 10 tests); mutants that dropped the sign mirror and that kept the later ready stamp failed mult_6, mult_9, multu_raw and replace_while_busy"
        status: pass
    human_judgment: false
  - id: D8
    description: "The COP0 page has an Interrupts section: the line, the stamp and its latencies as an inference unmeasured on silicon, the doc rows with tests/diag/allow.txt, the priority and the COP2 hook left for the GTE"
    requirement: "CPU-02"
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci (exit 0, 207 of 207 tests including the docs and policy checks) and cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake (exit 0, 207 tests ran once each)"
        status: pass
    human_judgment: false

duration: 20min
completed: 2026-11-09
status: complete
---

# Phase 2 Plan 19: Interrupts into the CPU and the exception, delay and timing tests Summary

**Interrupts reach the CPU through a retirement stamp (2 retirements for a device edge, I_MASK and SR writes, 1 for CAUSE and RFE), with the documented exception priority, one GTE hook, and 40 unit tests pinning the exception, load-delay and MULT/DIV timing rules**

## Performance

- **Duration:** 20 min
- **Started:** 2026-11-09T22:03:00Z
- **Completed:** 2026-11-09T22:24:00Z
- **Tasks:** 3
- **Files modified:** 13

## Accomplishments

- The interrupt line (CAUSE IP & SR mask & IEc) arms `irq_stamp` on its false-to-true change only. `irq_sync` (device edges, I_MASK), `MTC0` to SR and CAUSE and `RFE` call one helper, `schway__cop0_irq_edge`; `cpu_step` counts the stamp down after each retired instruction and enters ExcCode 0 at zero if the line is still true. The stamp is in the state walk, so a saved state resumes the same interrupt.
- The entry lands a pending load first, returns to EPC+0 (the branch with BD when the instruction is a delay slot), calls `schway__cop0_irq_entry_cop2` with the word at EPC and executes nothing; the phase-3 GTE plan executes the command there.
- An interrupt due on the same instruction as an exception takes priority over SYSCALL, BREAK, RI, CpU and the fetch errors and yields to data address errors, data bus errors and overflow, per PSX-SYSTEM section 2.
- `cpu.exception.*` (24), `cpu.delay.*` (6) and `cpu.timing.*` (10), with `tests/diag/allow.txt` created for dated changes to the doc rows. MULT/DIV timing, documented on the CPU page by plan 02-12 but unpinned, is now pinned exactly (latency = stamp minus counter, with a primed instruction cache so fetch adds nothing).
- An Interrupts section on `docs/hardware/cop0.md` records the model and says the latencies are an inference unmeasured on silicon.

## Task Commits

1. **Task 1: Tracer, a vblank edge with I_MASK and SR enabled is taken two retirements later** - `cf7609e` (feat)
2. **Task 2: Every exception class and doc row** - `fe980f5` (test)
3. **Task 3: Delay-slot and MULT/DIV timing tests, and the COP0 page's interrupt section** - `527b0eb` (test)

**Plan metadata:** the docs(02-19) commit that adds this summary, STATE.md and ROADMAP.md.

## Files Created/Modified

- `src/cpu.h` - `irq_stamp` field, the latency constants and the declarations of `schway__cop0_irq_pending`, `schway__cop0_irq_edge` and `schway__cop0_irq_entry_cop2`
- `src/cpu.c` - `irq_stamp` in the reset and the state walk (range 0 to 2)
- `src/cpu_exec.inc` - `cpu_irq_enter`, `cpu_below_interrupt`, the preemption in `cpu_exception` and the countdown at the end of `cpu_step`
- `src/cop0.c` - the pending predicate, the edge helper, the hook, and the arming in `MTC0` to SR and CAUSE and in `RFE`; the exception entry clears the stamp
- `src/irq.c` - `irq_sync` arms the stamp on a device edge or an I_MASK write
- `tests/cpu/exception_tests.c`, `delay_tests.c`, `timing_tests.c` - the three test executables
- `tests/diag/allow.txt` - the format comment only
- `tests/cpu/tests.cmake`, `tests/cpu/inventory.txt` - 40 registered tests
- `docs/hardware/cop0.md` - the Interrupts section and the Tests paragraph; `docs/hardware/cpu.md` - names the timing tests

## Decisions Made

- `irq_stamp` follows `wq_count` in the struct and the walk, so the layout assertion on `exc_repeat` (312) is unchanged, and its state bound is 0 to 2 so a hostile state cannot make an interrupt wait unboundedly.
- At the last retirement the interrupt is taken only if the line is still true: an acknowledged source, a cleared mask or a cleared IEc takes it back. Entry clears the stamp, and `RFE` re-arms it with latency 1.
- The preemption lives in `cpu_exception`, the single funnel for instruction exceptions, instead of re-ordering the decoder: a fetch AdEL is told from a data AdEL by the odd program counter.
- The word handed to the hook is read with `schway__bus_fetch` into a scratch cycle count with the fault record restored, so reading it charges and records nothing.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] The stamp has to be part of the saved state**
- **Found during:** Task 1
- **Issue:** The plan's file list has `src/cpu.h` but not `src/cpu.c`. An armed stamp that a save and load dropped would lose a pending interrupt, and every field that changes future behaviour is in the state walk.
- **Fix:** Added `cpu.irq_stamp` to the reset and `schway__cpu_state_io` in `src/cpu.c`, bounded 0 to 2.
- **Files modified:** `src/cpu.c`
- **Verification:** `cmake --workflow --preset ci` (exit 0, 207 of 207); the state, runner hash and vector tests are unchanged.
- **Committed in:** `cf7609e`

**2. [Rule 2 - Missing critical] Two cases beyond the plan's list for behaviour the truths name**
- **Found during:** Task 2
- **Issue:** The truths say RFE re-evaluates at once and a pending interrupt is a stamp, not a side effect; neither had a test in the 22 listed cases.
- **Fix:** Added `cpu.exception.irq_rfe` and `cpu.exception.irq_dropped_before_entry` (24 cases, the acceptance floor is 22).
- **Files modified:** `tests/cpu/exception_tests.c`, `tests/cpu/tests.cmake`, `tests/cpu/inventory.txt`
- **Verification:** a mutant that dropped the RFE re-evaluation failed `irq_rfe`.
- **Committed in:** `fe980f5`

**3. [Rule 2 - Missing critical] CPU page names the timing tests**
- **Found during:** Task 3
- **Issue:** The Multiply and divide section of `docs/hardware/cpu.md` documented the times with no test; plan 02-12 noted this.
- **Fix:** One sentence naming `cpu.timing.*`.
- **Files modified:** `docs/hardware/cpu.md`
- **Committed in:** `527b0eb`

---

**Total deviations:** 3 auto-fixed (1 blocking, 2 missing critical)
**Impact on plan:** Two files beyond the eleven the plan lists (`src/cpu.c`, `docs/hardware/cpu.md`), both one-line scope; no change to the design.

## Issues Encountered

None. Every case passed on its first run; mutants of the preemption, the RFE re-arm, the load landing, the sign mirror and the replacement rule each failed the case that pins them, which shows the tests are sensitive.

## Known Stubs

None. The COP2 hook is a deliberate seam, documented on the COP0 page and in the header, whose double-execute behaviour is the GTE plan's.

## Next Phase Readiness

- Interrupts now reach the CPU; the frame driver's vblank raise (`schway__irq_raise`) is a real source, so plan 02-21's traced copy of the loop and the `exc_matrix` rows of plan 02-25 can use the same edge.
- The doc rows are open to a dated line in `tests/diag/allow.txt` when evidence arrives; the file holds only its format comment.
- No blockers.

## Self-Check: PASSED

---
*Phase: 02-the-cpu-matches-the-public-vectors*
*Completed: 2026-11-09*
