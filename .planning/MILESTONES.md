# Project Milestones: schwaystation

## v2.0 The whole library (Shipped: 2026-11-26)

**Phases completed:** 10 phases, 125 plans, 221 tasks

**Key accomplishments:**
- A read-only `gh api` collector and weekly/manual workflow publish bounded, sanitized `triage.json` with seven-day retention.
- Full heavy pull requests now compare the measured CI dependency path only when the current attempt and workload match the checked-in baseline.
- Maintainer docs, quality records and roadmap now reflect the current CI paths and measured values while preserving the hardware and external guest evidence limits.
- Plan 10's focused CI and hygiene controls passed with measured timeout headroom; hosted exact-head CI confirmed the 595,820-byte WASI bound, the phase merged and released as v0.7.0, and the default-branch triage artifact passed inspection.
- Fresh runner processes prove literal serial saves, a PS-EXE guest checks card ID over JOY, and libretro save claims require both card tests.
- Pinned PocketStation command framing and added per-slot metadata commands with validated format-18 state that resumes without replacing card media.
- Libretro keeps card 1 frontend-owned and persists card 2 for EXE, disc, playlist, and fresh-instance reloads.
- Bounded 5Bh/5Ch physical and selected-file transfers, staged writes, and a real JOY-register guest transcript
- The required macOS RetroArch job now downloads the round-trip guest and checks that two fresh frontend processes advance both fixed-size card files.
- Runner serial naming now uses the first disc member's valid serial or a versioned digest of the complete declared media set.
- Format 18's card protocol state now has reproducible GPU motion pins, and the focused save integration checks pass.
- RetroArch resumed both libretro card slots across fresh processes, so the core now advertises its verified save support.
- The pinned CHD fixture creator now has a cleared permissive build closure and an opt-in native CMake target.
- Bounded CHD track metadata now projects the owned image into the exact logical TOC without exposing per-track storage padding.
- The shared disc image provider now reads CHD sectors with CUE-compatible CDDA bytes and complete logical parity evidence.
- CUE and CHD now share checked media bounds, and the production CHD parser has portable replay plus a bounded sanitizer campaign.
- A sorted feature evidence ledger now has a cheap schema preflight, protected fresh-result ratchet, legacy title compatibility, and generated documentation.
- Libretro CHD claims now follow the linked provider, with exact dependency notices and the source-wide SPDX scan deferred to its first tracked caller.
- Measured duplicate CI cost on the exact same source SHA and retained separate release gates because archive identity and a shorter critical path were not established.
- Required CI lanes now build and test CHD from job-local fixtures, and nightly runs a bounded CHD fuzz campaign.
- Runner and libretro now open CHD through the shared provider, with CUE parity, stable runner identity and failure-safe frontend behavior.
- CHD-enabled and disabled package archives now verify their contents, exact notices and configuration-specific libretro metadata.
- The owned boot, read, audio and FMV guests now exercise CHD with the same CUE expectations and time bounds.
- Cross builds now accept a checked native CHD generator while keeping its host-specific provenance out of byte-for-byte fixture comparison.
- A host-only PBP envelope boundary now rejects malformed or unsupported media, with its test target registered while all existing control cases remain intact.
- The PBP host path now has a pinned, privately named MIT inflater and a digest self-test that guards both legacy and new vendored sources.
- PBP now has a private raw inflater wrapper that accepts only complete, exact-length blocks and never exposes partial output.
- The owned single-track source now produces a bounded deterministic PBP, and preflight rejects fixture identity or geometry mistakes before later consumers.
- The host now validates and maps the fixture-proven PSISO directory, TOC and block table before a reader can be published.
- The production PBP reader now has deterministic malformed-input replay and a bounded sanitizer campaign without committed disc bytes.
- CHD-OFF and CHD-ON PBP packages retain exact notices and metadata while the private inflater links without system zlib.
- Nightly now keeps full coverage for uncertain PR changes, while exact PBP-only and docs/planning-only changes avoid workloads they cannot affect.
- The owned one-data-track PBP now passes bounded boot, read/replay and first-frame guest checks, with only that demonstrated subset recorded as compatible.
- The Phase 10 review is clean, malformed-PBP refusal preserves input redaction, and the complete local macOS workflow passes.
- Private ECM/CCD host readers compile, reject malformed input without publishing handles, and have focused portable test coverage.
- Literal ECM images now pass complete stream and EDC validation and support bounded random reads through the host disc callback.
- A supported single-track CCD fixture now opens directly or from an M3U playlist and returns its proven raw sectors and TOC.
- The runner now probes and loads fixture-supported ECM and CloneCD media, with identity checks that account for optional SUB data.
- Libretro now loads fixture-proven ECM and CCD through the shared disc reader, preserves active content after rejected replacements, and advertises only inventory-backed formats.
- Fixture-backed ECM and CloneCD guests now have bounded route coverage and compatibility evidence that stops at verified local behavior.
- A private bounded SBI parser and registered CD-ROM MMIO baseline now cover the first valid format-1 record and good-Q GetlocP path.
- The SBI parser now accepts formats 1–3, validates only the real absolute MSF, and publishes a bounded sorted set of unique bad-Q LBAs.
- Each opened image now owns its validated SBI bad-Q overlay, and M3U selection returns the matching child callback and TOC.
- Bad-Q sectors remain readable without replacing the prior accepted Q position, and a new disc starts with clean position state.
- Selected CCD child SBI sectors stream guest data while GetlocP preserves the previous valid position across a disc swap.
- Seven bounded SBI seeds now exercise the production parser through portable replay and the Nightly libFuzzer campaign.
- One runner PCM capture per region now feeds both callback comparisons while preserving every existing check.
- A fixture-gated movie15 first-frame check records the exact bounded MDEC input-exhaustion state without claiming playback or frame conformance.
- Versioned save states now preserve board identity, restore fixed board defaults, and reject incompatible tuples without mutating the target.
- The runner now selects documented console boards independently from region and resolves automatic region hints without hashing the full disc track.
- Libretro now applies a documented board and resolved region independently at content load, defaulting to PU-18 while retaining PAL/NTSC audio-cadence coverage.
- The core now has an explicit fast-boot profile that returns from the initialized BIOS shell entry while preserving ordinary full boot by default.
- Both shipped hosts now expose full BIOS boot by default and an opt-in fast path for eligible BIOS-backed disc executables.
- The compatibility ledger now records only the canonical machine profile predicates exercised by passing core and libretro checks, and the generated documentation states what those rows do not prove.
- A frozen v8 input caller passes through the public frame latch, and the Phase 14 short peripheral selectors are registered before their implementation plans.
- API v9 adds a bounded 28-byte-compatible input snapshot and eight independently latched controller endpoints under state format 20.
- API v9 exposes eight independent card images, and SIO routes tap card transactions to exactly one selected socket while preserving legacy aliases and save bytes.
- Direct and tap serial endpoints now return bounded mouse and neGcon payloads with the documented IDs, button bits, raw controls and startup/reset behavior.
- Dual Analog flight and Jogcon now report distinct documented poll payloads while existing digital and DualShock behavior stays unchanged.
- Controller and card state restore now validates cross-device transactions before apply and preserves receiver-owned card media across all eight endpoints.
- Socket-aware paths preserve both legacy A filenames and give each added tap card a distinct atomic save file.
- Runner card saves now cover all eight stable port/socket coordinates, and two independent options keep tap selection active across input movie updates.
- The default libretro players now have a focused callback-to-serial regression check using private MMIO inspection.
- Libretro maps active players compactly across either or both taps and keeps every player on its own physical socket.
- Seven libretro adapter sidecars now load and flush independently, with port1A remaining frontend-owned SAVE_RAM.
- Libretro regression checks now prove that state restore, reset, failed loads and disc swaps do not roll back card contents or change their save identity.
- Six compatibility rows now record selector-backed multitap, eight-card, mouse, neGcon, Jogcon, and flight behavior.
- The state-size bound and two protocol-state pins now match measured outputs, and existing guides expose the controller, save, and runner documentation.
- A frozen 228-byte API v9 caller reaches the frame latch, and 12 planned gun selectors are registered in the existing test graph.
- API v10 adds two 16-byte direct-port gun records after the released 228-byte v9 input and validates the normalized snapshot before applying it.
- GunCon input now samples live RGB555/RGB24 display pixels at bounded beam deadlines using the existing GPU scheduler.
- Justifier polling and beam-timed IRQ10 now use the existing SIO, GPU, and timer scheduling paths.
- GunCon and Justifier now use official libretro LIGHTGUN IDs, one frame-latched input snapshot, and explicit fail-closed behavior when host support is missing.
- Libretro now falls back to POINTER when LIGHTGUN is unavailable, while direct-port and frame-boundary tests guard against stale or unsupported gun input.
- Save states resume GunCon polls and GPU beam-sensor deadlines at format 21, with host-pending input excluded.
- API v10 frame behavior is preserved, and the planned patch, cheat, compatibility, and fuzz test graph is registered.
- API v11 exposes checked, tick-neutral writes to physical main RAM while retaining the released v9 and v10 input callers.
- The shared host layer parses bounded GameShark byte and halfword writes, applies them through API v11, and serializes validated cheat state as canonical little-endian bytes.
- Libretro GameShark callbacks now apply validated `30` byte and `80` halfword writes before each frame, with transactional rejection and explicit lifecycle behavior.
- Libretro savestates now preserve the canonical cheat table and reject malformed envelopes without partial mutation.
- The runner now applies bounded repeated GameShark codes at frame start and extends state hashes with normalized cheat configuration while preserving no-cheat hashes.
- PPF1 candidates now validate and overlay an explicitly mapped raw CUE track, with source identity, digest, limits, and failed replacement isolation enforced.
- PPF2 and PPF3 now validate their version-specific native headers and record layouts before the shared raw-track overlay becomes visible.
- BPS1 patches now stage and validate a complete raw-track target, with checked memory or temporary-file storage and transactional rejection.
- The runner now applies one validated PPF or BPS patch before emulation and reports its source checks without exposing input paths.
- Generated-seed replay and libFuzzer coverage now exercise production patch and cheat parsers, with exact PPF and cheat seeds in Nightly.
- The runner now counts every pinned DAT occurrence while keeping titles and serials out of public output and leaving untested entries unknown.
- The library route now validates cached DAT digests and reports pinned-source aggregate counts while keeping the DAT itself out of the repository.

---

## v1.0 PlayStation games play with sound in RetroArch (Shipped: 2026-11-18)

**Delivered:** A deterministic, permissively licensed PlayStation library, runner and libretro core with CPU, GTE,
GPU, SPU, CD-ROM, XA/CD-DA, MDEC and disk-control paths, plus an owned disc that boots through OpenBIOS in
RetroArch.

**Phases completed:** 1–6 (195 plans, 381 tasks)

**Key accomplishments:**
- Shipped the three deliverables and an automated release pipeline across six platforms, plus big-endian and wasm32 lanes.
- Matched the public R3000A vectors and 1,150 hardware GTE cases; integrated DMA and timer behavior.
- Added GPU rendering, display modes, digital and DualShock input, memory cards, OpenBIOS and a compatibility ledger.
- Matched pinned SPU memory-transfer logs and verified deterministic, cross-platform PCM parity between runner and libretro.
- Added bounded cue/bin, ISO and M3U media paths, XA and CD-DA playback, MDEC decoding and disk control; booted the owned `disc_boot` fixture in RetroArch.
- Audited all 30 requirements, six phase verifications and seven cross-phase flows; no critical requirement, integration or user-flow gap remains.

**Phase releases:** [PR #4](https://github.com/szTheory/schwaystation/pull/4), [PR #8](https://github.com/szTheory/schwaystation/pull/8), [PR #12](https://github.com/szTheory/schwaystation/pull/12), [PR #14](https://github.com/szTheory/schwaystation/pull/14), [PR #18](https://github.com/szTheory/schwaystation/pull/18), [PR #22](https://github.com/szTheory/schwaystation/pull/22).

**Package release:** [Release PR #23](https://github.com/szTheory/schwaystation/pull/23) published v0.6.0. The GSD v1.0 milestone is not a semver v1.0 package release; no v1.0 tag was configured.

**Hosted evidence:** [All-platform release-PR CI](https://github.com/szTheory/schwaystation/actions/runs/44253419074), [full reference-vector job](https://github.com/szTheory/schwaystation/actions/runs/44233700921/jobs/134564625732), and [v0.6.0 release workflow](https://github.com/szTheory/schwaystation/actions/runs/44262491280). Exact-source hosted results were reused for the planning-only closeout; no local long suite was rerun.

**Closeout type:** `override_closeout`.

**Known verification overrides:** 6 newly acknowledged, 41 carried forward (see [STATE.md Deferred Items](STATE.md#deferred-items)). The six new records are two pending CI follow-ups, one dormant seed, and three phase-deferred items. Carried-forward suppression records the current audit scan; one item acknowledged at v1.0 is no longer active. Acknowledgment records deferral and does not resolve an item.

**Technical debt and limits:** Exact 8-bit MDEC hardware output and complete external frame/movie conformance are not claimed. The pinned 4-bit guest, a separate deterministic 8-bit regression, direct controls, complete compressed-source preflights and the owned STR path are verified. TODO-051 keeps full Nightly coverage until a safe metadata route fits the fixed gate budget; TODO-052 keeps the BE32 shard pending until exact selector, fixture, JUnit, artifact and hash coverage plus hosted speed evidence exist. The dormant runtime-efficiency seed and three phase-deferred items remain recorded in the audit and state ledger.

**What's next:** No next milestone is active. Future directions remain in `.planning/PRODUCT-ROADMAP.md` and the seed ledger; this closeout does not start v2.0.

---
