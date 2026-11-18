# Feature Landscape: schwaystation v2.0 “The whole library”

**Domain:** PlayStation emulator library, runner and libretro core  
**Researched:** 2026-11-18  
**Overall confidence:** MEDIUM. Project contracts and libretro APIs are established. Some PBP variants, Jogcon semantics and cheat-code edge behavior remain underspecified.

## Scope and Evidence Boundaries

v2.0 extends the existing three deliverables through the current host boundaries. The core remains deterministic, integer-only and instance-owned. The host supplies BIOS and sectors; the core does not open files or operate physical input/audio/video devices. Format parsing belongs in the host disc-image helper. See [SEED-001](../seeds/SEED-001-whole-library.md), [DECISIONS.md](../preparation/DECISIONS.md) and [HOSTS-AND-INTEGRATION.md](../preparation/HOSTS-AND-INTEGRATION.md).

The compatibility index is the project’s pinned libretro Redump DAT snapshot: 13,407 entries at commit 12c21cf79f1e (2026-10-07), including 3,573 multi-disc entries across 568 titles. The DAT is an inventory, not proof that each disc boots or a mapping from games to console boards. Do not commit retail discs, BIOS, cheats or card saves. No owner hardware capture is available for this research; use public hardware tests and owned synthetic fixtures, and keep unmeasured revision claims qualified.

## Table Stakes

| Feature | Why expected | Testable expectation | Complexity |
|---------|--------------|----------------------|------------|
| CHD v5 | Common single-file compressed PS1 image format. | Load a small owned CHD fixture through the existing sector/TOC boundary. Compare sectors, audio and hashes with an equivalent supported image on all eight hash lanes. Bound malformed headers, codecs, hunks and LBAs. Keep libchdr optional behind SCHWAYSTATION_CHD and include its BSD-3 notice. | High. [libchdr](https://github.com/rtissera/libchdr) is a standalone BSD-3 reader for CHD v1–v5. Project seed requires a fixture under 512 KiB before enabling by default. [CHD media specification](https://psx-spx.consoledev.net/dev/ps1/cdr/cdromfileformats/diskimages/). |
| PBP | PSP/PSone libraries use PBP containers, including multi-disc collections. | Parse bounded offsets, TOC and compressed sectors; expose them through schway_disc. Compare decoded sectors against an owned equivalent image; reject truncated tables, bad offsets and inflate errors. | High. PS1 payload layout is reverse-engineered from homebrew; multi-disc/audio details are unresolved and retail EBOOT content may be PGD encrypted. Define supported variants. [PBP format](https://psx-spx.consoledev.net/dev/ps1/cdr/cdromfileformats/diskimages/). |
| ECM | Users may retain .bin.ecm images and expect direct loading. | Decode all sector types to equivalent raw-sector bytes. Test nonsequential LBAs, truncation, invalid varints and bounded output. Preserve random sector access with an index or bounded load-time decode. | Medium-high. ECM has no sector index; repeatedly scanning the full stream at each read is unacceptable. [ECM format](https://psx-spx.consoledev.net/dev/ps1/cdr/cdromfileformats/diskimages/). |
| CCD / IMG / SUB | CloneCD sets retain track layout and optional subchannel data. | Preserve track types, pregap/index locations, 2352-byte sectors and optional 96-byte P-W subchannels. Test missing/invalid companion files, descriptor errors, non-BCD values and path escapes. | Medium-high. If SUB is absent, retain existing synthesized-subchannel behavior. [CCD/IMG/SUB format](https://psx-spx.consoledev.net/dev/ps1/cdr/cdromfileformats/diskimages/). |
| SBI / LibCrypt | Protected PAL games rely on the controller seeing bad subchannel-Q sectors, not merely correct data reads. | Parse bounded sidecar entries by LBA. Listed sectors produce bad-Q-CRC behavior, so GetlocP repeats the previous valid position. Test record variants, malformed input, CD command behavior and protected-title ledger predicates with generated sectors. | High. SBI cannot store the Q CRC bits; the host must model them as invalid. About 100 PAL titles are known to use LibCrypt. [SBI specification](https://psx-spx.consoledev.net/dev/ps1/cdr/cdromfileformats/diskimages/), [LibCrypt notes](https://psx-spx.consoledev.net/cdromformat/). |
| PAL and board/GPU profiles | PAL titles and early/late console profiles differ in timing and device behavior. | Exercise every declared board: pu7, pu8_early, pu8_late, pu18, pu20, pu22, pu23, pm41, pm41_2; and each region selection path. Test documented GPU v0/v2/v2c/CXD9500, CD firmware and RAM-control differences. PAL uses PAL clocks and 314-line timing. Ledger rows record identity, selected region/profile, predicate and outcome. | High. Redump has disc metadata and region, not board assignment. Use profile fixtures and representative ledger titles; do not claim every disc ran on every board. [PSX-SYSTEM.md](../preparation/PSX-SYSTEM.md) §8, [GPU reference](https://psx-spx.consoledev.net/graphicsprocessingunitgpu/). |
| Per-game cards and import/export | Saves should follow a game, and existing card files should remain usable. | Use game serial as the per-game key and keep slots independent. Import raw 128 KiB cards and .gme, .mem, .psx wrappers by magic; pad supported short raw cards with 0xFF; export raw interoperable images. Prove a fresh runner process can read guest-written data. | Medium. Bound sizes and wrapper headers; treat guest directory/checksum fields as card data. [Card formats](https://psx-spx.consoledev.net/controllersandmemorycards/), [project policy](../preparation/PSX-PERIPHERALS-AND-SAVES.md). |
| libretro SaveRAM round trip | Frontends own persistent SaveRAM; an in-memory card loses progress at reload. | Expose card 1 as RETRO_MEMORY_SAVE_RAM; persist card 2 through the existing adapter path. Test guest write → host persistence → fresh load → guest read in-process and in one bounded hosted RetroArch case. Preserve .srm interoperability. | Medium. Keep persistent bytes outside save states. [libretro API](https://github.com/libretro/libretro-common/blob/master/include/libretro.h), [host contract](../preparation/HOSTS-AND-INTEGRATION.md) §1. |
| GameShark / Action Replay strings | Frontends and hackers expect supplied cheat codes to change guest memory. | Parse bounded strings through retro_cheat_set; support enable/disable and reset through retro_cheat_reset; apply deterministically at a frame boundary; cover writes, conditions, malformed codes and out-of-range addresses. | Medium-high. Code generations and signed/alignment details vary; name the supported matrix. [Code notes](https://psx-spx.consoledev.net/cheatdevices/), [libretro API](https://github.com/libretro/libretro-common/blob/master/include/libretro.h). |
| Runner --patch (PPF 1/2/3 and BPS) | Translation patches should be testable without rewriting the source image. | Apply through the host-side sector overlay. Compare patched sectors with owned expected bytes; reject bad headers, bounds and source CRCs; prove the input image stays unchanged; fuzz/replay invalid cases. | High. PPF targets raw track bytes; BPS patches a byte stream. Apply after image decoding and before the CD reader returns sectors. [PPF developer text](https://github.com/meunierd/ppf/blob/4f24f8c784fa/ppfdev/PPF3.txt), [project bounds](../preparation/PSX-PERIPHERALS-AND-SAVES.md) §§6–7. |
| Stable libretro .info | Frontends use the info file to describe suitability, extensions, firmware and features. | Once the project compatibility criterion passes, clear experimental status and keep metadata claims aligned with automated checks. Gate the metadata change on that criterion. | Low code cost; evidence-dependent. libretro defines is_experimental as suitability for general use. [Info example](https://github.com/libretro/libretro-super/blob/master/dist/info/00_example_libretro.info), [parser](https://github.com/libretro/RetroArch/blob/master/core_info.c). |

## Differentiators Within the Approved Scope

| Feature | Player value | Testable expectation | Complexity |
|---------|--------------|----------------------|------------|
| SCPH-1070 multitap | Four players per port, up to eight with two taps, and cards on the tap. | Implement both documented poll methods, 5A80 ID, 0xFFFF padding, empty-slot behavior, card addressing and per-port selection. Exercise both ports and 4/8-player fixtures. | Medium-high. [Multitap protocol](https://psx-spx.consoledev.net/controllersandmemorycards/), [libretro inputs](https://github.com/libretro/libretro-common/blob/master/include/libretro.h). |
| GunCon and Justifier | Light-gun games need screen position, trigger, offscreen aim and beam timing. | Map libretro lightgun inputs. GunCon uses output position/light; Justifier uses its distinct IRQ10/timer path. Test bright/dark/offscreen, PAL/NTSC and poll timing. | High. GunCon depends on GPU position/brightness; Justifier on GPU time and IRQ10. [GunCon/Justifier](https://psx-spx.consoledev.net/controllersandmemorycards/), [libretro lightgun](https://github.com/libretro/libretro-common/blob/master/include/libretro.h). |
| Mouse, neGcon and Jogcon | Enables mouse-driven and steering titles that do not work with a standard pad. | Map abstract host input to device IDs and deterministic SIO reports; cover button and raw motion/twist/dial bounds. | Medium. Keep response curves in the host. Jogcon dial semantics are partly undocumented; force feedback is outside this scope. [Controller protocols](https://psx-spx.consoledev.net/controllersandmemorycards/). |
| PocketStation storage | Preserves PocketStation game data written as memory-card files. | Round-trip ordinary 128 KiB card reads/writes and PocketStation file metadata through import/export and fresh-process tests using an owned fixture. | Medium. Storage only: ARM7, BIOS, LCD, RTC, buttons, IrDA and special command execution are separate system scope. [PocketStation](https://psx-spx.consoledev.net/pocketstation/), [project card scope](../preparation/PSX-PERIPHERALS-AND-SAVES.md) §2.8. |
| SEED-007 pipeline tune-up | Starts each later milestone with measured CI/quality and current docs. | Mandatory first phase: weekly triage digest, CI-efficiency report, quality.json, measured floors/ceilings, refreshed device budgets and policy.lessons_unique. Report slowest job, timeout evidence, flake signal, cache/parallelism decisions and lowest-value test rationale. Ratchets cover ir_per_frame.<fixture>, size.wasm, state.bytes and ci.critical_path_s; docs/pins/settings are checked for freshness. | Medium. Implement the seed’s artifacts only; no unrelated gates. [SEED-007](../seeds/SEED-007-milestone-tune-up.md), [engineering loops](../preparation/ENGINEERING.md) §§7–8, 12. |

## Anti-Features

| Anti-feature | Why avoid it | Do this instead |
|--------------|--------------|-----------------|
| Full PocketStation emulation | Scope is storage compatibility; an ARM handheld is separate. | Preserve game files through the ordinary card protocol. |
| File/device/window/audio code in the core | Breaks deterministic host boundary and embedding contract. | Parse media in the helper and pass sectors, BIOS and normalized input to the core. |
| Retail BIOS, disc, cheat database or save bytes in the repository | These are user-owned/copyrighted assets. | Use self-authored fixtures, public hardware results and local-only ledger runs. |
| Claim every Redump entry is playable because it parses | DAT is an inventory, not a compatibility oracle or board map. | Record per-entry predicates/outcomes; claim only tested behavior. |
| Unbounded media scans, decompression or patching | Hostile inputs can hang or exhaust the frontend. | Bound input size/work and return stable errors. |
| Adjacent formats, patch types, cheat UI/database or physical-device UI | Expands dependencies and host scope beyond the approved list. | Stay with CHD/PBP/ECM/CCD, SBI, PPF/BPS and the named peripherals. |

## Feature Dependencies

Disc readers → common sector/TOC/subchannel contract → SBI bad-Q handling → CD-ROM LibCrypt behavior → protected-title ledger rows.

SYSTEM.CNF/serial identification → per-game card key and region auto-selection → runner import/export and libretro SaveRAM → ledger identity and stable-core evidence.

SIO0 → multitap controller/card routing and mouse/neGcon/Jogcon reports. GPU position/brightness plus timers → GunCon and Justifier. Disc reader plus bounded patch parser → sector overlay → runner patch checks. Cheat callback plus parser and deterministic memory writes → GameShark. SEED-007 reports and ratchets → baseline for later phases and metadata claims.

## MVP Recommendation

Keep the approved scope intact and sequence by observable dependencies: (1) mandatory SEED-007 tune-up; (2) media readers and common SBI/LibCrypt behavior; (3) card import/export, per-game persistence and libretro round trip; (4) multitap and peripherals; (5) PAL/board/GPU profiles with ledger outcomes; (6) GameShark and --patch; then clear experimental status only after the compatibility criterion passes. Bounded negative cases belong beside each parser/device; owned fixtures prove behavior without retail assets.

## Executor Questions

These resolve acceptance details without changing scope or asking the owner.

- Does PBP acceptance include encrypted EBOOTs and PBP-contained multi-disc switching, or only unencrypted PS1-image variants with known layouts?
- For ECM random reads, use a compact load-time index or bounded decoded cache; do not rescan the full image for each read.
- For multi-disc playlists, does one serial-keyed card follow the playlist/title or each disc’s distinct serial?
- Define deterministic SBI lookup for CUE/CCD/CHD and multi-disc PBP, including explicit-path precedence.
- State the chosen Jogcon dial model; public docs mark direction/absolute-vs-relative and motor behavior uncertain.
- Name supported GameShark generations/types and compare/alignment rules; the public lists do not settle every edge case.
- Redump has no board assignment. Specify whether each board gets profile fixtures plus representative ledger titles, or every disc runs across a full board × region matrix.
- Set the numeric ledger-pass threshold for stable .info. Resolve “drops experimental metadata” versus the host-prep contract’s is_experimental = false: omit the key or set it false.
- SEED-001 also names MDEC edge cases and a fast boot option, absent from this task’s enumerated list and current PROJECT target list. Check active requirements before roadmap omission; any fast path must respect no-HLE/no-BIOS-bytes.

## Sources

| Source | Confidence | Supports |
|--------|------------|----------|
| [PSX disc-image specs](https://psx-spx.consoledev.net/dev/ps1/cdr/cdromfileformats/diskimages/) (project-pinned reference 6d7d1bc106a7) | MEDIUM; curated, partly reverse-engineered | CHD/PBP/ECM/CCD, SBI and bad-Q behavior. |
| [PSX controllers/memory cards](https://psx-spx.consoledev.net/controllersandmemorycards/) and [PocketStation](https://psx-spx.consoledev.net/pocketstation/) (pinned reference 6d7d1bc106a7) | MEDIUM; curated hardware reference | Device IDs/protocols, multitap, card layout and PocketStation. |
| [GPU](https://psx-spx.consoledev.net/graphicsprocessingunitgpu/) and [pinouts](https://psx-spx.consoledev.net/pinouts/) | MEDIUM; curated hardware reference | GPU generations, boards and region clocks. |
| [libchdr](https://github.com/rtissera/libchdr) (project pin 47d794151b2b) | HIGH for API/license | Optional CHD reader and BSD-3 license. |
| [libretro API header](https://github.com/libretro/libretro-common/blob/master/include/libretro.h) (project pin a3194c18ba65) | HIGH; upstream API | Device abstractions, SaveRAM and cheat callbacks. |
| [libretro info example](https://github.com/libretro/libretro-super/blob/master/dist/info/00_example_libretro.info), [RetroArch parser](https://github.com/libretro/RetroArch/blob/master/core_info.c), [core docs](https://docs.libretro.com/development/cores/developing-cores/) | HIGH; upstream contract | .info meaning and publication. |
| [Pinned Redump DAT](https://github.com/libretro/libretro-database/blob/12c21cf79f1e/metadat/redump/Sony%20-%20PlayStation.dat) (project pin 12c21cf79f1e) | HIGH for inventory only | Disc/serial inventory for ledger coverage. |
| [ps1-tests](https://github.com/JaCzekanski/ps1-tests) (MIT, project pin f727802fead1) | HIGH for published hardware-test results | Public evidence available without owner capture. |
| [PPF3 developer text](https://github.com/meunierd/ppf/blob/4f24f8c784fa/ppfdev/PPF3.txt), [patch bounds](../preparation/PSX-PERIPHERALS-AND-SAVES.md) §§6–7 | MEDIUM; format reference plus project contract | PPF parsing, patch limits and fuzz cases. |

---
*Feature research completed: 2026-11-18.*
