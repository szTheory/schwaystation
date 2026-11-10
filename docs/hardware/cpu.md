# The CPU

The PlayStation's CPU is a MIPS I R3000A with a coprocessor for system control (COP0) and another for geometry (COP2).
The core runs it as an interpreter: one function fetches, decodes and executes a single instruction, and both the
frame loop and the vector tests call that function, so there is one decoder. Everything here is integer arithmetic
and behaves the same on every platform. Facts come from the psx-spx CPU Specifications page.

This page covers what is implemented so far: the registers, the decode fields, the arithmetic, logic, shift,
set-on-less-than and load-upper-immediate class, the trapping adds, branches and jumps, loads and stores, multiply and
divide, COP2 and the GTE, the two delay latches, the vector tests, the run loop and the stub kernel's layout. COP0
and the exception entry are on their own page, [cop0.md](cop0.md). The instruction cache and the write queue are on the bus page, [bus.md](bus.md); the part of the
fetch that touches them is described here.

## Registers

- 32 general registers, `r0` to `r31`. `r0` always reads 0 and a write to it is discarded.
- `HI` and `LO`, the multiply and divide result registers.
- The program counter. At power-on it holds `BFC00000h`, the BIOS entry in KSEG1 (psx-spx CPU Specifications, Reset state).
- COP0 registers, indexed by number: `r6` TAR, `r12` SR, `r13` CAUSE, `r14` EPC and the others of the psx-spx CPU
  Specifications page. SR is `00400000h` (BEV set) after power-on.
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
by a decode flag before the switch and handled in one function, which reaches the GTE.

## Arithmetic, logic, shifts and sets

All of these read their operands before any load issued by the previous instruction lands, and none raises an
exception (psx-spx CPU Specifications, Load delay).

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

The R3000A has one load delay slot and one branch delay slot (psx-spx CPU Specifications, Load delay and Branch delay). The
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
branch's target (psx-spx CPU Specifications, Branch delay).

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
A target that is not a multiple of 4 is an address error when it is fetched (psx-spx CPU Specifications, Branch delay; the
exception rules are in [cop0.md](cop0.md)).

## Instruction fetch

Each instruction is fetched at the program counter before it is decoded. A program counter that is not a multiple of 4 is
an address error on the fetch (see Branches and jumps). Otherwise the fetch goes one of three ways, by segment
(psx-spx CPU Specifications, Segments and I-cache):

- In KUSEG or KSEG0 it goes through the instruction cache, a hit costing nothing beyond the instruction's own cycle and
  a miss filling the line from the missed word to its end.
- In the BIOS window, in any segment, it bypasses the cache and goes to the bus.
- In KSEG1 it first drains the write queue and then goes to the bus; KSEG2 is not executable and the bus refuses it.

A bus error on a fetch raises IBE (06h) at the fetched address, with no BadVaddr, and the bus records the first fault;
the word it read, 0, is never executed. The scratchpad is data only, so a fetch from it is that same bus error. An IBE
taken while fetching from an exception vector is a double fault, described under The run loop. With a replay log attached, as in the vector tests, the fetch is the next
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
   fault (psx-spx CPU Specifications, Address and bus errors and Segments).

`LB`, `LH` and `LW` read 1, 2 or 4 bytes and sign-extend a byte or half-word; `LBU` and `LHU` zero-extend. The value
goes through the load delay latch. `SB`, `SH` and `SW` write the low 1, 2 or 4 bytes of `rt`; the bus is handed all
32 bits whatever the width (psx-spx CPU Specifications, Stores). The address is `rs` plus the sign-extended immediate.

**Unaligned forms.** `LWL` and `LWR` read the aligned word that holds the address, and replace the bytes of `rt` from
the address toward the high end (`LWL`) or the low end (`LWR`); the other bytes of `rt` stay. The merge starts from
the value of a load the previous instruction issued to the same register, not from the register's old value, so
an `LWL`, `LWR` pair on one register needs no delay slot between them and the pair assembles an unaligned word
(psx-spx CPU Specifications, Stores, LWL/LWR). `SWL` and `SWR` write the matching bytes of `rt` into the bytes of the
aligned word from its low end to the address (`SWL`) or from the address to its high end (`SWR`), as the largest
aligned chunks of 4, 2 or 1 bytes, lowest first.

## Multiply and divide

`MULT` and `MULTU` put the 64-bit product of `rs` and `rt` in `HI:LO`, signed and unsigned. `DIV` and `DIVU` put the
quotient in `LO` and the remainder in `HI`; the remainder of `DIV` has the sign of `rs`. Nothing traps. A zero
divisor gives `LO` = -1 (`DIV` of a negative `rs` gives +1) and `HI` = `rs`, and `DIV` of -2147483648 by -1 gives
`LO` = `80000000h` and `HI` = 0 (psx-spx CPU Specifications, DIV/DIVU).

The unit takes time, which the CPU state keeps as a cycle counter and a ready stamp: the counter value at which the
running operation has produced `HI` and `LO`, or 0 when none is running. An operation issued at counter value t is
ready at t plus its length. `MULT` takes 6, 9 or 13 cycles by the size of `rs` with its sign mirrored (`rs` XOR its
sign bit spread over the word, below 800h, below 100000h, or more); `MULTU` classifies the same way on `rs` as it is;
`DIV` and `DIVU` take 36. `MFHI` and `MFLO` issued before the stamp wait for it, never longer than 36 cycles whatever
a loaded state holds. `MTHI` and `MTLO` write the register and set the stamp to 0, so a read after them does not wait.
A new multiply or divide replaces one still running: the wait a later read sees is the new operation's own time.
`HI`, `LO` and the stamp are not touched by an exception (psx-spx CPU Specifications, MULTU/MULT). The `cpu.timing.*` tests
pin these times and rules.

## COP2 and the GTE

Coprocessor 2 is the geometry engine, described on [its own page](gte.md). The CPU reaches it with the forms below.

- With SR.CU2 clear, every COP2 form raises coprocessor unusable (ExcCode 0Bh) with CE 2: the register moves `MFC2`,
  `CFC2`, `MTC2` and `CTC2`, the branches `BC2F` and `BC2T`, the commands, and `LWC2` and `SWC2`. The last two check
  alignment first, so an odd address is an address error. It records no first fault, because a handler may take it.
- With SR.CU2 set, `MTC2` and `CTC2` write data register `rd` or control register 32 + `rd` with `rt`. `MFC2` and
  `CFC2` read one into `rt` through the load delay, so the value is visible one instruction later. `LWC2` does the
  real bus read of the word and writes it to data register `rt`, and `SWC2` does the real bus write of data register
  `rt`; a read of a bus gap is still a bus error. `BC2F` always goes and `BC2T` never does, as the condition line is
  never true. None of these counts as an access to an unbuilt device.
- A command (an instruction with bit 25 of the word set) executes at once. It keeps the engine busy for its documented
  time, 5 to 44 cycles by command; the CPU keeps the engine's ready stamp, the cycle counter value at which the last
  command finishes, with the same convention as the multiply unit: a command issued at counter value t is ready at t
  plus its busy time.
- `MFC2`, `CFC2`, `SWC2` and a new command wait for the running command; `MTC2`, `CTC2` and `LWC2` do not (the
  gte-timing hardware results, MIT, What waits). The wait is what is left of the busy time when the instruction
  issues, so each instruction between the command and the one that waits shortens it by a cycle, and it is never more
  than 44 cycles, the longest command, whatever a loaded state holds. `MFC2` right after a command takes the command's
  busy time in all, as `MFLO` right after a multiply takes the multiply's.
- Interrupts: an interrupt taken before a GTE command lets the command execute at the entry and again after the return.
  The entry's busy stamp is set from the entry's own cycle. The command is the instruction at the program counter, in a
  branch delay slot the slot's, not the branch EPC names; see [the COP0 page](cop0.md#interrupts).

Deviation: on the console a write to a GTE register by `MTC2`, `CTC2` or `LWC2` is not visible to a command or a read
for two instructions. This implementation does not model that latency, so a command or read right after the write sees
the new value. Programs that depend on the old value in those two instructions are not reproduced, and the case is not
in the hardware vectors replayed here.

The `cpu.cop2.*` tests pin the moves and the unusable check, `cpu.timing.gte_cycles` and `cpu.timing.gte_stall_rules`
the busy times and the wait rule, and the `cpu.exception.irq_gte_*` and `irq_entry_peek` tests the interrupt entry.

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

## The stub kernel and kernel RAM

With no BIOS from the host the core maps its own stub kernel, 620 bytes of MIPS at `BFC00000h` (see [bus.md](bus.md)),
and the CPU starts there. The stub is written for this project, holds no Sony byte and provides no kernel service
beyond what is listed here. It uses only the addresses below, all in the low 64 KiB of RAM except the ROM, and the
table keeps them disjoint:

| Address | Holds |
|---|---|
| `80000080h`-`8000008Fh` | the general vector with SR.BEV clear: four words that load the handler's ROM address into `k0` and jump to it |
| `800000A0h`, `800000B0h`, `800000C0h` | the entries of the A, B and C kernel calls, four words each, written by the reset code |
| `BFC00180h` | the general vector while SR.BEV is set: a jump to the same handler |
| ROM after `BFC00180h` | the handler body, which uses only `k0` and `k1` |
| `80000200h`-`8000020Fh` | the launch block: `pc0`, `gp0`, `s_addr`, `s_size`, written by `schway_load_exe` |
| `80000210h`-`8000021Fh` | the exception record: Cause, EPC, BadVaddr, SR |
| `80010000h`-`800100FFh` | the mailbox of a loaded image |

**Reset sequence.** The stub writes the memory-control values the retail boot code writes, clears `I_MASK` and
`I_STAT`, copies the four-word vector to `80000080h`, clears SR.BEV, then reads the launch block. With no image loaded
(`pc0` zero) it stays in a loop. Otherwise `gp` is `gp0`, `sp` is `s_addr + s_size` (`801FFF00h` when `s_addr` is 0) and
it jumps to `pc0`. The scratchpad is never touched. Between the reset code and `BFC00180h` every byte is `FFh`, an
opcode that raises an exception, so a runaway that falls out of the reset code is recorded and never slides through
no-operations.

**Kernel calls.** The A and B entries jump to routines in ROM that implement one function, putchar: a call to `A0h`
with `t1` = `3Ch`, or to `B0h` with `t1` = `3Dh`, stores the low byte of `a0` to the debug port at `1F802080h` and
returns. Any other function number returns at once, and so does `C0h`. The routines use only `k0` and `k1`, so every
other register survives the call. With the profile's debug-port flag clear the store is ignored, and the byte goes
nowhere.

**The default handler.** An exception writes Cause, EPC, BadVaddr and SR to the exception record. If the mailbox magic is
there, the handler sets the mailbox word at `08h` to `000000FFh` (status FFh) and jumps to the address in the word at
`0Ch` when that is not zero; otherwise, and with no mailbox, it stops in a loop. An image replaces the default by
writing its own handler address to the mailbox, or its own code at `80000080h`.

**The mailbox.** A fixture image starts with 256 bytes at `80010000h` that the runner and the tests read:

| Offset | Content |
|---|---|
| `00h` | magic `SWMB` |
| `04h` | CRC-32 (IEEE) of bytes `10h`-`FFh`, little-endian |
| `08h` | status: `80h` running, `81h` waiting for input, `00h` done, `FFh` fault |
| `09h` | protocol version, 1 |
| `0Ah` | record count, at most 30 |
| `0Bh` | fixture id |
| `0Ch` | address of the image's exception handler, 0 for none |
| `10h + 8n` | record n: id (u16), flags (u16: bit 0 valid, bit 1 timed out, bit 2 profile dependent), value (u32) |

The stub's `FFh` is a word store, so a fault also clears the version, count and id bytes, and it leaves the CRC as it
was. An image that recovers writes them back.

**Regenerating the stub array.** `src/stub_kernel.c` is generated from `tests/fixtures/src/stub/stub.asm`, and
`fixtures.stub_drift` fails while the two differ. The commands are in
[Regenerating the stub kernel](../maintaining.md#regenerating-the-stub-kernel).

## The run loop

While a PS-EXE is loaded the frame advance runs the CPU; with none loaded no instruction runs and the machine shows its
test card. The CPU runs in blocks of instructions, and a block ends after:

- a branch's delay slot;
- an exception entry, which includes `SYSCALL` and `BREAK`, `RFE`, and any `MTC0`, which can change SR or CAUSE;
- a write to `I_STAT` or `I_MASK`, which can change the interrupt line;
- the instruction after which the next one lies in another 4 KiB page, and the 128th instruction of the block;
- a write to the exit port that takes effect;
- the first fault latched in the block.

The cycles of a block are added to the tick counter, and the audio pairs queued, once, when it ends; a first fault
latched inside a block carries the tick its own instruction began on, read from the block clock described under
"Devices, the block clock and the stall".

**Where a block splits at a budget.** After each instruction the loop compares the cycles run with what is left of the
budget, and the block ends at the first instruction boundary at or after it. The last instruction can carry the run past
the budget by the rest of its own cycles (a bus wait, a cache fill or the wait for a multiply), a few dozen at most; the
core assumes under 1,024. A stop carries no overshoot in the state: the ticks and the audio pairs sit at that boundary, the result reports
the ticks it reached, and the next call starts there. Running N cycles and then M therefore equals running N + M when N
is a boundary the one-shot run passes through, which is where a run of the same budget stopped.

**The end of a frame.** The run stops at the first boundary at or after the frame's end. A frame end passed inside an
instruction completes at that instruction's end: `I_STAT` bit 0 is raised, the frame count rises, and the next frame
ends where the exact cycle accumulator puts it, so the next frame is shorter by the overshoot and the cumulative
ticks stay exact. The CPU takes interrupts from the vertical blank, the DMA and the timers: each raises its own `I_STAT` bit, CAUSE
bit 10 follows `I_STAT` and `I_MASK`, and when SR allows it the CPU takes the interrupt as [cop0.md](cop0.md)
(Interrupts) describes.

**Stop reasons.**

| Stop | When |
|---|---|
| `SCHWAY_STOP_FRAME` | the frame ended |
| `SCHWAY_STOP_BUDGET` | the budget ran out inside the frame |
| `SCHWAY_STOP_HOOK` | the guest wrote the exit port with the debug-port flag set; the byte is the result's `exit_code`, and the next run continues |
| `SCHWAY_STOP_FAULT` | with `SCHWAY_RUN_STOP_ON_FAULT`, at the end of the instruction that latched the first fault; and, whatever the flags, at once on a double fault |

A double fault is an instruction fetch that fails at an exception vector (`80000080h`, or `BFC00180h` while SR.BEV is
set): the handler cannot run, so the exception would repeat for ever. When a hook and the frame's end coincide the hook
is reported and the frame still completes. A guest that loops, or hammers an address no device answers, returns from
every call: each call runs a bounded number of cycles, whatever the program does.

## Devices, the block clock and the stall

The tick counter moves only when a block ends, so a device that is read inside a block needs a clock of its own. The
CPU keeps two values that are not state, the tick and the CPU cycle count at the block's start, set at the start of
every block. The tick of an access is then

    now = block start tick + (cycles run since the block's start) + (cycles this access has spent before its own) - 1

where the last term counts a first cycle of 1, so an access on an instruction's own cycle sees the tick that
instruction began on. A loop that polls a counter sees it move inside a block, and a fault latched by an instruction
carries its own start tick whatever its place in a long block. The trace records use the same clock. A power reset, a
state load and a run with no program restart it at the instance's tick.

**Deadlines.** Each device that keeps time reports the tick it next needs attention at, and the earliest of them is
kept as the next deadline, derived from the devices and never saved: a power reset and a state load recompute it, and
a register write that moves a deadline ends the running block at the next instruction boundary, as a write to `I_STAT`
does. Every block is cut at the next deadline as well as at the budget and the frame's end, and the device's event runs
at the first instruction boundary at or after the deadline, once per pass of the run loop. A device whose deadline is
still due after its event waits for the next instruction before it is offered again, so the loop always advances. A
frame that no program runs holds no armed deadline in practice, and the loop then costs one compare per call.

**The bus stall.** While a DMA unit moves its words it holds the bus. A bus access that has to wait for it, a read of
RAM or an I/O register, an instruction fetch that misses the cache, or the fifth queued store (writes are buffered in
the four-deep write queue, so only a full queue waits), sets the tick the CPU is held until. The scratchpad, a cache
hit, COP0 and the GTE never touch the bus and never wait. The CPU does not execute while the stamp is ahead of the
tick: the loop advances the tick, within the budget and the frame and stopping at the next deadline so events run on
time, and the CPU cycle count does not move. This is a freeze rather than a charge on one instruction, because a unit
can hold the bus for tens of thousands of cycles and charging that to a single instruction would pass the overrun
bound above. The part of a stall that crosses the end of a budget or of a frame stays in the state and the next call
keeps waiting, so the overrun past any limit is still at most one instruction.

**The frame end.** With a program loaded the frame end raises `I_STAT` bit 0 and also drives the vertical blank to the
root counters, high and then low at the same tick, because there is no line machine yet to give the blank a width; the
counters page covers what they do with it. With no program no counter is stepped and only an event that is already
due runs.

## The trace

A run with `SCHWAY_RUN_TRACE` in its budget runs the traced copy of the executor, the same source compiled a second
time, and leaves its records in a ring of 4,096 in the instance, outside the state and every hash; `schway_get_trace`
reads them (the embedding guide has the fields). The copy is chosen once per call; the untraced loop has no trace code.

A record's `cycle` is the tick its block started on plus the cycles the CPU has spent since: for an instruction its
start, for a data access the start plus the cycles the instruction spent before it (its issue cycle is not counted).
The block's own ticks are added when it ends, so the position is exact at the block's start and follows the
instruction costs after it.

`line` and `dot` are derived until the GPU keeps a scan position of its own. They count the cycles since the record's
frame began, convert them to GPU clocks at 11 to 7, and divide by the region's clocks per line: 3,413 for NTSC and
3,406 for PAL. `line` is the quotient and `dot` the remainder, a count of GPU clocks into the line and not of dots. The
fault record in `schway_get_fault` keeps its own `line` and `dot` of 0 until the GPU does.
