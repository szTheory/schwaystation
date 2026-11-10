---
phase: 02-the-cpu-matches-the-public-vectors
plan: 17
subsystem: cpu
tags: [run-loop, blocks, stop-reasons, ps-exe, mailbox, stub-kernel, fixtures]

requires:
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: the own stub kernel mapped at BFC00000h and the committed array (plan 02-16), the write queue and cache (02-14), the frame_end state and schway_run (02-11), the launch block written by schway_load_exe (02-08)
provides:
  - schway__cpu_run and the cpu_block loop in src/cpu_exec.inc, with blocks, stop reasons and the budget split
  - the CPU frame mode: a loaded PS-EXE runs under the frame advance and the vertical sync raises I_STAT bit 0
  - IBE on a failed instruction fetch, and the double-fault stop at an exception vector
  - tests/fixtures/src/mailbox.inc (layout, header macro, bitwise CRC-32 seal), hello.exe reporting gp and sp, exc_default.exe and exc_handler.exe
  - fixtures.hello.boot and eight fixtures.boot.* cases
  - the kernel RAM table, reset sequence, mailbox layout, block rules and stop reasons on the CPU page
affects: [02-18, 02-19, 02-20, 02-21, 02-22, 02-23, 02-24, 02-25]

actuals:
  tokens: 15200
  tasks: 3
  commits: 3

plan_head_before: 778f1989889c1490b56d8e54e11ae94753e57723
plan_head_after: 6ede0b35e2b9d0bfb642efeb312749d6328ef91b
commits: 3

tech-stack:
  added: []
  patterns:
    - "the block loop lives in cpu_exec.inc beside cpu_step so the whole loop is what the traced copy of plan 02-21 compiles twice"
    - "a stop sits on an instruction boundary and carries no overshoot: ticks and audio pairs flush at block ends, the frame accumulator is independent of the overshoot"
    - "in-memory EXEs in boot_tests.c: a 2048-byte header and one sector, each code word written beside its mnemonic"

key-files:
  created:
    - tests/fixtures/src/mailbox.inc
    - tests/fixtures/src/exc_default.asm
  modified:
    - src/cpu.h
    - src/cpu.c
    - src/cpu_exec.inc
    - src/cop0.c
    - src/irq.c
    - src/frame.c
    - tests/fixtures/src/hello.asm
    - tests/fixtures/boot_tests.c
    - tests/fixtures/tests.cmake
    - tests/fixtures/inventory.txt
    - tests/api/v2.c
    - docs/hardware/cpu.md
    - docs/embedding.md

key-decisions:
  - "schway__cpu_run returns the stop reason; the cycles it ran are the advance of inst->ticks, which cpu_block adds once per block, and the frame advance computes the audio pairs from that delta"
  - "block_end is a non-state bit set (EVENT, DOUBLE_FAULT) cleared at the start of every block; every exception entry, RFE, MTC0 and I_STAT or I_MASK write sets EVENT"
  - "the exit port report consumes exit_pending (exit_code stays), so one write is one HOOK and the next run continues"
  - "with a program loaded the advance refuses, before running, a call within 1,024 cycles of the tick bound; one instruction takes far fewer"
  - "the mailbox layout follows the committed stub: CRC at 04h, status byte at 08h, version 09h, count 0Ah, id 0Bh, handler word 0Ch, records from 10h"

patterns-established:
  - "Fixture variants pass -definelabel through schway__fixture; an armips label is case-insensitive, so an option label never reuses a code label's name"

requirements-completed: [CPU-04, CPU-02]

coverage:
  - id: D1
    description: "schway__cpu_run runs blocks that end after a delay slot, an exception entry, RFE, a COP0 write, an I_STAT or I_MASK write, a page edge, 128 instructions, the exit port or a new first fault, and stops at the first instruction boundary at or after the cycle limit or the frame end with no overshoot in the state"
    requirement: "CPU-04"
    verification:
      - kind: unit
        ref: "tests/fixtures/boot_tests.c#split_with_exe, run as fixtures.boot.split_with_exe (exit 0); a mutant that clipped the ticks to the limit made it fail"
        status: pass
    human_judgment: false
  - id: D2
    description: "A run stops with HOOK and the exit byte when the exit port is written (once, the next run continues), with FAULT at the end of the instruction that latched the first fault under STOP_ON_FAULT, and with FAULT at once on a double fault whatever the flags"
    requirement: "CPU-04"
    verification:
      - kind: unit
        ref: "tests/fixtures/boot_tests.c#exit_port, #stop_on_fault and #double_fault, run as fixtures.boot.exit_port, fixtures.boot.stop_on_fault and fixtures.boot.double_fault (exit 0); mutants that kept exit_pending and that dropped the double-fault flag each failed their case"
        status: pass
    human_judgment: false
  - id: D3
    description: "A bus error on an instruction fetch raises IBE with no BadVaddr and the first fault names the device, so a jump past the stub image is recorded and never slides through no-operations"
    requirement: "CPU-04"
    verification:
      - kind: unit
        ref: "tests/fixtures/boot_tests.c#past_stub, run as fixtures.boot.past_stub (exit 0); a mutant that ignored the fetch error failed it and fixtures.boot.double_fault"
        status: pass
    human_judgment: false
  - id: D4
    description: "With an EXE loaded the frame advance runs the CPU to frame_end, raises I_STAT bit 0 at the vertical sync that ends the frame and reports HOOK and exit_code; with none loaded the CPU never runs and the test card hashes are unchanged"
    requirement: "CPU-04"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^(fixtures\\.hello\\.boot|runner\\.hash|vectors\\.r3000\\.sample)$' --no-tests=error (exit 0)"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci (exit 0, 164 of 164 tests)"
        status: pass
    human_judgment: false
  - id: D5
    description: "hello.exe boots from reset through the stub and seals its mailbox with SWMB, status 00h, a valid CRC-32 over 10h-FFh, record 1 equal to gp0 and record 2 equal to s_addr plus s_size, read from the file's own header"
    requirement: "CPU-04"
    verification:
      - kind: integration
        ref: "tests/fixtures/boot_tests.c#hello_boot, run as fixtures.hello.boot (exit 0)"
        status: pass
    human_judgment: false
  - id: D6
    description: "exc_default.exe pins the stub's default handler (Cause 24h, EPC the BREAK, BadVaddr and SR zero at 80000210h, status FFh) and exc_handler.exe its registered-handler path (the handler's record, a sealed mailbox, status 00h)"
    requirement: "CPU-04"
    verification:
      - kind: integration
        ref: "tests/fixtures/boot_tests.c#default_handler and #registered_handler, run as fixtures.boot.default_handler and fixtures.boot.registered_handler (exit 0)"
        status: pass
    human_judgment: false
  - id: D7
    description: "A hostile or looping EXE returns from every call: a jump to itself and a loop that takes a bus error on every pass each run whole frames and report FRAME"
    requirement: "CPU-04"
    verification:
      - kind: unit
        ref: "tests/fixtures/boot_tests.c#bounded, run as fixtures.boot.bounded (exit 0)"
        status: pass
    human_judgment: false
  - id: D8
    description: "docs/hardware/cpu.md holds the kernel RAM table of disjoint addresses, the reset sequence, the default handler, the mailbox layout, the block rules, where a block splits at a budget and the stop reasons"
    requirement: "CPU-04"
    verification:
      - kind: other
        ref: "grep -q 80000210 docs/hardware/cpu.md && grep -q 80000200 docs/hardware/cpu.md (exit 0); ctest --test-dir build/ci -R '^docs\\.' --no-tests=error (exit 0); cmake -DROOT=. -P tests/cmake/budgets.cmake (exit 0)"
        status: pass
    human_judgment: false

duration: 12min
completed: 2026-11-09
status: complete
---

# Phase 2 Plan 17: The run loop, stop reasons and hello.exe booting Summary

**A block-based CPU run loop (`schway__cpu_run`, `cpu_block`) that stops on instruction boundaries with HOOK, FAULT and double-fault reasons, wired into the frame advance with the vsync I_STAT edge, so hello.exe boots through the own stub and seals its mailbox with gp and sp records.**

## Performance

- **Duration:** 12 min (from plan 02-16's last commit to this plan's docs commit)
- **Started:** 2026-11-09T21:45:32Z
- **Completed:** 2026-11-09T21:57:46Z
- **Tasks:** 3
- **Files modified:** 15 (2 created, 13 modified, plus this summary)

## Accomplishments

- `cpu_block` and `schway__cpu_run` run real instructions for a loaded PS-EXE. Blocks end where the plan says, and a stop sits on the first instruction boundary at or after the limit or the frame end, so running T cycles and then B equals running T + B when T is where a run of B stopped. The divide loop in `fixtures.boot.split_with_exe` makes T exceed B, so the test exercises a real overshoot, and a mutant that clipped it fails.
- The stop reasons are tested one by one: HOOK once per exit-port write with the byte in `exit_code`, FAULT at the end of the instruction that latched a first fault, FAULT at once for a fetch that fails at an exception vector whatever the flags, and FRAME for two hostile loops (one taking a bus error on every pass).
- `hello.exe` runs from reset through the stub and reports `gp` and `sp` in a mailbox it seals with a bitwise CRC-32 that the test checks against the core's own CRC-32 routine, so the two agree only if both are right. The gp0 and `s_addr + s_size` values the test expects come from the file's own header.
- The test card hashes and the vector sample are unchanged (`runner.hash`, `vectors.r3000.sample`, the whole ci workflow at 164 of 164).

## Task Commits

1. **Task 1: Tracer - hello.exe boots through the stub and reports done in its mailbox** - `8474d43` (feat)
2. **Task 2: Stop reasons, the default handler, bounded hostile runs and the split with an EXE** - `51edc6f` (test)
3. **Task 3: The kernel layout and run loop on the CPU page** - `6ede0b3` (docs)

**Plan metadata:** the summary commit and the state commit that follow this file.

## Files Created/Modified

- `src/cpu_exec.inc` - `cpu_block`, and `cpu_fetch` reporting a bus error so `cpu_step` raises IBE (and flags a double fault at a vector)
- `src/cpu.c`, `src/cpu.h` - `schway__cpu_run`, the non-state `block_end` field, the vector, block and instruction-bound constants
- `src/cop0.c`, `src/irq.c` - set `block_end` on an exception entry, RFE, MTC0 and an I_STAT or I_MASK write; the vector addresses moved to `cpu.h`
- `src/frame.c` - the advance runs the CPU while `exe_loaded`, raises I_STAT bit 0 at the frame end, reports HOOK and `exit_code`
- `tests/fixtures/src/mailbox.inc`, `hello.asm`, `exc_default.asm` - the shared mailbox layout, the seal macro, and the three images
- `tests/fixtures/boot_tests.c`, `tests.cmake`, `inventory.txt` - the nine cases and their registration
- `tests/api/v2.c` - the budgeted-run check accepts the first boundary at or after the budget
- `docs/hardware/cpu.md`, `docs/embedding.md` - the kernel layout and run loop; the HOOK and FAULT rows and the boundary rule

## Decisions Made

- **Return shape.** `schway__cpu_run(inst, cycles, flags)` returns the stop reason and the cycles it ran are the advance of `inst->ticks`; the plan named only three parameters and this keeps them.
- **Ticks at block ends.** A first fault latched inside a block carries the ticks of the block's start (documented). Accurate per-instruction ticks cost nothing today, but the plan's seam is block-end flushing for device events.
- **The exit write is consumed.** `exit_pending` is cleared when HOOK is reported and `exit_code` stays, so a host that ignores the hook keeps running.
- **No exe, no vsync bit.** I_STAT bit 0 is raised only with an EXE loaded, so the no-program machine's state is byte-for-byte what it was.
- **Mailbox layout.** The committed stub writes the word `000000FFh` at 08h and reads the handler at 0Ch, so the layout puts the CRC at 04h and the status byte at 08h (see Deviations). `docs/hardware/cpu.md` is the one place that lists it for plan 02-23's runner.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] abi.api_v2 expected a budget to run exactly**
- **Found during:** Task 1 (the first ci workflow after the loop landed)
- **Issue:** `tests/api/v2.c` loads a PS-EXE and asserts `ticks + 1000` after a 1000-cycle budget. That held only while no instruction ran. With the CPU running, a stop is the first instruction boundary at or after the budget (1133417 against 1133409 expected).
- **Fix:** the check now accepts `ticks + 1000` up to `ticks + 1064`, with a comment saying why.
- **Files modified:** tests/api/v2.c
- **Verification:** `cmake --workflow --preset ci` exits 0, 164 of 164 tests
- **Committed in:** 8474d43

**2. [Rule 2 - Missing Critical] A failed instruction fetch was executed as a no-operation**
- **Found during:** Task 1 (the double-fault and past-stub truths)
- **Issue:** `cpu_fetch` ignored the bus's error and executed the word 0, so a jump past the stub image slid on as no-operations and an IBE could never be raised, let alone detected at a vector. ARCHITECTURE section 6 and the CPU page both say it is IBE.
- **Fix:** the fetch reports the error, `cpu_step` takes IBE (no BadVaddr) and, at BFC00180h or 80000080h, flags a double fault that the run loop turns into FAULT. `docs/hardware/cpu.md` no longer says the word reads as a no-operation.
- **Files modified:** src/cpu_exec.inc, docs/hardware/cpu.md
- **Verification:** `fixtures.boot.past_stub` and `fixtures.boot.double_fault`; the vector tests and cache tests are unchanged
- **Committed in:** 8474d43 and 6ede0b3

**3. [Rule 3 - Blocking] The mailbox layout in CONFORMANCE section 4 contradicts the committed stub**
- **Found during:** Task 1 (writing mailbox.inc)
- **Issue:** CONFORMANCE puts the status byte at 05h and the CRC at 08h; the stub from plan 02-16, already committed as an array, stores the word FFh at 08h and reads the handler at 0Ch, and the old hello.asm kept a status word at 08h.
- **Fix:** the layout follows the stub (CRC 04h, status byte 08h, version 09h, count 0Ah, id 0Bh, handler 0Ch, records 10h+8n). The stub's word store clears version, count and id on a fault; the registered handler writes them back. Plan 02-23's runner must read status at 08h and the count at 0Ah. CONFORMANCE section 4's table is a preparation file and was not edited.
- **Files modified:** tests/fixtures/src/mailbox.inc, docs/hardware/cpu.md
- **Verification:** fixtures.hello.boot and fixtures.boot.registered_handler check the CRC against the core's CRC-32
- **Committed in:** 8474d43 and 6ede0b3

**4. [Rule 3 - Blocking] armips labels are case-insensitive, and an equ breaks .ifndef**
- **Found during:** Task 2 (exc_handler.exe)
- **Issue:** the plan's `-equ HANDLER 1` cannot drive `.ifndef`, and a label named HANDLER collides with the code label `handler`.
- **Fix:** the variant is built with `-definelabel USE_HANDLER 1`, the same form header.inc already uses for PC0.
- **Files modified:** tests/fixtures/src/exc_default.asm, tests/fixtures/tests.cmake
- **Verification:** both images assemble and both cases pass
- **Committed in:** 51edc6f

**5. [Rule 1 - Bug] docs/embedding.md described HOOK as reserved and a budget as exact**
- **Found during:** Task 3
- **Issue:** the embedding guide's stop table said nothing returns HOOK and that N cycles then M equal N + M without qualification.
- **Fix:** HOOK and the double-fault FAULT are listed, and the boundary rule is stated for a loaded program.
- **Files modified:** docs/embedding.md
- **Verification:** `ctest -R '^docs\.'` exit 0
- **Committed in:** 6ede0b3

---

**Total deviations:** 5 auto-fixed (2 bugs, 1 missing critical, 2 blocking)
**Impact on plan:** each was necessary for a truth of the plan or for a test that already existed; none changed scope. The fixture case list gained no extra case beyond the eight named.

## Issues Encountered

- The first full ci workflow after the loop failed only `abi.api_v2` (deviation 1); everything else, including `runner.hash`, passed unchanged.
- The Task 2 files were staged away while Task 1 was committed (a stripped copy of the test files, restored afterwards), so each task's commit builds and passes on its own; the ci workflow was run on each state.

## Known Stubs

None.

## Threat Flags

None. The plan adds no network, file or trust-boundary surface; the run loop's bound (limit, frame end, 128-instruction blocks) is the AGENTS rule 4 mitigation and `fixtures.boot.bounded` is its test.

## Next Phase Readiness

- Plan 02-18 (TTY and the A/B putchar entries) and plan 02-19 (interrupt acceptance) can build on `schway__cpu_run`: `block_end` is where a pending interrupt is evaluated, and I_STAT bit 0 already arrives at the frame end.
- Plan 02-21 compiles `cpu_step` and `cpu_block` twice; the block loop is in the include, as that plan expects.
- Plan 02-23 reads the mailbox through `schway_peek`: status at 08h, count at 0Ah, CRC at 04h over 10h-FFh.

## Self-Check: PASSED

- Created files exist: `tests/fixtures/src/mailbox.inc`, `tests/fixtures/src/exc_default.asm`, this summary.
- Commits exist on the branch: `8474d43`, `51edc6f`, `6ede0b3`.
- `cmake --workflow --preset ci` exit 0 (164 of 164) after the last task commit; `ctest -L fixtures` 14 of 14.

---
*Phase: 02-the-cpu-matches-the-public-vectors*
*Completed: 2026-11-09*
