# Serial I/O and digital pads

SIO0 is the console's serial port for controllers and memory cards. The core models the SIO0 registers, an
eight-byte receive FIFO, scheduled byte completion, two digital-pad ports and one memory card per port. SIO1
is outside this module. Card Read (52h) and GetID (53h) are supported; Write (57h) is not supported yet.

## Digital poll

The host selects port 1 or 2 through JOY_CTRL and sends `01 42 00 00 00`. The digital pad replies `FF 41 5A`,
then the two active-low button bytes. A zero button bit means pressed; a one means released. The final button byte
ends the transaction without an ACK or IRQ7. A poll is counted only when that final byte completes. A SIO peek
reads register state without consuming the receive FIFO or advancing the pad.

## Timing model

At mode `000Ch` (eight data bits, no parity, factor MUL1) and baud reload `0088h`, the byte period is 1,088
CPU ticks: `136 × 8`. The receive byte is unavailable at tick 1,087 and appears at tick 1,088. For the present
digital pad, an ACK is scheduled 338 ticks after byte completion. It asserts the ACK-low status and, when JOY_CTRL
enables ACK IRQs, raises IRQ7 on that same tick. The signal is released after 68 ticks.

The 338-tick delay is a corroborating software-model value, not a measurement from a physical console. The
console reference describes the pad ACK delay as about 10 microseconds; the pinned model uses 338 CPU ticks. The
68-tick hold is the model's integer ceiling of the documented minimum 2 microseconds at 33,868,800 CPU ticks per
second. Together these values place IRQ7 at tick 1,426 after a transmit at tick 0, safely beyond the first 100
ticks that the kernel waits before acknowledging an older IRQ7. Tests assert these model boundaries; they do not
claim a new hardware measurement.

Card timing is separate from the pad rule: the model schedules a card ACK about 1,500 CPU ticks after the byte's
transmit start, and pauses about 31,000 ticks after the seventh byte of a Read command, when the sector address is
complete. These are modeled timing values, not measurements from a physical console. The 68-tick ACK hold is shared.

## Memory-card Read and GetID

The host selects port 1 or 2 with JOY_CTRL. A card transaction starts with `81`; `52` reads a sector and `53`
returns the card ID. A Read returns the flag, `5A 5D 00 00 5C 5D`, the sector address, 128 bytes, the XOR checksum
of the address and data, and `47`. The final `47` byte has no ACK. An address above sector `3FF` returns `FF FF` and
ends the transaction. GetID returns the Sony-card reply `5A 5D 5C 5D 04 00 00 80` after the flag. Other commands
abort without changing card bytes.

Both slots own separate 131,072-byte raw spans. A new instance starts with formatted blank cards and flag `08`;
the host can import an exact-size raw image before execution. Card bytes and their generation counters are outside
ordinary save states, while the in-flight SIO/card read phase is saved so a pending read can resume.

## Register and interrupt behavior

JOY_TX_DATA schedules a transfer when TXEN is set. JOY_MODE and JOY_BAUD determine its character duration.
Completed bytes enter the bounded receive FIFO when receive is enabled or a device is selected. A 16-bit data read
consumes one byte; a 32-bit read consumes up to four, with later FIFO bytes previewed in the upper lanes. A full FIFO
drops the new byte and sets the sticky parity-error status bit.

IRQ7 is edge-triggered in I_STAT while JOY_STAT bit 9 reports the device's sticky level. Software clears I_STAT
bit 7 first, then writes JOY_CTRL bit 4 to clear the SIO status. Reset or deselect cancels pending byte and ACK
deadlines; save states retain those deadlines and their per-port transaction state.

## Sources and confidence

- The register map, baud formula, FIFO widths, and acknowledge order follow the pinned psx-spx SIO and
  controller references at revision `6d7d1bc106a7`. These are secondary hardware-documentation sources.
- The approximate pad ACK delay and minimum ACK pulse width come from the pinned controller reference at the
  same revision. The 338-tick pad value and card timing are modeled values; they are not presented as direct
  hardware measurements.
- The 1,088-, 1,426-, 1,494-, 1,500- and 31,000-tick boundaries are deterministic model values and are covered
  by SIO and card tests.
