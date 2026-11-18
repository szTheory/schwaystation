---
phase: 02-the-cpu-matches-the-public-vectors
plan: 10
subsystem: cpu
tags: [cpu, r3000, cop0, exceptions, branches, delay-slots, vectors, first-fault]

requires:
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: the vector harness, the shared executor and the delay latches (plan 02-06)
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: the bus with segment decode, the first-fault record and the interrupt registers (plan 02-07)
provides:
  - schway__cop0_exception, the one exception entry (KU/IE push, ExcCode, BD, BT, CE, EPC, TAR, BadVaddr, vector, discarded branch record)
  - ADD, ADDI, SUB overflow traps, SYSCALL, BREAK, every branch and jump with the delay record, JR/JALR misaligned AdEL at the fetch
  - MFC0 through the load delay, MTC0 with the SR and CAUSE masks, RFE, reserved-instruction exceptions, COP1/COP3 CpU and BCzF/BCzT
  - the exception-loop counter in the cpu state (exc_epc, exc_code, exc_repeat)
  - 35 vector files, 3,500 cases passing with no allow line
  - fifteen cpu.cop0 unit cases and docs/hardware/cop0.md
affects: [02-11, 02-12, 02-13, 02-14, 02-17]

actuals:
  tokens: 17500
  tasks: 3
  commits: 3

plan_head_before: 8ce25bbfe7c4ce7d1f4a67f3b3dbf50d429c247b
plan_head_after: 5c34e99612b01ec585bb9c15c6cde8493dea2af3
commits: 3

tech-stack:
  added: []
  patterns:
    - "one exception entry in cop0.c called by the executor helper cpu_exception; the program counter lands on the vector and cpu_step skips its own pc update through a trapped flag"
    - "cpu_step builds the branch record and the load it issues in locals and writes both once at the end, so exceptions leave them empty and a later load plan reuses the same slot"
    - "a unit executable runs one named case, places words in RAM through the bus and steps with schway__cpu_step_one and no log"

key-files:
  created:
    - tests/cpu/cop0_tests.c
    - docs/hardware/cop0.md
  modified:
    - src/cpu.h
    - src/cpu.c
    - src/cpu_exec.inc
    - src/cop0.c
    - src/internal.h
    - tests/vectors/tests.cmake
    - tests/cpu/tests.cmake
    - tests/cpu/inventory.txt
    - docs/hardware/cpu.md

key-decisions:
  - "The branch base is the address the delay slot goes to next (pc + 4, or the pending target), so a branch in a taken branch's delay slot and the JAL/JALR/BxxAL return address follow the same rule the vectors show; all 1,400 new cases matched on the first full run"
  - "BCondZ links only for field values 10h and 11h and every other value acts as BLTZ (even) or BGEZ (odd); the vectors do not distinguish this from the alternative"
  - "MTC0 ignores writes to TAR, BadVaddr, EPC and PRID (PSX-SYSTEM table marks them read-only); this is a documented fact the vectors do not cover, pinned by cause_writable"
  - "MFC0 into r0 is a pending load that is discarded when it lands, not a reserved-instruction exception"
  - "COP0 instructions in user mode with CU0 clear raise CpU with CE 0 and no first fault, since a CpU on COP0 is an architectural exception"
  - "The cpu layout assertion pins offsetof(exc_repeat) == 296 instead of offsetof(log), because the pointer after a 300-byte state part is aligned to 304 on 64-bit targets only"
  - "The exception-loop counter counts SYSCALL, BREAK and overflow as well; only a loop is a fault, never one such exception"

requirements-completed: [CPU-01, CPU-02]

coverage:
  - id: D1
    description: "schway__cop0_exception is the one exception entry: BD, BT, TAR for a delay slot with EPC at the branch (a not-taken branch gives BD 1, BT 0 and TAR the fall-through), EPC the instruction otherwise, the SR KU/IE stack pushed, the pending branch discarded, CE only for CpU, BadVaddr only for address errors, vector BFC00180h with BEV set and 80000080h otherwise"
    requirement: "CPU-02"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^cpu\\.cop0\\.(entry_delay_slot|entry_plain|ce_only_cpu|bev_vectors)$' --no-tests=error (exit 0)"
        status: pass
      - kind: integration
        ref: "build/ci/tests/schwaystation-test-vectors tests/vectors/r3000/sst-sample.bin --only SYSCALL --expect 100 (exit 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The vector sample passes the ALU class plus ADD, ADDI, SUB, SYSCALL, BREAK, BCondZ, BEQ, BGTZ, BLEZ, BNE, J, JAL, JALR and JR, 35 files and 3,500 cases, with no allow line"
    requirement: "CPU-01"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -L vectors --no-tests=error (exit 0, 11 tests)"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci (exit 0, 129 tests)"
        status: pass
    human_judgment: false
  - id: D3
    description: "MFC0 has one delay slot through the load latch; MTC0 writes SR through the mask F04FFF3Fh and CAUSE bits 8-9 only and ignores the read-only registers; RFE shifts SR bits 2-5 into 0-3; MFC0 and MTC0 of registers 0-2, 4 and 10 raise RI"
    requirement: "CPU-02"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^cpu\\.cop0\\.(mfc0_delay|sr_mask|cause_writable|rfe|reserved_instruction)$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D4
    description: "COP1 and COP3 with CU clear raise CpU with CE 1 or 3 and latch cop_unusable; a reserved instruction raises RI and latches reserved_instruction; with CUz set BCzF always goes and BCzT never does; a COP0 CpU latches nothing"
    requirement: "CPU-02"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^cpu\\.cop0\\.(cop1_cop3_unusable|cop_branches|reserved_instruction|no_fault_for_traps)$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D5
    description: "The same EPC and ExcCode 1,000 consecutive times latches exception_loop with the count in the note; a different EPC or ExcCode resets the counter; ExcCode 0 is excluded; SYSCALL, BREAK and overflow latch no first fault on their own"
    requirement: "CPU-02"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^cpu\\.cop0\\.(exception_loop|no_fault_for_traps)$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D6
    description: "JR and JALR to a misaligned target run the delay slot and then raise AdEL at the fetch with EPC and BadVaddr the bad target; BLTZAL and BGEZAL write the return address whether or not the branch goes"
    requirement: "CPU-02"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^cpu\\.cop0\\.(jr_misaligned|branch_links)$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D7
    description: "docs/hardware/cop0.md covers registers, SR and CAUSE bits, entry rules, vectors, RFE, CE, BadVaddr and which exceptions are a first fault, with each fact cited and no planning ID"
    requirement: "CPU-02"
    verification:
      - kind: other
        ref: "grep -qE 'D-[0-9]{2}|CPU-0[0-9]|PX\\.[0-9]|DEC\\.[0-9]' docs/hardware/cop0.md tests/cpu/cop0_tests.c (exit 1, no match)"
        status: pass
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^docs\\.budgets$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false

duration: 13min
completed: 2026-11-09
status: complete
---

# Phase 2 Plan 10: COP0, the exception entry, branches and jumps Summary

**One `schway__cop0_exception` entry feeds overflow, SYSCALL, BREAK, reserved-instruction, coprocessor-unusable and address-error exceptions; every branch and jump sets the delay record; 3,500 of the 5,500 public vector cases pass with no allow line, and fifteen unit cases pin the entry rules the vectors cannot.**

## Performance

- **Duration:** 13 min
- **Started:** 2026-11-09T20:26:00Z (reading and planning from about 20:26; first command 20:31:35Z)
- **Completed:** 2026-11-09T20:39:27Z
- **Tasks:** 3
- **Files modified:** 11 (2 created, 9 modified)

## Accomplishments

- `src/cop0.c` holds the exception entry: it pushes the SR mode stack (bits 0-5 left by 2), writes ExcCode, CE, BT and BD into CAUSE without touching the interrupt bits, sets EPC and, for a delay slot, TAR, writes BadVaddr only on request, empties the branch record and moves the program counter to `BFC00180h` or `80000080h` by BEV. The exception-loop counter lives in the same function.
- `src/cpu_exec.inc` decodes ADD, ADDI and SUB with explicit signed-overflow tests, BREAK, BCondZ, BEQ, BNE, BLEZ, BGTZ, J, JAL, JR and JALR with the branch record, MFC0, MTC0 and RFE, COP1 and COP3, and turns every unknown encoding into a real RI exception. `cpu_step` builds the branch record and the load it issues in locals and writes them once at the end.
- A misaligned program counter raises AdEL at the fetch with EPC and BadVaddr the address and a `misaligned_fetch` first fault, so JR and JALR to an odd target fault on the right instruction.
- The vector sample now runs 35 files (3,500 cases) and all pass on the first full run; no `allow.txt` line was needed.
- `tests/cpu/cop0_tests.c` has fifteen `cpu.cop0.*` cases; four mutants of `src/cop0.c` (mode-stack shift, SR mask, loop EPC comparison, BD bit position) were each caught by the intended case.
- `docs/hardware/cop0.md` is the COP0 page; `docs/hardware/cpu.md` now describes the trapping adds, branches and jumps.

## Task Commits

1. **Task 1: Tracer - SYSCALL takes the exception through one entry path and its vector file passes** - `f7408be` (feat)
2. **Task 2: Overflow, BREAK, branches, jumps, MFC0/MTC0/RFE, RI, COP1/COP3 and the exception loop** - `bc07dcb` (feat)
3. **Task 3: COP0 unit tests and the COP0 page** - `5c34e99` (test)

**Plan metadata:** the docs commit that adds this summary, STATE.md and ROADMAP.md follows.

The tracer feedback gate ran at end-of-phase mode with an automated-only verify: `cmake --workflow --preset ci` and `ctest -R '^vectors\.r3000\.sample$'` exited 0 (2,200 cases) before Task 2 started, so expansion went ahead.

## Files Created/Modified

- `src/cop0.c` - exception entry, loop counter, MFC0, MTC0, RFE
- `src/cpu_exec.inc` - branch and jump decode, trapping adds, COP0 and COP1/COP3 decode, exception helpers, the single end-of-step write of the load and branch record
- `src/cpu.h` - ExcCode and SR bit constants, the three exception-loop fields, the new declarations
- `src/cpu.c` - reset and state walk of the three fields
- `src/internal.h` - the layout assertion pins `exc_repeat` at 296
- `tests/vectors/tests.cmake` - the exception and branch file list, `--expect 3500`
- `tests/cpu/cop0_tests.c`, `tests/cpu/tests.cmake`, `tests/cpu/inventory.txt` - the fifteen cases
- `docs/hardware/cop0.md` - the COP0 page; `docs/hardware/cpu.md` - branches, jumps and trapping adds

## allow.txt lines

None were needed: every one of the 3,500 sample cases passes, so `tests/vectors/r3000/allow.txt` is unchanged.

## Decisions Made

See `key-decisions`. The state format number and behaviour revision stay at 2: the three new fields extend a state format introduced in this same phase that has not been released.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] The cpu layout assertion cannot pin `offsetof(log)` once the state part is 300 bytes**
- **Found during:** Task 2
- **Issue:** Adding three u32 fields makes the state part 300 bytes, and the non-state `log` pointer after it is aligned to 304 on 64-bit and sits at 300 on 32-bit, so `offsetof(log) == 300` fails to compile on the host.
- **Fix:** The assertion is now `offsetof(struct schway__cpu, exc_repeat) == 296`, which pins the state part exactly on every target.
- **Files modified:** `src/internal.h`
- **Verification:** `cmake --workflow --preset ci` and `dev` exit 0; the 32-bit lanes run in hosted CI
- **Committed in:** `bc07dcb`

**2. [Rule 2 - Missing Critical] COP0 instructions in user mode with CU0 clear raise CpU**
- **Found during:** Task 2
- **Issue:** The plan's prohibition names a CpU on COP0 as an exception that must not latch a first fault, but no truth said when COP0 raises it; without a check a user-mode guest could read and write SR.
- **Fix:** MFC0, MTC0 and RFE raise CpU with CE 0 and no first fault when SR.KUc is 1 and SR.CU0 is 0 (psx-spx SR bits). `cop0.ce_only_cpu` and `cop0.no_fault_for_traps` pin it.
- **Files modified:** `src/cpu_exec.inc`, `tests/cpu/cop0_tests.c`
- **Committed in:** `bc07dcb`, `5c34e99`

**3. [Rule 2 - Missing Critical] Three unit cases beyond the plan's twelve**
- **Found during:** Task 3
- **Issue:** Four of the plan's truths (BCzF/BCzT with CUz set, BLTZAL/BGEZAL link, JR/JALR misaligned AdEL) had no named case in the plan's list, and the vectors cannot reach the first and third (single instruction, no following fetch).
- **Fix:** Added `cop_branches`, `branch_links` and `jr_misaligned`; the acceptance count (at least 12) still holds, at 15.
- **Files modified:** `tests/cpu/cop0_tests.c`, `tests/cpu/tests.cmake`, `tests/cpu/inventory.txt`
- **Committed in:** `5c34e99`

**4. [Rule 2 - Missing Critical] `docs/hardware/cpu.md` was stale after this plan**
- **Found during:** Task 3
- **Issue:** The page said an undecoded encoding retires as a no-op, that the trapping adds "come with exceptions" and that a branch was "once decoded"; all three became false. The page is not in the plan's file list.
- **Fix:** Updated those statements and added a short Branches and jumps section that points at the new page.
- **Files modified:** `docs/hardware/cpu.md`
- **Committed in:** `5c34e99`

---

**Total deviations:** 4 auto-fixed (1 blocking, 3 missing critical).
**Impact on plan:** All four are small and local; every artifact, symbol and test the plan names is present.

## Issues Encountered

- `cmake --workflow --preset asan` needs `clang-21`, which this host does not have, so the sanitizer check ran as a one-off: `cc -fsanitize=address,undefined -fno-sanitize-recover=undefined` on `tests/cpu/cop0_tests.c` with `src/*.c` (all fifteen cases exit 0) and on `tests/vectors/vectors.c` with `src/*.c` (1,400 new cases, 0 failures).
- The first draft of two unit cases left a taken branch's latch pending into the next case (`branch_links`) and used RFE in user mode (`rfe`); both were test bugs, fixed before the Task 3 commit.

## Known Stubs

- `src/cpu_exec.inc`: opcodes the executor does not decode yet (loads, stores, multiply, divide, HI/LO moves, COP2, LWCz and SWCz) now raise a real reserved-instruction exception and latch a first fault, instead of retiring as a no-op. Plans 02-11 and 02-12 decode them. Nothing reaches this path from a host until the run loop lands, and the vector runs select decoded files only.

## Threat Flags

None - the plan adds CPU-internal state and tests; no network, auth or file surface, and no trust boundary changes.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plans 02-11 onward can issue loads through `load_reg` and `load_value` in `cpu_step`: the end-of-step write is already in place and MFC0 uses it; a load that faults calls `cpu_exception` with `SCHWAY__EXC_ADEL` or `SCHWAY__EXC_ADES` and a BadVaddr.
- Plan 02-12 adds COP2 and the remaining files to the vector list; the harness list and `--expect` are the only edit there.
- The interrupt plan reaches the same entry with `SCHWAY__EXC_INT` and EPC at the instruction that would have run next.
- Not covered by any test yet, and left for the plans that own them: AdEL for a user-mode access outside KUSEG, the DBE/IBE entries from the bus, and the double fault in an exception vector.

---
*Phase: 02-the-cpu-matches-the-public-vectors*
*Completed: 2026-11-09*

## Self-Check: PASSED

- FOUND: src/cop0.c, src/cpu_exec.inc, src/cpu.h, src/cpu.c, tests/cpu/cop0_tests.c, docs/hardware/cop0.md
- FOUND commits: f7408be, bc07dcb, 5c34e99 (ancestors of HEAD)
