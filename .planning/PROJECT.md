# schwaystation

## What This Is

A Sony PlayStation (PS1) emulator core written in C17: a small MIT-licensed library, a headless runner
(`schwaystation-run`) for tests, automation and frontends such as Playstead, and a libretro core for RetroArch. One
core covers every console revision from the SCPH-1000 to the SCPH-102 PS one and the NTSC-J, NTSC-U/C and PAL
regions as machine profiles. The v1.0 milestone delivers a deterministic CPU, GPU, SPU and CD path, host-supplied
disc sectors, the runner and libretro host, and an owned disc that boots through OpenBIOS in RetroArch.

## Core Value

PlayStation games behave as they do on the console, identically on every platform, from a small MIT-licensed C
library that any host can embed.

## Current State

The v1.0 GSD milestone completed on 2026-11-18 across six phases and 195 plans. Phase releases through the disc
and FMV work are merged; the automatic release workflow published v0.6.0. The library, runner and libretro core
build and pass the hosted platform, deterministic hash, RetroArch and media checks recorded in the milestone
archive. This is a completed v1.0 project milestone, not a claim of full PlayStation library compatibility or
complete hardware conformance. In particular, exact 8-bit MDEC output and complete external frame/movie output
remain unproven under the accepted v1.0 scope. The detailed discrepancy and evidence are in the [milestone audit](.planning/milestones/v1.0-MILESTONE-AUDIT.md).

Milestone v2.0, “The whole library,” is now active. It expands disc-format and LibCrypt support, peripherals and
save compatibility, region and revision coverage, ROM-hacker patches and cheats, and the compatibility ledger.
The required milestone tune-up leads the phase sequence. The v1.0 limits in the [milestone audit](.planning/milestones/v1.0-MILESTONE-AUDIT.md)
remain explicit; v2.0 does not claim the unresolved MDEC or full-frame/movie hardware conformance.

## Current Milestone: v2.0 The whole library

**Goal:** Make more of the PlayStation library playable through common disc formats, protection data, peripherals,
save workflows, machine profiles, and ROM-hacker support.

**Target features:**
- CHD, PBP, ECM and CCD media, plus SBI/LibCrypt support behind the existing host-supplied disc boundary.
- Multitap, light guns, mouse, neGcon, Jogcon and PocketStation storage behavior.
- Per-game memory cards, card import/export formats and the libretro save round trip.
- PAL timing and console/GPU revision profiles across the compatibility ledger.
- GameShark strings, runner patch support, broader ledger coverage and removal of the experimental metadata only when supported.
- A first-phase pipeline and documentation tune-up, including the triage digest, CI-efficiency report and measured `quality.json` ratchets.

Save states, movies, browser UI, measured-accuracy campaigns, JITs and visual enhancements remain outside v2.0.

## Requirements

### Validated

- ✓ Phase 1: release pipeline, deterministic test frame, library API, runner and RetroArch integration — v1.0.
- ✓ Phase 2: R3000A, COP0, bus, memory, I-cache, interrupts, PS-EXE loading and diagnostics — v1.0.
- ✓ Phase 3: GTE public hardware cases, DMA channels and root counters — v1.0.
- ✓ Phase 4: GPU, display modes, pads, memory cards, OpenBIOS and compatibility ledger — v1.0.
- ✓ Phase 5: deterministic SPU audio, pinned memory-transfer logs and runner/libretro PCM parity — v1.0.
- ✓ Phase 6: CD-ROM images, XA, CD-DA, MDEC within the approved evidence scope, owned disc boot and disk control — v1.0.

### Active

- v2.0: The whole library — scoped in `.planning/REQUIREMENTS.md` and mapped in `.planning/ROADMAP.md`.

### Out of Scope

- A window, audio device or input device; hosts own presentation; RetroArch is the frontend.
- Shaders, menus, netplay UI; frontend features; the core exposes what they need.
- Any Sony BIOS or game bytes; copyright; the user's own dumps stay on the user's machine (`ASSET_POLICY.md`).
- GPL, LGPL, non-commercial or unlicensed code; MIT licence (`PROVENANCE.md`).
- Enhancement modes in the hashed path (internal resolution, PGXP, widescreen, texture filtering, true colour) and any overclock or CD-speed hack that changes timing; the native frame is the product.
- Signed macOS builds; a Developer ID certificate puts a legal name in artifacts.
- Later milestones: states and movies, measured accuracy, tools, ports with a cached interpreter and JIT, enhancements and the browser page remain seeds in `.planning/seeds/` and horizons in `.planning/PRODUCT-ROADMAP.md`.

## Context

- **Ecosystem.** DuckStation (CC BY-NC-ND 4.0, forks and packaging not permitted) sits under Steam Deck
  distributions; SwanStation (GPL-3), Mednafen and Beetle PSX (GPL-2) and PCSX-ReARMed (GPL-2) are the libretro
  and handheld defaults; PCSX-Redux's core is GPL-2 while its nugget submodule with OpenBIOS is MIT; ares (ISC) is
  the permissive reference but a whole-system emulator with an experimental PS1 core; MAME's BSD-3 device files sit
  inside a GPL driver. No permissive, embeddable, integer-only, deterministic PS1 core with a measured ledger and
  first-class tooling exists (`.planning/preparation/PSX-ECOSYSTEM-AND-USERS.md`).
- **Sibling projects.** cogmaster (Master System and Game Gear) and muhtari (Atari 7800) are the direct template
  and seed for the delivery skeleton: runner, libretro adapter, state visitor, tests scaffolding, CI, hygiene and
  release pipeline; nesturbator (NES), gabbaboy (Game Boy), glueyneo (Neo Geo), genewhip (Genesis) and Playstead
  (frontend) supplied the lessons in `.planning/LESSONS.md`.
- **How releases are tried.** The owner installs the libretro core into RetroArch (Metal build) on an Apple
  Silicon Mac with the README's one line and plays a disc dump from their own collection with their own BIOS.
- **Reference material** (`.planning/preparation/`): `PSX-SYSTEM.md` (clocks, CPU, memory map, DMA, timers,
  interrupts, BIOS, regions, power-on), `PSX-GPU-AND-GTE.md` (GPU commands, rasterisation, timing, GTE, frame
  format, enhancement seams), `PSX-SPU-CDROM-AND-MDEC.md` (SPU, CD-ROM controller, disc interface, MDEC, audio
  contract), `PSX-PERIPHERALS-AND-SAVES.md` (SIO0, pads, memory cards, states, identity, hostile bounds),
  `PSX-ECOSYSTEM-AND-USERS.md` (emulators, complaints, personas, budgets, library counts), `PSXDEV-EXTRACTS.md`
  (test suites, toolchain pins, OpenBIOS, stub kernel), `ARCHITECTURE.md`, `CONFORMANCE.md`,
  `HOSTS-AND-INTEGRATION.md`, `ENGINEERING.md`, `DECISIONS.md`, `BRIEF.md`; plus `PROVENANCE.md` and
  `ASSET_POLICY.md` at the root.

## Constraints

- **Deliverables**: library, runner, libretro core; nothing else in this repository (an `examples/sdl2/` host is an example, never a deliverable).
- **Language**: C17 without extensions; the core links only the C memory functions; the header is valid C++.
- **Reuse**: permissive seeds only, through `PROVENANCE.md`; GPL, LGPL, non-commercial and unlicensed source is never opened; reference emulators run only as released binaries.
- **Determinism**: integer-only, no `float` or `double` anywhere, no mutable globals, no host clock, files or environment; identical frame and audio hashes on six platforms plus the `be32` and `wasm32` lanes.
- **Efficiency**: handhelds and wasm32 are targets; a Cachegrind instructions-per-frame ratchet from phase 1; zero allocations after load; no busy loops.
- **Legal**: no Sony BIOS, HLE kernel, licence file or game bytes; fixtures are own armips assembly and mkpsxiso discs, or MIT/MPL-noticed builds; OpenBIOS is the CI stand-in and a retail BIOS runs only on the owner's machine.
- **Privacy**: public repository; no personal paths, names, disc names or e-mail addresses; noreply commit identity; hygiene checks every commit.
- **Verification**: automation-first; the owner is asked only for identity-bound consent or a run on their own hardware.
- **Delivery**: each phase is one branch and one pull request into a green `main`, released automatically; the release pull request is never held.
- **Process**: autonomous milestone work is authorized. Run remaining phases through verification one at a time with `$gsd-autonomous --from N --to N`, then `$gsd-ship N` and confirm merge, green main CI and the automatic release before advancing. After the final phase release, audit, complete and clean up the milestone. Do not start `$gsd-new-milestone` until the release is published, main CI is green and the working tree is clean. See `.planning/AUTONOMOUS.md`; no invented ceremony (`AGENTS.md` rules 8 and 9).

## Key Decisions

| Decision | Rationale | Outcome |
|---|---|---|
| One core; console revisions and regions as machine profiles (`region`, `board` with derived GPU, SPU and CD firmware); v2 GPU and vC2 CD firmware default | Hardware differs by board; tests need fixed values; the region decides the video clock and the SCEx answer | ✓ v1.0: region and board profiles are implemented and exercised. |
| The tick is one CPU cycle at 33,868,800 Hz; the GPU clock comes from an exact ratio accumulator; the SPU steps every 768 ticks; a CD sector every 451,584 ticks; no floating point anywhere | Every documented edge is an integer on this lattice; no small LCM with the GPU clock exists | ✓ v1.0: integer timing, regional sample cadence and cross-platform hashes pass. |
| A frame runs from vsync to vsync; `schway_run_frame` and `schway_run` return a stop reason and never sleep | Lowest input latency; bounded calls; handheld and browser hosts drive pacing | ✓ v1.0: bounded frame/run calls and stop-reason contracts pass. |
| The R3000A and the GTE are finished against vectors before any game: the SingleStepTests/r3000 sample per PR, the full set nightly, the 1150 ps1-tests GTE cases transcoded into an own vector file | Siblings that grew CPUs from game traces stalled; the GTE cases are hardware-captured | ✓ Phases 2–3: public CPU vectors and hardware GTE cases pass. |
| v1 is an interpreter written to the fast-path constraints; the cached interpreter and JIT are seeds behind `core.exec_mode_equivalence` | A faster core must hash identically on all eight lanes before it ships | ✓ v1.0 interpreter shipped; cached interpreter/JIT remain deferred. |
| Self-authored armips PS-EXE fixtures and one mkpsxiso disc with a RAM mailbox judged by `expected.csv` are the primary evidence | Licence-clear, byte-identical builds, oracles that cannot be fooled | ✓ v1.0: PS-EXE and owned disc fixtures, RAM mailboxes and host runs are integrated. |
| ares (ISC), MAME's BSD-3 device files, nugget with OpenBIOS (MIT), ps1-tests and the r3000 set (MIT) may be read; DuckStation, Mednafen, Beetle, ReARMed, Redux core, Avocado and Rustation never | MIT licence protection; two names the brief called MIT were GPL at the pin | ✓ Provenance and policy gates enforce permissive reuse and the prohibited-source rule. |
| A retail BIOS is user-supplied only, recognised by digest; OpenBIOS is built in CI as the stand-in; an own stub kernel boots every PS-EXE fixture; no HLE | Firmware copyright; HLE is the top compatibility complaint | ✓ v1.0: BIOS digest recognition, CI OpenBIOS and own stub paths pass; no proprietary bytes committed. |
| Video is the native display frame in GPU encoding with a metadata tuple plus a read-only VRAM view; conversion, borders and deinterlacing are host helpers outside the hash | Lossless and host-independent; ps1-tests compare whole VRAM | ✓ v1.0: native frames and host conversion are deterministic; GPU references pass. |
| Audio is int16 stereo at 44,100 Hz with exactly `frame_cycles / 768` samples per frame; the hash is over that stream before any host resampling | Mixer-independent determinism | ✓ v1.0: hardware transfer logs, cross-platform hashes and runner/libretro PCM parity pass. |
| The core never opens files: the host supplies `schway_disc` sector callbacks; an in-repo MIT helper parses cue/bin, iso and m3u; CHD, PBP and ECM are v2.0 | Hostile parsing stays out of the deterministic core; browser hosts serve sectors themselves | ✓ v1.0: host callbacks and cue/bin, ISO and M3U parsing pass; CHD/PBP/ECM remain deferred. |
| Nothing speed-changing exists: no CPU, GTE or GPU clock scale, no CD speedup; enhancements never enter the hashed path | rcheevos restricts such options; speedrun boards forbid them; accuracy complaints about PGXP and overclock | ✓ v1.0: no speed-changing option or enhancement enters the hashed path. |
| Every phase ends in an automated release; phase 1 is the content-free walking skeleton ported from cogmaster and muhtari at public pins | Sibling velocity evidence | ✓ Six phase PRs merged; the automatic release workflow published v0.6.0. |
| Cachegrind Ir/frame on three fixtures as the only per-PR speed gate; proportional gates; real-host e2e at most 300 lines; pins public and fetched with `gh api`; pre-seeded GSD bootstrap, interactive, one step per session | Wall-clock tails are noise on shared runners; two siblings stalled on their own machinery; a sibling lost six plans to an unpushed pin | ✓ v1.0: performance/policy checks passed, real-host e2e stayed within cap and source pins remained public. |
| v2.0 expands playable library breadth while keeping the deterministic core, permissive reuse policy and host-owned I/O boundaries | The owner approved the whole-library direction; the browser and save-state directions remain later milestones | — v2.0: requirements and verification are pending. |

The full list with sources is `.planning/preparation/DECISIONS.md`.

## Evolution

After each phase transition:
1. Validated requirements move from Active to Validated with the phase number and a ✓ outcome.
2. Key Decisions get their outcome column filled from the phase's verification.
3. New constraints or scope changes are recorded with the date.

After each milestone:
1. The Active list is rewritten from the next milestone's requirements.
2. `.planning/PRODUCT-ROADMAP.md` and `.planning/preparation/PROMPT-COVERAGE.md` are refreshed and snapshotted.
3. Lessons are exported to `.planning/LESSONS.md`.

---
*Last updated: 2026-11-18 after v2.0 milestone approval*
