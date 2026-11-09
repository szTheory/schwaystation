# The bus

Every load, store and instruction fetch the CPU makes goes through one module, `src/bus.c`, which decides what answers
the address, what the access costs and whether it is an error. The interrupt registers I_STAT and I_MASK live beside
it in `src/irq.c`. Everything is integer arithmetic and behaves the same on every platform. Facts come from the psx-spx
Memory Map and Memory Control pages and from sections 2, 3 and 6 of `.planning/preparation/PSX-SYSTEM.md`, which also
names the sources they were checked against; the cost model is in section 2 of `.planning/preparation/ARCHITECTURE.md`.
This page covers decode, memory control and costs, devices not built yet, gaps, the debug ports, the instruction cache,
the write queue and the interrupt registers. The cache and the queue sit between the CPU and the bus, in
`src/cpu.c` and `src/cpu_exec.inc`, and the bus itself knows nothing of them.

## Segments and the physical map

The CPU hands the bus the virtual address. The top three bits pick the segment (PSX-SYSTEM section 2, Segments): 0 to 3
are KUSEG, 4 is KSEG0, 5 is KSEG1 and 6 and 7 are KSEG2. KUSEG, KSEG0 and KSEG1 all map to the physical address
`addr & 1FFFFFFFh`, so a KUSEG address above 512 MiB lands on the same device as the one below it. KSEG2 answers only
`FFFE0130h`, the cache-control word, by word reads and writes; every other KSEG2 address is a bus error. The bus keeps
the word in `cache_ctrl`. The CPU reads two of its bits, TAG (bit 2) and IS1 (bit 11), for cache isolation, below; no
other bit has a meaning here.

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

The window reads its image byte by byte, little-endian. With no image from the host, power-on maps the core's own stub
kernel (524 bytes at `BFC00000h`); a mapped image survives a power reset. An access that ends past the image is a bus
error naming the BIOS, so a runaway never slides through padding. The window is 512 KiB, with no mirror above it. A
write to the ROM is dropped and is not an error. Reads cost what the BIOS delay register says, below.

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

## Instruction cache

The I-cache is 4 KiB, direct mapped: 256 lines of 4 words (PSX-SYSTEM section 2, I-cache; psx-spx Memory Map). Address
bits 11:4 pick the line and bits 3:2 the word. A line holds a tag, the physical address bits 31:12, and one valid bit
per word. `src/cpu.c` keeps the tags, the words and the valid bits in the CPU state, so a saved state restores them and
a power reset empties them: every line starts invalid.

- **Which fetches use it.** A fetch in KUSEG or KSEG0 does, except in the BIOS window (`1FC00000h`-`1FC7FFFFh`), which is
  never cached. A fetch in KSEG1 bypasses it and neither reads nor changes a line. Only fetches are cached; a load
  never fills a line. Cache control's enable bits are not modelled, so the cache is always on.
- **A hit** returns the word held and costs nothing beyond the instruction's own cycle.
- **A miss** is a fetch whose tag differs from the line's, or whose word is not valid. The line is filled from the missed
  word to its last word. On another tag the line is taken over and only the filled words are valid; on the same tag the
  words already valid stay valid. The psx-spx page says a tag hit on an invalid word refills the whole line; the model
  refills from the missed word on, which gives the same words for the next fetch of a straight-line run.
  IBLKSZ (bits 8 and 9 of cache control) is ignored: psx-spx says IBLKSZ = 0 cuts a fill to two words, and the model
  always fills to the line's end.
- **Fill cost.** The missed word costs what the bus charges for it, a RAM read being 5 cycles, and each further word one
  more cycle, the burst rate of RAM (4 cycles plus one per word; ARCHITECTURE section 2, Costs). That is an
  approximation for any other device.
- **Stores never invalidate.** A store to RAM leaves the line as it is, so code that rewrites itself runs the old words
  until the line is refilled; a program flushes the cache first (PSX-SYSTEM section 2).
- **A bus error** on the missed word leaves the line as it was and is the first fault. A bus error on a word after it
  ends the fill there, leaves those words invalid and records no fault, because the CPU never asked for them.
- **The vector tests** replay each instruction against a recorded bus with no cache and no queue; see
  [cpu.md](cpu.md).

**Isolation.** While SR.Isc (bit 16) is set, a load or store in KUSEG, KSEG0 or KSEG1 goes to the cache and never reaches
the bus, whichever segment it names; the cache-control word in KSEG2 still does, so the BIOS can restore it. The line is
the one address bits 11:4 name and the word bits 3:2 (PSX-SYSTEM section 2, Cache isolation, write queue).

- A load returns the bytes it covers of that word, whether or not the tag matches.
- A store writes the bytes it covers into the word, unless cache control has both TAG (bit 2) and IS1 (bit 11) set. Then it
  writes the line's tag, the datum's bits 31:12, and sets the four valid bits to the datum's low four bits. The BIOS'
  FlushCache does exactly this: it sets cache control to `0804h`, isolates the cache, stores 0 to each of the 256 lines
  and restores SR and cache control, which leaves every line invalid. `bus.icache_isolation` runs that sequence.
- The sources add that, with TAG clear, an isolated access reaches the scratchpad instead. The model does not do that:
  every isolated access goes to the instruction-cache arrays.

## Write queue

The CPU can hold 4 stores in flight, and a fifth waits until one has gone (PSX-SYSTEM section 2, Cache isolation, write
queue; psx-spx Memory Map). The model keeps a count of them, `wq_count`, 0 to 4, in the CPU state. Every store reaches the
bus at once and lands in order, so the queue never changes what a store does, only what it costs.

- A store that reaches the bus joins the queue. A store to the scratchpad does not: it is not behind the queue.
- With 4 queued, the next store waits for the oldest to leave, which costs one drain, and the queue stays at 4.
- Any access in KSEG1, a load, a store or a fetch, first drains the whole queue, so uncached memory and devices see the
  stores before it. Each store still queued costs one drain, and an empty queue costs nothing. A cached read leaves the
  queue alone.
- An exception does not touch it, and neither does the instruction after. The queue empties only through the rules
  above, never by time passing.
- The vector tests bypass it: with a replay log attached each store's bus cycle is emitted at its issue point, in the
  upstream order, and the count does not change.

A drain costs 4 cycles. That figure is an estimate: no source gives the drain rate, and ares charges a RAM write 1 cycle
with an unbounded queue (PSX-SYSTEM section 3, C4). Stall timing is therefore documented and not pinned: the tests assert
the depth, the order and the drain, and that a wait costs more than none, never the cycles of a wait. Retiring a queued
store as time passes is not modelled, so a long run of stores to cached RAM pays for the wait on each store after the
fourth until a KSEG1 access drains it; a measurement against hardware would settle a better rule.

`bus.write_queue_depth`, `bus.write_queue_drain`, `bus.write_queue_exception` and `bus.write_queue_state` pin these
rules, and `bus.icache_fill`, `bus.icache_valid_bits` and `bus.icache_isolation` the cache.

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
