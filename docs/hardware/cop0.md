# COP0 and exceptions

COP0 is the system-control coprocessor of the R3000A. It holds the mode and interrupt-enable stack, the exception
state and the breakpoint registers, and every exception the CPU takes goes through one function, `schway__cop0_exception`
in `src/cop0.c`. Everything here is integer arithmetic and behaves the same on every platform. Facts come from the
psx-spx CPU Specifications page (COP0 registers and exception handling) and from section 2 of
`.planning/preparation/PSX-SYSTEM.md`, which also names the sources it was checked against. Where this page says a fact
is not measured on the console, the tests pin the behaviour the documents describe and say so.

This page covers the registers, the entry rules, the vectors, MFC0, MTC0 and RFE, coprocessor-unusable, the exception
loop and interrupts. The breakpoint registers are held but do not break.

## Registers

`MFC0` and `MTC0` move a general register to or from COP0 register `rd` (PSX-SYSTEM section 2, COP0 registers).

| Register | Name | Reads | Writes |
|---|---|---|---|
| 3, 5, 9, 11 | BPC, BDA, BDAM, BPCM | what was written | stored whole |
| 6 | TAR | the branch target of the last delay-slot exception | ignored |
| 7 | DCIC | what was written | stored whole |
| 8 | BadVaddr | the address of the last address error | ignored |
| 12 | SR | the status register, below | kept under the mask `F04FFF3Fh` |
| 13 | CAUSE | the exception state, below | bits 8-9 only |
| 14 | EPC | the address to return to | ignored |
| 15 | PRID | `00000002h` | ignored |
| 16-31 | not there | 0, where the console returns unspecified bytes | ignored |
| 0-2, 4, 10 | not there | a reserved instruction exception | a reserved instruction exception |

The breakpoint registers keep their values and take no action: no breakpoint is modelled.

`MFC0` has one delay slot, like a load: the instruction after it still sees the old register, and the value lands as
that instruction ends. A write to the same register by the next instruction cancels it (PSX-SYSTEM section 2, Load delay
and COP delays). A COP0 instruction with the user-mode bit set and `CU0` clear is a coprocessor-unusable exception, below.

## SR

| Bits | Name | Meaning |
|---|---|---|
| 0, 2, 4 | IEc, IEp, IEo | interrupt enable: current, previous, old |
| 1, 3, 5 | KUc, KUp, KUo | mode: current, previous, old; 1 is user mode |
| 8-15 | Im | interrupt mask |
| 16-19 | Isc, Swc, PZ, CM | cache isolate, cache swap, parity zero, cache miss |
| 22 | BEV | 1 selects the boot vectors in ROM |
| 28-31 | CU0-CU3 | coprocessor usable; CU2 is the GTE, CU1 and CU3 are not on the console |

Bits 6-7, 20-21 and 23-27 read 0 and ignore writes. The three (IE, KU) pairs are a stack of three: an exception pushes it, `RFE` pops it
(PSX-SYSTEM section 2, SR bits).

## CAUSE

| Bits | Name | Meaning |
|---|---|---|
| 2-6 | ExcCode | the exception, below |
| 8-9 | software IP | two interrupt flags a guest sets by `MTC0`; the only writable bits |
| 10-15 | IP | hardware interrupt lines; bit 10 follows the interrupt status and mask registers |
| 28-29 | CE | the coprocessor of a coprocessor-unusable exception |
| 30 | BT | the branch of a delay-slot exception was taken |
| 31 | BD | the exception happened in a branch delay slot |

| ExcCode | Name | Raised by |
|---|---|---|
| 00h | INT | an interrupt |
| 04h | AdEL | an instruction fetch from an address that is not a multiple of 4; a misaligned load |
| 05h | AdES | a misaligned store |
| 06h, 07h | IBE, DBE | an instruction or data bus error |
| 08h | Sys | `SYSCALL` |
| 09h | Bp | `BREAK` |
| 0Ah | RI | an encoding that is not an instruction, or a COP0 register that is not there |
| 0Bh | CpU | a coprocessor instruction with its `CU` bit clear |
| 0Ch | Ovf | `ADD`, `ADDI` or `SUB` overflowing as signed numbers |

Codes 01h to 03h belong to the TLB, which the console does not have, and never occur (PSX-SYSTEM section 2, ExcCodes).

## Entry

An exception does these things at once (PSX-SYSTEM section 2, EPC, Priority and Vectors; psx-spx CPU Specifications):

1. SR bits 0-5 shift left by 2. The current pair becomes 0, so the handler runs in kernel mode with interrupts off, and
   the old pair is lost.
2. ExcCode, CE, BT and BD are written to CAUSE; the interrupt bits are left alone.
3. EPC is the address of the instruction, or, when it is in a branch delay slot, the address of the branch before it, so
   that returning re-runs the branch. Interrupts return to EPC itself.
4. For a delay-slot exception TAR is where the branch was going. A branch that was not taken has BT = 0 and TAR is the
   address after the delay slot. Outside a delay slot TAR is not touched.
5. BadVaddr is written by an address error and by nothing else; bus errors and breakpoints leave it.
6. CE is the coprocessor number for coprocessor-unusable and 0 for everything else.
7. The pending branch is discarded: the next instruction is the first one of the handler, never the branch target.
8. The program counter is the general vector.

| SR.BEV | General vector |
|---|---|
| 1 | `BFC00180h` |
| 0 | `80000080h` |

Reset leaves BEV set, so an exception before the guest clears it runs ROM code (PSX-SYSTEM section 2, Vectors and Reset
state).

A load issued by the instruction before an exception still lands, so an exception handler never sees it half done
(PSX-SYSTEM section 2, Load delay).

## RFE

`RFE` shifts SR bits 2-5 into 0-3 and leaves bits 4-5 as they are. It does not jump: the handler returns with
`JR` to EPC and `RFE` in the delay slot (PSX-SYSTEM section 2, SR bits and EPC).

## Address errors on the fetch

A jump register or `JALR` to an address that is not a multiple of 4 runs its delay slot and then raises AdEL when the
target is fetched. EPC and BadVaddr are both the bad target, and BD is 0 (PSX-SYSTEM section 2, Branch delay). Nothing
is read from the bus for the bad fetch.

## Coprocessor unusable

`COP1` and `COP3` with `CU1` or `CU3` clear raise CpU with CE 1 or 3. The console has neither coprocessor, so the first
such exception is also recorded as a first fault. With the bit set the instruction does nothing, except `BCzF`, which
always goes, and `BCzT`, which never does, because the coprocessor's condition line is never true (PSX-SYSTEM section 2,
SR bits). A COP0 instruction in user mode with `CU0` clear is CpU with CE 0, and is an ordinary exception a guest handler
may take, not a first fault. COP2, the geometry engine, is handled with the GTE.

## Which exceptions are a first fault

The core keeps the first event it calls impossible or unmodelled (ARCHITECTURE section 6). An exception the guest is
meant to take leaves nothing there:

| Exception | First fault |
|---|---|
| `SYSCALL`, `BREAK`, overflow, an interrupt | none |
| coprocessor unusable on COP0 or COP2 | none |
| coprocessor unusable on COP1 or COP3 | `cop_unusable` |
| a reserved instruction | `reserved_instruction` |
| a misaligned fetch | `misaligned_fetch` |
| the same EPC and ExcCode 1,000 times in a row | `exception_loop`, with the count in its note |

The loop counter counts consecutive exceptions with one EPC and one ExcCode, including `SYSCALL`, `BREAK` and overflow,
since a handler that returns to the instruction that raised it never makes progress. A different EPC or ExcCode starts
the count again, and an interrupt is neither counted nor does it break a run. The counter is part of the saved state.

## Interrupts

The interrupt line the CPU takes is true while any of CAUSE bits 8-15 is set with the matching SR mask bit (8-15) set
and SR.IEc set. CAUSE bit 10 is the hardware line: it is the live result of I_STAT AND I_MASK, changed by the write or
the raise that changes it, with no delay, so a `MFC0` of CAUSE right after an I_MASK write already sees the new value.
Bits 8 and 9 are the two software flags a guest sets itself (PSX-SYSTEM section 2, SR bits and CAUSE bits; section 6).

The interrupt is not taken at the change. The core keeps a retirement stamp, the `irq_stamp` field of the CPU state: 0
while nothing is armed, otherwise the number of instructions still to retire before the entry. It is set when the line
goes from false to true, by that change and nothing else, so reading CAUSE, SR or I_STAT never arms an interrupt, and
a change to false disarms it. Each retired instruction counts it down, and at 0 the interrupt is taken before the next
instruction runs, if the line is still true.

| Change that makes the line true | Retirements before the entry | EPC |
|---|---|---|
| a device edge, for example the vertical blank raised at the end of a frame | 2 after the edge | the third instruction after it |
| a write to I_MASK | 2, the write counting as the first | the write plus 8 |
| `MTC0` to SR | 2, the `MTC0` counting as the first | the `MTC0` plus 8 |
| `MTC0` to CAUSE | 1, the `MTC0` itself | the `MTC0` plus 4 |
| `RFE`, which restores IEc | 1, the `RFE` itself | the instruction after the `RFE` |

These latencies are an inference from the published behaviour, which is that a newly asserted interrupt is delayed by
about two instructions (PSX-SYSTEM section 6, interrupts). Nothing measured on the console separates them from one
instruction more or less, and the tests say so beside each value. The `doc` rows are pinned in
`tests/cpu/exception_tests.c`: `MTC0` to CAUSE gives EPC = pc + 4, `MTC0` to SR enabling a pending line gives
EPC = pc + 8, an I_MASK write gives EPC = pc + 8, and a CAUSE write in a delay slot is taken after the slot with BD = 0.
A change to one of them needs a dated line in `tests/diag/allow.txt` that names the evidence for it, in the format
`YYYY-MM-DD <fixture-or-test> <row> <new value> <evidence>`.

What the entry does, on top of the steps under Entry:

- ExcCode is 0 and no first fault is recorded. An interrupt returns to EPC itself: the instruction it was taken before,
  or the branch before it when that instruction is a delay slot, with BD, BT and TAR as for any delay-slot exception.
  A write to CAUSE in the slot of a branch is taken after the slot, when the branch is already gone, so BD is 0 and EPC
  is where the branch went.
- A load issued by the instruction just before lands first. The handler sees it, and the instruction the interrupt
  cut off, which would have read the old value, reads the new one when it runs after the return (PSX-SYSTEM section 2,
  Load delay).
- Stores in the write queue stay queued. They are already in RAM in store order, and the first uncached read in the
  handler empties the queue.
- `RFE` looks at the line at once. A handler that returns without acknowledging its source is entered again after the
  `RFE`.
- When an interrupt is due on the same instruction as an exception of that instruction, the order is data AdEL, AdES
  and DBE, then overflow, then the interrupt, then `SYSCALL`, `BREAK`, RI and CpU, then the fetch's AdEL and IBE
  (PSX-SYSTEM section 2, Priority). The ones above the interrupt are taken and the interrupt waits for the return from
  exception; the ones below are not, and the instruction runs again after the return.
- The entry calls one hook, `schway__cop0_irq_entry_cop2`, with the word at EPC. On the console an interrupt taken
  when the next instruction is a GTE command lets that command execute, so it runs twice (PSX-SYSTEM section 2, GTE in a
  delay slot of an interrupt). The GTE does not exist yet, so the hook only reports whether the word is a GTE command and
  executes nothing; the plan that builds the GTE executes it there.

## Tests

`ctest -L cpu` runs the `cpu.cop0.*` cases, which place words in RAM, run them one instruction at a time and compare the
registers, the delay latches and the first-fault record with values worked out from the facts above. The vector sample
runs the exception entry for `SYSCALL`, `BREAK` and the overflowing adds, 100 cases each, with the status register
cleared; the unit tests cover what the vectors do not: delay-slot state, the mode stack, `MFC0`, `MTC0`, `RFE`, the
coprocessor cases and the loop. The `cpu.exception.*` cases cover every exception class, the vector choice, bus errors,
the interrupt latencies and priority; `cpu.delay.*` the load delay and its interlocks; and `cpu.timing.*` the multiply
and divide times. Both the vectors and the facts come from documents rather than from the console, so a pass shows
agreement with them, not with silicon.
