# Timers

The three root counters are 16-bit up-counters with a target, a reset rule and an interrupt each. This page covers the
module in `src/timers.c`: the registers and the bits of MODE, the clock sources and which ones count yet, why the
counters are computed on demand instead of stepped, the rules for targets, wraps and the write hold, the four interrupt
modes, the one deadline a counter gives the scheduler, the state, and the sources. Everything is integer arithmetic and
behaves the same on every platform. The block clock the counters read their tick from is described in
[the CPU page](cpu.md) and the interrupt controller in [the bus page](bus.md).

## State of the counters

The counters on the system clock are built: counter 2 on the system clock and on the system clock over 8, and counters 0
and 1 on the system clock. A source that comes from the GPU's beam (the dot clock for counter 0, the horizontal blank for
counter 1) has no signal behind it yet, so a counter set to one of those sources does not advance. The synchronisation
modes of counters 0 and 1 wait for the same beam and have no effect yet; counter 2's synchronisation modes act (below).
The entry points for the vertical blank, the horizontal blank and the dot clock exist and change nothing yet: at the end
of a frame with a program loaded the frame pushes the vertical blank high and low at the same tick, since the frame has
no line machine to give it a width.

Every address of `1F801100h`-`1F80112Fh` answers at every width, so the range counts as owned. The word at offset `0Ch` of
each counter reads 0 and drops writes.

## The registers

| Address | Name | Reads |
|---|---|---|
| `1F801100h + 10h * n` | `COUNT` | the counter, 16 bits, upper half 0 |
| `1F801104h + 10h * n` | `MODE` | bits 0-12 as below; bits 11 and 12 clear after the read |
| `1F801108h + 10h * n` | `TARGET` | the target, 16 bits, upper half 0 |
| `1F80110Ch + 10h * n` | (unused) | 0 |

`n` is 0 to 2. A byte or halfword access reaches its lane of the word, and a write merges the lane into the register and
then applies the word's rules.

| MODE bits | Meaning |
|---|---|
| 0 | synchronisation enable |
| 1-2 | synchronisation mode |
| 3 | reset at the target (1) or at FFFFh (0) |
| 4 | interrupt when the counter reaches the target |
| 5 | interrupt when the counter reaches FFFFh |
| 6 | interrupt every time (1) or once (0) |
| 7 | toggle (1) or pulse (0) |
| 8-9 | clock source |
| 10 | interrupt line: 0 is requested, 1 is not; set by a write |
| 11 | reached the target (cleared by a read) |
| 12 | reached FFFFh (cleared by a read) |

Bits 0-9 are what a program writes. A MODE read of any width returns the register and then clears bits 11 and 12. Power-on
values are all 0, and a power reset restores them; a program always writes MODE before it relies on bit 10.

## Clock sources

Counters 0 and 1 count the system clock for sources 0 and 2 and, for sources 1 and 3, the dot clock (counter 0) or the
horizontal blank (counter 1), which do not advance yet. Counter 2 counts the system clock for sources 0 and 1 and the
system clock over 8 for sources 2 and 3 (psx-spx, Timers). The system clock is the CPU clock the instance's tick counts.

The eighth phase of counter 2's prescaler is the absolute tick: the counter takes a step at every tick that is a multiple
of 8. This keeps no state, so a counter that is switched to the prescaler or back needs no phase to be carried, and the
steps in a span of ticks are a difference of two shifts. Everything below that is in "cycles" is in the counter's own
cycles, so the hold of a counter on the prescaler lasts 16 ticks.

Counter 2 with synchronisation on is stopped for modes 0 and 3 and runs free for modes 1 and 2 (psx-spx, Timers). The
pause is worked out when MODE is written.

## Lazy counting

A counter is not stepped. It keeps its value, the hold, the reached flags, the interrupt line, the one-shot state and
the pause as they were at one tick, and the value and flags at a later tick are computed from those and the number of the
counter's cycles in between, in constant time: each rule below is a closed form over the few segments of the counter's
position, never a loop over cycles. Every access to the counter registers and every read of or
write to `I_STAT` first brings all three counters up to the tick of the access. The tick comes from the block clock, which
is live inside a block, so a program that polls `COUNT` in a loop reads a value that moves with every poll without the
run loop having to stop. Nothing runs between accesses; the cost of a counter is the cost of its accesses.

A span's effects are applied together. The counter lands on the target or on FFFFh some number of times in the span; the
landings set the reached flags, and the interrupt modes turn the number of interrupt events into one raise of the
`I_STAT` bit (bit 4 plus the counter's number). The raise is idempotent, so a counter at target 0 in repeat mode, which
has an event every second cycle, costs one call per catch-up whatever the span.

## Targets and wraps

With the reset at FFFFh the counter takes every value from 0 to FFFFh in turn and wraps from FFFFh to 0 with no extra
cycle, so its loop is 10000h cycles. It lands on the target when it takes that value, once per loop.

With the reset at the target a counter that shows the target goes to 0 on the next cycle and shows 0 for two cycles, so a
loop of target `t` is `t + 2` cycles. psx-spx gives the first case, with the target 1:

| cycle | 0 | 1 | 2 | 3 | 4 | 5 |
|---|---|---|---|---|---|---|
| counter | 0 | 0 | 1 | 0 | 0 | 1 |

A counter written above the target does not compare until it wraps: it counts to FFFFh, wraps to 0 with no extra cycle
and compares from there on (psx-spx, Timers). The landing on FFFFh on the way sets the FFFFh flag, and with the target at
FFFFh the two landings are one event.

A landing is the counter taking a value by counting or by the reset; a value a program writes is not a landing.

## The hold

A counter written shows the written value for two of its cycles before it counts or compares (psx-spx, Timers: "it will
stay at the written value for 2 clock cycles"). A MODE write zeroes the counter and holds it the same way. In the model a
write at tick `W` leaves the value on show at `W + 1` and `W + 2` and counts from `W + 3`: the hold is two cycles after
the cycle of the write, which is the reading the tests pin. A reset at the target shows 0 for two cycles counting
the one it lands on. The public page does not say whether the write cycle
itself counts toward the two, so the model's choice is that it does not; the register write path's own cost, which the
tests do not model, is a smaller uncertainty than this. A TARGET write has no hold.

## Interrupts

MODE bits 4 and 5 choose what raises, bit 6 how often and bit 7 how. An event is a landing the bits enable. Both
landings on one cycle are one event.

- **One-shot** (bit 6 clear): the first event raises and nothing else does until MODE is written; the counter keeps
  counting and the reached flags keep setting. If both sources are enabled the first to occur is the one that raises.
- **Repeat** (bit 6 set): every event is an event.
- **Pulse** (bit 7 clear): an event raises the interrupt and leaves bit 10 at 1. The pulse is too short to see.
- **Toggle** (bit 7 set): an event flips bit 10 and raises only when the flip takes it to 0, so with the line at 1 the
  first event raises and the second does not, and in repeat mode every second event raises (psx-spx, Timers). The level
  lives in the counter; `I_STAT` only latches. In one-shot mode the line stays at 0 after the raise.

A MODE write sets bit 10 to 1 and re-arms the one-shot. The effect of an event on the level and the one-shot depends
only on how many events a span holds, and for a toggle on the parity of that number, so a long span that nobody watched
costs no more than a short one and gives the same line.

## The deadline

The only deadline a counter gives the scheduler is the next event that would make a new edge on its `I_STAT` bit. None
is given while the bit is already set, since further events change nothing the program can see, and an acknowledgement
(a write to `I_STAT`) recomputes it. That write first catches the counters up, so the events that happened while the bit
was set are behind it and cannot come back as an interrupt after it. A toggle counter whose line is at 0 gives the
second event, since only that one raises; a one-shot that has raised gives none.

The counter finds the distance to that event by bisection over the same function that applies a span, so the two cannot
disagree, within a horizon of 30000h cycles that holds two events of the slowest loop. The result is in the counter's
cycles; for the prescaler it is converted to the tick of the multiple of 8 that the cycle falls on. Every deadline is
strictly after the tick the counter was computed at, so a run that stops at a deadline always advances. An advance that ends on a
deadline runs that event before it returns, so a state saved between two advances never has a deadline behind its own
tick.

A pulse counter at target 0 in repeat mode, with no acknowledgement, is one event over a whole frame, and the scheduler is
left with no deadline until the program acknowledges.

## The state

The state holds each counter's nine fields as they are at the instance's tick, computed by the same function that
applies a span, and not the tick the stored fields were last brought up to date at. The bytes therefore do not depend on
when a catch-up last ran, and saving changes nothing in the instance. A load stores the fields and makes the loaded tick
the base. Value and target are 16 bits, mode is the 10 written bits, `hold` is 0 to 2, `reached` has the two flags in
bits 0 and 1, and the line, one-shot, pause and free-after fields are single bits (the last is for the synchronisation
modes of counters 0 and 1 and is 0 until they are built). The deadline is derived from these, never stored: a stored
deadline from a hostile state would be an event storm or a wait that never ends.

## Not built yet

The beam sources and the synchronisation modes of counters 0 and 1, the dot-clock and horizontal-blank feeds, and a
vertical blank with a width come with the GPU's line machine. What the hardware shows when a program reads a counter that
runs from the dot clock or the horizontal blank, which is unstable by a cycle, is not modelled.

## Sources

- psx-spx, Timers page (`docs/timers.md` of psx-spx/psx-spx.github.io at 6d7d1bc106a7): the registers, the MODE bits, the
  clock sources, the synchronisation modes of counter 2, the reset and wrap rules, the hold and the first target case.

The register rules are the page's. The timings of the hold in the model, the one-cycle reading of the reset dwell and the
interrupt rules for a one-shot counter with both sources enabled follow its text and are labelled doc, not hardware; the
tests pin the model's reading.
