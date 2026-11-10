# DMA

The DMA controller moves blocks of words between main RAM and the devices without the CPU: seven channels, a priority
register, an interrupt register, and a hold on the CPU's bus accesses while a transfer runs. This page covers the
engine in `src/dma.c`: the registers and the bits they store, how the next channel is chosen, how a transfer is modelled
in time, OTC (which builds an ordering table in RAM), the block modes and the device ends of the other six channels,
chopping, linked lists and the guard that keeps a looping list from hanging a run, the cost of a word, what holds the CPU
and what does not, how hostile programs stay bounded, and the sources. Everything is integer arithmetic and behaves the
same on every platform. The bus side, the block clock and
the stall in the run loop are in [the CPU page](cpu.md) and [the bus page](bus.md).

## State of the engine

The registers, arbitration, the interrupt rules, the unit engine, OTC, the burst and request-block modes with their
device ends, chopping, linked lists, the costs and the CPU hold are built. A channel written with the reserved mode
keeps its registers and stays busy. Every address of `1F801080h`-`1F8010FFh` answers at
every width, so the whole range counts as owned rather than unbuilt or a gap.

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
The one write that does more is the word 0, which the public chain-looping test makes between its cases to stop a
looping list: it stops a channel that has no unit in flight (in a CPU window or waiting for its next node or block) at
once, and a channel with a unit in flight at the end of that unit, in both cases with no flag and no interrupt. This is
the procedure of that test and nothing more; every other write to a busy channel is left unmodelled, and the channel
stays busy.

## DPCR and arbitration

`DPCR` holds, per channel, a 3-bit priority in bits `4n` to `4n+2` (0 is highest) and a master enable in bit `4n+3`.
Bits 28-31 are the CPU's priority and enable fields; they are stored and read back, and have no effect, which the
public dma-priority test measured by moving them against a running channel and finding no change. A channel with its
enable clear does not start.

A channel is waiting when bit 24 of its `CHCR` is set and its mode has what it needs: OTC and synchronisation mode 0
need bit 28 too; mode 1 needs the device's request line to be high; mode 2 (the linked list) needs nothing more, the
device being taken to be ready, and a chopped burst or list waiting for its next window or node is waiting. Among the
waiting channels with
their enable set the lowest priority value goes first and a tie goes to the higher channel number (psx-spx, DMA
Channels; the public dma-priority test starts DMA2 and DMA6 together with one `DPCR` write and shows DMA6 finishing
first at equal priorities and when its value is lower). A `DPCR` write is also a start: a channel that was written
while its enable was clear starts when the enable is set, which is how the public `dma/dpcr` and nugget tests start a
channel. While a unit runs, a change to `DPCR` takes effect at the next unit boundary: the engine arbitrates afresh
whenever a unit ends, so raising a waiting channel's priority while another runs makes it the next one.

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

The engine's state is the current channel, the end tick of its unit and a phase per channel (idle, unit running, between
sync mode 1 blocks, in a CPU window, waiting for the next window or node, or stopping). The scheduler's next deadline
for the engine is the end tick while a unit runs or a CPU window lasts and none otherwise.

**The event step.** One call of the engine's event runs every unit that is due at the tick it is given: a unit whose end
tick is not past that tick finishes at its own end tick (the channel stops being busy, the flag is set where allowed,
the interrupt edge follows), and the channel that goes next then starts at that same tick, and so on until none is due.
Every finish clears a channel's bit 24, so the loop ends. Every return leaves the next deadline strictly greater than
the tick the call was given, either the end of a unit still running or none, so the scheduler and the run loop always
advance. A list that yields after 1000h nodes also returns with the channel still busy and no interrupt, and keeps this
rule (below).

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

## Sync mode 0, sync mode 1 and the request line

Channels 0 to 5 run two modes, chosen by `CHCR` bits 9-10. Bit 0 gives the direction (1 moves words from RAM to the
device, 0 from the device to RAM) and bit 1 the step (0 forward by 4, 1 backward by 4). `MADR` wraps at 24 bits, from
FFFFFCh to 0 and back, and is left after the last word moved. A word whose RAM address is past the RAM window is
dropped and reads as 0, with no fault.

**Sync mode 0** moves `BCR`'s low half words (0 means 10000h) as one unit, and starts when bits 24 and 28 are set and
the `DPCR` enable is set. It does not wait for the request line.

**Sync mode 1** moves one block of `BCR`'s low half words per raise of the channel's request line, and `BCR`'s high half
counts the blocks (0 means 10000h). Each block is a unit. A block takes the request with it: the engine clears the
channel's request bit when it starts the block, and the device raises the line again when it can take or give another.
Between blocks the channel stays busy with no unit running, so the CPU runs, and the next block starts as soon as the
line is high again and arbitration picks the channel. A channel that waits for the line holds nothing: it never stalls
the CPU, whatever its `CHCR` says. The last block ends the transfer: the flag and the interrupt follow as for any unit,
and not before. A raise of the line while another unit runs is picked up at that unit's end.

The line is a latched level per channel, set by `schway__dma_dreq`; the devices call it as they are built. The reserved
mode has no unit yet.

## Chopping

With `CHCR` bit 8 set, a synchronisation mode 0 burst is cut into DMA windows of 2^n words, n from bits 16-18, with a
CPU window of 2^m cycles, m from bits 20-22, between one window and the next (psx-spx, DMA Channels, CHCR). Each window
is a unit; after each but the last the channel stays busy for the CPU window, during which the CPU runs free and the
arbitration may pick another channel at the window's end. A window of `dw` words costs its words as any unit does, plus
6 cycles of setup, and the CPU window is the smaller of `2^m` and `32 * dw` cycles:

    window = ceil(17 * dw / 16) + 6        (at one cycle per word)
    gap    = min(2^m, 32 * dw)

The 6 and the cap of 32 cycles per word are fitted to the 64 rows of the public ps1-tests `dma/chopping` hardware log (2,048
words from RAM to the GPU channel, every window size from 1 to 128 words against every CPU window from 1 to 128 cycles).
They are labelled `derived`. The tests compare the model with the rows in bands: every row within 12 %, and the mean of
the signed errors within 2 %. The model's largest error is 10.4 % (1-word windows with a 1-cycle CPU window), the mean
signed error is about 1.0 % and the mean absolute error about 2.0 %. A fit to one log is evidence of the fit and says
nothing about other consoles or other transfers, which is why the bands are wide.

## Linked lists

Synchronisation mode 2 walks a list in RAM. `MADR` names a node's header word: its top byte is the number of words that
follow it and its low 24 bits are the address of the next header. Each node is a unit: the engine reads the header, sends
the words after it to the channel's device end (whatever the direction bit says; the header itself is not sent), and
leaves `MADR` at the next address. A next address with bit 23 set ends the list and is left in `MADR`, so the usual end
marker `FFFFFFh` reads back as `FFFFFFh`. A list whose first header already holds the marker finishes at once with the
flag and the interrupt. Only bit 24 of `CHCR` starts a list; bit 28 is cleared if set. A header or word address past the
RAM window reads 0.

Between two nodes the channel stays busy and the CPU runs free for a short window. A node costs 20 cycles of header fetch
and setup plus its words, and the CPU window is 6 cycles. Both figures are fitted to the public `dma/chain-looping`
hardware log, in which a loop that takes 16,040 cycles alone takes 25,632 next to a self-referencing list, about 1.6
times as long; they are labelled `derived`. The test runs a similar loop of its own against a list of one node and a
list of two that name themselves and accepts any slowdown from 1.4 to 1.8 times: the figure is calibrated to one log
and one loop, not measured on a console.

## The 1000h-node guard

A list that never ends does not finish. The public `dma/chain-looping` log shows a self-referencing list leaving the
channel busy with no flag and no interrupt, and so does the model: the guard is a yield, not an abort. The engine counts
the nodes since the transfer started (`nodes_done`, which wraps). When the 1000h node, the 2000h node and so on
ends, the step that is catching the engine up returns even if more nodes are due: the CPU window after it is
extended past the tick the step was given, so the next node starts later than that tick, the channel stays busy, and no
flag or interrupt is raised. The step contract therefore holds without exception: every return leaves the next deadline
strictly after the tick, and a call with a tick far ahead of the engine does 1000h nodes and no more. In a normal run a
node takes 26 cycles, so the guard is reached about every 100,000 cycles and costs nothing visible. A frame of a looping
list still ends with the frame stop, the channel still busy.

## Hostile programs and the frame bound

A program may set every channel to its largest transfer (`BCR` of FFFFFFFFh), or point a list at itself. Every unit is
bounded: a burst is at most 10000h words, a block at most FFFFh words, a chopping window at most 128 words and a node at
most 255 words and a header.
The longest unit is 2,691,328 cycles, the bound `SCHWAY__DMA_BUSY_MAX` that a state load enforces. The frame end
preempts the engine: a run never goes past the end of the frame for a unit, the unit's end stays in the state as a
deadline, and the next frame carries the remainder, so slicing the same time into frames or into smaller runs gives the
same state. A CPU read that waits on a unit freezes the CPU only up to the frame's end, so the overrun past a frame stays
one instruction. The tests start all seven channels at once with `BCR` FFFFFFFFh and a self-referencing list, with no
program and with a program that reads RAM in a loop, and check that four frames each end with the frame stop at the
frame's own length, with an engine state that saves and loads.

A hostile program can also fill the audio ring faster than a host drains it: up to four frames of audio can be dropped
and counted in `audio_dropped`. That is accepted and not an error.

A sweep of every DMA register offset at every width with four edge values, each access from a power reset, finds no fault
and no difference between two passes. It leaves out the `CHCR` word FFFFFFFFh of a channel other than OTC, which selects
the reserved mode with the start bit, and runs under the sanitizer lanes.

## The device ends

The device side of a channel is two functions in `src/dma.c`, one for words going to the device and one for words coming
from it, each a single `switch` on the channel with a case per channel. A case moves one word and returns the extra busy
cycles the device asks for. A device that is not built is a stand-in in its case: the sink counts the words it is given
in `dma.words` and folds their bytes, little-endian, into a CRC-32 per channel in `dma.crc`, both kept in the state from
power-on; the source supplies 0, as every unbuilt device does. The stand-ins ask for no extra cycles. Building a device
replaces its two cases, and nothing else in the engine changes; there is no table of callbacks and no function pointer
in the state.

## Costs

| Row | Cycles | Label |
|---|---|---|
| MDEC in, MDEC out, GPU and OTC, per word | 1 | `doc` |
| SPU, per word | 4 | `doc` |
| CD-ROM, per word | 24 | `doc` |
| PIO, per word | 20 | `doc` |
| RAM side, per 16 words | 1 | `doc` |
| sync mode 1, per block | 11 | `derived` |
| a chopped burst's window | 6 and a CPU window of at most 32 per word | `derived` |
| a linked list's node | 20 and a CPU window of 6 | `derived` |

The per-word rows are the psx-spx DMA Channels page, section "DMA Transfer Rates"; the RAM row is its "DRAM Hyper Page
mode" section (17 cycles per 16 words). A unit of `n` words takes `n * rate + ceil(n / 16)` cycles, which is
`ceil(n * 17 / 16)` at one cycle per word, and a sync mode 1 block of `bs` words takes that plus 11, so
`blocks * (ceil(17 * bs / 16) + 11)` for a transfer at the fastest rate. The 11, the chopping model and the node figures
are fitted to rows of public hardware logs and are labelled `derived`: a fitted figure is evidence that the model fits the log, not
evidence of the hardware. Tests pin the documented rows exactly and the fitted ones by their formula.

The CD-ROM row is the BIOS default, 24 cycles per word. Most games write the delay setting that makes it 40 (the same
page); that cost is not modelled yet, and the CD-ROM device end selects 40 from that setting when it is built. Nothing
in the engine reads the delay register. The slowest unit the engine bounds is 10000h words at 40 cycles.

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

The devices behind channels 0 to 5 and the fault for a mode a channel cannot use come with later stages. Every write to
a busy channel's `CHCR` other than the word 0, and the behaviour of bit 29, are not modelled.

## Sources

- psx-spx, DMA Channels page (`docs/dmachannels.md` of psx-spx/psx-spx.github.io at 6d7d1bc106a7): the registers, the
  DPCR and DICR layout, the interrupt edge, the transfer rates, the DRAM row and the CPU rules during DMA.
- ps1-tests (JaCzekanski/ps1-tests at f727802fead1, MIT): `dma/otc-test` for OTC's table, ignored bits and readback,
  `dma/dpcr` for starting a channel by enabling it, `dma/chopping` (`psx.log` lines 68-131) for the chopping bands and
  `dma/chain-looping` for a list that does not end, the CPU's slowdown beside it and the stop by writing 0.
- nugget (pcsx-redux/nugget at 9db6d88d23c4, MIT): `tests/dma-priority/RESULTS.md` for the priority order, the CPU field
  and the OTC timing, and `tests/dma/dma.c` for the `DICR` readings.

These are hardware-captured per upstream and not verified here. The cycle rows above are documented in psx-spx; the
model's figures are labelled doc, not hardware.
