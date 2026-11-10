# Timers

The three root counters are 16-bit up-counters with a target, a reset rule and an interrupt each. This page covers the
module in `src/timers.c`: the registers and the bits of MODE, the clock sources and which ones count yet, why the
counters are computed on demand instead of stepped, the rules for targets, wraps and the write hold, the four interrupt
modes, the one deadline a counter gives the scheduler, the state, and the sources. Everything is integer arithmetic and
behaves the same on every platform. The block clock the counters read their tick from is described in
[the CPU page](cpu.md) and the interrupt controller in [the bus page](bus.md).

## State of the counters

All three counters are built, on the system clock, on the system clock over 8 (counter 2) and on the signals the GPU's
beam makes. The core has no line machine yet, so nothing in it makes the dot clock or the horizontal blank: a counter
whose clock source is one of them does not advance by itself, and a synchronisation to the horizontal blank sees no
blank. The entry points exist and the unit tests push the signals through them. The vertical blank is the frame's own:
at the end of a frame with a program loaded the frame pushes it high and low at the same tick, since the frame has no
line machine to give it a width (below). The sections "Synchronisation" and "Beam sources" say what each signal does.

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
horizontal blank (counter 1), which advance only as the beam's signals are pushed (see "Beam sources"). Counter 2 counts the system clock for sources 0 and 1 and the
system clock over 8 for sources 2 and 3 (psx-spx, Timers). The system clock is the CPU clock the instance's tick counts.

The eighth phase of counter 2's prescaler is the absolute tick: the counter takes a step at every tick that is a multiple
of 8. This keeps no state, so a counter that is switched to the prescaler or back needs no phase to be carried, and the
steps in a span of ticks are a difference of two shifts. Everything below that is in "cycles" is in the counter's own
cycles, so the hold of a counter on the prescaler lasts 16 ticks.

Counter 2 with synchronisation on is stopped for modes 0 and 3 and runs free for modes 1 and 2 (psx-spx, Timers). The
pause is worked out when MODE is written, and no blank affects it afterwards.

## Synchronisation

Counter 0 listens to the horizontal blank and counter 1 to the vertical blank. With synchronisation on (MODE bit 0) the
mode in bits 1-2 says what a blank does (psx-spx, Timers):

| Mode | Counter 0 and 1 |
|---|---|
| 0 | paused while the blank is high |
| 1 | reset to 0 when the blank begins |
| 2 | reset to 0 when the blank begins, and paused while it is low |
| 3 | paused until the blank begins once, then free-running |

Each signal has a level, kept in the state. A push of the level it already has changes nothing. The beginning of a blank is
the push that takes it from low to high; the end is the push that takes it from high to low. A reset takes effect at the
tick of the push and the counter counts from the next cycle, with no hold, so a counter reset at the frame's end reads the
number of cycles since that tick. A pending hold is dropped by it. Every push first brings the counters up to its tick.

A MODE write takes the pause from the level the blank has at that moment: mode 0 is paused at once when the blank is high,
mode 2 is paused when it is low, mode 1 never is, and mode 3 is paused whatever the level, because it needs a blank to
begin. It does not reset the counter by itself, since the write zeroes the counter anyway. psx-spx does not say what mode
3 does when it is written inside a blank, nor whether the hold of a write runs while the counter is paused; the model
lets the hold run, and these two choices are the model's.

The frame's end makes the vertical blank begin and end at one tick, so a counter 1 in mode 0 is never paused by it, in mode
1 is reset at it, in mode 2 is reset at it and is then paused for the rest of the frame, and in mode 3 is released by the
first. When the GPU has a line machine the signal gets its real width and this page's rules need no change.

## Beam sources

`schway__timers_hblank(inst, tick, level)` and `schway__timers_vblank(inst, tick, level)` set a blank to `level` at
`tick`; `schway__timers_dots(inst, tick, dots)` says that `dots` dot clocks have passed. Counter 0 with MODE bit 8 set
counts the dots, and counter 1 with MODE bit 8 set counts each beginning of the horizontal blank, one step each (psx-spx,
Timers). A step is one cycle of the counter's own clock, so it uses the hold, the target, the wrap and the interrupt rules
above in the same way, and a push of many dots costs no more than a push of one; a push of dots to a counter that is not on
the dot clock does nothing. A counter on one of these sources has no tick to wait for, so it gives the scheduler no deadline:
it raises its interrupt in the push that makes the edge.

An access to a counter whose MODE needs a signal that nothing in the core makes yet is counted as an access to a device not
built yet (the `unbuilt dev=timer` line of the runner) and is never a fault, since a program may read the counter or write
its MODE for good reasons. These are counter 0 on the dot clock or with synchronisation on (it waits for the horizontal
blank) and counter 1 on the horizontal blank. Counter 1's synchronisation is not one, nor is anything on counter 2. The
count is made while a MODE of that kind is in force, whether or not a test pushes signals; a counter in that mode with no
push stays at its value however many ticks pass, which is what a program that has no beam would see. At power-on, with MODE
0, no access is counted.

The GPU's integer dot accumulator belongs here, so that the dot clock is made without a wide multiply. With `div` the
number of system clocks per dot of the video mode and `line_clocks` the system clocks of the line or span just passed, the
dots to push are `dots += (phase + line_clocks) / div` and `phase = (phase + line_clocks) % div`. Never multiply the tick by
the dot clock's numerator (the product of a 62-bit tick and a 20-bit factor overflows 64 bits, and the 32-bit big-endian and
wasm32 builds have no 128-bit type), and never use an 11/7 shortcut for the clock ratio.

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
modes of counters 0 and 1: it is set when mode 3 has seen its blank). After the counters come the levels of the two
blanking signals, bit 0 the vertical and bit 1 the horizontal. The deadline is derived from these, never stored: a stored
deadline from a hostile state would be an event storm or a wait that never ends.

## Not built yet

The beam itself: the dot clock and the horizontal blank have no source in the core, and the vertical blank has no width,
until the GPU's line machine feeds them. What the hardware shows when a program reads a counter that runs from the dot
clock or the horizontal blank, which is unstable by a cycle, is not modelled.

## Sources

- psx-spx, Timers page (`docs/timers.md` of psx-spx/psx-spx.github.io at 6d7d1bc106a7): the registers, the MODE bits, the
  clock sources, the synchronisation modes, the reset and wrap rules, the hold and the first target case.

The register rules are the page's. The edges the synchronisation modes act on, the start of a blank as the moment of a reset
and the choices named under "Synchronisation" are the model's reading of the page. The timings of the hold in the model, the one-cycle reading of the reset dwell and the
interrupt rules for a one-shot counter with both sources enabled follow its text and are labelled doc, not hardware; the
tests pin the model's reading.
