---
title: "PSX development-community extracts for schwaystation (PS1 / PSX)"
summary: "Paraphrased high-signal facts from psx-spx (nocash original and the consoledev.net edition), the PCSX-Redux nugget test farm, ps1-tests and the other PS1 test-program authors, with who measured what and how; the test-program inventory with licences checked at pins; the toolchain pins (armips, mipsel-none-elf GCC, PSn00bSDK, mkpsxiso) with SHA-256 digests; the BIOS stand-in options; the fixture candidate list; and the hash databases."
read_when: "Before citing any community (T2/T4) claim about R3000, GTE, GPU, SPU, CD-ROM, MDEC or timer behaviour; before choosing a test program, vector set, fixture, assembler, disc-image tool or BIOS stand-in for CI; before keying the compatibility ledger."
updated: 2026-10-19
---

# PSX development-community extracts

Harvested 2026-10-19. Method: `gh api` for every GitHub repository (licence field, LICENSE file, per-file headers, tree, commits, releases), `curl` of raw files at the pin, live fetches of psx-spx (consoledev.net), problemkaputt.de, psx.amidog.se and redump.org, and `shasum -a 256` on every archive this file pins. No Sony BIOS, no game bytes and no test executable was downloaded; only source tarballs and tool release zips. Tier: psx-spx pages are T2, repositories T1 for what they contain, hardware-farm result files T1 (measurement with stated method), emulator behaviour T3, forum posts T4. Prose is paraphrase; quotes are limited to register names, byte strings and single numbers. www.psxdev.net (wiki and forum) has been offline since October 2025 and the Wayback Machine holds no capture of its help or forum pages [PX.08], so every psxdev.net claim in any sibling file is (unverified) until the site returns; this file routes each topic to a source that answers today.

**Evidence types:** HW = measurement on a console with a stated method; farm = PCSX-Redux hardware farm run (nightly, real consoles, result files committed); test = program run on a console and its log committed; doc = nocash text or Sony text transcribed; EMU = behaviour of an emulator; RC = recollection; (derived) = this harvester's inference.

## 0. Quick reference

| Topic | Fact | Evidence | ID |
|---|---|---|---|
| Clocks | CPU 33.8688 MHz (44100 × 300h); PSone PAL video clock = CPU × 11/7 = 53.2224 MHz; the edition adds NTSC 53.693175 MHz and PAL 53.203425 MHz for the separate-crystal boards | doc | PX.10, PX.11 |
| Lines, dots | NTSC 263 lines of 3413 video cycles; PAL 314 lines of 3406; dotclock divisors 10/8/7/5/4 for 256/320/368/512/640; Timer0 on dotclock drops the fraction | doc | PX.12 |
| GTE command cost | RTPS 15, RTPT 23, NCLIP 8, MVMVA 8, NCDT 44 CPU cycles and so on; SCPH-1001 and SCPH-5501 byte-identical | farm | PX.20 |
| CD response | Nop first response about C4E1h cycles (33 MHz units) on a PAL PSone; GetID second response about 4A00h; Pause about 5 sectors; Stop single-speed about D38ACAh | HW | PX.14 |
| Timer sanity | a 1000-cycle busy loop reads 1011 on Timer0 (system clock) and 198 on the dotclock; a frame is about 112,5xx system clocks NTSC | test | PX.21 |
| MULT/DIV | MULT/MULTU 6/9/13 cycles by operand magnitude; DIV/DIVU fixed 36; cop2 writes land after 2..3 cycles | doc | PX.13 |
| SPU cadence | 768 system clocks per 44.1 kHz sample; about 720 of them are SPU RAM accesses (24 voices × 8 + 4 × 8 + 14 × 8); reverb at 22.05 kHz; writes realised at the sample rate | HW (logic analyser) | PX.27, PX.28 |
| CD data rate | 75 sectors per second at normal speed, 150 at double; XA sectors go to the decoder, not the data FIFO | doc | PX.29 |
| Test suites | ps1-tests MIT (MPL runtime in the binaries); nugget tests and OpenBIOS MIT inside the GPL-2 PCSX-Redux tree; amidog NC; PeterLemon and simias unlicensed | repo | PX.31–PX.35 |
| Vector set | SingleStepTests/r3000 exists: MIT, 55 instruction files, 55,000 cases, no GTE, no exceptions | repo | PX.30 |
| BIOS stand-in | OpenBIOS is MIT in `pcsx-redux/nugget` and boots many games; PCSX-Redux runs headless with it | repo + doc | PX.60, PX.61 |

## Where to look

| Question | Section |
|---|---|
| Which psx-spx to cite, at which pin, and why not the master branch | 1 |
| Hardware measurements that exist today and who made them | 2 |
| Every PS1 test program, its licence at pin, what it verifies, how it reports | 3 |
| Is there a MIPS SingleStepTests set; GTE vectors | 3.1 |
| Toolchain pins and digests for own fixtures; what bytes a build carries | 4 |
| BIOS stand-ins for CI: OpenBIOS, own stub kernel, HLE | 5 |
| Fixture candidates and what each proves | 6 |
| Hash databases, their terms, the hash method | 7 |

## 1. psx-spx provenance and edition

| ID | Fact | Source | Pin |
|---|---|---|---|
| PX.01 | The original is Martin "nocash" Korth's `psx-spx.htm` (also `.txt`) at problemkaputt.de; the page carries no version line in its header, only an About & Credits section; the fork's README records a new nocash release on 2022-08-20 | problemkaputt.de (fetched); fork README | accessed 2026-10-19 |
| PX.02 | The consoledev.net edition is `psx-spx/psx-spx.github.io`, a Markdown conversion done with an awk script, built with MkDocs; default branch `stable`; the site says the development version "includes recent AI-written changes" | repo README; psx-spx.consoledev.net | `stable` = `6d7d1bc106a7` (2026-10-08); `master` = `e303e4aba7fc` (2026-10-19) |
| PX.03 | No licence: the fork states it could not reach nocash and acquired no copyright or licence; it relies on fair use and derivative-work doctrine. It also states the document is **not clean-room**: parts were copied or paraphrased from confidential Sony code and documentation | repo README | `6d7d1bc106a7` |
| PX.04 | Consequence for schwaystation: cite psx-spx as T2 for facts, paraphrase only, never copy a passage, and pin `stable`, never `master` (AI-written edits are unreviewed by hardware) | (derived) | |
| PX.05 | Edition page set (docs/): cpuspecifications, geometrytransformationenginegte, graphicsprocessingunitgpu, soundprocessingunitspu, cdromdrive, cdromfileformats, macroblockdecodermdec, dmachannels, timers, interrupts, memorycontrol, memorymap, iomap, controllersandmemorycards, serialinterfacessio, kernelbios, pocketstation, hardwarenumbers, unpredictablethings, pinouts, psxdevboardchipsets, konamisystem573, arcadecabinets | repo tree | `6d7d1bc106a7` |
| PX.06 | The edition keeps nocash's original GPU timing notes as a quoted block under "Nocash's original GPU Timings notes" and prefixes its own measured crystal values; where the two differ, this file records both | gpu page | `6d7d1bc106a7` |
| PX.07 | Credits page names contributors to the original (CDINFO.TXT by doomed with Barubary) and the GitHub contributors; no per-fact attribution exists, so "who measured" below comes from the pages' own wording or from the nugget result files | aboutcredits.md | `6d7d1bc106a7` |
| PX.08 | psxdev.net: front page says "Temporary Shutdown" since October 2025 pending a server migration; `archive.org/wayback/available` returns no snapshot for `psxdev.net/help/testing.html` or the forum index | fetched 2026-10-19 | |

- **Recommendation (PX.09):** cite the consoledev.net edition at `stable` `6d7d1bc106a7` for every psx-spx fact, and nocash's `psx-spx.htm` only where the edition marks a passage as nocash's original. **Strongest alternative:** nocash's `.txt` as the sole source (one author, no AI edits, but no corrections since 2022-08-20 and no pinnable revision). **Would change if:** the fork publishes tagged releases or the `stable` branch starts carrying unreviewed AI edits (then pin the last reviewed commit by hand).

## 2. Hardware measurements and who made them

### 2.1 Clocks, GPU and timers

| ID | Fact | Who, how | Source | Evidence |
|---|---|---|---|---|
| PX.10 | CPU clock 33.868800 MHz = 44100 × 300h; PSone PAL video clock 53.222400 MHz = CPU × 11/7 | nocash, from the PSone PAL board | gpu page, nocash block | doc |
| PX.11 | Edition values: NTSC video clock 53.693175 MHz, PAL 53.203425 MHz; derived refresh: NTSC-mode on PAL clock 59.393 Hz interlaced / 59.280 non-interlaced, PAL-mode on NTSC clock 50.460 / 50.219; recommendation to use the matching clock per region | edition authors (crystal values; method not stated) | gpu page | doc (unverified method) |
| PX.12 | 263 lines NTSC, 314 PAL; 3413 and 3406 video cycles per line ("or 3413.6 or so?", "3406.1 or so?"); dots per line = cycles / divisor; Timer0 on dotclock ignores fractions (340 for 340.6, but 425.75 rounds up); hblank and dotclock run during blanking; frame ≈ 59.29 / 49.76 Hz with the PSone clock | nocash | gpu page | doc |
| PX.13 | MULT/MULTU fast 6, med 9, slow 13 cycles by `rs` magnitude; DIV/DIVU fixed 36; leave hi/lo alone for about 2 cycles after; cop2 register writes take 2..3 cycles (IRGB 3); store delays counted in clocks | nocash | cpuspecifications page | doc |
| PX.14 | CD-ROM response timings in 33 MHz units on a PAL PSone: Nop first response avg C4E1h (4A73h..3115Bh), stopped 5CF4h; Init first response 13CCEh; second responses GetID 4A00h, Pause single-speed 21181Ch and double 10BD93h (about 5 sectors), Pause-when-paused 1DF2h, Stop single D38ACAh, double 18A6076h; seek times unknown and distance-dependent | nocash, counting loops on a PAL PSone | cdromdrive page "Response Timings" | HW |
| PX.15 | DMA uses DRAM hyper-page mode at about 1 clock per word; a CPU DRAM access costs 1 opcode cycle plus 6 wait states (7 total); CDROM/SPU/PIO timings depend on the Memory Control delay registers | nocash | dmachannels page | doc |
| PX.16 | The video and CPU clocks come from different crystals on most boards, so a CPU-cycle count converts to video cycles only within crystal tolerance; the PAL-to-NTSC line-length ratio on one console is crystal-independent and decides between candidate line lengths; some consoles stop video timing entirely in the other region's mode | PCSX-Redux authors, `tests/scanline-timing` (Timer2 on sysclk, Timer1 hblank, Timer0 dotclock over 40 vblank edges) | nugget `tests/scanline-timing/scanline-timing.c` | farm (probe, prints) |

### 2.2 GTE, CPU and bus (nugget hardware farm)

| ID | Fact | Who, how | Source | Evidence |
|---|---|---|---|---|
| PX.20 | GTE command cycles measured as `cop2 cmd; mfc2 IR1` pairs, 64 per block, base subtracted: RTPS 15, RTPT 23, NCLIP 8, OP 6, DPCS 8, DPCT 17, DCPL 8, INTPL 8, MVMVA 8 (both forms), NCS 14, NCT 30, NCCS 17, NCCT 39, NCDS 19, NCDT 44, CC 11, CDP 13, SQR 5, AVSZ3 5, AVSZ4 6, GPF 5, GPL 5; equals the psx-spx list; NCCS reads 17.09 (10 cycles over the block, not a whole cycle); SCPH-5501 and SCPH-1001 identical | PCSX-Redux authors on two consoles | nugget `tests/gte-timing/RESULTS.md` | farm |
| PX.21 | Timer sanity log: Timer0 on the system clock reads 1011 for a 1000-cycle delay loop and 5011 for 5000; on the dotclock 198 and 983; a frame is 112,0xx..112,5xx system clocks (NTSC, detected and forced); Timer1 identical on the system clock | Jakub Czekański, `timers` test on a console | ps1-tests `timers/psx.log` | test |
| PX.22 | 1150 GTE register/opcode cases pass on a console with expected values generated by the `gte-fuzz` program (seed C0FFEEh, every 25th sample) and captured from hardware | Jakub Czekański | ps1-tests `gte/test-all/psx.log`, `tests.c` | test |
| PX.23 | nugget hardware farm: the test suite runs on real consoles once a night on `main`, about 47 leases on shared consoles, post-merge only and never a required check; result files are uploaded as `hwfarm-results` | PCSX-Redux authors | nugget `.github/workflows/hwfarm.yml` | farm |
| PX.24 | Probes that print hardware numbers without a verdict (sources for later SYS/GPU/SCM facts): `bcc-bits` (bus timing), `cdrom/cd-cmd-latency`, `cop0-sr`, `dma-modes`, `dma-priority`, `dma-chopping`, `gpu-fifo`, `gpu-info8`, `gte-timing`, `gte-math-bench`, `mult-timing`, `scanline-timing`, `spu-capring`, `spu-envx`, `spu-wide-writes`, `autoinc`, `bcc-narrow`, `exp2-width`, `regwrites`, `2mb-vram/*` | PCSX-Redux authors | nugget `tests/run-emulator.sh` SKIPS list | farm |
| PX.25 | Verdict tests graded on a positive success line (`Synthesis: SUCCESS ... Failing: 0` from libcester): basic, cop0, cop-branch, cpu (LWL/LWR interlock and delay), dma, gpu-command, gpu-fbrect, gpu-irq, gpu-polyline, gpu-vram-move, gpu-async-stress, gte, gte-latency (+color, dpc, math, misc, mvmva, perspective, singles), irq-branch-slot, libc, load-timings, memcpy, memops-unroll, memcard, memcard-bios, memset, pcdrv, psyqo, rumble, spu-volsweep, spu-irqrefire, sr-write, timers, gpu-raster-phase1..23, 2mb-vram probes | same | same TESTS list | farm + EMU |
| PX.26 | The LWL/LWR pair is interlocked: run back to back on one register with no delay the register still reads the original value; used alone, the half-written value is masked | PCSX-Redux authors | nugget `tests/cpu/cpu.c` | farm |

### 2.3 SPU, CD-ROM, MDEC and timers (edition pages)

| ID | Fact | Who, how | Source | Evidence |
|---|---|---|---|---|
| PX.27 | SPU: 512 KB RAM, 24 voices, 44100 Hz output; BRR ADPCM blocks of 16 bytes (header, shift/filter byte, 14 data bytes = 28 samples); the SPU applies a written value at the 44100 Hz rate, so a write can take up to 300h CPU clocks to be realised; 32-bit I/O writes are occasionally missed, so drivers split them into 16-bit writes (nugget `spu-wide-writes` probes the miss count) | nocash; edition authors | soundprocessingunitspu page | doc; farm probe |
| PX.28 | SPU internal state machine: 768 system clocks per 44.1 kHz sample (33.8688 MHz / 44100); the per-sample job measured as 24 voices × 8 + 4 × 8 + 14 × 8 = 720 of 768 cycles of RAM access, which also explains reverb running at 22.05 kHz; findings from a logic-analyser log of the SPU RAM bus during the boot sound, register values known from emulators | edition contributors (logic analyser on the SPU RAM bus) | same page, "SPU Internal State Machine from SPU RAM Timing" | HW |
| PX.29 | CD-ROM: Read commands deliver 75 sectors per second at normal speed and 150 at double speed; mode bits: 7 speed, 6 XA-ADPCM to SPU, sector sizes 800h or 924h; XA sectors are played by the decoder, not delivered as data; Sound Map mode lets XA-ADPCM be fed from main RAM, which the page recommends for testing the decoder from a program | nocash | cdromdrive page | doc |
| PX.17 | Timers: Timer0 dotclock or sysclk, Timer1 hblank or sysclk, Timer2 sysclk or sysclk/8; sync modes 0..3 per source (pause during blank, reset at blank, reset and pause outside blank, pause until one blank then free run); one-shot vs repeat IRQ, INTF toggling; a timer on an external clock must be read until two reads agree because the GPU clock is asynchronous to the CPU | nocash | timers page | doc |
| PX.18 | MDEC: the IDCT scale matrix and the quantisation tables are uploaded by command; ps1-tests `mdec/step-by-step-log` feeds data by hand and logs the status register at every step, `4bit`/`8bit` decode one 8×8 block each from a sanitised bitstream, `frame` decodes a whole frame through DMA in 15- and 24-bit modes; nugget `mdec-roundtrip` is a rig fed over pcdrv | nocash; Czekański; PCSX-Redux authors | macroblockdecodermdec page; ps1-tests README; nugget SKIPS | doc; test |
| PX.19 | Nothing found on hardware-measured GPU rasterisation timing beyond the edition's "GPU Rendering Timings" research section and nugget `gpu-raster-phase1..23` (verdict tests whose expectations come from the farm); schwaystation treats polygon fill timing as a locator, not a gate, until a RESULTS.md row exists for it | search 2026-10-19 | | (unverified gap) |

## 3. Test program inventory (consumed by CF; licences by AP)

Checked 2026-10-19 with `gh api repos/<o>/<r>` (licence field), `contents/LICENSE`, and the headers of at least one source file per repository. "Bytes" lists what a built executable contains beyond the author's code.

| ID | Suite | Author | Licence at pin (what was checked) | Bytes in a build | Covers | Reports via | Pin |
|---|---|---|---|---|---|---|---|
| PX.30 | SingleStepTests/r3000 | SingleStepTests (generator: ares fork `ares_r3000_ssts`, ISC) | MIT (LICENSE, 2025) | n/a (JSON) | 55 instruction files × 1000 cases: ADD ADDI ADDIU ADDU AND ANDI BCondZ BEQ BGTZ BLEZ BNE BREAK DIV DIVU J JAL JALR JR LB LBU LH LHU LUI LW LWL LWR MFHI MFLO MTHI MTLO MULT MULTU NOR OR ORI SB SH SLL SLLV SLT SLTI SLTIU SLTU SRA SRAV SRL SRLV SUB SUBU SW SWL SWR SYSCALL XOR XORI; **no GTE, no COP0 exceptions (disabled), flat 32-bit RAM, no MMIO, no alignment faults**; instructions only in 00000000h..EFFFFFFFh | `.json.bin` (23,946,132 bytes total, about 431 bytes per case); `transcode_json.py` documents the binary | `51e227447d0b` (2026-09-04) |
| PX.31 | JaCzekanski/ps1-tests | Jakub Czekański and contributors (CONTRIBUTORS.md) | MIT (LICENSE 2020; `common/test.c` no header) | PSn00bSDK `libpsn00b` (MPL-2.0) and libgcc via the author's docker image (GCC 7.4.0 `mipsel-unknown-elf`) | cpu: access-time, code-in-io, cop, io-access-bitwidth; gte: test-all (1150 hardware cases), gte-fuzz; gpu: animated-triangle, bandwidth, benchmark, clipping, clut-cache, gp0-e1, gpustat, lines, mask-bit, quad, rectangles, texture-flip, texture-overflow, transparency, triangle, uv-interpolation, version-detect, vram-to-vram-overlap; spu: memory-transfer, playback, ram-sandbox, stereo, test, toolbox; cdrom: disc-swap, getloc, terminal, timing, volume-regs; dma: chain-looping, chopping, dpcr, otc-test; timers; mdec: 4bit, 8bit, frame, movie, step-by-step-log; input: pad; timer-dump; tools: diffvram | `vram.png` expected images per GPU test, `psx.log` hardware logs, printf to TTY; executables set SP = 0 | `f727802fead1` (2023-03-05); release `build-158` `tests.zip` 6,613,816 bytes (2021-04-22) |
| PX.32 | amidog psxtest_cpu, psxtest_cpx, psxtest_gte | AmiDog | wiki footer CC BY-NC-SA 3.0; the zips state no terms (NC applies at best) | unknown (closed source) | cpu: basic CPU behaviour; cpx: more complex CPU behaviour; gte: registers and official functions incl. unofficial SF/LM use and MVMVA CV/V/MX bits, compared against a software simulation; full GTE run takes hours on a console, stops after N errors | on screen only; no expected-results file | psx.amidog.se `psx:download:cpu` (61.5 KB, 31.7 KB; page 2013-12-31), `psx:download:gte` (59.4 KB), accessed 2026-10-19 |
| PX.33 | PeterLemon/PSX ("krom") | Peter Lemon | **no LICENSE** (API field empty; README states none) | none: pure armips assembly, own 8×8 fonts (PNG → bin), `bin2exe.py` wraps the header | CPUTest/CPU: ADD ADDI ADDIU ADDU AND ANDI DIV DIVU LOADSTORE MULT MULTU NOR OR ORI SHIFT SUB SUBU XOR XORI; CPUTest/GTE: AVSZ, NCLIP; demos: GPU, SPU, MDEC, Cube, Compress, ImageLoad, HelloWorld (16/24 bpp) | each test draws pass/fail text; a `.png` of the real-console screen sits beside every `.exe` (hardware oracle); tested only on a real PSX | `6d20c132aba0` (2026-01-20) |
| PX.34 | simias/psx-hardware-tests | Lionel Flandrin | **no LICENSE**, no README, no file headers | own runtime; GCC 8.3.0 `mipsel-unknown-elf`, `elf2psexe` from psx-sdk-rs | bios_write, cdrom_read, cdrom_version, gpu_draw_rule, gpu_reset_state, gte_commands, icache, irq_reg, joystick_ack_len, load_delay, load_delay_override, mdec_quantization, mdec_reset, mfc_delay, pocketstation_loader, pocketstation_read_kernel, processor_id, ram_load_latency, shift_variable, textures, timer1_hs_count | printf | `f9569e7c8aac` (2020-02-09) |
| PX.35 | PCSX-Redux tests (Nicolas Noble et al.) | PCSX-Redux authors | **MIT**: the tests live in the `pcsx-redux/nugget` submodule (LICENSE MIT 2019; every checked file header MIT 2020), not under the GPL-2 of the emulator | libcester (MIT) and the nugget support runtime (MIT), libgcc; built `-nostdlib -ffreestanding` with `mipsel-none-elf` | PX.24, PX.25 lists; `RESULTS.md` files with hardware numbers; `run-emulator.sh` grades each ps-exe on a positive success regex, skips listed with reasons, unlisted executables fail the run | TTY lines via PCSX-Redux `-stdout`; `-testmode` exit code | nugget `9db6d88d23c4` (2026-10-19); pcsx-redux `91285bf736af` pins nugget `6dcabd9e1efd` |
| PX.36 | 240p Test Suite | Artemio Urbina | GPL-2.0 | n/a | **no PlayStation port**: `240psuite/` holds Dreamcast, Genesis, N64, NeoGeo, PCE, SNES, Saturn, Wii, X68000 only | | `89adfdd66853` |
| PX.37 | ares `ares/ps1/*` | ares team | ISC (LICENSE 2004-2025); file headers carry no SPDX line | n/a | cpu (interpreter, delay-slots, exceptions, icache, interpreter-gte, interpreter-scc), gpu, spu, disc, dma, mdec, timer, peripheral; the r3000 vectors derive from this interpreter, so ares is not an independent oracle for them | | `9408cb43d494`; v148 2026-05-30 |
| PX.38 | MAME PSX | MAME | `src/devices/cpu/psx/psx.cpp`, `gte.cpp`, `src/devices/video/psx.cpp`, `src/devices/sound/spu.cpp` headed `license:BSD-3-Clause`; **`src/mame/sony/psx.cpp` is `license:GPL-2`** (never opened); the driver quickloads PS-EXE (`-quik`) and needs a BIOS set | n/a | precedent for CPU, GTE, GPU, SPU | | master 2026-10-19 |

**3.0a ps1-tests, per test (from the README at the pin; what each verifies and what it leaves behind)** [PX.31]:

| Area | Test | Verifies | Artefact at the pin |
|---|---|---|---|
| cpu | access-time | CPU access time to each region of the memory map | `psx.log` |
| cpu | code-in-io | whether code executes from scratchpad and from I/O ports | `psx.log` |
| cpu | cop | behaviour when touching missing or disabled coprocessors | `psx.log` |
| cpu | io-access-bitwidth | how 8/16/32-bit writes land on each I/O device | `psx.log` |
| dma | chain-looping | a self-referencing GPU ordering-table chain | `psx.log` |
| dma | chopping | SyncMode 0 chopping with GPU VRAM DMA | `psx.log` |
| dma | otc-test, dpcr | channel 6 ordering-table clear; DPCR priorities | `psx.log` |
| gpu | clipping, gp0-e1, gpustat, lines, mask-bit, quad, rectangles, texture-flip, texture-overflow, transparency, triangle, uv-interpolation, vram-to-vram-overlap, clut-cache, version-detect | draw area E3/E4, E1 vs GPUSTAT sharing, busy bits per transfer mode, Bresenham and polyline, mask during copies, fill rules, every rectangle variant, UV flip bits, UV wrap past VRAM width, the four blend modes, Gouraud, interpolation accuracy 0..255 wide, overlapping copies, CLUT cache, GP1(10h) version | `vram.png` per test (hardware frame) |
| gte | test-all, gte-fuzz | 1150 register/opcode cases; the fuzzer that produced them | `psx.log` (1150 passed), `tests.c` |
| mdec | 4bit, 8bit, frame, movie, step-by-step-log | single-block decodes, full frame by DMA (15/24-bit), in-RAM player, status at each step | `vram-15bit.png`, `psx.log` |
| spu | memory-transfer, ram-sandbox, stereo, test, toolbox, playback | DMA and IO transfers to SPU RAM, two-voice stereo, data loss on 32-bit writes | `psx.log` |
| cdrom | disc-swap, getloc, terminal, timing, volume-regs | lid open/close status, seek and read stat bits and GetlocL/P, UART terminal, command timing, volume registers | `psx.log` |
| timers | timers | Timer0/1/2 on every clock source and sync mode against busy loops and vblank | `psx.log` (PX.21) |
| input | pad | controller protocol | `psx.log` |

**Named-but-not-found (PX.39).** No "psxtest" suite other than amidog's exists under that name; "psx-hardware-tests" is simias's repository (PX.34); no PS1 build of the 240p Test Suite, ZEXALL-style exerciser, or SingleStepTests GTE set exists; no hardware-captured per-instruction bus trace was located; the psxdev.net "testing" help page and its forum threads are unreachable (PX.08). Searches on 2026-10-19: GitHub code and repository search for `psxtest`, `psx-hardware-tests`, `r3000 json tests`, the SingleStepTests organisation listing, the 240pTestSuite tree, the amidog wiki, and the Wayback availability API.

### 3.1 Vector sets and reference traces (consumed by CF)

| ID | Fact | Source | Pin |
|---|---|---|---|
| PX.40 | r3000 case shape: `name`, `opcode`, `opcode_addr`, `initial`/`final` {R[32], hi, lo, EPC, TAR, CAUSE, PC, delay.load{target (-1 = none), val}, delay.branch{slot, take, target}}, `cycles` [{actions (READ 1, WRITE 2, FETCH 4), sz, addr, val}]; README: load the initial state, execute, compare the final state and each cycle | README | `51e227447d0b` |
| PX.41 | Binary record (transcode_json.py): 51-byte Pascal name; u32 opcode, u32 address; two states of 32 × u32 R, 6 × u32 (hi lo EPC TAR CAUSE PC), 3 × u32 load (target, slot, take), i32 + u32 branch; u32 cycle count then 24-byte cycles (`=qIqI`: val, actions, addr, sz); an opcode block of 8 + 20 bytes | script | same |
| PX.42 | History: README and transcoder were corrected on 2026-09-04 "to fix branch delay and load slot confusion"; the set has no issues filed; the ares fork used for generation was last touched 2026-03-12 | commits | same |
| PX.43 | No GTE vector set exists anywhere in the SingleStepTests organisation (22 repositories listed, none for cop2); the README says a GTE set "may" follow. The two hardware-grounded GTE sources are ps1-tests `gte/test-all/tests.c` (1150 cases, MIT, input/opcode/output as 64-register arrays, 3,874,318 bytes of C) and amidog psxtest_gte (NC, screen only) | org listing; PX.22; PX.32 | 2026-10-19 |
| PX.44 | No per-instruction hardware trace capture (logic analyser) of an R3000A/CW33300 was found; the closest hardware ground truth is the farm result files (PX.20, PX.24) and ps1-tests logs (PX.21) | search | 2026-10-19 |

## 4. Toolchains for own fixtures (consumed by CF and ENG)

| ID | Tool | Licence (checked) | Pin and digest | Build | Bytes injected | PSX support |
|---|---|---|---|---|---|---|
| PX.50 | armips (Kingcom) | MIT (LICENSE.txt, 2009-2020) | master `62adab4ef30d` (2026-08-01); commit tarball `https://github.com/Kingcom/armips/archive/62adab4ef30d.tar.gz` SHA-256 `f7a65102febb4811a336a06e6d93c7e84cc86548182d54e1b1b4e2bffe562f56` (230,419 bytes, tested). Last release v0.11.0 (2020-05-10) ships a Windows x86 7z only | CMake, C++17; **the tarball lacks the two submodules** (`ext/filesystem` = Kingcom/filesystem `3f1c185ab414`, `ext/tinyformat` `6175527d738c`), so CI fetches those two trees by SHA as well, or clones `--recursive` at the pin | none: `.create`/`.close` write exactly the bytes between them; no runtime, no timestamps | `.psx` sets the R3000 instruction set; `hi()`/`lo()` helpers; `.sym` output for no$psx; overlays by multiple `.create` files; PeterLemon's `bin2exe.py` pattern or an own 2048-byte header written with `.db`/`.dw` directives |
| PX.51 | mipsel-none-elf GCC 12.3.0 (PSn00bSDK release asset) | GPL-3.0 (PSn00bSDK LICENSE.md: the bundled toolchain is unpatched GCC; sources from GNU); libgcc under the GCC Runtime Library Exception | `gcc-mipsel-none-elf-12.3.0-linux.zip` 70,339,679 bytes and `-windows.zip` 63,077,446 bytes on release v0.24 (2023-07-03); **no macOS asset**; SHA-256 recorded at first CI fetch (not computed here) | prebuilt | libgcc helpers (`__divsi3`, `__mulsi3`, soft-float) when C uses them; the exception keeps the output free | needed only to build OpenBIOS (PX.60) and nugget tests, never for own fixtures |
| PX.52 | PSn00bSDK (Lameguy64, spicyjpeg) | MPL-2.0 core (libraries, CMake, tools); examples MPL with an extra permission; mkpsxiso GPL-2+ | `5d9aa2d3dfc7` (2025-01-28); release v0.24 zips 79,398,076 (Linux) and 75,100,474 (win32) bytes | CMake + the GCC above | `libpsn00b` objects (libc, psxapi, psxcd, psxetc, psxgpu, psxgte, psxpress, psxsio, psxspu, lzp, smd) linked into every executable: MPL-2.0 file-level copyleft on those bytes, notice required, no source obligation when unmodified | the toolchain used by ps1-tests and most homebrew; not used for own fixtures |
| PX.53 | mkpsxiso / dumpsxiso | GPL-2.0 (LICENSE.md) | tag v2.30 = `b6f9a25c8a96` (2026-07-06); tag tarball SHA-256 `b44778180d239a586c13762322a1f4b9692c787db8772790ddd2fa255a75d9e0` (77,331 bytes); release assets `mkpsxiso-2.30-Linux.zip` `8e387e6db51dee3eb7cb08f4d59293a24c48f58bd541631acf9301d78015a1fc` (733,590), `-Darwin.zip` `a64d4d57d78ce57cd810a6a044602ce3cf1b552ca7c7e8675518f554206e0b49` (1,085,657), `-win64.zip` `7d3963cd322df365766cee1dbc3e405ea103bbe4e11c0db86c9688b32f5a1017` (688,613) (all tested) | CMake; submodules tinyxml2, miniaudio, flac, ghc filesystem, threadpool | none in the image beyond ISO9660 structures and the optional licence file; the GPL covers the tool | `.bin`/`.cue` or `.iso`; mixed-mode CD-XA (XA audio, STR), CD-DA tracks from wav/flac/pcm/mp3 as tracks or `da` files; `date` attribute on files and directories for byte-identical rebuilds; `<license file>` injects Sony licence data (from PsyQ, never in this repository; emulators do not need it); `dumpsxiso` reproduces an XML in LBA order |
| PX.54 | Byte identity across the six legs: armips and mkpsxiso both write only what the source names; the rules are LF-only `.asm`/`.xml` via `.gitattributes`, explicit `date` on every mkpsxiso entry, no `.incbin` outside the fixture tree, identifiers (`system_id`, `volume_id`, `data_preparer`) stated in the XML, and a `hashes.txt` equality job (cogmaster CONFORMANCE §2 pattern) | (derived from PX.50, PX.53) | |
| PX.55 | nugget's build flags for freestanding PS1 code: `mipsel-none-elf-gcc -nostdlib -ffreestanding -fno-stack-protector`, `-Os`, `--gc-sections`, linker scripts `ps-exe.ld`/`psx-bios.ld`; a note that GCC 16 folds volatile loads, so hardware-register access is wrapped | nugget `common.mk` | `9db6d88d23c4` |

| PX.56 | GNU `as`/`ld` from the GCC zip (alternative assembler) | GPL-3 tools; output not covered | the same 70 MB zip as PX.51; no macOS asset | prebuilt (Linux, Windows) | none for pure assembly; `objcopy -O binary` then an own header step | full MIPS I syntax; `.set noreorder`; linker scripts; but no `.psx`-style header helpers and a 70 MB download per leg |
| PX.57 | PSn00bSDK `elf2x` (header tool) | MPL-2.0 | part of PX.52 | CMake | writes the 2048-byte PS-EXE header from an ELF | not needed: `header.inc` writes the header in armips |

**Recommendation (PX.58).** armips at `62adab4ef30d` built from the digest-checked tarball plus its two submodule trees, for every own fixture: no runtime bytes, MIT, the R3000 instruction set and `hi()`/`lo()` helpers, multi-file overlays, a `.sym` for no$psx and our own debugger, and the same assembler PeterLemon's hardware-verified tests use. **Strongest alternative:** GNU as from the PSn00bSDK GCC zip (no macOS asset, 70 MB per leg, header written by a script). **Would change if:** the armips CMake build fails on the Windows arm64 runner across two phases (then GNU as on Linux and digest-checked copies elsewhere), or a fixture needs C (then the GCC zip with `-nostdlib -ffreestanding`, as nugget does, and libgcc's exception noted).

## 5. BIOS stand-ins for CI

| ID | Option | Facts | Source |
|---|---|---|---|
| PX.60 | **OpenBIOS** | Lives in `pcsx-redux/nugget/openbios` (MIT; README, `main/main.c` and `kernel/events.c` headers MIT 2019/2020); rewritten from reverse engineering of an SCPH-7001 dump (MD5 stated in the README) "bugs, quirks and all"; builds `openbios.bin` (ROM) or a cart build; options `BOOT_MODE=fast` (skip the shell, boot CD), `EMBED_PSEXE=file` (run an executable before the shell), `BOOT_MODE=psexe`, `INSTALL_TTY_CONSOLE=true` (DTL-H2000 host console driver instead of the dummy TTY); status: "almost all the same things as the retail BIOS", many games boot; COMPATIBILITY.md lists emulators that ship it (PCSX-Redux, Beetle PSX, Beetle PSX HW, SwanStation) and two incompatible games (Azure Dreams NTSC, Wipeout NTSC-U/C) | nugget `openbios/README.md`, `COMPATIBILITY.md`, `Makefile` at `9db6d88d23c4` |
| PX.61 | OpenBIOS distribution: no release asset on nugget or pcsx-redux (both have zero releases); `openbios.bin` is built in nugget's CI and installed into the PCSX-Redux AppImage (`make install install-openbios`) published through distrib.app dev channels; PR builds upload it as an Actions artifact | nugget `build.yml`; pcsx-redux `linux-build.yml` |
| PX.62 | Toolchain for OpenBIOS: `mipsel-none-elf` GCC (PX.51 or a distro cross-GCC), make; bytes in `openbios.bin` = MIT nugget code + libgcc; the shell inside is `shello` (psyqo) by default | nugget `openbios/Makefile`, `common.mk` |
| PX.63 | OpenBIOS legality note from its own docs: a reimplementation, not a dump; the docs carry a "Legality" section; schwaystation treats it as third-party MIT code with its notice, never as Sony firmware | pcsx-redux.consoledev.net/openbios |
| PX.64 | Own stub kernel (design, (derived)): a 1..4 KB MIT image the runner maps at BFC00000h with reset vector code that sets SR, clears I_MASK and I_STAT, sets DPCR/DICR to power-on values, installs an exception handler at 80000080h that records Cause/EPC into the mailbox and returns, then jumps to the PS-EXE's `pc0` with `sp`/`gp` from the header; a putchar TTY hook on a write to 1F802080h (the port PCSX-Redux documents for its own debug output (unverified); fallback: the A(3Ch) syscall path) and an exit mailbox word; no CD boot, no card, no shell | this file |
| PX.65 | HLE: PCSX-Redux's OpenBIOS README argues HLE creates a per-emulator maintenance burden mixing LLE and HLE; MAME needs a BIOS set; DuckStation has no HLE | nugget README; MAME `src/mame/sony/psx.cpp` ROM declarations (listing only); DuckStation docs |
| PX.66 | BIOS-free execution needs the runner to load PS-EXE directly: header "PS-X EXE", `pc0`, `gp0`, `t_addr`, `t_size`, `s_addr`, `s_size`, region string at offset 4Ch ("Sony Computer Entertainment Inc. for ..."); rcheevos sizes the file as header field +2048 | rcheevos `rc_hash_disc.c`; MAME psx.cpp `load_psxexe` field list | PX.70, PX.38 |

**Own stub kernel, concretely (PX.68, (derived); the port question is settled by ARCHITECTURE 13 row 15, applied 2026-10-19).** The runner loads `tests/fixtures/src/stub/stub.exe` bytes (armips, MIT, about 2 KB) at BFC00000h in place of a BIOS when `--bios none` is given:

| Piece | Content | Why |
|---|---|---|
| Reset vector BFC00000h | set SR = 0 (BEV cleared after the vectors are in RAM), COP2 enabled (SR bit 30), clear I_MASK and I_STAT (1F801074h/1F801070h), DPCR = 07654321h, DICR = 0, Timer modes 0, GP1(00h) reset, SPUCNT 0; copy the handler to 80000080h; read the PS-EXE header fields the runner placed at 1F800000h (scratchpad); set `sp` = `s_addr` + `s_size` (or 801FFF00h when zero, which is what ps1-tests binaries need, PX.31), `gp` = `gp0`; jump to `pc0` | the same power-on state on every leg; no kernel RAM layout to emulate |
| Exception handler 80000080h | store Cause, EPC, BadVaddr, SR and the faulting opcode into the stub's own record area at 1F800300h; if the fixture registered a handler address in its mailbox, jump there; otherwise set mailbox status FFh and spin | fixtures measure exceptions; the runner reports a fault instead of silently returning |
| TTY | the core decodes 1F802080h (byte = character) only under the profile flag `debug_port` (clear in retail profiles; independent of the DEV8 window size; never a fault), and the runner hooks it together with the A(3Ch)/B(3Dh) putchar entry points at A0h/B0h, so programs that call the kernel's putchar still print | nugget and ps1-tests print through the kernel; own fixtures through the port |
| Exit | a write to 1F802082h (byte = exit code; same `debug_port` flag) ends the run with that code; the mailbox status 00h does the same | scripts need an exit code, not a frame cap |
| No | CD boot, memory-card services, events, threads, heap, shell | those need OpenBIOS (PX.60) and are exercised by the disc fixtures |

**Recommendation (PX.67).** OpenBIOS built in CI from the pinned nugget commit with the pinned GCC on the Linux leg, digest-checked, copied to the other legs through the Actions cache, used for the disc-boot and CD fixtures and for the PCSX-Redux oracle lane; the own stub kernel for every PS-EXE fixture so the CPU and GTE milestones carry no third-party bytes at all. **Strongest alternative:** HLE kernel in the core (every sibling's "no BIOS" path). **Would change if:** OpenBIOS fails to boot an own mkpsxiso disc twice, or its build on the runner exceeds 3 minutes.

## 6. Fixture candidates (consumed by CF)

| ID | Candidate | Licence and bytes | Proves | Disposition |
|---|---|---|---|---|
| PX.70 | Own armips PS-EXEs (CONFORMANCE §2) | MIT, no runtime | boot, pad counter, moving picture, tone, GTE, timers, memcard | commit source, build in CI |
| PX.71 | Own mkpsxiso disc with the same PS-EXE plus XA and CD-DA tracks | MIT content; ISO structures | CD reads, XA audio, CD-DA, SYSTEM.CNF boot through OpenBIOS | build in CI |
| PX.72 | nugget tests (PX.35) | MIT + libcester MIT + libgcc | cpu, cop0, gte, gte-latency, dma, gpu-*, spu-volsweep, memcard, timers | build-ci at pin (GCC needed); the success-line grading is theirs |
| PX.73 | ps1-tests release `tests.zip` (PX.31) | MIT code + MPL libpsn00b bytes | GPU vram.png oracles, GTE 1150 cases, timers, DMA, MDEC | fetch at pin (digest recorded at first fetch); GTE table transcoded to own vector file (MIT) |
| PX.74 | PeterLemon CPUTest and demos | no licence | hardware screenshots per test | local only; facts |
| PX.75 | amidog psxtest_* | CC BY-NC-SA wiki; zips unstated | CPU, CPX, GTE screen verdicts | nightly fetch at most, binary-oracle; never committed |
| PX.76 | simias/psx-hardware-tests | no licence | load delays, icache, mfc delay | rejected (no licence, dead toolchain); facts only |

## 7. Hash databases

| ID | Database | Licence or terms | Shape | Pin |
|---|---|---|---|---|
| PX.80 | Redump.org | downloads page public without login for Sony PlayStation (Cuesheets, Datfile, SBI and LSD, BIOS Datfile); **no terms or licence text found on the page**; wiki.redump.org returned 404 | clrmamepro DAT per system | accessed 2026-10-19 (terms unverified) |
| PX.81 | libretro-database | CC-BY-SA-4.0 (LICENSE) | `metadat/redump/Sony - PlayStation.dat` version 2026.10.07, 13,407 `game` entries with name, region, serial, per-track rom name/size/crc/md5/sha1; `rdb/Sony - PlayStation.rdb` 7,704,228 bytes; `dat/` has only PS3 and Minis for Sony | `12c21cf79f1e` (2026-10-10) |
| PX.82 | RetroAchievements rcheevos | MIT | `rc_hash_psx`: open track 1, find the boot file from SYSTEM.CNF `BOOT = cdrom:...` (fallback `PSX.EXE`), size = header u32 at offset 28 + 2048, MD5 over the boot file name then the executable bytes | `dfdc64ea035c` (2026-10-17) |
| PX.83 | DuckStation `data/resources/gamedb.yaml` | repository LICENSE at `124a4b16feda` is **CC BY-NC-ND 4.0** (LICENSE last changed 2024-09-01; the brief's "PolyForm" is superseded at this pin): facts only, no copying | 4,946,029 bytes, keyed by serial: name, localizedName, saveName, controllers, compatibility rating, metadata (publisher, developer, releaseDate, genre, players, blocks, vibration, multitap, linkCable) | `124a4b16feda` (2026-10-19) |
| PX.84 | ares `mia/Database` | ISC | no PlayStation database file (Arcade, Atari, Famicom, MSX, Neo Geo, Super Famicom and friends only) | `9408cb43d494` |
| PX.85 | libchdr | BSD-3-Clause | CHD reader for the fetch and fuzz lanes | `47d794151b2b` |

**Recommendation (PX.86).** Key the ledger by Redump serial plus the rcheevos boot-executable MD5 (both computable by our own probe from any dump, both already public in RetroAchievements and Redump lists), carry the Redump track-1 SHA-1 as an optional identity column, and take titles from the CC-BY-SA-4.0 libretro-database DAT with its notice. **Strongest alternative:** key by track-1 SHA-1 alone (exact dump identity, but a bad dump or a different pressing of the same game gets its own row, and a CHD needs decompression to compute it). **Would change if:** Redump publishes terms that permit bulk redistribution (then the Redump DAT becomes the title source directly), or libretro-database's share-alike proves incompatible with the repository's notice policy (then titles are omitted and only serials are shown).

## 8. Access log (what answered on 2026-10-19)

| Resource | Result | Consequence |
|---|---|---|
| psx-spx.consoledev.net, raw `docs/*.md` at `stable` | 200 | T2 source for every page cited |
| problemkaputt.de/psx-spx.htm | 200 (no version line in the header) | original consulted for the nocash GPU block only |
| www.psxdev.net (home, wiki, forum, help) | shutdown notice; wiki 404; Wayback has no capture of the help or forum pages | every psxdev.net fact is (unverified) |
| psx.amidog.se download pages | 200 (wiki footer CC BY-NC-SA 3.0; zips without terms) | fetch-only disposition |
| redump.org/downloads/ | 200, public PS1 DATs, no terms text; wiki.redump.org 404 | terms (unverified); facts only |
| GitHub API for every repository in §3, §4, §5, §7 | 200 (licence field, LICENSE, headers, trees, releases, commits) | pins and digests as listed |
| github.com/grumpycoders/pcsx-redux `src/mips/*` | 404 on contents: `src/mips` is a submodule record pointing at `pcsx-redux/nugget` | OpenBIOS and tests read from nugget (MIT) |
| pcsx-redux.consoledev.net `/cli_flags/`, `/openbios/` | 200 | headless flags and OpenBIOS facts from docs only |
| mednafen.github.io/documentation | 200 | no headless mode documented |
| GitHub tarballs for armips `62adab4ef30d`, mkpsxiso v2.30, and the three mkpsxiso v2.30 release zips | downloaded and hashed (sizes in §4) | digests pinned; the 70 MB GCC zips were not downloaded |

Who to cite, by topic: nocash (clocks, GPU and timer arithmetic, CD response timings, MULT/DIV cycles); the PCSX-Redux authors and their hardware farm (GTE command cycles, LWL/LWR interlock, scanline-length method, SPU and GPU probes); Jakub Czekański (GPU reference images, GTE hardware table, timer logs); AmiDog (CPU/GTE screen verdicts); Peter Lemon (per-instruction hardware screenshots) [PX.90].

## Sources
| ID | Tier | Source | Pin or access date | Supports |
|---|---|---|---|---|
| PX.01–PX.07 | T2 | problemkaputt.de/psx-spx.htm (fetched); github.com/psx-spx/psx-spx.github.io README, branches, `docs/` tree, `docs/aboutcredits.md`; psx-spx.consoledev.net front page | `6d7d1bc106a7`, `e303e4aba7fc`; 2026-10-19 | provenance, licence status |
| PX.08 | T1 | psxdev.net front page (fetched); archive.org availability API | 2026-10-19 | outage |
| PX.10–PX.15 | T2 | psx-spx stable: graphicsprocessingunitgpu.md (GPU Timings and nocash block), cpuspecifications.md, cdromdrive.md (Response Timings), dmachannels.md | `6d7d1bc106a7` | measured and documented numbers |
| PX.17–PX.19, PX.27–PX.29 | T2 | psx-spx stable: timers.md, macroblockdecodermdec.md, soundprocessingunitspu.md (incl. "SPU Internal State Machine from SPU RAM Timing"), cdromdrive.md | `6d7d1bc106a7` | SPU, CD, MDEC, timer facts |
| PX.16, PX.20, PX.23–PX.26 | T1 | github.com/pcsx-redux/nugget `tests/scanline-timing/scanline-timing.c`, `tests/gte-timing/RESULTS.md`, `tests/run-emulator.sh`, `tests/cpu/cpu.c`, `.github/workflows/hwfarm.yml`, `tests/common.mk` | `9db6d88d23c4` | farm facts |
| PX.21, PX.22, PX.31 | T1 | github.com/JaCzekanski/ps1-tests README, LICENSE, `timers/psx.log`, `gte/test-all/{main.c,tests.c,psx.log}`, `gte-fuzz/main.c`, `common.mk`, `.github/workflows/build.yml`, releases | `f727802fead1`; `build-158` | test logs, inventory |
| PX.30, PX.40–PX.43 | T1 | github.com/SingleStepTests/r3000 README, LICENSE, `transcode_json.py`, `v1/` tree (55 files, sizes summed), commits; `orgs/SingleStepTests/repos`; `ares_r3000_ssts` LICENSE | `51e227447d0b`; `0bba0a28621c` | vector set |
| PX.32 | T2 | psx.amidog.se `psx:download:cpu`, `psx:download:gte` (fetched) | 2026-10-19 | amidog tests |
| PX.33 | T1 | github.com/PeterLemon/PSX README, tree, `CPUTest/CPU/ADD/*`, `LIB/PSX.INC` | `6d20c132aba0` | krom tests |
| PX.34 | T1 | github.com/simias/psx-hardware-tests tree, `config.mk`, `install-toolchain` | `f9569e7c8aac` | simias tests |
| PX.35, PX.55, PX.60–PX.63 | T1 | github.com/pcsx-redux/nugget LICENSE, `openbios/{README.md,COMPATIBILITY.md,Makefile,main/main.c,kernel/events.c}`, `common.mk`, `.github/workflows/build.yml`; github.com/grumpycoders/pcsx-redux LICENSE (GPL-2), LICENSES.md, `.github/workflows/linux-build.yml`, `contents/src/mips` (submodule record); pcsx-redux.consoledev.net/openbios | `9db6d88d23c4`; `91285bf736af` | OpenBIOS, tests |
| PX.36 | T1 | github.com/ArtemioUrbina/240pTestSuite LICENSE, `240psuite/` tree | `89adfdd66853` | no PSX port |
| PX.37, PX.84 | T1 | github.com/ares-emulator/ares LICENSE, `ares/ps1/*`, `ares/ps1/cpu/*`, `mia/Database`, releases | `9408cb43d494`; v148 | ares facts |
| PX.38 | T1/T3 | github.com/mamedev/mame file headers (`license:` lines fetched raw); `src/mame/sony/psx.cpp` grep for the quickload symbol names only | master 2026-10-19 | licence split |
| PX.09, PX.39, PX.44, PX.58, PX.86, PX.90 | T1 | this harvest's search log and (derived) recommendations from the rows above | 2026-10-19 | absence; attribution; choices |
| PX.56, PX.57 | T1 | PSn00bSDK release v0.24 assets; `doc/toolchain.md`; `tools/` tree | `5d9aa2d3dfc7` | assembler alternatives |
| PX.50 | T1 | github.com/Kingcom/armips LICENSE.txt, README, releases, tags, `contents/ext`; tarball downloaded and hashed here | `62adab4ef30d` | assembler pin |
| PX.51, PX.52 | T1 | github.com/Lameguy64/PSn00bSDK LICENSE.md, README, `doc/installation.md`, `doc/toolchain.md`, releases v0.24 | `5d9aa2d3dfc7` | toolchain, SDK |
| PX.53 | T1 | github.com/Lameguy64/mkpsxiso LICENSE.md, README, `examples/example.xml`, releases v2.30; tarball and zips downloaded and hashed here | `b6f9a25c8a96` | disc tool |
| PX.64–PX.68, PX.70–PX.76 | T1 | (derived) from the rows above; cogmaster CONFORMANCE §2 and §5 for the pattern | 2026-10-19 | designs |
| PX.80 | T1 | redump.org/downloads/ (fetched, text scraped) | 2026-10-19 | public DATs, no terms |
| PX.81 | T1 | github.com/libretro/libretro-database LICENSE, README, `metadat/redump/Sony - PlayStation.dat` (header read, `game (` lines counted), tree sizes | `12c21cf79f1e` | dat facts |
| PX.82 | T1 | github.com/RetroAchievements/rcheevos `src/hash/rc_hash_disc.c` `rc_hash_psx` | `dfdc64ea035c` | hash method |
| PX.83 | T1 | github.com/stenzek/duckstation LICENSE (fetched), `commits?path=LICENSE`, `data/resources/gamedb.yaml` (header read) | `124a4b16feda` | gamedb facts, licence |
| PX.85 | T1 | github.com/rtissera/libchdr licence field | `47d794151b2b` | CHD reader |
