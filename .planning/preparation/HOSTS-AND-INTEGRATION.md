---
title: "Hosts and integration: libretro, RetroAchievements, RetroArch on Mac, runner + Playstead, automation, WASM"
summary: "What every host needs from schwaystation (libretro adapter for a disc console, rcheevos hash and memory rules for PS1, RetroArch on Apple Silicon, schwaystation-run and Playstead's protocol, scripting and movies, WASM, the C API every host shares) and which milestone each piece lands in."
read_when: "Building the libretro adapter or .info, the runner CLI, memory-card files, the RetroArch e2e job, a Playstead pin, scripting/Gym/TAS surfaces, the GDB or TTY seeds, or a web build."
updated: 2026-10-19
---

# Hosts and integration

Markers: (tested) run here on 2026-10-19, (derived), (untested), (unverified). "Lnnn" = line of libretro.h at RetroArch v1.22.2, the copy vendored in muhtari [HI.01]. Milestone labels follow muhtari's and cogmaster's packs (v1.0 core + runner, v2.0 saves + library breadth, v3.0 states + movies + continuation, v4.0 measured accuracy, v5.0 tools/AI, v6.0 ports); GSD-HANDOFF may renumber. Another console appears only as labelled precedent. Beetle PSX, PCSX-ReARMed, DuckStation and SwanStation are cited from their `.info` files and docs only; none of their source was opened (DuckStation's `LICENSE` at `124a4b16feda` is CC BY-NC-ND 4.0, the rest GPL) [HI.02][HI.06][HI.07].

## Where to look
| Question | Section |
|---|---|
| Extensions, `need_fullpath`, geometry, fps, pixel format, input map, rumble, memory cards, disc control, options, `.info`, firmware | 1 |
| rcheevos console id, PS1 hash rule, memory regions and descriptors, disallowed options, hardcore list | 2 |
| RetroArch Metal cask and DMG pin, directories, install one-liner, bounded hosted e2e, App Store limits | 3 |
| `schwaystation-run` options, exit table, `--probe`, mailbox/TTY protocol, save-dir layout, Playstead pin | 4 |
| Input scripts, lag frames, BK2 and PJM imports, Lua/Python/Gym, GDB stub, TTY | 5 |
| Emscripten build, browser disc and BIOS supply, hash lane | 6 |
| C API surface, other hosts, ABI versioning | 7 |
| Feature → host or core | 8 |

## 1. libretro contract
**Exports.** The 25 entry points (L7511 onward) in muhtari's 314-line adapter shape: one instance created in `retro_load_game`, every static reset in `retro_deinit`, the batch audio callback preferred, a failed frame sends nothing [HI.08]. `retro_get_region` 0/1 from the loaded profile (L443). `retro_reset` = the console Reset button (a warm reset through the BIOS; RAM content survives as on hardware). **Rec:** as stated. **Alt:** power cycle. **Would change if:** a game reads stale RAM after Reset in a way users notice.

**Content.** PS1 content is a disc image of up to 700 MB, so `need_fullpath = true` (L3896): the adapter opens the path through the library's disc reader and never loads the image into memory, the rule Beetle PSX and PCSX-ReARMed follow (`needs_fullpath = "true"` in both `.info` files) [HI.02]. `block_extract = true` (L3899): a zip holding a cue and its bins must not be extracted by the frontend into a temporary file set, so archives are refused with a message (`SET_MESSAGE_EXT` 60, L2073) (derived). `supports_no_game = "true"`: with a BIOS in the system directory and no content the core boots the BIOS shell; without one it shows the test card, which is the frame the hosted e2e compares (§3).

| Extension | v1.0 | Meaning and rule |
|---|---|---|
| `cue` | accept | descriptor; referenced files resolved relative to the cue; MODE1/2048, MODE2/2352, AUDIO tracks |
| `bin`, `img`, `iso` | accept | one raw track: sector size 2352 or 2048 sniffed from the ISO9660 volume descriptor at sector 16; a lone `bin` beside a cue is loaded as if through the cue (derived) |
| `chd` | refuse, v2.0 | "unsupported: CHD (planned v2.0)"; lands with libchdr (BSD-3, pin `47d794151b2b`) [HI.14] behind `SCHWAYSTATION_CHD` (ENGINEERING 7, DEC.43); CD-ROM hunks only |
| `m3u` | accept | one path per line, relative; drives the disc control interface |
| `exe`, `psexe` | accept | PS-X EXE header (`"PS-X EXE"`, 2 KB header, load address, size, PC, GP, SP) run from BIOS entry; homebrew path [HI.16] |
| `pbp` | refuse, v2.0 | PSP EBOOT container (DuckStation: unencrypted only) [HI.07] |
| `ecm` | refuse, v2.0 | needs ECM decoding; DuckStation: single-track only [HI.07] |
| `ccd`, `mds`, `toc`, `sbi` | refuse, v2.0 | subchannel-bearing formats, land with LibCrypt support |
| `psf` | never | a sound-player use; keep the core a console |

All five PS1 cores in libretro-core-info accept `cue`, `bin`, `chd`, `m3u` and `exe`; DuckStation/SwanStation add `psexe`, `iso`, `img`, `ecm`, `mds`, `psf`; Beetle adds `toc`, `ccd`, `pbp`; PCSX adds `mdf`, `cbn` [HI.02] (tested). A refused extension returns false from `retro_load_game` after one message naming the milestone. **Rec:** the seven accepted above (`chd` in v2.0). **Alt:** DuckStation's full list. **Would change if:** the owner's library needs `pbp` first.

| AV field | NTSC | PAL | Derivation |
|---|---|---|---|
| base w × h | 320 × 240 | 320 × 256 | Beetle and PCSX ship 320 × 240 as base [HI.06]; PAL games often use 256 active lines (derived) |
| max w × h | 640 × 480 | 640 × 576 | GPU display widths 256/320/368/512/640, heights up to 240 (480 interlaced) NTSC or 288 (576) PAL [HI.11]; Beetle at 1x reports 700 × 576 [HI.06] |
| fps | 53693175/897619 = 59.81733 | 53203425/1069484 = 49.74682 | the emulated rate: GPU clock over 263 × 3413 or 314 × 3406 clocks per progressive frame (ARCHITECTURE 1), so fps and samples per frame agree exactly and rate control sees no drift; the hardware nominal 59.826/49.761 (4.5 MHz/286 over 263 lines, 15625/314) stays documentary; interlaced field rates 1431818/23891 = 59.9313 and 2128137/42575 = 49.9856 [HI.11][HI.22] (derived, tested) |
| sample_rate | 44100 | 44100 | SPU native; 527,799,972/715,909 = 737.244 NTSC and 628,856,592/709,379 = 886.489 PAL samples per frame (derived) → 1,024-sample buffer; Beetle and PCSX report 44100 [HI.06][HI.22] |
| aspect_ratio | 4/3 | 4/3 | the display area fills a 4:3 raster at every width mode (derived); Beetle's options are corrected/uncorrected/4:3 [HI.06] |

**Rec:** fixed 4/3, geometry switched by `SET_GEOMETRY` (37, L1554) when the width or height mode changes, `SET_SYSTEM_AV_INFO` (32, L1365) only when the fps changes (region change or interlace toggle); report progressive rates like Beetle's `force_progressive` default [HI.06]. **Alt:** Beetle's "corrected" PAR. **Would change if:** players ask for the console's slight horizontal overscan.

**Pixel format.** `SET_PIXEL_FORMAT` (10, L865) XRGB8888. The GPU outputs 15-bit words, and in 24-bit display mode (MDEC video) packed RGB888; both convert losslessly into 8888, while RGB565 would drop the low bits of 24-bit frames and remap 15-bit green [HI.11] (derived). Native frames handed to hosts and hashed stay in the GPU's own encoding: 16-bit words for 15-bit mode, 3 bytes per pixel for 24-bit mode, with a per-frame tag. **Rec:** XRGB8888 only; no `USE_RGB565` build. **Alt:** RGB565 for handheld ports (v6.0). **Would change if:** a port's blitter needs 565.

**Frame delivery.** `retro_run` runs one frame to vblank and calls the video callback once; audio via the batch callback (L7465) for the SPU's samples of that frame. The frame-time callback (21, L1088) is not used: pacing is the frontend's audio clock; `GET_PERF_INTERFACE` (28, L1258) is not required [HI.01].

**Input** (`SET_INPUT_DESCRIPTORS` 11, L882; `SET_CONTROLLER_INFO` 35, L1506). The RetroPad map equals Beetle PSX's and PCSX-ReARMed's documented tables, so users' remaps carry over [HI.06]:

| RetroPad (L320–371) | PS1 | Notes |
|---|---|---|
| B 0 / A 8 / Y 1 / X 9 | Cross / Circle / Square / Triangle | Beetle: Cross = B, Circle = A, Square = Y, Triangle = X |
| L 10 / R 11 / L2 12 / R2 13 | L1 / R1 / L2 / R2 | |
| L3 14 / R3 15 | L3 / R3 | DualShock only |
| Select 2 / Start 3 / D-pad 4–7 | Select / Start / D-pad | `GET_INPUT_BITMASKS` (51, L1821), id 256 (L380) for one poll |
| `RETRO_DEVICE_ANALOG` 5 (L260): index LEFT 0 / RIGHT 1, id X 0 / Y 1 (L389–393) | left and right sticks, int16 → 0–255 | DualShock and Dual Analog |
| `GET_RUMBLE_INTERFACE` (23, L1143), `retro_rumble_interface` (L4925) | big motor 0–255 → STRONG 0–65535; small motor on/off → WEAK 0 or 65535 | Beetle rumbles only with the DualShock type [HI.06] |

Port subclasses (`RETRO_DEVICE_SUBCLASS`, L138), named as Beetle names them so RetroArch's per-core device remaps carry over [HI.06]:

| Subclass | Base device | Hardware | Lands |
|---|---|---|---|
| PlayStation Controller | JOYPAD 1 | SCPH-1080 digital pad | v1.0 |
| DualShock (default) | ANALOG 5 | SCPH-1200, two sticks, two motors | v1.0 |
| none | | empty port | v1.0 |
| Analog Controller, Analog Joystick | ANALOG 5 | SCPH-1180, SCPH-1110 | v2.0 |
| neGcon | ANALOG 5 | twist on left stick X with Beetle's response and deadzone options | v2.0 |
| Guncon / G-Con 45, Justifier | LIGHTGUN 4 | SCREEN_X/Y, TRIGGER, offscreen reload | v2.0 |
| Mouse | MOUSE 2 | SCPH-1030 | v2.0 |
| Multitap | option `schwaystation_multitap_port1/2` | eight descriptors as Beetle's "User 1–8" | v2.0 | **Rec:** DualShock default, booting in analog mode, with the Beetle toggle combination (Start+Select+L1+L2+R1+R2 held one second) behind `schwaystation_analog_toggle` [HI.06]. **Alt:** digital default. **Would change if:** games that refuse analog mode at boot outnumber those that need it in the compatibility ledger.

**Memory cards.** A card is 131,072 bytes (16 blocks of 8 KB, block 0 holding the header and directory frames); `.srm`/`.mcr`/`.mcd` files are the raw image, while DexDrive-style `.gme` files prepend an F40h-byte header and are an import converter's job (v5.0) [HI.12]. Beetle's `.srm` and Mednafen's `.mcr` are the same raw bytes; Beetle names slot 0 `<content>.srm` and slot 1 `<content>.1.mcr`, PCSX-ReARMed names slot 1 `pcsx-card2.mcd` shared by every game or `<serial>_1.mcd` by option, DuckStation (standalone) keeps per-game cards by title or serial [HI.06][HI.07]. **Rec (v1.0 saves in v2.0):** slot 0 = `RETRO_MEMORY_SAVE_RAM` (0, L509), 131,072 bytes, fixed from load to unload, so RetroArch writes `<content>.srm` and users keep every existing card by renaming; slot 1 = `<content>.1.mcr` written by the adapter in `GET_SAVE_DIRECTORY` (31, L1326), Beetle's exact layout, with an option for one shared card. `libretro_saves = "true"` only when the round trip is tested. **Alt:** both cards as files, SAVE_RAM empty. **Would change if:** RetroArch's save-on-exit proves lossy for a 128 KB span (it writes `.srm` on unload and at `autosave_interval` [HI.05]).

**Disc control.** `SET_DISK_CONTROL_EXT_INTERFACE` (58, L2032; struct L5455) from v1.0 for `m3u`: `set_initial_image`, `get_num_images`, `replace_image_index`, `get_image_path`, `get_image_label` (L5247–5409); eject = lid open as the CD-ROM controller reports it, insert = lid closed with a new TOC. `disk_control = "true"` in the `.info` as the four shipping cores declare [HI.02]. **Rec:** ext interface only (RetroArch 1.22.2 reports version ≥ 1). **Alt:** also the legacy interface 13. **Would change if:** a target frontend lacks the ext version.

**Core options** (`SET_CORE_OPTIONS_V2` 67, L2339; v1 fallback via 52, L1848). Keys mirror the shipping cores' names where one exists [HI.06]:

| Key | Values (default first) | In state hash | Lands |
|---|---|---|---|
| `schwaystation_region` | auto (disc licence string / serial prefix), ntsc-u, ntsc-j, pal (PCSX `region`) | yes | v1.0 |
| `schwaystation_bios` | auto (region's canonical file, then any known digest), then one row per file found (PCSX `bios`) | yes | v1.0 |
| `schwaystation_analog_mode`, `schwaystation_analog_toggle` | analog, digital; disabled, enabled (Beetle `analog_toggle`) | no | v1.0 |
| `schwaystation_memcard1` | per_game (`.1.mcr`), shared, none (Beetle `enable_memcard1`, `shared_memory_cards`) | no | v2.0 |
| `schwaystation_tty` | off, on: BIOS putchar text to the log interface (27, L1243) | no | v1.0 |
| `schwaystation_multitap_port1/2` | disabled, enabled | yes | v2.0 |
| `schwaystation_boot` | full, fast (skip the BIOS shell; Beetle `skip_bios` warns about Saga Frontier and LibCrypt titles [HI.06]) | yes | v2.0 |

No CPU, GTE or GPU overclock option by design: `cpu_freq_scale`, `gte_overclock`, `gpu_overclock` exist in Beetle and are rcheevos-restricted below 100 % [HI.03][HI.06]; schwaystation's clock is the profile's. No internal-resolution option: the core renders natively, upscaling is a later hardware-renderer host feature (§7).

**Serialize.** 0 and false until v3.0 (`savestate = "false"`); then a constant size per profile (2 MB RAM + 1 MB VRAM + 512 KB SPU RAM + 1 KB scratchpad + device state, about 3.6 MB plus the CD-ROM state) (derived), byte-wise little-endian, no `SET_SERIALIZATION_QUIRKS` (44, L1696). Rewind and run-ahead need determinism, netplay byte equality across platforms [HI.08].

**Firmware** (`GET_SYSTEM_DIRECTORY` 9, L850). libretro's `System.dat` lists 20 PS1 BIOS images of 524,288 bytes: `scph1000`, `scph1001`, `scph1002`, `scph100`, `scph101`, `scph102A/B/C`, `scph3000`, `scph3500`, `scph5000`, `scph5500`, `scph5501`, `scph5502`, `scph5552`, `scph7001`, `scph7002`, `scph7003`, `scph7502`, `scph9002(7502)`, plus `psxonpsp660.bin` and `ps1_rom.bin` [HI.04]. The canonical trio and their md5: `scph5500.bin` 8dd7d5296a650fac7319bce665a6a53c (JP), `scph5501.bin` 490f666e1afb15b7362b406ed1cea246 (US), `scph5502.bin` 32736f17079d0b2b7024407c39bd3050 (EU); Beetle and DuckStation declare them `opt = "false"`, PCSX `opt = "true"` with an HLE fallback; Beetle/SwanStation add `psxonpsp660.bin` c53ca5908936d412331790f4426c6c33 and `ps1_rom.bin` 81bbe60ba7a3d1cea1d48c14cbcc647b as optional; DuckStation's notes say it "also supports No-Intro BIOS images" [HI.02][HI.04]. **Rec:** three required rows plus two optional in the `.info`; the loader looks up the region's canonical name, then any 512 KiB file in the system directory whose SHA-256 is in the vendored System.dat list (name-independent), logs the match, and refuses an unknown image unless `schwaystation_bios` names it. No HLE BIOS (a PCSX feature with documented compatibility cost [HI.06]). **Alt:** names only. **Would change if:** users keep No-Intro names, which the digest lookup already covers.

**`.info`.** `display_name "Sony - PlayStation (schwaystation)"`, `corename "schwaystation"`, `systemname "PlayStation"`, `systemid "playstation"`, `database "Sony - PlayStation"`, `manufacturer "Sony"`, `supported_extensions "cue|bin|img|iso|m3u|exe|psexe"` (`chd` appended only by a build with `SCHWAYSTATION_CHD`, v2.0), `license "MIT"`, `needs_fullpath "true"`, `disk_control "true"`, `hw_render "false"`, `load_subsystem "false"`, `cheats "true"`, `display_version` from `version.txt`, generated from `schwaystation_libretro.info.in` as muhtari does [HI.02][HI.08]. Each true claim maps to a guard test at configure time:

| Claim → true | Lands | Guard test |
|---|---|---|
| `supports_no_game`, `input_descriptors`, `core_options` + `core_options_version "2.0"`, `disk_control` | v1.0 | in-process `libretro.host`: NULL game → test-card hash; m3u of two own images swaps and the TOC changes |
| `memory_descriptors` | v1.0 | vendored rcheevos resolver over our map (§2) |
| `libretro_saves` | v2.0 | `.srm` round trip in-process + hosted case 2 (§3) |
| `savestate`, `savestate_features` basic → serialized → deterministic | v3.0 | size constant; save+load every frame = straight run; six-platform state hashes |
| `is_experimental "false"` | v2.0 | compatibility-ledger pass count ≥ threshold |

**Buildbot (v6.0).** libretro ci-templates (pin `96f603ee450e`, from muhtari's pack, not re-fetched) build CMake cores with `-DLIBRETRO_STATIC=ON -DLIBRETRO_SUFFIX=_<platform>`; every global needs the `schway_`/`retro_` prefix; the Linux image is Ubuntu 16.04 + GCC 9 + CMake 3.31.7 [HI.09]. The `.info` PR goes to libretro-core-info (`5a74858ab2f7`); libretro-super's `dist/info` is where the files historically lived and still exists (`f0308e8b8765`) [HI.02].

## 2. RetroAchievements
rcheevos `rc_consoles.h` L27: `RC_CONSOLE_PLAYSTATION = 12` (tested at develop `dfdc64ea035c`, 2026-10-17; latest tag v12.5.0, 2026-09-14; LICENSE MIT) [HI.03]. The RA emulator list for PlayStation names Beetle PSX HW ("most recommended"), Beetle PSX, SwanStation and standalone DuckStation [HI.10]; unlisted cores may be excluded from hardcore, so ask RA in v6.0 once the buildbot ships the core.

**Hash** (`src/hash/rc_hash_disc.c` L868–990, `rc_hash_psx`): open track 1; read `SYSTEM.CNF`, parse `BOOT = cdrom:\<name>;1`, strip the prefix and backslashes, stop at whitespace or `;`; fall back to `PSX.EXE`; read the executable's first 32 bytes, and when they start with `PS-X EXE` take the size at header offset 28 plus 2,048 for the header; MD5 over the executable's **name** (as written in SYSTEM.CNF, without the `;1`) followed by the executable bytes, because engine-sharing titles differ only in their serial-named boot file [HI.03]. Extensions `cue`, `chd`, `m3u`, `iso` go through disc iterators that detect PS1 by SYSTEM.CNF; `bin` tries several consoles; `pbp` is PSP-only in the extension table (`rc_hash.c` L1224–1268) [HI.03]. A homebrew `exe` hashed as a file is not a disc (unverified which path rcheevos takes). `--probe` prints the same digest as `rcheevos_md5` (§4) from an own MD5, with a CTest that vendors `rc_hash_disc.c` and compares on the test discs.

**Memory** (`rc_consoleinfo.c` L803–810), rcheevos address → real address:

| rcheevos | Real | Type | schwaystation exposure |
|---|---|---|---|
| $000000–$00FFFF Kernel RAM | 0x00000000 | SYSTEM_RAM | first 64 KB of the 2 MB RAM |
| $010000–$1FFFFF System RAM | 0x00010000 | SYSTEM_RAM | rest of RAM; `RETRO_MEMORY_SYSTEM_RAM` (2, L517) = the whole 2 MB |
| $200000–$2003FF Scratchpad | 0x1F800000 | SYSTEM_RAM | 1 KB D-cache scratchpad |

The PS1 RAM sits at 0x00000000 (KUSEG) mirrored at 0x80000000 (KSEG0) and 0xA0000000 (KSEG1), the scratchpad at 0x1F800000 [HI.11]. With descriptors rcheevos resolves each real address through `SET_MEMORY_MAPS` (36, L1530; `retro_memory_descriptor` L3576) (`rc_libretro.c` L575–600); without them it walks the console regions pulling bytes from the matching `RETRO_MEMORY_*` id by offset and null-fills what is missing (L700–770), so a flat 2 MB `SYSTEM_RAM` alone would leave the scratchpad unmapped [HI.03]. Beetle and PCSX declare `memory_descriptors = "true"`, so sets were authored against resolved real addresses [HI.02]. **Rec:** descriptors from v1.0: RAM 2 MB at 0x00000000 with `RETRO_MEMDESC_SYSTEM_RAM` (L3505), the same pointer again at 0x80000000 and 0xA0000000, scratchpad 1 KB at 0x1F800000; `SYSTEM_RAM` export = RAM, `SAVE_RAM` = card 0; `SET_SUPPORT_ACHIEVEMENTS` (42, L1660) true in `retro_load_game`. **Alt:** flat exports only. **Would change if:** a set reads the scratchpad through the flat path in some core we must match.

**Disallowed options** (`rc_libretro.c` L32–38, L62–63, L124–126, L158–159): Beetle `cpu_freq_scale < 100`, DuckStation `CDROM.LoadImagePatches = true`, PCSX `psxclock` not auto and < 55 and `region = pal`, SwanStation `CPU_Overclock < 100` [HI.03]. schwaystation has no clock option; register `schwaystation_region = pal` (forced PAL timing) when the core is submitted, as PCSX did. Hardcore itself is enforced by RetroArch (no state loads, rewind, slow motion, cheats) [HI.10].

## 3. RetroArch on Apple Silicon
- **Install:** `brew install --cask retroarch-metal` is 1.22.2 with `sha256 :no_check` (tested, formulae.brew.sh and `brew info`), so Homebrew verifies nothing; the `retroarch` cask is the x86_64 DMG [HI.13]. v1.22.2 (2025-11-20) is still the newest GitHub release (tested) and the owner's machine runs it (`kMDItemVersion` 1.22.2) [HI.15]. **Pin:** `https://buildbot.libretro.com/stable/1.22.2/apple/osx/universal/RetroArch_Metal.dmg`, 232,801,022 bytes, SHA-256 `81b79121ba26d539064ae13b4d0419a120c3d165afbe656cf5f5412b15fdb434`, re-downloaded and hashed today, equal to cogmaster's, muhtari's and nesturbator's pins [HI.15][HI.08].
- **Directories:** cores in `~/Library/Application Support/RetroArch/cores`, info in `.../info`, BIOS files in `.../system` (the directory `GET_SYSTEM_DIRECTORY` returns); delete `info/core_info.cache` after an `.info` change; curl downloads carry no quarantine; the linker's ad-hoc signature suffices [HI.08] (tested: the directories exist on the owner's machine).
- **One-line install (cogmaster's pattern):** `curl -fsSL -O <release>/schwaystation-libretro-macos-arm64.zip -O <release>/SHA256SUMS && shasum -a 256 -c --ignore-missing SHA256SUMS && unzip -o schwaystation-libretro-macos-arm64.zip -d "$HOME/Library/Application Support/RetroArch"` where the zip holds `cores/schwaystation_libretro.dylib` and `info/schwaystation_libretro.info` (muhtari's archive layout) [HI.08]. BIOS files are the user's own and go into `system/`.
- **App Store build:** the sandboxed build ships only the cores libretro bundles ("cores we maintain in-house or whose upstream teams have given us explicit approval") and cannot add or update cores after installation, as libretro's iOS/tvOS guide states; the same signed-bundle rule is what keeps a sideloaded `.dylib` out of the Mac App Store build (unverified for macOS specifically) [HI.17]. The DMG is the target.

| Low-latency setting (cfg key) | Value | Why (untested with schwaystation) |
|---|---|---|
| video driver | Vulkan via MoltenVK or Metal | swapchain depth is a Vulkan/D3D knob [HI.05] |
| `video_max_swapchain_images` | 2 | one frame less queued [HI.05] |
| `video_frame_delay` | auto | a PS1 frame costs more than an 8-bit one; measure (`--bench`) before promising a margin |
| `vrr_runloop_enable` | on only with Adaptive-Sync | paces exactly 59.8173 / 49.7468 Hz |
| `audio_latency`, `audio_rate_control_delta` | 48 ms; 0.005 | 59.817 vs 60 Hz = 0.30 %, inside DRC's ±0.5 % (derived) |
| `run_ahead_frames` / `preemptive_frames_enable` | 1, preemptive | needs `savestate_features "deterministic"` (v3.0) and a state cost under a frame |

What RetroArch's CLI can do unattended (man page + `retroarch.c` option table at v1.22.2, cited through cogmaster's pack) [HI.05]:

| Flag | Use here | Note |
|---|---|---|
| `-L PATH` | load the built core | overrides the config's core |
| `-c PATH` / `--appendconfig A,B` | isolated config; Playstead appends `savefile_directory` and `system_directory` | later files win |
| `--max-frames=N`, `--max-frames-ss`, `--max-frames-ss-path=FILE` | run N frames, screenshot, exit | the only exit-after-N path |
| `--verbose` | log to stdout/stderr, captured by the driver | needed to diagnose a refused load or a missing BIOS |
| `--subsystem NAME` | not used; no PS1 core declares subsystems | `load_subsystem "false"` |
| content path | a cue or m3u (chd in v2.0); RetroArch passes the path unchanged because `need_fullpath` is true | archives refused by `block_extract` |

**Bounded hosted e2e** (RetroArch aborts inside the agent sandbox, so the hosted job is the evidence [HI.08]). muhtari's job, verified in its tree at `ecbe7f52aa37` (tested): one macos-15 arm64 job fetches the pinned DMG through `scripts/fetch-pin.sh` (SHA-256 checked before unpacking), `hdiutil attach -nobrowse -readonly`, `ditto` the one `.app`, asserts `CFBundleShortVersionString` = 1.22.2; a CMake `-P` driver (`run_retroarch.cmake`, 54 lines) creates a fresh run directory holding every directory the config names, writes an isolated config from `test.cfg.in` (46 lines: `config_save_on_exit=false`, `video_gpu_screenshot=false`, `audio_driver=null`, `pause_nonactive=false`, `menu_driver=rgui`, `bundle_assets_extract_enable=false`, all paths inside the run dir), sets `HOME`/`CFFIXED_USER_HOME` to it and runs `RetroArch --verbose --config test.cfg -L CORE --max-frames=5 --max-frames-ss --max-frames-ss-path=shot.png` with a 50 s timeout; `sips` converts PNG → BMP and a 54-line C compare (`bmp.c` 87 + `compare.c` 54) names the first differing pixel; a negative control flips one byte and expects exit 1; the log prints on failure [HI.08][HI.05]. Case 1 (v1.0): no content, no BIOS → test card; screenshot = `schwaystation-run --frames 5 --png 5:FILE`, byte-exact at 320 × 240. Case 2 (v2.0): an own PSn00bSDK-built EXE (MPL-2.0 notice on the binary) writes a boot counter to card 0 and draws it; run 1 shows 1, run 2 shows 2; compare both shots and the `.srm` bytes with the runner. BIOS-dependent cases stay local: CI has no BIOS bytes. Budget ≤ 300 test lines enforced by a line-count test; POSIX sh under `dash`; no stateful shell harness [HI.08]. **Rec:** two cases. **Alt:** in-process only. **Would change if:** a RetroArch update breaks loading unnoticed (add a version check, not a case).

## 4. Runner `schwaystation-run` and Playstead
Exit statuses: one X-macro table (`exit_status.h` as in muhtari), printed by `--help`, README equality test [HI.08]: **0** done (also after SIGTERM/SIGINT: finish the frame, flush cards, exit 0) · **1** failure (I/O, corrupt or hostile image, failure after fixture access) · **2** usage · **3** hash mismatch (`--hash`/`--expect`) · **4** save size mismatch · **5** state rejected · **6** fixture failure (mailbox status FFh) · **7** first fault (`--stop-on-fault`, CONFORMANCE 4) · **8** hang (frame cap reached with the mailbox still running) · **77** capability refusal (refused extension, BIOS required but absent or unknown, device not emulated) · **78** infrastructure error (harness, oracle or fetched digest broke; neither pass nor fail). CONFORMANCE 4 uses the same table.

| Option | Meaning | Lands |
|---|---|---|
| `--bios FILE` | required for any content; SHA-256 printed and matched against the System.dat list | v1.0 |
| `--disc FILE` / `--exe FILE` | cue/bin/img/iso/m3u (`chd` refused with exit 77 until v2.0), or a PS-X EXE run from the BIOS entry; none → test card | v1.0 |
| `--machine PROFILE`, `--region auto\|ntsc-u\|ntsc-j\|pal` | profile = revision + region (SCPH-1001, SCPH-5502, SCPH-101 ...) | v1.0 |
| `--frames N`, `--hash N`, `--hash-audio`, `--png N:FILE`, `--bench N` | SHA-256 of native pixels (15-bit words or 24-bit bytes plus the mode tag); PCM SHA-256 per channel; PNG via the 8888 conversion; fps + p50/p90/p99/p99.9/max µs | v1.0 |
| `--probe FILE` | fields below, no emulation; exit 0/77/1 | v1.0 |
| `--trace first-fault` | stop at the first unmapped or misaligned access or reserved instruction and print it | v1.0 |
| `--tty` | BIOS `putchar` (A(3Ch)/B(3Dh)) [HI.16] characters to stdout, lines prefixed `tty ` | v1.0 |
| `--stop-on mailbox` | end the run when the fixture mailbox reports (below) | v1.0 |
| `--inputs FILE` | per-frame input script (§5); prints the lag count | v1.0 |
| `--save-dir DIR`, `--save-interval F`, SIGUSR1 | card files below, temp → fsync → rename | v2.0 |
| `--expect FILE`, `--capabilities` | compare printed hashes; JSON, writes nothing | v1.0 / v3.0 |
| `--record FILE`, `--state-at-frame N:FILE`, `--load-state FILE`, `--hash-state N` | movies and states | v3.0 |
| `--trace full`, `--symbols FILE`, `--vram-dump N`, `--gdb PORT`, `--lua FILE` | §5 | v5.0 |

No window, config file, environment, stdin or network; invalid combinations exit 2 [HI.08]. **First-fault line:** `fault frame=N line=L pc=$XXXXXXXX op=$XXXXXXXX addr=$XXXXXXXX width=8|16|32 dev=<ram|scratch|bios|gpu|spu|cdrom|sio|timer|dma|irq|exp1|exp2|unmapped> kind=<read|write|exec|align|reserved>`; one line, then exit 0 unless `--expect` disagrees.

**`--probe` output** (one `key=value` per line): `format` (cue/bin/img/iso/m3u/exe; chd from v2.0), `tracks` and per track `track.N=<mode> <sector_size> <pregap> <lba> <file>`, `volume_id`, `system_cnf` (the BOOT line), `exe_name`, `exe_size`, `serial` (`SLUS-00001` form from the boot name, underscore and dot restored), `licence_area` (japan/europe/north_america from the executable's ASCII marker at 0x4C, or none) [HI.16], `region_guess` (serial prefix: SLUS/SCUS/LSP → ntsc-u, SLPS/SCPS/SLPM/SCPM → ntsc-j, SLES/SCES → pal; licence area breaks ties), `sha256` of the descriptor and of each track file, `sha256_tracks` over the concatenated raw sectors, `rcheevos_md5` (§2), `chd_sha1` from the CHD header (v2.0); for an EXE: header fields (PC, GP, load address, size, SP). The probe never emulates.

**Mailbox protocol** (CONFORMANCE 4 defines it; fixtures built with armips or PSn00bSDK): the first 256 bytes of every own fixture image at `t_addr` = 80010000h: magic `SWMB`, protocol version, a status byte (80h running, 81h waiting for input, 00h done, FFh fault), record count, fixture id, CRC-32 over 10h..FFh, an optional exception-handler address, then up to 30 records {test id u16, flags u16, value u32}; the runner reads it with `peek` once per frame and, with `--stop-on mailbox`, exits 0 at status 00h when every record matches `tests/diag/expected.csv`, 1 on a record outside its row, 6 on FFh, 8 at the frame cap, printing `id=value` lines. TTY text from the same fixture (`--tty`) carries the explanation; both are plain stdout, so a CTest reads them with `grep`. RAM at `t_addr` is used because the stub kernel keeps the scratchpad top for its header copy and exception record (PSXDEV-EXTRACTS PX.68) (derived).

**Save-dir layout** (`--save-dir DIR`): `DIR/<content basename>.srm` (card 0) and `DIR/<content basename>.1.mcr` (card 1), the RetroArch/Beetle names, so a directory moves between the runner and RetroArch unchanged; `--memcard-names serial` switches to `DIR/<serial>.srm` for libraries keyed by serial (multi-disc sets share one card that way, as DuckStation merges discs under one card [HI.07]). **Rec:** basename default. **Alt:** serial default. **Would change if:** Playstead keys its save artifacts by set, not by file.

**Playstead** (local checkout `f7a5c9ba293c`, accessed 2026-10-19) [HI.18]:

| Pin field | finding and `schwaystation-run` value | Lands |
|---|---|---|
| system | `psx` is in the frozen server registry (`system_id.ex`: `gba gb gbc nes snes md psx unknown`); the server maps `.cue` → `:psx` (`catalogue.ex` L37) and validates the cue structurally (`PsxCue`, Tier B), importing the cue plus its referenced bins as one ordered multi-file asset set (`import.ex` L565–600, IMPT-04); `.chd`, `.bin` alone, `.m3u` and `.exe` are not recognised today | Playstead |
| content extensions on the Mac client | `AdapterCatalog.swift` L43 derives `acceptedContentTypes` as `.<system>`, so the PS1 adapter would accept only `.psx` files: the client needs a per-system extension list (`cue`, `chd`, `m3u`) or a "descriptor set" launch path; `LaunchMaterializer.materialize` already copies every member of an asset set into `launch/<asset_set_id>/` with validated names, so `{romPath}` = the cue inside that directory works once the catalogue accepts it | Playstead (owner task) |
| emulator, version, download_url, sha256 | macos-arm64 runner zip from the attested release; digest from SHA256SUMS | v1.0 |
| launch.executable_relative_path, argument_template | `bin/schwaystation-run`; `["--save-dir","{saveDir}","--disc","{romPath}"]` (`AdapterPin.swift` substitutes only `{romPath}` and `{saveDir}`); the installer mounts a DMG and copies one `.app` (`AdapterInstaller.swift`), so a zip/CLI path is a Playstead change | Playstead |
| config_injection | `cli_flags`: `bios_path` → `--bios {path}` (the host appends the rendered template, `AdapterHost.swift` L275–285); `biosRequired` is hard-coded false in `AdapterDescriptor`, wrong for PS1 (no HLE), another owner task | v1.0 |
| save_contract | glob `{saveDir}/{romBaseName}.srm` (+ `.1.mcr`); medium `memcard_128k` 131,072; `size_is_medium_identity` true; `rom_sha256` binding: for a cue set bind to the descriptor's sha256 or the set manifest, since no single ROM file exists (open question 3); `SaveArtifactSet` already models multi-file artifacts ("memory cards" named in its comment) | v2.0 |
| flush triggers; on_demand_flush_supported | within `--save-interval` frames of a card write, SIGUSR1, SIGTERM/SIGINT, exit; true | v2.0 |
| worst_case_loss_seconds | interval ÷ 59.83 on SIGKILL; 0 on SIGTERM (flush inside the 2 s grace before SIGKILL) | v2.0 |
| exit_detection | clean 0/exit (also after SIGTERM); crash 6 or 11 uncaughtSignal; killed 9 | v2.0 |

Playstead today plays only through a GUI adapter (mGBA `.app`); the runner is headless, so play goes through the pinned RetroArch DMG plus the core zip and `-L <core> --appendconfig <cfg with savefile_directory={saveDir} and system_directory={biosDir}> {romPath}`: a two-artifact pin and a config writer, the Playstead-side change cogmaster also needs [HI.08][HI.18]. RetroArch on macOS installs no SIGTERM handler, so `autosave_interval` bounds the loss (from cogmaster's grep, untested) [HI.05]. Screenshots: Playstead has no screenshot path today; the runner's `--png` is the hook for a future library thumbnail. Continuation protocol (v3.0): the `qualify` / `initial-run` / `continue` actions as in cogmaster's pack, with the save dir holding two card files and `--hash M` as the visible-state oracle [HI.08].

## 5. Scripting and automation (YAGNI order)
| # | Surface | Design | Lands |
|---|---|---|---|
| 1 | TTY | BIOS `putchar` capture in the core (hook on A(3Ch)/B(3Dh) and the kernel's `std_out_putc`), runner `--tty`, libretro option → log; the cheapest homebrew test channel [HI.16] | v1.0 |
| 2 | Input scripts | text, one line per frame: `N p1=UDLRSsTOXQ1234<> lx,ly,rx,ry p2=...` with `.` for off, axes 0–255 (128 centre), `#` comments, `lag` column written by `--record`; absolute states, applied when the game polls the pad that frame | v1.0 |
| 3 | Inspection | `schway_peek` (bus view without side effects: no CD-ROM FIFO pops, no IRQ acks), CPU/GTE/GPU/SPU register getters, VRAM span, line and frame counters; property test: peeking all of RAM, VRAM and scratchpad leaves the state hash unchanged | v1.0–v3.0 |
| 4 | Movies | own versioned text format (header: disc hashes incl. `rcheevos_md5`, serial, profile, BIOS SHA-256, port devices, card digests at start, behaviour revision) + the frame lines above | v3.0 |
| 5 | Hooks | instruction, bus, DMA, IRQ, line and frame observers; the loop compiled twice, traced copy chosen at run entry; `schway_run(budget, &stop)` for breakpoints; Cachegrind proves hooks-off Ir unchanged | v5.0 |
| 6 | GDB stub (seed) | runner `--gdb PORT`: RSP over TCP, MIPS32 register layout, software breakpoints and memory read/write, for PSn00bSDK developers; precedent: pcsx-redux's stub (docs only) | v5.0 |
| 7 | Python, Gymnasium | ctypes over the C API; env obs native (320 × 240 16-bit) / rgb / ram (2 MB); 14 buttons + 4 axes; frameskip and sticky actions in the wrapper | v5.0 |
| 8 | Lua in the runner | `--lua FILE` on the hook API; Lua 5.4, MIT | on demand |

**Lag frame:** a frame in which no game code reads a controller through SIO0 (JOY_TX_DATA transfer to a pad on either port); peeks excluded (derived from the SIO protocol; BizHawk's Nymashock exposes a lag flag through its waterbox core, not inspected). **Sub-frame input. Rec:** per-frame input latched at vblank; `schway_input` size-tagged so a tick-stamped queue can follow. **Would change if:** a TAS needs mid-frame pad changes (games poll once per vblank by convention).

**Movie imports (v5.0, researched from specs and MIT C# only):**
- **BK2** (BizHawk; the only TASVideos-accepted PlayStation format, via Nymashock, a Mednafen port that is also the `[CoreConstructor(VSystemID.Raw.PSX)]` core; Octoshock is the older one) [HI.19][HI.20]: zip of `Header.txt`, `Input Log.txt`, `SyncSettings.json`, `Comments`, `Subtitles`, optional `CoreState`; PSX mnemonics `U D L R` for the D-pad, `Q T O` for Square/Triangle/Circle (`X` for Cross from the base table), `< >` stick buttons, `M` analog mode, `o c` open/close tray, axes `lsX lsY rsX rsY` as five-digit numbers (`Bk2MnemonicLookup.cs` L685–712, L1015–1027); the controller deck carries the "PSX Front Panel" with Reset, tray and disc select [HI.19].
- **PJM** (PSXjin; TASVideos lists it as a legacy format): 0x38-byte header (`"PJM "`, version 2, emulator version, flags: from power-on or savestate, NTSC/PAL, embedded cards, cheats, hacks, text/binary, multitap per port, analog hack), controller types 01 Mouse, 02 neGcon, 03 Konami Gun, 04 Standard, 05 Analog Joystick, 06 GunCon, 07 Analog Controller, offsets to the compressed savestate, two memory cards, cheat list, CD-ROM IDs (count + 9-byte id each) and controller data; 2 bytes per frame per standard pad, 6 per analog pad, plus one control byte (Reset, lid) [HI.21]. Movies starting from a state are refused.
- **DuckStation** has no movie format; **PCSX PXM** is unaccepted and undocumented enough to skip [HI.21].
**Rec:** own text format native, BK2 then PJM importers. **Would change if:** TASVideos accepts schwaystation (then BK2 native). Format interop is not sync: both importers re-time inputs against our frame boundaries and need the same BIOS (derived).

## 6. WASM (the hash lane from phase 1)
**Rec:** Emscripten build of the library + a ~200-line page as a seed: canvas 320 × 240 (resized on geometry change), AudioWorklet at 44.1 kHz, keyboard + Gamepad API. The browser supplies disc sectors through the File API: the user picks the cue and its bins (or one chd), the page keeps `File` handles and serves the core's sector callback with `Blob.slice(offset, offset + 2352)` reads, synchronously through a prefetch ring (the core requests sectors ahead through the same callback the CD-ROM model uses for seeks) (derived); the BIOS is a user-picked 512 KiB file read once into WASM memory and never uploaded. Memory: 2 MB RAM + 1 MB VRAM + 512 KB SPU RAM + 512 KB BIOS + sector ring ≈ 5 MB plus the frame buffer; a 64 MB initial heap, no threads (no SharedArrayBuffer, no COOP/COEP), no files, allocator hook, no mutable globals; check 32-bit `size_t` in state-size math [HI.08]. Hash lane: a WASI build of `schwaystation-run` under wasmtime runs the same fixtures (EXE, mailbox, `--hash`) as the native lanes in muhtari's `hash-equality` job, from phase 1, with no BIOS (test card and EXE-from-entry fixtures) [HI.08]. **Alt:** RetroArch's web player via `emscripten-static-cmake` (GPL-3 RetroArch, `${CORENAME}_libretro.{wasm,js}`) [HI.09]. **Would change if:** the owner wants shaders and menus on the web.

## 7. Multi-frontend ergonomics: the C API every host shares
| Group | Surface (all size-tagged structs, `schway_status` returns) | Used by |
|---|---|---|
| Lifecycle | `schway_get_versions`, `schway_create(cfg, &inst)` (allocator hook, profile, RAM fill), `schway_destroy` | all |
| Content | `schway_load_bios(bytes, n)`, `schway_open_disc(reader)` where `reader` is a sector callback + TOC (file reader provided in the library, browser reader in JS), `schway_load_exe(bytes, n)`, `schway_set_lid(open)`, `schway_swap_disc(reader)`, `schway_probe(reader, &info)` | runner, libretro, WASM, Playstead adapter |
| Run | `schway_run_frame(inst, &io)` with `io.video` (native span + mode tag + geometry), `io.audio` (int16 stereo, capacity, count), `io.input[2]` (device type, buttons, axes), `io.rumble[2]` out; `schway_run(inst, budget, &stop)` returns one stop reason from the single enum `FRAME`, `BUDGET`, `AUDIO_FULL`, `HOOK` (breakpoint, mailbox, tty line, exit port), `FAULT`, `LID` (ARCHITECTURE 1, ENGINEERING 14) | all; debuggers |
| Memory cards | `schway_memcard_span(slot)` (131,072 B, stable until destroy), `schway_memcard_generation(slot)` (bumps on write, drives flush timing) | libretro SAVE_RAM, runner, Playstead |
| Inspection | `schway_peek`, register getters, VRAM span, counters, lag flag; `schway_state_visit(visitor)` (v3.0) serialises through one visitor so save/load/hash/size share code | runner, Python, GDB |
| Hooks | observer registration (§5), zero cost when unset | v5.0 |

Hosts: `examples/sdl2/` (one file, ~300 lines: window, audio queue, game controller → DualShock, drag-and-drop cue, F-keys for lid and swap) proves the API outside libretro; a Playstead adapter later wraps the same calls behind its pin; the Python binding is ctypes over the same header. **ABI:** `SCHWAYSTATION_ABI_VERSION` integer in the header and in `schway_get_versions`; every struct starts with `uint32_t size` (muhtari's rule) so fields append without breaking older hosts; public symbols prefixed `schway_`, internals `schway__<module>_` (libretro option keys, file names and CMake names keep `schwaystation`; `abi.symbols` checks both), exported through a visibility map; behaviour revision (`behaviour_version`) separate from the ABI so hash-affecting fixes are visible to movies and Playstead's `core_version` provenance field [HI.08][HI.18].

## 8. Feature → host or core
| Feature | Owner | Core provides |
|---|---|---|
| 15-bit and 24-bit → XRGB8888 conversion | adapter (outside hashes) | native GPU words plus a mode tag per frame |
| Aspect, overscan crop, widescreen hacks, internal resolution, texture filtering, PGXP | not offered in v1.0; a hardware-renderer host feature later (Beetle HW/DuckStation territory [HI.06][HI.07]) | native 1x frames, display-area geometry |
| CRT shaders, bezels, screenshots, recording | RetroArch | nothing |
| Memory cards | libretro SAVE_RAM + `.1.mcr`; runner writes `.srm`/`.1.mcr`; Playstead captures them | two 128 KB spans with write generations |
| Disc swap, lid, m3u | RetroArch disk control; runner `--inputs` control lines; SDL example F-keys | lid state, TOC reload, swap API |
| Controller types, remaps, turbo, rumble | RetroArch; adapter maps RetroPad and analog → pad protocol | per-port device model, motor values out |
| BIOS lookup and digest check | adapter and runner (system dir, `--bios`) | `load_bios` with digest report |
| Cheats (GameShark style) | RetroArch cheat engine on `SYSTEM_RAM` | memory exports and descriptors; `retro_cheat_set` no-op in v1.0 |
| Rewind, run-ahead, netplay | RetroArch | fixed-size deterministic LE state (v3.0) |
| Achievements | RetroArch (rcheevos) | descriptors + `SYSTEM_RAM`/`SAVE_RAM` exports; disc hash in `--probe` |
| Region, BIOS and video timing | core profile + options | profile table, `retro_get_region` |
| TTY, mailbox, lag counter, trace, first fault | core (zero cost off) + runner | inspection and hooks |
| Fast boot | core option (v2.0), off by default | BIOS shell skip at the kernel hand-off |

## Open questions
1. rcheevos and homebrew `exe` content: which hash path a bare PS-X EXE takes through RetroArch (file MD5 or `rc_hash_psx` failure); one run with rcheevos' `rhash` CLI once a fixture EXE exists.
2. Interlaced timing report: keep progressive fps and let 480i games run at the progressive 59.817 (Beetle's default shape) or toggle `SET_SYSTEM_AV_INFO` to the field rate 1431818/23891; measure audio drift in RetroArch's statistics with one 480i game.
3. Playstead multi-file binding: which digest `rom_sha256` carries for a cue set (descriptor sha256, set manifest, or serial); owner decision before the save contract is written.
4. Playstead's `.psx`-only extension rule and `biosRequired = false`: both need a client change before a PS1 pin can qualify (owner tasks); CHD import is a server-side format addition.
5. RetroArch's SAVE_RAM write-on-exit with a 128 KB span and `autosave_interval`: one hosted run with case 2 confirms no partial writes.
6. Mac App Store build: confirm by reading the listing that sideloaded cores are refused (today's evidence is the iOS guide).

## Sources
| ID | Tier | Source (URL) | Pin or access date | Supports |
|---|---|---|---|---|
| HI.01 | T1 | github.com/libretro/RetroArch `libretro-common/include/libretro.h` (7,846 lines), read from muhtari's vendored copy `libretro/libretro.h` | `69a4f0ea1e8a` (v1.22.2) | entry points, env ids 9/10/11/21/23/27/28/31/32/35/36/37/42/44/51/52/58/60/67, devices L138–393, memory ids L509–520, memdesc L3505–3576, rumble L4925, disk control L5247–5455, system_info L3893–3899 |
| HI.02 | T1 | github.com/libretro/libretro-core-info `mednafen_psx_libretro.info`, `mednafen_psx_hw_libretro.info`, `duckstation_libretro.info`, `pcsx_rearmed_libretro.info`, `swanstation_libretro.info`; libretro-super `dist/info` listing | `5a74858ab2f7` (2026-09-16); libretro-super `f0308e8b8765` | extensions, systemid, needs_fullpath, disk_control, firmware rows and md5, savestate_features, memory_descriptors (tested) |
| HI.03 | T1 | github.com/RetroAchievements/rcheevos `include/rc_consoles.h` L27, `src/hash/rc_hash_disc.c` L868–990, `src/hash/rc_hash.c` L1224–1268, `src/runtime/rc_consoleinfo.c` L803–810, `src/libretro/rc_libretro.c` L32–38, L62–63, L124–126, L158–159, L497–511, L575–600, L700–770, `LICENSE` (MIT) | `dfdc64ea035c` (develop, 2026-10-17); latest tag v12.5.0 (2026-09-14) | console id, hash rule, memory regions, resolver, disallowed options (tested) |
| HI.04 | T1 | github.com/libretro/libretro-database `dat/System.dat` L497–519 | `12c21cf79f1e` | 20 PS1 BIOS names, sizes, crc/md5/sha1 (tested) |
| HI.05 | T1 | RetroArch `retroarch.cfg`, `docs/retroarch.6`, `retroarch.c` L1013, L7004–7006, `frontend/drivers/platform_darwin.m` (no signal handler), cited through cogmaster's pack | `69a4f0ea1e8a` | settings, CLI flags, SIGTERM behaviour (not re-fetched) |
| HI.06 | T1 | docs.libretro.com `library/beetle_psx/` (saves, geometry, options, RetroPad tables, rumble, multitap), `library/pcsx_rearmed/` (BIOS, saves, geometry, options); `library/duckstation/` and `library/swanstation/` return 404 | accessed 2026-10-19 | memcard layouts, RetroPad map, fps and sample rate, option keys, HLE caveat (tested) |
| HI.07 | T1 | github.com/stenzek/duckstation `README.md` and `LICENSE` (docs only; no source opened) | `124a4b16feda` (2026-10-19) | formats (CHD, ECM, MDS/MDF, CCD, unencrypted PBP, PSF), per-game and merged memory cards, rumble, multitap, licence CC BY-NC-ND 4.0 |
| HI.08 | T1 | muhtari `libretro/libretro.c` (314 lines), `libretro/muhtari_libretro.info.in`, `runner/{main.c,exit_status.h}`, `include/muhtari.h`, `.github/workflows/ci.yml` L259–380, `tests/retroarch/{run_retroarch.cmake,test.cfg.in,bmp.c,compare.c}` (241 lines), `scripts/fetch-pin.sh`; cogmaster `.planning/preparation/HOSTS-AND-INTEGRATION.md` (`c74db8374010`) | local checkouts `ecbe7f52aa37` and `c74db8374010`, accessed 2026-10-19 | adapter shape, exit table, e2e method, install pattern, WASM constraints, continuation protocol, sandbox abort |
| HI.09 | T1 | git.libretro.com/libretro-infrastructure/ci-templates; libretro-build-amd64-ubuntu `Dockerfile.xenial-gcc9` | `96f603ee450e`, `869fce5ac223` (from muhtari's pack, not re-fetched) | buildbot contract, toolchain floor |
| HI.10 | T1 | docs.retroachievements.org `general/emulator-support-and-issues.html`, `general/hardcore-compliance-requirements.html` | accessed 2026-10-19 | listed PS1 emulators, hardcore rules |
| HI.11 | T2 | psx-spx.consoledev.net `graphicsprocessingunitgpu/` (GPU Timings: clocks 53.693175/53.203425 MHz, 263/314 lines, 3413/3406 cycles, listed 59.826/49.761 Hz, dot clocks and widths, 15/24-bit display), `memorymap/` (RAM at 0/8000/A000 0000h, scratchpad 1F800000h) | accessed 2026-10-19 | timing, geometry, pixel encoding, memory map |
| HI.12 | T2 | psx-spx.consoledev.net `controllersandmemorycards/` (Memory Card Data Format: 128 KB = 131072 bytes, 16 blocks of 8 KB, block 0 = header and directory frames; raw images without header; `.gme`-style images carry an F40h-byte header) | accessed 2026-10-19 | card size, block layout, raw vs headered image files (tested) |
| HI.13 | T2 | formulae.brew.sh/api/cask/retroarch-metal.json; `brew info --cask retroarch-metal` | accessed 2026-10-19 | cask 1.22.2, `no_check`, DMG URL (tested) |
| HI.14 | T1 | github.com/rtissera/libchdr `LICENSE.txt` (BSD-3) | `47d794151b2b` (2026-10-11) | CHD reader licence and pin |
| HI.15 | T1 | Local measurement: DMG download + `shasum -a 256` + `stat`; `mdls` of the installed app; GitHub releases listing; `~/Library/Application Support/RetroArch` listing | 2026-10-19 | DMG digest and size, newest release, directories (tested) |
| HI.16 | T2 | psx-spx.consoledev.net `cdromfileformats/` (SYSTEM.CNF, boot name convention, PS-X EXE header, licence-area marker), `kernelbios/` (A(3Ch)/B(3Dh) putchar, std_out_putc) | accessed 2026-10-19 | probe fields, EXE loading, TTY channel |
| HI.17 | T1 | docs.libretro.com `guides/install-ios/`; apps.apple.com RetroArch listing (id6499539433) | accessed 2026-10-19 | signed-bundle core rule (iOS/tvOS stated; macOS unverified) |
| HI.18 | T1 | playstead `playstead-server/lib/playstead/formats/system_id.ex`, `catalogue.ex` L30–40, `formats.ex` L75–90, `import.ex` L565–600, `formats/validators/psx_cue.ex`; `playstead-mac/Playstead/Adapter/{AdapterPin.json,AdapterCatalog.swift L30–50,AdapterHost.swift L275–285,AdapterPin.swift,AdapterInstaller.swift}`, `Cache/LaunchMaterializer.swift` L85–120, `Saves/SaveArtifactSet.swift`; `.planning/PROJECT.md`, `README.md` | local checkout `f7a5c9ba293c`, accessed 2026-10-19 | system ids, cue recognition, multi-file sets, extension rule, launch and BIOS injection, save artifacts |
| HI.19 | T1/T3 | github.com/TASEmulators/BizHawk `src/BizHawk.Emulation.Cores/Consoles/Sony/PSX/Nymashock.cs`, `src/BizHawk.Emulation.Common/Base Implementations/Bk2MnemonicLookup.cs` L685–712, L1015–1027; `LICENSE` (C# parts MIT) | `c9de9a8bff0f` (2026-10-15) | PSX core, mnemonics, axes |
| HI.20 | T1 | tasvideos.org `EmulatorResources` (PlayStation: BizHawk BK2; legacy PCSX PXM, PSXjin PJM) | accessed 2026-10-19 | accepted formats |
| HI.21 | T1 | tasvideos.org `EmulatorResources/PSXjin/PJM` | accessed 2026-10-19 | PJM header, controller types, bytes per frame |
| HI.22 | T1 | Arithmetic run here (Python `fractions`): 53693175/(263·3413), 53203425/(314·3406), the field rationals, 33868800 cycles per frame over 768 for samples per frame; nominal 4500000/(286·263) and 15625/314 kept for comparison | 2026-10-19 | fps rationals, buffer sizing (tested) |
