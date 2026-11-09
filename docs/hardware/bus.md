# The bus

Every load, store and instruction fetch the CPU makes goes through one module, `src/bus.c`, which decides what answers
the address, what the access costs and whether it is an error. The interrupt registers I_STAT and I_MASK live beside
it in `src/irq.c`. Everything is integer arithmetic and behaves the same on every platform. Facts come from the psx-spx
Memory Map and Memory Control pages and from sections 2, 3 and 6 of `.planning/preparation/PSX-SYSTEM.md`, which also
names the sources they were checked against; the cost model is in section 2 of `.planning/preparation/ARCHITECTURE.md`.
This page covers decode, memory control and costs, devices not built yet, gaps, the debug ports and the interrupt
registers. The instruction cache and the write queue are added when they land.

## Segments and the physical map

The CPU hands the bus the virtual address. The top three bits pick the segment (PSX-SYSTEM section 2, Segments): 0 to 3
are KUSEG, 4 is KSEG0, 5 is KSEG1 and 6 and 7 are KSEG2. KUSEG, KSEG0 and KSEG1 all map to the physical address
`addr & 1FFFFFFFh`, so a KUSEG address above 512 MiB lands on the same device as the one below it. KSEG2 answers only
`FFFE0130h`, the cache-control word, by word reads and writes; every other KSEG2 address is a bus error. The bus keeps
the word in `cache_ctrl` and gives it no further meaning yet.

| Physical range | Answers |
|---|---|
| `00000000h`-`007FFFFFh` | main RAM, 2 MiB, inside the window `RAM_SIZE` selects |
| `1F000000h`-`1F7FFFFFh` | first expansion region (DEV0) |
| `1F800000h`-`1F8003FFh` | the scratchpad, 1 KiB |
| `1F801000h`-`1F801FFFh` | the I/O page, below |
| `1F802000h`-`1F803FFFh` | second expansion region (DEV8) |
| `1FA00000h`-`1FBFFFFFh` | third expansion region (DEV1) |
| `1FC00000h`-`1FC7FFFFh` | the BIOS window (DEV2), 512 KiB |
| anything else | a bus error |

An address no device owns is never open bus: a read or fetch there is a bus error, and so is a write, which is also
dropped (PSX-SYSTEM section 3). Each bus error records the first fault, with the device it names, the address, the width
and the program counter, and the access returns non-zero with the value 0. Only the first fault is kept.

## RAM and its mirrors

`RAM_SIZE` at `1F801060h` says how the two DRAM banks are laid out (psx-spx Memory Control, DRAM_CTRL). The banks follow
each other from address 0. Bank 0 (`/RAS0`) is `1 MiB << (bit 9 * 2 + bit 11)` and bank 1 (`/RAS1`) is
`1 MiB << (bit 8 * 2 + bit 11)`, present only when bit 10 is set. The console has 2 MiB of RAM, so a bank larger than
that mirrors it and a bank of 1 MiB wraps at 1 MiB; a second bank maps onto the RAM after the first. An address past
the banks is a bus error naming RAM, or no device when it is also past the 8 MiB window.

At power-on the register holds `00000B88h`: bits 9 and 11 set, so an 8 MiB bank holds the 2 MiB four times, and bit 10
clear, so there is no second bank. The same word is what the BIOS writes first (PSX-SYSTEM section 3). `0888h` is a
single 2 MiB bank with no mirror, and `0000h` a 1 MiB bank. Only the low 16 bits of the register are kept.

An access that crosses a bank edge or the end of the window is checked byte by byte, so it is a bus error and writes
nothing; an aligned access cannot cross one. A read of RAM costs 4 cycles plus one per word, so 5 for any width, and a
write costs 1 (ARCHITECTURE section 2, Costs).

## The scratchpad

The 1 KiB at `1F800000h` is the data-cache SRAM. It answers in KUSEG and KSEG0 and is a gap in KSEG1, and an
instruction fetch from it is a bus error in every segment (PSX-SYSTEM section 2, D-cache). An access that starts inside
it and ends outside it is a bus error naming the scratchpad. It costs no extra cycle.

## The BIOS window

The window reads the image the host supplied, byte by byte, little-endian. The core does not own the image: a power
reset keeps the host's pointer. While no image is mapped, or when an access ends past the image, the access is a bus
error naming the BIOS. The window is 512 KiB, with no mirror above it. A write to the ROM is dropped and is not an
error. Reads cost what the BIOS delay register says, below.

## Memory control

Nine registers at `1F801000h`-`1F801020h` set where the expansion regions sit and how fast each port is (psx-spx Memory
Control; PSX-SYSTEM section 3). A write reaches only the bytes it covers, so a byte or halfword write is a lane of the
register, and a read returns its lane.

| Address | Register | Kept bits | Power-on |
|---|---|---|---|
| `1F801000h` | DEV0 base | 0-23; bits 24-31 read `1Fh` | `1F000000h` |
| `1F801004h` | DEV8 base | 0-23; bits 24-31 read `1Fh` | `1F802000h` |
| `1F801008h` | DEV0 delay and size | all but 21-23 and 28 | `00000FFFh` |
| `1F80100Ch` | DEV1 delay and size | all but 21-23 and 28 | `00000FFFh` |
| `1F801010h` | DEV2 (BIOS) delay and size | all but 21-23 and 28 | `00000FFFh` |
| `1F801014h` | DEV4 (SPU) delay and size | all but 21-23 and 28 | `00000FFFh` |
| `1F801018h` | DEV5 (CD-ROM) delay and size | all but 21-23 and 28 | `00000FFFh` |
| `1F80101Ch` | DEV8 delay and size | all but 21-23 and 28 | `00000FFFh` |
| `1F801020h` | COM_DELAY | 0-19 | `00031125h` |

The delay registers start at the maximum delays: both delay fields `Fh` and all four COM periods selected, on an 8-bit
bus. The BIOS' first stores write `0013243Fh` to DEV2 and `0B88h` to `RAM_SIZE`. Bit 28, the address-error flag, is
never set here, so a write to it changes nothing. The addresses between registers (`1F801024h`-`1F80103Fh`,
`1F801064h`-`1F80106Fh`) are gaps.

**Costs.** Each port (DEV0, DEV1, DEV2, DEV4, DEV5, DEV8) has six costs in CPU cycles: read and write at widths 1, 2
and 4. A write to the port's delay register recomputes that port's six, and a write to COM_DELAY recomputes all
of them, so the same value written twice gives the same table. The table is derived, so it is not part of a saved
state; loading a state rebuilds it. The formula is the one psx-spx gives (Memory Control), with delay the read or
write field plus one:

```
1ST = SEQ = 0, MIN = 0
COM0 selected (bit 8):  1ST, SEQ += COM0 - 1
COM2 selected (bit 10): 1ST, SEQ += COM2
COM3 selected (bit 11): MIN = COM3
if 1ST < 6: 1ST += 1
1ST, SEQ += delay + 2
1ST = max(1ST, MIN + 6), SEQ = max(SEQ, MIN + 2)
```

A byte or halfword on a 16-bit bus (bit 12 set) costs 1ST, a word costs 1ST + SEQ; on an 8-bit bus a halfword is
1ST + SEQ and a word 1ST + 3 SEQ. The hold period (COM1, bit 9) is not in the published formula and adds nothing.

Worked example, BIOS read with DEV2 = `0013243Fh` and COM_DELAY = `00031125h`: the read field is 3, so delay 4; bit 10 is
set, so COM2 = 1 is added; bits 8, 9 and 11 are clear. 1ST starts at 1, is below 6 so becomes 2, and with delay + 2 = 6
is 8. SEQ is 1 + 6 = 7. A word on the 8-bit bus costs 8 + 3 * 7 = 29 cycles, the figure PSX-SYSTEM section 3 gives for
a BIOS word. The write side, delay 16, gives 1ST 20 and SEQ 19. At power-on every port is slow: 24 cycles a byte, 47 a
halfword and 93 a word.

Ports without a delay register of their own (memory control, `RAM_SIZE`, the interrupt registers, the serial, DMA, timer,
GPU and MDEC ports) cost one cycle, which is approximate. The SPU and the CD-ROM use their ports' costs.

## Devices not built yet

The DMA registers (`1F801080h`-`1F8010FFh`), the timers (`1F801100h`-`1F80112Fh`), the serial ports
(`1F801040h`-`1F80105Fh`), the CD-ROM (`1F801800h`-`1F801803h`), the GPU (`1F801810h`-`1F801817h`), the MDEC
(`1F801820h`-`1F801827h`) and the SPU (`1F801C00h`-`1F801FFFh`) are documented but not modelled. At any width a read
returns 0 and a write is dropped, with no fault. Each access is counted per device in `unbuilt_n`, saturating at the
largest 32-bit value, and the last one is kept as the device, address, width and program counter. Both are part of the
state. A program that polls a register of one of these devices therefore spins with a count that grows and a last
program counter that names the loop. A later release replaces each case with the device itself.

The rest of the I/O page between and around those ranges is a gap. `core.bus_unbuilt` sweeps all of `1F801000h`-
`1F801FFFh` at widths 1, 2 and 4, for reads and writes, and requires every address to be owned, unbuilt or a gap,
including the edges such as `1F80105Fh` against `1F801060h`.

## Expansion regions and the debug ports

A read or fetch of the first (`1F000000h`-`1F7FFFFFh`) or third (`1FA00000h`-`1FBFFFFFh`) expansion region is a bus error
naming it, because nothing is plugged in; a write is dropped and costs the port's write cost. Of the second region
(`1F802000h`-`1F803FFFh`) only two addresses answer, and every other address is a bus error naming it.

| Address | Port |
|---|---|
| `1F802080h` | a byte written here is queued for the host to read as TTY output |
| `1F802082h` | a byte written here is the exit code, and the guest has asked to stop |

Both ports act only while the profile's debug-port flag is set, and only for a byte write; a wider write, or any
access with the flag clear, answers and does nothing. A read returns 0. A retail profile leaves the flag clear. The
TTY ring holds 4 KiB, and a byte that does not fit is dropped and counted.

## Interrupt registers

I_STAT at `1F801070h` and I_MASK at `1F801074h` each hold bits 0-10, one per source (PSX-SYSTEM section 6); the rest
read 0, and the word `1F801078h`-`1F80107Fh` is a gap.

- A source sets its I_STAT bit on its 0 to 1 edge, through `schway__irq_raise`. Raising a bit that is already set
  changes nothing, and a number above 10 is ignored.
- A write to I_STAT acknowledges: a 0 clears the bit and a 1 leaves it. A narrow write reaches only its own bytes.
- A write to I_MASK keeps the bits written, 0 to 10 only.
- The line the CPU sees is `(I_STAT & I_MASK) != 0`. It is not latched. At each raise and each write to either register,
  CAUSE bit 10 is set to the line's new value, with no delay, so a read of CAUSE right after the change sees it. CAUSE
  bits 8 and 9 are software latches and the interrupt registers never touch them.

`bus.irq_edge`, `bus.irq_line` and `bus.irq_cause_bit` pin these rules. Whether and when the CPU takes the interrupt is
the exception logic's business and is described with it.
