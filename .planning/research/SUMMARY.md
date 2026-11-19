---
title: "Research summary: schwaystation v2.0 — The whole library"
summary: "Synthesis of the v2.0 stack, feature, architecture, and risk research for roadmap creation."
updated: 2026-11-18
---

# Research Summary

## Executive Summary

v2.0 extends the existing deterministic C17 PlayStation library, headless runner, and libretro core so more of the game library works through the same three deliverables. The research recommends keeping file parsing, decompression, persistence, and patch handling in host helpers; the core continues to receive BIOS, sectors, and integer input through its existing boundaries. Keep new hardware behavior in small per-device C modules and per-instance state. The only new runtime code dependency recommended is an optional, default-off, BSD-3-Clause libchdr build for CHD, plus MIT-licensed miniz source so PBP remains available when CHD is disabled.

Start with the required SEED-007 tune-up to measure CI cost, quality, and documentation freshness before the larger feature work. Then stabilize content identity and card persistence, add media providers with bounded format-specific fixtures, implement protection and peripheral protocols, and expand profiles and the compatibility ledger as evidence arrives. Do not infer playability from parsing or from a Redump inventory. Risks concentrate in hostile container sizes, losing CD track/subchannel semantics, incorrect LibCrypt bad-Q behavior, state/hash drift, and license closure. Use cheap source, count, boundary, and protocol preflights before expensive builds and guest runs; retain them as CI gates only when they prevent a named recurring fault.

This is a breadth milestone, not a measured-accuracy or presentation milestone. The accepted v1.0 boundary still excludes proven exact 8-bit MDEC output and complete external frame/movie conformance. v2.0 must not claim those results. The owner approved the full SEED-001 direction; its fast boot and Dual Analog flight-mode items are included, along with targeted MDEC edge checks that preserve the accepted evidence boundary. Save states, movies, and the browser remain outside v2.0.

## Key Findings

### Stack

- Keep the v1 C17, CMake/CTest, libretro, host-supplied sector, and deterministic hash contracts. No floating point, file access, or environment access enters the core.
- Use libchdr v0.3.0 at the approved public pin as an optional static host-side CHD backend behind SCHWAYSTATION_CHD=ON, default OFF. Enable raw sectors/subcode and CRC checking; preserve all applicable license notices. Do not select GPL-2 Zstandard terms or libchdr's optional C++ micro-flac path.
- Vendor only the miniz 3.1.2 inflater source needed for PBP, independently of the CHD option, so PBP has no system-zlib requirement. Include its MIT notice in all archives. Keep ECM, CCD/SUB, SBI, cards, patches, cheats, and controller protocols in bounded in-tree C modules.
- For SEED-007, use existing CMake string(JSON), POSIX shell, and GitHub CLI. Keep tool pins unless a measured comparison justifies an update. Do not add Python, Node, a dashboard, cache service, or a package manager; measure compile share and critical path before deciding on caching.
- Tradeoff: vendoring a small inflater and optional CHD closure adds source and notice maintenance, but avoids platform-specific codec dependencies and keeps the core/API unchanged.

### Features

**Table stakes:** CHD v5, supported PBP variants, ECM, CCD/IMG/SUB, SBI/LibCrypt, region and board/GPU profiles, per-game cards with raw and approved wrapper import/export, libretro SaveRAM round-trip, GameShark strings, runner PPF/BPS patching, and capability metadata that changes only after the ledger criterion passes.

**Differentiators in the approved direction:** multitap and tap cards; GunCon and Justifier; mouse, neGcon, Jogcon and Dual Analog flight mode; PocketStation storage compatibility. These are serial protocols or storage behavior, not physical-device or full PocketStation emulation. Dual Analog is included explicitly in v2.0 requirements even though the feature survey did not cover it separately.

**Keep bounded or defer:** PBP support should name the tested unencrypted PSISO layout; PGD-encrypted payloads and undocumented audio/multi-disc variants need evidence before inclusion. Keep full PocketStation CPU/BIOS/LCD/RTC/IrDA behavior, adjacent media formats, cheat databases/UIs, physical devices, and unbounded scans out of scope. A 13,407-entry DAT is an inventory, not evidence that every disc was booted or mapped to each board.

### Architecture

- Normalize every supported disc into the existing schway_disc contract: raw sector, TOC, subcode, identity, and optional bad-Q status. Keep format parsing, paths, overlays, and I/O in host/; do not teach the core about containers or files.
- Preserve append-only, size-tagged public input/profile structures. Resolve region separately from board revision; add only profile fields with a real consumer. Include behavior-affecting profile, disc, patch, cheat, and protocol state in instance validation, serialization, and hashes as applicable.
- Keep byte timing and protocol state in SIO0 and small device modules. Host adapters translate controls into frame-latched integer input. Gun behavior needs a deterministic GPU beam/brightness query; input-coordinate availability varies by frontend.
- Keep card bytes and emulated serial behavior in the instance; host helpers validate wrappers, name files, load before the first frame, and flush changes. Decide multi-disc/set identity and slot naming before per-game persistence. Preserve card 1 as libretro SAVE_RAM; manage additional cards in the save directory.
- Apply PPF/BPS after media decoding as host-side overlays. GameShark text parsing stays host-side, but deterministic writes need a narrow explicit core API; never mutate the read-only memory span.
- Treat the compatibility ledger as evidence, with pinned identities and explicit pass/unknown outcomes. Add rows with each feature; only the final measured threshold can support stable .info metadata claims.

### Pitfalls and Prevention

1. **Hostile media sizes, offsets, and paths:** checked arithmetic and strict limits must precede allocation or decode. Confine CCD companion paths. Count tracks/sectors and read track boundaries plus the final LBA before a long guest run.
2. **Losing track or subchannel meaning:** preserve audio, pregap/index, leadout, sector type, and subcode through each provider. Test representative sectors and Q at boundaries; a bad Q is not a failed data-sector read.
3. **Misimplementing LibCrypt:** bind SBI to the selected disc and LBA map, model bad-Q independently from sector payload, and verify the previous-good-position/GetlocP response before protected-title runs.
4. **License or build-closure mistakes:** audit the exact pinned source tree and per-file notices, use only the BSD-3 option for Zstandard, keep default builds offline, and verify the actual release archive's notices.
5. **Nondeterminism and persistence drift:** keep region and board distinct, and validate split-run, save/load/continue, and cross-lane state/hash behavior for new timing/profile fields. Test card import non-mutation, two-slot independence, and fresh-process persistence; do not use save-state tests as a persistence substitute.
6. **Misleading quality or compatibility claims:** compare equivalent workload/toolchain fingerprints, protect ledger denominators, and leave metrics descriptive until baselines match. Treat issue/log text as untrusted and redact private data from the digest.

## Implications for Roadmap

The following is an ordering recommendation, not a claim that all title/profile combinations are understood. Each feature phase should add its focused fixtures and ledger evidence. Reuse the current host/core seams and keep phases small enough to release independently.

| Suggested phase | Delivers and dependencies | Rationale, cost, and main guard |
|---|---|---|
| **1. SEED-007 tune-up** | Weekly triage digest, measured CI-efficiency report, quality.json ratchets, policy.lessons_unique, refreshed pins/docs/device budgets. No feature dependency. | Mandatory first: establishes comparable baselines before v2.0 adds CI load. **Medium cost.** Use existing tools; report matched workloads, keep the digest read-only, and measure before changing caching or parallelism. |
| **2. Content identity and card lifecycle** | Stable single-disc, multi-disc, and unknown-identity policy; raw and .gme/.mem/.psx import/export; per-game cards; libretro SaveRAM/fresh-load round trip; PocketStation storage only. | Establishes the identity and persistence contract reused by new media and multitap cards. **Medium cost.** Keep bytes lossless and host-owned; test two slots, malformed-import non-mutation, atomic flush, and a fresh process. |
| **3. CHD provider** | Optional CHD v1–v5 backend through schway_disc, fixture parity, feature-gated .info extension. | CHD is the high-demand format and its provider exercises the media contract. **High cost.** Requires pinned license closure, bounded allocation/metadata, and a sub-512 KiB fixture with equivalent sectors and hashes before considering default-on support. |
| **4. PBP provider** | Bounded PSISO directory/TOC and DEFLATE sectors for the explicitly supported variant matrix. | Separate because layout, encryption, audio, and multi-disc behavior are the least settled. **High cost / highest format uncertainty.** Use miniz independently; reject unsupported PGD and undocumented variants instead of inferring support. |
| **5. ECM and CCD/IMG/SUB providers** | Indexed or bounded-decoded ECM sectors; CCD track metadata and IMG/SUB reads. | Completes the named disc formats after the common provider and identity seams are proven. **Medium-high cost.** Never rescan the whole ECM stream for every LBA; validate paths, track/pregap data, subcode, and first/last sector. |
| **6. SBI and LibCrypt** | Sidecar-to-disc binding, bad-Q callback use, previous-good GetlocP behavior, protected-title predicates. | Depends on stable selected-disc identity and media TOCs. **High cost.** Keep Q validity separate from payload delivery and prove a short CD command sequence before any long title run. |
| **7. PAL and board/GPU profiles** | Explicit legal region/board tuples, documented per-revision behavior, profile-specific ledger rows. | Independent of media codec work and should not be inferred from a title serial or from the Redump DAT. **High cost.** Test each declared tuple and hash/state inclusion; claim only behaviors with evidence. |
| **8. Non-gun SIO peripherals** | Multitap on both ports, tap-addressed cards, mouse, neGcon, Jogcon, and the seed's Dual Analog flight mode. | Depends on the append-only frame-input seam and card slot mapping. **Medium-high cost.** Keep device byte counts, ACK/IRQ timing, config, abort, and empty-slot behavior in protocol vectors before game tests. |
| **9. Light guns** | GunCon and Justifier with host input mapping, beam timing, brightness threshold, and interrupt behavior. | Separate because these depend on GPU sensing and timer behavior, not only a new SIO report. **High cost / host-dependent.** Test synthetic bright/dark/offscreen/edge coordinates; document frontends without lightgun input. |
| **10. ROM-hacker support and capability closeout** | PPF1/2/3 and BPS overlays, bounded GameShark parsing with an explicit deterministic core write seam, final ledger threshold review, stable .info metadata only if it passes. | The patches need the completed media representation; cheats need a real, narrow RAM-write consumer. **High cost.** Validate source identity/CRCs and immutable inputs, reject unsafe codes, and do not drop experimental status until the numeric criterion and metadata semantics are settled. |

Ledger rows should accrue with the feature that produces the evidence, rather than being deferred to a large final sweep. A final compatibility review can compare the protected denominator and determine whether stable metadata is justified; it must not claim exhaustive DAT playability or a board × region × media cross-product without runs.

### Approved scope and acceptance boundary

- **Fast boot:** included as the existing documented, opt-in BIOS-shell skip at the kernel hand-off. It stays off by default, still requires the user-supplied BIOS, and does not add HLE or alter emulated timing.
- **MDEC edge cases:** included as bounded edge-case behavior and regression coverage from supported FMV inputs. Keep exact 4-bit output and the separate deterministic 8-bit regression; do not change expected hardware bytes, cycle cap, or the v1.0 acceptance boundary. Report compressed-source coverage, decoder output, guest completion, and image result separately. Run the cheap source-count preflight before an expensive guest run; source coverage alone is not output conformance.
- **Dual Analog flight mode:** included as a named SIO peripheral behavior from the approved seed, with its own protocol/input tests.
- **Out-of-scope boundary:** browser UI, save states, movies, measured-accuracy campaigns, JITs, and visual enhancements remain outside v2.0. No phase should silently absorb them.

## Research Flags

**Plan-phase research recommended:** CHD dependency/source closure and target memory behavior; supported PBP variants; ECM random-access/index strategy; CCD path and subcode cases; SBI/LibCrypt title sequences; board/profile behavior; SIO timing and device-specific input mapping; GPU beam/brightness support; GameShark code matrix and explicit write API; the final numeric compatibility threshold and .info semantics. Recheck mutable upstream facts at the implementation pin.

**Patterns already well documented:** CMake JSON and shell/GitHub CLI reporting for the tune-up; the existing schway_disc/size-tagged API boundary; libretro SaveRAM and controller API contracts; per-instance deterministic state and cross-lane hashes. These can skip broad survey research, but still need phase-specific acceptance decisions and tests.

## Confidence Assessment

| Area | Confidence | Notes |
|---|---|---|
| Stack | **High** for the project boundary and audited libchdr pin; **medium-high** for dependency/license closure | Exact source pins and license components are identified. Re-audit if the pin or build options change. |
| Features | **Medium** | Project scope and libretro contracts are clear. PBP variants, Jogcon semantics, GameShark edge rules and the exact board/profile matrix need phase-level evidence. Fast boot, bounded MDEC work, Dual Analog and the conditional compatibility-metadata threshold are resolved in the requirements. |
| Architecture | **High** for host/core, API, and state boundaries; **medium** for undocumented devices/profiles | Existing interfaces are inspected. New format layouts, lightgun sensing, and some profile combinations need evidence. |
| Pitfalls | **Medium-high** | Resource, licensing, determinism, ledger, and CI risks are well grounded. Format prevalence and title-specific behavior remain less certain. |

**Overall: MEDIUM-HIGH.** The implementation shape and project constraints are strong; evidence for several user-visible edge cases remains incomplete.

### Gaps to Address

- Define PBP support and playlist/sidecar precedence, ECM indexing, and SBI lookup across CUE/CCD/CHD and multi-disc sets.
- Validate the requirements' per-game identity fallback and card key behavior for multi-disc titles, unknown serials, tap slots, and libretro card 2 flush lifecycle against the existing host/API seams.
- Name supported Jogcon and GameShark behavior; settle mouse deltas and lightgun input fallback in the public API.
- Specify which board × region tuples and representative titles the ledger actually covers. The DAT provides no board mapping.
- Keep the required-capability matrix small, explicit and evidence-backed; `is_experimental` becomes false only at 100% required-row pass, otherwise it remains true.
- Keep fast boot opt-in and MDEC work bounded to the documented accepted v1.0 evidence boundary; do not expand into a measured-accuracy campaign.
- Define equivalent-input fingerprints for quality and CI comparisons; decide cache or parallelism changes only after measurement.

## Sources

Sources below are aggregated from the four research documents and the required project scope. Format references are evidence for described behavior, not licenses to copy code. Any implementation seed must also pass the project's exact-path provenance checks; prohibited emulator sources remain unopened and unused.

- Project scope and decisions: [PROJECT.md](../PROJECT.md), [PRODUCT-ROADMAP.md](../PRODUCT-ROADMAP.md), [SEED-001](../seeds/SEED-001-whole-library.md), [SEED-007](../seeds/SEED-007-milestone-tune-up.md), [preparation index](../preparation/README.md), [LESSONS.md](../LESSONS.md), and the settled architecture/host/provenance documents in .planning/preparation/.
- CHD: [libchdr v0.3.0 release](https://github.com/rtissera/libchdr/releases/tag/v0.3.0), [approved source tree](https://github.com/rtissera/libchdr/tree/47d794151b2b64f6aba60304e6480f4f5a2bc24b), and its pinned [CMake options](https://github.com/rtissera/libchdr/blob/47d794151b2b64f6aba60304e6480f4f5a2bc24b/CMakeLists.txt), [BSD-3 license](https://github.com/rtissera/libchdr/blob/47d794151b2b64f6aba60304e6480f4f5a2bc24b/LICENSE.txt), [MAME-derived chd.h notice](https://github.com/rtissera/libchdr/blob/47d794151b2b64f6aba60304e6480f4f5a2bc24b/include/libchdr/chd.h), and [Zstandard BSD-3 text](https://github.com/facebook/zstd/blob/v1.5.7/LICENSE). The exact miniz, LZMA, and dr_flac notices are linked in STACK.md.
- Hardware-test evidence: the pinned MIT [ps1-tests repository](https://github.com/JaCzekanski/ps1-tests/tree/f727802fead1) supplies published hardware-test programs and results where applicable. The research does not establish every PBP, Jogcon, protection, or board-profile edge from primary evidence; those remain phase-level validation gaps.
- Host contracts: pinned [libretro API header](https://github.com/libretro/libretro-common/blob/a3194c18ba65/include/libretro.h), official [RetroArch input-controller guide](https://docs.libretro.com/guides/input-controller-drivers/), and the [libretro .info example](https://github.com/libretro/libretro-super/blob/master/dist/info/00_example_libretro.info).
- Patches, inventory, and reporting: reviewed [PPF3 developer text](https://github.com/meunierd/ppf/blob/4f24f8c784fa/ppfdev/PPF3.txt); pinned [Redump DAT snapshot](https://github.com/libretro/libretro-database/blob/12c21cf79f1e/metadat/redump/Sony%20-%20PlayStation.dat) (inventory only); [CMake string(JSON) documentation](https://cmake.org/cmake/help/v3.25/command/string.html#json); [GitHub CLI API manual](https://cli.github.com/manual/gh_api); and [GitHub Actions concurrency documentation](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax#concurrency).

---
*Research synthesis completed: 2026-11-18.*
