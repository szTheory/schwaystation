# Sound processing unit

The SPU is an instance-owned integer device. It advances once every 768 CPU cycles, producing one signed stereo
sample pair at 44,100 Hz. Its 512 KiB of RAM is little-endian and byte-addressed. The host receives samples through
`schway_get_audio`; the core opens no audio device and performs no host-rate resampling. The documented baseline is
summarized in `.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md` §1. Current behavior is implemented in
[`src/spu.c`](../../src/spu.c), [`src/spu_voice.c`](../../src/spu_voice.c),
[`src/spu_envelope.c`](../../src/spu_envelope.c), [`src/spu_noise.c`](../../src/spu_noise.c), and
[`src/spu_reverb.c`](../../src/spu_reverb.c).

## Clock, registers and state

The scheduler advances one SPU event per 768 CPU cycles. Non-FIFO register writes are queued until an SPU event;
FIFO reads and writes happen at the bus access. The implementation uses this sample-granular order: apply queued
writes and drain pending manual-transfer entries, advance the shared noise clock; update voice 1's envelope, sample
and volume; update voice 3's envelope; then capture the current voice 1/3 outputs before voice 3's sample step. Next,
process voices 2–24 in order (voice 3's envelope was already updated), advance main volume sweeps, then mix and push
the pair. The reverb's 22.05 kHz
side alternates on successive native ticks. This order is the model's contract; it does not model sub-tick RAM slots.

Each voice occupies eight halfwords in the `1F801C00h–1F801D7Fh` range. The remaining control registers include
`KON`, `KOFF`, `PMON`, `NON`, `EON`, `ENDX`, `ATTR`, `STATX`, `TSA`, `FIFO`, `IRQA`, `ESA` and `RAM_CTRL`.
`1F801E00h–1F801E5Eh` expose current voice volumes. Addresses and bit meanings follow the pinned psx-spx source
listed in the preparation file; the model's current register paths are in `src/spu.c` and `src/spu_envelope.c`.

The state visitor records SPU RAM, register values, voice registers and decoder history, sample phase, envelope and
sweep counters, FIFO contents and cursors, queued register writes, noise phase/LFSR, capture position, reverb FIR and
work histories, reverb cursors, transfer addresses, IRQ state and the next SPU tick. State format 5 validates these
fields before applying them. The host audio ring is excluded and is cleared after a valid load; a refused load leaves
the instance unchanged. See `src/spu.c` and `src/state.c`.

## Voice playback

There are 24 voices. `KON` starts the selected voice at its start address with cleared decoder history and an attack
envelope; `KOFF` starts release. `PITCH` advances the 28-sample BRR block phase and is capped at `4000h`; zero
produces silence. `PMON` uses the preceding voice's output to modulate the next voice's step. Loop-start records
update `LSAX`; loop-end sets `ENDX` and either repeats at `LSAX` or ends the voice.

A BRR block is 16 bytes: one header, one flag byte and 28 low-nibble-first signed samples. The fixed-point decoder
uses the documented five predictor pairs, three carried history samples and signed 16-bit saturation. Reserved shift
encodings map to 9 and filters above 4 are clamped to 4 in this baseline. Four taps from the 512-value Gaussian table
interpolate the current sample; the table is in `src/spu_tables.inc`, and `tests/spu/voices.c` checks the table and
decoder behavior. These tests establish the implemented baseline, not a captured silicon waveform. See psx-spx's
[SPU description at the pinned preparation source](../../.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md#12-voice-registers-and-brr-blocks)
and its Gaussian interpolation section.

## ADSR and volume envelopes

Attack rises from zero to `7FFFh`, decay falls to `(sustain level + 1) * 800h`, sustain follows its selected direction
and rate, and release falls to zero. `ENVX` may be written to set the current level and is updated by the envelope
stepper. The integer baseline uses the documented step/shift and period rules; exponential decrease scales its step
by the current level, and exponential attack adjusts step or period above `6000h`. All-ones rate encodings do not
step. The 2-by-128 rate table is tested against an independent generator for the ares ISC source at
[`9408cb43d494`](https://github.com/ares-emulator/ares/tree/9408cb43d494); that is a baseline comparison, not a
hardware capture.

Direct volume is signed 15-bit and doubled. Sweep registers retain separate left/right level and counter state,
including the documented phase-negative bit. The rate behavior above 26 and phase-negative sweep edge cases remain
unresolved without a direct hardware oracle. No machine-profile switch changes the baseline. Muting the final output
does not stop voice decode, envelopes, volume sweeps, capture or RAM/IRQA work.

## Noise

All noise-selected voices share a signed 16-bit LFSR. Each clock shifts left and inserts
`b15 XOR b12 XOR b11 XOR b10 XOR 1`. `ATTR` selects a period and one of four eight-slot schedules. The clock advances
at most once during each sample, even when the shortest period places several slots in that window. Clearing the SPU
enable bit does not reset the LFSR or its phase. The schedule and capture signal follow the exact public
[psx-spx update](https://github.com/psx-spx/psx-spx.github.io/commit/3717718f642ce2ca9ff590b65918fb3a46279cc4).
`tests/spu/noise.c` checks this sequence and slot schedule; it is not a hardware-captured noise waveform.

## FIFO, DMA4, capture and IRQ9

`TSA` (`1F801DA6h`) is in eight-byte units and sets the wrapped byte transfer pointer. `FIFO` (`1F801DA8h`) holds
32 halfwords: a full-FIFO write is ignored, an empty-FIFO read returns zero, and an ordinary read consumes one word.
A register peek returns the front word without consuming it. In ManualWrite mode, the next SPU event drains pending
FIFO entries to SPU RAM. `RAM_CTRL` values `4`, `6`, `8` and `A` replicate each written halfword 1, 2, 4 or 8 times.

DMA4 channel 4 is integrated with the shared FIFO. `ATTR` bits 5–4 choose stop, ManualWrite, DMAWrite or DMARead.
For RAM-to-SPU DMA, the low halfword of each guest word is transferred first. The request path uses the selected
`ATTR` mode and FIFO availability; sync-mode-1 blocks wait for the next SPU request deadline. For SPU-to-RAM DMA,
the model returns `FFFFh` at each 32-halfword boundary unless `DEV4_CTRL` is `220931E1h`. The pinned
`spu/memory-transfer` guest checks seven named transfer cases; its log proves only those cases. No manual-read path
is modeled.

Capture writes zero-valued Phase-5 CD-L/CD-R samples to `00000h` and `00400h`, and voice 1/3 values to `00800h` and
`00C00h`. In noise mode the captured voice signal is the full signed LFSR value scaled by the voice envelope. The
cursor advances through 200h halfwords and wraps. Capture is enabled by `RAM_CTRL` bits 2–3 and continues while the
audible SPU output is disabled. CD audio inputs remain zero in Phase 5; CD-DA and XA are Phase 6 work.

`STATX` reports delayed `ATTR` control bits, IRQ9, manual-transfer busy, DMARead/DMAWrite FIFO availability, and the
capture half. Every modeled SPU RAM read or write uses the shared little-endian helper. A matching eight-byte `IRQA`
range raises IRQ9 only when SPU enable and IRQ9 enable are both set; clearing IRQ9 enable acknowledges the latch.
The model checks IRQ at each modeled RAM access, but does not claim measured sub-tick access placement.

## Reverb

EON-selected voice output enters a symmetric 39-tap fixed-point FIR before the 22.05 kHz feedback stage. Left and
right work alternate on successive native ticks; the 39-tap input and output filters contribute 38 samples of total
44.1 kHz delay. A SCPH-1001 digital-I2S measurement found that delay, the alternating channels and intermediate
16-bit saturation. Its exact per-stage rounding positions remain unknown. See the public
[psx-spx measurement](https://github.com/psx-spx/psx-spx.github.io/commit/a37f2fec163e149f4e3e39599eb43c65c21181a8).

The deterministic baseline applies register-selected SAME/DIFF IIR delays, `vIIR`, `vWALL`, four comb taps and two
all-pass stages, then scales the resampled return by EVOL. Signed register values use Q15 arithmetic with explicit
rounding and 16-bit saturation at the implemented intermediate boundaries. The documented `vIIR = -8000h` case
negates the completed IIR result; measured `vLIN/vRIN = -8000h` uses ordinary signed Q15 scaling. `ESA` selects the
buffer base, work offsets wrap from that base through `7FFFEh`, and every work-memory access checks IRQA. With reverb
disabled, the model preserves the documented read behavior while suppressing work-memory writes. `tests/spu/reverb.c`
checks delay, alternating phases, constraints and local impulse digests. Those digests are regressions, not captured
silicon output.

## PCM output and evidence limits

Each native tick emits one interleaved signed 16-bit stereo pair. The mixer saturates each voice contribution, sums
the dry and wet paths, applies the main left/right volumes when both SPU enable and unmute are set, then saturates the
final pair. The bounded host ring is four PAL frames deep; `schway_get_audio` drains pairs to the caller. The runner
hashes the drained PCM as canonical little-endian stereo bytes and fails if its drain reports lost samples. The host
is responsible for any resampling.

No full silicon PCM waveform is available in the pinned SPU test corpus. Transfer pass lines are hardware evidence
for named transfer behavior; the noise/reverb measurements constrain particular facts; `spu_tone` and impulse
digests are deterministic regressions; the CPU-free block replay checks guest/table equality; and `libretro.host`
checks callback delivery against the runner. None of these alone establishes complete bit-exact hardware audio.
