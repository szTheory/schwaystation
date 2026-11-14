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

## Noise

All noise-enabled voices share one signed 16-bit LFSR. Each clock shifts left and inserts
`b15 XOR b12 XOR b11 XOR b10 XOR 1`; `FFFFh` is not reached from the reset seed. ATTR shift sets a period of
`20000h >> shift` output samples. The period is divided into eight slots; ATTR step 0–3 select respectively
`0,2,4,6`; `0,1,2,4,6`; `0,1,2,4,5,6`; and `0,1,2,3,4,5,6`. The model clocks at most once in each sample,
including at the shortest periods. Clearing ATTR bit 15 does not reset the shared LFSR or its phase. This schedule
and the LFSR behavior follow the pinned psx-spx update at
[commit `3717718f642ce2ca9ff590b65918fb3a46279cc4`](https://github.com/psx-spx/psx-spx.github.io/commit/3717718f642ce2ca9ff590b65918fb3a46279cc4); they are documented behavior, not a captured noise waveform.

## Capture and status

The four 400h-byte rings at `00000h`, `00400h`, `00800h`, and `00C00h` receive zero-valued Phase-5 CD-L/CD-R inputs
and voice 1/3 samples after envelope scaling. In noise mode the voice capture value is the full signed LFSR level
multiplied by ENVX and arithmetic-shifted by 15; it is not a one-bit signal. Capture is suppressed when `RAM_CTRL`
bits 2–3 are both clear, but otherwise continues when ATTR disables or mutes audible output. The shared cursor
advances once per tick through 200h halfwords; `STATX` bit 11 reports the upper half after the lower 100h samples.
Capture writes use the same little-endian RAM helper as other SPU accesses, so matching `IRQA` can raise IRQ9.

`STATX` (`1F801DAEh`) reports delayed `ATTR` bits 0–5 in bits 0–5, `ATTR` bit 5 again in bit 7, IRQ9 in bit 6,
manual transfer busy in bit 10, and DMA request state in bits 8–9. DMAWrite requests data while the FIFO has room;
DMARead reports data while the FIFO is nonempty. Those request bits describe FIFO availability only; a DMA engine is
not part of this model. IRQ9 is raised on a modeled RAM access matching `IRQA` only when both `ATTR` bit 15 (SPU
Enable) and bit 6 (IRQ9 Enable) are set. Clearing bit 6 acknowledges the IRQ9 latch without changing RAM.

The documented noise capture scaling is implemented in the sample-granular model. The exact placement of multiple RAM
accesses within one sample is not claimed as measured hardware timing; the pinned memory-transfer program remains the
hardware oracle for its named transfer cases. Noise mode still performs the voice's modeled RAM fetches before sample
selection, preserving their IRQA behavior when the voice is muted or its BRR output is inaudible.

## Reverb resampling and mix

Voice sums selected by EON enter the reverb path before the final main-volume stage. The reverb input uses the
documented symmetric 39-tap integer FIR before its 22.05 kHz stage; the return uses the matching polyphase FIR
to produce 44.1 kHz samples. The coefficients and channel-phase selection are in `src/spu_tables.inc` and
`src/spu_reverb.c`; their fact source is `.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md` section 1.7.

Left and right 22.05 kHz work alternates on successive native sample ticks. Each centered 39-tap filter adds
19 native samples of delay, so the pair contributes 38 samples before the wet return is mixed. The SCPH-1001
I2S measurement confirms that delay and the alternating-channel effect; it also finds intermediate 16-bit
saturation, while the exact per-stage rounding positions remain unknown. See the public psx-spx measurement at
[commit `a37f2fec163e149f4e3e39599eb43c65c21181a8`](https://github.com/psx-spx/psx-spx.github.io/commit/a37f2fec163e149f4e3e39599eb43c65c21181a8).

The current 22.05 kHz feedback seam forwards its input as a unity stage; the register-driven feedback network
remains a separate part of the model. EVOL still scales the interpolated return; a zero EVOL setting therefore
contributes no wet PCM. The regression checks treat these constraints as timing and signal-path evidence, not as a
captured silicon preset waveform.
