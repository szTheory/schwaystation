# The CPU

The PlayStation's CPU is a MIPS I R3000A with a coprocessor for system control (COP0) and another for geometry (COP2).
The core runs it as an interpreter: one function fetches, decodes and executes a single instruction, and both the
frame loop and the vector tests call that function, so there is one decoder. Everything here is integer arithmetic
and behaves the same on every platform. Facts come from the psx-spx CPU Specifications page and from section 2 of
`.planning/preparation/PSX-SYSTEM.md`, which also names the sources it was checked against.

This page covers what is implemented so far: the registers, the decode fields, the arithmetic, logic, shift,
set-on-less-than and load-upper-immediate class, the trapping adds, branches and jumps, the two delay latches and the
vector tests. COP0 and the exception entry are on their own page, [cop0.md](cop0.md). Loads, stores, multiply and divide
and the caches are added to this page as they land.

## Registers

- 32 general registers, `r0` to `r31`. `r0` always reads 0 and a write to it is discarded.
- `HI` and `LO`, the multiply and divide result registers.
- The program counter. At power-on it holds `BFC00000h`, the BIOS entry in KSEG1 (PSX-SYSTEM section 2, Reset state).
- COP0 registers, indexed by number: `r6` TAR, `r12` SR, `r13` CAUSE, `r14` EPC and the others of PSX-SYSTEM
  section 2. SR is `00400000h` (BEV set) after power-on.
- Two delay latches, described below.

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
instruction exception and records a first fault; until loads, stores, multiply and divide and COP2 are decoded they
arrive at the same place.

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
load is cancelled and the register keeps the later write. A load into `r0` is held like any other and discarded when
it lands. With no load pending the register number reads 32 and the value 0.

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

**Running it.** `schwaystation-test-vectors FILE --expect N` takes the sample or any file in the container form.

- `--only NAME,...` runs the named upstream files; an unknown name is an error.
- `--expect N` is the zero-case guard: the run fails when it replayed fewer than N cases, so an empty or truncated
  file, or a `--only` list that selected nothing, cannot pass.
- `--allow FILE` reads `YYYY-MM-DD <file> <case> <issue URL>` lines, one per accepted upstream mismatch. The case is
  the text after the `$` in its name. A line covers a failing case; a line for a file that ran and matched no failing
  case is an error. The file starts empty.
- `--flip-expected FIELD` inverts one bit of the first case's expected final `r5`, `epc`, `tar`, `cause`,
  `branch_target` or `load_value`. It exists only for the negative controls, one per field, that show a wrong
  register, COP0 field or latch cannot pass.

The harness rejects a short or a long file, a case name over 50 bytes, a cycle count over 8, a case count over 1,000,
a non-zero upper half in a value or address, a non-zero `r0`, and a branch flag that is not 0 or 1.

`ctest -L vectors` runs the sample test and its controls. Today the sample test covers the 21 arithmetic, logic,
shift, set and `LUI` files, 2,100 cases.
