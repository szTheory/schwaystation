# Requirements: schwaystation v2.0 — The whole library

**Defined:** 2026-11-18  
**Core Value:** PlayStation games behave as they do on the console, identically on every platform, from a small MIT-licensed C library that any host can embed.

## Milestone contract

v2.0 grows library breadth through the existing library, headless runner and libretro core. Every behavior change has a gating test run by `cmake --workflow --preset ci`; no test is non-gating. Deterministic behavior is checked on the six supported native platforms and the `be32` and `wasm32` hash lanes where the feature applies. No test requires the owner's BIOS, discs, memory cards or physical console. Host-side file parsing, paths and persistence stay outside the deterministic core; core behavior stays integer-only, bounded, per-instance and independent of files, environment and wall clock.

The pinned 13,407-entry Redump DAT is an inventory and source for ledger titles, not evidence that every entry boots or that every board, region and format combination works. Untested titles and combinations remain explicitly `unknown`. No v2.0 claim upgrades the accepted v1.0 MDEC evidence: exact 8-bit hardware output and full external frame/movie conformance remain unproven.

Repository-check code (`scripts/*.sh`, `tests/cmake/*.cmake`, `.githooks/*`) stays at or below **3,901 total lines in every v2 phase**, measured with the phase's declared inventory and ratcheted down to the measured total at ship. On 2026-11-18, Phase 7 measured 3,133 lines after Plan 01 and 3,299 after Plan 06. The original 3,300 estimate covered 90–135 remaining lines, but Plans 05–06 consumed 166 lines and Plans 07–08 still require an estimated 370–500 counted lines for the named production-path and negative controls. The corrected 3,900 ceiling covered the 500-line upper estimate plus 100 lines of headroom; the final Phase 7 total is 3,901 after adding the one-line Windows CRLF normalization in the CI timing producer. Checks remain in the same declared inventory. [Plan 01 measurement](phases/07-faster-and-more-trustworthy-ci/07-01-SUMMARY.md), [original and superseding budget decisions](phases/07-faster-and-more-trustworthy-ci/07-CONTEXT.md).

The real-host RetroArch test code remains at or below **300 lines total**. Every retained gate names the recurring fault it prevents. Before long builds, guest runs or hosted workflows, run a relevant inexpensive input/count/inventory/selector/budget preflight.

## v2 Requirements

### Pipeline, quality and documentation tune-up (SEED-007)

- [x] **TUNE-01**: A scheduled weekly triage job creates a read-only artifact from open project issues and the latest run status; it does not post comments or messages, and it sanitizes untrusted issue/log text and removes private paths and identifiers before publication.
- [x] **TUNE-02**: A CI-efficiency report compares up to the latest 40 runs with matching workflow, matrix and workload fingerprints; it identifies critical-path jobs, duplicated work, flaky checks, core usage and caching evidence. Any optimization retains required checks and states its measured before/after result; if runs are not comparable, the report says so and does not infer a speedup.
- [x] **TUNE-03**: The report records the slowest job and its run ID, reviews every flaky test by naming the stronger signal that replaces it or fixing the cause, and derives job timeouts as twice the slowest matching run with the evidence run ID beside each value.
- [x] **TUNE-04**: A linked 1–5 review covers performance, CI/CD, ease of use, architecture, documentation/README, upgrade path and compatibility breadth; the phase makes one evidence-backed improvement to the weakest dimension and verifies its stated result.
- [x] **TUNE-05**: `quality.json` records the existing Ir/frame, wasm size, serialized-state bytes and CI critical-path measurements, with CI enforcing checked-in ceilings/floors ratcheted to measured values; `policy.lessons_unique` rejects duplicate lesson identities or duplicate lesson text and has a negative control.
- [x] **TUNE-06**: The phase checks repository settings, Dependabot majors, runner images and the existing tool pins; refreshes affected maintainer/hardware docs, README install guidance, size budgets, roadmap and prompt-coverage records; and records which items were unchanged with evidence. No pin, cache or runner change is made without a measured reason.

### Content identity and card lifecycle

- [x] **SAVE-01**: A documented host policy gives stable per-game card names from normalized disc serials; discs in a declared multi-disc set share the intended card identity, while a missing/invalid serial uses an explicit content-identity fallback that cannot silently collide with another game.
- [x] **SAVE-02**: The host imports and exports raw cards and supported `.gme`, `.mem` and `.psx` wrappers by validated content/magic, preserving card bytes exactly; invalid, truncated, oversized or unsupported input leaves the current card and file unchanged.
- [x] **SAVE-03**: Runner serial-based naming and both card slots load and flush through host helpers, with two-game and two-slot isolation and a fresh-process persistence round trip; failed writes do not publish partial card files.
- [x] **SAVE-04**: The libretro core exposes card 1 through `RETRO_MEMORY_SAVE_RAM`, reports `libretro_saves "true"` only with the implemented save path, and passes an in-process plus fresh-process save/load/continue round trip; additional card slots use the host save directory without aliasing card 1.
- [x] **SAVE-05**: The serial storage protocol answers the documented PocketStation memory-card commands from `58h` onward and round-trips card data; no PocketStation CPU, BIOS, LCD, RTC or IrDA emulation is claimed.

### Shared media boundary and providers

- [x] **MEDIA-01**: Each supported provider presents the existing host-supplied disc contract with consistent identity, TOC, track/index/pregap/leadout, sector type, raw sector and applicable subcode; format errors and unsupported variants are explicit and do not partially replace an active disc.
- [x] **MEDIA-02**: Format readers use checked arithmetic and documented limits before allocation or decode, confine companion paths to the selected image directory, and have bounded regression-fuzz tests for malformed headers, offsets, track counts and truncated inputs. A cheap fixture inventory checks track/sector counts and boundary LBAs before any long guest run.
- [x] **CHD-01**: When `SCHWAYSTATION_CHD=ON`, the host opens the documented CHD v1–v5 subset using the pinned libchdr BSD-3 closure, returns matching sector/TOC/subcode data and deterministic fixture hashes on all eight applicable lanes, and fuzzes malformed CHD inputs; `OFF` remains a dependency-free default build until the sub-512-KiB parity fixture passes. Every distributed archive includes the exact required notices, and `.info` advertises `chd` only for a build that contains the provider.
- [x] **PBP-01**: The host reads the explicitly tested, unencrypted PSISO PBP layout and supported track variants with bounded directory/TOC parsing and deterministic sector output; encrypted PGD and undocumented layouts are rejected with a clear unsupported result. The inflater is in-tree permissive source with its notice and does not require system zlib.
- [x] **ECM-01**: The host decodes the documented ECM input subset with bounded random access, preserving sector bytes and rejecting invalid tables, offsets and truncated streams; a preflight confirms source extents and requested/produced sector counts before booting a guest.
- [x] **CCD-01**: The host parses the documented CCD descriptor subset and its confined IMG/SUB companions, preserving track modes, indexes, pregaps, subcode and first/last-sector reads; malformed or escaping companion paths are rejected before opening files outside the image directory.
- [x] **CRYPT-01**: An SBI sidecar is bound to the selected disc identity and LBA map; its bad-Q result remains separate from data-sector success and the CD-ROM GetlocP path returns the required previous-good position. A short command-sequence test verifies the result before any protected-title guest run, and `fuzz.sbi` plus seed replay cover malformed input.

### Region, board and bounded boot/MDEC behavior

- [x] **PROFILE-01**: PAL timing uses the documented integer clock ratios and is covered by frame, audio cadence and cross-platform hash tests; profile region stays distinct from board revision and title serial.
- [x] **PROFILE-02**: Every documented legal board/GPU revision tuple can be selected, every illegal tuple is rejected, and behavior-affecting profile fields participate in instance validation, serialization and hashes; the compatibility ledger records only tuples actually exercised.
- [x] **BOOT-01**: The opt-in `fast` runner/core path skips only the documented BIOS-shell handoff for eligible executable content, remains off by default, still requires the user-supplied BIOS, adds no HLE, and does not alter emulated clock/timing; the ordinary BIOS path remains available and separately tested.
- [x] **MDEC-01**: Targeted FMV edge regressions exercise the supported MDEC input cases without changing the accepted v1.0 4-bit expected output, frozen deterministic 8-bit baseline or 4,915,200-cycle cap. The source-unit preflight runs first; compressed input coverage, decoder output, guest completion and displayed-image result are reported separately. No exact 8-bit hardware or complete frame/movie conformance claim is made.

### Serial peripherals and light guns

- [x] **SIO-01**: The multitap works on both SIO ports with four independently selected controller/card slots per tap, correct protocol lengths, acknowledgements, interrupts and empty-slot behavior; tap cards remain independent and persist through the save lifecycle.
- [x] **SIO-02**: The mouse protocol reports deterministic frame-latched integer movement and buttons with documented saturation/reset behavior, tested through serial vectors and the libretro input mapping.
- [x] **SIO-03**: neGcon and Jogcon report their documented serial/configuration modes and frame-latched integer controls with bounded, deterministic protocol tests; unsupported modes fail explicitly.
- [x] **SIO-04**: Dual Analog flight mode reports its documented analog axes/buttons through the existing size-tagged input boundary and libretro mapping, without changing the behavior of existing digital-pad or DualShock callers.
- [x] **SIO-05**: Every delivered controller class uses the official libretro device/subclass identifiers and a public, documented mapping; the host latches input at a frame boundary and does not add device or window code to the deliverables.
- [x] **GUN-01**: GunCon and Justifier serial protocols report trigger, reload/offscreen and coordinate state using deterministic GPU beam timestamps and a documented brightness threshold; tests cover bright, dark, edge and offscreen samples plus interrupt timing.
- [x] **GUN-02**: Light-gun inputs map through available libretro pointer/lightgun controls; missing host input is reported as unsupported and cannot create a false hit. No physical device or owner hardware capture is required by CI.

### ROM-hacker support and compatibility claims

- [ ] **PATCH-01**: `schwaystation-run --patch` applies the documented PPF 1/2/3 and BPS subset as a host-side sector overlay, validates patch identity/bounds before use, preserves the source image, and produces deterministic patched sectors; malformed patches fail without partial application.
- [ ] **CHEAT-01**: The runner and libretro host parse a documented bounded GameShark text subset and apply only supported deterministic memory writes through a narrow explicit core operation; invalid codes are rejected, source memory is not mutated, and cheat state is included in relevant hashes/serialization.
- [ ] **COMPAT-01**: A pinned, licensed Redump DAT inventory accounts for all 13,407 source entries with explicit pass/fail/unknown evidence and provenance; untested title/profile/media combinations remain unknown and the generated compatibility document says the inventory does not imply playability.
- [x] **COMPAT-02**: Feature-specific compatibility rows accrue with each provider, profile, protection mechanism and peripheral; the protected ledger ratchet preserves prior passes and requires a dated allowance for any permitted regression or removal. A cheap count/key preflight runs before any broad ledger or guest suite.
- [ ] **COMPAT-03**: The libretro metadata sets `is_experimental "false"` only when 100% of a committed required-capability matrix passes with zero required unknown/fail rows; that matrix contains at least one boot-to-play fixture per shipped disc-provider family, each declared region/board tuple, each implemented peripheral class, card persistence and each supported patch/cheat family. If the matrix is incomplete or fails, the metadata remains `true`. DAT entries outside the required matrix remain explicitly unknown and are not represented as passing.

## Out of Scope

| Feature | Boundary |
|---|---|
| Save states, rewind, run-ahead, movies and TAS support | SEED-002 is a later milestone. |
| Browser/WebAssembly user interface | SEED-008 remains outside this milestone. The `wasm32` hash lane stays. |
| Measured-accuracy campaign, full external frame/movie conformance, or exact 8-bit MDEC hardware matching | These require evidence beyond this breadth milestone; v1.0 discrepancies and accepted bounds remain intact. |
| Full PocketStation emulation, physical device support, host window/audio/input device code | Only the defined card storage and protocol paths are included; hosts own devices and presentation. |
| HLE BIOS, JITs, visual enhancements, overclocking or timing-changing speed hacks | Preserve user-supplied BIOS, deterministic integer core and unchanged emulated timing. |
| Encrypted PBP, unspecified media variants and a claim that every DAT title plays | Unsupported cases stay explicit; inventory is not playability evidence. |

## Traceability

The roadmap maps each requirement to exactly one phase. Phase numbering continues after v1.0 Phase 6.

| Requirement | Phase | Status |
|---|---|---|
| TUNE-01 | Phase 7 | Complete |
| TUNE-02 | Phase 7 | Complete |
| TUNE-03 | Phase 7 | Complete |
| TUNE-04 | Phase 7 | Complete |
| TUNE-05 | Phase 7 | Complete |
| TUNE-06 | Phase 7 | Complete |
| SAVE-01 | Phase 8 | Complete |
| SAVE-02 | Phase 8 | Complete |
| SAVE-03 | Phase 8 | Complete |
| SAVE-04 | Phase 8 | Complete |
| SAVE-05 | Phase 8 | Complete |
| MEDIA-01 | Phase 9 | Complete |
| MEDIA-02 | Phase 9 | Complete |
| CHD-01 | Phase 9 | Complete |
| COMPAT-02 | Phase 9 | Complete |
| PBP-01 | Phase 10 | Complete |
| ECM-01 | Phase 11 | Complete |
| CCD-01 | Phase 11 | Complete |
| CRYPT-01 | Phase 12 | Complete |
| PROFILE-01 | Phase 13 | Complete |
| PROFILE-02 | Phase 13 | Complete |
| BOOT-01 | Phase 13 | Complete |
| MDEC-01 | Phase 13 | Complete |
| SIO-01 | Phase 14 | Complete |
| SIO-02 | Phase 14 | Complete |
| SIO-03 | Phase 14 | Complete |
| SIO-04 | Phase 14 | Complete |
| GUN-01 | Phase 15 | Complete |
| GUN-02 | Phase 15 | Complete |
| SIO-05 | Phase 15 | Complete |
| PATCH-01 | Phase 16 | Pending |
| CHEAT-01 | Phase 16 | Pending |
| COMPAT-01 | Phase 16 | Pending |
| COMPAT-03 | Phase 16 | Pending |

**Coverage:**
- v2 requirements: 34
- Mapped to phases: 34
- Unmapped: 0

---
*Requirements defined: 2026-11-18*
*Last updated: 2026-11-22 after Phase 11 verification*
