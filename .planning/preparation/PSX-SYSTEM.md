---
title: "PlayStation system: clocks and the master tick, R3000A CPU, memory map and bus control, DMA, root counters, interrupts, BIOS and boot, regions and revisions, power-on state"
summary: "Tick-level system facts with sources and disagreements: the 768 x 44100 Hz CPU clock and the asynchronous GPU clock as exact rationals, the CW33300 pipeline, delay slots, mul/div latencies, COP0 and exception rules, the decoded memory map with the BIU delay registers, the seven DMA channels and CPU stall rules, the three root counters and their edge cases, I_STAT/I_MASK semantics, the BIOS family with public digests, the PS-EXE format and the BIOS-free CI boot decision, machine-profile fields and the power-on image."
read_when: "Before designing or changing the time base, bus, CPU core, memory control, DMA, timers, interrupt controller, BIOS loading, EXE side-loading, machine profiles, power-on state or any timing that crosses chips."
updated: 2026-11-10
---

# PlayStation system timing, CPU, maps, DMA, timers, interrupts and boot

| Question | Section |
|---|---|
| What is one tick, how are the GPU, dot, SPU and CD-ROM clocks derived from it without floating point, and what are the exact line and frame rates? | 1 |
| What must the CPU core model: pipeline, delay slots, mul/div, COP0, exceptions, caches, scratchpad, address mapping, undocumented behaviour? | 2 |
| What answers each address, how do the BIU delay registers and RAM_SIZE shape timing, what do unmapped regions do? | 3 |
| How do the seven DMA channels, DPCR, DICR, sync modes, chopping and CPU stalls work? | 4 |
| How do root counters 0 to 2 count, sync, reset, and raise IRQs; which edge cases matter? | 5 |
| How do I_STAT and I_MASK interact with COP0 CAUSE bit 10; what is edge and what is level? | 6 |
| Which BIOSes exist with which digests, what the boot does, what an EXE needs, and how CI boots without a Sony BIOS | 7 |
| Which regions and board revisions differ, and which profile fields the core needs | 8 |
| What every register holds at power-on | 9 |
| Per-subsystem confidence and the test that settles each | 10 |

Coordinates: times are CPU cycles (sysclk) unless stated; hex carries an `h` suffix. GPU pixel behaviour lives in
PSX-GPU-AND-GTE.md; SPU, CD-ROM and MDEC internals in PSX-SPU-CDROM-AND-MDEC.md.

## 1. Clocks and the master tick

| Clock | Value | Derivation | Sources |
|---|---|---|---|
| CPU sysclk | 33,868,800 Hz | 44,100 Hz x 300h (768); "should-be" value, boards reach it through different oscillators and PLLs | [SYS.11][SYS.12] |
| Board oscillators | X101 67.737 MHz (PU-7 to PU-20); X201 + IC204 PLL from 14.318 MHz NTSC (4 x Fsc) or 17.734 MHz PAL (4 x Fsc) (PU-22 to PM-41(2)) | CPU pins DSYSCK0 67 MHz, SYSCK0/1 33 MHz; CRYSTALP always fed by an external oscillator; PSone makes CPU, GPU and colour clocks from one oscillator | [SYS.11] |
| GPU video clock NTSC | 53,693,175 Hz | 15 x 3,579,545 (NTSC subcarrier); a console only has its own region's clock | [SYS.10] |
| GPU video clock PAL | 53,203,425 Hz | 12 x 4,433,618.75 (PAL subcarrier); nocash's "CPU x 11/7" = 53,222,400 Hz is nominal only and matches neither clock | [SYS.10] |
| GPU/CPU exact ratio | NTSC 715909/451584 = 1.585329; PAL 709379/451584 = 1.570868 | derived (gcd 75) | derived from [SYS.10][SYS.11] |
| Dot clock divisors | 256: /10, 320: /8, 368: /7, 512: /5, 640: /4 of the GPU clock | NTSC dots per line 341.25, 426.5625, 487.5, 682.5, 853.125 (at 3412.5 cycles/line); Timer0 drops fractions | [SYS.10] |
| GPU cycles per line | NTSC 3413 (nocash: "or 3413.6"); PAL 3406 ("or 3406.1") | exact NTSC standard gives 53,693,175 / 15,734.2657 = 3412.5; PAL 53,203,425 / 15,625 = 3405.02 | [SYS.10]; derived |
| Lines per frame | NTSC 263 (262.5 interlaced); PAL 314 (312.5 interlaced) | | [SYS.10] |
| Frame rate | NTSC 59.826 Hz progressive = 1431818/23933, 59.940 interlaced = 2863636/47775; PAL 49.761 = 15625/314, 50.000 interlaced | psx-spx's figures follow from 3412.5 and 3405.02 cycles per line, not from 3413/3406; CPU cycles per line 2152.55 NTSC, 2167.60 PAL | [SYS.10]; derived |
| SPU sample | 44,100 Hz = exactly 768 sysclk | | [SYS.12] |
| CD-ROM 1x | 75 sectors/s = exactly 451,584 sysclk per sector; 2x = 150/s | | [SYS.09] |
| ares' choice | CPU 33,868,500 Hz; GPU 53,693,175 for NTSC-J, NTSC-U and PAL alike; 3413/3406 cycles per line, 263/314 lines | one implementation's rounding, not hardware | [SYS.14][SYS.20] |

- The CPU and GPU clocks are not integer multiples on any board (denominator 451,584), so no small LCM tick exists; the SPU
  (768), CD (451,584), DMA, timer and bus delays are all exact in CPU cycles. 3413/3406 clocks per line is what every open
  implementation uses, while the refresh rates in [SYS.10] correspond to 3412.5 and 3405.02 (derived).

**C1 Tick unit.** Recommendation: one `uint64_t` counter of CPU cycles (33,868,800 Hz) is the master tick. The GPU keeps a
Bresenham accumulator with numerator 715909 (NTSC) or 709379 (PAL) and denominator 451584: for every CPU cycle add the
numerator, emit one GPU clock per 451584, so no floating point ever enters the schedule and the long-run ratio is exact.
Dot clocks divide that GPU clock by 10/8/7/5/4; SPU every 768 ticks; CD every 451,584 ticks at 1x. Strongest alternative:
the GPU clock as master (cogmaster's choice) with the CPU as a rational; rejected because the CPU, DMA, timers, SPU and CD
are all exact in sysclk and the CPU is the hot loop. Would change if: a scope measurement showed the PSone's single-PLL
boards phase-lock CPU and GPU at a small ratio (then that profile uses the small ratio directly).
**C2 Line length.** Recommendation: 3413 (NTSC) / 3406 (PAL) GPU clocks per line with the accumulator above, as every open
implementation does [SYS.20]; report the emulated rates 53,693,175/897,619 = 59.8173 Hz and 53,203,425/1,069,484 =
49.7468 Hz (derived) so fps and samples per frame agree exactly; the nominal 59.826/49.761 figures stay documentary. Would change if: a hardware counter test (timer1 on hblank against timer2 on sysclk for 1000 lines) disagrees.

## 2. The CPU (CXD8530/CXD8606, LSI CW33300 core, MIPS I)

Moved to the public pages: [docs/hardware/cpu.md](../../docs/hardware/cpu.md) (pipeline, load and branch delay, multiply and divide, loads and stores, the instruction fetch, the run loop), [docs/hardware/cop0.md](../../docs/hardware/cop0.md) (COP0 registers, SR, CAUSE, exceptions and their order, the vectors, interrupts, the interrupt that runs a GTE command twice) and [docs/hardware/bus.md](../../docs/hardware/bus.md) (segments, the instruction cache, the scratchpad, cache isolation and the write queue). Each cites psx-spx and the IDT manual and says where the core's own model replaces a measurement. The facts this section held are in those pages; the Sources table below still defines every [SYS.nn] ID that other files cite.

Corrections phase 3 found (the pages carry them): the GTE command that the interrupt entry runs a second time is the instruction at the program counter, which is not the word at EPC when the interrupted instruction sits in a branch delay slot (EPC then names the branch); and the wait for a running GTE command applies to a read of a result (MFC2, CFC2, SWC2) and to a new command, not to a move in (MTC2, CTC2, LWC2), so the row's "MFC2/CFC2 have one-instruction delays; COP2 writes need 2-3 cycles" is replaced by the stamp model on [docs/hardware/gte.md](../../docs/hardware/gte.md) (Timing); the two-instruction write-to-use latency is not modelled.

**C3 CPU core shape.** Recommendation: an interpreter with an explicit load-delay queue and branch-delay record (ares'
`delay-slots` model [SYS.15]), a mul/div completion timestamp (stall on an early HI/LO read), the full direct-mapped
I-cache with per-word valid bits, the 4-entry write queue as a cost only until a conformance test needs ordering, and one
`trigger(code)` path capturing BD/BT/EPC/TAR. Strongest alternative: MAME's delayed-register model with costs folded into
the bus [SYS.21]. Would change if: a JIT becomes necessary for the handheld budget; the interpreter stays the oracle.

## 3. Memory map and bus control

Moved to the public page [docs/hardware/bus.md](../../docs/hardware/bus.md): the physical map and the segments, RAM and its mirrors, the scratchpad, the BIOS window, the memory-control registers with their boot values and the per-port cost formula, the expansion regions and debug ports, the instruction cache and the write queue, and the interrupt registers. The Sources table below keeps [SYS.02], [SYS.03] and [SYS.19]. Nothing in this section was found wrong in phase 3; the map and the delay formula are as the page states them.

**C4 Bus model.** Recommendation: decode on physical address (mask 1FFFFFFFh after a 3-bit segment switch, as ares and MAME
do), charge the delay-register formula for every port, keep RAM_SIZE bank mirroring and bus faults real, and keep the memory
control registers writable with their boot values as the profile default so an EXE that never runs the BIOS sees the same
timing. Strongest alternative: fixed per-region costs. Would change if: ps1-tests `cpu/access-time` disagrees with the
formula on a real board; then the table is corrected from that measurement.

## 4. DMA

Moved to the public page [docs/hardware/dma.md](../../docs/hardware/dma.md): the channels and registers, DPCR and arbitration, DICR and the interrupt edge, the unit model, OTC, the sync modes and the request line, the fault for an unusable mode, chopping, linked lists, the 1000h-node guard, the hostile-program bound, the device ends, the costs and what holds the CPU.

Corrections phase 3 found (the page carries them): (1) the DICR row of the table this section held named bits 0-6 for the per-channel interrupt mode, where ares and the built core store bits 0-5 and bit 6 is unverified and unmodelled; (2) a list node whose next address is above 8 MiB cannot raise a DMA error, because a next address with bit 23 set ends the list first, so the claim that some games end a chain that way through an error is not modelled; (3) a list that passes 1000h nodes in one step yields and the channel stays busy, with no flag and no interrupt, as the hardware log of a self-referencing chain shows (it does not abort); (4) DICR bit 31 is not stored, and the per-channel enables gate only the setting of a flag, not the bit; (5) a lower DPCR priority value wins and a tie goes to the higher channel number, and the CPU priority field has no measured effect; (6) the CD-ROM channel's per-word cost is 24 in the model, and 40 is the choice of the CD-ROM device end.

**C5 DMA scheduling.** Recommendation: a single DMA scheduler on the CPU tick that runs whole slice blocks or nodes
atomically, charges the per-word costs above, stalls the CPU by the stall rule (ares `waitDMA`), and implements chopping for
sync 0 as alternating windows. Strongest alternative: per-word interleaving with the CPU. Would change if: ps1-tests
`dma/chopping` or `gpu/gpustat` show visible intra-block CPU progress on hardware that the block model cannot reproduce.

Phase 3 built the scheduler as data-atomic and time-evented: a unit moves its words at once and the channel is busy for the unit's cost, with a CPU window after it, and a bus access that waits freezes the CPU until the unit ends (see the page). The "alternating windows" recommendation above holds for chopping only.

## 5. Root counters (timers)

Moved to the public page [docs/hardware/timers.md](../../docs/hardware/timers.md): the registers and MODE bits, the clock sources, the synchronisation modes, the beam sources, lazy counting, targets and wraps, the hold, the interrupts, the deadline and the saved state.

Corrections phase 3 found (the page carries them): (1) ares holds a written timer value 1 cycle (its `WAIT_CYCLES` is 1, not 2 as the row said), while psx-spx says the value stays for 2 clock cycles and the core holds it two counter cycles after the write's own cycle; (2) a reset at the target has the period target + 2, and a wrap at FFFFh has no extra dwell (the row's "stays 0 for 1 cycle" is dropped); (3) pulse mode leaves bit 10 at 1, and toggle mode raises only when the flip takes bit 10 to 0; (4) only sysclk and sysclk/8 are modelled until the GPU line machine exists, so Timer 0 on the dot clock or hblank and Timer 1 on hblank do not advance and the access counts as unbuilt.

**C6 Timer model.** Recommendation: step timers from the CPU tick (sysclk, /8) and from GPU events (dots via the C1
accumulator, hblank at the GPU's display-range X2 as ares does), with the 2-cycle write hold and 1/2-cycle zero dwell.
Strongest alternative: event-scheduled target hits. Would change if: ps1-tests `timers` on hardware show hblank edges at a
different point in the line than X2; then the edge moves, not the model.

Phase 3 built the counters as a lazy closed form with one derived deadline instead of per-step ticking, so the "step timers from the CPU tick" recommendation above is superseded; the "strongest alternative" is what was built.

## 6. Interrupts

Moved to the public pages: the I_STAT and I_MASK registers and their bits in [docs/hardware/bus.md](../../docs/hardware/bus.md) (Interrupt registers), the CAUSE bit 10 line, the interrupt entry and its latency in [docs/hardware/cop0.md](../../docs/hardware/cop0.md) (Interrupts), the DMA interrupt edge in [docs/hardware/dma.md](../../docs/hardware/dma.md) and the counters' interrupt bits in [docs/hardware/timers.md](../../docs/hardware/timers.md). I_STAT is set on a 0-to-1 edge of the source only, and the DMA bit is raised when DICR bit 31 goes from 0 to 1, as the pages say; no correction to this section's table was needed beyond the DMA one above.

## 7. BIOS, kernel and boot

| ROM version | Models | ROM CRC32 (psx-spx) | libretro name, MD5 / SHA-1 (System.dat) | Sources |
|---|---|---|---|---|
| 1.0j | SCPH-1000, DTL-H1000 | 3B601FC8 | scph1000.bin 239665b1a3dade1b5a52c06338011044 / 343883a7b555646da8cee54aadd2795b6e7dd070 | [SYS.07][SYS.23] |
| 1.1j | SCPH-3000, DTL-H1000H | 3539DEF6 | scph3000.bin 849515939161e62f6b866f6853006780 / b06f4a861f74270be819aa2a07db8d0563a7cc4e | [SYS.07][SYS.23] |
| 2.0a / 2.0e; 2.1j / 2.1a / 2.1e | DTL-H1001 / SCPH-1002; SCPH-3500 / SCPH-1001 / SCPH-1002 | 55847D8C / 9BB87C4B; BC190209 / AFF00F2F / 86C30531 | scph1002.bin (2.0e) 54847e693405ffeb0359c6287434cbef / 20b98f3d80f11cbf5a7bfd0779b0e63760ecc62c; scph3500.bin cba733ceeff5aef5c32254f1d617fa62 / e38466a4ba8005fba7e9e3c7b9efeba7205bee3f | [SYS.07][SYS.23] |
| 2.2j / 2.2a / 2.2e | SCPH-5000, DTL-H1200 / SCPH-1001, DTL-H1201, DTL-H3001 / SCPH-1002, DTL-H1202, DTL-H3002 | 24FC7E17 / 37157331 / 1E26792F | scph5000.bin eb201d2d98251a598af467d4347bb62f (libretro's dump, CRC 8C93A399, differs from psx-spx); scph1001.bin 924e392ed05558ffdb115408c263dccf / 10155d8d6e6e832d6ea66db9bc098321fb5e8ebf | [SYS.07][SYS.23] |
| 2.2v / 2.2d; 4.0j | SCPH-5903 (VCD, 1 MB) / DTL-H1100; SCPH-7000, 9000 | 446EC5B2 / DECB22F5; EC541CD0 | not in System.dat | [SYS.07] |
| 3.0j / 3.0a / 3.0e | SCPH-5500 / SCPH-5501, 7003 / SCPH-5502, 5552 | FF3EEB8C / 8D8CB7E4 / D786F0B9 | scph5500.bin 8dd7d5296a650fac7319bce665a6a53c / b05def971d8ec59f346f2d9ac21fb742e3eb6917; scph5501.bin 490f666e1afb15b7362b406ed1cea246 / 0555c6fae8906f3f09baf5988f00e55f88e9f30b; scph5502.bin 32736f17079d0b2b7024407c39bd3050 / f6bc2d1f5eb6593de7d089c425ac681d6fffd3f0 | [SYS.07][SYS.23][SYS.24] |
| 4.1w / 4.1a / 4.1e | SCPH-7000W / SCPH-7001, 7501, 7503, 9001 / SCPH-7002, 7502, 9002 | B7C43DAD / 502224B6 / 318178BF | scph7001.bin 1e68c231d0896b7eadcad1d7d8e76129 / 14df4f6c1e367ce097c11deae21566b4fe5647a9; scph7002.bin b9d9a0286c33dc6b7237bb13cd46fdee / 8d5de56a79954f29e9006929ba3fed9b6a418c1d | [SYS.07][SYS.23] |
| 4.3j; 4.4a / 4.4e | SCPH-100; SCPH-101 / SCPH-102 (PS one) | F2AF798B; 6A0E22A0 / 0BAD7EA9 | scph100.bin 8abc1b549a4a80954addc48ef02c4521 / 339a48f4fcf63e10b5b867b8c93cfd40945faf6c; scph102A.bin b10f5e0e3d9eb60e5159690680b1e774 / beb0ac693c0dc26daf5665b3314db81480fa5c7c | [SYS.07][SYS.23] |
| 4.5a / 4.5e | SCPH-101 / SCPH-102 | 171BDCEC / 76B880E5 | scph101.bin 6e3735ff4c7dc899ee98981385f6f3d0 / dcffe16bd90a723499ad46c641424981338d8378; scph102B.bin de93caec13d1a141a40a79f5c86168d6 / dbc7339e5d85827c095764fc077b41f78fd2ecae | [SYS.07][SYS.23] |
| Net Yaroze DTL-H3000/3001/3002; PSP/PS3 images | 2.2 family (a/e rows per psx-spx; the J unit is 2.2j, unverified); region-free CD firmware ("for NETNA") accepts SCEE/SCEA/SCEI plus SCEW discs | | psxonpsp660.bin c53ca5908936d412331790f4426c6c33; ps1_rom.bin 81bbe60ba7a3d1cea1d48c14cbcc647b | [SYS.07][SYS.09][SYS.23][SYS.24] |

Digests only recognise a user's own dump; no BIOS byte enters the repository. The 04-Dec-1995 "CEX-3000/1001/1002" kernel
is in v2.2 to v4.5 except v4.0 ("CEX-7000", differing only in the 1 MB ROM setting at BFC00000h); v2.1 is the older
"CEX-3000 KT-3" (RAM_SIZE handling differs at BFC00014h..1Fh) [SYS.07].

- **ROM layout and boot.** Kernel part 1 at BFC00000h runs from uncached ROM; part 2 at BFC10000h is relocated to RAM;
  intro/bootmenu at BFC18000h; charsets at BFC64000h; BCD date BFC00100h, console type BFC00104h, version strings
  BFC00108h, GUI version string BFC7FF32h (not in v1.0) [SYS.07]. First stores: DEV2 = 0013243Fh, then RAM_SIZE = 0B88h
  [SYS.03]; the kernel copies its exception stub to 80000080h (and 80000000h), the A0/B0/C0 stubs to A00000A0h..CFh, kernel
  memory at A000E000h size 2000h, kernel variables at 7460h..891Fh, RAM size in MB at 60h, table of tables at 100h,
  SYSTEM.CNF command line at 180h; user RAM starts at 10000h; default stack top 1FFF00h; TTY is off by default via
  KernelRedirect. The intro shows "S" then "PS" (SCPH-1000 only "S"); the bootmenu appears with no disc [SYS.07].
- **Licence and region.** The CD controller reads the "SCEx" wobble string from the lead-in (SCEI Japan, SCEA America, SCEE
  Europe); GetID returns it and the BIOS refuses to boot on mismatch; unlicensed discs fail ReadN with error 40h unless the
  Unlock commands ("Licensed by", "Sony", "Computer", "Entertainment", region) are sent (vC1+, no-op on Japanese units); a
  modchip makes GetID report SCEx for any disc [SYS.09]. The PlayStation logo data is at sectors 5..11 (3278h bytes) of
  every disc [SYS.08]. The "Licensed  by" / "Sony Computer Entertainment Inc." text in the system area (sector 4) is
  documented on psx-spx's CDROM Format page (fact-checked 2026-10-19; see PER.04 and SCM.03); a TOC/data-track
  check is commonly described but was not found in the fetched pages (unverified).
- **Kernel tables.** A table to A(B4h) (malloc 33h, InitHeap 39h, printf 3Fh, GPU 46h-4Eh, SetMem 9Fh, _boot A0h), B
  table to about B(5Dh) (OpenEvent 08h, OpenTh 0Eh, InitPAD2 12h, ReturnFromException 17h, HookEntryInt 19h, InitCARD2
  4Ah), C table to C(1Dh) (EnqueueTimerAndVblankIrqs 00h, SysEnqIntRP 02h, ExceptionHandler 06h, InstallExceptionHandlers
  07h, SysInitMemory 08h, InstallDevices 12h); calls go through 00A0h/00B0h/00C0h with the number in r9, SYS calls through
  SYSCALL with r4; args in r4-r7 then stack from SP+10h, result in r2, r16-r23, r29, r30 preserved [SYS.07].
- **Interrupt chains the kernel leaves.** Priority 0: CdromDmaIrq, CdromIoIrq, SyscallException; 1: CardSpecificIrq,
  VblankIrq, Timer2/1/0Irq; 2: PadCardIrq; 3: DefInt (never removable). The VBLANK handler services pads and cards; the
  hardware timers are unused by the BIOS; TCB default 4 (one for the boot EXE), EvCB 16 (5 used by CD), exception stack
  1000h [SYS.07]. I_MASK and SR at Exec are not stated by psx-spx (unverified; traced in the boot phase).
- **PS-EXE header (800h).** 000h "PS-X EXE"; 010h initial PC; 014h GP; 018h destination; 01Ch size (N x 800h, excluding
  the header); 020h/024h data; 028h/02Ch bss; 030h SP/FP base (usually 801FFFF0h, 0 = keep caller's); 034h SP offset;
  038h-04Bh reserved (the BIOS stores the caller's RA, SP, R30, R28, R16 there); 04Ch marker "Sony Computer Entertainment
  Inc. for ... area" (never verified by the BIOS); code at 800h loaded flat to 018h. Exec zero-fills bss, sets SP/FP, GP and
  jumps to PC with r4 = 1, r5 = 0; the header's stack is usually overridden by SYSTEM.CNF STACK (801FFF00h in examples)
  [SYS.08][SYS.07]. SYSTEM.CNF keys: BOOT = cdrom:\path;1 (plus optional argument), TCB, EVENT, STACK; PSX.EXE is the
  fallback filename without SYSTEM.CNF [SYS.08]. ps1-tests EXEs ship with SP = 0, so the loader must supply a stack [SYS.25].
- **Side-loading and fast boot.** ares hooks the instruction at 80030000h (shell entry after kernel init): with an EXE it
  copies the body to the 018h address, sets PC and GP from the header (SP untouched) and continues; with Fast Boot and a disc
  it returns to r31, skipping the shell [SYS.15]. DuckStation documents fast boot as skipping the BIOS splash and requires a
  BIOS for any boot [SYS.27]. The warm-boot A(A0h) skips the intros but needs the same disc [SYS.07].

**C7 BIOS-free CI boot.** Recommendation: build OpenBIOS from the pcsx-redux `nugget` repository (MIT at `9db6d88d23c4`;
pcsx-redux's LICENSES.md points OpenBIOS at that licence [SYS.26]) as a pinned CI artefact, load it as the BIOS, and
side-load test EXEs with the ares hook (PC reaches 80030000h, copy body, PC/GP from the header, SP = 801FFFF0h when the
header says 0). OpenBIOS gives real A0/B0/C0 tables, exception chains, events and printf, so PSn00bSDK and psyq-built
tests behave; libretro's SwanStation already defaults to it [SYS.24]. The core also ships a 300-line own stub kernel (MIT)
for CPU-only vectors: it writes the memory-control boot values of section 3, sets BEV = 0, installs a handler at 80000080h
that acknowledges I_STAT and returns, and jumps to the EXE; the stub is the oracle when an OpenBIOS behaviour is in
question. Strongest alternative: HLE of the A0/B0/C0 tables in C (what PCSX-era emulators did); rejected because it
duplicates a maintained MIT kernel and hides kernel-side timing. Would change if: nugget's licence changes at a later pin,
or a test is found to depend on a retail-kernel quirk OpenBIOS lacks; then that test runs only in the owner's local
BIOS-present job, never with BIOS bytes in CI.

## 8. Regions and console revisions

| Board | Models (psx-spx does not map SCPH numbers to boards; version rows above do) | CPU | GPU | SPU | CD sub-CPU firmware | Notes | Sources |
|---|---|---|---|---|---|---|---|
| PU-7 | SCPH-1000 class | CXD8530BQ/CQ | CXD8514Q 160-pin, dual-ported VRAM, external DAC, colour clock chosen by frame rate | CXD2922Q/BQ | vC0 (19 Sep / 18 Nov 1994), MC68HC05L16 | AV multiout, S-Video, composite; RAM_SIZE boot 0B80h | [SYS.11][SYS.09][SYS.03] |
| early / late PU-8 | | CXD8530 | CXD8514Q / CXD8561Q 208-pin DRAM | CXD2922 / CXD2925Q | vC0-vC1 / vC1 (16 May, 24 Jul 1995) | four 8-bit RAM chips; the BIOS sets DRAM_CTRL bit 3 from late PU-8 on | [SYS.11][SYS.09] |
| PU-18 | | CXD8606Q | CXD8561BQ | CXD2925Q | vC2 (12 Sep 1996 J, 10 Jan 1997 U/E) | single 32-bit RAM, multiout only | [SYS.11][SYS.09] |
| PU-20 / PU-22 / PU-23 | | CXD8606AQ/BQ | CXD8561BQ | CXD2925Q | vC2 (14 Aug 1997) / vC3 (10 Jun 1998) / vC3 (1 Feb 1999) | PU-22 moves to the X201 PLL clock; PU-23 has a serial port and no parallel port | [SYS.11][SYS.09] |
| PM-41 | SCPH-100/101/102 | CXD8606CQ | CXD8561CQ | CXD2938Q (SPU + CD) | vC3 (1 Feb 1999) | 2 MB VRAM chip, 1 MB addressable; no serial port | [SYS.11][SYS.09] |
| PM-41(2) | PS one late | CXD8606CQ | CXD9500Q (VRAM on chip) | CXD2941R (SPU + CD + SPU RAM) | vC3 (6 Jun 2001) | | [SYS.11][SYS.09] |

- GPU v0 (CXD8514Q) versus v2 (CXD8561): v2 DRAM is faster; during vblank it accepts DMA blocks above 10h words that lose
  data on v0; v0 selects the colour clock from the frame rate, so a 50/60 Hz switch gives the wrong colour on old boards
  [SYS.10][SYS.11]. Firmware feature steps: vC1+ adds ReadTOC, GetQ, unlock/lock; vC2+ adds test sub-function 51h [SYS.09].
- Regions: the GPU runs at the region's video clock whatever mode a game selects, so a PAL game on an NTSC console runs at
  50.460/50.219 Hz and an NTSC game on a PAL console at 59.393/59.280 Hz [SYS.10]; the CD firmware's region answer ("for
  Japan", "for U/C", "for Europe", "for NETNA" Yaroze, "for US/AEP" debug) decides which SCEx discs boot [SYS.09]. PAL
  profiles need 314-line frames and the PAL dot clocks; 768 cycles per sample is unchanged; the 4.1/4.5 GUIs differ between
  A and E only in the last byte of the version string [SYS.07].

**C8 Profile fields.** Recommendation: `region` {ntscj, ntscu, pal}; `gpu_clock_hz` (53,693,175 or 53,203,425);
`board` {pu7, pu8_early, pu8_late, pu18, pu20, pu22, pu23, pm41, pm41_2}; derived defaults from `board`: `cpu` {cxd8530,
cxd8606}, `gpu` {v0, v2, v2c, cxd9500}, `spu` {cxd2922, cxd2925, cxd2938, cxd2941}, `cd_firmware` {c0, c1, c2, c3},
`ram_ctrl_boot` (0B80h or 0B88h), `has_parallel_port`, `has_serial_port`, `cd_region` {japan, uc, europe, netna, debug};
plus `bios` (user bytes or the OpenBIOS artefact) and the section 9 power-on values. Strongest alternative: region only.
Would change if: no behaviour beyond region and GPU version is ever observed to differ; then `board` collapses to
`gpu` + `cd_firmware`.

## 9. Power-on state

| State | Hardware | Core default | Sources |
|---|---|---|---|
| CPU, I-cache | SR: SWc, KUc, IEc = 0, BEV = 1, rest undefined; PC = BFC00000h; GPRs, HI/LO, cache undefined | SR = 00400000h (BEV), CAUSE 0, PRID 2, DCIC 0, BPCM = BDAM = FFFFFFFFh, GPRs/HI/LO 0, PC BFC00000h; cache tags invalid, words seeded | [SYS.13][SYS.21][SYS.15] |
| RAM 2 MB, scratchpad 1 KB | undefined; kernel leaves residue | ZERO; `ram_fill` SEEDED(seed) option (ares seeds random, fixed seed in homebrew mode) | [SYS.14][SYS.15] |
| Memory control | all ports at maximum delays; the ROM writes the section 3 values in its first instructions | reset: delay fields at maximum; EXE side-load without BIOS applies the section 3 boot values | [SYS.03] |
| RAM_SIZE, cache control | reset values not documented; BIOS writes 0B88h/0B80h and (unverified) 1E988h | RAM_SIZE 0 (MAME: 800h) then the boot value; scratchpad and I-cache enabled, IBLKSZ 1 | [SYS.03][SYS.21][SYS.19] |
| DMA | DPCR 07654321h, DICR 0, CHCR 0, MADR/BCR 0 | same | [SYS.04][SYS.18][SYS.22] |
| Timers | COUNT, MODE, TARGET 0; IRQ line bit 10 = 1 | same | [SYS.16] |
| I_STAT, I_MASK | 0, 0 | same | [SYS.22][SYS.17] |
| GTE; GPU, SPU, CD-ROM, SIO, MDEC | undefined; see their files | GTE registers 0; the rest per their files | [SYS.15] |
| Beam | CPU/GPU phase undefined (asynchronous clocks) | line 0, GPU clock 0, accumulator 0; phase is a save-state field | derived |

**C9 Reset kinds.** Recommendation: `power` applies the table and `ram_fill`; `reset` (the console has only a power switch)
re-applies the table but keeps RAM, scratchpad, VRAM and SPU RAM. Strongest alternative: one hard reset. Would change if: a
TAS consumer wants a hardware-faithful power-cycle residue model.

## 10. Accuracy claims and the tests that settle them

| Subsystem | Claim | Confidence | Settling test |
|---|---|---|---|
| Clock ratio and line length | GPU/CPU = 715909/451584 NTSC, 709379/451584 PAL; 3413/3406 clocks per line | nominal clocks documented; ratio derived; line length is implementation consensus while measured rates imply 3412.5/3405.02 | timer1 (hblank) against timer2 (sysclk) over 10,000 lines on hardware; a half-clock residue shows as a 1-line drift per 2000 lines |
| CPU delays, exceptions | load/branch delay, mul/div 6/9/13/36; vectors, priority, BD/BT, GTE double-execute | documented (nocash, by test programs); the GTE bug reproduced by ares | ps1-tests cpu, a mul/div loop against timer2, an interrupt storm over a GTE loop |
| I-cache | 256 x 4 words, per-word valid, fill-to-end | documented (measured by nocash) | ps1-tests cpu/code-in-io and a self-modifying-code probe |
| Memory control timing | formula with COM fields; 29-cycle BIOS word | documented as approximate | ps1-tests cpu/access-time |
| RAM mirrors and faults | 8 MB bank mirrors, bus error above | documented; relied on by games | read at 00200000h and 00800000h under 0B88h |
| DMA, timers | 1/4/24-40 clocks per word and the stall rule; 2-cycle holds, toggle semantics, sync modes | documented, DMA costs partly inferred; ares reproduces the timers | ps1-tests dma/chopping, otc-test, gpu/bandwidth; timers against a `timer-dump` hardware log |
| Interrupts | edge I_STAT, non-latched CAUSE bit 10, 2-instruction delay | documented; the delay is ares' inference | an I_STAT write followed by an immediate `mfc0` CAUSE read |
| BIOS digests; kernel state at Exec | section 7 table; I_MASK, SR, DPCR at Exec | documented (libretro, psx-spx CRCs; scph5000 disagrees); Exec state unverified | hash the owner's dumps; trace a retail BIOS boot to 80030000h in the owner's BIOS-present job |
| Licence check | SCEx wobble via GetID; logo sectors 5..11 | documented; the sector-4 licence string is documented (psx-spx CDROM Format), whether the BIOS checks it beyond the logo is unverified | boot an unlicensed image under a retail BIOS with a logged CD command stream |

## Sources

| ID | Tier | Source | Pin or access date | Supports |
|---|---|---|---|---|
| SYS.01 | T2 | psx-spx, CPU Specifications, https://psx-spx.consoledev.net/cpuspecifications/ (docs/cpuspecifications.md) | `6d7d1bc106a7` (2026-10-08) | delays, mul/div, COP0, exceptions, vectors, garbage registers, DCIC |
| SYS.02 | T2 | psx-spx, Memory Map, https://psx-spx.consoledev.net/memorymap/ | `6d7d1bc106a7` | map, mirrors, I-cache, scratchpad, write queue, bus errors |
| SYS.03 | T2 | psx-spx, Memory Control, https://psx-spx.consoledev.net/memorycontrol/ | `6d7d1bc106a7` | delay registers, boot values, RAM_SIZE, cache control, boot instructions |
| SYS.04 | T2 | psx-spx, DMA Channels, https://psx-spx.consoledev.net/dmachannels/ | `6d7d1bc106a7` | channels, DPCR, DICR, CHCR, sync modes, timings, stalls |
| SYS.05 | T2 | psx-spx, Timers, https://psx-spx.consoledev.net/timers/ | `6d7d1bc106a7` | registers, modes, holds, IRQ modes |
| SYS.06 | T2 | psx-spx, Interrupts, https://psx-spx.consoledev.net/interrupts/ | `6d7d1bc106a7` | I_STAT/I_MASK, edge rule, CAUSE bit 10 |
| SYS.07 | T2 | psx-spx, Kernel (BIOS), https://psx-spx.consoledev.net/kernelbios/ | `6d7d1bc106a7` | versions, CRCs, ROM/RAM layout, tables, chains, _boot, Exec |
| SYS.08 | T2 | psx-spx, CDROM File Formats, https://psx-spx.consoledev.net/cdromfileformats/ | `6d7d1bc106a7` | PS-EXE header, SYSTEM.CNF, logo sectors 5..11 |
| SYS.09 | T2 | psx-spx, CDROM Drive, https://psx-spx.consoledev.net/cdromdrive/ | `6d7d1bc106a7` | SCEx, unlock, 75 sectors/s, firmware versions and region strings |
| SYS.10 | T2 | psx-spx, GPU (GPU Timings), https://psx-spx.consoledev.net/graphicsprocessingunitgpu/ | `6d7d1bc106a7` | video clocks, 11/7 note, lines, rates, dot clocks, v0/v2 notes |
| SYS.11 | T2 | psx-spx, Pinouts, https://psx-spx.consoledev.net/pinouts/ | `6d7d1bc106a7` | boards, chips, oscillators, 768 x 44100 derivation |
| SYS.12 | T2 | psx-spx, SPU, https://psx-spx.consoledev.net/soundprocessingunitspu/ | `6d7d1bc106a7` | 44100 Hz, 768 cycles per sample, 512 KB |
| SYS.13 | T1 | IDT 79R3051 Family Hardware User's Manual (1991), https://bitsavers.mirrorservice.org/components/idt/risc/1991_IDT79R3051_Hardware_Users_Manual.pdf | 1991 edition, fetched 2026-10-19 | 5-stage pipeline, load and branch delay, mul/div interlock, segments, user-mode address error, reset state, write buffer |
| SYS.14 | T3 | ares `ares/ps1/system/system.cpp`, `system.hpp` (ISC), https://github.com/ares-emulator/ares | `9408cb43d494` (2026-10-06) | 33,868,500 Hz, 53,693,175 for all regions, homebrew seed, fast boot option |
| SYS.15 | T3 | ares `ares/ps1/cpu/cpu.cpp`, `memory.cpp`, `icache.cpp`, `exceptions.cpp` (ISC) | `9408cb43d494` | power state, segment decode, bus errors, GTE-in-interrupt, side-load hook at 80030000h |
| SYS.16 | T3 | ares `ares/ps1/timer/timer.cpp`, `io.cpp` (ISC) | `9408cb43d494` | 2-cycle holds, sync modes, toggle, reset values |
| SYS.17 | T3 | ares `ares/ps1/interrupt/interrupt.cpp`, `io.cpp` (ISC) | `9408cb43d494` | poll, 2-instruction delay, ack semantics |
| SYS.18 | T3 | ares `ares/ps1/dma/dma.cpp`, `channel.cpp`, `io.cpp` (ISC) | `9408cb43d494` | OTC words, chain guard, reset priorities, block model |
| SYS.19 | T3 | ares `ares/ps1/memory/io.cpp`, `bus.hpp` (ISC) | `9408cb43d494` | memory-control bit layout, cache-control bits, I/O decode, access costs |
| SYS.20 | T3 | ares `ares/ps1/gpu/gpu.hpp`, `gpu.cpp` (ISC) | `9408cb43d494` | 3413/3406, 263/314, hblank at X2, vblank at Y1/Y2, dot divider |
| SYS.21 | T3 | MAME `src/devices/cpu/psx/psx.cpp` (header `license:BSD-3-Clause`), https://github.com/mamedev/mame | `96df6a708bdd` (2026-10-19) | reset values, BIU bits, scratchpad bus-error mapping |
| SYS.22 | T3 | MAME `src/devices/cpu/psx/dma.cpp`, `irq.cpp`, `rcnt.cpp` (BSD-3-Clause) | `96df6a708bdd` | DMA/IRQ/timer reset values, DICR bit 31 logic |
| SYS.23 | T3 | libretro-database `dat/System.dat`, https://github.com/libretro/libretro-database | `12c21cf79f1e` (2026-10-10) | BIOS file names, CRC32, MD5, SHA-1 |
| SYS.24 | T4 | libretro docs, Beetle PSX core page, https://docs.libretro.com/library/beetle_psx/ | accessed 2026-10-19 | scph5500/5501/5502 MD5, PSP/PS3 images, OpenBIOS default |
| SYS.25 | T3 | JaCzekanski/ps1-tests README (MIT), https://github.com/JaCzekanski/ps1-tests | `f727802fead1` (2023-03-05) | test list, SP = 0 note |
| SYS.26 | T1 (project's own) | pcsx-redux `LICENSES.md` `91285bf736af`; pcsx-redux/nugget `LICENSE` (MIT), https://github.com/pcsx-redux/nugget | `9db6d88d23c4` | OpenBIOS licence |
| SYS.27 | T4 | DuckStation README (docs only), https://github.com/stenzek/duckstation | `124a4b16feda` | BIOS required, fast boot feature |

Licence checks at pin: ares `LICENSE` is ISC [SYS.14]; MAME `src/devices/cpu/psx/{psx,dma,irq,rcnt}.cpp` and
`src/devices/video/psx.cpp` carry `license:BSD-3-Clause` [SYS.21], while `src/mame/sony/psx.cpp` is `license:GPL-2.0+` and was
not used; nugget (OpenBIOS) is MIT [SYS.26]; simias/psx-guide has no LICENSE file and simias/rustation's LICENSE is GPL text,
so neither was opened; ps1-tests is MIT [SYS.25]. Not fetched: the psxdev.net wiki (no stable URL), the DTL-H3000 digest,
the kernel's I_MASK/SR at Exec (the "Licensed by Sony" system-area text is on the CDROM Format page, read for PER/SCM).
