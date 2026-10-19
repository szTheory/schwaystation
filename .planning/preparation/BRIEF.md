---
title: "Preparation writing brief"
summary: "The conventions every preparation file follows, the licence rules for research, and the fan-out method; read by every agent that writes or revises a file in this directory."
read_when: "Before writing or revising any file in .planning/preparation/."
updated: 2026-10-19
---

# Preparation writing brief

schwaystation is a **Sony PlayStation (PS1 / PSX, SCPH-1000 through SCPH-102 and the PS one)** emulator core in
C: one core, with console revisions and regions (NTSC-J, NTSC-U/C, PAL) as machine profiles. It is NOT the
PlayStation 2, PSP or any other console; another console appears only as labelled precedent and never lends
timing. The owner's sibling projects (cogmaster = Master System/Game Gear, muhtari = Atari 7800, nesturbator = NES,
gabbaboy = Game Boy, glueyneo = Neo Geo, genewhip = Genesis, playstead = frontend) are precedent for process and
engineering only; cogmaster and muhtari are the direct templates for every process, engineering and host file.

## File conventions (identical to cogmaster's and muhtari's preparation packs)
- Frontmatter: `title`, `summary`, `read_when`, `updated` (ISO date). Then a **Where to look** table
  (`| Question | Section |`), then numbered sections (`## 1. …`, subsections `### 1.2`).
- Every factual claim cites `[PFX.NN]` resolving to a **Sources** table at the end of the same file:
  `| ID | Tier | Source | Pin or access date | Supports |`. Prefixes: SYS (PSX-SYSTEM), GPU (PSX-GPU-AND-GTE),
  SCM (PSX-SPU-CDROM-AND-MDEC), PER (PSX-PERIPHERALS-AND-SAVES), ECO (PSX-ECOSYSTEM-AND-USERS),
  PX (PSXDEV-EXTRACTS), CF (CONFORMANCE), HI (HOSTS-AND-INTEGRATION), ENG (ENGINEERING), ARCH (ARCHITECTURE),
  GH (GSD-HANDOFF), PV (PROVENANCE.md), AP (ASSET_POLICY.md). `DEC.NN` is a row of DECISIONS.md.
- **Tiers.** T1 primary: Sony and LSI/MIPS documents (the official PlayStation hardware and SDK documentation,
  the IDT/MIPS R3000 family manuals, the LSI CW33300 material), schematics, die or RTL analysis, measurements on
  hardware reported with their method, a project's own repository for what that project does. T2 curated
  community reference (psx-spx by nocash and its consoledev.net edition, the psxdev.net wiki, documents by named
  authors). T3 one implementation's behaviour as evidence of hardware (permissively licensed emulator source,
  FPGA RTL, test-suite results tables). T4 forum, Discord and issue-tracker posts. A tier describes the source
  for the claim it supports.
- **Pins.** A 12-character commit SHA, a release tag, a wiki revision, or "accessed YYYY-MM-DD".
- **Markers.** (tested) was run here; (untested) was not; (derived) is inferred from cited facts;
  (unverified) comes from recall or was not fetched. Never present recall as a cited fact.
- **Choices.** Each gives a **Recommendation**, the **Strongest alternative**, and **Would change if**: the
  observation that would flip it. Settle every decision point: the GSD phases follow these without re-asking.
- **IDs** use a dot (`SYS.07`), never GSD's `D-NN` form. Prose is in our own words with a citation; no verbatim
  copying of documents longer than a short phrase.
- Hygiene: no e-mail addresses, no home-directory paths, no disc names from the owner's private library, no
  BIOS bytes, no credentials. Say "the owner" not a name.

## Licence rules for research (PROVENANCE)
- May be **opened and read, and seeded with notice** once the licence is confirmed at the exact pin: ares (ISC;
  `ares/ps1/*`, `ares/component/processor/mips/*` if present), MAME files headed `license:BSD-3-Clause` (check
  each file header: `src/devices/cpu/psx/*`, `src/devices/video/psx.cpp`, `src/devices/sound/spu.cpp`,
  `src/devices/bus/psx/*`, `src/mame/sony/psx.cpp`), libchdr (BSD-3), permissively licensed test suites (verify each: JaCzekanski/ps1-tests, PeterLemon/PSX,
  SingleStepTests/r3000 (MIT, generated from ares)), PSn00bSDK (MPL-2.0: facts and headers only; built bytes carry notices),
  pcsx-redux's OpenBIOS only if its own subproject licence is permissive at the pin, and the owner's muhtari and
  cogmaster (MIT, same author).
- **Never opened** (facts about them come only from their docs, issues, changelogs and released binaries):
  DuckStation (CC BY-NC-ND 4.0 at `124a4b16feda`, PolyForm Strict before that), Mednafen and Beetle PSX (GPL-2), PCSX-ReARMed and PCSX-Reloaded
  (GPL-2), PCSX-Redux core (GPL-2 text at `91285bf736af`; its nugget submodule with OpenBIOS is MIT at `9db6d88d23c4`), SwanStation (GPL), Avocado (GPL-2.0 at `d7fcf9060ac7`, checked 2026-10-19),
  Rustation (GPL-2+ at `5e650b949d6e`), the simias psx-guide (no LICENSE file: facts only), PeterLemon/PSX (no
  licence: fetch only, never commit), BizHawk's GPL cores, ePSXe, XEBRA, no$psx, pSX (closed).
- Check a licence at the exact pin (LICENSE file and per-file headers) with `gh api`; report what you checked.

## Fan-out method
For each decision point, consider breadth and depth through every relevant stakeholder lens (emulator author,
hardware engineer, C/embedded engineer, performance engineer, security/fuzzing, product, speedrunner/TAS, ROM
hacker and translator, homebrew developer, frontend integrator, handheld/browser porter), with pros, cons,
trade-offs, examples, patterns, anti-patterns, footguns and lessons from other emulators and ecosystems.
Research online; prefer primary sources. Run an adversarial pass on your own recommendation. Then synthesise ONE
coherent recommendation per point. Record everything high-signal with provenance so the file works as a RAG
source.

## Size and style
Each file 200–350 lines, dense, scannable, no padding. Tables over prose where rows are parallel. Sentences
short. No em-dashes. Every number that matters is in a table with its source.
