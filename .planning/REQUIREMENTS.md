# Requirements: schwaystation

**Defined:** 2026-10-19
**Core Value:** PlayStation games behave as they do on the console, identically on every platform, from a small MIT-licensed C library that any host can embed.

## v1 Requirements

Milestone v1.0. Each category is one phase; its pull request builds and rehearses the release archives (FRAME-06), and
merging it publishes them, which is confirmed at ship (AGENTS rule 6). A platform is one of six: Linux x64, Linux arm64,
macOS arm64, macOS x64, Windows x64, Windows arm64; "every platform" means all six plus the `be32` and `wasm32` lanes
(eight `hashes.txt` files). Expected values in tests cite their source document (`.planning/preparation/*`).
Acceptance is on exit status and tolerant regexes, never on exact tool output. No test needs a Sony BIOS, a
commercial disc or the owner's hardware; those run only in the local `library` preset. Every nightly lane is also
`workflow_dispatch`-able on a branch and is run once on the phase branch before `/gsd-ship`; the run URL is the evidence.

### A test frame in RetroArch

- [ ] **FRAME-01**: `cmake --workflow --preset ci` (`ci-msvc` on Windows) configures, builds, tests and packages the library, `schwaystation-run` and the libretro core on every platform, with the exact test inventory in `tests/inventory/ci.txt` executed once each and no skipped test.
- [x] **FRAME-02**: `schwaystation-run --frames 60 --hash` with no BIOS and no disc prints a frame hash of the built-in test card (320 × 240, 15-bit, with the 13-word metadata tuple) that is byte-identical on every platform, including the `be32` lane under qemu-ppc; the `wasm32` lane under wasmtime prints the same hash and is report-only until the phase-1 `ci:` tail promotes it (DEC.99), while `be32` gates from day one; `hash-equality` compares the eight `hashes.txt` files.
- [x] **FRAME-03**: A CTest (`libretro.host`) loads the libretro core in an in-process host, runs 60 frames with no content, and compares the delivered XRGB8888 frame with the runner's conversion byte for byte; `retro_get_system_av_info` reports fps 53,693,175/897,619 (NTSC) or 53,203,425/1,069,484 (PAL) as rationals, sample rate 44,100, base geometry 320 × 240 and aspect 4/3.
- [x] **FRAME-04**: `nm`-based CTests prove the core exports only `schway_*` symbols (`abi.symbols`), has no mutable globals (`abi.globals`), imports only `memcpy`, `memmove`, `memset` and `memcmp` plus the per-OS allowlist (`abi.undefined`), links no floating point (`hygiene.no_float`, the `nofp` preset with `-mgeneral-regs-only`), and that `include/schwaystation.h` compiles as C17 and C++17; the header stays under 40 KB and the README under 16 KB (`docs.budgets`).
- [ ] **FRAME-05**: `scripts/hygiene.sh --tree`, the title check (`ci.title`), the timeout-comment check (`hygiene.timeout_comments`), the SHA-pin check, `hygiene.no_bios_bytes` (signatures compared as hex) and the no-self-skip policy (`policy.no_skip`) run in CI, each with a negative control (`<name>.self_test` through `expect_fail.cmake`) that fails with its named message; `policy.gate_budget` holds policy and hygiene code (`scripts/*.sh`, `tests/cmake/*.cmake`, `.githooks/*`) at or under 1,500 lines in phase 1, re-pinned to the measured value at ship; `build.revision_matches_head` fails when the embedded revision differs from HEAD.
- [x] **FRAME-06**: The pull request's prerelease rehearsal (job `rehearsal` inside `CI required`, on release-input changes) runs every publish step with the App token against `rehearsal-<run_id>-<run_attempt>` and produces exactly 18 version-free archives `schwaystation-{library,runner,libretro}-{linux,macos,windows}-{x64,arm64}.zip`, `SHA256SUMS` and verified attestations, then deletes itself; `release.archive_contents` shows `LICENSE` and `THIRD-PARTY-NOTICES.md` in each archive; release-please (`simple`, `version.txt`, draft by ID) is configured so that merging publishes the same set with no manual step (the real publish is confirmed at ship).
- [x] **FRAME-07**: The README install line, run in CI against the built macOS arm64 libretro archive (and against `releases/latest/download/` once a release exists), verifies the checksum and installs `cores/schwaystation_libretro.dylib` and `info/schwaystation_libretro.info` where RetroArch 1.22.2 (Metal, DMG pinned by SHA-256) finds them; a hosted macOS job runs RetroArch with the core and no content for `--max-frames=5` and compares its screenshot with `schwaystation-run --frames 5 --png 5:FILE` pixel for pixel, with a one-byte negative control; `policy.e2e_budget` holds the e2e code at or under 300 lines.
- [x] **FRAME-08**: An installed-package C consumer (`install.consumer`), a C++ header consumer and an `add_subdirectory` embed with tests, runner and libretro off (`embed.subdirectory`) build in CI on every platform; `tests/api/v1.c` touches every public name.
- [x] **FRAME-09**: `schwaystation-run --bench` reports frame-time p50/p90/p99/p99.9/p99.99/p99.999, max µs and max RSS; a Cachegrind instructions-per-frame measurement ((Ir at 360 frames − Ir at 60) / 300 on the test card) runs on every pull request against its merge base with the +1 % gate present and report-only until two zero-spread control runs arm it (the budgeted `ci:` tail PR, DEC.99); `core.no_alloc_after_load` proves zero allocations after load through the counting allocator and `core.no_host_clock` proves the core never reads a host clock; the state visitor round-trips the test card byte for byte.

### The CPU matches the public vectors

- [ ] **CPU-01**: `ctest -L vectors` runs the committed sample `tests/vectors/r3000/sst-sample.bin` (the first 100 cases of each of the 55 SingleStepTests/r3000 files at `51e227447d0b`, 5,500 cases, under 4 MB) and every case matches on final registers, hi/lo, PC, EPC/TAR/CAUSE, both delay latches and every bus cycle's action, size, address and value; the harness exits 1 on a zero-case run; a nightly lane (`vectors-full`) fetches and runs the full 55,000 at the pin; a documented upstream inaccuracy is a dated line in `tests/vectors/r3000/allow.txt`, never a skip.
- [ ] **CPU-02**: `tests/cpu/exception_tests.c` and the `exc_matrix` fixture (`mailbox@30`, rows sourced `doc`) cover address error on LH/LW misalignment with BadVaddr, overflow on ADD, reserved instruction, cop1 unusable, SYSCALL and BREAK EPC with and without BD, IRQ masking through SR, the BEV vector choice and DBE/IBE on unmapped reads; `tests/cpu/delay_tests.c` covers the load-delay queue, the write-cancels-pending rule and the LWL/LWR interlock; `tests/cpu/timing_tests.c` covers the MULT/DIV completion stamps (6, 9, 13 and 36 cycles); `exc_matrix` writes the CAUSE value read by an `mfc0` in the instruction after an I_STAT write into `mailbox@30`; the test pins it as a `doc` row (DECISIONS open question), changed only by a dated `allow.txt` line.
- [ ] **CPU-03**: `ctest -L bus` passes register-level tests: physical decode under mask 1FFFFFFFh with KSEG2 answering only FFFE0130h, RAM_SIZE bank mirrors, the memory-control delay registers with the per-port cost table recomputed on every DEVn write, the direct-mapped I-cache (256 lines of 4 words, per-word valid bits, fill to line end, Isc and TAG isolation) with the 4-deep write queue under Isc, the data-only scratchpad, and I_STAT edge-set with CAUSE bit 10 as the live AND of I_STAT and I_MASK; no DMA channel and no root counter is in this phase.
- [ ] **CPU-04**: `schwaystation-run --exe FILE` loads a PS-X EXE validated per PER 7 (`t_addr`/`t_size` inside RAM, non-zero `pc0`, no write outside the 2 MB) under the own armips stub kernel (MIT, about 2 KB: power-on memory-control values, BEV cleared after the vectors are copied, an exception handler recording into the mailbox, SP from `s_addr + s_size` or 801FFF00h when zero, TTY port 1F802080h and exit port 1F802082h decoded only under the `debug_port` profile flag); `--tty` prints kernel A(3Ch)/B(3Dh) putchar bytes and stub-port bytes as lines prefixed `tty `; `--stop-on mailbox` exits 0 on status 00h and 6 on FFh; `hello.exe` shows the `SWMB` magic and the loader honours `pc0`, `gp0` and `s_addr`; armips at `62adab4ef30d`, tarball pinned by SHA-256, is built on every leg and assembles every fixture to one digest in `hashes.txt`; `fixtures.armips_pin.self_test` fails on a wrong digest; `tests/api/v2.c` touches every name added in this phase.
- [ ] **CPU-05**: `schwaystation-run --probe FILE` prints `format`, `serial`, `licence_area`, `region_guess`, `exe_name`, `exe_size`, `sha256`, `rcheevos_md5` (for a PS-EXE the MD5 rcheevos computes for that executable's bytes, or `n/a`) and the EXE header fields without emulating; `schwaystation-run --trace first-fault` stops at the first unmapped access, misaligned access, reserved instruction or exception loop for a crafted EXE and prints one `fault frame=N line=L dot=D pc=... op=... addr=... width=... dev=... kind=...` line plus a 4,096-record trace tail, exit status 7; the loader fuzz target `fuzz.exe` runs in the nightly `fuzz` preset (60 s) and `fuzz.regress.exe` replays its committed seeds on every leg, report-only until a tune-up (SEED-007) promotes them after consecutive green nightly runs (ENGINEERING 3).

### The GTE matches the hardware cases; DMA and timers

- [ ] **GTE-01**: `ctest -L gte` runs the 1,150 ps1-tests GTE cases (test-all) transcoded by `tools/gte2bin.py` from the MIT C table at `f727802fead1` into `tests/vectors/gte/ps1tests-1150.bin` with the notice in `THIRD-PARTY-NOTICES.md`, with 64-bit accumulators, 44-bit clipping, ares flag placement, the UNR table asserted against the printed table and the documented IR3/lm, FC, mx = 3 and H sign-extend quirks; the GTE cycle counts and the busy stall are covered in `tests/cpu/timing_tests.c`; the `gte_vectors` fixture's `mailbox@30` records match `expected.csv` rows sourced `hw`; a property test compares random inputs against a straight C reference including the flags.
- [ ] **GTE-02**: `ctest -L dma` passes register-level tests for the seven DMA channels with DPCR/DICR, per-word costs (1, 4, 24 or 40), the CPU stall rule, chopping windows, the 1000h-node linked-list guard, MADR wrapping at FFFFFCh, OTC (DMA6) clearing, and the `chain-looping` self-reference terminating by the guard; a fixture `mailbox` record of a DMA transfer is pinned in `expected.csv`.
- [ ] **GTE-03**: `ctest -L timers` passes register-level tests for the three root counters with sync modes, targets, the 2-cycle write hold and the zero dwell, and the IRQ modes (one-shot, repeat, pulse and toggle); the `timers_lines` fixture is assembled and its sysclk, sysclk/8 and vblank-count records judged per profile against PX.12 and PX.21 (its dot-clock and hblank records pin in GAME-02 once the GPU line machine exists).

### Games play in RetroArch

- [ ] **GAME-01**: `ctest -L gpu` renders triangles (edge functions with the top-left bias, 32-bit fixed-point Gouraud and UV steppers), lines (integer DDA), rectangles, fills and VRAM copies through the pixel pipeline (texel fetch via the CLUT cache, modulation, the 4 × 4 dither, the four blend modes, mask bits, texture window, GP0(E3/E4) clipping, 1023 × 511 size rejection); the nightly lane fetches ps1-tests `build-158` and compares `--hash-vram` with the `vram.png` references for `clipping`, `lines`, `rectangles`, `transparency`, `quad`, `triangle`, `uv-interpolation`, `texture-overflow`, `vram-to-vram-overlap`, `mask-bit`, `clut-cache`, `gp0-e1` and `gpustat` quantised to RGB555 with tolerance 0 unless `allow.txt` names a documented variant; the `gpu_motion` fixture's `frame@60/120/180` hashes are distinct and pinned on every platform; `fuzz.regress.gpu` replays its seeds.
- [ ] **GAME-02**: Display scan-out follows DEC.34: width `(((X2 − X1) / div) + 2) & ~3`, height Y2 − Y1, dot dividers 10/8/7/5/4, 15-bit and 24-bit (3 bytes per pixel) modes, field-sequential interlace at native height, display-off frames all zero with the flag set, GP1(05h/06h/07h) window moves; the frame hash covers the metadata tuple; DMA2 runs linked lists through the GPU busy budget with bits 26 and 28 and the stall; the libretro adapter switches geometry with `SET_GEOMETRY` on a mode change; the `timers_lines` fixture's `mailbox@120` records (263/314 lines, dot counts per line per horizontal mode, hblank per line) are pinned per profile and all fixture hashes are identical on every platform (the `portable` label selects the `be32` subset CONFORMANCE 5 names).
- [ ] **GAME-03**: SIO0 is cycle-scheduled (a write schedules byte-done at the baud-derived count, 1,088 cycles at 0088h; pad ack about 340, card ack about 1,500; IRQ7 never within 100 cycles of the TX write) and serves the digital pad (id 5A41h) and the DualShock (digital and analog modes, config commands 43h to 47h, 4Ch and 4Dh, both rumble methods, the 1 s watchdog) with rumble reported per frame as {large 0 to 255, small 0/1}; the `pad_counter` fixture proves a control changes play: between movies `none` and `press@60` the `frame@300` hash and the mailbox `presses` record differ, `press@60+120+180` counts exactly 3, and a held button counts once; `schwaystation-run --inputs FILE` applies the per-frame script and prints the lag count (a frame with no SIO0 pad transfer, peeks excluded); the in-process `libretro.host` maps the Beetle RetroPad and `RETRO_DEVICE_ANALOG` layout, delivers rumble through the rumble interface, and its `press@60` frame equals the runner's; `fuzz.regress.sio` replays its seeds.
- [ ] **GAME-04**: Memory cards are two raw 131,072-byte spans (`schway_get_memory` CARD1/CARD2) with a generation counter that bumps on each completed Write ending 47h; `schwaystation-run --save-dir DIR` writes `<basename>.srm` and `<basename>.1.mcr` through temp, fsync and rename on exit and on SIGTERM (exit 0); import accepts raw and short-padded-FFh images and lints without rewriting; the `memcard_roundtrip` fixture continues from its saved value in a fresh process (run 1 shows 1, run 2 shows 2, the two `frame@120` hashes differ, the second card equals the first plus one), with missing-card and wrong-card controls that differ; `fuzz.regress.memcard` replays its seeds.
- [ ] **GAME-05**: OpenBIOS is built in CI from nugget `9db6d88d23c4` (MIT, notice kept) with the pinned mipsel GCC on the Linux leg, its digest checked against `tests/built/expected.txt`, cached to the other legs within the run and never committed or uploaded; `schwaystation-run --bios openbios.bin --frames N --hash` boots to the OpenBIOS shell with a pinned frame hash identical on every platform that runs it; `schway_load_bios` reports the SHA-256 of any 524,288-byte image, matches it against the vendored libretro `System.dat` digest table (`12c21cf79f1e`) regardless of file name, and `release.firmware_scan` proves no archive member matches a retail digest.
- [ ] **GAME-06**: `tests/compat/ledger.csv` (ten columns keyed by serial plus rcheevos MD5 and predicate) and the generated `docs/compatibility.md` exist with the fixture rows, the ratchet (`unknown` to anything, `fail` to `pass`, `pass` to `pass`; anything else needs a dated `tests/compat/allow.txt` line) and the protected baseline that checks out the base ledger first; `policy.compat_doc_fresh` regenerates and compares; the local `library` preset (`scripts/library.sh`, never registered in `ci*`) reads `SCHWAYSTATION_BIOS_DIR` and `SCHWAYSTATION_DISC_DIR` from gitignored `.env.local`, runs every cue/bin against the ledger and writes only `build/library/results.csv` with serial, MD5, track SHA-1, DAT title, result and date, shown by a test that points it at a fixture in a temporary directory; the libretro `.info` carries `is_experimental "true"` until v2.0.
- [ ] **GAME-07**: One integrated session in RetroArch (the hosted macOS job, at most 300 lines of e2e code in total, checked by `policy.e2e_budget`) loads `pad_counter.exe` as content with no BIOS in `system/` (the adapter boots PS-EXE content under the stub kernel), the picture differs between two `--max-frames` values, and each screenshot matches the runner's `--png` frame pixel for pixel; `supports_no_game`, `input_descriptors`, `core_options`, `memory_descriptors` (RAM at 0, 80000000h, A0000000h and the scratchpad at 1F800000h) and `disk_control` claims in the `.info` are each tied to a configure-time guard test.

### The sound matches the console

- [ ] **SOUND-01**: `ctest -L audio` steps the SPU model (one tick per 768 cycles, voices 1 to 24 then reverb, BRR decode with carried history, the 512-entry gaussian table, the ADSR step rule checked against the ares-generated table, the psx-spx noise form, the two-phase reverb with the 39-tap FIR, capture buffers written every sample, int32 mixing saturated at the documented points, IRQ on any RAM access at IRQA, the 32-halfword FIFO and DMA4 transfer paths) and the nightly lane compares the ps1-tests `spu/memory-transfer` log's seven hardware pass lines; `schwaystation-run --hash-audio` prints the SHA-256 over the int16 stereo stream for the `spu_tone` fixture (`audio@600`, `mailbox@600` with per-voice ENVX records), identical on every lane, where a unit test derives the same PCM from the block table without the CPU; exactly `frame_cycles / 768` samples per frame (737 or 738 NTSC, 886 or 887 PAL), never padded; `fuzz.regress.spu` replays its seeds.
- [ ] **SOUND-02**: The libretro batch and single-sample audio paths deliver PCM equal to the runner's stream, shown by the in-process `libretro.host` test; the hosted RetroArch session boots `spu_tone.exe` as content under the stub kernel (the GAME-07 path) and proves video only; `core.no_busy_loop` holds for the idle SPU.

### Discs and FMV

- [ ] **DISC-01**: `ctest -L disc` runs the CD-ROM controller FSM in cycles only: fixed acknowledge 50,000, Init second response 476,000, Pause 2,170,000 >> speed (7,666 when paused), INT1 every 451,584 >> speed, the integer square-root seek model in 1/75 s ticks, the firmware delivery order (ADPCM first, INT1 only when no INT is pending, at most one undelivered INT1), the sector buffer exposing only the oldest and newest sector, GetID answered from the profile region against the licence string, the unsolicited INT5 on lid open; the nightly lane compares the ps1-tests `cdrom/timing` and `getloc` logs; `fuzz.regress.cdrom` replays its seeds.
- [ ] **DISC-02**: `host/disc_image.c` parses cue/bin (MODE1/2048, MODE2/2352, AUDIO), iso and m3u with the PER 7 bounds behind the `schway_disc` callbacks, refuses `.chd`, `.pbp`, `.ecm` and `.ccd` with "unsupported: CHD (planned v2.0)" style messages and exit 77, with `fuzz.cue`, `fuzz.cnf` and `fuzz.m3u` nightly and `fuzz.regress.cue`, `fuzz.regress.cnf` and `fuzz.regress.m3u` replaying their seeds per leg; the `disc_boot` fixture (one mkpsxiso v2.30 image under 512 KiB from a committed XML, built on every leg and never committed: the own PS-EXE, a Form2 XA track at 37.8 kHz stereo, a short STR, one generated CD-DA tone track, a Redump-style cue and a hand-written `.sbi`) boots through OpenBIOS from `SYSTEM.CNF` (`BOOT = cdrom:\SWAY.EXE;1`), reads 16 sectors with ReadN into a CRC-32 record, records GetID/GetTN/GetTD, plays the XA track and the CD-DA track; `boots@600`, `mailbox@900`, `audio@900` and `frame@900` are pinned and identical on every platform that runs it; the vendored rcheevos `rc_hash_disc.c` (MIT, `dfdc64ea035c`) CTest compares the MD5 on `disc_boot`; if OpenBIOS requires licence sectors, the fixture XML carries an own-authored placeholder licence area (never Sony bytes) and the requirement is met with it.
- [ ] **DISC-03**: `ctest -L mdec` decodes MDEC blocks with the integer two-pass IDCT (int16 Q14 scale table, ares rounding, Q16 colour coefficients) at 1000 cycles per block into 4/8/15/24-bit output for DMA1, and the nightly lane compares the ps1-tests `mdec` references with a fixed tolerance: bit-exact for the IDCT reference blocks (4-bit and 8-bit); STR frames (`frame` and `movie`) within ±1 per channel; the decoded-frame hash is over the raw output words before DMA1 reordering; the STR on `disc_boot` plays through DMA3 to DMA0 to DMA1 to DMA2 into the 24-bit display path; `fuzz.regress.mdec` replays its seeds.
- [ ] **DISC-04**: The libretro `SET_DISK_CONTROL_EXT_INTERFACE` (`set_initial_image`, `get_num_images`, `replace_image_index`, `get_image_path`, `get_image_label`) drives an m3u of two own images: eject opens the lid as the controller reports it and raises INT5, insert installs the new TOC, and `libretro.host` shows the TOC change; `schway_swap_disc` with the lid closed returns `ERR_LID_CLOSED` and changes nothing; the runner's `--inputs` control lines open, swap and close the lid in the `disc_boot` `press@300` movie; the hosted RetroArch session boots `disc_boot` through the CI-built OpenBIOS from `system/` and its screenshot matches the runner's `--png` frame; the `examples/sdl2/` host (one file, about 300 lines) builds against the installed library and is never packaged or tested beyond building; `core.no_busy_loop` proves an idle drive costs a fixed Ir bound.

## v2 Requirements

Later milestones. Each has a seed in `.planning/seeds/` that says when it comes back.

### The whole library (SEED-001)

- **LIB-01**: CHD through libchdr (BSD-3, behind `SCHWAYSTATION_CHD`), PBP, ECM, CCD and SBI/LibCrypt (`bad_q` served to GetlocP as hardware), with `fuzz.chd` and `fuzz.sbi`; the `.info` adds `chd`.
- **LIB-02**: Multitap SCPH-1070 with cards on the tap, GunCon and Justifier from the GPU-timestamped beam with a brightness test, mouse, neGcon, Jogcon, Dual Analog flight mode; the libretro subclasses named as Beetle names them.
- **LIB-03**: PocketStation as card storage (commands 58h onward answered, no ARM core), per-game cards by serial as a host policy (`--memcard-names serial`, `schwaystation_memcard1`), `.gme`, `.mem` and `.psx` import by magic, the libretro `SAVE_RAM` round trip in-process and in the second hosted case, `libretro_saves "true"`.
- **LIB-04**: PAL and NTSC-J timing and every board and GPU revision profile over the whole ledger (the libretro-database Redump DAT, 13,407 entries), the `fast` boot option, GameShark strings and the runner's `--patch` (PPF1/2/3, BPS) through the sector overlay, MDEC edge cases from the ledger's FMV titles; the libretro `.info` drops `is_experimental`.

### States and movies (SEED-002)

- **STATE-01**: Fixed-size, CRC-checked save states (header, behaviour revision, disc identity, cards outside by default) validated before apply, fork-tested at every frame, with the libretro `serialized` and `deterministic` claims behind their tests; rewind and run-ahead in RetroArch within the e2e line budget.
- **STATE-02**: An own text movie format with lag frames, BK2 then PJM import, emulated-time print and input display in the runner, Playstead continuation (`qualify`, `initial-run`, `continue`).

### Measured accuracy (SEED-003)

- **ACC-01**: Fixtures run on the owner's console settle the open constants (GPU clocks per line, the CD seek curve and INT1 period, per-primitive GPU cost, MDEC block cost, ADSR edge rates, the XA resampler, reverb rounding); nightly oracle lanes against released binaries (PCSX-Redux with OpenBIOS in CI; DuckStation regtest, Mednafen, ares, MAME locally); amidog and PeterLemon as local binary oracles.

### Tools (SEED-004)

- **TOOL-01**: Hooks at zero cost when off (instruction, bus, DMA, IRQ, line, frame, TTY), a GDB stub (`--gdb PORT`), `--symbols`, `--vram-dump`, `--trace full`, a memory-card editor, Python `ctypes` bindings and a Gymnasium environment, Lua on demand.

### Everywhere (SEED-005) and the browser (SEED-008)

- **PORT-01**: The cached interpreter behind `core.exec_mode_equivalence`, then a JIT seed; compile lanes for `armhf`, devkitPro and pspdev SDKs; the libretro buildbot recipe and the libretro-core-info `.info` PR; the Playstead `psx` adapter pin; per-device Ir budgets as ratchet inputs.
- **WEB-01**: An Emscripten build with a static page (canvas 320 × 240, AudioWorklet at 44.1 kHz, File-API sector serving, a user-picked BIOS never uploaded) whose frame and audio hashes equal native.

### Enhancements (SEED-006)

- **ENH-01**: Internal resolution, PGXP-style precision, widescreen, texture filtering, true colour and deinterlacers as a host renderer over the `gpu_backend` record stream and the GTE sideband, never in the hashed native path, each flagged in the replay identity; until then `policy.no_enhancement_in_core` rejects their option names in `src/`.

## Out of Scope

| Feature | Reason |
|---|---|
| A window, audio device or input device in this repository | Hosts own presentation (DEC.02) |
| Shaders, menus, netplay UI | Frontend features; the core exposes what they need |
| Any Sony BIOS, HLE kernel, licence file or game bytes | Copyright (ASSET_POLICY, DEC.49) |
| GPL, LGPL, non-commercial or unlicensed code | MIT licence (PROVENANCE, DEC.52 to DEC.54) |
| Overclock, CD speedup or any enhancement in the hashed path | Exact timing and the native frame are the product (DEC.08, DEC.32) |
| Signed macOS builds | A Developer ID certificate puts a legal name in artifacts; ad-hoc signing only |
| PS2, PSP and PS2 peripherals; `.psf` playback | Another console appears only as labelled precedent (BRIEF) |

## Traceability

| Requirement | Phase | Status |
|---|---|---|
| FRAME-01 | Phase 1 | Pending |
| FRAME-02 | Phase 1 | Complete |
| FRAME-03 | Phase 1 | Complete |
| FRAME-04 | Phase 1 | Complete |
| FRAME-05 | Phase 1 | Pending |
| FRAME-06 | Phase 1 | Complete |
| FRAME-07 | Phase 1 | Complete |
| FRAME-08 | Phase 1 | Complete |
| FRAME-09 | Phase 1 | Complete |
| CPU-01 | Phase 2 | Pending |
| CPU-02 | Phase 2 | Pending |
| CPU-03 | Phase 2 | Pending |
| CPU-04 | Phase 2 | Pending |
| CPU-05 | Phase 2 | Pending |
| GTE-01 | Phase 3 | Pending |
| GTE-02 | Phase 3 | Pending |
| GTE-03 | Phase 3 | Pending |
| GAME-01 | Phase 4 | Pending |
| GAME-02 | Phase 4 | Pending |
| GAME-03 | Phase 4 | Pending |
| GAME-04 | Phase 4 | Pending |
| GAME-05 | Phase 4 | Pending |
| GAME-06 | Phase 4 | Pending |
| GAME-07 | Phase 4 | Pending |
| SOUND-01 | Phase 5 | Pending |
| SOUND-02 | Phase 5 | Pending |
| DISC-01 | Phase 6 | Pending |
| DISC-02 | Phase 6 | Pending |
| DISC-03 | Phase 6 | Pending |
| DISC-04 | Phase 6 | Pending |

**Coverage:** v1 requirements: 30 total; 30 mapped (9/5/3/7/2/4); 0 orphaned; 0 duplicated.

---
*Requirements defined: 2026-10-19*
*Last updated: 2026-10-19 during preparation*
