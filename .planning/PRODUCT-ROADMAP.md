---
title: "Product roadmap (rolling)"
summary: "Immediate / Near / Mid / Long direction for schwaystation with the evidence and promotion trigger for each horizon; ROADMAP.md is the execution contract, seeds hold the source narratives."
read_when: "At every new-milestone and complete-milestone step, and when a finding changes priorities."
updated: 2026-10-19
---

# Product roadmap (rolling)

`ROADMAP.md` (created at kickoff) is GSD's execution contract for the current milestone. This file carries the
direction beyond it; each horizon names the seed that holds its narrative and the trigger that promotes it.
Planning milestones (v1.0, v2.0, ...) are not package versions: packages follow pre-1.0 semver and every phase that
adds a capability publishes a new minor release. Every milestone names a behaviour a player can see (sibling
lesson XFER-047); governance and evidence work rides inside the tune-up phase, never as a milestone of its own.

**Refresh rule.** At each milestone boundary the agent reconciles shipped and partial work, keeps unresolved debt
visible, re-ranks the horizons, records why the order changed or stayed, presents the preferred next scope with
alternatives, and moves only owner-approved scope into Immediate. The file is snapshotted as
`milestones/vX.Y-PRODUCT-ROADMAP.md` at close. Future horizons stay less detailed than the current one.
SEED-007 (the tune-up) opens every milestone after the first.

## Immediate: v1.0 "PlayStation games play with sound in RetroArch" (committed)

Six phases, each merged by one pull request and released automatically: (1) a test frame in RetroArch with the
whole delivery path (library, runner, libretro core, six-platform CI, big-endian and wasm32 lanes, attested
releases, install line, `hello.exe` source committed); (2) the R3000A matches the public vectors (the
SingleStepTests sample per PR and the full set nightly), with COP0 exceptions, bus decode, memory control, the
I-cache and interrupt registers, PS-EXE loading under the own stub kernel with armips on every leg, the probe and
the first-fault trace; (3) the GTE matches the 1,150 ps1-tests cases, with DMA and timers; (4) games play in
RetroArch: the GPU rasteriser and display modes, the DualShock and digital pad with rumble, memory cards,
self-authored fixtures with boot-to-play checks, OpenBIOS booting to its shell in CI, a local preset for the user's
own dumps, the compatibility ledger; (5) sound plays in RetroArch: the SPU against the hardware logs and libretro
audio equal to the runner's stream; (6) discs and FMV: the CD-ROM controller with cue/bin, iso and m3u, XA and
CD-DA, the MDEC, disk control, the own disc fixture booting through OpenBIOS, hashes identical on every platform. Requirements: `.planning/REQUIREMENTS.md` v1.
Evidence: `.planning/research/SUMMARY.md`.

## Near: v2.0 "The whole library" (SEED-001)

CHD through libchdr, PBP, ECM, CCD and SBI/LibCrypt; the multitap, GunCon and Justifier, mouse, neGcon and Jogcon;
PocketStation as card storage; per-game cards by serial and the libretro save round trip; PAL and every revision
profile over the whole ledger (13,407 Redump entries, 3,573 multi-disc entries across 568 titles, about 100
LibCrypt PAL titles); GameShark strings and `--patch` for ROM hackers; the `.info` drops `is_experimental`.
**Trigger:** v1.0 shipped. **Why next:** what collectors and players notice first after the core plays; the owner's
disc collection becomes the ratchet's input; CHD is the format most frontend libraries already hold. Opens with
SEED-007. **Weighed at the same boundary:** the browser page (SEED-008), which the owner ranks highly.

## Mid: v3.0 "States and movies" (SEED-002), v4.0 "Measured accuracy" (SEED-003)

v3.0: fixed-size validated save states with cards outside by default, rewind and run-ahead claims in the libretro
`.info` behind their tests, an own movie format with BK2 and PJM import, emulated-time print and input display for
speedrunners, Playstead continuation. **Trigger:** v2.0 shipped, or Playstead's `psx` integration needs
continuation first.
v4.0: fixtures run on the owner's console settle the fitted constants (GPU clocks per line, the CD seek curve and
INT1 period, per-primitive GPU cost, MDEC block cost, ADSR edges, XA and reverb rounding); nightly oracle lanes
against released binaries. **Trigger:** a stable failing set in the ledger that documentation does not explain.

## Long: v5.0 "Tools" (SEED-004), v6.0 "Everywhere" (SEED-005), enhancements (SEED-006), the browser (SEED-008)

Hooks, a GDB stub, debugger views and ROM-hacker aids, Python and Gymnasium bindings; the cached interpreter and a
JIT behind `core.exec_mode_equivalence`, handheld and console compile lanes, the libretro buildbot, Playstead;
internal resolution, PGXP-style precision, widescreen, texture filtering and true colour as a host renderer outside
the hashed path; the WebAssembly page. **Triggers:** a user ask, a met instructions-per-frame budget or a named
port target; each seed names its own.

## Decision log

| Date | Change | Why |
|---|---|---|
| 2026-10-19 | Initial horizons set during preparation | Library shares from the libretro Redump DAT (13,407 entries at `12c21cf79f1e`: Japan 6,491, USA 2,386, Europe 2,308; 3,573 multi-disc entries; about 100 LibCrypt PAL titles); sibling velocity evidence (every phase a release; fixtures and the BIOS stand-in chosen at kickoff); owner decisions: one core with revisions and regions as profiles, no Sony bytes anywhere, CHD deferred to v2.0 |
| 2026-10-19 | The browser page is SEED-008 with a wasm32 hash lane already in phase 1; power efficiency is a phase-1 ratchet | Owner: running efficiently in the browser and on low-power devices is very important; the Ir/frame gate and device budgets measure it from the first release |
| 2026-10-19 | Speedrun, TAS and AI surfaces staged as v3.0 (states, movies, lag frames) and v5.0 (Gymnasium, hooks); nothing speed-changing exists in the hashed path | Owner ask; speedrun boards forbid seek speedups and TASVideos requires determinism and a matching region |
| 2026-10-19 | ROM-hacker aids (PPF/BPS through a sector overlay, GameShark strings, probe of serial and SYSTEM.CNF) land in v2.0 with the library pass; the probe is v1.0 | Owner ask; hackers and translators need the whole-library disc path first |
