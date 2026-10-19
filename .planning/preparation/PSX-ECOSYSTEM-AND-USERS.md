---
title: "PlayStation emulators, users and personas"
summary: "Every PS1 emulator, core and FPGA port with its licence checked at a pin; what players, speedrunners, TASers, developers and porters complain about, mapped to schwaystation responses by horizon; low-power and WASM budgets; library sizes and popularity seeds; the slot a permissive, deterministic, integer-only PS1 core fills."
read_when: "Before ranking features or horizons, choosing an accuracy oracle, writing persona or frontend requirements, planning a port, deciding what may be opened, or answering 'why not DuckStation / Beetle / ReARMed'."
updated: 2026-10-19
---

# PlayStation emulators, users and personas

| Question | Section |
|---|---|
| Which PS1 emulators exist: licence at pin, language, accuracy and speed tier, status, features? | 1.1 |
| Which source may be opened, and is OpenBIOS usable as a CI BIOS stand-in? | 1.2 |
| Which cores do RetroArch, handheld distros, Steam Deck and browsers ship by default? | 1.3 |
| What do users dislike, and what does schwaystation do about each (C.NN, G.NN)? | 2 |
| What does each persona need, and in which horizon? | 3 |
| Speedrun and TAS rules, movie format, lag frames, RL environment, homebrew debugging | 3.2 to 3.6 |
| Can an integer-only C interpreter hold full speed on Apple Silicon, Pi 4, RG35XX-H, WASM? | 4 |
| Library sizes, LibCrypt share, multi-disc count, popularity seeds, test suites | 5 |
| What combination does nobody offer today, and what are the risks? | 6 |

Licence rule applied: GPL, CC BY-NC-ND and closed source was not opened; LICENSE files, first-line headers, READMEs,
docs, issue trackers and release pages only. GitHub facts came from the REST API at the pins named in section 8;
libretro docs, psx-spx, TASVideos and the speedrun.com API were fetched on 2026-10-19. Blocked today: redump.org
(connection refused), Emulation General wiki and speedrun.com HTML (bot challenge), retroachievements.org (403);
the Redump counts below come from the libretro mirror of the Redump DAT and are marked so.

## 1. Emulators
### 1.1 Landscape
Accuracy tier: A hardware-verified or test-suite-led; B high (passes amidog CPU/GTE, whole library playable); C
playable with known timing gaps; D low or unknown. Speed tier: 1 dynarec; 2 cached interpreter; 3 interpreter.
RA RetroAchievements, RAh run-ahead, LR libretro core, D debugger.

| Emulator | Licence at pin (checked) | Lang | Acc | Speed | Active | Notable |
|---|---|---|---|---|---|---|
| DuckStation | CC BY-NC-ND 4.0 (LICENSE at 124a4b16feda); GPL-3 until 2024-09-01, PolyForm Strict 1.0.0 2024-09-01 to 09-13, then CC BY-NC-ND; README: modified forks and packaging not permitted; issues disabled on GitHub [ECO.01][ECO.02][ECO.03] | C++ | B | 1 (x86-64, AArch32/64, RV64 JIT; cached interpreter; interpreter) | yes, nightly 2026-10-19 | PGXP, upscaling, true colour, texture replacement, CHD/PBP/ECM/MDS, m3u, per-game cards + editor, 4,000-game cheat DB, RA, RAh, rewind, GunCon/Justifier, overclock; passes amidog CPU/GTE (own claim); Android build unsupported [ECO.02] |
| SwanStation (libretro hard fork of GPL-era DuckStation) | GPL-3 (cc94ebcd38a8) [ECO.04] | C++ | B | 1 | yes, 2026-10-19 | LR; RA; on the macOS arm64 buildbot [ECO.05][ECO.06] |
| Mednafen `psx` | GPL-2; 1.32.1 docs 2024-03-19 [ECO.07] | C++ | B (TASVideos' reference lineage) | 3 | slow cadence | DualShock, Dual Analog, neGcon, GunCon, Justifier, mouse, dance pad, multitap; no overclock; recommends a 3 GHz Phenom II class CPU; states and memcards "do not mix well" |
| Beetle PSX / Beetle PSX HW | GPL-2 (COPYING at 9bcc75d0a3e8); core-info v0.9.44.1 [ECO.08][ECO.05] | C | B | 1 opt (Lightrec dynarec option, off by default) / 3 | yes, 2026-10-19 | LR; RA (HW is RA's "most recommended"); PGXP, Vulkan/GL renderers, 1x to 16x, OpenBIOS fallback, m3u, CHD, PBP, SBI, CD speed 2x to 14x, CPU 50 to 750 %, GTE overclock "unstable"; netplay unsupported; RAh second instance breaks HW renderer [ECO.09][ECO.10] |
| PCSX-ReARMed (notaz; libretro fork) | GPL-2 (COPYING at c8816799b503; notaz 8b6c163575e6) [ECO.11][ECO.12] | C | C ("expect some inaccurate emulation", RetroPie) | 1 (Ari64 ARM/ARM64, Lightrec elsewhere; interpreter variant) | yes, 2026-10-03 | NEON GPU by Exophase (also SSE2), HLE BIOS fallback, enhanced resolution, threaded SPU/rendering, CHD since 2018 bounty; the handheld default everywhere (1.3); Switch libnx buildbot's only PS1 core [ECO.12][ECO.13][ECO.06] |
| PCSX-Reloaded | GPL-2 (lineage of ReARMed; not fetched separately, unverified) | C | C | 1 (x86) | dead | plugin architecture (PEOPS, Pete's) |
| PCSX-Redux | LICENSE is GPL-2 text at 91285bf736af (GitHub also detects GPL-2.0; the brief's "GPL-3" is wrong at this pin) [ECO.14] | C++ | B | 1/3 (dynarec + interpreter) | yes, 2026-10-19 | developer-first: MIPS debugger, GDB server, Lua breakpoints and web server, Ghidra, PPF, VRAM viewer; OpenBIOS is a submodule (1.2) [ECO.14][ECO.15] |
| ares `ps1` | ISC (LICENSE at 9408cb43d494); v148 2026-05-30 [ECO.16] | C++ | C (compat page: "Experimental", 118 completable, 82 in-game, 63 starts, 12 nothing, 8,595 untested of 8,870) [ECO.17] | 3 (threaded GPU; accuracy flags for address/bus errors and breakpoints) | yes | openable reference; whole-system emulator, not a library |
| Avocado | GPL-2 (LICENSE at d7fcf9060ac7); the brief's "MIT" is wrong [ECO.18] | C++ | C | 3 | dead, last commit 2023-09-19 | author's ps1-tests suite is MIT (5.4) |
| Rustation / rustation-ng | GPL-2 or later (both LICENSE files; 5e650b949d6e, aa1fb446e03a); the brief's "MIT" is wrong [ECO.19] | Rust | C ("still very glitchy and incomplete") | 3 | low: pushes 2026-06 | ng is a libretro core with a threaded software renderer, low-level CD controller, GDB debugger; psx-guide has no licence file [ECO.19] |
| XEBRA | closed; download xebra221106.zip; Windows + Android [ECO.20] | ? | B by reputation | 3 | dormant | timing research by its author |
| ePSXe | closed; 2.0.18 2025-12-23; HLE BIOS since 1.9.25; CHD in 2.0.18; Android [ECO.21] | ? | C | 1 | yes | plugins; HLE "lower compatibility, limited memory card" |
| no$psx | freeware, closed; v2.3 2026-06-05; Win32 [ECO.22] | ? | B | 3 | yes | debugger, profiler, breakpoints, I/O map, VRAM and polygon viewers, TTY console; psx-spx author |
| pSX | closed; dead since the late 2000s (unverified) | ? | ? | ? | dead | - |
| BizHawk Nymashock + Octoshock | repo LICENSE "a minefield"; both cores derived from Mednafen (GPL) [ECO.23] | C++/C# | B | 3 | yes, 2.11.1 | TASVideos' only accepted PS1 platform; BK2; RA hashing fixed in 2.9.x; rumble bug #4232 [ECO.24][ECO.25] |
| MAME psx driver | `src/mame/sony/psx.cpp` GPL-2.0; devices BSD-3-Clause: `cpu/psx/{psx,gte,dma,mdec,sio,rcnt,irq}.cpp`, `psx.h`, `psxdefs.h`, `video/psx.cpp/.h`, `sound/spu.cpp/.h`, `bus/psx/{ctlrport,analogue,memcard,multitap,gamebooster}.cpp`, `mame/sony/psxcd.cpp/.h` (96df6a708bdd) [ECO.26] | C++ | C | 3 | yes | openable devices; the driver file itself is GPL |
| psxact | GPL-3 forks only (whaison, johnny-mac; last push 2018); original repo gone [ECO.27] | C++ | D | 3 | dead | - |
| MiSTer PSX_MiSTer | GPL-2 (LICENSE at cd17b5a0ad26) [ECO.28] | VHDL | A-ish: README 90 % per block; MDEC "4996/5376" timing, CD seek model "should be added", GTE transfer delay "not fully correct" | FPGA | yes, 2026-08-21 | facts only (GPL) |
| Browser | EmulatorJS = RetroArch via Emscripten; PS1 Mate extension "powered by PCSX-ReARMed via EmulatorJS"; no permissive WASM PS1 core [ECO.29] | C | C | 1/3 | - | - |
| Mobile | DuckStation Android (unsupported by author), ePSXe Android, XEBRA Android [ECO.02][ECO.21][ECO.20] | - | - | - | - | - |

### 1.2 What may be opened
Openable with notice: ares `ares/ps1/*` (cpu, disc, dma, gpu, interrupt, mdec, memory, peripheral, spu, system,
timer) [ECO.16]; the MAME BSD-3 files listed above [ECO.26]; JaCzekanski/ps1-tests (MIT) [ECO.30]; SingleStepTests/r3000
(MIT; 1,000 cases per valid encoding, generated from ares' interpreter, so T3 not hardware) [ECO.31]; PSn00bSDK headers
(MPL-2.0 core; mkpsxiso GPL-2+) [ECO.32]. **OpenBIOS finding:** pcsx-redux's `src/mips` is the submodule
pcsx-redux/nugget; at 9db6d88d23c4 the repository LICENSE is MIT and `openbios/kernel/alloc.c` carries an "MIT License,
Copyright (c) 2025 PCSX-Redux authors" header; `openbios/` has no separate LICENSE file [ECO.15]. Its COMPATIBILITY.md
says it ships with PCSX-Redux, Beetle PSX, Beetle PSX HW and SwanStation, works with PCSX ReARMed, and lists Azure
Dreams (NTSC) and Wipeout (NTSC-U/C) as incompatible; Beetle's docs confirm "if none is provided, OpenBIOS is used"
[ECO.15][ECO.09]. **Recommendation:** build openbios.bin from the nugget pin in CI (MIT, notice kept) as the
no-retail-BIOS lane; the retail BIOS lane stays on the owner's machine (AP). **Strongest alternative:** an own HLE BIOS
(ReARMed-style), rejected: HLE is the top compatibility complaint (C.01). **Would change if:** nugget relicenses.
Never: DuckStation, SwanStation, Mednafen, Beetle, ReARMed, Redux core, Avocado, Rustation, MiSTer RTL, BizHawk cores.

### 1.3 Defaults in the wild
| Host | PS1 default | Source |
|---|---|---|
| RetroArch core-info (5a74858ab2f7) | mednafen_psx, mednafen_psx_hw, pcsx_rearmed (+interpreter, neon), swanstation, rustation, pcsx1; duckstation info says licence "Non-commercial" | [ECO.05] |
| libretro buildbot macOS arm64 latest | mednafen_psx_hw, mednafen_psx, pcsx_rearmed, swanstation (no duckstation) | [ECO.06] |
| buildbot Switch libnx / PSP | pcsx_rearmed only / none | [ECO.06] |
| RetroPie (Pi 2 to 4) | lr-pcsx_rearmed "excellent dynamic recompiler ... expect some inaccurate emulation"; standalone for x86; HLE "most games will have issues" | [ECO.13] |
| muOS (RG35XX-H, RG40XX, RG28XX) | PCSX ReARMed core; SwanStation also present; full speed after disabling rewind and "Highest" resampler; 45 to 48 fps with enhanced resolution | [ECO.33] |
| Onion (Miyoo Mini) | PCSX-ReARMed standalone and core; PS1 "playable with minor dips" | [ECO.34][ECO.35] |
| EmuDeck / RetroDECK (Steam Deck) | DuckStation standalone (RetroDECK lists it "Legacy" beside RetroArch) | [ECO.36] |
| Browser (EmulatorJS) | pcsx_rearmed (wasm) | [ECO.29] |
| RetroAchievements | Beetle PSX HW (most recommended), Beetle PSX, SwanStation, DuckStation (save-state "burn-in" warning); console id 12 | [ECO.37][ECO.38] |

## 2. Complaints and wishes → schwaystation response
Who: P player, D developer, H ROM hacker, T TASer/speedrunner, F frontend or porter. Horizons are the proposed ones of
3.1. **won't** = not in this repository, with the reason. DuckStation has GitHub issues disabled, so its rows cite docs
and forums [ECO.03].

| # | Complaint or wish (who) | Evidence | schwaystation response |
|---|---|---|---|
| C.01 | BIOS friction: HLE fallback breaks Suikoden memcards and black-screens games; "most games will have issues"; users rename wrong revisions; OpenBIOS fails Azure Dreams and Wipeout (P, F) | [ECO.12][ECO.13][ECO.07][ECO.15] | **v1.0** retail BIOS by md5 under the libretro names (scph5500/5501/5502, psxonpsp660, ps1_rom); OpenBIOS as the documented fallback with its known-bad list in the ledger; **won't** HLE BIOS |
| C.02 | Accuracy vs enhancements: PGXP stops Silent Hill cutscenes, "memory only" recommended, vertex cache "leave off"; GTE overclock "unstable"; CPU clock 50 to 750 % desyncs A/V; widescreen and texture replacement wanted (28 reactions on Beetle #492) (P) | [ECO.09][ECO.39] | **v1.0** native 15-bit output, dithering as hardware, nothing speed- or geometry-changing on by default; **seed v6.0** PGXP-style precision and upscaling as a host-side renderer over the core's command stream, never in the hashed frame; **won't** overclock in the hashed path |
| C.03 | "Wobbly textures" and vertex jitter reported as bugs; libretro and MiSTer threads explain affine mapping and no sub-pixel precision (P) | [ECO.40] | **v1.0** not-a-bug list in `docs/compatibility.md` with the libretro explanation linked |
| C.04 | Audio crackling and stutter: Beetle single-core load, vsync, audio latency, overclock; brief pauses on CD access (image preload fixes it); muOS slowdown from rewind and "Highest" resampler (P) | [ECO.41][ECO.07][ECO.33] | **v1.0** exact rational rates reported (59.826/59.940 NTSC, 49.761/50.000 PAL), fixed 44,100 Hz SPU output, no host blocking I/O in `run_frame`; disc reads from memory or a sector cache; **v3.0** frame-time percentiles in the runner |
| C.05 | CHD and multi-disc: `.CD#` renames break CHD m3u; CHD has no subchannel so `.sbi` must sit beside it; PBP must be added by hand; 3DS CHD crash; "resume a few frames after eject" (P) | [ECO.39][ECO.09][ECO.42] | **v1.0** cue/bin, m3u; **v2.0** CHD via libchdr (BSD-3), PBP, ECM, SBI beside any container, disc swap with the lid-open timing as hardware |
| C.06 | Memory cards: per-game vs shared confusion, `.srm` vs `.mcr`, missing folders, second card shared, states overwrite cards, "normalise CPU clock during card access" (P) | [ECO.43][ECO.09][ECO.07][ECO.39] | **v1.0** raw 128 KiB cards with a generation counter, `.srm` + `.1.mcr` names (PER file); cards never inside states; **v2.0** per-game by serial as a host policy; **v5.0** editor in the runner |
| C.07 | Analog and DualShock: analog mode toggle combos, self-calibration, rumble only with DualShock type and only when all three layers agree; rumble framerate drop with 8BitDo; "analog default via database" (P) | [ECO.09][ECO.42][ECO.07] | **v1.0** DualShock with both rumble methods, Analog button as an input, config commands (PER 2.9); **v2.0** per-title default pad from a hash list, never a behaviour hack |
| C.08 | Lightgun: Beetle HW "lightgun API is broken" open since 2019; Point Blank crosshair; scale X/Y wanted; Justifier wrong on sub-ports (P) | [ECO.39][ECO.42][ECO.07] | **v2.0** GunCon and Justifier from the exact (line, dot) of each pixel with a brightness test (PER 2.x); libretro lightgun + pointer; offsets in the host |
| C.09 | Run-ahead and latency: second-instance RAh breaks the HW renderer; DuckStation lists RAh, Beetle docs do not; speedrun rules forbid it (P, T) | [ECO.09][ECO.02][ECO.44] | **v1.0** pad latched at the frame boundary, no internal buffering; **v3.0** fixed-size states fast enough for RAh, measured latency |
| C.10 | Save states: version breaks, memory leaks and "burn-in" with states (RA warning on DuckStation), states vs cards (P, T) | [ECO.37][ECO.07] | **v3.0** versioned, endian-fixed state with header and refusal codes; cards outside states |
| C.11 | Cheats: GameShark joker codes always true; built-in cheats fail; native cheats unsupported in Beetle; scratchpad not exposed to RA (P, H) | [ECO.42][ECO.09] | **v2.0** GameShark/Action Replay decoder (incl. joker, D0 types) in the runner, recorded in replay identity; **v1.0** scratchpad in the rcheevos map (5.3) |
| C.12 | PAL and region: 50 Hz forced or ignored, "PAL video timing override" toggle wanted, mismatched BIOS region, forced progressive timing (P, T) | [ECO.39][ECO.09] | **v1.0** NTSC-J, NTSC-U/C and PAL profiles with exact line and rate tables [ECO.45]; region from the disc's SYSTEM.CNF/serial, user override; TASVideos' "region must match" documented |
| C.13 | Performance on handhelds and browsers: interpreter 2.5× slower than dynarec; ARM needs Ari64; enhanced resolution costs 10 to 15 fps on H700; PS1 among the "harder" browser cores (P, F) | [ECO.46][ECO.33][ECO.29] | **v1.0** Ir/frame gate; **v6.0** cached interpreter and device budgets (4); **seed** JIT |
| C.14 | FMV/MDEC glitches and YUV filtering options; MiSTer MDEC timing off; chroma filter Vulkan-only (P) | [ECO.09][ECO.28] | **v1.0** MDEC decoded with the documented IDCT and fixed-point rounding, hashed; 24-bit output path native |
| C.15 | CD timing: faster loading "can crash games which do not expect the CD-ROM to operate faster"; Beetle speeds above 2x "glitches or softlocks"; speedrun boards polluted by seek speedup (P, T) | [ECO.47][ECO.09] | **v1.0** 2x drive model with seek and spin-up timing from measurement (SCM file); **won't** read/seek speedup in the hashed path; **v2.0** optional, flagged in replay identity |
| C.16 | LibCrypt: PAL titles need `.sbi`; Skip BIOS breaks them; CHD lacks subchannel (P) | [ECO.09][ECO.48] | **v2.0** SBI and LSD readers, subchannel Q served to GetlocP as hardware; ledger rows for the ~100 titles |
| C.17 | Licence and forking drama: DuckStation's two relicensings "to deter certain parties", packagers "collateral damage", SwanStation hard fork, boycott calls; core-info marks it "Non-commercial" (F, D) | [ECO.01][ECO.03][ECO.49][ECO.05] | **v1.0** MIT core, attested releases, CLA-free contributions; the signal: a permissive, embeddable PS1 core is the thing distributions, frontends and forks cannot get today |
| C.18 | Developer tooling: no$psx and PCSX-Redux have debuggers, TTY and GDB; libretro cores have none; GTE test "closed source and doesn't tell you why you're failing" (D) | [ECO.22][ECO.14][ECO.50] | **v1.0** TTY capture of A(3Ch)/B(3Dh) putchar in the runner, side-effect-free peek; **v5.0** GDB stub, breakpoints, VRAM viewer, trace |
| C.19 | Compatibility reporting: ares lists 8,595 of 8,870 untested; DuckStation claims amidog passes; nobody publishes hardware-verified timing (P, D) | [ECO.17][ECO.02] | **v1.0** generated ledger per disc hash with evidence class; **v4.0** hardware captures |
| G.01 | Determinism and netplay: Beetle "netplay unsupported"; states and rewind corrupt cards (P, T) | [ECO.09][ECO.07] | **v1.0** cross-platform frame/audio hashes; **v3.0** deterministic states tested from power-on |
| G.02 | Frame pacing: core-reported FPS "force_progressive" vs interlaced toggles confuse hosts (P, F) | [ECO.09] | **v1.0** report the exact rate of the current video mode and a mode-change event |
| G.03 | Dependency weight and build pain: Qt, Vulkan, SDL, Lightrec, glibc; Mednafen wants a 3 GHz desktop (F) | [ECO.07][ECO.02] | **v1.0** C17, no dependencies, no FP, no allocation after load |
| G.04 | Docs and release cadence: DuckStation nightly, Mednafen yearly, Avocado and Rustation dead (P) | [ECO.02][ECO.07][ECO.18][ECO.19] | **v1.0** release every phase with a changelog and ledger diff |
| G.05 | Telemetry and UI: none wanted in a core (F) | [ECO.49] | **won't** GUI, network, telemetry |
| G.06 | Debuggers for porters: Switch and Vita crashes with no core diagnostics (F) | [ECO.42] | **v1.0** runner exit codes and `--probe`; **v6.0** per-platform CI lanes |

Praise to keep: RetroPie on ReARMed's dynarec; RA on Beetle HW; TASVideos on Mednafen's lineage; Redux's debugger.

## 3. Personas and jobs to be done
### 3.1 Needs by horizon
Proposed horizons (derived; sibling pattern [ECO.51]): v1.0 boot and play with sound in RetroArch (interpreter,
software GPU, DualShock, one card per port) · v2.0 whole library (multitap, guns, LibCrypt, CHD/PBP, cheats, all
regions) · v3.0 states, movies, run-ahead, netplay · v4.0 measured accuracy (hardware captures, CD timing) · v5.0 tools
and AI · v6.0 everywhere (cached interpreter, handhelds, WASM, Playstead).

| Persona | Job | Table stakes [horizon] | Differentiators [horizon] |
|---|---|---|---|
| Player, Mac + RetroArch | Play dumps as the console did; keep saves | arm64 dylib [v1.0]; cue/bin, m3u [v1.0]; retail BIOS or OpenBIOS [v1.0]; DualShock + rumble [v1.0]; PAL [v1.0]; CHD, PBP, SBI, guns, multitap [v2.0]; states, RAh, rewind [v3.0]; RA map [v1.0] | exact rates, not-a-bug list, ledger [v1.0] |
| Host builder (Playstead, OpenEmu, custom) | Embed one small permissive core | `.info` with Beetle's BIOS names [v1.0]; fixed serialize size [v3.0]; MIT C API, no globals [v1.0] | Playstead continuation [v3.0]; sector hook for custom disc sources [v2.0] |
| Speedrunner | Fair, verifiable runs | 2x CD timing with no speedup [v1.0]; exact rates [v1.0]; practice states [v3.0]; input display [v5.0] | emulated-time print and run replay verifies a run [v3.0] |
| TASer | A movie that syncs everywhere | determinism, states, lag counter [v3.0] | BK2 import [seed v3.0]; Lua [v5.0] |
| AI/RL researcher | Train on PS1 games | Gym env, RGB frames + 2 MiB RAM, deterministic reset, frame stepping [v5.0] | vectorised instances, video-off, 5,000+ frames/s per thread [v5.0] |
| Homebrew developer (PSn00bSDK, Psy-Q, nugget/psyqo, Net Yaroze) | Know it runs on hardware; iterate in seconds | TTY from putchar A(3Ch)/B(3Dh) [v1.0]; PS-EXE and ELF load [v1.0]; headless hashes for CI [v1.0] | GDB stub, breakpoints, VRAM and GTE views, symbol files [v5.0] |
| ROM hacker and translator | Patch and prove the patch | PPF1/2/3 and BPS in the runner, sector overlay (PER file) [v2.0]; SYSTEM.CNF and serial probe [v1.0] | hash diff of original vs patched runs [v2.0]; text-table extraction hooks [v5.0] |
| Porter (ARM handhelds, Switch, wasm32) | Hold 60 fps on a given CPU | integer-only C17, no threads [v1.0]; wasm32 hash lane [v1.0] | cached interpreter and per-device Ir budgets [v6.0]; JIT seed |

### 3.2 Speedrunners
speedrun.com platform "PlayStation" is `wxeod9rn` with 1,688 games (API, 2026-10-19); every ruleset fetched allows
emulators except Final Fantasy VII "PSX Disc" (console with physical CDs only) [ECO.52]. The 2022 PS1 emulation rules
post (seen through a search excerpt; the page itself is bot-blocked) accepts BizHawk 2.8+ with Octoshock or Nymashock
and a named DuckStation build, requires an audio throttle setting on Octoshock and `.cue` images [ECO.44]. DuckStation
seek and read speedups are described by DuckStation itself as able to "crash games which do not expect the CD-ROM to
operate faster"; a community thread reports most DuckStation leaderboard runs used a non-zero seek speedup and a board
changed its rules to forbid altering seek/read speeds [ECO.47]. Main-category run counts (API, top-1000 cap): Crash Team
Racing 520, Metal Gear Solid 412, Castlevania SotN 381, Spyro 333, Crash Bandicoot 260, Crash 2 100 % 175, FF VII 147
[ECO.52]. **Recommendation:** no speedups in the hashed path, emulated time printed, input display in the runner.

### 3.3 TASers
TASVideos: 259 PSX publications; BizHawk is the only preferred PS1 emulator (BK2); PCSX (PXM) and PSXjin (PJM) are
listed as not accepted [ECO.24][ECO.53]. Framerates used: NTSC 59.94005994005994 (114 movies) and 59.94006013870239
(6), PAL 49.764559357596745 (6), plus progressive rates near 59.826 [ECO.54]. Movie rules: BIOS unmodified, region must
match, no forced 50/60 Hz swaps [ECO.53]. Publications by game (top of the 2026-10-19 list): Symphony of the Night, Crash
2, Crash Warped, Crash Bandicoot, Final Fantasy VII, Bishi Bashi Special 3 [ECO.24]. Nymashock and Octoshock coexist in
BizHawk; 2.9.x fixed PS1 RA hashing and a Gex lock-up in both [ECO.25]. **Recommendation [v3.0]:** own text movie with
disc hashes, BIOS identity, region, port devices, behaviour revision; lag = no SIO0 pad poll in the frame; BK2 importer
as a whole-game sync test [seed]. **Would change if:** TASVideos accepts a standalone runner.

### 3.4 AI agents
stable-retro lists no PS1 core (Snes9x, GPGX, FCEUmm, Stella, Gambatte, mGBA, PCE, PicoDrive, Beetle Saturn, melonDS,
FBNeo, N64, Flycast) [ECO.55]. **Recommendation [v5.0]:** Gymnasium env over the C API; observations RGB frame plus
2 MiB RAM and 1 KiB scratchpad; DualShock as 16 buttons + 4 axes; sticky actions and frame skip in the env.

### 3.5 Homebrew developers
BIOS TTY: A(3Ch)/B(3Dh) putchar, A(3Eh)/B(3Fh) puts, A(3Fh) printf [ECO.56]. PCSX-Redux offers a GDB server, Lua
breakpoints, a web server API, Ghidra and PPF tools; no$psx a TTY console, profiler and breakpoints [ECO.14][ECO.22].
PSn00bSDK core is MPL-2.0 [ECO.32]. **Recommendation:** TTY capture and PS-EXE load in v1.0; GDB stub seeded for v5.0.

### 3.6 Frontends
Beetle's option keys and BIOS names are the de-facto libretro schema (section 1.3 and [ECO.09]); DuckStation's
standalone shape drives EmuDeck [ECO.36]. schwaystation: same BIOS names and compatible option keys [v1.0];
libretro first, Playstead adapter [v6.0], OpenEmu plugin seed.

## 4. Low-power device budgets
**Work per NTSC frame** (derived from [ECO.45]): CPU 33.8688 MHz / 59.8173 Hz = 566,203.775 CPU cycles (405,350,378,496/715,909, ARCHITECTURE 1, derived); GPU 3413 × 263 =
897,619 video cycles; SPU 24 voices × 737.244 samples = 17,694 voice-samples at 44.1 kHz (derived); MDEC up to a 320×240 frame per
2 video frames in FMV. **Anchor** [ECO.46]: PCSX-ReARMed on an i7-7700K (about 4.5 GHz turbo) runs Final Doom at 246 fps
interpreter / 621 dynarec, Resident Evil 250 / 642, Tekken 3 190 / 279; that is 1.1 to 1.4 G host cycles per emulated
second for the interpreter and 0.4 to 1.0 G for the dynarec (derived), including a SIMD software GPU. **Cost model**
(unverified until the v1 Cachegrind gate): a plain C interpreter with per-access timing, integer software GPU and SPU is
expected at 1.5 to 2.5 G host cycles per emulated second, i.e. 25 to 42 M Ir per frame at an IPC near 1; a cached
interpreter removes decode and the fetch path (expect 1.3 to 1.6× faster); WASM runs 45 to 55 % slower on average, 2.1 to
2.5× at peak [ECO.57].

| Device | CPU | PS1 today | Budget (G host cycles/s per core) | Interpreter | Cached | JIT |
|---|---|---|---|---|---|---|
| Apple Silicon Mac (owner) | high-IPC, 3.5 GHz class | DuckStation, Beetle HW | 3.5 × IPC ≥ 3 → ≥ 10 | yes, 4 to 7× realtime | yes | n/a |
| Raspberry Pi 4 | Cortex-A72 ×4, 1.5 to 1.8 GHz [ECO.58] | lr-pcsx_rearmed dynarec "adequate" [ECO.13] | 1.5 to 1.8 × IPC ~1.5 → 2.2 to 2.7 | marginal (0.9 to 1.8×) | yes | yes |
| RG35XX H / Plus, RG40XX (Allwinner H700, Cortex-A53 ×4; clock unverified here) | in-order A53 | ReARMed dynarec full speed; 45 to 48 fps with enhanced resolution [ECO.33] | ~1.5 × IPC ~0.8 → 1.2 | no (0.5 to 0.8×) | marginal | yes |
| Miyoo Mini Plus | Cortex-A7 ×2, 1.2 GHz (1.4 to 1.8 overclock) [ECO.35] | ReARMed dynarec "minor dips" | 1.2 × IPC ~0.8 → 1.0 | no | no | yes |
| Nintendo Switch (homebrew) | Cortex-A57 ×4, 1.02 GHz (unverified) | pcsx_rearmed only [ECO.06] | ~1.5 to 2 | marginal | yes | yes |
| PSP | Allegrex 333 MHz | no libretro PS1 core; native POPS only | 0.3 | no | no | no |
| WASM, desktop Chrome | ÷1.5 to 2.5 | EmulatorJS pcsx_rearmed | 4 to 6 | yes, 2 to 3× | yes | no |
| WASM, 2 GHz phone | ÷1.5 to 2.5 | - | 1 to 1.5 | no | marginal | no |

**Recommendation:** v1.0 ships the interpreter with an Ir/frame CI gate (target ≤ 30 M Ir per NTSC frame on the
reference set: BIOS boot plus three titles); v6.0 adds a cached interpreter (block cache keyed by physical PC, invalidated
on RAM writes) that makes A72 and A57 comfortable and A53 marginal; a JIT stays a seed because it breaks the integer-only,
no-mmap, wasm32 posture. Handhelds below A72 keep PCSX-ReARMed. **Strongest alternative:** JIT in v2.0 for handheld
distros. **Would change if:** v1 measures ≤ 15 M Ir/frame (A53 becomes comfortable) or ≥ 50 M (Pi 4 slips).

## 5. Library and compatibility measurement
### 5.1 Library sizes
| List | Count | Source |
|---|---|---|
| Redump "Sony - PlayStation" entries (libretro mirror of the DAT, version 2026.10.07; redump.org itself unreachable today) | 13,407 total; Japan 6,489; USA 2,386; Europe 2,307; Germany 688; France 583; Italy 377; Spain 299; Australia 22; Asia 18; Korea 15 | [ECO.59] |
| Multi-disc | 3,570 "(Disc N)" entries across 566 titles; demo and sampler entries 557 (unverified: a name grep for "demo" or "sampler" gives 599) | [ECO.59] (derived) |
| LibCrypt | psx-spx: "about 100 PSX games", PAL; community counts 223 European Redump discs and a 237-disc patcher list; about 10 % of Europe entries (derived) | [ECO.48] |
| speedrun.com PS1 games | 1,688 | [ECO.52] |
| TASVideos PSX publications | 259 | [ECO.24] |
| ares compatibility rows | 8,870 (118 completable) | [ECO.17] |
| RetroAchievements PS1 sets | not fetched (403); open question | [ECO.37] |

### 5.2 Popularity seed for the ledger
Union of speedrun.com run counts and TASVideos publications (3.2, 3.3): Crash Team Racing, Metal Gear Solid, Castlevania
SotN, Spyro the Dragon, Crash Bandicoot 1 to 3, Final Fantasy VII, Tekken 3, Tony Hawk's Pro Skater 2, Gran Turismo
[ECO.52][ECO.24]; plus the OpenBIOS incompatibles (Azure Dreams, Wipeout) and the LibCrypt examples (MediEvil, CTR PAL,
Dino Crisis PAL, Legacy of Kain PAL) as timing and protection probes [ECO.15][ECO.48].

### 5.3 RetroAchievements
`RC_CONSOLE_PLAYSTATION = 12`; regions: Kernel RAM 0 to 0xFFFF at 0x00000000, System RAM 0x10000 to 0x1FFFFF at
0x00010000, Scratchpad 0x200000 to 0x2003FF at 0x1F800000 (dfdc64ea035c) [ECO.38]. Hash: parse SYSTEM.CNF, take the
`BOOT=` executable's path and name plus its contents into one buffer, MD5 it [ECO.60]. schwaystation: identical map and
a runner `--probe` that prints the RA hash [v1.0].

### 5.4 Test suites and oracles
| Suite | Licence | Covers | Use |
|---|---|---|---|
| JaCzekanski/ps1-tests | MIT | CPU, GTE, GPU, timers, DMA, CD, SPU behaviour tests with expected images | CI oracle [ECO.30] |
| SingleStepTests/r3000 | MIT | per-instruction JSON cases from ares' interpreter (valid encodings only) | CPU unit tests; T3 [ECO.31] |
| amidog psxtest_cpu / psxtest_gte | closed binaries | CPU and GTE arithmetic; "doesn't tell you why you're failing" | pass/fail smoke [ECO.50] |
| PeterLemon/PSX | no licence file | bare-metal demos | run only, do not copy [ECO.61] |
| OpenBIOS (nugget) | MIT | boot path without retail BIOS | CI BIOS lane [ECO.15] |
| MiSTer RTL, TASVideos movies | GPL facts / BK2 | timing reference; whole-game sync | differential, v3.0 to v4.0 [ECO.28][ECO.24] |

## 6. The slot schwaystation fills
No PS1 emulator combines a permissive licence, an embeddable C library, integer-only determinism, a measured ledger
and first-class tooling. The popular ones are CC BY-NC-ND (DuckStation) or GPL (SwanStation, Mednafen/Beetle, ReARMed,
Redux core, MiSTer); the permissive one (ares, ISC) is a whole-system emulator with an experimental, mostly untested PS1
core; the two "permissive" names in the brief turned out GPL (Avocado, Rustation) and are dead or glitchy; MAME's BSD-3
devices sit inside a GPL driver and frontend [ECO.01][ECO.16][ECO.17][ECO.18][ECO.19][ECO.26].
- Underserved: frontends and distributions that cannot ship NC or GPL code (the DuckStation relicensing made this
  concrete [ECO.03][ECO.49]); homebrew developers wanting CI-grade determinism with TTY and GDB in a library; browser
  and Playstead hosts needing wasm32 without a JIT; TASers and researchers needing frame stepping and RAM access
  outside BizHawk.
- Risks and the posture that answers them: (1) scope, a PS1 core is 5 to 10× an 8-bit core (CPU with load delays and
  cache, GTE, GPU rasteriser, SPU with reverb and ADPCM, CD controller with seek model, MDEC, SIO, DMA, timers): the
  horizons ship a playable v1.0 before breadth, and ares/MAME/ps1-tests are openable references; (2) BIOS legal
  position: OpenBIOS (MIT) is the CI lane, retail images never leave the owner's machine (AP); (3) CD timing: v1.0
  models a 2x drive from documented sector rates and seek settling [ECO.62], v4.0 measures against hardware and the
  MiSTer model; (4) speed: integer-only interpreter gated per frame, cached interpreter later, JIT seeded.
- **Strongest reason this is wrong:** ares could promote its PS1 core and expose a library API; SwanStation already
  gives GPL-tolerant frontends a strong core; a browser host may accept GPL. **Would change if:** a permissive PS1
  core with a libretro adapter, published hashes and a ledger appears.

## 7. Open questions
- Redump's own per-region counts and LibCrypt list (site unreachable); RetroAchievements PS1 set count and most-played list.
- speedrun.com's current PS1 emulator rules text and the Crash-series thread (bot-blocked); confirm the DuckStation build named.
- Allwinner H700 and Switch clocks from a primary page; IPC of emulator code on A53/A57 for the section 4 verdicts.
- Whether TASVideos would accept a standalone runner; whether BizHawk wants a native C PS1 core.
- DuckStation and Beetle interpreter vs cached interpreter numbers on one named CPU (nobody publishes); measure in v1.
- pSX and PCSX-Reloaded status lines were not fetched (marked unverified).

## 8. Sources
| ID | Tier | Source (URL) | Pin or access date | Supports |
|---|---|---|---|---|
| ECO.01 | T1 | github.com/stenzek/duckstation LICENSE (CC BY-NC-ND 4.0), LICENSE history (7f4e5d55dbde 2024-09-01) | 124a4b16feda | licence at pin |
| ECO.02 | T1 | github.com/stenzek/duckstation README.md (features, requirements, "Contributing and Forks", Android) ; releases | 124a4b16feda; latest 2026-10-13 | features, fork terms, amidog claim |
| ECO.03 | T4 | gamingonlinux.com 2024-09 DuckStation licence article and comments; GitHub API `has_issues: false` | accessed 2026-10-19 | relicensing dates and reasons, issues disabled |
| ECO.04 | T1 | github.com/libretro/swanstation LICENSE (GPL-3) | cc94ebcd38a8 | SwanStation licence |
| ECO.05 | T1 | github.com/libretro/libretro-core-info *_libretro.info for mednafen_psx(_hw), pcsx_rearmed, swanstation, duckstation | 5a74858ab2f7 | shipped cores, licences, BIOS names, versions |
| ECO.06 | T1 | buildbot.libretro.com/nightly/apple/osx/arm64, nintendo/switch/libnx, playstation/psp latest listings | accessed 2026-10-19 | cores per platform |
| ECO.07 | T1 | mednafen.github.io/documentation/psx.html (1.32.1) | 2024-03-19 | BIOS names, peripherals, limitations, host CPU |
| ECO.08 | T1 | github.com/libretro/beetle-psx-libretro COPYING (GPL-2) | 9bcc75d0a3e8 | Beetle licence |
| ECO.09 | T1 | docs.libretro.com/library/beetle_psx_hw/ | accessed 2026-10-19 | options, formats, SBI, OpenBIOS fallback, RA/RAh/netplay flags |
| ECO.10 | T4 | github.com/libretro/beetle-psx-libretro issue 214 (dynarec bounty, 393 comments) | accessed 2026-10-19 | dynarec option history |
| ECO.11 | T1 | github.com/libretro/pcsx_rearmed COPYING (GPL-2) | c8816799b503 | ReARMed licence |
| ECO.12 | T1 | github.com/notaz/pcsx_rearmed README (Ari64, lightrec, NEON GPU by Exophase); docs.libretro.com/library/pcsx_rearmed/ | 8b6c163575e6; accessed 2026-10-19 | dynarec and GPU history, HLE warnings, options |
| ECO.13 | T1 | github.com/RetroPie/RetroPie-Docs docs/Playstation-1.md | master, accessed 2026-10-19 | Pi defaults, HLE compatibility, accuracy note |
| ECO.14 | T1 | github.com/grumpycoders/pcsx-redux LICENSE (GPL-2 text), README, .gitmodules; pcsx-redux.consoledev.net index | 91285bf736af; accessed 2026-10-19 | Redux licence, debugger, GDB, Lua, OpenBIOS submodule |
| ECO.15 | T1 | github.com/pcsx-redux/nugget LICENSE (MIT), openbios/kernel/alloc.c header, openbios/README.md, openbios/COMPATIBILITY.md | 9db6d88d23c4 | OpenBIOS licence and compatibility |
| ECO.16 | T1 | github.com/ares-emulator/ares LICENSE (ISC), ares/ps1 tree, ares/ps1/accuracy.hpp, releases | 9408cb43d494; v148 | ares licence, components, flags |
| ECO.17 | T1 | ares-emu.net/compatibility (PlayStation row) | accessed 2026-10-19 | 118/82/63/12/8,595 of 8,870 |
| ECO.18 | T1 | github.com/JaCzekanski/Avocado LICENSE (GPL-2), last commit | d7fcf9060ac7 (2023-09-19) | Avocado licence and status |
| ECO.19 | T1 | github.com/simias/rustation LICENSE and README; simias/rustation-ng LICENSE and README; simias/psx-guide (no licence) | 5e650b949d6e; aa1fb446e03a | GPL-2+, status, features |
| ECO.20 | T1 | drhell.web.fc2.com/ps1/index.html | accessed 2026-10-19 | XEBRA status |
| ECO.21 | T1 | epsxe.com | accessed 2026-10-19 | ePSXe 2.0.18, HLE BIOS, CHD |
| ECO.22 | T1 | problemkaputt.de/psx.htm | v2.3 2026-06-05 | no$psx features and licence |
| ECO.23 | T1 | github.com/TASEmulators/BizHawk LICENSE, README (Nymashock, Octoshock) | c9de9a8bff0f | BizHawk licence and PS1 cores |
| ECO.24 | T1 | tasvideos.org/Movies-PSX (259), EmulatorResources | accessed 2026-10-19 | publication count, titles, emulators used, accepted list |
| ECO.25 | T1 | tasvideos.org/Bizhawk/ReleaseHistory (Nymashock, Octoshock entries) | accessed 2026-10-19 | core coexistence, RA hash fix, bugs |
| ECO.26 | T1 | github.com/mamedev/mame COPYING and first-line headers of the psx device and driver files named in 1.1 | 96df6a708bdd | per-file licences |
| ECO.27 | T1 | GitHub search "psxact" (whaison, johnny-mac forks) | accessed 2026-10-19 | psxact status |
| ECO.28 | T1 | github.com/MiSTer-devel/PSX_MiSTer LICENSE (GPL-2), README "Status" | cd17b5a0ad26 | MiSTer licence and accuracy notes |
| ECO.29 | T1/T4 | emulatorjs.org docs; shop.zimaspace.com EmulatorJS requirements; PS1 Mate extension listing | accessed 2026-10-19 | browser core and difficulty |
| ECO.30 | T1 | github.com/JaCzekanski/ps1-tests LICENSE (MIT) | accessed 2026-10-19 | test suite licence |
| ECO.31 | T1 | github.com/SingleStepTests/r3000 README, licence MIT | pushed 2026-09-04 | r3000 SSTs provenance |
| ECO.32 | T1 | github.com/Lameguy64/PSn00bSDK LICENSE.md | master, accessed 2026-10-19 | MPL-2.0 core, GPL tools |
| ECO.33 | T4 | community.muos.dev/t/playstation-1-sound-issues/133 | accessed 2026-10-19 | muOS core, H700 fps, fixes |
| ECO.34 | T1 | onionui.github.io/docs/emulators | accessed 2026-10-19 | Onion ReARMed standalone |
| ECO.35 | T4 | heldgames.com, joelchrono.xyz, techtactician.com Miyoo Mini Plus reviews (search excerpts) | accessed 2026-10-19 | A7 1.2 GHz, PS1 playable, overclock |
| ECO.36 | T1 | emudeck.github.io (DuckStation PlayStation 1 pages); retrodeck.readthedocs.io emulator index | accessed 2026-10-19 | Steam Deck defaults |
| ECO.37 | T1 | docs.retroachievements.org/general/emulator-support-and-issues.html | accessed 2026-10-19 | supported PS1 emulators, DuckStation warning |
| ECO.38 | T1 | github.com/RetroAchievements/rcheevos include/rc_consoles.h, src/runtime/rc_consoleinfo.c | dfdc64ea035c | console id, memory regions |
| ECO.39 | T4 | beetle-psx-libretro issues 492, 240, 501, 610, 752, 789, 841, 895, 519, 929 | accessed 2026-10-19 | C.02, C.05, C.06, C.08, C.12 |
| ECO.40 | T2/T4 | libretro.com/?p=45256 (PGXP article); misterfpga.org viewtopic p=44888 | accessed 2026-10-19 | wobble explanation |
| ECO.41 | T4 | forums.libretro.com threads 24710, 22153, 15335, 11203, 15263, 26170 | accessed 2026-10-19 | audio crackling causes |
| ECO.42 | T4 | pcsx_rearmed issues 137, 170, 534, 336, 452, 573, 480, 845, 797, 808, 176, 180, 856 | accessed 2026-10-19 | C.05, C.07, C.08, C.11, G.06 |
| ECO.43 | T4 | forums.libretro.com/t/33891; RetroDECK known-issues wiki; wiki-next.recalbox.com PS1 memory cards guide | accessed 2026-10-19 | C.06 |
| ECO.44 | T4 | speedrun.com/posts/xtjpe and series posts (search excerpts; HTML bot-blocked) | accessed 2026-10-19 | accepted emulators and settings |
| ECO.45 | T2 | psx-spx.consoledev.net/graphicsprocessingunitgpu/ GPU Timings; /cpuspecifications/ | accessed 2026-10-19 | clocks, cycles per line, lines, rates, RAM access |
| ECO.46 | T1 | libretro.com/?p=48164 (PCSX ReARMed Lightrec benchmarks, i7-7700K) | accessed 2026-10-19 | interpreter vs dynarec fps |
| ECO.47 | T1/T4 | DuckStation seek/read speedup help text (via retrodeck mirror commit); speedrun.com posts kszqj, 2r6yf | accessed 2026-10-19 | C.15 |
| ECO.48 | T2/T4 | psx-spx.consoledev.net/cdromformat/ LibCrypt section; misterfpga.org p=94462; psx-place LibCrypt patcher thread | accessed 2026-10-19 | LibCrypt mechanism and counts |
| ECO.49 | T4 | feddit.it/post/11105187; gamingonlinux comments | accessed 2026-10-19 | community reaction |
| ECO.50 | T4 | search excerpts: DuckStation README amidog claim; hackclub devlog on closed GTE test; misterfpga demoscene thread | accessed 2026-10-19 | amidog tests |
| ECO.51 | T4 | cogmaster `SMS-ECOSYSTEM-AND-USERS.md` (process precedent only) and this pack's `PSX-PERIPHERALS-AND-SAVES.md` (v1.0/v2.0 peripheral placement) | updated 2026-10-18 / 2026-10-19 | horizon pattern |
| ECO.52 | T1 | speedrun.com/api/v1 platforms, games?platform=wxeod9rn, categories and leaderboards for the games named | accessed 2026-10-19 | counts, rules, run counts |
| ECO.53 | T1 | tasvideos.org/MovieRules; EmulatorResources tables | accessed 2026-10-19 | BIOS and region rules, accepted formats |
| ECO.54 | T1 | tasvideos.org/PlatformFramerates (PSX rows) | accessed 2026-10-19 | movie framerates |
| ECO.55 | T1 | github.com/Farama-Foundation/stable-retro docs/supported_emulators.md | master, accessed 2026-10-19 | no PS1 RL env |
| ECO.56 | T2 | psx-spx.consoledev.net/kernelbios/ (A/B function tables) | accessed 2026-10-19 | putchar, puts, printf numbers |
| ECO.57 | T1 | usenix.org ATC'19 Jangda et al. "Not So Fast" abstract | accessed 2026-10-19 | WASM 45 to 55 % slower, 2.08 to 2.5× peak |
| ECO.58 | T2 | en.wikipedia.org/wiki/Raspberry_Pi_4 | accessed 2026-10-19 | A72 ×4, 1.5 to 1.8 GHz |
| ECO.59 | T1 | github.com/libretro/libretro-database metadat/redump/Sony - PlayStation.dat (version 2026.10.07) | 12c21cf79f1e | entry counts per region, multi-disc, demos |
| ECO.60 | T1 | docs.retroachievements.org/developer-docs/game-identification.html | accessed 2026-10-19 | PS1 hash method |
| ECO.61 | T1 | github.com/PeterLemon/PSX (no licence file) | pushed 2026-01-20 | status |
| ECO.62 | T2 | psx-spx.consoledev.net/cdromdrive/ (75/150 sectors per second, seek settling, spin-down) | accessed 2026-10-19 | CD timing figures |
