# The CPU

The PlayStation's CPU is a MIPS I R3000A with a coprocessor for system control (COP0) and another for geometry (COP2).
The core runs it as an interpreter: one function fetches, decodes and executes a single instruction, and both the
frame loop and the vector tests call that function, so there is one decoder. Everything here is integer arithmetic
and behaves the same on every platform. Facts come from the psx-spx CPU Specifications page and from section 2 of
`.planning/preparation/PSX-SYSTEM.md`, which also names the sources it was checked against.

This page covers what is implemented so far: the registers, the decode fields, the arithmetic, logic, shift,
set-on-less-than and load-upper-immediate class, the trapping adds, branches and jumps, loads and stores, multiply and
divide, COP2 before the GTE, the two delay latches and the vector tests. COP0 and the exception entry are on their own
page, [cop0.md](cop0.md). The instruction cache and the write queue are on the bus page, [bus.md](bus.md); the part of the
fetch that touches them is described here.

## Registers

- 32 general registers, `r0` to `r31`. `r0` always reads 0 and a write to it is discarded.
- `HI` and `LO`, the multiply and divide result registers.
- The program counter. At power-on it holds `BFC00000h`, the BIOS entry in KSEG1 (PSX-SYSTEM section 2, Reset state).
- COP0 registers, indexed by number: `r6` TAR, `r12` SR, `r13` CAUSE, `r14` EPC and the others of PSX-SYSTEM
  section 2. SR is `00400000h` (BEV set) after power-on.
- Two delay latches, described below.
- A cycle counter and the ready stamp of the multiply and divide unit, described under Multiply and divide.

## Decode

An instruction is one 32-bit word. The executor takes its fields with shifts and masks:

| Field | Bits | Use |
|---|---|---|
| op | 31-26 | the primary opcode; 0 selects the SPECIAL group |
| rs | 25-21 | first source register |
| rt | 20-16 | second source register, or the destination of an immediate instruction |
| rd | 15-11 | destination of a register instruction |
| sa | 10-6 | shift amount |
| funct | 5-0 | the operation within the SPECIAL group |
| imm | 15-0 | immediate, sign-extended or zero-extended as the instruction says |

Dispatch is a `switch` on op, and inside SPECIAL on funct. An encoding that is not an instruction raises a reserved
instruction exception and records a first fault. The coprocessor 2 forms (`COP2`, `LWC2` and `SWC2`) are picked out
by a decode flag before the switch, so the GTE can take them over without a change to the rest of the decoder.

## Arithmetic, logic, shifts and sets

All of these read their operands before any load issued by the previous instruction lands, and none raises an
exception (PSX-SYSTEM section 2, Load delay).

| Instructions | Result |
|---|---|
| `ADDU`, `ADDIU`, `SUBU` | wrap-around add and subtract. The immediate of `ADDIU` is sign-extended. |
| `ADD`, `ADDI`, `SUB` | the same, but a result that overflows as signed numbers raises an overflow exception and leaves the destination as it was |
| `AND`, `OR`, `XOR`, `NOR` | bitwise, register to register |
| `ANDI`, `ORI`, `XORI` | bitwise with the zero-extended immediate |
| `SLL`, `SRL`, `SRA` | shift `rt` left, right logical or right arithmetic by `sa` |
| `SLLV`, `SRLV`, `SRAV` | the same by the low five bits of `rs` |
| `SLT`, `SLTI` | 1 when `rs` is less than the operand as signed numbers, else 0. The immediate is sign-extended. |
| `SLTU`, `SLTIU` | 1 when `rs` is less than the operand as unsigned numbers, else 0. The immediate of `SLTIU` is sign-extended first, then compared unsigned. |
| `LUI` | the immediate in the upper half, the lower half zero |

`SLL` with every field zero is the usual no-op. It writes `r0`, so it is discarded, but it still cancels a pending
load into `r0` like any other write.

## Delay latches

The R3000A has one load delay slot and one branch delay slot (PSX-SYSTEM section 2, Load delay and Branch delay). The
executor holds each as a small latch in the CPU state, so a saved state taken between two instructions resumes exactly.

**Load delay.** A load issued by one instruction is held as a register number and a value, and writes the register
as the next instruction ends. That instruction therefore reads the old value. If it writes the same register, the
load is cancelled and the register keeps the later write; if it is itself a load into that register, the earlier
load is dropped and only the later one is pending. A load into `r0` is held like any other and discarded when it
lands. A load that fails, by an address error or a bus error, issues nothing and leaves its register untouched. With no
load pending the register number reads 32 and the value 0. `MFC0`, `MFC2` and `CFC2` use the same latch.

**Branch delay.** The instruction after a branch or jump always executes. The latch holds whether the instruction
about to run is a delay slot, whether its branch is taken, and the branch target. There is no separate next-program-
counter register: the next program counter is the target while a taken branch is pending, and the current program
counter plus 4 otherwise. Each instruction ends by clearing the latch, and a branch sets it for the following one.
A branch in a delay slot computes its target from the address its own slot would go to next, so it follows a taken
branch's target (PSX-SYSTEM section 2, Branch delay).

## Branches and jumps

| Instructions | Behaviour |
|---|---|
| `BEQ`, `BNE` | go when `rs` equals, or differs from, `rt` |
| `BLEZ`, `BGTZ` | go when `rs` is at most, or above, 0 as a signed number |
| `BLTZ`, `BGEZ` | go when `rs` is below, or at least, 0; bit 16 of the instruction picks which |
| `BLTZAL`, `BGEZAL` | the same with the two instruction-field values `10h` and `11h`, and the return address is written to `r31` whether or not the branch goes |
| `J`, `JAL` | the 26-bit field times 4 in the 256 MiB region of the delay slot; `JAL` writes `r31` |
| `JR`, `JALR` | the target is `rs`; `JALR` writes the return address to `rd` |

A branch offset is the signed 16-bit immediate times 4, added to the address of the delay slot. The return address is
the address after the delay slot. The latch is always set by a branch, taken or not, with the target it would have had.
A target that is not a multiple of 4 is an address error when it is fetched (PSX-SYSTEM section 2, Branch delay; the
exception rules are in [cop0.md](cop0.md)).

## Instruction fetch

Each instruction is fetched at the program counter before it is decoded. A program counter that is not a multiple of 4 is
an address error on the fetch (see Branches and jumps). Otherwise the fetch goes one of three ways, by segment
(PSX-SYSTEM section 2, Segments and I-cache):

- In KUSEG or KSEG0 it goes through the instruction cache, a hit costing nothing beyond the instruction's own cycle and
  a miss filling the line from the missed word to its end.
- In the BIOS window, in any segment, it bypasses the cache and goes to the bus.
- In KSEG1 it first drains the write queue and then goes to the bus; KSEG2 is not executable and the bus refuses it.

A bus error on a fetch reads as the word 0, a no-operation, and the bus records the first fault. The scratchpad is data
only, so a fetch from it is that same bus error. With a replay log attached, as in the vector tests, the fetch is the next
recorded cycle and no cache or queue is used. The cache's rules, the isolation that SR.Isc turns on and the queue are in
[bus.md](bus.md).

## Loads and stores

Every load and store goes through the data access front-end before it reaches the bus, in this order:

1. **Alignment.** `LH`, `LHU`, `SH` take a half-word and `LW`, `SW` a word; an address that is not a multiple of the
   width raises an address error, AdEL (ExcCode 04h) for a load and AdES (05h) for a store, with BadVaddr set to the
   address. It happens before any bus access, and it is a first fault (`misaligned_read` or `misaligned_write`).
   `LWL`, `LWR`, `SWL` and `SWR` never fault on alignment.
2. **Segment.** In user mode (SR.KUc = 1) only KUSEG, the addresses below `80000000h`, may be touched; any other
   address raises the same address error. A handler may take it, so it records no first fault.
3. **The bus.** With SR.Isc set, a KUSEG, KSEG0 or KSEG1 access goes to the instruction cache instead and never reaches
   the bus (see [bus.md](bus.md)). Otherwise a KSEG1 access first drains the write queue, and then the virtual address
   goes to the bus, which decodes the segment and maps it (KSEG2 gets the full
   address, because only the cache-control word answers there). The bus charges its wait cycles to the instruction, and
   a store that landed joins the write queue. A
   bus error, an address no device answers, raises DBE (07h); it sets no BadVaddr, and the bus records the first
   fault (PSX-SYSTEM section 2, Address and bus errors and Segments).

`LB`, `LH` and `LW` read 1, 2 or 4 bytes and sign-extend a byte or half-word; `LBU` and `LHU` zero-extend. The value
goes through the load delay latch. `SB`, `SH` and `SW` write the low 1, 2 or 4 bytes of `rt`; the bus is handed all
32 bits whatever the width (PSX-SYSTEM section 2, Stores). The address is `rs` plus the sign-extended immediate.

**Unaligned forms.** `LWL` and `LWR` read the aligned word that holds the address, and replace the bytes of `rt` from
the address toward the high end (`LWL`) or the low end (`LWR`); the other bytes of `rt` stay. The merge starts from
the value of a load the previous instruction issued to the same register, not from the register's old value, so
an `LWL`, `LWR` pair on one register needs no delay slot between them and the pair assembles an unaligned word
(PSX-SYSTEM section 2, Stores, LWL/LWR). `SWL` and `SWR` write the matching bytes of `rt` into the bytes of the
aligned word from its low end to the address (`SWL`) or from the address to its high end (`SWR`), as the largest
aligned chunks of 4, 2 or 1 bytes, lowest first.

## Multiply and divide

`MULT` and `MULTU` put the 64-bit product of `rs` and `rt` in `HI:LO`, signed and unsigned. `DIV` and `DIVU` put the
quotient in `LO` and the remainder in `HI`; the remainder of `DIV` has the sign of `rs`. Nothing traps. A zero
divisor gives `LO` = -1 (`DIV` of a negative `rs` gives +1) and `HI` = `rs`, and `DIV` of -2147483648 by -1 gives
`LO` = `80000000h` and `HI` = 0 (PSX-SYSTEM section 2, DIV/DIVU).

The unit takes time, which the CPU state keeps as a cycle counter and a ready stamp: the counter value at which the
running operation has produced `HI` and `LO`, or 0 when none is running. An operation issued at counter value t is
ready at t plus its length. `MULT` takes 6, 9 or 13 cycles by the size of `rs` with its sign mirrored (`rs` XOR its
sign bit spread over the word, below 800h, below 100000h, or more); `MULTU` classifies the same way on `rs` as it is;
`DIV` and `DIVU` take 36. `MFHI` and `MFLO` issued before the stamp wait for it, never longer than 36 cycles whatever
a loaded state holds. `MTHI` and `MTLO` write the register and set the stamp to 0, so a read after them does not wait.
A new multiply or divide replaces one still running: the wait a later read sees is the new operation's own time.
`HI`, `LO` and the stamp are not touched by an exception (PSX-SYSTEM section 2, MULTU/MULT).

## COP2 before the GTE

The GTE is not built yet, so COP2 behaves as the processor does when the coprocessor has not been enabled or has
nothing behind it, and a later GTE replaces this with no change to the decoder.

- With SR.CU2 clear, every COP2 form raises coprocessor unusable (ExcCode 0Bh) with CE 2: the register moves `MFC2`,
  `CFC2`, `MTC2` and `CTC2`, the branches `BC2F` and `BC2T`, the commands, and `LWC2` and `SWC2`. The last two check
  alignment first, so an odd address is an address error. It records no first fault, because a handler may take it.
- With SR.CU2 set, `MFC2` and `CFC2` read 0 into `rt` through the load delay; `MTC2`, `CTC2` and the commands do
  nothing; `BC2F` always goes and `BC2T` never does, as the condition line is never true; `LWC2` does the real bus read
  and drops the data, so a read of a bus gap is still a bus error, and `SWC2` does the real bus write of 0. Each of
  them counts once as an access to the unbuilt GTE, which the runner reports (PSX-SYSTEM section 2, SR bits).

`COP1` and `COP3` do not exist on the console. With SR.CU1 or SR.CU3 clear their instructions, loads and stores
included (`LWC1`, `SWC1`, `LWC3`, `SWC3`), raise coprocessor unusable with CE 1 or 3 and record a first fault; with the
bit set they do nothing, except that `BCzF` always goes and `BCzT` never does.

## Vector tests

The SingleStepTests `r3000` vectors give, for each of 1,000 cases of every instruction, the CPU state before, the
state after and the bus cycles in between. The repository commits the first 100 cases of each of the 55 files in
`tests/vectors/r3000/sst-sample.bin`. The vectors were generated by another
emulator's interpreter, so a pass shows agreement with that interpreter and with the public documents, not with
silicon; behaviour the tests cannot justify is pinned by the repository's own tests instead.

**Container.** All numbers are little-endian. A `u16` slice count (55), then per slice a 16-byte name padded with
NULs (the upstream file's name), a `u32` byte length and the verbatim upstream bytes: a `u32` case count patched to
100, then the cases. A case is a 51-byte name (a length byte, then up to 50 characters), a `u32` opcode, a `u32`
address, an initial and a final state, a `u32` cycle count and 24 bytes per cycle (value, kind, address and size,
the value and address as 64-bit numbers whose upper half must be zero). A one-cycle case takes 431 bytes.

**State.** 172 bytes: the 32 general registers, then `HI`, `LO`, EPC, TAR, CAUSE and the program counter, then the
branch target, branch slot and branch take, then the load target (-1 for none) and the load value, each a `u32`.

**The replay bus.** `schway__cpu_step_one` runs one instruction through the same executor as the frame loop. With a
log attached, its fetches and reads return the recorded values in order and its writes are recorded, so no RAM,
cache, scratchpad or device is involved. The harness loads the 32 registers, `HI`, `LO`, the program counter, EPC,
TAR, CAUSE and both latches from the initial state into a freshly reset CPU, sets SR to 0, and steps once.
SR is zero so that no interrupt, mode or cache bit changes the result.

**The comparison.** Every access is compared with the case's cycle of the same position (kind, size, address, value),
and an extra or a missing cycle fails. The final state is then compared field by field in the order of the state
layout: `r0` to `r31`, `hi`, `lo`, `pc`, `epc`, `tar`, `cause`, the branch target, slot and take, and the load target
and value. A failure prints `<file> <case name> final <field>: want 0x%08x got 0x%08x`, or the cycle form with the
cycle index. The first dozen failing cases print; every one counts.

**Two named rules** adjust the comparison, are not lines of `allow.txt`, and print how many cases each handled; the
sample has 310 cases of the first and 400 of the second.

- *Misaligned accesses.* The upstream interpreter copies bits 27 and 26 of the instruction into CAUSE's CE field
  for every exception, where the console sets it only for coprocessor unusable. For a misaligned `LH`, `LHU`, `LW`,
  `SH` or `SW`, found from the opcode and the effective address, the harness therefore asserts the exception itself:
  the vector `80000080h`, ExcCode 4 for a load or 5 for a store, BadVaddr the address and only the fetch on the bus.
  It then compares every final field except the CE bits.
- *Unaligned forms.* `LWL`, `LWR`, `SWL` and `SWR` are recorded upstream as chunks of 1, 2 and 4 bytes. The harness
  coalesces the chunks to the set of bytes they cover, checks that set is the one the instruction names (bytes 0 to
  the address for `LWL` and `SWL`, the address to 3 for `LWR` and `SWR`), and compares that. A load is replayed as one
  read of the aligned word whose covered bytes hold the recorded data; a store's chunks are coalesced the same way
  on the core's side, so chunking is not compared.

**Running it.** `schwaystation-test-vectors FILE --expect N` takes the sample or any file in the container form.

- `--only NAME,...` runs the named upstream files; an unknown name is an error. The sample test does not use it.
- `--expect N` is the zero-case guard: the run fails when it replayed fewer than N cases, so an empty or truncated
  file, or a `--only` list that selected nothing, cannot pass. The sample test expects 5,500.
- `--allow FILE` reads `YYYY-MM-DD <file> <case> <issue URL>` lines, one per accepted upstream mismatch. The case is
  the text after the `$` in its name. A line covers a failing case; a line for a file that ran and matched no failing
  case is an error. The file starts empty.
- `--flip-expected FIELD` inverts one bit of the first case's expected final `r5`, `epc`, `tar`, `cause`,
  `branch_target` or `load_value`; `FIELD@FILE` does it to the first case of the named upstream file instead. It exists
  only for the negative controls, one per field and one for each of the two rules, that show a wrong register, COP0
  field or latch cannot pass.

The harness rejects a short or a long file, a case name over 50 bytes, a cycle count over 8, a case count over 1,000,
a non-zero upper half in a value or address, a non-zero `r0`, and a branch flag that is not 0 or 1.

`ctest -L vectors` runs the sample test and its controls. The sample test covers all 55 files, 5,500 cases.
