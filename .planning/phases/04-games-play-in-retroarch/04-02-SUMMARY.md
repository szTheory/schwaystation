---
phase: 04-games-play-in-retroarch
plan: "02"
subsystem: cpu-bus
tags: [r3000a, cop0, swl, swr, icache, save-state]

requires:
  - phase: 04-games-play-in-retroarch
    plan: "01"
    provides: [phase-4 test registration and CI inventory seams]
provides:
  - User-mode KSEG and BIOS fetches enter guest AdEL before bus access.
  - SWL/SWR RAM and MMIO lanes, write widths, and queue behavior are pinned through the CPU.
  - Failed later I-cache refill words remain unchanged and invalid.
  - Invalid r0, COP0 masks, PRID, and IRQ-derived CAUSE state are rejected before apply.
  - Repeated exceptions stop counting after a guest RFE returns past EPC.
affects: [gpu, firmware, cpu, bus, save-state]

actuals:
  tokens: 6758
  tasks: 3
  commits: 7
plan_head_before: 3c26e640c6493b69950d1fb411c454de41eebe70
plan_head_after: e199fbbe3f185028c3a3b0c439fdecdcdb3df0ac

tech-stack:
  added: []
  patterns:
    - Validate serialized cross-field invariants before applying state.
    - Read cache refill words into temporaries and publish only successful reads.

key-files:
  created: []
  modified:
    - src/cpu_exec.inc
    - src/bus.c
    - tests/cpu/exception_tests.c
    - tests/bus/bus_tests.c
    - docs/hardware/bus.md
    - src/cpu.c
    - tests/core/test_state.c
    - docs/hardware/cpu.md
    - src/cop0.c
    - docs/hardware/cop0.md
    - src/state.c
    - src/state.h
    - tests/cpu/cop0_tests.c

key-decisions:
  - User-mode kernel fetches reuse the existing data-access permission check so the fault stays guest-visible.
  - State validation checks Cause.IP2 against the serialized interrupt status and mask before application.
  - An RFE return in a taken branch delay slot counts as progress only when a non-interrupt handler targets beyond its trapped EPC.

patterns-established:
  - Failed refill words do not overwrite previously cached data.
  - TDD cases extend existing test registrations without changing inventories.

requirements-completed: []
coverage:
  - id: D1
    description: User-mode forbidden fetches and width-sensitive partial stores follow the CPU and bus contracts needed by GPU callers.
    requirement: GAME-01
    verification:
      - kind: unit
        ref: tests/cpu/exception_tests.c#ibe_unmapped_fetch via cmake --workflow --preset ci
        status: pass
      - kind: unit
        ref: tests/bus/bus_tests.c#ram_word via cmake --workflow --preset ci
        status: pass
    human_judgment: false
  - id: D2
    description: Cache refill failures preserve cached words, and malformed CPU state is rejected atomically while valid state reloads.
    requirement: GAME-05
    verification:
      - kind: unit
        ref: tests/bus/bus_tests.c#ram_word via cmake --workflow --preset ci
        status: pass
      - kind: unit
        ref: tests/core/test_state.c#state.rejects and tests/core/test_state.c#state.cpu_fields via cmake --workflow --preset ci
        status: pass
    human_judgment: false
  - id: D3
    description: A BIOS-style handler can return past EPC through 1001 repeated traps, while a same-EPC guest return still reaches the loop limit.
    requirement: GAME-05
    verification:
      - kind: unit
        ref: tests/cpu/exception_tests.c#syscall_epc via cmake --workflow --preset ci
        status: pass
      - kind: unit
        ref: tests/cpu/cop0_tests.c#exception_loop via cmake --workflow --preset ci
        status: pass
    human_judgment: false

duration: 32 min
completed: "2026-11-10"
status: complete
---

# Phase 04 Plan 02: CPU, Bus, and Exception Follow-ups Summary

**CPU fetch and partial-store contracts, I-cache refill recovery, atomic CPU-state validation, and progress-aware exception handling are implemented and covered by the registered CI suite.**

## Performance

- **Duration:** 32 min
- **Started:** 2026-11-10T17:43:00Z
- **Completed:** 2026-11-10T18:15:37Z
- **Tasks:** 3
- **Files modified:** 13

## Accomplishments

- User-mode KSEG0, KSEG1, and BIOS instruction fetches now raise guest AdEL before bus access. CPU-driven SWL/SWR tests pin all byte offsets against RAM and width-sensitive MMIO, including write-queue behavior.
- I-cache refill bus errors leave the failed word's old contents untouched and clear its valid bit. State loading rejects invalid r0, reserved SR bits, unsupported CAUSE bits, wrong PRID, and a CAUSE.IP2 value that disagrees with I_STAT/I_MASK, before changing the live instance.
- A non-interrupt handler returning through RFE to a taken branch target beyond EPC resets the repeated-exception count. A handler returning to EPC still trips the existing limit.

## Task Commits

1. **Trace a user-mode partial store and forbidden fetch through the real CPU/bus** - `5d7420d` (test), `28c8334` (feat)
2. **Preserve cache words and reject impossible CPU states** - `f4fa412` (test), `ef68ab1` (fix)
3. **Keep progressing BIOS exception handlers running** - `04e7110` (test), `5fc6bdc` (test), `e199fbb` (fix)

## Files Created/Modified

- `src/cpu_exec.inc`, `src/cpu.c`, and `src/cop0.c` implement user fetch checks, safe cache refills, CPU-state invariants, and progress-aware exception counting.
- `src/bus.c`, `src/state.c`, and `src/state.h` carry the serialized IRQ values through validation so Cause.IP2 consistency is checked before apply.
- `tests/cpu/exception_tests.c`, `tests/cpu/cop0_tests.c`, `tests/bus/bus_tests.c`, and `tests/core/test_state.c` cover the guest-visible and atomicity behavior without changing test inventories.
- `docs/hardware/bus.md`, `docs/hardware/cpu.md`, and `docs/hardware/cop0.md` describe the modeled bus, fetch, refill, state, and exception rules.

## Decisions Made

- The Cause.IP2 cross-field invariant is checked during the existing validate-then-apply walk, using validation-only scratch fields; serialized format and public APIs do not change.
- Exception progress is inferred only from an RFE in a taken branch delay slot targeting beyond the last trapped EPC. Interrupt returns do not reset the counter.
- Requirements remain incomplete: this plan closes CPU/bus prerequisites but does not deliver the full GPU or OpenBIOS behaviors named by GAME-01 and GAME-05.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Kept COP0 tests on valid instruction-fetch segments after enabling user-mode fetch faults**
- **Found during:** Task 1
- **Issue:** Four existing COP0 tests left the CPU in user mode and then placed their next instruction in KSEG0, so the new fetch check raised AdEL before the behavior under test.
- **Fix:** Moved those test instructions to KUSEG while preserving the intended COP0 assertions.
- **Files modified:** `tests/cpu/cop0_tests.c`
- **Verification:** Full `cmake --workflow --preset ci` passed after the adjustment.
- **Committed in:** `28c8334`

**2. [Rule 3 - Blocking issue] Carried serialized IRQ fields through validation for the planned Cause.IP2 invariant**
- **Found during:** Task 2
- **Issue:** CPU validation runs before IRQ fields are visited, so checking Cause.IP2 against saved I_STAT/I_MASK required a validation-only bridge without mutating the target instance.
- **Fix:** Added scratch fields to `schway__state_io`, captured the serialized IRQ values in `bus_state_io`, and checked their relationship in `body_io`. Updated the synthetic valid-state fixture to keep Cause.IP2 consistent with its enabled IRQ line.
- **Files modified:** `src/state.h`, `src/bus.c`, `src/state.c`, `tests/core/test_state.c`
- **Verification:** `core.state.rejects` and `core.state.cpu_fields` passed; the full CI workflow passed.
- **Committed in:** `ef68ab1`

**Total deviations:** 2 auto-fixed issues (Rule 1: 1; Rule 3: 1).
**Impact on plan:** The adjustments were required to preserve existing regression intent and enforce the planned pre-apply state invariant; no unrelated scope was added.

## Verification Evidence

- TDD RED for task 1: `cpu.exception.ibe_unmapped_fetch` failed on the missing user-mode fetch exception; the partial-store RAM/MMIO/queue checks already passed against existing behavior. The fetch failure report was accepted by `check tdd-red-evidence` as `RED_EVIDENCE_OK`. GREEN commit: `28c8334`.
- TDD RED for task 2: `bus.ram_word` and `core.state.rejects` failed because the cache word remained valid/was overwritten and malformed CPU state was accepted. Both reports were accepted as `RED_EVIDENCE_OK`. GREEN commit: `ef68ab1`.
- TDD RED for task 3: `cpu.exception.syscall_epc` failed because the progressing handler hit the 1,000-exception fault; the same-EPC handler assertions passed. The report was accepted as `RED_EVIDENCE_OK`. GREEN commit: `e199fbb`.
- `cmake --workflow --preset ci` — exit 0; all 339 tests passed, followed by both release archive checks.
- `cmake --workflow --preset hygiene` — exit 0; all 23 checks passed. An earlier run failed only because temporary TDD XML/JSON reports contained local absolute paths; the generated reports were removed before the passing run.
- Focused verification: `bus.ram_word`, `core.state.rejects`, `core.state.cpu_fields`, `cpu.exception.syscall_epc`, and `cpu.cop0.exception_loop` passed in the final code state.

## Issues Encountered

The first task-2 full CI run rejected the `core.state.cpu_fields` fixture because it directly enabled the IRQ mask without setting derived Cause.IP2. The fixture was corrected to model the state produced by the IRQ path, after which focused tests and full CI passed. The first task-3 test setup also omitted setting the guest PC to KUSEG; the test setup was corrected before the accepted RED run.

## User Setup Required

None.

## Next Phase Readiness

The CPU and bus contracts needed by the first GPU and firmware callers are covered. Plan 04-03 can proceed with GPU command decoding. No user action is needed.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-10*

## Self-Check: PASSED

All 10 plan files and the summary exist; task commits `5d7420d`, `28c8334`, `f4fa412`, `ef68ab1`, `04e7110`, `5fc6bdc`, and `e199fbb` are ancestors of HEAD; `git diff --check 3c26e64..HEAD` passed.
