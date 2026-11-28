# Roadmap: schwaystation

## Milestones

- ✅ **v1.0 — PlayStation games play with sound in RetroArch** — Phases 1–6; shipped 2026-11-18. [Archived roadmap](milestones/v1.0-ROADMAP.md) · [requirements](milestones/v1.0-REQUIREMENTS.md) · [audit](milestones/v1.0-MILESTONE-AUDIT.md)
- ✅ **v2.0 — The whole library** — Phases 7–16; shipped 2026-11-26 as package v0.16.0. [Archived roadmap](milestones/v2.0-ROADMAP.md) · [requirements](milestones/v2.0-REQUIREMENTS.md) · [audit](milestones/v2.0-MILESTONE-AUDIT.md)

- 🚧 **v3.0 — Prove a game plays** — Phases 17–19; requirements approved, execution roadmap ready for review.

## v3.0 — Prove a game plays

Prove one complete input-driven game session through the existing library, runner and libretro core. Mandatory
SEED-007 opens the milestone. An owned homebrew disc then proves the integrated boot, disc, input, picture and
sound path in hosted RetroArch with deterministic runner parity. Finally, one owner-selected retail title proves
the same playable threshold in a local RetroArch session. Each requirement belongs to exactly one phase.

This roadmap originally authorized no phase execution: milestone planning ended at roadmap review. On
2026-11-27, the owner separately authorized Phase 17 execution after its plans passed independent review. That
phase-specific authorization covers its nine plans, their required local checks, code review and verification; it
does not authorize shipping or Phases 18–19. The rolling
[product roadmap](PRODUCT-ROADMAP.md) and later milestones remain separate. No physical-console capture is
available or required. A one-title pass does not establish broad compatibility or remove experimental status.

## Phases

- [ ] **Phase 17: A measured and trustworthy playability baseline** - Mandatory SEED-007 tune-up grounds CI, quality and user guidance in current evidence.
- [ ] **Phase 18: An owned game plays in RetroArch CI** - One generated homebrew disc boots, responds to input, moves and sounds, with deterministic runner parity.
- [ ] **Phase 19: One retail game plays locally** - One owner-selected title reaches gameplay in local RetroArch and receives only the compatibility predicates actually measured.

## Phase Details

### Phase 17: A measured and trustworthy playability baseline

**Goal**: Players and maintainers can rely on current installation guidance, scoped compatibility claims and measured performance and CI limits before playability work begins.
**Depends on**: Phase 16 (shipped); first phase of v3.0, mandatory SEED-007
**Requirements**: TUNE-07, TUNE-08, TUNE-09, TUNE-10, TUNE-11
**Success Criteria** (what must be TRUE):
  1. A maintainer can inspect a linked comparison of up to the latest 40 CI runs that identifies matching workflow, matrix and workload fingerprints, critical path, duplicate work, flaky checks and core use; unmatched runs carry no speedup claim.
  2. A maintainer can trace each changed timeout to twice the slowest comparable run and its slowest matching job, and each flaky-check change to a fixed cause or a named stronger replacement signal.
  3. A user can identify one verified improvement to the weakest dimension in the linked 1–5 review of performance, CI/CD, ease of use, architecture, documentation, upgrade path and compatibility breadth.
  4. Published quality bounds match remeasured existing producers, and low-power guidance distinguishes measured results from device assumptions without guessed budget changes.
  5. A user can follow current install, hardware and maintainer guidance; the linked audit covers repository settings, Dependabot, runner images and tool pins, with refreshed roadmap/prompt coverage and exported lessons, and retains unrelated pins and caches unless evidence supports a change.
**Plans**: 9 plans across 7 waves
Plans:
**Wave 1**
- [ ] 17-01-PLAN.md — Establish the existing CMake/inventory path and audit installation guidance

**Wave 2** *(blocked on Wave 1 completion)*
- [ ] 17-02-PLAN.md — Compare bounded CI runs and reliability evidence
- [ ] 17-03-PLAN.md — Remeasure quality producers and retain only supported bounds
- [ ] 17-04-PLAN.md — Audit repository policy, runners, tools and update settings

**Wave 3** *(blocked on Wave 2 completion)*
- [ ] 17-05-PLAN.md — Re-evaluate the BE32 shard candidate with its full preservation contract

**Wave 4** *(blocked on Wave 3 completion)*
- [ ] 17-06-PLAN.md — Score all seven dimensions and select one bounded improvement

**Wave 5** *(blocked on Wave 4 completion)*
- [ ] 17-07-PLAN.md — Implement and verify only the evidence-selected improvement

**Wave 6** *(blocked on Wave 5 completion)*
- [ ] 17-08-PLAN.md — Refresh only guidance whose drift is reproduced

**Wave 7** *(blocked on Wave 6 completion)*
- [ ] 17-09-PLAN.md — Refresh roadmap/lessons and reconcile phase evidence

### Phase 18: An owned game plays in RetroArch CI

**Goal**: A player can boot an owned homebrew disc through OpenBIOS in RetroArch, change gameplay with a controller and receive moving video and audible sound, reproducibly matching the runner.
**Depends on**: Phase 17
**Requirements**: PLAY-01, PLAY-02
**Success Criteria** (what must be TRUE):
  1. The hosted RetroArch job loads the generated, self-authored bootable disc through the normal OpenBIOS handoff and disc-access path and reaches interactive homebrew gameplay in one bounded session.
  2. Scripted controller input changes gameplay during that session, the picture changes over time and audible output is present; a boot screen or separate subsystem checks cannot satisfy the integrated result.
  3. Repeating the same guest and input sequence produces the same named video and audio results, and the RetroArch results agree with the runner's reference for that guest and those inputs.
**Plans**: TBD

### Phase 19: One retail game plays locally

**Goal**: A player can reach input-driven gameplay with visible motion and audible sound in one owner-selected retail title through local RetroArch, while public compatibility reporting states only the measured result.
**Depends on**: Phase 18
**Requirements**: RETAIL-01, COMPAT-04
**Success Criteria** (what must be TRUE):
  1. One owner-selected retail title loads through RetroArch's normal content path on the owner's machine using the owner's BIOS and disc and reaches interactive gameplay; input changes gameplay, the picture changes over time and audible output is present in that session.
  2. The retained result contains only the title identity, predicate results and date permitted by ASSET_POLICY.md, scoped to the exact profile and media measured; commercial bytes, captures and private file names and paths stay local and never enter CI, the repository or releases.
  3. Compatibility reporting leaves every untested title or combination unknown and keeps libretro experimental until every required-capability row passes, regardless of the one-title result.
**Plans**: TBD

## Execution Constraints

- Plan 1 of each phase establishes its CMake skeleton and per-directory test inventories. Split work that would
  exceed 30 minutes into smaller plans; use parallel waves only where inventories and headers are disjoint.
  Detailed plan counts and tasks are deferred to the authorized phase-planning step.
- Every behavior change has a gating check in `cmake --workflow --preset ci`. Review code before final
  verification, retain all required cases/platforms and use the cheapest relevant preflight before long runs.
- Reuse the existing RetroArch path and fixture tools. Real-host RetroArch test code remains at or below 300
  lines total. Phase 17's repository-check ceiling is 4,694 lines and ratchets to its measured total at ship;
  a later phase declares its absolute ceiling in REQUIREMENTS.md before Plan 1 if it needs a check change.
- Owned fixtures remain self-authored source with generated bytes handled under ASSET_POLICY.md, including its
  size, build-storage and distribution limits. CI uses OpenBIOS and never depends on an owner's BIOS or disc.
- The retail session is local only. Retained identity facts are limited to the allowed serial, public rcheevos
  MD5, public Redump track SHA-1 and DAT title, plus measured predicates and date. Do not retain commercial
  screenshots, recordings, saves, RAM, traces or TTY output, or hashes of unlisted files. No console capture is
  planned. Ask for owner-machine action only when it is actually required under the standing automation policy.
- Repair reproducible faults needed for these proofs with gating regressions. Broader accuracy campaigns,
  states/movies, browser UI, tools, ports/JIT and enhancements remain outside this milestone. One retail title
  result does not complete the required-capability matrix.
- Once execution is separately authorized, ship and release each phase before advancing under AGENTS.md and
  AUTONOMOUS.md. Published release checks belong to ship, not phase success criteria. Stop at the authorized
  milestone boundary; this roadmap does not start a later milestone.

## Progress

**Execution order:** Phase 17 → Phase 18 → Phase 19. Phase 17 planning is complete; its execution was separately authorized by the owner on 2026-11-27.

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 17. A measured and trustworthy playability baseline | 3/9 | Executing | - |
| 18. An owned game plays in RetroArch CI | 0/TBD | Not started | - |
| 19. One retail game plays locally | 0/TBD | Not started | - |

**Coverage:** All 9 approved v3.0 requirements map to exactly one phase; no orphaned or duplicate assignments.

## Shipped phases

<details>
<summary>✅ v1.0 — Phases 1–6 (shipped 2026-11-18)</summary>

- [x] Phase 1: A test frame in RetroArch — 15 plans
- [x] Phase 2: The CPU matches the public vectors — 27 plans
- [x] Phase 3: The GTE matches the hardware cases; DMA and timers — 26 plans
- [x] Phase 4: Games play in RetroArch — 58 plans
- [x] Phase 5: The sound matches the console — 24 plans
- [x] Phase 6: Discs and FMV — 45 plans

</details>

<details>
<summary>✅ v2.0 — The whole library, Phases 7–16 (shipped 2026-11-26)</summary>

- [x] Phase 7: Faster and more trustworthy CI — 10 plans; 2026-11-19
- [x] Phase 8: Per-game card saves — 10 plans; 2026-11-19
- [x] Phase 9: CHD disc images — 15 plans; 2026-11-20
- [x] Phase 10: PBP disc images — 13 plans; 2026-11-22
- [x] Phase 11: ECM and CloneCD images — 15 plans; 2026-11-22
- [x] Phase 12: SBI and LibCrypt — 7 plans; 2026-11-23
- [x] Phase 13: Regions, console revisions and playback — 9 plans; 2026-11-23
- [x] Phase 14: Multitap and special controllers — 16 plans; 2026-11-24
- [x] Phase 15: GunCon and Justifier — 12 plans; 2026-11-25
- [x] Phase 16: Patches, cheats and compatibility evidence — 18 plans; 2026-11-26

</details>
