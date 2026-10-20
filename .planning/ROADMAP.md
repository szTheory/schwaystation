# Roadmap: schwaystation

## Overview

schwaystation goes from an empty repository to PlayStation games playing with sound and discs in RetroArch. The
release path ships first, with a test card on eight lanes. The R3000A, then the GTE, DMA and timers, are proven
against public and hardware-captured oracles before any game runs. After that, the GPU, pads, memory cards and
OpenBIOS make fixtures playable in RetroArch, the SPU adds sound that hashes the same everywhere, and the CD-ROM
controller and MDEC finish v1.0 with an own disc booting through OpenBIOS. Merging each phase publishes a release.

## v1.0 PlayStation games play with sound in RetroArch

**Milestone Goal:** PlayStation games play with sound in RetroArch from a small MIT-licensed C library, with frame and
audio hashes identical on six platforms plus the `be32` and `wasm32` lanes.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [ ] **Phase 1: A test frame in RetroArch** - The library, runner and libretro core ship automatically, and a built-in test card hashes identically on eight lanes
- [ ] **Phase 2: The CPU matches the public vectors** - The R3000A, COP0, bus, memory control, I-cache and interrupts pass the vectors; PS-EXEs run under the own stub kernel
- [ ] **Phase 3: The GTE matches the hardware cases; DMA and timers** - The GTE passes the 1,150 hardware cases; seven DMA channels and three root counters pass register-level tests
- [ ] **Phase 4: Games play in RetroArch** - GPU, display modes, pads with rumble, memory cards, OpenBIOS and the compatibility ledger make fixtures playable in RetroArch
- [ ] **Phase 5: The sound matches the console** - The SPU matches the hardware logs, and audio hashes are identical on every lane and in libretro
- [ ] **Phase 6: Discs and FMV** - CD-ROM with cue/bin, iso and m3u, XA, CD-DA, MDEC and disk control, and the own disc boots through OpenBIOS in RetroArch

## Phase Details

### Phase 1: A test frame in RetroArch

**Goal**: Merging a pull request builds, tests, attests and publishes the library, runner and libretro core on every platform; one install line puts the core into RetroArch on a Mac, where it shows a test card that hashes identically on eight lanes with no BIOS and no disc
**Depends on**: Nothing (first phase)
**Requirements**: FRAME-01, FRAME-02, FRAME-03, FRAME-04, FRAME-05, FRAME-06, FRAME-07, FRAME-08, FRAME-09
**Success Criteria** (what must be TRUE):
  1. `cmake --workflow --preset ci` (`ci-msvc` on Windows) builds, tests and packages the library, `schwaystation-run` and the libretro core on all six platforms, running each test in `tests/inventory/ci.txt` once and skipping none. The installed-package C consumer, the C++ header consumer and the `add_subdirectory` embed all build.
  2. `schwaystation-run --frames 60 --hash`, run with no BIOS and no disc, prints the same test-card hash on all eight lanes, and `hash-equality` passes over the eight `hashes.txt` files. The in-process `libretro.host` frame equals the runner's conversion byte for byte, and the reported AV info matches the documented rationals and geometry.
  3. The README install line, run on macOS arm64, installs the core where RetroArch 1.22.2 (Metal) finds it. RetroArch's `--max-frames=5` screenshot with no content matches `schwaystation-run --frames 5 --png` pixel for pixel, and the one-byte negative control fails.
  4. The pull request's rehearsal produces exactly 18 version-free archives, each with `LICENSE` and `THIRD-PARTY-NOTICES.md`, plus `SHA256SUMS` and verified attestations, then deletes itself. Merging publishes the same set with no manual step.
  5. Each ABI, hygiene and policy gate (`abi.*`, `hygiene.*`, `policy.*`, `ci.title`, `build.revision_matches_head`) fails its negative control with its named message. `--bench` reports the frame-time percentiles, and the Cachegrind Ir/frame measurement runs on every pull request. The core makes zero allocations after load and never reads a host clock.

**Plans**: 14/15 plans executed in 8 waves

Plans:
**Wave 1**
- [x] 01-01-PLAN.md — CMake skeleton, presets, per-directory inventories, complete header, thin core and the `--frames 60 --hash` tracer (wave 1)
- [x] 01-02-PLAN.md — Vendored libretro.h and notices, PROVENANCE pins to cogmaster v0.1.0, README, publish.sh re-sync, setup script removed (wave 1)

**Wave 2** *(blocked on Wave 1 completion)*
- [x] 01-03-PLAN.md — Core and state tests: card, tuple, audio accumulator, API contracts, zero allocations, visitor round trip; embedding guide (wave 2)
- [x] 01-04-PLAN.md — Runner hashes per region and frame, hashes.txt, PNG reference, converter known answers, usage refusals (wave 2)
- [x] 01-05-PLAN.md — ABI gates (symbols, globals, imports, host clock), header C17/C++17, tests/api/v1.c, nofp fixture (wave 2)
- [x] 01-06-PLAN.md — Hygiene gates: tree scan, gate budget, BIOS bytes, float scan, title, classify, publish tests; script trims (wave 2)
- [x] 01-07-PLAN.md — Ir/frame comparison and its gate cases, docs budgets, no-skip policy, testing guide (wave 2)
- [x] 01-08-PLAN.md — Installed-package C and C++ consumers and the add_subdirectory embed (wave 2)

**Wave 3** *(blocked on Wave 2 completion)*
- [x] 01-09-PLAN.md — Runner bench percentiles, signals, failure status, revision gate, runner doc (wave 3)
- [x] 01-10-PLAN.md — libretro adapter and the in-process libretro.host tests (wave 3)

**Wave 4** *(blocked on Wave 3 completion)*
- [x] 01-11-PLAN.md — Packaging, release.archive_contents and the README install line (wave 4)

**Wave 5** *(blocked on Wave 4 completion)*
- [x] 01-12-PLAN.md — ci.yml lanes, RetroArch e2e compare and the workflow gates (wave 5)

**Wave 6** *(blocked on Wave 5 completion)*
- [x] 01-13-PLAN.md — Rehearsal and cleanup, release-please, release and nightly workflows, maintainer guide (wave 6)

**Wave 7** *(blocked on Wave 6 completion)*
- [x] 01-14-PLAN.md — Hosted run driven green on a probe pull request; nightly dispatched; job costs measured (wave 7)

**Wave 8** *(blocked on Wave 7 completion)*
- [ ] 01-15-PLAN.md — Measured timeouts, bound todos, gate-budget re-pin handed to ship, owner-applied ruleset (wave 8)

**Cross-cutting constraints:**
- The cruft grep exits 1 (D-27)

**UI hint**: no
**Canonical refs:** `.planning/preparation/ENGINEERING.md`, `.planning/preparation/HOSTS-AND-INTEGRATION.md`, `.planning/preparation/ARCHITECTURE.md`, `.planning/preparation/CONFORMANCE.md`, `PROVENANCE.md`

### Phase 2: The CPU matches the public vectors

**Goal**: The R3000A matches the public vectors before any game runs, with COP0 exceptions, bus decode, memory control, the I-cache and interrupt registers; PS-EXEs load and run under the own stub kernel, and the probe and first-fault trace diagnose any EXE
**Depends on**: Phase 1
**Requirements**: CPU-01, CPU-02, CPU-03, CPU-04, CPU-05
**Success Criteria** (what must be TRUE):
  1. On every lane, `ctest -L vectors` passes all 5,500 cases in the committed SingleStepTests sample on registers, delay latches and every bus cycle, and a zero-case run exits 1. The nightly `vectors-full` lane passes the full 55,000 at the pin.
  2. The exception, delay-slot and MULT/DIV timing tests pass, and the `exc_matrix` fixture's `mailbox@30` records match their `expected.csv` rows.
  3. `ctest -L bus` passes on physical decode, RAM mirrors, memory-control costs, the I-cache with isolation, the write queue, the scratchpad and I_STAT/I_MASK.
  4. `schwaystation-run --exe hello.exe --tty --stop-on mailbox` prints `tty ` lines, shows the `SWMB` magic and exits 0. armips, built on every leg at its pin, assembles every fixture to one digest, and `fixtures.armips_pin.self_test` fails on a wrong digest.
  5. `schwaystation-run --probe FILE` prints the identity fields without emulating. `--trace first-fault`, run on a crafted EXE, prints one `fault ...` line and a 4,096-record tail and exits 7.

**Plans**: TBD
**UI hint**: no
**Canonical refs:** `.planning/preparation/PSX-SYSTEM.md`, `.planning/preparation/ARCHITECTURE.md`, `.planning/preparation/CONFORMANCE.md`, `.planning/preparation/PSXDEV-EXTRACTS.md`

### Phase 3: The GTE matches the hardware cases; DMA and timers

**Goal**: The GTE matches the 1,150 hardware-captured cases, and the seven DMA channels and three root counters pass register-level tests and their fixture records, giving the GPU, SPU and CD-ROM a proven transfer and timing base
**Depends on**: Phase 2
**Requirements**: GTE-01, GTE-02, GTE-03
**Success Criteria** (what must be TRUE):
  1. `ctest -L gte` passes all 1,150 transcoded ps1-tests cases, the UNR-table assertion and the property test against a straight C reference with flags. The `gte_vectors` fixture's `mailbox@30` records match the `hw` rows.
  2. `ctest -L dma` passes for all seven channels, covering costs, chopping, MADR wrap, OTC clearing and the CPU stall. The `chain-looping` list terminates by the 1000h-node guard, and the pinned DMA fixture record matches.
  3. `ctest -L timers` passes for the three root counters with sync modes, targets, the write hold and every IRQ mode. The `timers_lines` sysclk, sysclk/8 and vblank-count records match per profile.

**Plans**: TBD
**UI hint**: no
**Canonical refs:** `.planning/preparation/PSX-GPU-AND-GTE.md`, `.planning/preparation/PSX-SYSTEM.md`, `.planning/preparation/ARCHITECTURE.md`, `.planning/preparation/CONFORMANCE.md`, `.planning/preparation/PSXDEV-EXTRACTS.md`

### Phase 4: Games play in RetroArch

**Goal**: PlayStation programs play in RetroArch, with the GPU rasteriser and display modes, digital and DualShock pads with rumble, memory cards that persist across runs, OpenBIOS booting to its shell, and a compatibility ledger that only ratchets up
**Depends on**: Phase 3
**Requirements**: GAME-01, GAME-02, GAME-03, GAME-04, GAME-05, GAME-06, GAME-07
**Success Criteria** (what must be TRUE):
  1. `ctest -L gpu` passes, and the nightly lane matches the named ps1-tests VRAM references with tolerance 0. The `gpu_motion` frames at 60, 120 and 180 differ from each other and hash identically on every platform. Display-mode changes produce the documented width, height and depth, and libretro switches geometry with them.
  2. A button press changes play. With the `press@60` movie, the `frame@300` hash and the `presses` record of `pad_counter` both differ from the `none` movie, and three presses count exactly 3. DualShock rumble is reported per frame, and the `libretro.host` `press@60` frame equals the runner's.
  3. `memcard_roundtrip`, run with `--save-dir`, continues in a fresh process: run 1 shows 1 and run 2 shows 2, and the missing-card and wrong-card controls differ.
  4. OpenBIOS, built in CI from nugget at its pin, boots to its shell with a pinned frame hash that is the same on every platform that runs it. The compatibility ledger rejects any unratcheted regression. The local `library` preset, pointed at a fixture in a temporary directory, writes only `build/library/results.csv`.
  5. In the hosted RetroArch session, `pad_counter.exe` loads as content with no BIOS in `system/`. The picture changes between two `--max-frames` values, and each screenshot matches the runner's `--png` frame pixel for pixel.

**Plans**: TBD
**UI hint**: no
**Canonical refs:** `.planning/preparation/PSX-GPU-AND-GTE.md`, `.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md`, `.planning/preparation/CONFORMANCE.md`, `.planning/preparation/ARCHITECTURE.md`, `.planning/preparation/HOSTS-AND-INTEGRATION.md`

### Phase 5: The sound matches the console

**Goal**: The SPU sounds as the console does: it matches the hardware logs, its audio hashes are identical on every platform including `be32` and `wasm32`, and libretro delivers exactly the runner's audio stream
**Depends on**: Phase 4
**Requirements**: SOUND-01, SOUND-02
**Success Criteria** (what must be TRUE):
  1. `schwaystation-run --hash-audio` on the `spu_tone` fixture prints the same SHA-256 on all eight lanes. Each frame has exactly `frame_cycles / 768` samples, never padded, and a unit test derives the same PCM from the block table without the CPU.
  2. `ctest -L audio` passes on BRR, ADSR, noise, reverb, capture, IRQA and the transfer paths. The nightly lane reproduces the seven hardware pass lines of the ps1-tests `spu/memory-transfer` log.
  3. The libretro batch and single-sample audio paths deliver PCM equal to the runner's stream in `libretro.host`. RetroArch boots `spu_tone.exe` as content, and the idle SPU passes `core.no_busy_loop`.

**Plans**: TBD
**UI hint**: no
**Canonical refs:** `.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md`, `.planning/preparation/ARCHITECTURE.md`, `.planning/preparation/CONFORMANCE.md`, `.planning/preparation/HOSTS-AND-INTEGRATION.md`

### Phase 6: Discs and FMV

**Goal**: Discs and FMV work: the CD-ROM controller reads cue/bin, iso and m3u images, plays XA and CD-DA, and decodes MDEC video, and the own `disc_boot` image boots through OpenBIOS in RetroArch with working disk control
**Depends on**: Phase 5
**Requirements**: DISC-01, DISC-02, DISC-03, DISC-04
**Success Criteria** (what must be TRUE):
  1. `ctest -L disc` passes the CD-ROM controller's cycle timings, delivery order, sector buffer, GetID and lid behaviour, and the nightly lane matches the ps1-tests `cdrom/timing` and `getloc` logs.
  2. The mkpsxiso `disc_boot` image, built on every leg, boots through OpenBIOS from `SYSTEM.CNF`. It reads its sectors, plays the XA and CD-DA tracks, and its pinned `boots`, `mailbox`, `audio` and `frame` values are identical on every platform that runs it. `.chd`, `.pbp`, `.ecm` and `.ccd` are refused with exit 77 and a named message, and the rcheevos disc MD5 matches.
  3. `ctest -L mdec` passes, and the nightly lane matches the ps1-tests `mdec` references within the stated tolerance. The STR on `disc_boot` plays through DMA into the 24-bit display.
  4. Disk control works. Swapping images in a two-image m3u changes the TOC in `libretro.host` and raises INT5 on eject, and `schway_swap_disc` with the lid closed returns `ERR_LID_CLOSED`. The hosted RetroArch session boots `disc_boot` through the CI-built OpenBIOS, and its screenshot matches the runner's `--png` frame.

**Plans**: TBD
**UI hint**: no
**Canonical refs:** `.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md`, `.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md`, `.planning/preparation/ARCHITECTURE.md`, `.planning/preparation/CONFORMANCE.md`, `.planning/preparation/HOSTS-AND-INTEGRATION.md`

## Progress

**Execution Order:**
Phases execute in numeric order: 1 → 2 → 3 → 4 → 5 → 6

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. A test frame in RetroArch | 14/15 | In Progress | - |
| 2. The CPU matches the public vectors | 0/TBD | Not started | - |
| 3. The GTE matches the hardware cases; DMA and timers | 0/TBD | Not started | - |
| 4. Games play in RetroArch | 0/TBD | Not started | - |
| 5. The sound matches the console | 0/TBD | Not started | - |
| 6. Discs and FMV | 0/TBD | Not started | - |
