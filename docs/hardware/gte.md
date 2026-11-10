# The geometry engine

The geometry transformation engine (GTE) is the PlayStation's coprocessor 2: a fixed-point vector unit that rotates and
projects vertices, lights colours and averages depths. This page covers the engine in `src/gte.c`: its registers and the
rules for reading and writing them, the command encoding, the FLAG register, timing, and how the hardware-captured
cases are replayed. Everything is integer arithmetic and behaves the same on every platform. Facts come from the
psx-spx pages on the Geometry Transformation Engine (registers, opcode summary, FLAG), which the hardware cases in the
public ps1-tests repository support; the numerics, the commands and the quirks follow.

## State of the engine

The engine is complete: the register file, the numerics, the divide and all 22 documented commands are built and
replayed against the 1,150 hardware cases, and the quirks the cases pin only thinly have tests of their own. The CPU's
COP2 forms reach this engine, as "COP2 and the GTE" in [the CPU page](cpu.md) describes.

## The two register files

Data registers are `cop2r0` to `cop2r31` and control registers `cop2r32` to `cop2r63`. A program reaches the data
registers with `MFC2`, `MTC2`, `LWC2` and `SWC2` and the control registers with `CFC2` and `CTC2`. The engine keeps
two 32-word arrays in the form a program stores them: a 16-bit register holds its 16-bit pattern, and the read side
adds the sign or zero extension the hardware shows. A write of a 32-bit value to a 16-bit register drops the upper
half and sets no flag.

### Data registers

| Register | Name | Stored as | Read | Write |
|---|---|---|---|---|
| 0, 2, 4 | VXY0, VXY1, VXY2 | two signed 16-bit halves | the word | the word |
| 1, 3, 5 | VZ0, VZ1, VZ2 | 16 bits | sign-extended | low 16 bits |
| 6 | RGBC | four bytes R, G, B, CODE | the word | the word |
| 7 | OTZ | 16 bits | zero-extended | low 16 bits |
| 8 | IR0 | 16 bits | sign-extended | low 16 bits |
| 9, 10, 11 | IR1, IR2, IR3 | 16 bits | sign-extended | low 16 bits |
| 12, 13, 14 | SXY0, SXY1, SXY2 | two signed 16-bit halves | the word | the word |
| 15 | SXYP | nothing | SXY2 | pushes the screen FIFO |
| 16 to 19 | SZ0 to SZ3 | 16 bits | zero-extended | low 16 bits |
| 20, 21, 22 | RGB0, RGB1, RGB2 | four bytes | the word | the word |
| 23 | RES1 | the word | the word | the word |
| 24 to 27 | MAC0 to MAC3 | 32 bits | the word | the word |
| 28 | IRGB | nothing | as ORGB | sets IR1 to IR3 |
| 29 | ORGB | nothing | IR1 to IR3 as 5-bit fields | ignored |
| 30 | LZCS | 32 bits | the word | the word, and LZCR follows |
| 31 | LZCR | the count | the count | ignored |

The rules that are not a plain store:

- **SXYP.** A read returns SXY2. A write moves SXY1 into SXY0, SXY2 into SXY1 and stores the value in SXY2, which is
  how the screen FIFO takes a point. A write to SXY0, SXY1 or SXY2 stores only that register.
- **IRGB and ORGB.** A write of IRGB sets IR1 to the low 5 bits times 80h, IR2 to bits 5 to 9 times 80h and IR3 to bits
  10 to 14 times 80h. A read of either returns the 5-bit fields back: each of IR1 to IR3 divided by 80h, a negative
  value counted as 0 and a value above 1Fh clamped to 1Fh, with no flag. A write of ORGB does nothing.
- **LZCS and LZCR.** LZCR holds the count of leading zero bits of LZCS when LZCS is positive or zero, and of leading one
  bits when it is negative, so it is 1 to 32. It is worked out when LZCS is written, so after power-on, before any
  write, it reads 0. A write to LZCR does nothing.
- **RES1.** It stores and returns the word it is given and has no other effect.
- **MAC1 to MAC3 and MAC0** are not saturated: a program that writes them reads the same word back.

### Control registers

| Register | Name | Stored as | Read | Write |
|---|---|---|---|---|
| 32 to 35 | RT11 to RT32, two per word | signed 16-bit halves | the word | the word |
| 36 | RT33 | 16 bits | sign-extended | low 16 bits |
| 37, 38, 39 | TRX, TRY, TRZ | 32 bits | the word | the word |
| 40 to 43 | L11 to L32, two per word | signed 16-bit halves | the word | the word |
| 44 | L33 | 16 bits | sign-extended | low 16 bits |
| 45, 46, 47 | RBK, GBK, BBK | 32 bits | the word | the word |
| 48 to 51 | LR1 to LB2, two per word | signed 16-bit halves | the word | the word |
| 52 | LB3 | 16 bits | sign-extended | low 16 bits |
| 53, 54, 55 | RFC, GFC, BFC | 32 bits | the word | the word |
| 56, 57 | OFX, OFY | 32 bits | the word | the word |
| 58 | H | 16 bits | sign-extended | low 16 bits |
| 59 | DQA | 16 bits | sign-extended | low 16 bits |
| 60 | DQB | 32 bits | the word | the word |
| 61, 62 | ZSF3, ZSF4 | 16 bits | sign-extended | low 16 bits |
| 63 | FLAG | bits 12 to 30 | bits 12 to 30 and bit 31 | below |

H is unsigned in the arithmetic of the projection commands but reads back sign-extended, so a program that writes
8000h to H reads FFFF8000h. The matrices are 3 by 3 with nine 16-bit elements packed in row order, two to a word; the
ninth element sits alone in the last word of its matrix.

## The command encoding

A COP2 command is the instruction word `0100101b` in bits 31 to 25 followed by a 25-bit field:

| Bits | Name | Meaning |
|---|---|---|
| 24 to 20 | | unused: the hardware ignores them, and some programs set them (the field `0140006h` for NCLIP) |
| 19 | sf | shift the MAC results right by 12 when set, by 0 when clear |
| 18 to 17 | MVMVA matrix | rotation, light, colour, or a fixed odd matrix |
| 16 to 15 | MVMVA vector | V0, V1, V2 or IR |
| 14 to 13 | MVMVA translation | TR, BK, FC or none |
| 12 to 11 | | unused |
| 10 | lm | clamp IR1 to IR3 at 0 instead of -8000h when set |
| 9 to 6 | | unused |
| 5 to 0 | command | the command number |

The engine decodes `op & 3Fh` as the command, takes sf from bit 19 and lm from bit 10 into locals, and stores neither.
A command number outside the 22 of the table below changes no register, FLAG included, and takes 5 cycles; that is a
choice of this engine, not a claim about the hardware, and the two numbers the seed treated as copies of other commands
(00h and 1Ah) are not copies here.

| Number | Name | Busy cycles | Number | Name | Busy cycles |
|---|---|---|---|---|---|
| 01h | RTPS | 15 | 1Ch | CC | 11 |
| 06h | NCLIP | 8 | 1Eh | NCS | 14 |
| 0Ch | OP | 6 | 20h | NCT | 30 |
| 10h | DPCS | 8 | 28h | SQR | 5 |
| 11h | INTPL | 8 | 29h | DCPL | 8 |
| 12h | MVMVA | 8 | 2Ah | DPCT | 17 |
| 13h | NCDS | 19 | 2Dh | AVSZ3 | 5 |
| 14h | CDP | 13 | 2Eh | AVSZ4 | 6 |
| 16h | NCDT | 44 | 30h | RTPT | 23 |
| 1Bh | NCCS | 17 | 3Dh | GPF | 5 |
| 3Eh | GPL | 5 | 3Fh | NCCT | 39 |

The cycle counts are the ones psx-spx lists for the opcode summary. They are measured constants, not derived ones.

## The numerics

Every value a command works on is a 64-bit signed integer, so the product of two 16-bit values, or of the 17-bit divide
result and a 16-bit value, is a widening multiply and cannot overflow. There is no signed right shift anywhere: a left
shift of a signed value is a multiply by 4096 or 16, and a right shift is an arithmetic shift built on unsigned
arithmetic that brings the sign bits in from the top. A command takes `sf` and `lm` from the command word into locals and
builds FLAG in a local, which replaces the register when the command ends.

- **MAC1 to MAC3** are 44-bit accumulators. Each term added to one is checked against minus 2 to the 43 and 2 to the 43
  minus 1: a value above sets the positive bit (30, 29, 28), a value below sets the negative bit (27, 26, 25). A chain
  of terms, as in a matrix row, checks and wraps to 44 bits after every addition. The register holds the low 32 bits of
  the result shifted right by 12 when `sf` is set.
- **MAC0** is a 32-bit accumulator with bit 16 for a value above and bit 15 for a value below. It is stored unshifted.
- **IR1 to IR3** take the low 32 bits of the shifted MAC and clamp to minus 8000h up to 7FFFh, or 0 up to 7FFFh when `lm`
  is set; a clamp sets bit 24, 23 or 22. **IR0** clamps to 0 up to 1000h and sets bit 12.
- **IR3 in the projection** has a quirk: its flag comes from the value shifted right by 12 and clamped as if `lm` were
  clear, but the register takes MAC3 clamped with the command's `lm`, whatever `sf` is.
- **The Z FIFO and OTZ** clamp to 0 up to FFFFh and set bit 18. **The screen FIFO** clamps SX and SY to minus 400h up to
  3FFh and sets bit 14 for X and bit 13 for Y.

### The divide

The projection divides H by SZ3 to a 17-bit result. If SZ3 times 2 is not above H, the result is 1FFFFh and bit 17
is set. Otherwise the divisor is shifted left until bit 15 is set, a reciprocal comes from a 257-entry table with one
Newton-Raphson step, and the product with the shifted dividend is rounded and clamped to 1FFFFh. The table is a literal
in `src/gte.c`; entry i is the larger of 0 and `(40000h / (i + 100h) + 1) / 2 - 101h`, and the last entry is 0. The test
`gte.unr_table` works the formula out in integers and compares all 257 bytes with the literal. The hardware cases pin
the table only at the entries their divisors reach, so the comparison is what pins the rest.

## The commands

Every command builds FLAG afresh, takes `sf` and `lm` from the command word and ends with FLAG recomputed; the table
says what each computes. A "colour" is the RGBC bytes R, G, B each times 10h, and "pushed" means the colour FIFO takes
the three results (below).

| Command | What it computes |
|---|---|
| RTPS | rotation, translation and perspective transform of V0: MAC1 to MAC3 and IR1 to IR3, SZ3, SX2 and SY2 from H over SZ3 and the offsets, then MAC0 and IR0 from the depth cue |
| RTPT | the same for V0, V1 and V2 in turn; MAC0 and IR0 come from the last vertex only, and the three points and depths fill the FIFOs |
| NCLIP | MAC0 as the signed area of the screen triangle SXY0, SXY1, SXY2 |
| AVSZ3, AVSZ4 | MAC0 as ZSF3 times SZ1 to SZ3, or ZSF4 times SZ0 to SZ3, and OTZ as that over 1000h |
| SQR | IR1 to IR3 squared into MAC1 to MAC3 and IR1 to IR3 |
| OP | the outer product of IR with the diagonal of the rotation matrix, into MAC1 to MAC3 and IR1 to IR3 |
| MVMVA | a matrix times a vector plus a translation, chosen by command bits 17 to 13, into MAC1 to MAC3 and IR1 to IR3 |
| NCS, NCT | the light matrix on V0 (NCS) or V0, V1 and V2 (NCT), then the colour matrix on the result plus the background colour; each result is pushed |
| NCCS, NCCT | as NCS and NCT, then the result times the colour is pushed |
| NCDS, NCDT | as NCS and NCT, then the result is blended with the far colour by IR0 and pushed |
| CC | the colour matrix on IR plus the background colour, times the colour, pushed |
| CDP | the colour matrix on IR plus the background colour, blended with the far colour by IR0, pushed |
| DCPL | IR times the colour, blended with the far colour by IR0, pushed |
| DPCS, DPCT | the colour blended with the far colour by IR0, pushed; DPCT does it for RGB0, RGB1 and RGB2 in turn, each time taking the oldest colour the pass before pushed out |
| INTPL | IR blended with the far colour by IR0, pushed |
| GPF | IR times IR0, pushed |
| GPL | MAC shifted left by 12 when `sf` is set, plus IR times IR0, pushed |

**The colour FIFO.** A push takes MAC1 to MAC3 shifted right by 4, clamps each to 0 up to FFh (bits 21, 20 and 19 for
red, green and blue), moves RGB1 into RGB0 and RGB2 into RGB1 and stores the three bytes with the code byte of RGBC in
RGB2. A blend with the far colour takes two steps: IR1 to IR3 first become the far colour less the operand, clamped as
if `lm` were clear, and then the operand plus IR0 times that; only the second step honours `lm`. The first step still
sets its saturation flags.

**MVMVA.** Bits 18 to 17 pick the matrix (rotation, light, colour, or the reserved fourth), bits 16 to 15 the vector
(V0, V1, V2, or IR1 to IR3) and bits 14 to 13 the translation (TR, BK, FC, or none). The sum for each row is the
translation shifted left by 12 and then the three products, checked against 44 bits after every addition.

## The quirks

These are the behaviours the hardware cases pin only thinly. Each has its own test, with its expected values worked
out by hand in the test's comment from the psx-spx pages.

- **The far colour.** MVMVA with the far colour as translation (bits 14 to 13 equal to 2) leaves the translation and the
  first product out of MAC and IR: they hold only the second and third products. FLAG and the saturation of IR on the
  way behave as if the full first term had been added. `gte.quirk.fc`.
- **The reserved matrix.** MVMVA with bits 18 to 17 equal to 3 multiplies by a matrix built from other registers: the
  first row is minus R times 10h, R times 10h and IR0, the second row is RT13 three times and the third is RT22 three
  times, with R the red byte of RGBC. `gte.quirk.mx3`.
- **IR3 and lm.** The projection's IR3 flag ignores `lm`, as the numerics section says. `gte.quirk.ir3_lm` runs a
  negative MAC3 with `lm` set and clear.
- **H.** H reads back sign-extended but divides as an unsigned 16-bit value. `gte.quirk.h_sign`.
- **Saturation at the end.** IR saturates once, on the finished sum of a row, not on its partial products, so a row whose
  products each exceed 16 bits but whose total fits sets no flag. `gte.quirk.clip_end`.
- **RTPT and IR0.** IR0 and MAC0 are those of the last vertex alone. `gte.quirk.rtpt_ir0`.
- **Unused bits.** The command field bits 20 to 24 are ignored: NCLIP issued as `0140006h`, with the fake number and
  bit 18 set as Wipeout 2097 does, gives the same 64 outputs and cycles as `6`. `gte.nclip_fake_bits` covers it,
  because the hardware cases never set those bits.
- **A number outside the table.** It changes no register and costs 5 cycles. `gte.unlisted_noop` runs all 42 such
  numbers with `sf` and `lm` both ways on registers that all differ and requires all 64 to read as before.

## The FLAG register

FLAG is control register 63. Bits 12 to 30 are written and read as stored; bits 0 to 11 always read 0. Bit 31 is not
stored: it is the OR of the error bits, bits 30 to 23 and bits 18 to 13, and the engine recomputes it when FLAG is
written and when a command ends.

| Bit | Meaning | Bit | Meaning |
|---|---|---|---|
| 30, 29, 28 | MAC1, MAC2, MAC3 larger than 43 bits, positive | 18 | SZ3 or OTZ saturated |
| 27, 26, 25 | MAC1, MAC2, MAC3 larger than 43 bits, negative | 17 | divide overflow |
| 24, 23, 22 | IR1, IR2, IR3 saturated | 16, 15 | MAC0 larger than 31 bits, positive, negative |
| 21, 20, 19 | colour FIFO R, G, B saturated | 14, 13 | SX2, SY2 saturated |
| 12 | IR0 saturated | 31 | any error bit set |

A command clears FLAG before it runs, so the register shows the faults of the last command only. The bits that make
up bit 31 are 30 to 23 and 18 to 13; bits 22 to 19 and 12 do not raise it.

## Timing

Each command keeps the engine busy for the cycles in the table above, and the engine reports that count when it runs a
command. The model is: the command computes at issue, the engine is then busy until a stamp, and a read of a result
(`MFC2`, `CFC2`, `SWC2`) or a new command issued before the stamp waits for it, which the CPU page's multiply and divide
stamp does the same way. A move into the engine (`MTC2`, `CTC2`, `LWC2`) does not wait. No wait is longer than 44 cycles,
the longest command. The stamp is saved with the state, bounded by the CPU's cycle count plus 44, and clamped again at
use. The CPU side of this is described on [the CPU page](cpu.md) when COP2 is connected to the engine.

## The hardware cases

The upstream ps1-tests repository carries 1,150 GTE cases captured on a console: 50 that only write and read back
registers and 50 for each of 22 commands. Each case holds the 64 register values written before the command and the 64
read back after it. `tools/gte2bin.py` turns the upstream table into one binary file,
`tests/vectors/gte/ps1tests-1150.bin`, with values unchanged: a 32-bit count, then 516-byte records of an opcode word
(`FFFFFFFFh` for a register-only case), 64 input words and 64 output words, all little-endian. The transcoder refuses
any input other than the pinned upstream blob, and the procedure to regenerate the file is in
[Maintaining](../maintaining.md). The values are hardware-captured per upstream, not verified here: this repository
trusts the table and checks the engine against it.

A case replays in one fixed order. The harness writes 0 to the registers 0 to 63, writes the 64 inputs in order,
runs the command unless the case is register-only, and reads the registers 0 to 63. Writing in order matters: LZCR
and the screen FIFO depend on it, and setting registers directly instead of writing them fails about a thousand cases.
The replay reaches the engine through three functions its caller supplies, so the same file can drive another
implementation. `gte.vectors.ps1tests` replays all 1,150 cases and
checks a histogram of the file: 50 register-only cases and 50 for each of the 22 commands. Its count is checked, so a
run that replays fewer fails. Three controls prove the run can fail: `gte.vectors.empty.self_test` and
`gte.vectors.truncated.self_test` feed it an empty file and the first 1,000 bytes, and `gte.vectors.flipped.self_test`
inverts one expected bit of case 600; each must fail and say why. `gte.regs.sweep` writes every register with edge
values (0, 7FFFh, 8000h, FFFFh, 7FFFFFFFh, 80000000h and FFFFFFFFh) and runs every command number from 00h to 3Fh with
sf and lm both ways, and checks that reads repeat and no command claims more than 44 cycles. The address and memory
sanitizers watch it, and between them they show that the engine touches nothing outside its struct.

## The seed and its notice

`src/gte.c` is derived from the interpreter's GTE file in the ares emulator, ISC-licensed, at the commit its first
lines name. It is ported to integer C: a `switch` for the dispatch instead of tables of functions, no signed shifts, and
an interface that takes only the engine struct. The SPDX line at the top of the file and the section "ares GTE seed" of
[the third-party notices](../../THIRD-PARTY-NOTICES.md) carry the licence text and the holders as the upstream names
them. [PROVENANCE](../../PROVENANCE.md) records what replaces the seed: an implementation written from psx-spx and the
hardware cases that matches the seed on every case in a zero-difference differential.
