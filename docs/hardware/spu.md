# Sound processing unit

The SPU is an instance-owned device clocked once per 768 CPU cycles (44.1 kHz). Its 512 KiB RAM is byte-addressed,
little-endian, and wraps at `80000h`. Register writes other than the data FIFO take effect at the next SPU tick. The
model keeps that sample-granular timing; it does not claim sub-tick ordering. Hardware facts and confidence are indexed
in `.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md` §§1.1, 1.5–1.6.

## RAM transfers

`TSA` (`1F801DA6h`) is expressed in eight-byte units. A write latches its wrapped byte address into the transfer
pointer. `FIFO` (`1F801DA8h`) holds up to 32 halfwords in order: writes to a full FIFO are ignored, reads from an empty
FIFO return zero, and a normal read consumes one entry. A pure register peek returns the front entry without consuming
it. FIFO accesses take effect when the bus access occurs; this preserves the data-port behavior while other register
writes are delayed to the next sample tick.

`ATTR` (`1F801DAAh`) bits 5–4 select Stop, ManualWrite, DMAWrite, or DMARead. In ManualWrite, one FIFO halfword is
transferred per SPU tick. The byte pointer advances by two for every halfword written and wraps at the end of SPU RAM.
`RAM_CTRL` (`1F801DACh`) values `4`, `6`, `8`, and `A` replicate each transferred halfword 1, 2, 4, and 8 times.
Replication writes, capture writes, and modeled RAM reads all use the central little-endian helpers, which check the
eight-byte IRQA range on every access. No DMA engine or manual-read path is implied by this slice.

## ADSR and volume envelopes

`KON` starts a voice in attack at level zero; reaching `7FFFh` enters decay. Decay falls toward the sustain level
`(N + 1) * 800h`, then sustain follows its selected direction and rate. `KOFF` moves any active voice into release;
release ends at zero. The guest may write `ENVX` to jump the current level. Subsequent envelope ticks update both the
level used by playback and the `ENVX` readback.

The documented rate value combines a five-bit shift with a two-bit step. For rates below 48, the period is one
sample and the signed step is shifted by `11 - shift`; from rate 48 onward the step is unshifted and the period is
`1 << (shift - 11)` samples. Increasing steps are `+7` through `+4`; decreasing steps are `-8` through `-5`. An
exponential decrease scales its step by the current level. An exponential increase above `6000h` quarters the step
for shifts below 10, halves the step and doubles the interval at shift 10, or quadruples the interval at shift 11 and
above. Levels clamp to the documented range. All-ones rate encodings do not step.

Direct voice and main volumes are signed 15-bit values doubled to the sample scale. Sweep registers update separate
left and right current-volume state, including phase-negative direction, and current voice volumes are readable at
`1E00h` through `1E5Eh`. Muting suppresses the final PCM output while voice decoding, envelopes and volume sweeps keep
advancing. The implementation uses a per-sample integer mixer and saturates each voice and the final stereo pair.

The full 2-by-128 rate baseline is compared against an independent generator for the ares ISC implementation at
`9408cb43d494`. This is baseline conformance, not a captured hardware envelope waveform. Hardware anomalies above
rate 26 and phase-negative sweep behavior remain unresolved; this core follows the documented baseline without a
machine-profile switch. The SPU hardware notes and unresolved questions are indexed in
`.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md` §1.4.

## Capture and status

The bounded capture model writes zero-valued CD-L, CD-R, voice 1, and voice 3 inputs to the four rings at `00000h`,
`00400h`, `00800h`, and `00C00h`. Capture is suppressed when `RAM_CTRL` bits 2–3 are both clear. Its shared cursor
advances once per tick through 200h halfwords; `STATX` bit 11 is set while writing the upper 100h halfwords.

`STATX` (`1F801DAEh`) reports delayed `ATTR` bits 0–5 in bits 0–5, `ATTR` bit 5 again in bit 7, IRQ9 in bit 6,
manual transfer busy in bit 10, and DMA request state in bits 8–9. DMAWrite requests data while the FIFO has room;
DMARead reports data while the FIFO is nonempty. Those request bits describe FIFO availability only; a DMA engine is
not part of this model. IRQ9 is raised on a modeled RAM access matching `IRQA` only when both `ATTR` bit 15 (SPU
Enable) and bit 6 (IRQ9 Enable) are set. Clearing bit 6 acknowledges the IRQ9 latch without changing RAM.

The capture values and exact placement of multiple RAM accesses within one sample are not claimed as measured hardware
waveforms. They are deterministic, tick-bounded inputs for later SPU work; the pinned memory-transfer program remains
the hardware oracle for its named transfer cases.
