# Serial I/O and digital pads

SIO0 is the console's serial port for controllers and memory cards. The core models the SIO0 registers, an
eight-byte receive FIFO, scheduled byte completion, and two controller ports with independently enabled
multitaps. Each enabled tap exposes four controller sockets and four memory-card sockets, for up to eight card
endpoints total. With a tap disabled, the existing `81h` card address selects socket A on that port. SIO1 is
outside this module. Card Read (52h), GetID (53h), and Write (57h) are supported.

## Digital poll

The host selects port 1 or 2 through JOY_CTRL and sends `01 42 00 00 00`. The digital pad replies `FF 41 5A`,
then the two active-low button bytes. A zero button bit means pressed; a one means released. The final button byte
ends the transaction without an ACK or IRQ7. A poll is counted only when that final byte completes. A SIO peek
reads register state without consuming the receive FIFO or advancing the pad.

## Multitap controller access

Each port's tap is disabled by default and enabled independently. Method 2 uses `0n 42 00 ...`, where `n` is
`1` through `4`, to poll one socket with that controller's normal protocol length. This preserves longer
DualShock and configuration replies.

Method 1 sends a normal Slot A poll with `01 42 01 ...`. The third transmitted byte is REQ: its reply still
belongs to the current Slot A poll, and REQ controls the next access. REQ=0 selects Slot A on the next access.
REQ=1 after a Slot A reply selects a long reply on the next access. That reply starts with the multitap ID
`80 5A`, then four fixed eight-byte socket records. Each record has the controller ID, `5A`, two button bytes,
and four analog bytes; shorter device replies and missing later sockets are padded with `FF` bytes.

The documented Method 1 history is: REQ=0 returns Slot A after either kind of reply; REQ=1 after Slot A
returns the long reply; REQ=1 after the long reply returns the four-byte short reply `FF 80 5A <Slot A ID>`;
and REQ=1 after that short reply returns the long reply. The short reply is the documented HiZ/garbage response.
An empty Slot A aborts at its first address byte, before a request can be accepted. The reply history and active
per-port route are saved with SIO state; reset, deselect, or switching physical ports cancels an in-flight route
and its pending ACK events.

This controller model does not add the tap's IRQ10/lightgun wiring behavior.

## Mouse and neGcon polls

The mouse returns ID `5A12` and a four-byte payload. Its first halfword has right and left on active-low bits
10 and 11; the other documented bits are fixed (`1` in bits 0–7 and 12–15, `0` in bits 8–9). The next
halfword is signed X then signed Y. Host movement is an integer relative delta: each axis is clamped to
`[-128, 127]` when latched at frame start, reused for polls within that frame, and zero on later frames without
a fresh input snapshot. This is the core's input contract; it does not claim a measured physical mouse resolution.
Button levels remain latched across frames. Reset, disconnect or device change clears movement and partial reply
state; aborting a poll leaves the latched movement available for the next poll. The final payload byte has no ACK.

A newly selected mouse holds the modeled ACK-low status until its addressed endpoint receives `01`. The hold is
visible through JOY_STAT without a host polling loop and is released by an `01` addressed to port 1 or port 2 as
appropriate. Tap endpoints use the same per-endpoint rule. This models the documented startup condition while
keeping host progress bounded.

The neGcon returns ID `5A23` and a six-byte payload. Start, the D-pad, R, B and A use the documented active-low
button bits; the following bytes are raw twist, I, II and L values. Twist is `00` left through `FF` right, while
the three pressure bytes are passed through without a response curve or deadzone. These values remain independent
of the pad buttons and of other endpoints.

For Method 1 tap polling, the mouse's six-byte ID-plus-payload reply occupies a shorter socket record and is
padded with two `FF` bytes; the neGcon's ID-plus-payload fills its eight-byte record exactly.

## Dual Analog flight and Jogcon

The Dual Analog flight selection returns ID `5A53` and six payload bytes: the two active-low button bytes,
then right X, right Y, left X and left Y. Each axis is the frame-latched raw `00`–`FF` input, with `80` as
center. Its fixed eight-byte multitap record contains the ID, `5A`, both button bytes and all four axes.

Jogcon's native `42h` poll returns ID `5AE3` and six payload bytes. The first two bytes are the active-low
digital buttons. The next halfword carries the supplied `jogcon_dial` with wire bit 0 cleared because PER.01
marks that bit unknown; the final halfword uses bit 0 for turned left and bit 1 for turned right, with all
remaining bits zero. The dial halfword is copied from the frame latch on every poll; this model does not infer
whether the physical value is an absolute position or a relative delta and does not accumulate it. Jogcon uses
the existing digital device selection for digital-pad behavior.

| Jogcon command | Result |
| --- | --- |
| `42h` | Supported native `5AE3` button, dial and direction payload |
| `43h`, `44h`–`47h`, `4Ch`, `4Dh` | Reply `FF`, no ACK; do not enter config mode or alter rumble |
| Any other command | Reply `FF`, no ACK |

PER.01 reports a `4Dh` command and describes a motor, but does not establish a usable configuration/feedback
protocol here. This implementation makes no Jogcon force-feedback claim. It also does not assign behavior to the
unknown dial bit or claim that a controller-mode switch can be requested in software.

## GunCon polls

The direct-port GunCon answers poll command `42h` with ID `5A63h` and six payload bytes. The first
halfword is active low: A is bit 3, trigger is bit 13, and B is bit 14. The next halfwords are X
low byte then high byte, followed by Y low byte then high byte. A valid light observation supplies
X in 8 MHz clocks since HSYNC and Y in scanlines since VSYNC; host-pixel coordinates are not sent
directly over SIO. The IDs, button bits and coordinate units follow the PER.01 controller notes in
`.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md` §2.7. These notes are a secondary source and do
not establish a calibrated optical response.

When input is unavailable or no light sample completed, the selected deterministic response is
X=`0001h`, Y=`000Ah`. An explicit offscreen or reload input returns X=`0001h`, Y=`0005h`, even
when the latched cursor has coordinates. PER.01 associates these response words with overlapping
dark, busy and offscreen cases; the choices here resolve that ambiguity for this model and are not
physical response measurements. A repeated or aborted poll uses the current frame-latched state;
it cannot turn an invalid input into a hit or reuse an observation cleared by a newer snapshot.

## Justifier polls

The direct-port Justifier answers poll command `42h` with ID `5A31h` and two payload bytes. Its
active-low button word assigns Start to bit 3, Back to bit 14, and trigger to bit 15. The `10h` bit
in the first host-to-controller payload byte enables its optical sensor; the state changes only
when both payload bytes complete. An aborted poll therefore keeps the previous enable state.
Reset or switching the attached device clears the enable state and any pending shot. The ID and
button assignments follow the Justifier notes in
`.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md` §2.7, which summarizes PER.01. This model
supports only direct physical ports and does not claim multitap selection or multi-Justifier
wiring behavior.

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

All eight port/socket endpoints own separate 131,072-byte raw spans. When a port's tap is enabled, addresses
`81h`–`84h` select sockets A–D; with the tap disabled, the existing `81h` address selects socket A. A new
instance starts with formatted blank cards and flag `08`; before execution the host can import a raw image up to
131,072 bytes, padded with `FFh` when shorter. Card bytes and their generation counters are outside ordinary save
states, while the in-flight SIO/card protocol state is saved so a pending transfer can resume.

Write (57h) stages the addressed sector and its checksum while the old raw span remains visible. The final
successful `47` response copies all 128 bytes at once, increments that slot's generation once, and clears the
fresh-card flag. A bad checksum returns `4E`; an out-of-range sector returns `FF`. These rejected writes set the
write-error flag, which the next supported command returns and clears, and leave card bytes and generation
unchanged. Reset or port deselection cancels an incomplete write without changing either. Staged payload,
address, checksum and protocol position are
saved so a state taken mid-write can resume; loading a state still does not roll back card bytes or generation.
Atomic failed-write handling is a deterministic model choice where the reference does not establish whether a
failed transfer can have partially changed the card.

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
- Multitap controller addressing and the next-transaction REQ history follow PER.01 at pinned revision
  `6d7d1bc106a7e0b616b0330fe58401ab1ba57f0f`, under “Multitap Controller Access” and “Multitap Controller
  Access, Method 1 Details” in `docs/controllersandmemorycards.md`.
- Mouse and neGcon IDs, button bits, and payload ordering follow PER.01 at pinned revision
  `6d7d1bc106a7e0b616b0330fe58401ab1ba57f0f`, under “Controllers - Mouse” and “Racing Controllers” >
  “neGcon Racing Controller” in `docs/controllersandmemorycards.md`. The frame-latched movement clamp is this
  emulator's deterministic host-input contract, not a physical sensor measurement.
- Dual Analog flight and Jogcon IDs and payload fields follow PER.01 at the same revision, under “Dual Analog
  flight mode” and “Namco Jogcon” in `docs/controllersandmemorycards.md`. PER.01 leaves Jogcon dial bit 0,
  absolute-versus-relative dial interpretation, software mode switching and command `4Dh` behavior unresolved;
  the wire masking and command refusal here are the bounded behavior selected for this core.
- The 1,088-, 1,426-, 1,494-, 1,500- and 31,000-tick boundaries are deterministic model values and are covered
  by SIO and card tests.
