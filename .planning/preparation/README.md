---
title: "Preparation index"
summary: "Which preparation file answers which question, and the conventions every file in this directory shares."
read_when: "First, before opening any other file in this directory; again when adding, renaming or retiring one."
updated: 2026-10-19
---

# Preparation index

Reference material gathered before schwaystation's first line of code: PlayStation hardware facts with sources,
the test and oracle plan, engineering, host and process decisions, and how GSD is run. It is the project's
searchable second brain: each file opens with frontmatter saying when to read it and a Where to look table, cites
tiered, pinned sources, and gives one recommendation per choice. Open the file a question points to; none needs
reading end to end. `BRIEF.md` holds the conventions and the fan-out method every file follows.

## Files
| File | Holds | Read when |
|---|---|---|
| [BRIEF.md](BRIEF.md) | Conventions, source tiers, licence rules for research, the fan-out method | Before writing or revising any file here |
| [ARCHITECTURE.md](ARCHITECTURE.md) | The core's spine: the CPU-cycle time base and frame boundary, modules and the bus, the `schway_` C API with stop reasons, I/O contracts, the fixed-size visitor state, observability, determinism and the three hashes, the interpreter and its fast-path constraints, the rasteriser, the SPU/CD/MDEC pipeline, build order, performance posture, settled conflicts | Before designing a core subsystem, the public header, the libretro adapter or the runner |
| [DECISIONS.md](DECISIONS.md) | One row per settled choice (`DEC.NN`) with its reason and supporting section; open questions with the test or phase that settles each | Before reopening a choice, or when a phase needs to know what is settled |
| [PSX-SYSTEM.md](PSX-SYSTEM.md) | Clocks and the master tick, the R3000A and COP0, memory map and bus control, DMA, root counters, interrupts, BIOS and boot, regions and revisions, power-on state, confidence | Changing the time base, CPU, bus, DMA, timers, interrupts, boot path, region handling or any timing that crosses chips |
| [PSX-GPU-AND-GTE.md](PSX-GPU-AND-GTE.md) | GPU revisions and GPUSTAT, GP0/GP1 commands, rasterisation rules, video clocks and display, the GTE, tests and oracles, accuracy model, enhancement seeds and the v1 frame format, low-power rasteriser | Writing or changing `gpu.c` or `gte.c`, the frame format or display timing, or when a picture diverges |
| [PSX-SPU-CDROM-AND-MDEC.md](PSX-SPU-CDROM-AND-MDEC.md) | The SPU, the CD-ROM controller and its timings, disc images and the host disc interface, the MDEC, tests and fixtures, determinism and A/V sync, confidence | Building the SPU, CD-ROM, XA, MDEC, the disc helper, the mixer or audio tests |
| [PSX-PERIPHERALS-AND-SAVES.md](PSX-PERIPHERALS-AND-SAVES.md) | SIO0, every pad and gun, memory cards and card file formats, save-state constraints, game identity and rcheevos hash, patches and cheats, hostile-input bounds, milestone placement | Changing SIO0, a device model, cards, saves, the probe, patch handling or fuzz targets |
| [PSX-ECOSYSTEM-AND-USERS.md](PSX-ECOSYSTEM-AND-USERS.md) | Every PS1 emulator with its licence at a pin, user complaints mapped to responses, personas by horizon, low-power and WASM budgets, library sizes, the slot schwaystation fills | Ranking features or horizons, choosing an oracle, planning a port, answering "why not emulator X" |
| [PSXDEV-EXTRACTS.md](PSXDEV-EXTRACTS.md) | Paraphrased psx-spx and community facts with who measured what, the test-program inventory with licences, toolchain pins and digests, BIOS stand-ins, fixture candidates, hash databases, the access log | Before relying on a community claim cited in another file; when admitting a fixture or tool |
| [CONFORMANCE.md](CONFORMANCE.md) | Vector sets and test programs with dispositions, own PS-EXE and disc fixtures with the mailbox and TTY protocol, oracles, the compatibility ledger and ratchet, first-fault diagnostics, CI tiers and cost, fuzz targets, admission rules, the milestone map | Admitting a fixture, writing diagnostics or the result protocol, building the ledger or a CI tier |
| [HOSTS-AND-INTEGRATION.md](HOSTS-AND-INTEGRATION.md) | libretro contract, RetroAchievements, RetroArch on Apple Silicon, `schwaystation-run` and Playstead, scripting order, WASM, the shared C API, feature-to-host table | Building the adapter or `.info`, the runner CLI, saves, the RetroArch e2e job, a Playstead pin or a scripting surface |
| [ENGINEERING.md](ENGINEERING.md) | Re-verified facts, C rules and presets, tests, CI, release, repository settings, triage, performance ratchet, portability lanes, style, docs, recurring loops, secrets, WASM and low power, sibling lessons as rules | Creating or changing the build, tests, workflows, repository settings, the release pipeline or quality gates |
| [GSD-HANDOFF.md](GSD-HANDOFF.md) | The kickoff text, per-phase and milestone routines, settings and why, what stops GSD running on, owner tasks, what was checked | Before the kickoff, before each GSD step, and when a step behaves unexpectedly |
| [PROMPT-COVERAGE.md](PROMPT-COVERAGE.md) | Every owner ask mapped to its home with status and what remains; owner constraints vs preparation's choices | At kickoff review and at every milestone close |
| [PROVENANCE.md](../../PROVENANCE.md) | Which code, tables and facts may enter; the upstream licence audit at each pin; strangler-fig seeds; notices; the never-opened list | Before reusing or citing any outside code or data |
| [ASSET_POLICY.md](../../ASSET_POLICY.md) | Which bytes may be committed, built in CI or kept local; the BIOS digest table; how it is checked; size caps | Before adding a BIOS, disc, executable, card, state, screenshot or fixture |
| [LESSONS.md](../LESSONS.md) | Sibling lessons imported as hypotheses with their local disposition; the export template | At every discuss step, at phase and milestone boundaries, or when the owner asks to export lessons |
| [PRODUCT-ROADMAP.md](../PRODUCT-ROADMAP.md) | Immediate / Near / Mid / Long horizons with evidence, triggers and a decision log | At every milestone step, or when a finding changes priorities |
| [seeds/](../seeds/) | SEED-001 to SEED-008 hold the later horizons' narratives; SEED-007 is the tune-up that opens every later milestone | When a seed's trigger fires, and at every new milestone |

## Where to look
| Question | File and section |
|---|---|
| What is one tick; how are the GPU, dot, SPU and CD clocks derived from it without floating point; what are the exact line and frame rates? | PSX-SYSTEM 1; ARCHITECTURE 1; PSX-GPU-AND-GTE 4 |
| What must the CPU model: pipeline, delay slots, mul/div, COP0, exceptions, I-cache, scratchpad? | PSX-SYSTEM 2; DECISIONS CPU rows |
| What answers each address, how do the BIU delay registers shape timing, what do unmapped regions do? | PSX-SYSTEM 3; ARCHITECTURE 2 |
| How do the DMA channels, chopping and CPU stalls, the root counters and I_STAT/I_MASK work? | PSX-SYSTEM 4 to 6 |
| Which BIOSes exist with which digests, what an EXE needs, and how CI boots without a Sony BIOS? | PSX-SYSTEM 7; PSXDEV-EXTRACTS 5; ASSET_POLICY 5 |
| Which regions and board revisions differ, what are the profile fields, what is the power-on state? | PSX-SYSTEM 8, 9 |
| GPU revisions, VRAM and GPUSTAT; the GP0/GP1 command set; dither, blend, modulation, mask, texture decode, fill rule? | PSX-GPU-AND-GTE 1 to 3 |
| Video clocks, hblank and vblank, FIFO and DMA, primitive cost, display mapping? | PSX-GPU-AND-GTE 4 |
| GTE registers, opcodes, cycle counts, flags and the UNR division? | PSX-GPU-AND-GTE 5 |
| What does the v1 output frame look like, and where do enhancements hook in later? | PSX-GPU-AND-GTE 8; ARCHITECTURE 9; HOSTS-AND-INTEGRATION 8 |
| SPU voices, ADSR, reverb and mixing; the audio contract and hash; A/V sync? | PSX-SPU-CDROM-AND-MDEC 1, 6; ARCHITECTURE 10 |
| CD-ROM commands, INT semantics, measured timings, GetID and region, LibCrypt? | PSX-SPU-CDROM-AND-MDEC 2 |
| Disc image formats and the host sector interface the core expects? | PSX-SPU-CDROM-AND-MDEC 3; ARCHITECTURE 4; DECISIONS DEC.42, DEC.43 |
| MDEC commands, the integer IDCT and the FMV path? | PSX-SPU-CDROM-AND-MDEC 4 |
| SIO0 protocol and timing; pads, config mode, rumble, multitap, guns; what ships in v1.0? | PSX-PERIPHERALS-AND-SAVES 1, 2 |
| Memory card layout, file formats, libretro naming and export; save-state constraints? | PSX-PERIPHERALS-AND-SAVES 3, 4; ARCHITECTURE 5; HOSTS-AND-INTEGRATION 1, 4 |
| How is a disc identified; which bytes does rcheevos hash? | PSX-PERIPHERALS-AND-SAVES 5; HOSTS-AND-INTEGRATION 2 |
| PPF, cheats, what ROM hackers get; which parsers face hostile input and with which bounds? | PSX-PERIPHERALS-AND-SAVES 6, 7; CONFORMANCE 6; ENGINEERING 3 |
| Which PS1 emulators exist with which licence, and what do users dislike or wish for? | PSX-ECOSYSTEM-AND-USERS 1, 2 |
| What do speedrunners, TASers, AI agents, homebrew developers and frontends need; can low-power devices keep up? | PSX-ECOSYSTEM-AND-USERS 3, 4 |
| Which test programs and vector sets exist, under which licence; what is committed and what is fetched? | PSXDEV-EXTRACTS 3; CONFORMANCE 1, 7; ASSET_POLICY 2, 3 |
| Which toolchains build the fixtures (armips, mkpsxiso, GCC) at which pin and digest? | PSXDEV-EXTRACTS 4; CONFORMANCE 2 |
| Which fixtures do we write, what does each prove, and how do the mailbox and TTY protocols work? | CONFORMANCE 2, 4 |
| Which oracle and diagnostics come first; how do the ledger and the ratchet work; what runs per PR and nightly at what cost? | CONFORMANCE 3, 5; ENGINEERING 4, 8 |
| Which flags, presets, tests and style rules apply? How is a release made or withdrawn? | ENGINEERING 2, 3, 10; ENGINEERING 5 |
| What must the libretro core export, how does a release reach RetroArch on a Mac, what are the runner's options and exit statuses? | HOSTS-AND-INTEGRATION 1, 3, 4 |
| Every public function and struct with its contract, stop reasons and ABI versioning? | ARCHITECTURE 3; HOSTS-AND-INTEGRATION 7 |
| May this code or table be reused; how do I test with my own BIOS and discs? | PROVENANCE 1 to 4; ASSET_POLICY 4 |
| What is already decided, which conflicts between files were settled, and how is GSD started and run each phase? | DECISIONS; ARCHITECTURE 13; GSD-HANDOFF 2, 3 |
| Does every owner ask have a home? | PROMPT-COVERAGE 1 |

## Conventions
See `BRIEF.md`: frontmatter, Where to look table, numbered sections, `[PFX.NN]` citations resolving in the file's
Sources table, tiers T1 to T4, pins, markers, Rec/Alt/Flip choices, dotted IDs, own words only, no personal data.

## Keeping it current
- A phase that finds one of these files wrong or stale fixes it and its `updated` date in the same pull request.
- When a subsystem lands, its hardware facts graduate into public `docs/hardware/<chip>.md`, which code comments
  cite (ENGINEERING 10, 11), and the preparation file's section shrinks to a pointer there.
- Research for a single phase lives with that phase under `.planning/phases/`; this directory keeps only
  cross-phase reference. A new, renamed or retired file updates both tables here; PROMPT-COVERAGE and
  PRODUCT-ROADMAP are refreshed at each milestone close (GSD-HANDOFF 4).
- psxdev.net has been offline since October 2025 (shutdown notice, wiki 404, no archive capture of the help or
  forum pages) and redump.org refused connections for its wiki and dumping rules on 2026-10-19, so every fact
  taken from either is marked (unverified) in PSXDEV-EXTRACTS 8, PSX-PERIPHERALS-AND-SAVES and
  PSX-SPU-CDROM-AND-MDEC, and is re-read at the phase 2, phase 5 and phase 6 research steps.
