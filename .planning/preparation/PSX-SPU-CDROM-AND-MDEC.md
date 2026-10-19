---
title: "PSX SPU, CD-ROM controller and MDEC: register models, timings, disc images, FMV path and the audio contract"
summary: "Cited facts and settled choices for the three streaming subsystems: the 24-voice SPU (512 KB RAM, BRR ADPCM, ADSR step/shift rules, gaussian table, noise, reverb formula and 39-tap FIR, capture buffers, IRQ, DMA4, 768 cycles per sample), the CXD1199-family CD-ROM host interface (index model, command set, INT1-INT5, Setmode bits, measured response and seek timings, sector formats, XA-ADPCM, GetID by region, LibCrypt), disc image formats and the host disc-reader interface, the MDEC command set and fixed-point IDCT, the test and fixture plan, and the determinism and A/V sync model."
read_when: "Before designing, implementing or testing spu.c, cdrom.c, cdxa.c, mdec.c, the disc-reader interface, the audio output contract, audio hashes, the fixture disc, or any CD timing."
updated: 2026-10-19
---

# PSX SPU, CD-ROM and MDEC

## Where to look
| Question | Section |
|---|---|
| Licence status of what was opened for this file | 0 |
| SPU time base, RAM map, voice registers, BRR block format, pitch and gaussian interpolation | 1.1-1.3 |
| ADSR and sweep envelope rules, key on/off, ENDX | 1.4 |
| Noise, ATTR/STATX, transfers, DMA4, IRQ, capture buffers | 1.5-1.6 |
| Reverb registers, formula, FIR, buffer timing | 1.7 |
| Mixing, saturation, CD/external inputs, pitfalls, SPU choices (SPU-MODEL, SPU-INTERP, HASH, OUT) | 1.8-1.9 |
| CD-ROM ports and index model, HSTS, INT semantics, FIFOs | 2.1 |
| Command set, status byte, error codes, responses | 2.2 |
| Setmode bits, sector delivery, XA filter, sector buffer | 2.3 |
| Measured timings (ACK, Pause, Init, INT1 rate), seek model, CD timing choice (CD-TIME) | 2.4 |
| Sector formats, XA-ADPCM decoder and resampler, CD-DA volume matrix | 2.5 |
| GetID, SCEx, region, firmware versions, lid open, disc swap | 2.6 |
| LibCrypt and SBI | 2.7 |
| Disc image formats, Redump conventions, cue pitfalls, disc-interface choice (DISC-IF) | 3 |
| MDEC ports, commands, bitstream, IDCT, colour conversion, output modes, FMV scheduling (MDEC-IDCT) | 4 |
| Tests, oracles, fixture disc, tool licences | 5 |
| Determinism and A/V sync (SYNC) | 6 |
| Confidence table and the test that settles each row | 7 |

## 0. Licence status of sources opened
| Source | Checked at pin | Result | Use here |
|---|---|---|---|
| ares `ares/ps1/{spu,disc,mdec}` | 9408cb43d494, LICENSE "ares team, Near et al" (ISC) [SCM.10] | opened | T3 behaviour evidence; seeds with notice |
| MAME `src/devices/sound/spu.cpp` | 96df6a708bdd, header `license:BSD-3-Clause`, pSXAuthor and R. Belmont [SCM.11] | header only | permitted; not read for this file |
| JaCzekanski/ps1-tests | f727802fead1, LICENSE MIT 2020 [SCM.12] | opened | tests, hardware logs |
| rtissera/libchdr | 47d794151b2b, LICENSE.txt BSD-3, deps lzma 26.02, miniz 3.1.2, zstd 1.5.7 [SCM.13] | opened | CHD reader |
| Lameguy64/mkpsxiso | a6b11ea86e67, LICENSE.md **GPL-2.0**; bundles tinyxml2, miniaudio, FLAC [SCM.14] | README only | build tool only; its output is ours; never vendor |
| spicyjpeg/psxavenc | bdc24e897ed0, licence **Zlib** [SCM.15] | metadata only | XA/STR encoder for fixtures |
| Lameguy64/PSn00bSDK | 5d9aa2d3dfc7, LICENSE.md: core **MPL-2.0**; mkpsxiso GPL-2; toolchain GPL-3 [SCM.16] | licence only | headers and facts only |
| JaCzekanski/Avocado | d7fcf9060ac7, LICENSE **GPL-2.0** (the brief says MIT: wrong) [SCM.17] | licence only | **never opened**; docs and issues only |
| simias/rustation | 5e650b949d6e, LICENSE **GPL-2 or later** (the brief says MIT: wrong) [SCM.18] | licence only | **never opened** |
| simias/psx-guide (Rustation guide) | a89043ea499a, **no LICENSE file**; `guide.tex` states none [SCM.19] | not opened | treat as all-rights-reserved; not cited |
| PeterLemon/PSX | 6d20c132aba0, no licence file or statement [SCM.20] | listing only | not usable as a seed |

PROVENANCE must record the two brief corrections (Avocado, Rustation) so no later phase opens them.

## 1. SPU
### 1.1 Time base and memory
| Item | Fact | Source |
|---|---|---|
| Clock | CPU 33,868,800 Hz = 44,100 x 300h; one output sample every 768 CPU cycles | [SCM.01][SCM.04] |
| Register map | 1F801C00-1D7F voices (8 halfwords each), 1D80-1D87 volumes, 1D88-1D9F voice flags, 1DA2-1DBF control, 1DC0-1DFF reverb, 1E00-1E5F per-voice current volumes, 1E60-1E7F and 1DA0/1DBC unused RAM-like | [SCM.01] |
| Bus | 16-bit; 8-bit writes to odd addresses ignored, to even addresses executed as 16-bit; 32-bit writes occasionally lost on hardware (ps1-tests `spu/test` confirms) | [SCM.01][SCM.12] |
| Write latency | values applied at the 44.1 kHz tick, so up to 300h cycles late; ATTR bits 0-5 appear in STATX bits 0-5 delayed | [SCM.01] |
| RAM 512 KB | 00000-003FF CD-L capture, 00400-007FF CD-R, 00800-00BFF voice 1, 00C00-00FFF voice 3 (each 400h bytes = 200h samples), 01000 up ADPCM, top = reverb work area ending at 7FFFE | [SCM.01] |
| RAM_CTRL 1F801DAC | must be 0004h (single 512 KB bank); other values double or octuple halfwords on write; capture IRQs and reverb need bits 2-3 set | [SCM.01] |
| RAM timing | per 768-cycle frame: 24 voices x 3 RAM accesses, 4 capture writes, 14 reverb accesses, 8 cycles each = 720 cycles; order voice 1, CD-L, CD-R, V1, V3, reverb, voices 2-24 (logic-analyser capture) | [SCM.01] |

### 1.2 Voice registers and BRR blocks
| Register (+10h x N) | Meaning | Source |
|---|---|---|
| C00/C02 VOLL/VOLR | bit15 = 0: volume/2 in bits 0-14 (-4000h..3FFFh); bit15 = 1: sweep (bit14 exp, bit13 decrease, bit12 phase negative, bits 6-2 shift, bits 1-0 step) | [SCM.01] |
| C04 PITCH | 1000h = 44,100 Hz; step clamped to 4000h (176.4 kHz); 0 stops | [SCM.01] |
| C06 SSA | start address in 8-byte units, copied to the current address on key on; never increments | [SCM.01] |
| C08/C0A ADSR1/2 | see 1.4 | [SCM.01] |
| C0C ENVX | current ADSR level 0..7FFFh, R/W; writing jumps the envelope; hardware overwrites on the next step | [SCM.01] |
| C0E LSAX | repeat address; set by a Loop Start header or by software | [SCM.01] |
| BRR block | 16 bytes: byte 0 shift (bits 0-3, 13-15 act as 9) and filter (bits 4-6, 0-4; ares clamps >4 to 4); byte 1 flags bit0 Loop End, bit1 Loop Repeat, bit2 Loop Start; 14 data bytes = 28 nibbles, low nibble first | [SCM.01][SCM.10] |
| Flag codes | 0 continue; 1 End+Mute (jump to LSAX, set ENDX, release, env = 0); 2 same as 0; 3 End+Repeat (jump, set ENDX) | [SCM.01] |
| Filter table | pos = {0, 60, 115, 98, 122}, neg = {0, 0, -52, -55, -60}; s = (nibble << 12 >> shift) + (old x pos + older x neg) >> 6, saturated to 16 bits (ares; psx-spx gives (+32)/64 for XA, same table minus entry 4) | [SCM.10][SCM.03] |
| Pitch modulation | PMON bit x (x > 0): step = ((sext16(pitch) x (OUTX(x-1) + 8000h)) >> 15) & FFFFh, then clamp 4000h; glitchy above 7FFFh | [SCM.01] |
| Counter | bits 12+ select the sample in the block (28 per block), bits 4-11 index the gaussian table | [SCM.01] |

### 1.3 Gaussian interpolation
out = (g[FFh-i] x oldest + g[1FFh-i] x older + g[100h+i] x old + g[i] x new) >> 15 (each product shifted separately), i = counter bits 4-11; the 512-entry table is printed in full on psx-spx (first entries -1, ..., last 59B3h); the four taps sum to 7F7Fh..7F81h, so no overflow [SCM.01]. The table is checked in as `spu_gauss[512]` and a unit test asserts the per-index sum range.

**SPU-INTERP.** **Recommendation:** gaussian table interpolation exactly as above, with the three previous samples carried across blocks (ares keeps samples 25-27 of the prior block) [SCM.10]. **Strongest alternative:** linear or none (cheaper, audibly wrong on high pitch). **Would change if:** nothing; this is the hardware.

### 1.4 ADSR and sweep envelopes
| Field | Bits | Source |
|---|---|---|
| ADSR1 (C08) | 15 attack mode (0 lin, 1 exp), 14-10 attack shift, 9-8 attack step (+7..+4), 7-4 decay shift (exp decrease, step -8 fixed), 3-0 sustain level ((N+1) x 800h) | [SCM.01] |
| ADSR2 (C0A) | 31 sustain mode, 30 sustain direction, 28-24 sustain shift, 23-22 sustain step, 21 release mode, 20-16 release shift (step -8 fixed) | [SCM.01] |

Envelope step rule (psx-spx, confirmed by ares' table): `step = 7 - stepValue` (negated to -8..-5 when decreasing XOR phase-negative), `step <<= max(0, 11 - shift)`, `counterIncrement = 8000h >> max(0, shift - 11)`; each 44.1 kHz tick adds the increment and steps when bit 15 of the counter is set. Exponential increase above 6000h: shift < 10 step/4; shift >= 11 increment/4; else both /2. Exponential decrease: `step = step x level / 8000h`. Step value all ones (7Fh, or 1Fh for decay/release) never steps and never saturates. Levels clamp to 0..7FFFh (envelope) or -8000h..7FFFh (volume sweep) [SCM.01]. ares encodes the same as a 2 x 128 table: rate < 48 gives ticks = 1 and step shifted by 11 - rate/4; rate >= 48 gives ticks = 1 << (rate/4 - 11) [SCM.10]. Hardware shows rates above 26 misbehave (76h acts like 6Ah) and phase-negative sweep decreasing snaps to zero; Wipeout 3 depends on the all-ones no-step rule [SCM.01]. Key on clears ENDX, copies SSA, zeroes the level and history, starts attack; key off switches any phase to release; ENDX sets on Loop End [SCM.01][SCM.10]. Decay target = sustain level, release target 0 [SCM.10].

### 1.5 Noise and control
Noise (shared by all voices, rate from ATTR bits 13-8): each tick `timer -= step(4..7)`; `parity = b15 ^ b12 ^ b11 ^ b10 ^ 1`; when timer < 0: `level = level x 2 + parity`, `timer += 20000h >> shift` (twice if still negative) [SCM.01]. ares implements the equivalent 64-entry wave table with a fractional count (frequency {0, 84, 140, 180} of 210) [SCM.10]; the psx-spx form is the one checked in.

| ATTR 1F801DAA | bit15 SPU enable, 14 unmute, 13-10 noise shift, 9-8 noise step, 7 reverb master, 6 IRQ9 enable (and acknowledge when 0), 5-4 transfer mode (0 stop, 1 manual write, 2 DMA write, 3 DMA read), 3/2 PIO/CD reverb, 1/0 PIO/CD input enable | [SCM.01] |
|---|---|---|
| STATX 1F801DAE | bit11 capture half, 10 transfer busy, 9 DMA read request, 8 DMA write request, 7 = ATTR.5, 6 IRQ flag, 5-0 = ATTR 5-0 delayed | [SCM.01] |
| Transfer | TSA 1F801DA6 (address/8, latched to an internal incrementing pointer), FIFO 1F801DA8 (32 halfwords), DMA4 blocksize 10h; manual write needs Stop, address, FIFO, ManualWrite, wait busy; DMA read needs DEV4_CTRL 220931E1h or the data arrives with FFFFh inserted every 32 halfwords | [SCM.01] |
| IRQ | IRQA 1F801DA4 (address/8); fires on any voice fetch (even silent voices, even noise mode), reverb access, capture writes (addresses 0..1FFh, about 86.13 Hz) and transfers; mid-block addresses miss sometimes; MGS, DDR, CTR, Um Jammer Lammy, Tron Bonne, NFS3 rely on it | [SCM.01] |

### 1.6 Capture buffers
CD-L, CD-R, voice 1 and voice 3 are written every sample to the four 400h-byte rings regardless of ATTR enable or lid state; STATX bit 11 shows the half being written, only when RAM_CTRL bits 2-3 are set [SCM.01]. Required by Crash Team Racing lip sync and Um Jammer Lammy [SCM.01].

### 1.7 Reverb
Registers 1F801DC0-1DFE in this order: dAPF1, dAPF2, vIIR, vCOMB1-4, vWALL, vAPF1, vAPF2, mLSAME, mRSAME, mLCOMB1, mRCOMB1, mLCOMB2, mRCOMB2, dLSAME, dRSAME, mLDIFF, mRDIFF, mLCOMB3, mRCOMB3, mLCOMB4, mRCOMB4, dLDIFF, dRDIFF, mLAPF1, mRAPF1, mLAPF2, mRAPF2, vLIN, vRIN; plus EVOLL/R (1D84/86), ESA (1DA2, base, writing also resets the current address), EON0/1 (1D98/9A) [SCM.01]. Addresses are in 8-byte units relative to the current buffer address, wrapped within ESA..7FFFEh; the buffer address advances by 2 per 22.05 kHz step: `addr = max(ESA, (addr + 2) & 7FFFEh)` [SCM.01]. The documented formula per 22.05 kHz step, every product >> 15 and every stored value saturated to 16 bits [SCM.01]:
```
[mLSAME] = (Lin + [dLSAME]*vWALL - [mLSAME-2])*vIIR + [mLSAME-2]   ; same side, L and R
[mLDIFF] = (Lin + [dRDIFF]*vWALL - [mLDIFF-2])*vIIR + [mLDIFF-2]   ; cross side
Lout = vCOMB1*[mLCOMB1] + ... + vCOMB4*[mLCOMB4]
Lout = Lout - vAPF1*[mLAPF1-dAPF1]; [mLAPF1] = Lout; Lout = Lout*vAPF1 + [mLAPF1-dAPF1]
Lout = Lout - vAPF2*[mLAPF2-dAPF2]; [mLAPF2] = Lout; Lout = Lout*vAPF2 + [mLAPF2-dAPF2]
LeftOutput = Lout*EVOLL
```
Hardware does left on one 44.1 kHz tick and right on the next; input and output pass through a 39-tap FIR (coefficients printed on psx-spx, centre 4000h, symmetric, odd taps zero) for the 44.1 to 22.05 kHz resample [SCM.01]. vIIR = -8000h negates the stored result (a documented bug); ATTR bit 7 = 0 turns the four writes into reads but output continues [SCM.01]. ares implements the IIR stage as `((in*alpha >> 14) + ((1-alpha)*prev >> 14)) >> 1` with a special case for alpha = -8000h, a 20-tap half-band FIR on a 64-entry ring, and the same address wrap [SCM.10]. Nine presets (Room, Studio S/M/L, Hall, Half Echo, Space Echo, Chaos Echo, Delay, Off) with sizes are listed on psx-spx and are the test vectors [SCM.01].

### 1.8 Mixing and inputs
Per voice: `Lvol = (ENVX x VOLXL) >> 15`, sample x ADSR level then x voice volume; the 24 voices sum, reverb return x EVOL, CD input x AVOLL/R (1DB0/1DB2, signed 16-bit), PIO input x BVOLL/R (1DB4/1DB6), then main volume MVOLL/R (1D80/82, same direct/sweep encoding as voice volumes) [SCM.01]. The CD-ROM decoder applies its own 2x2 matrix first (ATV0-3, 80h = unity, saturation works to double volume, "FFh,FFh,FFh,FFh" overflows) [SCM.02]; ares saturates each matrix term to 16 bits and lets the sum overflow [SCM.10]. Negative volumes invert phase [SCM.01].

**SPU-MIX.** **Recommendation:** int32 accumulators; saturate to int16 at exactly the points the documentation names (each voice after volume, the reverb stored values, the CD matrix terms, the final left/right). **Strongest alternative:** saturate only at the output (fewer ops, wrong on loud mixes). **Would change if:** a hardware capture of the Chaos Echo preset shows the reverb overflowing where the model saturates.

### 1.9 Pitfalls and the SPU choices
| Pitfall | Fact | Response |
|---|---|---|
| Key-on timing | KON applied at the next 44.1 kHz tick, not at the write; 32-bit KON writes may be lost on hardware | apply all flag writes at the tick; test both halfword orders |
| Envelope precision | rates above 26 and phase-negative sweeps deviate from the formula | implement the formula, keep a profile knob for the 76h = 6Ah quirk, mark (untested) |
| Reverb correctness | addressing is relative and wraps; left/right alternate ticks | implement the two-phase form, verify against presets with a self-authored impulse |
| RAM read-back | DMA read inserts FFFFh unless DEV4_CTRL set; ps1-tests `memory-transfer` has 7 pass lines from hardware | run the test in CI; it is the only RAM oracle |
| 8-bit writes | odd ignored, even widened | unit test |

**SPU-MODEL.** **Recommendation:** one integer SPU tick per 768 CPU cycles (no sub-tick model), processing voices 1-24 then reverb in the hardware order of 1.1, with the IRQ check at each RAM access so streaming games see IRQs at the right sample. **Strongest alternative:** cycle-slot model following the 8-cycle RAM schedule (needed only for exact IRQ phase within a sample). **Would change if:** an IRQ-streaming title (MGS, DDR) desynchronises at sample granularity.

**HASH.** **Recommendation:** the audio hash is SHA-256 over the int16 stereo stream the SPU emits at native 44,100 Hz after main volume and final saturation, before any host resampling, with the CD-DA/XA contribution included (it is mixed inside the SPU) and reverb included. One record per sample, no transition stream (the PSX output is already a fixed-rate PCM stream, unlike the PSG precedent) [SCM.21]. **Strongest alternative:** hash voice outputs pre-mix for finer fault localisation (costly, 24 x). **Would change if:** hashing costs more than 2 % in the runner.

**OUT.** **Recommendation:** the core emits int16 stereo at 44,100 Hz, exactly `frame_cycles / 768` samples per frame with the remainder carried in an accumulator; the samples-per-frame rational is 527,799,972/715,909 = 737.244 for NTSC (263 × 3413 GPU clocks = 405,350,378,496/715,909 CPU cycles) and 628,856,592/709,379 = 886.489 for PAL (314 × 3406 clocks = 482,961,862,656/709,379 cycles) (derived from the exact clock ratios 715909/451584 and 709379/451584 of PSX-SYSTEM C1 and ARCHITECTURE 1, not from the 11/7 approximation [SCM.22]). The host resamples (section 6). **Strongest alternative:** core-side resampling to 48 kHz. **Would change if:** a host cannot resample (then add the sibling's Kaiser kernel as an optional stage).

## 2. CD-ROM controller
### 2.1 Ports and index model
Four 8-bit registers 1F801800-1803 in four banks selected by writing bits 0-1 of 1800h [SCM.02]:
| Bank | 1801 W | 1802 W | 1803 W | 1801 R | 1802 R | 1803 R |
|---|---|---|---|---|---|---|
| 0 | COMMAND | PARAMETER | HCHPCTL (bit7 BFRD, bit6 BFWR, bit5 SMEN) | RESULT | RDDATA | HINTMSK |
| 1 | WRDATA | HINTMSK (bits 0-4 enable) | HCLRCTL (bits 0-4 ack, 5 clear sound map, 6 clear params, 7 chip reset) | RESULT | RDDATA | HINTSTS |
| 2 | CI (sound map coding) | ATV0 L->L | ATV1 L->R | RESULT | RDDATA | HINTMSK |
| 3 | ATV2 R->R | ATV3 R->L | ADPCTL (bit0 mute XA, bit5 apply ATV) | RESULT | RDDATA | HINTSTS |
HSTS (1800 R): bits 0-1 bank, 2 ADPBUSY, 3 PRMEMPT, 4 PRMWRDY, 5 RSLRRDY, 6 DRQSTS, 7 BUSYSTS [SCM.02]. Parameter FIFO 16 bytes (datasheet says 8; longest command needs 13), result FIFO 16 bytes padded with 00h then wrapping [SCM.02]. HINTSTS bits 0-2 are one 3-bit value INT0-INT7 (INT1 data/report, INT2 complete, INT3 acknowledge, INT4 data end, INT5 error or lid), bits 3-4 BFEMPT/BFWRDY, bits 5-7 read as 1; responses queue (INT3 then INT5), never OR; IRQ when HINTMSK & HINTSTS != 0 [SCM.02]. RDDATA delivers 800h or 924h bytes and repeats byte [800h-8] or [924h-4] beyond; DMA3 reads it as 512 words because the BIU splits words into byte reads [SCM.02]. Sending a command with an INT pending is unpredictable; the kernel acknowledges with 1Fh first [SCM.02]. Flag polling can see partial values on LATE-PU-8 but not PSone (a double read is the workaround) [SCM.02].

### 2.2 Command set
| Op | Name (params) | Responses | Notes | Source |
|---|---|---|---|---|
| 01 | Nop | INT3 stat | clears shell-open flag if lid closed | [SCM.02] |
| 02 | Setloc (mm, ss, ff BCD) | INT3 | ss < 60h, ff < 75h else INT5 (stat, 10h); marks target unprocessed | [SCM.02] |
| 03 | Play (track opt.) | INT3, INT1 reports if mode bit2 | INT4 at end of disc (motor off) or end of track with autopause | [SCM.02] |
| 04/05 | Forward/Backward | INT3, INT1 | only in Play state else INT5 (stat+1, 80h) | [SCM.02] |
| 06/1B | ReadN/ReadS | INT3, then INT1 per sector | ReadS = no retry; unlicensed disc gives INT5 (03h, 40h) first | [SCM.02] |
| 07 | MotorOn | INT3, INT2 | INT5 (stat, 20h) if already on; (stat, 80h) no disc | [SCM.02] |
| 08 | Stop | INT3, INT2 | brakes, head to track 1 | [SCM.02] |
| 09 | Pause | INT3, INT2 | INT5 (stat, 80h) in some seek phases | [SCM.02] |
| 0A | Init | INT3 (late), INT2 | mode = 20h, motor on, aborts; a second Init while pending is dropped silently | [SCM.02] |
| 0B/0C | Mute/Demute | INT3 | forces CD output volume to zero; XA still decoded (ADPBUSY) | [SCM.02] |
| 0D | Setfilter (file, channel) | INT3 | XA only | [SCM.02] |
| 0E | Setmode (mode) | INT3 | see 2.3 | [SCM.02] |
| 0F | Getparam | INT3 stat, mode, 00, file, channel | | [SCM.02] |
| 10 | GetlocL | INT3 mm, ss, ff, mode, file, channel, sm, ci | newest buffered sector; INT5 80h on audio or during seek | [SCM.02] |
| 11 | GetlocP | INT3 track, index, mm, ss, ff, amm, ass, aff | subchannel Q; works during seek; LibCrypt reads it | [SCM.02] |
| 12 | Setsession (n) | INT3, INT2 | errors 10h (n = 0), seek errors on absent sessions | [SCM.02] |
| 13 | GetTN | INT3 stat, first, last (BCD) | | [SCM.02] |
| 14 | GetTD (track) | INT3 stat, mm, ss | 00 = end of last track; rounded down to seconds, index 1 | [SCM.02] |
| 15/16 | SeekL/SeekP | INT3, INT2 | SeekL by data headers (lands N-8..N at 1x, N-5..N+2 at 2x), SeekP by subchannel Q (N-9..N-1, N-2..N); both abort reads; SeekL on audio errors (stat+4, 04h) after about 2 s | [SCM.02] |
| 19 | Test (sub) | varies | 20h = BIOS date/version; 22h region string; 04h/05h SCEx counters | [SCM.02] |
| 1A | GetID | INT3, INT2 or INT5 (8 bytes) | see 2.6 | [SCM.02] |
| 1C | Reset | INT3 only | needs 1/8 s (400000h cycles) delay | [SCM.02] |
| 1D/1E | GetQ/ReadTOC | INT3, INT2 | vC1 and up only; ReadTOC second response about 1 s | [SCM.02] |
| 1F, 50-57 | VideoCD, Unlock | INT5 (11h, 40h) | SCPH-5903 and vC1+ non-Japanese only | [SCM.02] |
| others | | INT5 (11h, 40h); 58-5F crash the HC05 | parameters stay in the FIFO after INT5 | [SCM.02] |

Status byte: bit7 Play, 6 Seek, 5 Read (at most one set; Gran Turismo waits for Read to rise, not Seek to fall), 4 ShellOpen (sticky until Nop), 3 IdError, 2 SeekError, 1 motor on, 0 Error [SCM.02]. Error bytes: 10h bad parameter, 20h wrong count, 40h invalid command, 80h cannot respond yet (no disc, TOC not read); 04h seek failed (second response); 08h lid opened (unsolicited INT5 with stat = 10h or 11h) [SCM.02]. Sub-CPU model: one flag each for pending INT2 and INT1, so at most one undelivered INT1; commands are not executed while any INT is pending [SCM.02].

### 2.3 Setmode and sector delivery
Setmode bits: 7 double speed, 6 XA-ADPCM to SPU, 5 sector size (0 = 800h data, 1 = 924h from byte 0Ch), 4 ignore (keeps the last size and returns position minus 0..3 sectors with IdError), 3 XA filter, 2 report, 1 autopause, 0 CD-DA (also lets reads pass on wrong-region discs) [SCM.02]. Delivery order per sector: try ADPCM (reject audio tracks, non-MODE2, ADPCM off, filter mismatch, submode without audio+realtime bits 2 and 6) then try data (INT1 unless another INT is pending; a retry later applies the file/channel check, a documented BUG) [SCM.02]; ares mirrors this with `(sector[18] & 0x44) == 0x44` [SCM.10]. During Play only bits 7, 2, 1 matter; report INT1 carries 8 bytes alternating absolute and in-track time every 10h sectors [SCM.02]. The sector buffer is 32 KB (8 slots) but the firmware exposes the oldest and the newest sector only; sectors are lost silently on overrun, GetlocL returning a far-ahead sector is the only hint [SCM.02]. XA interleave: 1/8 for 37.8 kHz stereo at 2x, up to 1/32 for 18.9 kHz mono; CTR uses 37.8 kHz mono (1/16) [SCM.03].

### 2.4 Timings and the CD timing choice
| Quantity | Hardware value (CPU cycles) | Source |
|---|---|---|
| INT1 period | exactly 33,868,800 x 930h / 4 / 44,100 = 451,584 at 1x, 225,792 at 2x (psx-spx); ps1-tests measured avg 446,1xx at 1x and 222,2xx at 2x over runs | [SCM.02][SCM.12] |
| First response (Nop) | avg C4E1h, range 4A73h..3115Bh (PAL PSone); ps1-tests avg 45,993 range 24,024..137,104; Setloc avg 57,955 | [SCM.02][SCM.12] |
| Init first response | avg 13CCEh (about 81 k); ps1-tests init ack avg 75,086, complete avg 476,300 | [SCM.02][SCM.12] |
| GetID second | avg 4A00h | [SCM.02] |
| Pause second | 1x avg 21181Ch (about 2.17 M), 2x avg 10BD93h (1.1 M), when paused 1DF2h; ps1-tests about 1.01 M at 1x and 1.04 M at 2x from a read | [SCM.02][SCM.12] |
| Stop second | 1x avg D38ACAh, 2x 18A6076h, when stopped 1D7Bh | [SCM.02] |
| First sector after ReadN from Setloc | ps1-tests: 0.55 M..1.02 M (seek included) | [SCM.12] |
| Seek time | undocumented; coarse plus fine stepping; spiral density varies | [SCM.02] |

ares' model: ticks of 1/75 s; `t = 0.5 x (3 + 0.2 r + 2.5 sqrt(r) + d/60000)` with r = |sqrt(lba_b + 150) - sqrt(lba_a + 150)|, clamped 1..150, << speed, plus a 3-tick (<< speed) delay, 80-tick delay on a speed change, and arrival 25 sectors early for SeekP [SCM.10]. DuckStation's documentation describes its seek model as approximate (ignores sectors per track) and reports that randomising repeated seek times was needed for Dino Crisis 2 lockups; "Seek Speedup" is documented to crash games that expect drive time [SCM.23]. Beetle PSX documents "CD Loading Speed" above 2x as causing freezes and corruption [SCM.24].

**CD-TIME.** **Recommendation:** all CD timing in CPU cycles from the image alone: INT1 period exactly 451,584 >> speed; acknowledge latency a fixed 50,000 (the ps1-tests mean), Init second response 476,000, Pause second response 2,170,000 >> speed (psx-spx mean) and 7,666 when already paused; seek time = an integer version of ares' sqrt model (isqrt tables, Q16) expressed in 1/75 s ticks, never a wall clock. No randomisation; if a title needs jitter the fixture records the observation and the knob is a profile constant. **Strongest alternative:** DuckStation-style sled model with randomness (undocumented and non-deterministic). **Would change if:** the fixture disc timing test (section 5) on hardware yields a seek curve that the sqrt model misses by more than 20 % at three distances.

### 2.5 Sector formats, XA-ADPCM, CD-DA
| Layout | Bytes | Source |
|---|---|---|
| Audio | 930h: L lsb, L msb, R lsb, R msb | [SCM.03] |
| MODE1 | sync 0Ch, header 4 (BCD mm ss ff, mode 01h), data 800h at 010h, EDC at 810h, 8 zero, ECC 114h | [SCM.03] |
| MODE2 Form1 | header, subheader at 010h (file, channel, submode, codinginfo) and copy at 014h, data 800h at 018h, EDC 818h, ECC 81Ch | [SCM.03] |
| MODE2 Form2 | submode bit5 set, data 914h at 018h, EDC at 92Ch (optional) | [SCM.03] |
| Submode | bit0 EOR, 1 video, 2 audio, 3 data, 4 trigger, 5 form2, 6 realtime, 7 EOF | [SCM.03] |
| Codinginfo | bits 0-1 stereo, 2 18.9 kHz, 4 8-bit, 6 emphasis | [SCM.03] |

XA-ADPCM: 18 portions of 128 bytes per sector (900h), 14h zero bytes then EDC; 16-byte portion header (bytes 4-7 the block headers, duplicated at 0-3 and 8-15), 28 data words; header shift 0-12 (13-15 act as 9), filter 0-3 with pos {0, 60, 115, 98}, neg {0, 0, -52, -55}; `s = (nibble << 12 >> shift) + (old x f0 + older x f1 + 32) / 64` saturated [SCM.03]; ares uses the same with `>> shift` on a sign-extended nibble [SCM.10]. Resampling 37.8 kHz to 44.1 kHz: a 7-phase, 29-tap "zigzag" filter table printed on psx-spx, every 6 input samples producing 7 outputs, 18.9 kHz by sample doubling first; the six-step counter's power-on phase is random [SCM.03]. The old/older history is not reset between sectors [SCM.03]. CD-DA and XA pass through the ATV matrix and then SPU AVOL (1.8) [SCM.02].

**XA-RESAMPLE.** **Recommendation:** implement the 7-phase table exactly, six-step counter reset to 0 at power on (documented random; a fixed value is the deterministic choice). **Strongest alternative:** linear 6:7 (audible aliasing on sibilants). **Would change if:** a hardware capture of the fixture XA track shows the two-stage theory (21-tap zigzag then interpolation) matters at -60 dB.

### 2.6 GetID, region, firmware, lid
| Case | Second response | Source |
|---|---|---|
| Door open / spin-up / detect busy | first response INT5 (11h, 80h) / (01h, 80h) / (03h, 80h) | [SCM.02] |
| No disc | INT5 08h, 40h, 00 x6 | [SCM.02] |
| Audio disc | INT5 0Ah, 90h, 00 x6 | [SCM.02] |
| Unlicensed Mode1 / Mode2 / Mode2+audio | INT5 0Ah, 80h, 00/20h, 00, 00 x4 (90h with audio) | [SCM.02] |
| Licensed Mode2 | INT2 02h, 00h, 20h, 00h, "SCEx" (x = I Japan, A America, E Europe) | [SCM.02] |
| Modchip audio/Mode1 | INT2 02h, 00h, 00h, 00h, "SCEx" | [SCM.02] |
| Yaroze | spaces for non-SCEA | [SCM.02] |

The SCEx string lives in the wobble of the lead-in; the controller firmware (not the BIOS) checks it and refuses to boot a mismatch; Test 22h returns "for Europe", "for U/C", "for Japan", "for NETNA", "for US/AEP" or INT5 on vC0 [SCM.02][SCM.03]. Firmware versions by Test 20h: vC0 (PU-7, 1994), v01 (DTL-H2000), vC1 (LATE-PU-8, 1995; adds Test 22h-25h, GetQ, ReadTOC), vC2 (PU-16/18/20, 1996-97; adds Test 51h, unlock commands outside Japan), vC3 (PU-22/23, PM-41, 1998-2001) [SCM.02]. ares reports 95-05-16 vC1 and answers GetID from the image's region string [SCM.10]. Opening the lid raises an unsolicited INT5 (stat with bits 4 and 0, error 08h), clears play/seek/read; games either Stop first or poll Nop until bit 4 clears; ps1-tests `disc-swap` logs the hardware sequence [SCM.02][SCM.12]. Reads on wrong-region discs return 40h unless CD-DA mode [SCM.02].

**CD-REGION.** **Recommendation:** the machine profile's region selects the SCEx letter; GetID compares it with the image's licence string (ISO system area) and answers from the table above; an `unlock` profile flag (vC1+, non-Japan) accepts the unlock sequence; firmware version is a profile constant defaulting to vC2 97-01-10 for NTSC-U/PAL and 96-09-12 for NTSC-J. **Strongest alternative:** always licensed (breaks modchip-detection titles such as Dino Crisis, which Mute around the check). **Would change if:** a title in the ledger needs the Yaroze responses.

### 2.7 LibCrypt
About 100 PAL titles store a 16-bit key as deliberately bad subchannel Q (two MSF bits flipped and the CRC made wrong by one of three schemas) at 16 fixed sectors in minute 3 (each bit twice, five sectors apart) with a backup in minute 9; GetlocP keeps returning the previous good position because the controller discards bad-CRC Q; the key is used to XOR 800h-byte sectors, and Legacy of Kain keeps it in COP0 debug registers [SCM.03]. Redump `.sbi` files list the affected sectors with 10-byte Q (format 1) or 3-byte wrong relative/absolute MSF (formats 2 and 3) but cannot carry the CRC, so an emulator treats every listed sector as bad-CRC [SCM.05]. The `.m3s` format (ePSXe) is 72,000 bytes of minute-3 Q and exists in three variants, two corrupt [SCM.05]. DuckStation and Beetle document `.sbi` beside the image (Beetle: `[name]_[disc].sbi` for multi-disc PBP) and that chdman does not embed `.sbi` [SCM.23][SCM.24].

## 3. Disc images and the disc interface
| Format | Layout facts | Hostile-input surface | v1 |
|---|---|---|---|
| CUE/BIN | text cue; BIN starts at 00:02:00 (150-sector pregap absent) with 930h sectors; PREGAP sectors absent from the file; INDEX 00 present or not; types AUDIO, MODE1/2048, MODE1/2352, MODE2/2336, MODE2/2352; malformed cues with 1-digit numbers exist | text parser: line length, track count <= 99, MSF ranges, file count, path traversal | yes |
| ISO (2048) | one MODE1/2048 or MODE2 Form1 data track; headers and subheaders synthesised (subheader 00 00 08 00) | size check only | yes |
| CCD/IMG/SUB | ini-like text; IMG 930h; SUB 60h per sector non-interleaved P..W; non-BCD values; version 2 lacks [TRACK]; INDEX 0 often missing although Tomb Raider 2 needs it | text parser | yes |
| CHD v5 | big-endian header 7Ch, hunks up to 512 KB of 990h units (930h + 60h subcode), codecs cdlz/cdzl/cdfl/cdzs, CHT2 metadata "TRACK:n TYPE:MODE2_RAW SUBTYPE:NONE FRAMES:n PREGAP:n PGTYPE:V... POSTGAP:0"; AUDIO stored big-endian; tracks padded to multiples of 4 frames | libchdr (BSD-3) does the parsing; fuzzed upstream | yes via libchdr |
| PBP | "\0PBP" header, PSISOIMG0000 at 10000h, TOC list of 10-byte Q entries at 10800h, sector list (32-byte entries, 10h sectors per deflate block of 9300h); multi-disc and audio compression undocumented; retail files PGD-encrypted | inflate plus offsets | no (deferred) |
| ECM | double extension; type 0-3 records rebuild sync/EDC/ECC; no sector index, so random access needs a full scan | varint lengths | no (convert offline) |
| MDS/MDF | binary header "MEDIA DESCRIPTOR", session and 50h-byte data blocks, sector size 800h..990h, subchannel interleaved after each sector | binary parser | no |
| M3U | one image per line; disc index selects | text | yes (host) |
All layout facts [SCM.05]; CHD codec tags and metadata formats [SCM.13]; Beetle and DuckStation support lists [SCM.24][SCM.23].

Redump conventions (search snippets and emulator docs; the wiki was unreachable today): one BIN per track, audio tracks with `INDEX 00` pregap lines, official cue files published per disc, `.sbi` published per LibCrypt title and placed beside the image with the same base name [SCM.25] (unverified beyond those snippets). Cue pitfalls: PREGAP versus INDEX 00 (absent versus present in the file), relative paths and case, BOMs, CRLF, quoted versus bare FILE names, `MODE2/2336` images needing synthesised sync and header, a missing cue for a bare BIN (fall back to one MODE2/2352 track), and the 2-second lead-in that every MSF in a cue omits [SCM.05].

**DISC-IF.** **Recommendation:** the core never opens files. The host supplies `schway_disc` with `read_sector(lba) -> 2352 bytes + 96 bytes subcode (or none)`, `toc()` (up to 99 tracks: type, index 0 and index 1 LBAs, pregap), `lid(open)` and `swap(disc)`; the core synthesises Q for images without subcode and applies an `.sbi` list through `bad_q(lba)`. A separate `schway_disc_image` helper library (MIT, same repository, fuzzed) implements CUE/BIN, ISO and M3U parsers in v1.0, and is what the runner and the libretro core use; CCD, and CHD through vendored libchdr behind `SCHWAYSTATION_CHD`, arrive in v2.0, and v1.0 refuses `.chd` with "unsupported: CHD (planned v2.0)" (ARCHITECTURE 13, DEC.43). **Strongest alternative:** parsers inside the core (simpler API, but puts hostile text parsing and a 3-dependency archive library into the deterministic core). **Would change if:** a host needs disc access without the helper (then the helper's parsers are already separable).

## 4. MDEC
| Port | Function | Source |
|---|---|---|
| 1F801820 W | command then parameters (usually by DMA0, blocksize 20h; pad with FE00h) | [SCM.04] |
| 1F801820 R | 8x8 block data, re-ordered to 16x16 by DMA1 | [SCM.04] |
| 1F801824 R | bit31 out FIFO empty, 30 in FIFO full, 29 busy, 28 DREQ0, 27 DREQ1, 26-25 depth (0 4-bit, 1 8-bit, 2 24-bit, 3 15-bit), 24 signed, 23 bit15, 18-16 current block (0-3 Y1-Y4, 4 Cr, 5 Cb), 15-0 words remaining minus 1 | [SCM.04] |
| 1F801824 W | bit31 reset (status = 80040000h), 30 enable DMA0, 29 enable DMA1 | [SCM.04] |

Commands: MDEC(1) decode (bits 28-27 depth, 26 signed, 25 bit15, 15-0 parameter words); MDEC(2) set quant tables (64 bytes luma, plus 64 chroma if bit 0); MDEC(3) set scale table (64 signed halfwords, 14 fractional bits, hardware uses the top 13 bits; standard values 5A82h, 7D8Ah...); MDEC(0) and 4-7 only reflect bits [SCM.04]. Bitstream per block: DC halfword (bits 15-10 quant scale, 9-0 signed DC), RLE halfwords (bits 15-10 zero run, 9-0 AC), EOB FE00h; FE00h at a block start is padding [SCM.04]. Decode: `val = DC x qt[0]` (or DC x 2 when scale = 0, stored without zigzag), each later `val = (AC x qt[k] x scale + 4) / 8`, saturated to 11 bits, stored at `zagzig[k]`; macroblock order Cr, Cb, Y1-Y4 for colour, one Y for mono [SCM.04]. IDCT: the hardware form is two passes of 8x8 matrix products with the scale table, `(sum + FFFh) / 2000h` per pass with the exact rounding unknown [SCM.04]; ares does `sum = Σ src[y+z*8] x scale[x+z*8]`, pass 0 `(sum + 8000h) >> 16`, pass 1 `clamp8(clip9((sum + 8000h) >> 16))` [SCM.10]. Colour: `R = Y + 1.402 Cr`, `G = Y - 0.3437 Cb - 0.7143 Cr`, `B = Y + 1.772 Cb`, clamp -128..127, XOR 80h when unsigned; mono clips Y to 9 bits then saturates to 8 [SCM.04]. Output packing: 4-bit (two pixels per byte), 8-bit, 15-bit (bit 15 from the command), 24-bit packed [SCM.10]. ares' decode step is a fixed 1000 cycles per block with the comment that Final Fantasy IX breaks if decoding is too fast and drops frames if too slow [SCM.10]. The FMV path is DMA3 (CD to RAM) -> DMA0 (RAM to MDEC) -> DMA1 (MDEC to RAM) -> DMA2 (RAM to GPU) with 16x240 vertical strips; STR frames arrive at 15 fps on 7/8 of the sectors at 2x with XA audio in the other 1/8 [SCM.04][SCM.03].

**MDEC-IDCT.** **Recommendation:** integer two-pass IDCT with the scale table as int16 Q14 values (top 13 bits used), 32-bit sums, ares' rounding (`+8000h >> 16`) as the starting point, and float-free colour conversion with the coefficients as Q16 integers (1.402 = 91881, 0.3437 = 22524, 0.7143 = 46811, 1.772 = 116130); block decode cost 1000 cycles per block until the ps1-tests step log fixes it. The decoded-frame hash is over the raw MDEC output words before DMA1 reordering. **Strongest alternative:** the fast 80-multiply IDCT with scalezag (valid only for the standard table). **Would change if:** the ps1-tests `frame` and `movie` VRAM references differ from our output by more than +-1 per channel.

## 5. Tests, oracles and fixtures
| Oracle | What it gives | Licence | Use |
|---|---|---|---|
| ps1-tests `spu/memory-transfer` (+ psx.log) | 7 named pass lines from hardware for manual write, DMA read/write, timing, control-to-status copy | MIT [SCM.12] | CI gate for SPU RAM |
| ps1-tests `spu/{stereo,test,toolbox,playback}` | voice playback, 32-bit write loss, register preview, binary SPU dump playback (dump captured with Avocado: facts only) | MIT [SCM.12] | listening and hash locators |
| ps1-tests `cdrom/{timing,getloc,disc-swap,volume-regs,terminal}` with psx.log | hardware ACK, first-sector, INT1 and Pause timings at 1x and 2x; stat bits during seek; lid sequence; ATV behaviour | MIT [SCM.12] | timing table in 2.4; the CD-TIME oracle |
| ps1-tests `mdec/{4bit,8bit,frame,movie,step-by-step-log}` with `.mdec` inputs and `vram-*.png` | decoded-frame references in 15 and 24-bit, status-register transitions per word | MIT [SCM.12] | bit-exact MDEC references (+-1 tolerance pending) |
| psx-spx reverb presets, gauss table, zigzag table, XA filter table | register vectors and constants | T2 [SCM.01][SCM.03] | unit tests |
| amidog CPU/GTE tests | DuckStation's README once claimed to pass them; not MDEC/SPU; site unreachable today, closed source | unknown [SCM.26] (unverified) | none for this file |
| PeterLemon/PSX `SPU`, `MDEC` demos | ADPCM playback and DCT block decode sources | none stated [SCM.20] | not used |

**Fixture disc.** Build with mkpsxiso from an XML the repository owns: track 1 MODE2/2352 with a PSn00bSDK-built `.EXE` (MPL-2.0 headers; binary carries the notice), a Form2 XA file with two interleaved channels at 37.8 kHz stereo (encoded by psxavenc, Zlib) and a short STR, plus one CD-DA track from a generated 440 Hz tone, with a Redump-style cue (one BIN per track, INDEX 00 on the audio track) and a hand-written `.sbi` that marks the 16 minute-3 sectors; no Sony licence file (the fixture tests GetID's unlicensed path and a `licensed` profile override tests the licensed one) [SCM.14][SCM.15][SCM.16]. mkpsxiso's GPL-2 covers the tool, not its output; it is run in CI, never vendored.

**Test matrix (settles the rows of section 7):** BRR decode of each filter against a Python reference in the test; gauss sums; ADSR step table versus ares' 2 x 128 table generated in the test; noise sequence; reverb impulse through each preset against our own frozen digest plus a sanity check that the Off preset is silent; CD command FSM against the ps1-tests `cdrom/timing` log (ACK and INT1 within min..max); GetID table; Setmode delivery rules on the fixture XA track; XA decode and 6:7 resample against a Python reference; LibCrypt GetlocP returns the previous good position on `.sbi` sectors; MDEC 4-bit, 8-bit, 15-bit, 24-bit against the ps1-tests PNGs; status word trace against `step-by-step-log/psx.log`; disc parsers fuzzed (libFuzzer harness per parser, 1 M iterations before each release).

## 6. Determinism and A/V sync
- Inputs to CD timing are the image bytes, the TOC and the command stream only; the sector clock is derived from the CPU cycle counter (451,584 >> speed), and seeks use the integer model of 2.4; the host's file latency never changes emulated time (the host may pre-read; the core blocks on `read_sector` only inside `schway_run`, so a slow host stalls the frame, never the drive) [SCM.02][SCM.10].
- XA-ADPCM and CD-DA samples are produced by the CD side at exactly one sector per INT1 period and queued into the SPU's CD input at 44.1 kHz; the queue depth is bounded (two sectors) and underflow produces zeros, matching the hardware's silence on a missed sector.
- Audio per frame: `schway_run` emits the frame's samples in one batch (737 or 738 NTSC, 886 or 887 PAL; 737.244 and 886.489 on average, derived) with the accumulator carried; never padded; state saves include the accumulator, the XA ring and six-step counter, the reverb address and the envelope counters.
- Resampling is the host's job (the libretro core reports 44,100 Hz and the exact field rate from PSX-SYSTEM so dynamic rate control absorbs the difference); handheld ports that need 48 kHz use the host's resampler or the optional kernel stage from OUT.

**SYNC.** **Recommendation:** as above; frame boundary = the VBlank the GPU file defines; all three subsystems step on the CPU cycle counter with event scheduling (next SPU tick, next sector, next MDEC block), no per-cycle polling. **Strongest alternative:** run the SPU in 768-cycle slices from the scheduler only when audio is enabled (saves time with audio off, but IRQ-driven games need the SPU to run regardless). **Would change if:** profiling shows SPU time above 10 % of a frame on the wasm32 target.

## 7. Confidence
| Area | Confidence | Settled by |
|---|---|---|
| SPU BRR, gauss, pitch, PMON | high (documented and two implementations agree) | unit tests, ps1-tests stereo |
| ADSR formula | high for rates <= 26, medium above (hardware quirk) | ADSR table test; capture of a sweep on hardware (open) |
| Noise | high | sequence test |
| Reverb | medium (formula documented, rounding order from ares) | preset impulse digests; hardware capture of Room preset (open) |
| Capture buffers, IRQ timing | medium | ps1-tests memory-transfer; CTR and MGS in the ledger |
| CD register and INT model | high | ps1-tests terminal and getloc |
| CD response latencies | medium (means known, distribution wide) | cdrom/timing log bounds |
| Seek time | low (no hardware model exists) | fixture timing test on hardware; CD-TIME flip rule |
| XA decode | high; resampler medium | reference decode; hardware capture of fixture XA (open) |
| GetID, region, lid | high | table tests; disc-swap log |
| LibCrypt | medium (SBI lacks CRC; semantics inferred) | PAL titles in the ledger |
| Disc parsers | high after fuzzing | fuzz harness |
| MDEC bitstream | high; IDCT rounding medium; colour fixed point low | ps1-tests PNG diffs with tolerance, then tighten |
| MDEC timing | low | ps1-tests step log; FF9 and FMV-heavy titles |

## Sources
| ID | Tier | Source | Pin or access date | Supports |
|---|---|---|---|---|
| SCM.01 | T2 | psx-spx consoledev edition, Sound Processing Unit (SPU) page (nocash, Jakub Czekański, contributors) | accessed 2026-10-19 | SPU registers, BRR, gauss table, ADSR, noise, ATTR/STATX, transfers, IRQ, capture, reverb registers, formula, FIR, presets, RAM timing state machine |
| SCM.02 | T2 | psx-spx, CDROM Drive page | accessed 2026-10-19 | ports, HSTS, INTs, commands, status, errors, Setmode, delivery, timings, GetID, Test, firmware versions, sector buffer |
| SCM.03 | T2 | psx-spx, CDROM Format page (sector encoding, subchannels, XA subheader and ADPCM, SCEx, LibCrypt) | accessed 2026-10-19 | sector layouts, XA decoder and tables, zigzag resampler, interleave, SCEx, LibCrypt |
| SCM.04 | T2 | psx-spx, Macroblock Decoder (MDEC) page | accessed 2026-10-19 | MDEC ports, commands, bitstream, IDCT, colour, DMA path |
| SCM.05 | T2 | psx-spx, CDROM File Formats page (disk image sections: CCD, CUE/BIN, MDS, ECM, subchannel images SBI/M3S, PBP, CHD) | accessed 2026-10-19 | image formats, SBI, PBP, CHD metadata |
| SCM.10 | T3 | ares `ares/ps1/spu/{adsr,envelope,noise,reverb,adpcm,voice}.cpp`, `disc/{drive,command,cdxa,cdda}.cpp`, `mdec/{decoder,tables}.cpp`, LICENSE (ISC) | 9408cb43d494 | ADSR table, noise, reverb rounding, BRR, seek model, latencies, XA decode, GetID, MDEC IDCT and packing |
| SCM.11 | T3 | MAME `src/devices/sound/spu.cpp` header | 96df6a708bdd | licence header only |
| SCM.12 | T1 | github.com/JaCzekanski/ps1-tests README, LICENSE, `cdrom/timing/psx.log`, `spu/memory-transfer/psx.log`, `mdec/step-by-step-log/psx.log`, directory tree | f727802fead1 | test inventory, hardware timing numbers, SPU transfer passes |
| SCM.13 | T1 | github.com/rtissera/libchdr README, LICENSE.txt, `include/libchdr/chd.h`, deps/ | 47d794151b2b | licence, CHD v5 header, codecs, CHT2 metadata format |
| SCM.14 | T1 | github.com/Lameguy64/mkpsxiso README, LICENSE.md | a6b11ea86e67 | GPL-2, XA/STR mixed mode, CDDA tracks, cue output, PREGAP support |
| SCM.15 | T1 | github.com/spicyjpeg/psxavenc repository metadata | bdc24e897ed0 | Zlib licence |
| SCM.16 | T1 | github.com/Lameguy64/PSn00bSDK LICENSE.md | 5d9aa2d3dfc7 | MPL-2.0 core, GPL tool and toolchain split |
| SCM.17 | T1 | github.com/JaCzekanski/Avocado LICENSE | d7fcf9060ac7 | GPL-2.0 (brief correction) |
| SCM.18 | T1 | github.com/simias/rustation LICENSE | 5e650b949d6e | GPL-2 or later (brief correction) |
| SCM.19 | T1 | github.com/simias/psx-guide tree and `guide.tex` grep | a89043ea499a | no licence present |
| SCM.20 | T1 | github.com/PeterLemon/PSX tree and README | 6d20c132aba0 | no licence; SPU and MDEC demo names |
| SCM.21 | T1 | cogmaster `.planning/preparation/SMS-AUDIO.md` HASH and OUT (method precedent) | 2026-10-18 edition | hash and output contract pattern |
| SCM.22 | T2 | psx-spx, GPU page, GPU Timings (video clocks, 263/314 lines, CPU x 11/7) | accessed 2026-10-19 | frame rationals (line length unverified) |
| SCM.23 | T4 | DuckStation README (formats, SBI placement, CHD subchannel) and public commit messages on seek speedup, seek randomness, Road Rash audio | accessed 2026-10-19 via search | seek model status, format list, SBI handling |
| SCM.24 | T4 | docs.libretro.com Beetle PSX page | accessed 2026-10-19 | formats, m3u, PBP, SBI naming, CD access method, loading speed warning |
| SCM.25 | T4 | search snippets: libretro forums and Recalbox wiki on Redump cue and SBI layout; wiki.redump.org unreachable (272-byte response) | accessed 2026-10-19 | Redump conventions (unverified) |
| SCM.26 | T4 | search snippets on amidog psxtest (site 404 today) | accessed 2026-10-19 | existence only |
