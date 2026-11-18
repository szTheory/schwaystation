# Domain Pitfalls

**Domain:** Deterministic PS1 emulator library, media helper, runner and libretro core  
**Researched:** 2026-11-18  
**Overall confidence:** MEDIUM. The v1 interfaces, tests and constraints are directly inspectable; some v2 media-format details are community-documented rather than standardized.

## Critical Pitfalls

### 1. Treating compressed-media metadata as trusted sizes
**What goes wrong:** A malformed CHD can overflow hunk-size arithmetic, loop through cyclic metadata, or request a huge allocation before any sector is read. PBP and ECM add offset tables, compressed blocks or long sequential decoding; a valid signature does not make their declared ranges safe. CCD text also contains independently supplied image/subchannel paths.
**Why it happens:** Parsers often validate the first header and defer bounds checks until allocation, decompression or seek. Container frame counts, codec output and caller-requested LBA can disagree.
**Consequences:** Memory exhaustion, unbounded work, out-of-bounds reads, hangs in the runner, or a host path escape. The pinned libchdr release documents fixes for exactly these malformed-header allocation and cyclic-chain cases.
**Prevention:** Keep parsing in the host helper, use the exact audited libchdr pin with integrity checks enabled, and check every add/multiply, file range, hunk count, decompressed-size ceiling and LBA before use. Reject unsupported PBP encryption/variants explicitly. For ECM, decode through a bounded sequential source rather than rescanning from byte zero per sector. Resolve CCD/CUE child paths beneath the descriptor directory. Keep the core's existing sector callback boundary.
**Cheap preflight:** Before full disc generation or emulation, compare declared tracks/frames/hunks with a checked total-sector count; read one sector from each track boundary and the final sector; assert every decoded byte count and callback LBA is in range. Run the same count against the requested fixture and the guest's produced-sector count.
**Detection:** Fuzz each production parser and sector reader with small malformed inputs under ASan/UBSan; require a fixed time/memory bound and exact failure status. Add a regression only for a recurring defect or format edge.
**Confidence:** HIGH for CHD allocation/metadata risks at libchdr v0.3.0; MEDIUM for less completely documented PBP/ECM/CCD variants. [libchdr v0.3.0 release](https://github.com/rtissera/libchdr/releases/tag/v0.3.0), [disc-format reference](https://psx-spx.consoledev.net/cdromfileformats/), [current bounded helper](../../host/disc_image.c), [fuzz inventory](../../tests/fuzz/).

### 2. Normalizing sectors while losing track and subchannel meaning
**What goes wrong:** Flattening all images to 2048-byte data sectors loses audio tracks, pregap/index boundaries, raw sector headers, or subchannel Q. CHD audio byte order and subcode packing differ from common BIN assumptions. SBI carries a bad-Q indication but not the bad CRC bytes themselves.
**Why it happens:** A file-open path is mistaken for a sector interface, or one generic conversion is used for data, CD-DA and subcode. Sector position is treated as the same fact as sector readability.
**Consequences:** Multi-track music or disc swaps fail; copy-protection checks see the wrong prior Q position; valid sector payload is rejected or advanced as though a bad Q were a bad data sector.
**Prevention:** Preserve track type, index 0/index 1, pregap, lead-out and raw/subcode availability in the TOC. Define one canonical internal sector representation per callback and test each format's conversion separately. Model SBI as the specified bad-Q behavior while still delivering the sector payload; do not pretend SBI stores a full subchannel record or CRC. Keep source image bytes and patches separate from normalized guest-visible sectors.
**Cheap preflight:** For each format, compare track count, first/last LBA, pregap and sector type with the decoded TOC; compare representative data/audio sectors and Q fields at boundaries before running a guest.
**Detection:** Golden sector vectors and track-boundary reads; a LibCrypt case checks both that the bad-Q position is retained and that the data sector still reaches the guest.
**Confidence:** HIGH that bad Q is ignored by the drive's Q-position path and SBI omits the CRC; MEDIUM for cross-format edge-case prevalence. [psx-spx disk images and SBI notes](https://psx-spx.consoledev.net/dev/ps1/cdr/cdromfileformats/diskimages/), [project CD-ROM preparation](../preparation/PSX-SPU-CDROM-AND-MDEC.md#27-libcrypt), [sector delivery](../../src/cdrom_sector.c).

### 3. Treating LibCrypt as a file-format toggle instead of CD controller behavior
**What goes wrong:** Loading an SBI file but returning its listed Q bytes as valid, advancing GetlocP to the bad sector, or failing the sector read entirely defeats the protection protocol. Multi-disc PBP or M3U sidecars can also be associated with the wrong disc.
**Why it happens:** The sidecar is attached to a container name instead of a stable selected-disc identity, or the parser confuses Q payload with Q CRC validity.
**Consequences:** Affected games may boot and then fail later key validation, while unrelated sectors appear to be I/O errors.
**Prevention:** Bind SBI records to the selected disc and LBA map after TOC construction. Represent bad-Q status independently from read success and payload bytes; verify the previous-good-position behavior and the relevant CD-ROM response sequence. Reject duplicate, unsorted, truncated or out-of-range SBI entries deterministically.
**Cheap preflight:** Check the sidecar's parsed entry count, order, LBA bounds and selected-disc identity, then run only the short bad-Q sector sequence before a long protected-title fixture.
**Detection:** A fixture asserts Q position, GetlocP result, delivered sector bytes and state hash separately.
**Confidence:** MEDIUM. The mechanism is documented in psx-spx and in the existing preparation; exact title coverage is not yet an implementation result. [psx-spx SBI/LibCrypt explanation](https://psx-spx.consoledev.net/dev/ps1/cdr/cdromfileformats/diskimages/), [SCM.05 and SCM.03](../preparation/PSX-SPU-CDROM-AND-MDEC.md#sources).

### 4. Assuming the top-level dependency license describes the whole shipped archive
**What goes wrong:** Vendoring a permissively licensed CHD library can still pull in codec sources with different notices, incompatible license terms, or build-time downloads that are absent from the recorded provenance. An option mismatch can also make consumer headers disagree with the compiled library ABI.
**Why it happens:** Review stops at the root LICENSE or a moving branch and misses nested dependencies, per-file notices, generated files, codec options and platform-specific code.
**Consequences:** A release can violate the no-GPL/no-unlicensed policy, omit required notices, build differently across lanes, or silently corrupt ABI/state.
**Prevention:** Pin a public commit; audit the archive, each bundled dependency and relevant file headers at that exact pin; record notices and the complete dependency closure. Keep default builds offline, build only required codecs, and make codec/configuration flags private and consistent with the compiled archive. Re-run the exact-pin license and archive-content checks whenever the pin changes.
**Cheap preflight:** Before compiling a third-party tool or full fixture matrix, verify the archive digest, source/build files, submodule closure, required codec configuration, license rows and expected input coverage.
**Detection:** Provenance and archive-content checks run on the actual release inputs, not a developer's cached checkout.
**Confidence:** HIGH for the project's policy and the v0.3.0 dependency closure described by the pinned upstream/project audit; MEDIUM for any future pin until re-audited. [libchdr v0.3.0](https://github.com/rtissera/libchdr/tree/v0.3.0), [project provenance](../../PROVENANCE.md), [dependency audit rule](../preparation/ENGINEERING.md#7-triage-automation-minimal-and-dependencies).

### 5. Letting host policy or host time leak into deterministic emulation state
**What goes wrong:** Host wall time, file timestamps, cache order, frontend input timing, conversion helpers or an unrecorded machine option changes output or a state hash. A state restores controller/device FIFOs but not the profile, disc identity or timing phase that gave them meaning.
**Why it happens:** Media policy, front-end persistence and core behavior are implemented in one path, or new v2 profile fields are added without updating state validation and hashing.
**Consequences:** Cross-platform hashes drift, split `run(N)+run(M)` calls diverge from `run(N+M)`, or a state loads on a different board/region/disc and continues incorrectly.
**Prevention:** Keep all core behavior in the integer CPU-cycle timebase. Put behavior-affecting region, board/revision, patch identity and relevant drive state in the instance and state contract; validate the full input before applying it. Keep filesystem/save naming and host clocks outside the core. Define whether each setting is emulation behavior or host policy before adding it.
**Cheap preflight:** For every new timing/profile/state field, check two split-run partitions, save-load-continue versus uninterrupted output, and identical serialized bytes on the portable lanes before longer game runs.
**Detection:** Existing `run(N)+run(M)`, state round-trip and cross-platform hash properties should catch these recurring faults; update their inputs as profiles are added.
**Confidence:** HIGH. This is an explicit architecture contract and is exercised in existing tests. [architecture: state and determinism](../preparation/ARCHITECTURE.md#5-state), [current state tests](../../tests/core/test_state.c), [hash inventory](../../tests/inventory/ci.txt).

### 6. Treating controller devices as button maps instead of serial protocols
**What goes wrong:** A device returns plausible button bits but wrong IDs, byte count, ACK delay, config exchange, abort behavior or port topology. A multitap may appear to work on port 1 only; a gun may use scaled screen coordinates with no beam timestamp or brightness threshold; relative mouse movement may be handled as an absolute pointer.
**Why it happens:** Pads are modeled as one fixed response and other devices are swapped in by ID, instead of driving the SIO byte state machine and its scheduled ACK/IRQ transitions.
**Consequences:** Titles hang on handshakes, detect wrong devices, lose port-2 or multitap slots, or accept light shots that the console would miss. Host polling cadence then changes guest-visible behavior.
**Prevention:** Implement one device protocol per device behind SIO's byte transport. Preserve port selection, byte lengths, config/watchdog state and cycle-based ACKs in state. Keep frame input as bounded integer values; model a gun against native emitted beam coordinates and brightness, and keep libretro coordinate conversion in the adapter. Follow libretro's device-specific contracts for mouse, pointer, lightgun and analog inputs rather than polling subclass IDs as base devices.
**Cheap preflight:** Compare command/response lengths and ACK/IRQ timing tables for both ports and each tap slot before full game runs; verify disconnected, connected and empty-slot cases. For guns, test on-screen, dark, off-screen and field-boundary coordinates against a tiny synthetic beam pattern.
**Detection:** Protocol-vector and SIO state-hash tests; one runner/libretro input-parity case per exposed device class.
**Confidence:** MEDIUM-HIGH. Core SIO evidence is pinned community documentation plus local implementation; libretro input coordinate rules are defined in the upstream API. [controller/card reference](https://psx-spx.consoledev.net/dev/ps1/controllersandmemorycards/), [libretro input API](https://github.com/libretro/RetroArch/blob/master/libretro-common/include/libretro.h), [current SIO tests](../../tests/sio/).

### 7. Selecting per-game card files before identity is stable
**What goes wrong:** A save key derived only from the current disc filename changes when a playlist or disc is renamed; a serial-only key splits multi-disc titles whose discs have different serials. Conversely, a shared-card fallback can merge two unrelated games. A wrapper import may be truncated or converted before its magic, exact size and payload are validated.
**Why it happens:** Save naming is chosen before a complete disc set is identified, or frontend `SAVE_RAM` and core-managed second-slot files have ambiguous ownership and flush timing.
**Consequences:** The game sees a blank card, one game overwrites another, or a card appears saved in-process but is lost after unload/restart.
**Prevention:** Keep card bytes fixed-size and host-backed; define the key for single-disc, multi-disc and unknown-serial content explicitly. Use content/set identity or a documented per-game policy, with a predictable fallback when identity is absent. Maintain clear ownership for libretro card 1 (`SAVE_RAM`) and card 2 (save-directory file); flush changed card 2 on the supported lifecycle and keep atomic replacement. Detect import wrappers by validated magic/length, not extension alone; reject malformed data before mutating a live card.
**Cheap preflight:** Round-trip raw, short raw, `.gme`, `.mem` and `.psx` imports through export; verify exact bytes, two independent slots, failed-import non-mutation and a fresh-process reload. Check two discs in one set select the intended same/different save keys before booting them.
**Detection:** Existing card generation, atomic-write, import lint and hosted save round-trip checks. Do not rely on save-state reload tests to prove persistent-card flushing; the state contract deliberately keeps persistent card bytes separate.
**Confidence:** MEDIUM-HIGH. The memory-card and libretro rules are specified in the project prep and upstream header, but the multi-disc identity policy remains an implementation decision. [card and save contracts](../preparation/PSX-PERIPHERALS-AND-SAVES.md#3-memory-cards), [libretro save API](https://github.com/libretro/libretro-common/blob/a3194c18ba65/include/libretro.h), [current host card importer](../../host/cards.c), [card tests](../../tests/card/).

### 8. Applying patches or cheats without binding them to the content and execution point
**What goes wrong:** PPF offsets are applied to logical 2048-byte files although the patch targets raw track bytes; BPS/PPF arithmetic wraps; undo/trailer bytes are interpreted as records; a cheat parser accepts a truncated string or writes outside guest memory. A patched disc can accidentally retain the unpatched identity/hash.
**Why it happens:** Format parsing, patch target validation and guest-memory access share unchecked offsets, or cheats are applied at arbitrary host polling time.
**Consequences:** Corrupt media, wrong achievement identity, out-of-range writes, host memory faults, or nondeterministic frame hashes when toggles change mid-frame.
**Prevention:** Validate format version, header length, record count, patch offset+length, base-image identity, optional block checks and all CRCs before installing an overlay. Define whether each patch format addresses raw sectors or decoded data; apply overlays in the host sector provider, never by modifying user files or adding file reads to the core. Parse cheat strings into bounded typed records, reject unknown/malformed codes without writes, and apply enabled records at a documented deterministic boundary. Bind patch and cheat configuration to state/hash identity where they affect emulation.
**Cheap preflight:** Check each patch record range against source size and target representation; compare the overlay's changed sector list and output byte count with a tiny fixture; run cheat parse boundary cases before any long patched boot.
**Detection:** Valid/invalid patch corpus, before/after source immutability, overlay sector hashes, guest-memory range checks and frame-boundary cheat toggle tests.
**Confidence:** MEDIUM. PPF layout comes from developer documentation; BPS/GameShark behavior needs phase-specific confirmation against the pinned accepted source. [PPF format notes](../preparation/PSX-PERIPHERALS-AND-SAVES.md#61-ppf-playstation-patch-file), [PPF3 developer text](https://github.com/meunierd/ppf/blob/4f24f8c784fa/ppfdev/PPF3.txt), [libretro cheat API](https://github.com/libretro/libretro-common/blob/a3194c18ba65/include/libretro.h#L8992-L9007).

### 9. Turning compatibility-ledger growth into denominator manipulation
**What goes wrong:** Dataset updates add duplicate serials or alternate revisions, and a generated count appears to improve while newly covered behavior does not. A missing fixture or unsupported format is counted as a pass, or a bad row disappears when the baseline is regenerated.
**Why it happens:** The ledger's identity key, dataset pin, profile coverage and status meanings are implicit; total row count is treated as the compatibility metric.
**Consequences:** Documentation and quality scores overstate library coverage; regressions can be masked by rebaselining or changing the denominator.
**Prevention:** Pin the source dataset, keep unique disc identity separate from title/set identity, and record explicit status/evidence for each claimed result. Compute coverage from defined categories and profile/media dimensions; report added, removed and changed identities against the protected base. Preserve all required rows and statuses; update a baseline only for a documented source-data change, not to absorb a regression.
**Cheap preflight:** Before fixture construction or emulator runs, verify the source pin, unique keys, exact expected row count by status/region/format, ledger schema and generated-doc row count. Fail when output count differs from the request or fixture input count.
**Detection:** Existing protected-baseline ratchet and generated-doc freshness checks, with small unit controls for duplicate keys and transitions.
**Confidence:** HIGH for the project's ledger and ratchet contracts; MEDIUM for an ecosystem-wide denominator, because DAT snapshots change. [compatibility ledger rules](../preparation/CONFORMANCE.md#32-compatibility-ledger-and-ratchet), [ledger tests](../../tests/compat/), [current pinned ledger](../../tests/compat/ledger.csv).

### 10. Ratcheting CI cost and quality from noisy or mismatched runs
**What goes wrong:** A timeout is doubled from an unrepresentative run, a “slowest job” improvement moves work to another bottleneck, or a quality ceiling is lowered from a different build/fixture/profile. An over-broad concurrency group may cancel a needed pending run; cache restores may hide missing inputs or change which tool binary actually ran.
**Why it happens:** Wall-clock, queue time, cache state and workload are collapsed into one number. A report is treated as a gate before it has a stable baseline.
**Consequences:** Flaky false failures, misleading speedups, stale artifacts, or apparent quality progress caused by narrower workload. Required coverage can be lost while the report looks better.
**Prevention:** Measure job p50/p95, queue time, runner minutes, cores used and repeated work against the same workflow, source revision and workload. Tune timeouts from a measured sample; make cache keys include toolchain and inputs and always validate outputs. Ratchet `quality.json` only against equivalent evidence and preserve all required checks and fixtures. Use workflow path filters or concurrency only where cancellation cannot remove a required release or changed-head result. GitHub documents that a new pending run in a concurrency group can replace the previous pending run, so group scope must be deliberate.
**Cheap preflight:** Before full CI, compare changed paths with job triggers, check workflow parse/required job dependencies, verify report input count and fixture fingerprints, then run the affected configure/build/target checks. Before a long emulation or hosted run, verify decoded counts and selectors first.
**Detection:** Keep the efficiency report descriptive until it has matched inputs; compare same-head results and critical-path time; report removed tests only with the stronger named signal and evidence that it covers the same faults.
**Confidence:** MEDIUM-HIGH. Local CI policy and seed scope are explicit; pending-run replacement is specified by current GitHub Actions docs. [SEED-007](../seeds/SEED-007-milestone-tune-up.md), [engineering CI and ratchet rules](../preparation/ENGINEERING.md#4-ci-design), [GitHub Actions concurrency](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax#concurrency), [current CI workflow](../../.github/workflows/ci.yml).

### 11. Making the triage digest a noisy or unsafe substitute for a reproducible failure
**What goes wrong:** A digest merges issues that share a symptom but have different causes, repeats closed or superseded failures, leaks private paths or disc names, or treats issue text and log snippets as trusted workflow instructions.
**Why it happens:** The summarizer lacks stable keys, source timestamps, a bounded evidence schema or privacy filtering; narrative output is used instead of the underlying run/issue links.
**Consequences:** Maintainers spend time on false clusters, a real regression is hidden, or sensitive user data enters a public artifact.
**Prevention:** Build the digest from explicit issue/run identifiers and normalized failure categories; retain links and counts so every summary is auditable. Treat issue bodies, logs and tool output as untrusted data, never as commands. Redact personal paths, e-mail addresses, BIOS/disc names and credentials before publication. Make the first digest read-only; automate a write or close action only if a recurring defect is demonstrated and the action is in approved scope.
**Cheap preflight:** Check each input row has a stable ID, a valid run URL and a date/status; verify the output count equals included inputs and scan rendered text for the repository's privacy-sensitive patterns before publishing.
**Detection:** Compare a small set of manually understood duplicate and non-duplicate controls; preserve links to source records rather than storing a detached summary as evidence.
**Confidence:** MEDIUM. This is a risk analysis from the approved digest scope and repository privacy policy; effectiveness depends on the eventual digest implementation. [SEED-007](../seeds/SEED-007-milestone-tune-up.md), [project privacy and evidence rules](../../AGENTS.md), [current nightly issue workflow](../../.github/workflows/nightly.yml).

## Moderate Pitfalls

### 1. Collapsing region and console revision into one profile switch
**What goes wrong:** Region selects the video clock and SCEx answer, while board revision selects chip/firmware traits. Deriving both from the game serial or a single profile default can produce mixed hardware settings that are individually plausible but never belonged together.
**Prevention:** Keep region, board and derived GPU/SPU/CD firmware fields explicit, validate legal profile combinations, and include them in state/hash identity. Preflight the requested profile matrix by checking the expected unique tuples before emulating every row.
**Confidence:** HIGH. [Settled profile decision](../preparation/DECISIONS.md), [profile fields](../preparation/ARCHITECTURE.md#7-determinism-and-hashes).

### 2. Treating successful card conversion as successful save compatibility
**What goes wrong:** A wrapper's header or checksum is accepted but directory chains, free/deleted block flags or truncated trailing sectors are mishandled. A card may open in the emulator but fail in BIOS manager or a second emulator.
**Prevention:** Keep a lossless raw 128 KiB representation, validate wrapper bounds before mutation, and distinguish lint warnings from conversion errors. Check directory-chain and checksum invariants on export/import without “repairing” bytes silently.
**Confidence:** MEDIUM-HIGH. [Memory-card layout and formats](../preparation/PSX-PERIPHERALS-AND-SAVES.md#3-memory-cards), [current card tests](../../tests/card/).

### 3. Growing parser defenses into a permanent check suite without evidence
**What goes wrong:** One-off format checks become slow, duplicated repository gates that are not tied to recurring faults.
**Prevention:** Use the cheapest relevant preflight locally first; retain a CTest/CI gate only when it catches a repeatable defect or protects a named requirement. Keep full fixture construction and all required acceptance coverage intact.
**Confidence:** HIGH. This follows the project's proportional-gate and automation rules. [Project verification rules](../../AGENTS.md), [fuzz and CI tiers](../preparation/CONFORMANCE.md#5-ci-tiers-and-cost).

## Minor Pitfalls

### 1. Assuming a current source URL is a stable compatibility claim
**What goes wrong:** A moving dataset, docs page or upstream branch changes after the research snapshot, and the cited behavior no longer describes the build.
**Prevention:** Pin implementation and test sources to public commits or tags; keep access dates for mutable documentation and recheck at the phase where the behavior is implemented.
**Confidence:** HIGH. [Preparation source and pin rules](../preparation/BRIEF.md).

## Phase-Specific Warnings

| Phase topic | Likely pitfall | Mitigation |
|---|---|---|
| Milestone tune-up: CI report and `quality.json` | Ratchets reflect different workload or noisy timing | Fingerprint inputs and compare equivalent source/toolchain/workload; keep coverage unchanged |
| CHD, PBP, ECM, CCD and SBI | Hostile sizes, expansion, index overflow, wrong sidecar/LBA association | Strict checked ranges, bounded decode, exact source/license pin, count and boundary preflight, parser fuzzing |
| LibCrypt and sector overlays | Bad Q conflated with failed sector read; patch targets wrong byte representation | Separate Q validity from payload; bind sidecars/overlays to selected-disc identity and test sector hashes |
| Saves and per-game identity | Multi-disc naming, slot ownership and flush behavior disagree | Decide the set key and slot ownership, then round-trip both slots through in-process and fresh-process persistence |
| New SIO devices | Correct button map but incorrect wire protocol/timing | Device protocol vectors and both-port/tap-slot preflight before game fixtures |
| PAL/board/GPU profile matrix | Region and hardware revision combinations get silently omitted | Validate the complete requested tuple list and state/hash inclusion before long emulation runs |
| Patches, cheats and compatibility ledger | Content identity/coverage changes without source evidence | Validate patch target/CRCs; compare ledger identities and category counts against pinned base |

## Sources

| Source | Tier / confidence | Supports |
|---|---|---|
| [libchdr v0.3.0 release](https://github.com/rtissera/libchdr/releases/tag/v0.3.0) and [pinned tree](https://github.com/rtissera/libchdr/tree/v0.3.0) | Upstream / HIGH for the release notes; re-audit future pins | CHD integer overflow, cyclic metadata traversal and oversized allocation fixes; dependency pin |
| [psx-spx CDROM image formats](https://psx-spx.consoledev.net/dev/ps1/cdr/cdromfileformats/diskimages/) and [project source record SCM.05](../preparation/PSX-SPU-CDROM-AND-MDEC.md#sources) | Curated hardware reference / MEDIUM | CCD, CHD, PBP, ECM, SBI layout notes and bad-Q semantics; not a complete normative format standard |
| [libretro API header at project pin](https://github.com/libretro/libretro-common/blob/a3194c18ba65/include/libretro.h) and [project source record HI.01/PER.14](../preparation/HOSTS-AND-INTEGRATION.md#sources) | Upstream / HIGH for API contract | SAVE_RAM, save directory, device inputs, cheat entry points and serialization contracts |
| [PPF3 developer text at reviewed pin](https://github.com/meunierd/ppf/blob/4f24f8c784fa/ppfdev/PPF3.txt) and [project source record PER.16](../preparation/PSX-PERIPHERALS-AND-SAVES.md#sources) | Format documentation / MEDIUM | PPF headers, record offsets, undo data and trailer bounds; format text has no stated code license |
| [GitHub Actions workflow syntax](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax#concurrency) | Official docs / HIGH for pending-run semantics | Concurrency groups can replace an existing pending run; scope cancellation carefully |
| [Architecture and determinism](../preparation/ARCHITECTURE.md), [engineering rules](../preparation/ENGINEERING.md), [conformance and ledger](../preparation/CONFORMANCE.md) | Project decisions/tests / HIGH for project contracts | State/hash behavior, integer timing, media boundary, ledger, CI tiers and resource constraints |
| [Current disc helper](../../host/disc_image.c), [CD sector path](../../src/cdrom_sector.c), [SIO tests](../../tests/sio/), [card tests](../../tests/card/), [compatibility tests](../../tests/compat/) | Current repository / HIGH for current implementation | Existing boundaries and checks that v2 changes must preserve or extend |

