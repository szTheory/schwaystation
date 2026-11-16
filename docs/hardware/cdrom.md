# CD-ROM controller

This page documents the deterministic command and register model in `src/cdrom.c`, `src/cdrom_audio.c`,
`src/cdrom_sector.c` and `src/cdxa.c`.
It covers the indexed registers, FIFOs, sector timing, RDDATA delivery, command responses, region answers and lid changes.
All deadlines use CPU ticks, integer arithmetic and the instance scheduler.

## Register banks and FIFOs

The four byte ports at `1F801800h`-`1F801803h` use the low two bits written to `1F801800h` to select a bank.
Byte, halfword and word accesses are split into byte lanes by the bus.

| Port | Bank 0 | Bank 1 | Bank 2 | Bank 3 |
|---|---|---|---|---|
| `1800 R` | HSTS | HSTS | HSTS | HSTS |
| `1800 W` | Select bank | Select bank | Select bank | Select bank |
| `1801 R` | RESULT | RESULT | RESULT | RESULT |
| `1801 W` | COMMAND | WRDATA | CI | ATV3 |
| `1802 R` | RDDATA | RDDATA | RDDATA | RDDATA |
| `1802 W` | PARAMETER | HINTMSK | ATV0 | ATV1 |
| `1803 R` | HINTMSK | HINTSTS | HINTMSK | HINTSTS |
| `1803 W` | HCHPCTL | HCLRCTL | ATV2 | ADPCTL |

ATV0-ATV3 writes stage an 8-bit matrix; ADPCTL bit 5 applies all four staged values together. The reset matrix is
`80h,00h,80h,00h` (normal stereo). The active matrix feeds the bounded CD audio queue; a full queue drops new
samples, and underflow at the SPU sample tick supplies silence. The sample queue holds at most two CDDA sectors
(1,176 stereo pairs) and is serialized with the controller state. CDDA sectors are read at the selected drive speed
and feed the fixed native 44,100-Hz SPU sample clock; CDDA/XA volumes follow ATV then SPU AVOL [SCM.02][SCM.03].

HSTS reports the selected bank in bits 0-1, parameter empty/write-ready in bits 3-4, result ready in bit 5, sector
data request in bit 6 when BFRD is enabled and unread data is available, and command-transfer busy in bit 7. BUSYSTS
is set after a command write until the sub-CPU accepts it; it can clear while the separate status byte still reports
Seek or Read. Bit 2 (ADPBUSY) stays clear until XA decoding is called by the sector path. The
parameter and result FIFOs each hold 16 bytes. Writes to a full parameter FIFO are ignored; empty result reads return
zero. The circular result FIFO preserves unread bytes when new responses wrap around it.

HINTSTS bits 0-2 contain the current queued interrupt number. Bits 3-4 report result-empty and parameter-write-ready;
bits 5-7 read as one. Interrupt numbers are queued as distinct values: INT1 through INT5 are never combined with a bitwise
OR. HINTMSK bit `n-1` enables INTn on IRQ2. HCLRCTL bit `n-1` acknowledges the current INTn; bit 6 clears the parameter
FIFO and bit 7 resets the CD controller. IRQ2 is latched in I_STAT, so software also clears its I_STAT bit.

## Command behavior

A command is accepted only when no command or interrupt is pending. It reads its parameters from the bounded FIFO;
wrong parameter counts return INT5 with error `20h`, and malformed BCD or ranges return INT5 with error `10h`. Rejected
parameters remain in the FIFO, as do parameters after unsupported commands. Unsupported commands return INT5 with
status `11h` and error `40h`. The status byte uses bit 7 for Play, 6 for Seek, 5 for Read, 4 for sticky ShellOpen, 3 for IdError, 2 for
SeekError, 1 for motor on and 0 for Error.

| Command | Implemented response and state change |
|---|---|
| `01 Nop` | INT3 and status; clears sticky ShellOpen while the lid is closed |
| `02 Setloc` | Validates BCD minute, second and frame; records the target LBA and returns INT3 |
| `03 Play` | Optional BCD track parameter selects the TOC track; zero continues from the current position |
| `04/05 Forward/Backward` | INT3 while playing; otherwise INT5 with error `80h` |
| `06 ReadN`, `1B ReadS` | Starts Read and returns INT3; if Setloc differs from the current position, the drive seeks before delivering sectors. No disc or a region rejection returns INT5 (unlicensed read status `03h`, error `40h`) |
| `07 MotorOn` | Starts the motor, returns INT3, then status and INT2 |
| `08 Stop` | Stops Read/Play/Seek, returns INT3, then status and INT2 |
| `09 Pause` | Stops Read/Play, returns INT3, then status and INT2 |
| `0A Init` | Resets mode/activity, starts the motor and returns INT3 then INT2; a duplicate while pending is dropped |
| `0B/0C Mute/Demute` | Changes the XA mute state and returns INT3 |
| `0D Setfilter` | Stores file and channel bytes and returns INT3 |
| `0E Setmode` | Stores mode and speed, returns INT3 |
| `0F Getparam` | Returns status, mode, zero, file and channel with INT3 |
| `10 GetlocL` | Returns the newest buffered sector's eight-byte header at raw offset `0Ch`; errors if no sector is buffered, the medium is audio, a seek is active, or SeekError is set. A completed data-track seek requests its target sector through the host callback so the header is available immediately after SeekL completes |
| `11 GetlocP` | Returns the latest CRC-valid track/index/MSF Q record from host subcode; a TOC-derived record is used until one is available. Errors after a failed seek |
| `12 Setsession` | Rejects session zero; other values return INT3 then INT2 |
| `13 GetTN` | Returns status and first/last track numbers in BCD |
| `14 GetTD` | Returns the selected track's index-1 MSF, or lead-out for track zero |
| `15/16 SeekL/SeekP` | Starts the integer seek deadline; SeekP arrives 25 sectors early; completion returns INT2. SeekL on audio media returns INT5 with SeekError after about two seconds |
| `19 Test 20h/22h` | Returns the selected regional firmware date/version or region string |
| `1A GetID` | Returns INT3, then the eight-byte media/licence answer and INT2 or INT5 |
| `1C Reset` | Resets mode/activity and returns INT3 after `400000h` cycles |
| `50h`-`56h` unlock | Ordered payload sequence; each command returns INT5/error `40h`; supported only by the non-Japanese unlock profile |
| `57 Lock` | Clears the unlocked state; INT5 with status `11h` and error `40h` |

MotorOn uses a fixed 476,000-cycle completion delay. Init uses the same 476,000-cycle second-response delay. Stop uses
`D38ACAh` cycles at 1x, `18A6076h` at 2x, or `1D7Bh` when already stopped. These Stop values are the published
controller averages, held fixed for determinism; they are not per-drive measurements. Other commands use the fixed
50,000-cycle first response selected by DEC.44.

## Timings

One CPU tick is one 33.8688 MHz CPU cycle. These choices are deterministic model values; the cited log ranges show why a
fixed point was selected and do not claim that every console drive has identical latency.

| Event | Selected delay | Evidence and confidence |
|---|---:|---|
| First command ACK | 50,000 cycles | Selected from the ps1-tests average 45,993; measured runs span 24,024-137,104 cycles (SCM.12, medium confidence) |
| Init completion | 476,000 cycles after ACK | Selected from the ps1-tests completion average 476,300 (SCM.12, medium confidence) |
| GetID answer | `4A00h` cycles after ACK | Published controller average (SCM.02, medium confidence) |
| Pause completion | 2,170,000 cycles at 1x; half at 2x; 7,666 when already paused | Fixed DEC.44 baseline from published averages; tests pin both speeds and the short repeated case |
| Seek | Integer Q16 square-root model, 1-150 ticks, plus 3 ticks; 80 more ticks after a speed change; scale by 1x/2x | Inferred from ares' published formula (SCM.10); low confidence because no hardware seek curve is recorded |
| Reset response | `400000h` cycles | Published controller behavior (SCM.02) |

The pinned timing source log records minimum, maximum and average bands. The release guest's TTY prints the row
template with literal format placeholders, so the harness checks its ordered rows and compares host callback
intervals for consecutive LBA requests against those source-log bands. The host callback does not expose a read
generation, so an adjacent-LBA restart is part of this observable interval definition; nonconsecutive LBA gaps are
excluded. The harness also verifies the selected 451,584-cycle 1x and 225,792-cycle 2x periods as exact model values
in unit tests. This does not claim that the fixed schedule reproduces an individual drive's timing distribution.

The seek model computes the radial term from the difference of Q16 square roots of `LBA + 150`, then applies
`floor(0.5 * (3 + 0.2*r + 2.5*sqrt(r) + distance/60000))`, clamped to 1-150 ticks. Tick conversion is 451,584 CPU
cycles at 1x and half that at 2x. The maximum selected seek is 233 ticks at 1x, including the three-tick base and
speed-change delay. It has no random component or wall-clock input.

Pinned Getloc observations keep the raw sector header separate from the Q position. The run at `00:02:16` reports a
header at `00:02:16` and Q absolute position `00:02:11`; seeks at `00:00:30` and `74:00:00` report distinct lead-in
and lead-out records. GetlocP uses each validated host Q record directly, without applying a general seek offset.

## XA-ADPCM decode and resampling

The decoder accepts a complete 2,352-byte raw Mode2/Form2 audio sector. It checks the sync/mode bytes, matching
subheaders, audio and Form2 flags, supported coding fields, and the repeated 16-byte XA portion headers. Each sector
contains 18 portions of 128 bytes and produces bounded stereo PCM at the encoded 37.8 or 18.9 kHz rate. Mono input
is copied to both output channels. Reserved channel, sample-width and coding values, nonzero portion-header reserved
bits, and XA emphasis are rejected before decoder state changes. The emphasis formula remains unknown, and no known
PSX game uses XA emphasis.

The four XA predictor pairs are `(0,0)`, `(60,0)`, `(115,-52)` and `(98,-55)`. A signed 4-bit sample is expanded by
4,096 before its shift; a signed 8-bit sample is expanded by 256. Shift values 13-15 use shift 9. Each sample adds
`(old * positive + older * negative + 32) / 64` and saturates to signed 16-bit. Division truncates toward zero for the
predictor; the 8-bit right shift uses explicit sign-preserving floor division. Two predictor samples per channel carry
across portions and sectors, and reset clears them.

The resampler keeps 29 input samples per channel and applies the seven documented Q15 tap columns in
[`src/cdxa.c`](../../src/cdxa.c), sourced from [psx-spx CDROM Format, XA-ADPCM interpolation](https://psx-spx.consoledev.net/cdromformat/).
It emits seven samples after each six decoded samples. At 18.9 kHz, it duplicates each decoded input sample before
filtering. Its six-step phase starts at zero and, like both histories, is instance state. The chosen integer baseline
divides the signed tap sum by 32,768 with truncation toward zero, then saturates to signed 16-bit. The source notes
small rounding mismatches and a possible two-stage hardware implementation; without a hardware capture, these exact
rounding and phase choices are deterministic behavior, not proof of a silicon waveform. The decoded samples are not
yet connected to the SPU audio queue; the later XA/SPU integration plans own that path.

## Region, firmware, licence and lid

The machine profile region selects the default controller identity. Test `20h` returns the vC2 marker and the date
`96-09-12` for NTSC-J or `97-01-10` for NTSC-U/C and PAL. Test `22h` returns `for Japan`, `for U/C` or `for Europe`.
These are selected profile defaults, not an exposed firmware-version field.

GetID compares the machine region with the licence line at offset `20h` in raw sector 4's data area (raw offset `38h`).
This sector check supplies the region; it is separate from the physical SCEx lead-in wobble, which the host callback does
not expose. `SCHWAY_PROFILE_CD_LICENSED` is an explicit host override that treats a data disc as licensed for the
selected profile region. A matching Mode2 disc returns INT2 with `02 00 20 00 SCEx`; a matching Mode1 disc returns
`02 00 00 00 SCEx`. No disc, audio media, an unmarked data disc and a mismatched region return the documented INT5
response classes. `SCHWAY_PROFILE_CD_UNLOCK` enables the ordered `50h`-`56h` unlock sequence on non-Japanese profiles;
each part still reports INT5/error `40h`. Once the sequence completes, ReadN/ReadS can read an unlicensed or mismatched
disc, while GetID continues to report the image's licence answer.

Opening the lid cancels active reads, play and seek, sets sticky ShellOpen and queues an unsolicited INT5 with error
`08h`. Nop clears ShellOpen after the host closes the lid. ReadN/ReadS and Play request one sector per 451,584 cycles,
halved at double speed. Host callbacks run only while a public run call advances the instance. The bounded eight-slot
ring overwrites its oldest sector on overrun; RDDATA reads use the 800h or 924h window selected by Setmode, advance the
oldest-sector cursor, and repeat the documented tail byte after a window is consumed. BFRD gates reads and DRQ status.
Starting ReadN/ReadS discards a raw target sector cached for GetlocL, so the first RDDATA bytes come from a sector
delivered by the active read.
At most one INT1 waits behind an already queued interrupt. ReadN retries a failed or malformed sector once; ReadS
reports the failure immediately. A mismatched mode-2 file/channel filter retries once in ReadN and is skipped in ReadS.
The mode-2 XA audio/realtime test runs before data selection, so accepted XA payloads do not reach RDDATA; the decoder
is implemented, while its sector caller and SPU audio output are connected by later XA/SPU plans. Play accepts raw
2352-byte CD-DA sectors on audio tracks without
requiring a data header. Supplied Q subcode is deinterleaved and checked for BCD fields, TOC track range and CRC.
Missing or invalid Q is synthesized from the TOC until a valid record exists; later missing or invalid records preserve
the last good position. `current_lba` follows valid Q position unless the host's `bad_q` callback marks that sector,
in which case the previous good position remains visible. Disc callbacks and TOC remain host-owned and are not part of
save states; controller flags, sector bytes, the latest validated 12-byte Q record, FIFO contents and scheduled
deadlines, ATV staging and active values, CDDA queue contents, XA predictor history and resampler history are
serialized in state format 13.

## Sources and confidence

- Register banks, command semantics, status/error bytes and regional answers follow the local research synthesis
  [PSX-SPU-CDROM-AND-MDEC, sections 2.1-2.6](../../.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md), sourced there as
  SCM.02 (psx-spx) and SCM.12 (ps1-tests).
- ACK, Init, Pause and INT1 choices are recorded in [DEC.44](../../.planning/preparation/DECISIONS.md); region,
  firmware and unlock choices are in [DEC.45](../../.planning/preparation/DECISIONS.md).
- The integer seek formula is an adaptation of ares' model (SCM.10), with low confidence pending the plan's fixture
  timing capture on hardware.
- The sector-4 licence line supplies a profile-region comparison, but does not decode the SCEx lead-in wobble. The
  licensed profile override is the deterministic host input when that line is unavailable.
