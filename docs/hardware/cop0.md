# COP0 and exceptions

COP0 is the system-control coprocessor of the R3000A. It holds the mode and interrupt-enable stack, the exception
state and the breakpoint registers, and every exception the CPU takes goes through one function, `schway__cop0_exception`
in `src/cop0.c`. Everything here is integer arithmetic and behaves the same on every platform. Facts come from the
psx-spx CPU Specifications page (COP0 registers and exception handling) and from section 2 of
`.planning/preparation/PSX-SYSTEM.md`, which also names the sources it was checked against. Where this page says a fact
is not measured on the console, the tests pin the behaviour the documents describe and say so.

This page covers the registers, the entry rules, the vectors, MFC0, MTC0 and RFE, coprocessor-unusable and the
exception loop. Interrupts reach the same entry when the interrupt plumbing lands, and the breakpoint registers are held
but do not break.

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

## Tests

`ctest -L cpu` runs the `cpu.cop0.*` cases, which place words in RAM, run them one instruction at a time and compare the
registers, the delay latches and the first-fault record with values worked out from the facts above. The vector sample
runs the exception entry for `SYSCALL`, `BREAK` and the overflowing adds, 100 cases each, with the status register
cleared; the unit tests cover what the vectors do not: delay-slot state, the mode stack, `MFC0`, `MTC0`, `RFE`, the
coprocessor cases and the loop. Both the vectors and the facts come from documents rather than from the console, so a
pass shows agreement with them, not with silicon.
