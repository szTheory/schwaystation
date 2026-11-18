# Roadmap: schwaystation

## Overview

v2.0 expands the PlayStation library that the C library, headless runner and libretro core can play. The milestone opens with a measured pipeline tune-up, then delivers game-specific card saves, more disc formats, SBI support, regional and revision behavior, more controllers, and patch and cheat support. Compatibility claims come from reproducible results on named inputs. The [v1.0 roadmap](milestones/v1.0-ROADMAP.md), [requirements](milestones/v1.0-REQUIREMENTS.md), and [audit](milestones/v1.0-MILESTONE-AUDIT.md) record the six released predecessor phases and the accepted MDEC evidence limits.

## v2.0 The whole library

**Milestone Goal:** Make more PlayStation games playable through the existing deliverables, with dependable saves, disc handling and input, while preserving deterministic operation and making the extent of verified support visible.

## Phases

- [ ] **Phase 7: Faster and more trustworthy CI** - Measure the build and release pipeline, fix the weakest quality dimension, and establish reproducible quality baselines.
- [ ] **Phase 8: Per-game card saves** - Use separate saves per game, move cards between supported formats, and resume a game in the runner or libretro.
- [ ] **Phase 9: CHD disc images** - Open supported CHD images with equivalent disc behavior and establish the shared media checks and feature-ledger workflow.
- [ ] **Phase 10: PBP disc images** - Open the verified unencrypted PSISO layouts and reject unsupported PBP variants clearly.
- [ ] **Phase 11: ECM and CloneCD images** - Read ECM and CCD/IMG/SUB images with their correct sector and track data.
- [ ] **Phase 12: SBI and LibCrypt** - Serve bad Q responses for a matching SBI sidecar while keeping sector reads intact.
- [ ] **Phase 13: Regions, console revisions and playback** - Select supported hardware profiles, use the opt-in BIOS shortcut, and verify bounded FMV behavior.
- [ ] **Phase 14: Multitap and special controllers** - Play with tap controllers and cards, mouse, neGcon, Jogcon and Dual Analog flight mode.
- [ ] **Phase 15: GunCon and Justifier** - Aim and fire with deterministic beam sensing and documented libretro inputs.
- [ ] **Phase 16: Patches, cheats and compatibility evidence** - Apply supported modifications and publish an evidence-backed account of library and core support.

## Phase Details

### Phase 7: Faster and more trustworthy CI

**Goal**: Maintainers can see where CI time goes, find work that needs attention, and verify that the pipeline and published guidance meet measured quality targets.
**Depends on**: Released v1.0 baseline
**Requirements**: TUNE-01, TUNE-02, TUNE-03, TUNE-04, TUNE-05, TUNE-06
**Success Criteria** (what must be TRUE):
  1. A weekly workflow produces a sanitized triage artifact covering open issues and current run status. Its GitHub access is read only for project data and it sends no messages.
  2. A CI report compares up to 40 matching runs and shows the critical path, duplicate work, core usage and caching evidence. It links each timeout to twice the slowest matching duration and its run ID, records a fix or stronger replacement signal for each flaky check, and reports speed changes only for comparable workloads with required coverage intact.
  3. The seven-dimension quality review links supporting evidence, identifies the weakest dimension, and verifies the result of one change targeting it.
  4. The checked-in `quality.json` baselines cover instructions per frame, Wasm size, state bytes and comparable CI critical-path time. CI enforces the applicable bounds, and the lesson uniqueness check rejects both duplicate identities and duplicate lesson text.
  5. Maintainers can trace current settings, pin and runner checks, device and documentation budgets, install instructions, and roadmap and prompt coverage to evidence. The tune-up records why any measured pin, cache or runner change was made.

**Plans**: 5/10 plans executed

Plans:
**Wave 1**
- [x] 07-01-PLAN.md — Register gating triage, lesson and measured-bound controls
- [x] 07-02-PLAN.md — Audit comparable CI runs, flakiness and timeouts

**Wave 2** *(blocked on Wave 1 completion)*
- [x] 07-03-PLAN.md — Apply the revised gate cap, measure quality inputs and audit settings, runners and pins

**Wave 3** *(blocked on Wave 2 completion)*
- [x] 07-04-PLAN.md — Score quality dimensions and choose one improvement
- [x] 07-05-PLAN.md — Publish the sanitized weekly triage artifact

**Wave 4** *(blocked on Wave 3 completion)*
- [ ] 07-06-PLAN.md — Avoid repeat release-metadata PR work and apply measured timeouts

**Wave 5** *(blocked on Wave 4 completion)*
- [ ] 07-07-PLAN.md — Enforce instruction, Wasm and serialized-state bounds

**Wave 6** *(blocked on Wave 5 completion)*
- [ ] 07-08-PLAN.md — Enforce measured CI critical-path time

**Wave 7** *(blocked on Wave 6 completion)*
- [ ] 07-09-PLAN.md — Refresh installation, maintainer guidance and coverage records

**Wave 8** *(blocked on Wave 7 completion)*
- [ ] 07-10-PLAN.md — Assemble focused review, verification and release evidence

**UI hint**: no
**Canonical refs:** `.planning/seeds/SEED-007-milestone-tune-up.md`, `.planning/preparation/ENGINEERING.md`, `.planning/preparation/CONFORMANCE.md`, `.planning/preparation/PROMPT-COVERAGE.md`, `.planning/AUTONOMOUS.md`

### Phase 8: Per-game card saves

**Goal**: Players can use independent memory cards per game, import and export supported card files, and recover their progress after reopening the emulator.
**Depends on**: Phase 7
**Requirements**: SAVE-01, SAVE-02, SAVE-03, SAVE-04, SAVE-05
**Success Criteria** (what must be TRUE):
  1. Normalized serials produce stable save names, discs in a declared set share the intended identity, and missing or invalid serials use a distinct content-derived fallback.
  2. Raw and supported `.gme`, `.mem` and `.psx` cards round trip by validated contents with identical card bytes. Invalid inputs leave both the active card and the stored file intact.
  3. The runner reloads saved progress in a new process, keeps two games and both slots separate, and retains the previous complete save after a failed write.
  4. The libretro host exposes and reloads card 1 via `RETRO_MEMORY_SAVE_RAM`, passes the in-process and fresh-process hosted RetroArch round trips, keeps additional slots separate in the host save directory, and advertises the save capability once verified.
  5. A guest using documented PocketStation storage commands from `58h` onward can write and read its card data.

**Plans**: TBD
**UI hint**: no
**Canonical refs:** `.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md`, `.planning/preparation/HOSTS-AND-INTEGRATION.md`, `.planning/preparation/ARCHITECTURE.md`, `.planning/preparation/CONFORMANCE.md`

### Phase 9: CHD disc images

**Goal**: Players can open the supported CHD subset with the same behavior as the corresponding uncompressed discs.
**Depends on**: Phase 8 for content identity
**Requirements**: MEDIA-01, MEDIA-02, CHD-01, COMPAT-02
**Success Criteria** (what must be TRUE):
  1. A `SCHWAYSTATION_CHD=ON` build opens the documented CHD v1–v5 subset. The fixture under 512 KiB yields matching identity, TOC, raw sectors, applicable subcode and hashes on all eight applicable lanes. The default stays off until that parity is verified.
  2. Both CHD build configurations expose accurate `.info` extensions and archive notices, with the reader sourced from the pinned libchdr BSD-3 closure.
  3. The shared media checks bound counts, offsets and allocations and confine companion paths. Malformed CHD inputs have fuzz and regression coverage. The preflight verifies track modes, indexes, pregaps, leadout and boundary sectors before guest boot, and invalid images leave the active disc intact. Following providers apply this contract and add their own regression cases.
  4. The feature ledger records the CHD evidence. Its preflight rejects missing or duplicate keys and its protected ratchet preserves prior passes except where a dated allowance permits change. Subsequent providers, profiles, protection and peripherals add their rows during their own phases.

**Plans**: TBD
**UI hint**: no
**Canonical refs:** `.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md`, `.planning/preparation/ENGINEERING.md`, `.planning/preparation/CONFORMANCE.md`, `.planning/research/STACK.md`, `PROVENANCE.md`

### Phase 10: PBP disc images

**Goal**: Players can open documented unencrypted PlayStation PBP images and get a clear result for other variants.
**Depends on**: Phase 9 for the shared media checks and feature ledger
**Requirements**: PBP-01
**Success Criteria** (what must be TRUE):
  1. The runner and libretro host read the verified unencrypted PSISO layouts and track variants with matching sectors and deterministic hashes. The provider functions with CHD disabled and without system zlib, and the bundled inflater carries its permissive notice.
  2. Bad PSISO offsets, track tables, sizes and truncated blocks fail at the media boundary. Encrypted PGD and undocumented layouts return a clear unsupported result before guest execution. The ledger records the variants exercised.

**Plans**: TBD
**UI hint**: no
**Canonical refs:** `.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md`, `.planning/preparation/HOSTS-AND-INTEGRATION.md`, `.planning/research/STACK.md`, `.planning/research/FEATURES.md`, `PROVENANCE.md`

### Phase 11: ECM and CloneCD images

**Goal**: Players can open ECM and CloneCD images while retaining their documented track, sector and subcode behavior.
**Depends on**: Phase 9 for the shared media checks and feature ledger
**Requirements**: ECM-01, CCD-01
**Success Criteria** (what must be TRUE):
  1. Supported ECM files produce the expected sector bytes on repeated and out-of-order reads. Source extents and sector counts are checked before guest runs, and malformed tables, offsets and truncated streams fail within documented bounds.
  2. Supported CloneCD fixtures preserve modes, indexes, pregaps, subcode and first and last sector reads through CCD and its IMG/SUB companions. Invalid companion paths are rejected before an external file is opened.
  3. The ledger records fixture results for both providers against the existing disc contract.

**Plans**: TBD
**UI hint**: no
**Canonical refs:** `.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md`, `.planning/preparation/HOSTS-AND-INTEGRATION.md`, `.planning/preparation/CONFORMANCE.md`, `.planning/research/FEATURES.md`

### Phase 12: SBI and LibCrypt

**Goal**: With a matching SBI sidecar, supported protected discs receive the required subchannel responses.
**Depends on**: Phases 8–11 for disc identity and provider track layouts
**Requirements**: CRYPT-01
**Success Criteria** (what must be TRUE):
  1. Each documented sidecar case binds SBI records to the selected disc identity and LBA map, including after disc selection changes. A malformed or mismatched sidecar cannot mark another disc's sectors as bad Q.
  2. A short CD command sequence verifies that a listed bad Q sector still delivers its data and GetlocP returns the previous valid position. SBI fuzz and seed replay cover malformed inputs, and the ledger records only the evaluated protection predicates.

**Plans**: TBD
**UI hint**: no
**Canonical refs:** `.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md`, `.planning/preparation/HOSTS-AND-INTEGRATION.md`, `.planning/preparation/CONFORMANCE.md`

### Phase 13: Regions, console revisions and playback

**Goal**: Players can select documented hardware profiles, opt in to the eligible BIOS-shell shortcut, and play supported FMV cases within verified bounds.
**Depends on**: Phase 9 for the feature ledger and v1.0 for the existing GPU, timing and MDEC paths
**Requirements**: PROFILE-01, PROFILE-02, BOOT-01, MDEC-01
**Success Criteria** (what must be TRUE):
  1. PAL frame timing and audio cadence follow the documented integer ratios and agree across applicable lanes. Region selection and overrides distinguish region, board and title serial, with the existing NTSC paths covered by regression checks.
  2. Users can select each documented legal board and GPU tuple. Illegal tuples are rejected, and behavior-affecting fields participate in validation, existing state handling and deterministic hashes. Compatibility rows identify tuples actually exercised.
  3. The ordinary BIOS boot path remains the default. The opt-in `fast` runner and core paths require a supplied BIOS and skip only the documented eligible shell handoff, with the same emulated clock settings and timing rules.
  4. Targeted FMV checks run a source-unit preflight before guest execution and report input coverage, decoder output, guest completion and display results separately. The exact v1.0 4-bit expected output, deterministic 8-bit baseline and 4,915,200-cycle bound are preserved.

**Plans**: TBD
**UI hint**: no
**Canonical refs:** `.planning/preparation/PSX-SYSTEM.md`, `.planning/preparation/PSX-GPU-AND-GTE.md`, `.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md`, `.planning/preparation/HOSTS-AND-INTEGRATION.md`, `.planning/milestones/v1.0-MILESTONE-AUDIT.md`

### Phase 14: Multitap and special controllers

**Goal**: Players can use the multitap on either port and use the PlayStation mouse, neGcon, Jogcon and Dual Analog flight mode.
**Depends on**: Phases 8 and 9 for card persistence and feature evidence
**Requirements**: SIO-01, SIO-02, SIO-03, SIO-04
**Success Criteria** (what must be TRUE):
  1. A tap on either port independently selects four controller and card slots. Fixtures show documented lengths, acknowledgements, interrupts and empty slots, and tap cards round trip without cross-slot changes.
  2. Serial and libretro input fixtures reproduce the mouse's frame-latched movement, buttons, saturation and reset behavior.
  3. neGcon and Jogcon report documented serial modes and integer controls, and explicitly reject unsupported modes.
  4. The size-tagged input and libretro mappings carry Dual Analog flight axes and buttons. Existing digital-pad and DualShock callers still pass their compatibility checks. Ledger rows record the exercised controller modes.

**Plans**: TBD
**UI hint**: no
**Canonical refs:** `.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md`, `.planning/preparation/HOSTS-AND-INTEGRATION.md`, `.planning/preparation/ARCHITECTURE.md`

### Phase 15: GunCon and Justifier

**Goal**: GunCon and Justifier trigger, aim and reload behave deterministically using the emulated beam and available host input.
**Depends on**: Phases 13 and 14 for display timing and the controller input boundary
**Requirements**: GUN-01, GUN-02, SIO-05
**Success Criteria** (what must be TRUE):
  1. GunCon and Justifier protocol fixtures reproduce bright, dark, edge and offscreen cases and the corresponding trigger, reload, coordinates and interrupt timing on a documented brightness threshold and GPU beam timestamp.
  2. Supported libretro lightgun or pointer input maps to the expected game inputs. A host missing the required input reports it as unsupported without registering false hits. These checks use synthetic input and displays.
  3. The public mapping covers every delivered controller class with official device and subclass identifiers, frame-latched input and a reproducible integration check. The ledger includes gun evidence.

**Plans**: TBD
**UI hint**: no
**Canonical refs:** `.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md`, `.planning/preparation/PSX-GPU-AND-GTE.md`, `.planning/preparation/HOSTS-AND-INTEGRATION.md`, `.planning/preparation/CONFORMANCE.md`

### Phase 16: Patches, cheats and compatibility evidence

**Goal**: Players can use supported disc patches and cheat strings, and can see which library claims the shipped evidence supports.
**Depends on**: Phases 8–15 for media, saves, profiles, protection and controllers
**Requirements**: PATCH-01, CHEAT-01, COMPAT-01, COMPAT-03
**Success Criteria** (what must be TRUE):
  1. The runner applies supported PPF 1, 2 and 3 and BPS patches deterministically to sector reads. It validates identity and bounds before use, preserves the original disc image, and rejects an invalid patch without exposing partial results.
  2. The runner and libretro host apply the documented bounded GameShark subset through the supported write operation. Invalid codes leave state intact and the applicable state and hash checks account for active cheats.
  3. The generated compatibility report accounts for all 13,407 entries at the pinned, licensed DAT revision and links supported pass and fail results to evidence. Untested entries and combinations remain explicitly unknown.
  4. `is_experimental` is false only when the committed required-capability matrix passes every row, with zero unknown or failing required rows. It stays true when that evidence is missing or fails. The matrix covers the specified boot-to-play disc families, region and board tuples, peripheral classes, card persistence, patches and cheats. DAT-wide playability is outside that claim.

**Plans**: TBD
**UI hint**: no
**Canonical refs:** `.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md`, `.planning/preparation/HOSTS-AND-INTEGRATION.md`, `.planning/preparation/CONFORMANCE.md`, `.planning/preparation/PSXDEV-EXTRACTS.md`, `PROVENANCE.md`, `ASSET_POLICY.md`

## Progress

Phases ship in numeric order. Dependency lines identify the implementation inputs each phase uses. Each phase follows its own `phase/NN-slug` branch and pull request, from discussion through planning, execution, code review and automated verification. After `$gsd-ship N`, confirm the merge, green main CI and the automatic release before the next phase. Follow [standing autonomy and delivery instructions](AUTONOMOUS.md) and the [requirements budgets](REQUIREMENTS.md), including the 3,300-line outer repository-check limit ratcheted to measured totals and the 300-line real-host test limit. Run the cheapest relevant consistency check before builds, guest runs or hosted workflows. After the final release, audit, complete and archive v2.0.

| Phase | Plans Complete | Status | Completed |
|---|---|---|---|
| 7. Faster and more trustworthy CI | 5/10 | In Progress | - |
| 8. Per-game card saves | 0/TBD | Not started | - |
| 9. CHD disc images | 0/TBD | Not started | - |
| 10. PBP disc images | 0/TBD | Not started | - |
| 11. ECM and CloneCD images | 0/TBD | Not started | - |
| 12. SBI and LibCrypt | 0/TBD | Not started | - |
| 13. Regions, console revisions and playback | 0/TBD | Not started | - |
| 14. Multitap and special controllers | 0/TBD | Not started | - |
| 15. GunCon and Justifier | 0/TBD | Not started | - |
| 16. Patches, cheats and compatibility evidence | 0/TBD | Not started | - |
