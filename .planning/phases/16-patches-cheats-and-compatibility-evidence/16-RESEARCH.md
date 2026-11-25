# Phase 16: Patches, cheats and compatibility evidence - Research

**Researched:** 2026-11-25
**Domain:** Host-side PlayStation disc patching, memory cheat adapters, compatibility evidence and libretro metadata
**Confidence:** MEDIUM

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
### Patch identity and media mapping
- **D-01:** Keep patch parsing and transformation in the host. PPF addresses use an explicit raw 2352-byte track-file mapping; initially reject compressed, multi-track, virtual-pregap, synthesized-sector, or otherwise ambiguous layouts unless implementation proves an exact mapping. Validate the entire patch and staged result before patched sectors become visible, and preserve patch-record order for overlaps. Require a caller-supplied whole-source SHA-256 for every PPF version and enforce every applicable native check: PPF2 source size/block check and PPF3 optional 1,024-byte block check (PPF3 has no source-size field). Report exactly which checks were performed and never describe CRC/block checks as cryptographic identity. BPS source/target/patch CRCs remain its native checks and are likewise non-cryptographic. Reject BPS size changes that would change the source sector geometry. Never redefine `schway_disc.identity_sha256`, which currently identifies the first 16 data sectors plus serial.
- **D-02:** Apply PPF as a sparse overlay and BPS as its full byte-stream transformation; do not model BPS as sparse PPF records. Stage and validate the completed BPS target before exposing sectors, with bounded hostile-input handling and no source-image mutation.

### Cheat subset and state
- **D-03:** Start with only unconditional GameShark `30` 8-bit writes and `80` 16-bit writes, applied in input order at the frame boundary. Limit writes to aligned in-range main RAM offsets, and bound the total active writes across all entries so per-frame work remains bounded. Reject conditional, stateful, unsupported, malformed, or out-of-range codes. Rejecting a replacement leaves the current cheat configuration and RAM untouched. Disabling a cheat stops later writes but does not undo earlier writes. Set the libretro `cheats` metadata claim true only after the implemented subset and its claim guard pass.
- **D-04:** Add one narrow public RAM-write operation with physical offsets, widths, alignment and bounds checked; it has no MMIO behavior and consumes no emulated ticks. Increment `SCHWAY_API_VERSION` from 10 to 11 for this additive API operation. Preserve the existing public struct layouts and library ABI/SOVERSION. Keep parsing and cheat configuration in the hosts, not the core.
- **D-05:** Include normalized cheat entries, ordering and enabled state in runner state-hash inputs and in a versioned libretro serialization envelope around the existing core state. Validate the whole envelope and cheat table before applying state. Do not reinterpret sampled disc identity or broaden this into disc save-state work.

### Library accounting and experimental metadata
- **D-06:** Fetch the licensed Redump DAT only at pinned commit `12c21cf79f1e20eff6ac68b80875816c717d0a7a` and SHA-256 `52978215854d307bfb5353052c9ce3c34995310a970d586f0100bb67f7de01a8`. Account for every source entry, including duplicate or missing serials, in aggregate pass/fail/unknown counts. Do not commit or publish the full DAT or a bulk title table; retain only the allowed sparse measured rows and their provenance. The report must say explicitly that inventory is not playability evidence.
- **D-07:** Generate required capability-matrix rows from fresh, attributable results for the declared build configuration. Missing, stale, duplicate, unknown, or failing required rows fail closed. Keep `is_experimental` true unless every required row passes, covering boot-to-play for each shipped disc-provider family and each declared region/board tuple, implemented peripheral classes, card persistence, and each supported patch and cheat family. Existing profile-selection and protocol tests do not count as boot-to-play. Preserve and extend the protected feature-evidence ratchet: Main CI sparse checkout and its workflow-consistency test must both include `tests/compat/features.csv` before its results can support the claim. DAT-wide playability is outside the metadata claim.

### CI follow-up
- **D-08:** Update TODO052's evidence and scope to include both long BE32 tests, `libretro.host` and `libretro.input.movies`. Keep all required tests and assertions. Pursue a shard only if cheap inventory/selector/JUnit preflights and a measured exact-head run show a real critical-path reduction without duplicate work erasing the gain.

The planner may choose module/file boundaries, CLI spelling, concrete line/entry caps within the preparation bounds, temporary target storage, report formatting, fixture design, and the test decomposition. It may refine the supported raw-track subset where current provider behavior proves a narrower or broader safe mapping. It must preserve the decisions above and the project's deterministic, bounded, no-partial-application contracts.

### Folded Todos
- **TODO052 — Isolate the serial BE32 movie test from the main test shard:** its original Phase 15 note predates the observed `libretro.host` runtime. Update the evidence to include both tests and evaluate the full candidate together in this phase; keep it pending unless the complete shard contract is proven and measured.

### Deferred Ideas (OUT OF SCOPE)
None — discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| PATCH-01 | `schwaystation-run --patch` applies the documented PPF 1/2/3 and BPS subset as a host-side sector overlay, validates patch identity/bounds before use, preserves the source image, and produces deterministic patched sectors; malformed patches fail without partial application. | PPF3 author format text, BPS author's complete stream-transform specification, existing provider-to-sector mapping and overflow checks. PPF1/2 details need conservative tests because local preparation marks them unverified. [VERIFIED: `.planning/REQUIREMENTS.md:64`] |
| CHEAT-01 | The runner and libretro host parse a documented bounded GameShark text subset and apply only supported deterministic memory writes through a narrow explicit core operation; invalid codes are rejected, source memory is not mutated, and cheat state is included in relevant hashes/serialization. | Local pinned cheat reference, existing checked memory read API, libretro frame and serialization boundaries, and core state validate-then-apply pattern. [VERIFIED: `.planning/REQUIREMENTS.md:65`] |
| COMPAT-01 | A pinned, licensed Redump DAT inventory accounts for all 13,407 source entries with explicit pass/fail/unknown evidence and provenance; untested title/profile/media combinations remain unknown and the generated compatibility document says the inventory does not imply playability. | Existing pinned download and digest check; licensing/asset restrictions; existing compatibility report and ledger patterns. A source-occurrence counter must be added; the current library script does not account for all DAT entries. [VERIFIED: `.planning/REQUIREMENTS.md:66`] |
| COMPAT-03 | The libretro metadata sets `is_experimental "false"` only when 100% of a committed required-capability matrix passes with zero required unknown/fail rows; that matrix contains at least one boot-to-play fixture per shipped disc-provider family, each declared region/board tuple, each implemented peripheral class, card persistence and each supported patch/cheat family. If the matrix is incomplete or fails, the metadata remains `true`. DAT entries outside the required matrix remain explicitly unknown and are not represented as passing. | Existing feature ledger and protected ratchet, requirement row categories, .info guard, and missing protected `features.csv` in the CI sparse checkout. [VERIFIED: `.planning/REQUIREMENTS.md:68`] |

</phase_requirements>

## Project Constraints (from AGENTS.md)

1. Reuse only permissively licensed code/data, preserve notices, and never open or retain source from the named prohibited emulator projects. These changes need no external patch library.
2. Ship only the C library, headless runner and libretro core. Do not add a window or host I/O device layer.
3. Keep firmware, commercial disc bytes, save/state bytes and executable payloads out of the tree except manifest-listed fixtures. Do not add the bulk Redump DAT or bulk title rows.
4. Keep the core integer-only, deterministic and bounded. Core behavior remains instance-owned; host code supplies files and sectors.
5. Every behavior change needs a test in cmake --workflow --preset ci. Keep the README short and public-facing, put technical behavior in docs/, and keep the public header terse.
6. Prefer automation evidence. Run required checks as the agent; do not make the owner verify routine behavior.
7. Repository checks must be CMake -P or POSIX sh, name the fault prevented, and stay inside the phase line budget. Real RetroArch test code remains at or below 300 lines total. Review source code before verification.
8. Do not add attempt quotas, approval ceremonies, lifetime counters or self-metering gates.
9. Keep commits on phase/16-patches-cheats-compatibility, record runtime/model in STATE.md at each phase step, and stop only at the authorized phase verify/ship boundary. Do not set SCHWAYSTATION_ALLOW_MAIN_COMMIT.
10. Before the first plan execution commit, confirm the branch remains based on origin/main. Ship through a PR, use gh pr merge --auto --squash in a shell, and confirm main CI and release status after merge.
11. Evidence is a command, exit status and run URL; write it from the repository root and commit no raw logs.
12. Reference pins must be public commits; fetch sibling code using GitHub API, never a sibling checkout.
13. Imported sibling lessons are hypotheses until reproduced; read .planning/LESSONS.md at discuss boundaries.
14. Plan 1 must establish the CMake skeleton and each per-directory test inventory. Later plans should maximize independent waves, and work over 30 minutes should split. No test is non-gating.

Also preserve the repo's fixed check budgets, run cheap data/selector/budget preflights before long jobs, inspect hosted CI critical path and runner time, keep temporary CMake binaryDir aligned to its handoff path, and record CI/release URLs. The repository budget says `3,901 total lines in every v2 phase`; update it only to a measured lower total at ship. Do not broaden TODO052 unless both long test shards have exact coverage proof and measured wall-time benefit. [VERIFIED: `.planning/REQUIREMENTS.md:12`; quote: `3,901 total lines in every v2 phase`]

## Summary

**Primary recommendation:** keep all parsing outside the emulated core, reuse its checked state/memory conventions, and make each host transform transactional. PPF patches can remain sparse sector overlays. BPS is a complete byte-stream transformation and needs a complete staged output before sectors are switched over. Share a bounded cheat parser/normalizer between runner and libretro; each host applies writes at its own frame boundary. [CITED: PPF3 developer format file; BPS File Format Specification] [VERIFIED: `src/state.c:247-268`; `libretro/libretro.c:1932-1950`]

A primary-source detail conflicts with one sentence of D-01: PPF3's author specification describes an optional 1,024-byte validation block and 64-bit patch offsets; the distribution README says image-size checking was removed. PPF3 does not define the PPF2-style size check. The local preparation also marks PPF1/2 layouts unverified beyond the method byte. Therefore do not claim a PPF3 size check. Use version-specific native checks, and require exact source SHA-256 when PPF1 has no check or PPF2/3 checks are absent/disabled or insufficient for the selected identity claim. [CITED: PPF3 developer format file](https://raw.githubusercontent.com/meunierd/ppf/4f24f8c784faf27b54081caee3835fa59a8a009e/ppfdev/PPF3.txt), [CITED: PPF3 distribution README](https://github.com/meunierd/ppf/blob/4f24f8c784faf27b54081caee3835fa59a8a009e/readme.txt), [VERIFIED: `.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md:219-265`]

For compatibility, preserve one count per source DAT occurrence even when title keys are missing or duplicated; join evidence only where serial/hash identity supports it. Generate the required matrix from current measured results and leave absent or stale evidence unknown. Profile and peripheral rows remain subsystem evidence, not retail boot/play. The template currently contains the exact claims `cheats = "false"` and `is_experimental = "true"`; `info_claims.cmake` requires those conservative values until the feature matrix proves otherwise. [VERIFIED: `scripts/library.sh:114-125`; `ASSET_POLICY.md:29-31`; `PROVENANCE.md:187-190`; `libretro/schwaystation_libretro.info.in:24-27`; `tests/cmake/info_claims.cmake:48-69`; `.github/workflows/ci.yml:395-400`]

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| PPF/BPS parsing, source checks, staged output | Runner/host C | Disc provider | Format parsing and file preservation are host responsibilities; the core receives sectors only. |
| Cheat parsing and normalized active codes | Shared host C | Runner and libretro adapters | One grammar/validation path should produce consistent records; each host owns timing and active state. |
| Memory writes | Core public API | Host cheat adapters | Core owns checked RAM bounds; hosts own parsing and frame scheduling. |
| Libretro cheat persistence | Libretro adapter | Core state API | Current serialization delegates to the core; the host needs a validated envelope for cheat records. |
| DAT inventory/report | Host script and report generator | Compatibility ledger/docs | File acquisition and source-entry accounting do not belong in the core. |
| Required matrix and metadata claims | CMake/POSIX-sh test and guard | libretro .info generation | Build-time metadata must fail closed when current required results are missing. |

## Standard Stack

### Core
| Component | Version | Purpose | Why Standard |
|-----------|---------|---------|--------------|
| C | C17 | Parsers, overlays, RAM API and host glue | Repository standard, fixed-width and integer-only. [VERIFIED: `CMakeLists.txt:16-17`; quote: `set(CMAKE_C_STANDARD 17)`; project rule in `AGENTS.md` §4] |
| CMake | Existing workflow/presets | Build graph, CMake -P repository gates and CTest inventory | The project uses `cmake --workflow --preset ci`; do not add another test harness. [VERIFIED: `.planning/REQUIREMENTS.md:8`; `CMakePresets.json:17-25`] |
| POSIX sh | System shell | DAT fetch/accounting orchestration and compatibility ratchet | Existing scripts use checked shell inputs and run under sh. [VERIFIED: `scripts/library.sh:1-3`; `scripts/compat-ratchet.sh:1-3`] |
| C standard library | Current toolchain | File staging, parsing, bounds checks and byte transforms | Host code already owns file access; no runtime dependency is needed. [VERIFIED: `host/disc_image.c:1378-1446`] |

The existing public API constant is exactly `#define SCHWAY_API_VERSION 10`; increment it when adding the write operation. This additive symbol preserves the separate library ABI/SOVERSION and existing public layouts. [VERIFIED: `include/schwaystation.h:14`; `.planning/phases/16-patches-cheats-and-compatibility-evidence/16-CONTEXT.md` D-04]

### Supporting
| Component | Version | Purpose | When to Use |
|-----------|---------|---------|-------------|
| Existing runner SHA-256 | In-repo | Exact source identity and reproducible report/hash values | Reuse instead of adding crypto code or dependencies. [VERIFIED: `runner/sha256.c:1-20`] |
| Existing runner CRC-32 | In-repo | BPS trailer validation | `runner/crc32.c` implements reflected Ethernet/PNG CRC-32 with initial and final XOR `0xFFFFFFFF`; check BPS known-answer vectors against its seed semantics. [VERIFIED: `runner/crc32.c:1-3,44-50`; quote: `0xFFFFFFFF`] |
| Existing feature ledger/ratchet | In-repo | Protected compatibility evidence | Extend the current row scheme and fresh-result matching rather than inventing another ledger. [VERIFIED: `tests/compat/features.csv`; `scripts/compat-ratchet.sh:1-45`] |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Sparse PPF overlay and staged BPS output | Rewriting the source image in place | Mutates the user's input and exposes partial output after late validation errors. |
| Existing C/CMake/sh stack | New parser or CSV package | Adds dependencies and a new security surface where bounded parsers and existing helpers fit. |

**Installation:** None. Do not add third-party packages.

## Architecture Patterns

### System Architecture

~~~mermaid
flowchart LR
  A[Runner CLI and source image] --> B[Existing media provider]
  B --> C{Patch format}
  C -->|PPF| D[Validate identity and ordered records]
  C -->|BPS| E[Validate source and build full target]
  D --> F[Publish immutable sector overlay]
  E --> F
  F --> G[Disc sector callback]
  G --> H[Integer-only core]
  I[GameShark text] --> J[Shared parser and normalized ordered table]
  J --> K[Runner frame boundary]
  J --> L[Libretro frame boundary]
  K --> M[Checked core RAM write]
  L --> M
  M --> H
  L --> N[Versioned libretro state envelope]
  N --> O[Validate envelope, cheats and core state]
  O --> H
  P[Pinned DAT occurrences] --> Q[Join supported identity/evidence rows]
  Q --> R[Aggregate pass/fail/unknown report]
  S[Fresh required test results] --> T[Fail-closed required matrix]
  T --> U[Generated metadata claims]
~~~

### Recommended Project Structure

Keep patch code in a host module invoked by runner, not in `src/` core code. Keep cheat parsing/normalizing usable by runner and libretro. Add a RAM-only public function beside the current memory inspection API. Add a small serialization-envelope helper in libretro rather than putting host cheat definitions into core state. Put DAT counting and report generation in the existing compatibility path; preserve the established feature ratchet and protected-base tests. [VERIFIED: `.planning/preparation/DECISIONS.md` DEC.65, DEC.69, DEC.75, DEC.77; `host/disc_image.c`; `libretro/libretro.c`; `scripts/compat-ratchet.sh:1-45`]

### Pattern 1: Transactional transform
**What:** validate headers, flags, varints, source identity, target size and record bounds before making transformed bytes visible.
**When:** both patch formats; stage BPS output until target CRC and geometry pass.
**Source pattern:** `src/state.c:247-268` validates without writing and then applies. Do not expose partial patched sectors. [VERIFIED: `src/state.c:247-268`]

### Pattern 2: PPF record decoding and raw-track offsets
Use little-endian byte readers and a 64-bit host offset accumulator; never cast a patch buffer to a packed struct. The upstream ApplyPPF reference parses PPF1 records from byte offset 56 as a 32-bit little-endian file offset, one-byte length, then that many replacement bytes. PPF2 stores its expected image byte size at offset 56, a 1,024-byte validation block at offset 60 (standard BIN block at source offset `0x9320`), and begins records at 1084; record offsets are 32-bit little-endian. These PPF1/2 details are reference facts but local preparation still calls them unverified, so require byte fixtures before declaring support. [CITED: upstream PPF ApplyPPF source](https://github.com/meunierd/ppf/blob/master/ppfdev/applyppf_src/applyppf3_linux.c#L1267-L1452) [VERIFIED: `.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md:219-265`]

The project reference identifies method/version headers verbatim as `PPF1 ("PPF10", method 0)`, `PPF2 ("PPF20", method 1)`, and `PPF3 ("PPF30", method 2)`. Validate the signature and method together, not method in isolation. Local preparation explicitly says PPF1/PPF2 layouts are unverified beyond their header; use self-authored golden byte fixtures and reject unsupported flags/trailers until validated. [VERIFIED: `.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md:221-227`; quote: `PPF1 ("PPF10", method 0)`, `PPF2 ("PPF20", method 1)`, `PPF3 ("PPF30", method 2)`] [CITED: PPF3 developer format file]

PPF3's fixed header is 60 bytes. At offsets 56, 57, and 58 it carries image type, block-check flag and undo flag; byte 59 is dummy/reserved. When block-check is enabled, the 1,024-byte check block follows at offset 60 and records begin at 1084, otherwise records begin at 60. Standard BIN and GI block-check source offsets are `0x9320` and `0x80A0`. Each PPF3 record uses an eight-byte little-endian offset, one-byte length, payload bytes, then optional undo bytes. This format has no image-size field; its offsets must remain 64-bit throughout validation and overlay application. [CITED: PPF3 developer format file](https://raw.githubusercontent.com/meunierd/ppf/4f24f8c784faf27b54081caee3835fa59a8a009e/ppfdev/PPF3.txt) [CITED: upstream PPF ApplyPPF source](https://github.com/meunierd/ppf/blob/master/ppfdev/applyppf_src/applyppf3_linux.c#L1460-L1620) [VERIFIED: `.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md:225`]

For an accepted single raw track, define patch byte offset `p` as track-file byte `p`, then derive `lba = p / 2352` and byte-within-sector `p % 2352`; the public sector callback returns 2352-byte raw sectors. Preserve the original track file and overlay only fully validated changed bytes. This mapping is an implementation constraint inferred from the locked decision and the source provider's sector size, not a PSX-wide rule. Reject any input where track-file byte zero and raw-sector byte zero do not match. [VERIFIED: `host/disc_image.c:28`; quote: `#define SCHWAY__DISC_IMAGE_RAW_SECTOR_BYTES 2352u`; `include/schwaystation.h:319-334`; `host/disc_image.c:1378-1446`] [ASSUMED: mapping validation must be demonstrated by fixture]

Treat the PPF2 size field and PPF2/3 validation block as compatibility checks, not cryptographic identity. They can be omitted, disabled or insufficient for a claimed exact image identity; caller-supplied SHA-256 is the strongest supported exact identity input in this repository. [CITED: PPF3 developer format file; PPF3 distribution README] [VERIFIED: `runner/sha256.c:1-20`]

### Pattern 3: Checked offset arithmetic
**What:** compare offset to size, then length to remaining bytes; avoid offset-plus-length overflow.
**When:** PPF records, BPS reads/copies and file-backed sector access.
**Existing code:**
~~~c
if (file_offset > source->size ||
    track->sector_bytes > source->size - file_offset) {
    return 0u;
}
~~~
Source: `host/disc_image.c:1423-1424`. [VERIFIED: `host/disc_image.c:1423-1424`]

### Pattern 4: Host owns side effects, core owns memory boundary
**What:** parse cheat input into validated records, then call checked RAM-only API at frame boundary.
**When:** runner and libretro cheat support.
**Evidence:** `src/instance.c:139-165` bounds-checks current `schway_peek`; `libretro/libretro.c:1944-1949` queues input before `schway_run_frame`. [VERIFIED: `src/instance.c:139-165`; `libretro/libretro.c:1944-1949`]

### Pattern 5: Decode GameShark addresses before the core write
PSX-SPX documents `30aaaaaa 00dd` as an 8-bit write and `80aaaaaa dddd` as a 16-bit write, with `aaaaaa` as the target address field; conditional and increment/decrement forms are separate families and should remain rejected. The preparation states `GameShark/Action Replay code TTaaaaaa dddd with the address in KSEG0 (80aaaaaa)`. The PSX memory map puts main RAM at KUSEG `0x00000000`, KSEG0 `0x80000000`, and KSEG1 `0xA0000000`, with 2 MiB RAM mirrored in the first 8 MiB. [CITED: PSX-SPX cheat format](https://psx-spx.consoledev.net/cheatdevices/) [CITED: PSX-SPX memory map](https://psx-spx.consoledev.net/memorymap/) [VERIFIED: `.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md:231-232`; quote: `address in KSEG0 (80aaaaaa)`]

Do not blindly mask or wrap an arbitrary cheat address into RAM. Validate that the code's KSEG0 address is in the accepted main-RAM window, map only then to a physical RAM offset, and reject non-RAM regions. A 16-bit write must be halfword-aligned; a byte write has no alignment requirement. The exact accepted address window is a plan-level choice within D-03's in-range contract and should have explicit tests. The prep bounds each line to `8 + 4 hex digits per line, ≤ 64 lines`; the project can use that existing bound unless testing gives a reason to ratchet it lower. [CITED: PSX-SPX memory map] [VERIFIED: `.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md:252-253`; quote: `8 + 4 hex digits per line, ≤ 64 lines`] [ASSUMED: recommended conservative policy is canonical first-2-MiB RAM offsets only; mirror aliases need explicit tests]

### Pattern 6: BPS builds a complete target stream
Read `BPS1`, then bounded variable-length source size, target size and metadata size; metadata is arbitrary bytes for this application and should be skipped by its declared length. Each action encodes `(length - 1)` in the high bits and a 2-bit action kind: SourceRead copies from source at the current output position, TargetRead consumes literal patch bytes, SourceCopy reads from a signed relative source cursor, and TargetCopy reads from a signed relative target cursor. The target output is built sequentially. TargetCopy may refer only to already emitted target bytes, while overlap may extend that earlier pattern forward during the copy. The final 12 bytes carry source CRC32, target CRC32 and patch CRC32; patch CRC is over all preceding patch bytes. CRC detects accidental corruption and wrong inputs but is not cryptographic identity. [CITED: BPS File Format Specification](https://floating.muncher.se/byuu/bps/bps_spec.html)

Before applying any BPS action, prove `target_size == source_size` for the locked no-geometry-change scope, and bound patch/source/target sizes by project media limits. Use subtraction-form checks for every source, patch and target range; reject cursor underflow/overflow before changing it. Stage to a separately owned output and publish only after all three CRC checks, exact target length, and `target_size % 2352 == 0` pass. Existing media rejects files using the diagnostic `"disc image exceeds the 1 GiB limit"`, and preparation states `total image ≤ 1 GiB` and `≤ 1 GiB logical`; quote those existing bounds and use them unless the chosen raw track layout needs a narrower bound. A bounded temporary file can avoid allocating a second full image, but file creation/cleanup and every I/O failure must be handled atomically. [CITED: BPS File Format Specification] [VERIFIED: `host/disc_image.c:1159-1162`; quote: `"disc image exceeds the 1 GiB limit"`; `.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md:242-244`] [ASSUMED: temp-file staging is the practical fallback if memory staging exceeds the chosen cap]

### Pattern 7: Versioned libretro serialization envelope
The libretro API's serialization size/save/load callbacks and cheat reset/set callbacks are separate host responsibilities. Wrap the unchanged core state with a bounded host envelope containing a magic/version, exact total and core-payload lengths, normalized ordered cheat entries, per-entry enabled state, and checksum. During load: validate all envelope fields and the complete cheat table into temporary storage; call the core's existing validate-then-apply loader; only after it succeeds swap the temporary host cheat state into place. `retro_serialize_size` must include the envelope and use overflow-checked size arithmetic; unknown envelope versions should fail before core state is touched. [CITED: libretro/libretro.h:8966-9007] [VERIFIED: `libretro/libretro.c:1987-2010`; `src/state.c:222-268`]

For runner state hashing, append the normalized ordered cheat table and enabled bits to the existing core-state input, using an unambiguous fixed-width encoding. The runner currently hashes the serialized core state body after its header; including cheat configuration is required so two identical RAM/CPU states with different next-frame writes do not collide semantically. [VERIFIED: `runner/main.c:1321-1345`]

### Anti-Patterns to Avoid
- Treating BPS as sparse PPF byte ranges; BPS creates a complete target stream in order.
- Rewriting the caller's source image or exposing patched sectors before validation completes.
- Assuming PPF3 checks file size, or trusting an optional disabled PPF block.
- Truncating PPF3 64-bit offsets to 32 bits or using wrapping offset-plus-count bounds checks.
- Applying patches to cooked/synthesized sectors, virtual pregaps, compressed providers or multi-track inputs without proving an exact mapping.
- Treating DAT entry counts, profile-select rows or protocol vectors as boot-to-play evidence.
- Turning prior results into pass when a current test did not run; absence remains unknown.
- Hashing only RAM while omitting active cheat definitions that change future execution.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| SHA-256 source identity | New cryptographic implementation or package | Existing runner/sha256.c | Already shared by runner/core for deterministic hash operations. |
| IEEE CRC-32 | New table or duplicate implementation | Existing runner/crc32.c | Already implements reflected Ethernet/PNG CRC-32; verify BPS vectors and incremental seed semantics. |
| State validation | Another partial mutation path | Core validate-then-apply pattern | Existing loader validates all fields first and refuses atomically. |
| Retail title inventory | Bulk copied DAT dataset | Pinned local DAT and aggregate accounting plus permitted sparse facts | Asset/provenance policy prohibits bulk Redump rows. |
| Disc patch behavior | Copying a patch parser from another emulator | Small parser derived from specs and self-authored byte fixtures | Project forbids prohibited emulator sources; local PPF1/2 layout notes remain unverified. |

## Common Pitfalls

### PPF identity and mapping
**What goes wrong:** A valid-looking patch is applied to a different image or at wrong cooked-sector offsets.
**Why it happens:** PPF uses raw-file offsets while providers map logical LBAs to source offsets, synthesize sectors, or provide virtual pregaps. PPF3 can disable its 1,024-byte blockcheck and has no PPF2-style image-size field. Local preparation labels PPF1/2 details unverified beyond method encoding.
**How to avoid:** Document and enforce a narrow supported source layout; match magic/method; validate header flags, record lengths, undo lengths, file-id/trailer placement and bounds; verify source identity with exact SHA where native checks are absent or insufficient; publish overlays only after full parsing. Treat sample block checks as compatibility hints, not cryptographic identity.
**Warning signs:** patch offsets hit synthesized headers; source mode changes offsets; truncated trailer accepted; a malformed record after a valid prefix still changes reads.

### BPS whole-file parsing
**What goes wrong:** Bad varint/cursor arithmetic, invalid TargetCopy or late CRC failure yields partially patched reads.
**Why it happens:** BPS has separate source and target cursors; four action types emit every target byte; TargetCopy reads only previously emitted target bytes; CRCs appear after actions.
**How to avoid:** Bound source/target to existing media limits, check every cursor and length without overflow, verify source and patch CRC, write the target to bounded staging storage, verify target CRC and sector geometry, then switch the reader. Reject malformed/truncated footer and target size changes.
**Warning signs:** in-place writes, unchecked varint shifts, TargetCopy at or beyond current output, offset-plus-length arithmetic without overflow checks.

### Cheat parser and memory effects
**What goes wrong:** Invalid replacement partially installs lines or writes RAM; unsupported conditionals act differently between hosts; a write reaches MMIO/out-of-range memory.
**Why it happens:** Libretro callbacks return void and cheat strings are untrusted; conditional/timing semantics beyond constant writes are unsettled.
**How to avoid:** Parse into temporary normalized records, validate full input and caps, then replace config atomically. Restrict writes to aligned main RAM. Keep order and enable state explicit. Validate the complete serialization envelope before the core or host state changes. Test order, repeated writes, reset/disable behavior, invalid replacement and state continuation.
**Warning signs:** parser mutates live table while reading; invalid address is masked into range; memory changes during validation; only core bytes are serialized despite active cheats.

### Compatibility matrix and DAT
**What goes wrong:** DAT entries with duplicate/missing serials disappear; stale rows count as pass; the metadata reports support without boot/play evidence.
**Why it happens:** Joining by title or serial can collapse source occurrences. Existing feature rows are synthetic. Some current claims are hard-coded.
**How to avoid:** Count every source occurrence before identity joins; preserve unmatched and duplicate totals; key result rows by supported identity; build current-run matrix from exact config/test evidence; fail on missing, stale, duplicate, unknown and failing rows. Keep inventory separate from capability/play evidence.
**Warning signs:** counting unique serials only; counting profile.select as playable; empty result treated as success; protected CI base sparse checkout lacks features.csv.

The DAT is the CC-BY-SA-4.0 libretro-database Redump input. Fetch only the pinned source and digest already recorded in `scripts/library.sh`; count each source `<game>` occurrence before joining any title, serial or digest evidence. Tally each occurrence as pass, fail or unknown, explicitly retaining missing/duplicate serial counts, and report a total that reconciles with the 13,407-entry requirement. Keep the raw DAT and a bulk title table out of git and published reports; only allowed sparse evidence rows and the attribution notice may remain. The current download/digest code is lines 114-118, while asset and provenance policy prohibit Redistributing the bulk DAT and state the notice requirement. [VERIFIED: `.planning/REQUIREMENTS.md:66`; quote: `all 13,407 source entries`; `scripts/library.sh:114-118`; `ASSET_POLICY.md:29-31`; `PROVENANCE.md:187-190`]

Keep the capability matrix separate from DAT inventory. For each declared build configuration, derive the required row set from the requirement categories and compare it with fresh attributed results. Missing, duplicate, stale, unknown or failing rows must leave the experimental flag true. The protected-base workflow sparse checkout currently includes `tests/compat/ledger.csv` but omits `tests/compat/features.csv`, and the workflow consistency test also encodes the old file list. Include the feature ledger in both the workflow checkout and its CMake assertion before making the feature ratchet authoritative. `info_claims.cmake` currently requires `is_experimental true` and unsupported `cheats false`; revise the guard only after tests supply the corresponding current evidence. [VERIFIED: `.github/workflows/ci.yml:395-400`; `tests/cmake/workflows.cmake:15`; `tests/cmake/info_claims.cmake:48-69`; `.planning/REQUIREMENTS.md:68`]

### Safe test and CI decomposition

Keep tests in the existing CMake/CTest flow and make every new behavior gating per the repository policy. Plan independent parser-focused suites, but let the first plan create the required CMake skeleton and per-directory test inventories. Use self-authored fixtures so no third-party patch bytes, commercial media, BIOS or save state enter the repository. [VERIFIED: `.planning/REQUIREMENTS.md:8-14`; `.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md:238`; project rules in `AGENTS.md` §5 and §14]

Recommended focused cases: (1) PPF1/2/3 valid records, PPF3 with and without block-check and undo data, overlap order, sector-boundary offsets, wrong image check/source SHA, huge/truncated offset and invalid later record after a valid prefix; prove the caller's image and reader remain unchanged on every failure. (2) BPS SourceRead, TargetRead, SourceCopy, overlapping TargetCopy, malformed/overflowing varints and relative cursors, each of three CRC failures, truncation, target-size/sector-geometry mismatch, short I/O and cleanup; prove no patched sector is visible until commit. (3) cheats at byte/halfword boundaries, address translation/window, order of repeated same-frame writes, disabled entries, invalid line/table replacement, and runner hash/libretro serialization round trips including unknown version, bad length/checksum and invalid cheat entry; compare complete pre/post core and host state after every rejected operation. (4) DAT duplicates, missing serials, no matching evidence and mixed pass/fail/unknown counts; assert per-occurrence accounting and no bulk title output. (5) matrix stale/fresh/duplicate/missing/unknown/failing result rows and metadata true/false generation; assert false is reachable only with every required row passing. [ASSUMED: these test groups are suggested decomposition; exact filenames and caps are discretionary]

Run inexpensive checks first: required input existence/count, sorted row inventory, evidence freshness and uniqueness, feature-ledger expected rows, sparse-checkout contents, repository-check line budget, CMake selector wiring and JUnit test counts. Then run changed CTest targets and the full `cmake --workflow --preset ci` only once on the changed head. For TODO052, include both `libretro.host` and `libretro.input.movies` in one exact-head timing/coverage comparison; keep all cases and assertions, compare the complete critical path and avoid duplicating jobs if the shard saves no wall time. Evidence must be command, exit status and CI run URL, not a committed log. [VERIFIED: `.planning/REQUIREMENTS.md:12-14`; `.planning/phases/16-patches-cheats-and-compatibility-evidence/16-CONTEXT.md` D-08; `.planning/todos/pending/052-be32-movie-test-shard.md`; project rules in `AGENTS.md` §11 and CI/CD efficiency]

## Code Examples

### Checked source range
~~~c
if (file_offset > source->size ||
    track->sector_bytes > source->size - file_offset) {
    return 0u;
}
~~~
This exact guard is at host/disc_image.c:1423-1424.

### Existing state validate/apply
The state loader checks the complete payload, runs VALIDATE without mutation, then APPLY after all validation succeeds (src/state.c:247-263). Keep that order when adding a libretro envelope: validate envelope/version/size and the full cheat table first; call the core's loader next; update host cheat state only after the core accepts its state.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| No-op libretro cheat callbacks and no runner patch option | Host parsing plus narrow RAM operation and sector overlay | Phase 16 | Adds bounded behavior without moving parsing into core. |
| Local library output counts unmatched CUE media | Pinned DAT source-occurrence accounting with explicit pass/fail/unknown totals | Phase 16 | Shows inventory size without a playability claim. |
| Profile-select rows treated as compatibility evidence | Separate fresh required-capability matrix | Phase 16 | Keeps experimental metadata tied to actual measured requirements. |

**Deprecated/outdated:**
- The info-claim guard unconditionally requires experimental=true and cheats=false. Revise it with actual patch/cheat tests and the fresh required matrix; do not simply weaken the assertion.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | PPF1/2 field layouts in the preparation are accurate enough to implement. | PPF identity and mapping | Incorrect header lengths shift parsing; local source notes explicitly mark them unverified. Require exact byte fixtures and independent parser expectations before claiming support. |
| A2 | Chosen raw-track file layouts have exact byte-to-sector mapping. | Architecture Patterns | Wrong map patches different data; reject any layout not shown by provider source and a known-answer test. |
| A3 | A BPS target can be staged within the media bound and available temporary storage. | BPS whole-file parsing | If not, reject safely or use capped temp-file output; always clean on error and preserve input. |

## Resolved Research Questions

1. **Native source identity — RESOLVED.** Require the caller-supplied whole-source SHA-256 for every PPF version, then enforce each version's native checks. PPF2 carries size and block checks; PPF3 has no source-size field and its 1,024-byte block check is optional; PPF1 has no native source check. BPS uses its source/target/patch CRCs. Report the checks performed and do not call CRCs or block checks cryptographic identity. This follows locked D-01 and is implemented/tested in plans 07–10.
2. **Raw provider mapping — RESOLVED.** Support one CUE data track backed by a raw file whose bytes start at file offset zero, with no stored or virtual pregap. Reject other layouts until they have an exact mapping. Plan 07 adds independent byte-to-sector fixtures at offset zero, the final byte and a 2352-byte boundary; Plan 10 exercises the same production sector-provider path through the actual runner CLI.
3. **BPS staging — RESOLVED.** Use the plan-selected bounded staging policy: stage targets up to 1 MiB in memory; use private temporary storage for larger targets, within the existing 1 GiB logical-image bound, with a 64 MiB patch bound and 1 MiB metadata bound. Validate CRCs and unchanged sector geometry before publication; clean the task-owned temporary target on every exit. Plan 09 covers transformation and cleanup.

These resolutions select conservative support boundaries; they do not assert that unimplemented layout variants are supported.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake | Build and CI workflow | Yes | 4.4.3 | None needed |
| Python 3 | Existing generated inventory/test helpers | Yes | 3.14.4 | Keep emulation/parser logic in C |
| GitHub CLI | Pin and hosted-run lookup; phase ship | Yes | 2.101.0 | None for GitHub actions |
| act | Local workflow routing if CI shard changes | Yes | Installed | Hosted runs remain required |
| Docker | act reproduction | Yes | Installed | Hosted CI supplies host/QEMU truth |
| curl and shasum | Pinned local DAT fetch/digest | Yes | curl 8.7.1; shasum present | Existing library script |
| ctx7 | Library/API doc lookup | No | — | No external library required; use vendored header and format specs |

**Missing dependencies with no fallback:** None.

**Missing dependencies with fallback:** ctx7 is not installed; the phase adds no dependency and its API behavior is in vendored libretro headers.

## Sources

### Primary (HIGH source authority; MEDIUM research-tier confidence)
- Icarus/Paradox, PPF3 developer format at pinned commit 4f24f8c784faf27b54081caee3835fa59a8a009e and the corresponding PPF distribution README — method/header fields, optional blockcheck, 64-bit offsets and PPF3 image-size check removal.
- byuu, BPS File Format Specification — source/target size, variable integer encoding, four target actions, CRC footer, cursor and TargetCopy constraints.
- Vendored libretro header, libretro/libretro.h:8966-9007 — state size/serialize/unserialize and cheat reset/set contracts.
- Pinned DAT download in scripts/library.sh:114-118 and licensing restrictions in ASSET_POLICY.md:29-31 and PROVENANCE.md:187-190.

### Secondary (MEDIUM)
- Project-pinned PlayStation preparation PSX-PERIPHERALS-AND-SAVES.md:219-265; source PER.07 points to the pinned stable PSX-SPX cheat reference. It documents GameShark 30 byte and 80 halfword writes, plus explicitly calls PPF1/2 field details unverified.
- docs/compatibility.md:21-25; tests/compat/features.csv; tests/cmake/info_claims.cmake:48-69; .github/workflows/ci.yml:395-400 — current evidence boundaries, hard-coded metadata defaults, and protected CI checkout contents.
- include/schwaystation.h:14,319-334,356-368; src/instance.c:139-165; src/state.c:222-268; host/disc_image.c:1378-1446; libretro/libretro.c:1932-2010; runner/main.c:1321-1345 — current APIs, provider mapping, state behavior and hash inputs.

### Tertiary (LOW)
No tertiary-only claim supports an implementation decision.

## Security and Validation Notes

The config contains the exact entries `"nyquist_validation": false` and `"security_enforcement": false`, so omit GSD Validation Architecture and Security Domain/ASVS sections. Hostile-input bounds, transactional application, parser fuzz targets and required CTest tests remain mandatory from project requirements. [VERIFIED: `.planning/config.json:37-42,65`; quotes: `"nyquist_validation": false`, `"security_enforcement": false`; `.planning/REQUIREMENTS.md:8-14`]

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — existing C17/CMake/POSIX-sh patterns and hash helpers are in the repository.
- Architecture: HIGH — host/core ownership and frame/state paths were read directly.
- Pitfalls: MEDIUM — BPS and PPF3 behavior have format-author sources; PPF1/2 details remain unverified in local preparation.

**Research date:** 2026-11-25
**Valid until:** 2026-12-25
