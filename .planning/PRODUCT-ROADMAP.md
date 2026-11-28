---
title: "Product roadmap (rolling)"
summary: "Immediate / Near / Mid / Long direction for schwaystation with the evidence and promotion trigger for each horizon; ROADMAP.md is the execution contract, seeds hold the source narratives."
read_when: "At every new-milestone and complete-milestone step, and when a finding changes priorities."
updated: 2026-11-28
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

## v2.0 tune-up evidence (Phase 7, complete)

Phase 7 shipped as v0.7.0. It records repeatable Cachegrind work for `testcard` and `gpu_motion`, serialized state
size, a comparable full-CI push critical-path range, and the linked WASI module measurement. The original producers
and cohort remain in the [Phase 7 baselines](milestones/v2.0-phases/07-faster-and-more-trustworthy-ci/07-BASELINES.md); later
phase-owned producer updates set the current values to 1,620,101 and 138,319,540 Ir/frame, 3,757,638 state bytes,
and 947,969 WASI bytes. The two comparable full-CI pushes measured 2,249–3,293 seconds.

The release-App PR #29 retained `CI required` and skipped the measured heavy matrix; its run is about 70 seconds.
Because the merge-ref SHA was not retained, it is route and required-check evidence rather than an exact workload
comparison or a speedup claim. The detailed [CI efficiency report](milestones/v2.0-phases/07-faster-and-more-trustworthy-ci/07-CI-EFFICIENCY.md)
and [delivery record](milestones/v2.0-phases/07-faster-and-more-trustworthy-ci/07-DELIVERY-EVIDENCE.md) preserve that limitation.
`gpu_motion` exceeds provisional budgets for each listed low-power device class, whose sustained MIPS remain
assumptions; SEED-005 retains the cached-interpreter and target-device follow-up. The wasm32 hash lane is not a
browser-speed measurement. The quality review selected the README's false BIOS/disc status statement and verified
its correction; it did not broaden compatibility claims.

## v3.0 tune-up evidence (Phase 17; final verification pending)

Phase 17 remeasured the existing quality producers and refreshed the audited testing, compatibility, and roadmap
evidence. `testcard` is 1,620,381 Ir/frame, `gpu_motion` is 138,406,446 Ir/frame, and stable state is 3,757,754
bytes. The local linked-Wasm result is 1,014,617 bytes; the latest successful hosted producer passes the retained
1,014,617–1,014,665-byte interval but does not expose its exact value. The 40-run CI review found no repeated
comparable full-CI workload, and PR execution SHAs are unavailable. The `ci.critical_path_s` interval remains
historical until the registered producer obtains a matching current result. See [Phase 17 baselines](phases/17-a-measured-and-trustworthy-playability-baseline/17-BASELINES.md)
and [CI efficiency evidence](phases/17-a-measured-and-trustworthy-playability-baseline/17-CI-EFFICIENCY.md).

The one selected improvement adds the existing NTSC-U `gpu_motion` frame-120 hash predicate to the measured
compatibility ledger. The generated report now has five measured fixture predicates; all 13,407 Redump entries
and the 27 required profile rows remain unknown, and libretro remains experimental. This result does not establish
retail playability, device speed, browser speed, or a complete game session. Audited document freshness, quality
ratchets, and handoff preparation are required phase work kept separate from that selected improvement. Final
review and phase verification remain pending. The approved v3.0 homebrew and owner-selected retail scopes are
unchanged.

## Shipped: v1.0 "PlayStation games play with sound in RetroArch" (completed 2026-11-18)

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
CD-DA, the MDEC, disk control, the own disc fixture booting through OpenBIOS, hashes identical on every platform. Requirements: [archived v1.0 requirements](milestones/v1.0-REQUIREMENTS.md). Evidence: [milestone audit](milestones/v1.0-MILESTONE-AUDIT.md). Exact 8-bit MDEC output and complete external frame/movie conformance remain unproven under the accepted v1.0 scope.

## Shipped: v2.0 "The whole library" (completed 2026-11-26)

Ten phases and 125 plans added CHD, PBP, ECM, CloneCD/IMG/SUB and SBI/LibCrypt support; per-game card lifecycle,
multitap, special controllers and guns; PAL/revision profiles and opt-in BIOS-shell fast boot; PPF/BPS overlays,
bounded GameShark support, and a 51-row required-capability matrix. All requirements passed phase verification,
and v0.16.0 was published after the exact-head Main and Release gates passed. See the [milestone audit](milestones/v2.0-MILESTONE-AUDIT.md)
for the requirement and integration map. The DAT inventory is not playability evidence: 35 of the 51 required
capability rows remain unknown, so the metadata remains experimental. Exact 8-bit MDEC hardware output and full
external frame/movie conformance remain unproven.

The final Main CI critical path was about 69 minutes and the full Release lane about 77 minutes, with BE32 the
slowest job. TODO-051 and TODO-052 preserve the full Nightly and BE32 gates until a coverage-preserving route or
shard proves an improvement. The measured BE32 candidate remains a follow-up, not an unvalidated workflow change.

## Immediate: v3.0 "Prove a game plays" (new scope; SEED-007 tune-up first)

One integrated, self-authored bootable homebrew session passes in RetroArch CI: BIOS handoff, disc access,
input-driven gameplay change, moving picture, audible output, deterministic replay and runner parity. Then one
owner-selected retail game reaches that same playable threshold locally through RetroArch using the owner's BIOS
and disc. Retail bytes, paths and captures stay local under `ASSET_POLICY.md`; evidence names only the approved
title identity and predicates. One title does not change the 13,407-entry inventory or clear the experimental
marker. **Trigger:** owner selected game playability as the next milestone after v2.0; Phase 17 starts with
SEED-007.

## Near: v4.0 "States and movies" (SEED-002), v5.0 "The browser" (SEED-008)

v4.0: fixed-size validated save states with cards outside by default, measured rewind and run-ahead claims,
frame-exact movies and replay, and Playstead continuation. **Trigger:** v2.0 shipped; now promoted to Near after
the playability proof because its state and replay surface supports repeatable play sessions.

v5.0: a browser host for the existing deterministic core, with a real WebAssembly browser launch and measured
playability/performance on a named desktop browser; the wasm32 hash lane remains only a determinism check.
**Trigger:** the game-shaped RetroArch path is proven and browser work can be measured against it; retain the
owner's stated high priority for browser and low-power performance.

## Mid: v6.0 "Measured accuracy" (SEED-003), v7.0 "Tools" (SEED-004)

v6.0: settle documented GPU, CD, MDEC, SPU/XA and reverb uncertainties with a stable reproducible mismatch and
an available oracle; do not assume a hardware capture is available. **Trigger:** a retail or homebrew playability
run produces a repeatable mismatch that the current evidence cannot explain.

v7.0: debugger/GDB views, ROM-hacker aids, Python bindings and Gymnasium hooks. **Trigger:** a concrete user,
speedrunner or tooling request identifies the first consumer.

## Long: v8.0 "Everywhere" (SEED-005), enhancements (SEED-006)

The cached interpreter and a JIT behind `core.exec_mode_equivalence`, handheld and console compile lanes, the
libretro buildbot and additional Playstead targets; internal resolution, PGXP-style precision, widescreen,
texture filtering and true colour as a host renderer outside the hashed path. **Triggers:** a named port target
or measured instructions-per-frame budget for Everywhere; a concrete renderer consumer for enhancements.

## Decision log

| 2026-11-18 | Completed v1.0 and refreshed the shipped/future boundary; v2.0 remains a seed, not an active milestone | Six phases and 195 plans are archived; future scope is not authorized by this closeout. |
| 2026-11-18 | Owner approved v2.0 “The whole library” as the next milestone; SEED-001 is promoted and mandatory SEED-007 opens the sequence | v1.0 is released and main CI is green; browser UI remains outside this milestone. |
| 2026-11-19 | Phase 7 tune-up records current deterministic quality and CI baselines while preserving device/browser evidence limits | `gpu_motion` exceeds provisional low-power budgets; device MIPS are unmeasured, and hosted release-PR timing plus WASI module size remain for ship. |
| 2026-11-24 | Final Phase 7 evidence reconciled | The automatic v0.7.0 release PR route, exact WASI producer and release evidence are recorded; the missing merge-ref SHA still prevents an exact release-App speedup comparison. Current producer values are linked above. |
| 2026-11-26 | v2.0 shipped and archived after Phase 16 | Ten phases passed verification, 34 requirements and seven integration flows are covered, and v0.16.0 was published. v3.0's trigger is met but remains in Mid pending a separate scope decision; TODO-051/052 retain their evidence requirements. |
| 2026-11-27 | Promoted v3.0 playability proof to Immediate; states/movies and browser to Near | The 51-row required matrix still has 35 unknowns and no retail title pass. Existing RetroArch evidence covers interactive fixtures, sound and bootable-disc behavior in separate checks. |
| 2026-11-28 | Reconciled Phase 17's measured tune-up evidence; kept the approved v3.0 scope | One existing NTSC-U fixture predicate was added at frame 120. Quality results, CI comparability limits, unknown compatibility rows, and unmeasured device/browser performance remain explicit; review and final phase verification are pending. |

| Date | Change | Why |
|---|---|---|
| 2026-10-19 | Initial horizons set during preparation | Library shares from the libretro Redump DAT (13,407 entries at `12c21cf79f1e`: Japan 6,491, USA 2,386, Europe 2,308; 3,573 multi-disc entries; about 100 LibCrypt PAL titles); sibling velocity evidence (every phase a release; fixtures and the BIOS stand-in chosen at kickoff); owner decisions: one core with revisions and regions as profiles, no Sony bytes anywhere, CHD deferred to v2.0 |
| 2026-10-19 | The browser page is SEED-008 with a wasm32 hash lane already in phase 1; power efficiency is a phase-1 ratchet | Owner: running efficiently in the browser and on low-power devices is very important; the Ir/frame gate and device budgets measure it from the first release |
| 2026-10-19 | Speedrun, TAS and AI surfaces staged as v3.0 (states, movies, lag frames) and v5.0 (Gymnasium, hooks); nothing speed-changing exists in the hashed path | Owner ask; speedrun boards forbid seek speedups and TASVideos requires determinism and a matching region |
| 2026-10-19 | ROM-hacker aids (PPF/BPS through a sector overlay, GameShark strings, probe of serial and SYSTEM.CNF) land in v2.0 with the library pass; the probe is v1.0 | Owner ask; hackers and translators need the whole-library disc path first |
