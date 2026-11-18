---
phase: 02-the-cpu-matches-the-public-vectors
plan: 12
subsystem: cpu
tags: [cpu, r3000, loads, stores, load-delay, mul-div, cop2, vectors, harness-rules]

requires:
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: COP0, the one exception entry, branches and the load latch slot in cpu_step (plan 02-10)
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: the bus with segment decode, the first-fault record and unbuilt-device counters (plan 02-07)
provides:
  - cpu_data_check, cpu_bus_read and cpu_bus_write, the data access front-ends (alignment, user-mode segment, bus error to DBE)
  - LB, LBU, LH, LHU, LW, LWL, LWR, SB, SH, SW, SWL, SWR through the load-delay latch
  - MULT, MULTU, DIV, DIVU, MFHI, MFLO, MTHI, MTLO with cpu.muldiv_ready and cpu.cycle in the state
  - COP2, LWC2 and SWC2 before the GTE, with the is_cop2 decode flag; LWC1, SWC1, LWC3 and SWC3 follow the COP1/COP3 rule
  - all 55 vector files, 5,500 cases passing with no allow line, rule (a) on 310 cases and rule (b) on 400
  - cpu.cop2_prephase.cu2_clear and cpu.cop2_prephase.cu2_set, and the loads, stores, mul/div and COP2 sections of docs/hardware/cpu.md
affects: [02-13, 02-14, 02-17, 02-19, 02-25]

actuals:
  tokens: 11800
  tasks: 3
  commits: 3

plan_head_before: ff8f5529e092e97a9034968365735b5d3207b092
plan_head_after: 9d369bb7a0c5233870dca75da8e1746f89e078d2
commits: 3

tech-stack:
  added: []
  patterns:
    - "the data front-end is three small functions: cpu_data_check (alignment then segment), cpu_bus_read and cpu_bus_write (bus error to DBE); every load and store is check then bus then latch"
    - "cpu_load_issue is the one way a load enters the latch and drops a pending load to the same register; MFC0, MFC2, CFC2 and every load use it"
    - "a harness rule is a named function with a counter, never an allow line: the run prints how many cases each rule handled"
    - "the 64-bit cpu fields come first in the struct, so the layout assertion holds on every ABI"

key-files:
  created:
    - tests/cpu/cop2_prephase.c
  modified:
    - src/cpu.h
    - src/cpu.c
    - src/cpu_exec.inc
    - src/internal.h
    - tests/vectors/vectors.c
    - tests/vectors/tests.cmake
    - tests/vectors/inventory.txt
    - tests/cpu/cop0_tests.c
    - tests/cpu/tests.cmake
    - tests/cpu/inventory.txt
    - docs/hardware/cpu.md

key-decisions:
  - "The upstream vectors do raise AdEL and AdES for a misaligned access; they differ from the core only in the CAUSE CE field, which the upstream interpreter fills from instruction bits 27 and 26. Rule (a) therefore asserts the exception (vector 80000080h, ExcCode, BadVaddr, fetch-only bus) and then compares every other final field with CE left out, which is stronger than the planned exception-only check and needs no allow line; it handles exactly 310 cases"
  - "LWL and LWR read the whole aligned word in one bus access, as the hardware does, and SWL and SWR write the covered bytes as the largest aligned chunks; rule (b) coalesces the upstream chunks to the covered byte set, checks it against the set the instruction names, replays a load as one word read and coalesces the core's store chunks the same way, so no bus-hook field and no flag in the shared path was needed (400 cases)"
  - "A multiply or divide issued at counter value t is ready at t plus 6, 9, 13 or 36; MFHI and MFLO issued before the stamp wait for it, capped at 36 cycles so a hostile loaded state cannot make one instruction unbounded; MTHI and MTLO set the stamp to 0"
  - "A user-mode access outside KUSEG raises AdEL or AdES with BadVaddr and records no first fault, as a handler may take it; a misaligned access records misaligned_read or misaligned_write"
  - "A GTE access before the GTE exists is counted with an address that names the register: the data register number for MFC2 and MTC2, 32 plus the control register number for CFC2 and CTC2, the memory address for LWC2 and SWC2, the command word for a command"
  - "LWC1, SWC1, LWC3 and SWC3 follow the COP1/COP3 rule (CpU with CE 1 or 3 and a first fault when the bit is clear, nothing when set) instead of the reserved-instruction exception the previous plan left for them"
  - "The cpu layout assertion pins exc_repeat at 312 because the two 64-bit fields moved to the front of the struct; the state format number stays 2, the new fields extend a format that has not been released"

requirements-completed: [CPU-01, CPU-02]

coverage:
  - id: D1
    description: "All 55 files of the committed sample, 5,500 cases, pass with no allow line; the harness runs the whole file with expect 5500, and a run that parses fewer cases, or is asked for 5501, exits 1"
    requirement: "CPU-01"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -L vectors --no-tests=error (exit 0, 13 tests)"
        status: pass
      - kind: integration
        ref: "build/ci/tests/schwaystation-test-vectors tests/vectors/r3000/sst-sample.bin --expect 5500 --allow tests/vectors/r3000/allow.txt (exit 0)"
        status: pass
      - kind: integration
        ref: "build/ci/tests/schwaystation-test-vectors tests/vectors/r3000/sst-sample.bin --expect 5501 (exit 1)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The access front-ends check alignment and the user-mode segment before the bus, raise AdEL or AdES with BadVaddr and latch misaligned_read or misaligned_write, and turn a bus error into DBE with no BadVaddr; LWL, LWR, SWL and SWR never fault on alignment"
    requirement: "CPU-01"
    verification:
      - kind: integration
        ref: "tests/vectors/vectors.c rule_misaligned over the 310 misaligned cases of the sample (vectors.r3000.sample, exit 0)"
        status: pass
      - kind: unit
        ref: "tests/cpu/cop2_prephase.c#cu2_set (LWC2 of a bus gap raises ExcCode 7 with no BadVaddr; odd LWC2 and SWC2 raise AdEL and AdES)"
        status: pass
    human_judgment: false
  - id: D3
    description: "Harness rule (a) for LH, LHU, LW, SH and SW and rule (b) for LWL, LWR, SWL and SWR are named functions with counters, neither is an allow line or a flag in the executor, and each has a negative control"
    requirement: "CPU-01"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^vectors\\.r3000\\.rule_(misaligned|unaligned)\\.self_test$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D4
    description: "The load-delay latch holds one pending load; a write by the next instruction cancels it, a later load to the same register replaces it, a failed load leaves rt untouched, LWL and LWR merge from a pending load of the same register, and MFC0, MFC2 and CFC2 share the latch"
    requirement: "CPU-02"
    verification:
      - kind: integration
        ref: "vectors.r3000.sample over LB, LBU, LH, LHU, LW, LWL and LWR (exit 0)"
        status: pass
      - kind: unit
        ref: "tests/cpu/cop2_prephase.c#cu2_set (MFC2 and CFC2 land 0 one instruction later) and tests/cpu/cop0_tests.c#mfc0_delay"
        status: pass
    human_judgment: false
  - id: D5
    description: "MULT, MULTU, DIV and DIVU give the documented results including division by zero and the most negative number by -1; the ready stamp, the 6, 9, 13 and 36 cycle classes, the early-read wait, replacement and MTHI/MTLO clearing are implemented and documented, with the timing pinned by the timing tests of a later plan"
    requirement: "CPU-02"
    verification:
      - kind: integration
        ref: "vectors.r3000.sample over MULT, MULTU, DIV, DIVU, MFHI, MFLO, MTHI and MTLO (exit 0)"
        status: pass
    human_judgment: false
  - id: D6
    description: "With SR.CU2 clear every COP2 form raises CpU with CE 2 and latches no fault (alignment first for LWC2 and SWC2); with it set MFC2 and CFC2 give 0 through the latch, MTC2, CTC2 and commands do nothing, BC2F goes and BC2T does not, LWC2 does the real bus read and SWC2 the real write of 0, each counted as an unbuilt dev=gte access"
    requirement: "CPU-02"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^cpu\\.cop2_prephase\\.(cu2_clear|cu2_set)$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D7
    description: "docs/hardware/cpu.md covers loads, stores, the front-end, the latch and interlock, mul/div stamps, the COP2 pre-phase and the two harness rules with their case counts, with no planning ID"
    requirement: "CPU-01"
    verification:
      - kind: other
        ref: "grep -qE 'D-[0-9]{2}|CPU-0[0-9]|PX\\.[0-9]' docs/hardware/cpu.md (exit 1, no match)"
        status: pass
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^docs\\.budgets$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false

duration: 14min
completed: 2026-11-09
status: complete
---

# Phase 2 Plan 12: Loads, stores, mul/div and COP2 before the GTE Summary

**Every R3000A load and store through alignment, segment and bus checks into the load-delay latch, MULT/DIV with a ready stamp, and COP2 before the GTE, so all 5,500 public vector cases pass with two named harness rules (310 and 400 cases) and no allow line.**

## Performance

- **Duration:** 14 min
- **Started:** 2026-11-09T20:52:20Z (the previous plan's last commit; the start time was not recorded)
- **Completed:** 2026-11-09T21:06:00Z
- **Tasks:** 3
- **Files modified:** 12 (1 created, 11 modified)

## Accomplishments

- `cpu_data_check`, `cpu_bus_read` and `cpu_bus_write` in `src/cpu_exec.inc` are the data front-ends: a misaligned half or word is AdEL or AdES with BadVaddr before any bus access, user mode outside KUSEG is the same error, and a bus error is DBE with no BadVaddr. The 12 load and store opcodes use them, and every load enters the latch through `cpu_load_issue`, which also drops a pending load to the same register.
- `MULT`, `MULTU`, `DIV`, `DIVU`, `MFHI`, `MFLO`, `MTHI` and `MTLO` with `cpu.muldiv_ready` and `cpu.cycle` in the saved state; a read before the stamp waits up to 36 cycles, a new operation replaces a running one, and `MTHI`/`MTLO` clear the stamp.
- The vector test runs the whole sample (`--expect 5500`, no file list): 5,500 cases, 0 failures, 310 handled by rule (a) and 400 by rule (b), no `allow.txt` line.
- COP2, LWC2 and SWC2 behave as documented before the GTE (`cpu_cop2`, dispatched by the `is_cop2` flag), covered by `cpu.cop2_prephase.cu2_clear` and `cu2_set`.
- `docs/hardware/cpu.md` gained Loads and stores, Multiply and divide and COP2 before the GTE sections and the two rules with their counts.

## Task Commits

1. **Task 1 (tracer): LW through the front-end, the bus and the latch passes its vector file** - `84e9f73` (feat)
2. **Task 2: every load and store, mul/div and HI/LO, the two harness rules, all 5,500** - `3455890` (feat)
3. **Task 3: COP2 before the GTE, its tests, and the CPU page sections** - `9d369bb` (feat)

**Plan metadata:** the docs commit that carries this summary, STATE.md and ROADMAP.md.

The tracer gate ran at end-of-phase mode with an automated-only verify: `cmake --workflow --preset ci` and `ctest -R '^vectors\.r3000\.sample$'` exited 0 (3,600 cases) before Task 2 started.

## Files Created/Modified

- `src/cpu_exec.inc` - front-ends, load, store, unaligned, mul/div and COP2 helpers, the new opcode cases and the cycle counter
- `src/cpu.h`, `src/cpu.c` - `muldiv_ready` and `cycle` (first in the struct), reset, state walk, `SCHWAY__MULDIV_MAX`
- `src/internal.h` - the layout assertion now pins `exc_repeat` at 312
- `tests/vectors/vectors.c` - rules (a) and (b), their counters, `--flip-expected FIELD@FILE`
- `tests/vectors/tests.cmake`, `tests/vectors/inventory.txt` - whole-sample run at 5,500, two new negative controls, controls re-pointed at the whole file
- `tests/cpu/cop2_prephase.c`, `tests/cpu/tests.cmake`, `tests/cpu/inventory.txt` - the two COP2 cases
- `tests/cpu/cop0_tests.c` - LWC1, SWC1, LWC3 and SWC3 added to `cop1_cop3_unusable`
- `docs/hardware/cpu.md` - the new sections

## allow.txt lines

None. All 5,500 cases pass; `tests/vectors/r3000/allow.txt` is unchanged.

## Harness rule case counts

- Rule (a), misaligned LH, LHU, LW, SH, SW: 310 cases (the same 310 the discussion measured).
- Rule (b), LWL, LWR, SWL, SWR: 400 cases (every case of the four files).

## Decisions Made

See `key-decisions`. The two later plans lean on: the timing rules (`muldiv_ready` is the counter value at which HI and LO are ready, counted from the issue) that plan 02-19's timing tests pin, and `cpu_data_check`, which the interrupt and exception tests of the same plan exercise.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] The cpu layout assertion in src/internal.h had to change**
- **Found during:** Task 2
- **Issue:** two 64-bit fields added to the cpu struct move `exc_repeat`; the assertion pinning it at 296 would fail to compile. `src/internal.h` is not in the plan's file list.
- **Fix:** the two fields go first in the struct, so their offsets do not depend on the target's alignment of a 64-bit integer, and the assertion pins `exc_repeat` at 312.
- **Files modified:** `src/internal.h`, `src/cpu.h`
- **Committed in:** `3455890`

**2. [Rule 3 - Blocking] tests/vectors/inventory.txt edited although the plan does not list it**
- **Found during:** Task 1
- **Issue:** the configure step refuses a registered test missing from the directory's inventory, and this plan adds two controls.
- **Fix:** two lines added in byte order.
- **Files modified:** `tests/vectors/inventory.txt`
- **Committed in:** `84e9f73`, `3455890`

**3. [Rule 2 - Missing critical] Rule (a) compares the whole final state, not only the exception**
- **Found during:** Task 1
- **Issue:** the plan words rule (a) as asserting the exception "instead of" the final-state comparison. The sample shows the upstream interpreter does raise the exception for these cases and differs from the core only in CAUSE bits 29 and 28, copied from instruction bits 27 and 26; an exception-only check would stop comparing registers, EPC, TAR and the latches for 310 cases.
- **Fix:** rule (a) asserts vector, ExcCode, BadVaddr and a fetch-only bus, then compares every final field with the CE bits masked.
- **Files modified:** `tests/vectors/vectors.c`
- **Committed in:** `84e9f73`, `3455890`

**4. [Rule 2 - Missing critical] The controls cannot name a file with the harness's file-list option, and two rule controls were added**
- **Found during:** Task 2
- **Issue:** the acceptance check requires the option string to be absent from `tests/vectors/tests.cmake`, but the existing flip and allow controls used it to reach a known first case. A misaligned first case for rule (a) was also needed.
- **Fix:** the controls run the whole file; `--flip-expected FIELD@FILE` flips the first case of the named upstream file; the allow controls name `ADD 000`; `vectors.r3000.rule_misaligned.self_test` and `vectors.r3000.rule_unaligned.self_test` show each rule still compares the final state.
- **Files modified:** `tests/vectors/vectors.c`, `tests/vectors/tests.cmake`, `tests/vectors/inventory.txt`
- **Committed in:** `3455890`

**5. [Rule 2 - Missing critical] LWC1, SWC1, LWC3 and SWC3 decoded as coprocessor instructions**
- **Found during:** Task 3
- **Issue:** the previous plan left them as reserved-instruction exceptions pending this plan, but a guest executing them with the bit clear must take CpU like any other COP1 or COP3 instruction.
- **Fix:** they share the COP1/COP3 case, which now picks the usable bit from the low bits of the opcode and restricts the branch decode to the two real coprocessor opcodes; `cpu.cop0.cop1_cop3_unusable` covers them. `tests/cpu/cop0_tests.c` is not in the plan's file list.
- **Files modified:** `src/cpu_exec.inc`, `tests/cpu/cop0_tests.c`
- **Committed in:** `9d369bb`

---

**Total deviations:** 5 auto-fixed (2 blocking, 3 missing critical).
**Impact on plan:** all five are small and local; every artifact, symbol and test the plan names exists. `src/cop0.c` needed no change.

## Issues Encountered

- `cmake --workflow --preset asan` and `nofp` cannot configure on this host (no `clang-21`, GCC needed for `-mgeneral-regs-only`), so the sanitizer check was a one-off: `cc -fsanitize=address,undefined -fno-sanitize-recover=undefined` on `tests/vectors/vectors.c`, `tests/cpu/cop2_prephase.c` and `tests/cpu/cop0_tests.c` with `src/*.c` all exit 0.
- The first COP2 test had a branch word with the offset in the wrong field (a test bug, fixed before the Task 3 commit); `cop_branches` failed once after an over-broad edit of the COP1/COP3 case and was fixed in the same task.

## Known Stubs

- `src/cpu_exec.inc` `cpu_cop2`: COP2 does nothing but the documented pre-phase behaviour; the GTE of phase 3 replaces it. This is the plan's stated deliverable, not a gap.
- The multiply and divide wait and the cycle counter are exercised by no test in this plan: the vectors do not carry the timing. The timing tests of plan 02-19 pin them. Nothing calls `cpu_step` from a run loop yet (plan 02-17), so no guest can observe the counter.

## Threat Flags

None - the plan adds CPU-internal behaviour and tests; no network, auth or file surface, and no trust boundary changes. The new state fields are bounded in use: the wait on a loaded `muldiv_ready` is capped at 36 cycles.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 02-13 onward can rely on every opcode decoding; the only RI encodings left are the genuinely reserved ones and LWC0/SWC0.
- Plan 02-17's run loop calls `cpu_step`, which now advances `cycle` by what each instruction cost, bus charges and read stalls included.
- Plan 02-19 pins the delay and timing rules (`delay_tests.c`, `timing_tests.c`); the semantics to pin are in `docs/hardware/cpu.md` under Multiply and divide.
- Not covered by a test yet and left to that plan: AdEL for a user-mode access outside KUSEG, DBE on an ordinary load or store (only LWC2 reaches it), and the double fault in an exception vector.

## Self-Check: PASSED

- FOUND: tests/cpu/cop2_prephase.c, src/cpu_exec.inc, docs/hardware/cpu.md
- FOUND commits: 84e9f73, 3455890, 9d369bb (ancestors of HEAD)
- `cmake --workflow --preset ci` exit 0 (142 tests); `cmake --workflow --preset hygiene` exit 0 (21 tests); `ctest -L vectors` exit 0 (13 tests); `ctest -L cpu` exit 0

---
*Phase: 02-the-cpu-matches-the-public-vectors*
*Completed: 2026-11-09*
