# Project Research Summary

**Project:** schwaystation
**Domain:** Sony PlayStation (PS1) emulator core in C (embeddable library, headless runner, libretro core)
**Researched:** 2026-10-19
**Confidence:** HIGH for structure, stack, delivery and the CPU and GTE plan; MEDIUM for several timing constants (GPU clocks per line, CD seek and INT1 period, per-primitive GPU cost, MDEC block cost, ADSR edges) that are inferred from emulators rather than measured (named, cited and testable; see Gaps)

## Executive Summary

schwaystation is the owner's seventh emulator and inherits a proven shape: a deterministic, integer-only C17 core
with a headless runner and a libretro adapter, shipped by an automated pipeline from the first phase. The PlayStation
is five to ten times the size of the 8-bit siblings (an R3000A with load delays and an I-cache, the GTE, a GPU
rasteriser, an SPU with reverb and ADPCM, a CD-ROM controller with a seek model, the MDEC, SIO, DMA and timers), so
one core with console revisions and regions as machine profiles covers the family. Four facts make this console
cheaper than its size suggests: an MIT per-instruction R3000 vector corpus (SingleStepTests, 55 files), 1,150
hardware-captured GTE cases and VRAM-image references under MIT (ps1-tests), a maintained MIT kernel (OpenBIOS in
nugget) that stands in for the retail BIOS in CI, and permissive references that may be read (ares, ISC; MAME's
BSD-3 device files). The libretro Redump DAT lists 13,407 discs; the slot is a permissive, embeddable core with
published hashes and a ledger, which no PS1 emulator offers today.

The sibling evidence is equally clear about what to avoid: growing the CPU from game traces, self-invented
verification ceremony, real-host test harnesses that outgrow the emulator, release qualification deferred to
milestone end, and fixture provenance on the critical path. Every rule that came from those failures is in
`AGENTS.md`, and every lesson is a ledger row in `.planning/LESSONS.md`.

## Key Findings

### Recommended Stack
See `STACK.md`; details, pins and the rule → check table in `.planning/preparation/ENGINEERING.md`.

### Expected Features
v1.0: the delivery path with a test card in RetroArch; the R3000A and GTE against the public vectors with COP0,
bus, DMA, timers and interrupts; PlayStation games playing in RetroArch with the GPU, DualShock, memory cards,
OpenBIOS, self-authored fixtures and a compatibility ledger; the SPU, CD-ROM with cue/bin, iso and m3u, XA, CD-DA
and MDEC. Later: CHD, PBP, ECM, LibCrypt, multitap, guns, PocketStation storage, per-game cards, PAL and every
revision over the ledger (v2.0); states, rewind, run-ahead, movies, Playstead continuation (v3.0);
hardware-measured accuracy (v4.0); hooks, a GDB stub, Python and Gymnasium (v5.0); the cached interpreter, JIT,
handheld and console lanes and the buildbot (v6.0); enhancements outside the hashed path (SEED-006); the browser
page (SEED-008).

### Architecture Approach
One `uint64_t` tick per CPU cycle at 33,868,800 Hz; the GPU clock comes from an exact ratio accumulator; the CPU
drives the bus and devices catch up lazily on one deadline; a frame runs from vsync to vsync with input latched
there; the GPU renders immediately into VRAM on a line-granular state machine with a busy budget; video is the
native display frame in GPU encoding plus a metadata tuple and a read-only VRAM view; audio is int16 stereo at
44,100 Hz with an exact rational sample count per frame; the CD-ROM is a command FSM in cycles over host-supplied
sector callbacks; state is a four-pass visitor over fixed-size integer structs; observability is always-on counters
plus a first-fault record. Details in `.planning/preparation/ARCHITECTURE.md`.

**Major components:**
1. **Core library** (`schwaystation`, prefix `schway_`): instance and test card, cpu, cop0, gte, bus, dma, timers, irq, gpu, spu, cdrom and cdxa, mdec, sio and pad, card, exe and bios (stub kernel), state; plus the MIT `host/disc_image` helper outside the determinism boundary.
2. **Runner** (`schwaystation-run`): frames, frame and audio hashes, PNG, probe, mailbox and TTY, first-fault trace, input scripts, save dir, bench; the automation and Playstead surface.
3. **libretro core**: the adapter over the library with the Beetle RetroPad map, BIOS names, option keys, XRGB8888, batch audio, disk control and memory descriptors.

### Critical Pitfalls
1. Growing the CPU opcode by opcode from game traces instead of the vectors (three siblings repeated it; the R3000A and GTE finish against vectors first).
2. Real-host e2e and verification machinery that outgrows the emulator (10k test lines, no `src/` change).
3. Deferring releases to a milestone-end qualification phase (eight phases merged unpublished).
4. Fixture and toolchain provenance on the critical path; hence armips and mkpsxiso pinned by digest, own fixtures, OpenBIOS built in CI, no Sony bytes anywhere.

## Implications for Roadmap

Milestone heading for ROADMAP.md: `## v1.0 PlayStation games play with sound in RetroArch`. Six phases. Merging each one publishes a release.

### Phase 1: A test frame in RetroArch
**Rationale:** Ship the whole delivery path before any emulation exists, so every later phase ends in a release by merging; the test card proves the frame contract on eight lanes with no BIOS and no disc.
**Delivers:** The library with its public header, test card and state visitor, `schwaystation-run` with hashes and bench, the libretro core, six-platform CI plus `be32` and `wasm32` lanes, hygiene and policy gates with negative controls, release-please with the App token, the rehearsal, 18 attested archives, the README install line, the hosted RetroArch screenshot compare, installed and embedded consumers, the Ir/frame measurement.
**Addresses:** FRAME-01 to FRAME-09
**UI hint**: no
**Canonical refs:** `.planning/preparation/ENGINEERING.md`, `.planning/preparation/HOSTS-AND-INTEGRATION.md`, `.planning/preparation/ARCHITECTURE.md`, `.planning/preparation/CONFORMANCE.md`, `PROVENANCE.md`

### Phase 2: The CPU matches the public vectors
**Rationale:** Public vectors judge the R3000A on its own before any other chip exists; COP0 exceptions, bus decode, memory control, the I-cache, interrupt registers and PS-EXE loading under the stub kernel come with it, and the first-fault trace makes every later stall diagnosable.
**Delivers:** An own interpreter (load and branch delays, mul/div stamps, I-cache, write queue, exception path) matching the committed SingleStepTests sample with the full set nightly; `exc_matrix` under the own stub kernel; armips built on every leg assembling every fixture; bus decode, memory-control costs, I-cache, scratchpad and I_STAT/I_MASK; `--probe`, `--tty`, `--trace first-fault`; the loader fuzz target.
**Addresses:** CPU-01 to CPU-05
**UI hint**: no
**Canonical refs:** `.planning/preparation/PSX-SYSTEM.md`, `.planning/preparation/ARCHITECTURE.md`, `.planning/preparation/CONFORMANCE.md`, `.planning/preparation/PSXDEV-EXTRACTS.md`

### Phase 3: The GTE matches the hardware cases; DMA and timers
**Rationale:** The GTE has its own hardware-captured oracle and DMA and timers are register-level devices that every later chip depends on; landing them as one phase keeps phase 2 small and gives the GPU, SPU and CD-ROM a proven transfer and timing base.
**Delivers:** The GTE against the transcoded 1,150-case table with the UNR table, flags, cycle counts and a property test; the `gte_vectors` fixture; the seven DMA channels with DPCR/DICR, costs, chopping, the linked-list guard, MADR wrap and OTC; the three root counters with sync modes, targets and IRQ modes; the `timers_lines` sysclk, sysclk/8 and vblank-count records.
**Addresses:** GTE-01 to GTE-03
**Uses:** the stub kernel and armips fixtures from Phase 2
**UI hint**: no
**Canonical refs:** `.planning/preparation/PSX-GPU-AND-GTE.md`, `.planning/preparation/PSX-SYSTEM.md`, `.planning/preparation/ARCHITECTURE.md`, `.planning/preparation/CONFORMANCE.md`, `.planning/preparation/PSXDEV-EXTRACTS.md`

### Phase 4: Games play in RetroArch
**Rationale:** With the CPU, GTE, DMA and timers proven, the GPU, pads and memory cards turn fixtures into playable games; OpenBIOS boots to its shell in CI and the ledger starts ratcheting, with a local preset for the user's own dumps.
**Delivers:** The rasteriser against ps1-tests VRAM references; display modes with the metadata tuple; DMA2 lists and the busy budget; timers on dot and hblank; cycle-scheduled SIO0 with the digital pad and DualShock and rumble; memory-card spans with atomic runner writes and fresh-process continuation; the `gpu_motion`, `pad_counter` and `memcard_roundtrip` fixtures and the `timers_lines` dot and hblank records; OpenBIOS built in CI; the ledger and generated compatibility page; the local `library` preset; one integrated RetroArch session.
**Addresses:** GAME-01 to GAME-07
**Uses:** the first-fault trace, the stub kernel and the fixtures from Phases 2 and 3
**UI hint**: no
**Canonical refs:** `.planning/preparation/PSX-GPU-AND-GTE.md`, `.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md`, `.planning/preparation/CONFORMANCE.md`, `.planning/preparation/ARCHITECTURE.md`, `.planning/preparation/HOSTS-AND-INTEGRATION.md`

### Phase 5: The sound matches the console
**Rationale:** The SPU's oracles (the ps1-tests SPU logs and an own tone fixture) need only a running EXE, so sound lands before discs; audio equality is proven in-process, and the hosted RetroArch session proves video only.
**Delivers:** The SPU with ADSR, reverb, noise, capture buffers and transfer paths; the `spu_tone` fixture; audio hashes identical on every lane with an exact sample count per frame; libretro batch and single-sample audio equal to the runner's stream in `libretro.host`; the idle SPU under `core.no_busy_loop`.
**Addresses:** SOUND-01 to SOUND-02
**Uses:** DMA4 from Phase 3 and the GAME-07 content path from Phase 4
**UI hint**: no
**Canonical refs:** `.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md`, `.planning/preparation/ARCHITECTURE.md`, `.planning/preparation/CONFORMANCE.md`, `.planning/preparation/HOSTS-AND-INTEGRATION.md`

### Phase 6: Discs and FMV
**Rationale:** Discs last, because their oracles (the ps1-tests CD-ROM and MDEC logs, the own disc booting through OpenBIOS) need a playing game with sound, and because the disc path is the one host-side parser of v1.0.
**Delivers:** The CD-ROM FSM with documented timings, GetID and the lid; cue/bin, iso and m3u parsing with fuzz targets; XA and CD-DA through the ATV matrix; the MDEC against the ps1-tests references; the `disc_boot` fixture built by mkpsxiso and booted through OpenBIOS; the rcheevos disc MD5 check; disk control with m3u swap; a hosted RetroArch session booting `disc_boot`; the SDL2 example host.
**Addresses:** DISC-01 to DISC-04
**UI hint**: no
**Canonical refs:** `.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md`, `.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md`, `.planning/preparation/ARCHITECTURE.md`, `.planning/preparation/CONFORMANCE.md`, `.planning/preparation/HOSTS-AND-INTEGRATION.md`

### Phase Ordering Rationale
Delivery first so nothing is ever unreleased; the CPU against exhaustive vectors before any chip that depends on
it; the GTE, DMA and timers next, each with its own register-level or hardware-captured oracle; video, input and
saves together because one integrated play check proves all three; sound before discs because the SPU's evidence
needs only a running EXE; discs and FMV last because their evidence needs a playing game with sound. Each phase
opens with a tracer plan through every layer before widening, and plans that do not share a test inventory run in
parallel waves (opcode groups; bus and irq; gte, dma and timers; rasteriser, sio and card; spu voices and reverb;
cdrom and mdec).

### Research Flags
Phases likely needing deeper research during planning: Phase 4 (GPU blend width, size rejection and stepper
precision settled by the named ps1-tests images; GPU clocks per line through `timers_lines`; the adapter's
stub-kernel path for PS-EXE content in RetroArch), Phase 6 (the CD seek model and INT1 period; whether OpenBIOS
boots the own mkpsxiso disc without licence data; the PCSX-Redux AppImage under Xvfb). Phases with standard
patterns: Phase 1 (cogmaster's and muhtari's pipeline, renamed), Phase 2 (the vectors harness pattern from muhtari
and cogmaster), Phase 3 (the same pattern over the GTE table), Phase 5 (in-process audio equality, no hosted capture).

## Confidence Assessment

| Area | Confidence | Notes |
|---|---|---|
| Stack | HIGH | Sibling-proven; dated facts re-verified 2026-10-19 |
| Features | HIGH | Library counts from the libretro Redump DAT; complaints mapped from docs, issue trackers and forums |
| Architecture | HIGH | Exact cycle lattice; API conventions held three sibling milestones; every preparation conflict settled in ARCHITECTURE 13 |
| Pitfalls | HIGH | Drawn from six siblings' histories, with revisions |
| Timing constants | MEDIUM | GPU clocks per line, CD seek and INT1 period, GPU per-primitive cost, MDEC block cost, ADSR edges and XA rounding are emulator inferences; fixtures and v4.0 hardware runs settle them |

**Overall confidence:** HIGH

### Gaps to Address
- GPU clocks per line (3413/3406 vs 3412.5/3405.02), the CD seek model and INT1 period, per-primitive GPU cost, MDEC block cost, ADSR edge rates, XA resampler rounding, blend channel width, the 1023/1024 size boundary, stepper precision, hblank and vblank edges, interrupt latency, GTE flag chaining in MVMVA, memory-control timing and DMA per-word costs, reverb rounding (DECISIONS open questions): settled by the named fixtures and ps1-tests images in Phases 2 to 6, then hardware in v4.0.
- The licence-string check, the kernel state at Exec, the scph5000 digest and GPU v0 behaviour: local runs with the owner's BIOS or ledger evidence.
- Whether OpenBIOS boots the own mkpsxiso disc without licence data and whether the stub kernel runs nugget tests unmodified: first build in Phase 2 (stub) and Phase 6 (disc).
- The interlaced rate report and audio drift in RetroArch, the rcheevos path for a bare EXE, Playstead multi-file binding: Phases 4 to 6 and a Playstead decision.
- Microsecond frame budgets and device MIPS, `be32` and `wasm32` per-PR cost, the ppc32 libgcc multiply and wasmtime instruction-count stability: measured in Phase 1.
- Whether the PCSX-Redux AppImage runs under Xvfb on ubuntu-26.04 and which build id to pin: the first nightly oracle run; Dependabot `vars` and the App's `workflows: write` need: the first Dependabot PR; GSD 1.18.0's stale `gaps_found` routing: the first occurrence; which console, loader and capture path the owner has: asked at Phase 4 at the earliest.

## Sources
Tiered and pinned in each preparation file's Sources table; the index is `.planning/preparation/BRIEF.md`.

---
*Research completed: 2026-10-19*
*Ready for roadmap: yes*
