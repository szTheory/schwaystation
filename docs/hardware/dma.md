# DMA

The DMA controller moves blocks of words between main RAM and the devices without the CPU: seven channels, a priority
register, an interrupt register, and a hold on the CPU's bus accesses while a transfer runs. This page covers the
engine in `src/dma.c`: the registers and the bits they store, how the next channel is chosen, how a transfer is modelled
in time, the first channel built (OTC, which builds an ordering table in RAM), what holds the CPU and what does not, and
the sources. Everything is integer arithmetic and behaves the same on every platform. The bus side, the block clock and
the stall in the run loop are in [the CPU page](cpu.md) and [the bus page](bus.md).

## State of the engine

The registers, arbitration, the interrupt rules, the unit engine, OTC and the CPU hold are built. Channels 0 to 5 keep
their registers and read them back, and arbitration sees them, but no transfer starts on them yet: a channel written
with bit 24 set stays busy until the block modes and the device ends are built. Every address of `1F801080h`-`1F8010FFh`
answers at every width, so the whole range counts as owned rather than unbuilt or a gap.

## The registers

| Address | Name | Stored bits | Reads |
|---|---|---|---|
| `1F801080h + 10h * n` | `D<n>_MADR` | 0-23 | the stored bits |
| `1F801084h + 10h * n` | `D<n>_BCR` | all 32 | the stored bits |
| `1F801088h + 10h * n` | `D<n>_CHCR` | 0, 1, 8-10, 16-18, 20-22, 24, 28-30 | the stored bits; OTC below |
| `1F80108Ch + 10h * n` | (unused) | none | 0 |
| `1F8010F0h` | `DPCR` | all 32 | the stored bits |
| `1F8010F4h` | `DICR` | 0-5, 15, 16-30 | the stored bits and the derived bit 31 |
| `1F8010F8h`, `1F8010FCh` | (unused) | none | 0 |

`n` is 0 to 6: MDEC in, MDEC out, GPU, CD-ROM, SPU, PIO, OTC. A byte or halfword access reaches its lane of the word;
a write merges the lane into the stored word and then applies the word's rules, except that a flag in `DICR` is
cleared only by a lane that was written. Power-on values are `DPCR` 07654321h and everything else 0. A power reset
restores them.

`CHCR` bit 24 is both the start and the busy bit: a program sets it to start and the hardware clears it when the
transfer ends. Bit 28 is cleared when the transfer begins. A write to the `CHCR` of the channel whose unit is running
changes nothing about that unit: its words have moved and it ends on its own event, so bit 24 stays set until then.
The hardware's way of stopping a running transfer by writing 0 is not modelled.

## DPCR and arbitration

`DPCR` holds, per channel, a 3-bit priority in bits `4n` to `4n+2` (0 is highest) and a master enable in bit `4n+3`.
Bits 28-31 are the CPU's priority and enable fields; they are stored and read back, and have no effect, which the
public dma-priority test measured by moving them against a running channel and finding no change. A channel with its
enable clear does not start.

A channel is waiting when bit 24 of its `CHCR` is set and, where the mode needs a trigger, bit 28 is too: OTC and
synchronisation mode 0 need it; the request modes start on their own request line. Among the waiting channels with
their enable set the lowest priority value goes first and a tie goes to the higher channel number (psx-spx, DMA
Channels; the public dma-priority test starts DMA2 and DMA6 together with one `DPCR` write and shows DMA6 finishing
first at equal priorities and when its value is lower). A `DPCR` write is also a start: a channel that was written
while its enable was clear starts when the enable is set, which is how the public `dma/dpcr` and nugget tests start a
channel. While a unit runs, a change to `DPCR` takes effect at the next unit boundary.

## DICR and the interrupt edge

`DICR` stores bits 0-5 (the per-slice interrupt bits, kept but unused), bit 15 (the force bit), bits 16-22 (the
per-channel interrupt enables) and bit 23 (the master enable). Bits 24-30 are the per-channel flags: a write of 1 clears
a flag, a write of 0 keeps it. Bit 31 is not stored. It reads as bit 15, or bit 23 with any flag set; the per-channel
enables do not enter it (psx-spx, DMA Channels, DICR).

A flag is set at the end of a channel's transfer only when the channel's enable (bit `16+n`) and the master enable
(bit 23) are both set. This differs from `I_STAT`, whose bits are set whatever the mask says. I_STAT bit 3 is raised
when bit 31 changes from 0 to 1, at a completion or at a `DICR` write, and never when a write keeps it at 1 or takes it
down, so a second channel finishing while the first is unacknowledged raises nothing new, and the program must clear
the flags and `I_STAT` before the next edge can show.

The public nugget `dma` tests give four `DICR` readings that the tests here reproduce on a channel's flag: 84840000h
(the enables and bit 23 set, channel 2's flag set), 84800000h (the enables written back to 0 with the flag still set),
04000000h (everything written to 0, flag kept, no bit 31) and 84008000h (only bit 15 written).

## The unit model

A transfer is cut into units. A unit moves its words at once and keeps the channel busy for the time the transfer
costs; the event at the end of the busy time finishes it. This is data-atomic and time-evented. The two things it
avoids are a transfer that runs to completion inside the register write, which for a 65,535-word burst is a hang of
emulated time inside one instruction, and a word-by-word interleave with the CPU, which costs tens of millions of
events a second. The data is already in RAM when the unit starts, so a program that reads it early reads the final
value, but every read that waits on the bus is held until the unit ends (below), so no program can see the difference.

The engine's state is the current channel, the end tick of its unit and a phase per channel (idle, or unit running).
The scheduler's next deadline for the engine is the end tick while a unit runs and none otherwise.

**The event step.** One call of the engine's event runs every unit that is due at the tick it is given: a unit whose end
tick is not past that tick finishes at its own end tick (the channel stops being busy, the flag is set where allowed,
the interrupt edge follows), and the channel that goes next then starts at that same tick, and so on until none is due.
Every finish clears a channel's bit 24, so the loop ends. Every return leaves the next deadline strictly greater than
the tick the call was given, either the end of a unit still running or none, so the scheduler and the run loop always
advance. A later stage of the engine that yields after a long list also returns with the channel still busy and no
interrupt, and keeps this rule.

A register access first brings the engine up to the access's own tick. A write then changes the registers and, with
nothing running, starts the channel that goes next; every write recomputes the scheduler's deadline. A register read
while a unit runs has already held the CPU until the unit's end, so the unit is finished as of that end before the read
and the program reads the completed state: a `CHCR` read right after a long transfer starts shows bit 24 clear.

## OTC

Channel 6 builds an ordering table: a list in RAM that a later linked-list transfer walks. It starts only with bits 24
and 28 of its `CHCR` set and its `DPCR` enable set. It writes the number of words in `BCR` (0 means 10000h), going
downward from `MADR`: each word holds the address of the word before it, and the last holds FFFFFFh. The address wraps
from 0 to FFFFFCh, a word whose address is past the RAM window is dropped, and `MADR` is left as written. A dropped word
is not a bus error and records no fault; what the hardware does to `DICR` bit 15 for such a word has not been measured,
so it is not built and nothing here asserts it.

OTC ignores the direction, step, synchronisation mode and chopping bits. The public ps1-tests `dma/otc-test` shows the
same table for the forward-step, from-RAM, block, linked-list and reserved settings and with chopping set, and the
tests here do the same with every one of those bits set.

`CHCR` of OTC stores only bits 24, 28 and 30, and bit 1 always reads as 1. A write of 70770703h with bit 24 clear reads
back as 50000002h, a write of 0 reads back as 00000002h, and so does any write whose bits 24, 28 and 30 are clear
(ps1-tests `dma/otc-test`). After a transfer started by 11000000h the register reads 00000002h.

**Cost.** A word costs one cycle, and the RAM side adds one cycle per 16 words for the row address, so a transfer of
`w` words ends `ceil(w * 17 / 16)` cycles after it starts: 4,096 words take 4,352 cycles and 10000h words take 69,632.
The row is documented in psx-spx (DMA Transfer Rates; DRAM Hyper Page mode). The nugget dma-priority test measured 4,358
cycles for 4,096 words on two consoles, 6 more than the model, which is the cost of the register write path and not
modelled; a test that pins a range around the model's value is the cross-check, and a measured row is not claimed.

## What holds the CPU

While a unit transfers, the CPU may run on the instruction cache, the scratchpad, COP0 and the GTE, may use the
four-entry write queue for RAM and I/O writes, and is stopped by anything else that needs the bus: a read from RAM or an
I/O register, an instruction fetch that misses the cache, and a fifth write into a full queue (psx-spx, DMA Channels,
CPU Operation during DMA; the same figures appear in the public dma-priority results: a RAM-loading loop waits for the
whole transfer, a cached ALU loop and a scratchpad loop do not).

In the model the bus tells the engine about every read and fetch that reaches RAM, the I/O page, an expansion region
or the BIOS, and the CPU's write queue tells it when a store finds the queue full. If a unit is transferring and the
access happens before the unit's end, the tick the CPU is held until becomes the later of its old value and the unit's
end. The CPU then does not execute until that tick: the run loop advances the tick without running an instruction, within
the budget and the frame. This is a freeze until the end, not a charge on the instruction that waited, because a unit
can hold the bus for tens of thousands of cycles and charging that to one instruction would pass the overrun bound
of the run loop. Stores are buffered, so the first four stores to RAM after the queue is empty do not wait and the
fifth does; stores to the scratchpad are not behind the queue and never wait.

A channel that is only waiting for its request line is not a running unit and never holds the CPU. The hold is checked
against the tick of the access, so a read that comes after the unit's end waits for nothing.

## Not built yet

The synchronisation modes and device ends of channels 0 to 5, chopping, linked lists and the 1000h-node yield, and the
fault for a mode a channel cannot use come with later stages of the engine; this page grows with them. The hardware's
stop by writing 0 to `CHCR` while busy, and the behaviour of bit 29, are not modelled.

## Sources

- psx-spx, DMA Channels page (`docs/dmachannels.md` of psx-spx/psx-spx.github.io at 6d7d1bc106a7): the registers, the
  DPCR and DICR layout, the interrupt edge, the transfer rates, the DRAM row and the CPU rules during DMA.
- ps1-tests (JaCzekanski/ps1-tests at f727802fead1, MIT): `dma/otc-test` for OTC's table, ignored bits and readback, and
  `dma/dpcr` for starting a channel by enabling it.
- nugget (pcsx-redux/nugget at 9db6d88d23c4, MIT): `tests/dma-priority/RESULTS.md` for the priority order, the CPU field
  and the OTC timing, and `tests/dma/dma.c` for the `DICR` readings.

These are hardware-captured per upstream and not verified here. The cycle rows above are documented in psx-spx; the
model's figures are labelled doc, not hardware.
