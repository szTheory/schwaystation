# Phase 11: ECM and CloneCD images — Research

**Researched:** 2026-11-21
**Domain:** bounded host-side media providers and conservative CI routing
**Confidence:** MEDIUM for documented formats; proposed bounds require implementation tests.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

### ECM representation and random access

- **D-01:** Follow Neill Corlett's ECM v1.0 record and termination rules. Perform one bounded open-time scan to
  validate the complete encoded extent, decoded byte/sector count, terminator and global EDC before making the
  reader available. Provide bounded repeated and out-of-order sector reads with a bounded index/checkpoint
  scheme; do not rescan the full image per read or materialize the entire decoded image in memory.
- **D-02:** Preserve source bytes exactly for every supported record/sector layout. ECM type 0 may carry
  arbitrary literal byte spans, so record boundaries cannot be treated as sector boundaries. Resolve the exact
  type 1–3-to-2352-byte mapping with primary-spec reading and byte-for-byte owned fixtures. Reject any record
  variant whose callback-sector mapping is not demonstrated; do not synthesize unproven header bytes.
- **D-03:** ECM carries no TOC. Reuse the existing CUE source and bare-image policy where it can provide a
  documented track layout; require exact sector alignment for any single-track fallback. Never infer a
  multi-track table from compressed record boundaries. Document the supported open path and clear errors.
- **D-04:** Bound source size, decoded output, record count/index storage and decode-forward work using checked
  arithmetic. Keep random-read work bounded independently from the full-file validation pass. Preserve the
  active disc if opening a replacement fails.

### CloneCD descriptors and companions

- **D-05:** Support only descriptor versions and fields demonstrated by owned fixtures. Resolve companions by
  the descriptor's same-stem `.img` and optional `.sub` convention; do not accept descriptor-controlled
  arbitrary paths. Confine companion resolution to the selected image directory before opening files, including
  canonical symlink escape checks.
- **D-06:** Reconcile track mode/index fields with `[Entry]` PLBA values and the image's exact sector extents.
  Preserve explicit INDEX 0 / INDEX 1 and pregap positions. Reject conflicting or ambiguous metadata and never
  invent the common two-second pregap when INDEX 0 is absent. Accept only mode and session variants whose TOC
  mapping is fixture-proven.
- **D-07:** A missing or empty optional `.sub` follows the existing no-subcode path. A non-empty `.sub` must
  cover the corresponding `.img` sectors exactly. The project-pinned PSX reference describes CloneCD P–W data
  as channel-major 12-byte groups; convert those groups into the existing callback's bit-interleaved P–W byte
  form. Add an independent known-answer transpose vector so fixture generation and reading cannot share the
  same unnoticed layout error. Keep the documentation's confidence boundary: there is no complete official
  public CloneCD specification.

### CI iteration and scope

- **D-08:** Add a conservative PR-Nightly fast route only for the complete trusted changed-file set
  `.release-please-manifest.json`, `CHANGELOG.md`, and `version.txt`, which are generated release metadata.
  Missing, mixed, incomplete, unknown, renamed, malformed or untrusted file lists run the full Nightly route;
  scheduled/manual runs do not change. Preserve every test and every CI/release matrix lane.
- **D-09:** The owner selected the existing 3,901-line gate cap and deferred the generic SPDX scanner. Keep that
  scope; preserve exact CloneCD/ECM notice and provenance work when any dependency or source material is added.
- **D-10:** Keep Main CI and Release CI as separate required checks. Phase 10 measured different bytes in all
  18 product archives, so a green run cannot substitute for the other workflow. Before long suites or hosted
  CI, run the relevant cheap inventory, extent, selector, workflow-routing and budget preflights. Use
  `act --dryrun` for routing and `act` only when it covers executable steps not already covered locally. Keep
  hosted-only evidence in GitHub and inspect job timing while running and after completion.
- **D-11:** Use generated, owned fixture bytes and source-vs-reader comparisons. Check source extents and
  requested/produced sector counts before guest execution. Keep provider, shared consumer, malformed, fuzz/replay,
  endian, sanitizer and applicable wasm32 coverage in the existing test and CI structure. Sort the exact test
  inventory before configuring.

### the agent's Discretion

No separately named discretion section exists in CONTEXT.md; implementation choices below implement its locked decisions.

### Deferred Ideas (OUT OF SCOPE)

- Generic tracked-source SPDX scanner: explicitly deferred by the owner; retain exact dependency notice and
  provenance checks within the current ceiling.
- Broader Nightly or release workflow redesign: no evidence supports dropping platform coverage, merging Main
  and Release checks, or changing required lanes in this phase.
- Unsupported/undocumented CCD versions, track modes, arbitrary companion-path syntax, and inferred pregaps:
  defer until primary evidence and an independent fixture establish exact behavior.
- ECM variants that cannot be mapped byte-for-byte to the 2352-byte callback: report unsupported until that
  mapping has evidence; no public ABI expansion is in scope.
</user_constraints>

## Project Constraints (from AGENTS.md)

[VERIFIED: AGENTS.md, rules 1–14 and Style] Apply these actionable constraints:
- Implement in the host helper, preserving the integer-only, instance-owned deterministic core and unchanged public ABI. Keep library, runner and libretro deliverables.
- Never open forbidden emulator source or keep it in the workspace. Use documentation facts in original prose. Any copied permissive source needs its exact public pin, per-file licence and notices.
- Generate owned media under the build directory; commit only fixture sources admitted by the root asset policy and manifest. No private paths or firmware/media bytes.
- Every behavior change has a gating test selected by the CI workflow. Plan 1 owns CMake structure and exact sorted per-directory inventories. Split plans beyond 30 minutes; parallelize only disjoint file/header/inventory ownership.
- Preserve all assertions and named lanes, the 3,901-line repository-check cap and 300-line RetroArch test cap. Do not relocate or minify counted code to evade budgets. Generic SPDX scanning stays deferred.
- Review code before final verification. Use automation; do not ask the owner to inspect ordinary results or perform unavailable hardware captures.
- Use phase branch commits with the noreply identity, never main or the prohibited override. Phase release/merge rules and current autonomous authorization remain controlling.
- Evidence is a command, exit status and hosted URL where applicable, written by the command; never commit raw logs. Inspect CI critical paths and duplicate work. Background launches are the default.
- Each SUMMARY includes requirements-completed and a block-style coverage list with id, description, requirement, verification(kind/ref/status), human_judgment. Record actual runtime/model and elapsed-time evidence in STATE.
- Keep small plain C17 modules, fixed-width integers, documented hardware names and source citations; no planning IDs in code/test names/user docs. Prefer existing code and the standard library over dependencies.

## Summary

[CITED: https://sources.debian.org/src/ecm/1.00-1/format.txt/] ECM is a decoded byte stream, not a sector-record container. Types 2/3 deliberately omit the first 16 sector bytes; a full raw source can preserve those bytes through literal records. Build byte-offset checkpoints, validate the whole stream once, and test exact source equality before exposing the provider.

[CITED: https://www.gnu.org/software/ccd2cue/manual/ccd2cue.html] The GNU project describes its CCD specification as reverse-engineered and incomplete. Treat support as an explicit, fixture-proven subset, not a claim of universal CloneCD compatibility.

[VERIFIED: .planning/phases/11-ecm-and-clonecd-images/11-CONTEXT.md, D-01–D-11] Both providers must use the current disc callbacks, preserve active media on failure, and carry bounds and format errors through the shared host path. The independent narrow Nightly fix can proceed alongside provider work.

## Architectural Responsibility Map

| Capability | Primary tier | Secondary tier | Rationale |
|---|---|---|---|
| ECM parsing, EDC/ECC reconstruction and random access | host helper | owned generator/tests | File IO and hostile parsing stay outside the core |
| CCD text, companion confinement, TOC and SUB conversion | host helper | existing CD-ROM callback consumer | No second core path |
| Loading, probing, replacement and identity | runner/libretro | shared provider | Preserve one disc lifecycle |
| Exact metadata-only route | existing classifier/workflow | route controls | No CI/release matrix redesign |

[VERIFIED: .planning/preparation/DECISIONS.md:95–103] This map implements the settled host/file boundary and helper ownership.

## Standard Stack

[VERIFIED: AGENTS.md, Style; cmake/DiscTargets.cmake:7–27] Use existing C17, CMake/CTest, Python fixture generation and POSIX sh/CMake repository controls. No external runtime dependency or package installation is required. The existing target is quoted verbatim: `add_library(schwaystation_disc_image STATIC` with `host/disc_image.c host/disc_chd.c host/disc_chd_metadata.c` and `host/disc_pbp.c host/disc_pbp_metadata.c`. New module filenames are planner-selected additions, not existing artifacts.

## ECM Format Facts and Mapping

[CITED: https://sources.debian.org/src/ecm/1.00-1/format.txt/] The four-byte signature is 45 43 4D 00. Record type occupies the first byte's two low bits; count-minus-one spans up to five bytes. Ordinary counts at or above 2^31 are invalid. Type 0 with encoded count FFFFFFFF terminates records, followed by a four-byte whole-decoded-file EDC and EOF. Type 0 emits its literal bytes. Type 1 stores three address and 2048 data bytes and reconstructs 2352 bytes. Type 2 stores four flags and 2048 data bytes; type 3 stores four flags and 2324 data bytes; each reconstructs 2336 bytes. Duplicate flags and documented integrity bytes are reconstructed.

**Planning recommendation (derived from those facts):** Define source mapping over cumulative decoded bytes. A raw sector may be assembled from several literal/modelled units. Never use record ordinal as LBA. For types 2/3, require owned raw-2352 fixtures in which source header bytes are supplied literally before the modelled 2336-byte tail. Validate against the original raw file. A bare 2336 layout without a demonstrated existing CUE mapping is unsupported; do not create missing address bytes from LBA.

[VERIFIED: host/disc_image.c:163–233] Existing Mode1 code has `schway__disc_image_ecc`, `schway__disc_image_mode1_header`, `schway__disc_image_mode1_trailer`, and the polynomial literal `UINT32_C(0xd8018001)`. Reuse/refactor private arithmetic where justified, but do not let the expected fixture invoke the production reconstruction implementation.

**Independent proof recommendation:** For XA, generate valid owned raw sectors with the existing pinned media producer, strip only fields stored by ECM, and compare every reconstructed byte with the untouched raw source, including EDC/ECC. For Mode1, add a separately implemented small fixture generator plus frozen independently derived trailer known answers; do not compute both sides with a shared helper. Include malformed source integrity carried literally, crossing literals, mixed record types, nonzero addresses, and tail alignment rejection.

## Bounded Random Access Design

[VERIFIED: host/media_limits.h:7–24] The existing source-size cap is quoted verbatim: `#define SCHWAY__MEDIA_IMAGE_MAX_BYTES UINT64_C(1073741824)`. Checked arithmetic helpers are `schway__media_add_u64` and `schway__media_mul_u64`.

**Recommended new design; constants are choices, not existing limits:** Cap encoded and decoded extents at the current media maximum. During the complete validation/EDC scan, retain restart checkpoints at decoded-byte spacing no greater than 65,536 plus one emission unit. Save encoded offset, decoded offset, record type and remaining units; split literal runs and permit restart inside large repeated records. Modelled emissions are at most 2352 bytes, so limit one sector request to 65,536 + 2×2352 decoded work and an explicit crossed-header bound (70,240 is a conservative candidate). Maintain at most ceil(decoded_cap/65,536)+2 entries, or 16,386 at the existing cap. A fixed entry width no greater than 48 bytes keeps the index below 1 MiB. Derive and assert the actual structure-size/memory bound at compile/test time.

**Acceptance recommendation:** Test these caps at and beyond their boundary, including highly fragmented one-byte literals and huge literal/modelled run counts. Instrument read-at calls/bytes in a private test seam to prove repeated, reverse and interleaved reads obey the work bound independently from image length. Test missing terminator, excess continuation, invalid high count bits, global EDC mismatch, extra trailing bytes, allocation/IO failure and zero/unaligned output. Bounds failure must return before publishing a candidate.

## CloneCD Subset and TOC

[CITED: https://psx-spx.consoledev.net/dev/ps1/cdr/cdromfileformats/diskimages/] The documented IMG unit is 2352 bytes and SUB unit 96 bytes, with P–W represented by eight consecutive 12-byte channel groups. Missing/zero-length SUB means absent subcode. Version 3 has TRACK metadata; MODE 0/1/2 denotes audio/Mode1/Mode2. Track INDEX positions are numeric sectors; ordinary track PLBA identifies INDEX1. Multi-session index and PLBA origins differ. Metadata and numeric addressing are not BCD. Absent INDEX0 does not establish its duration.

**Recommended demonstrated subset:** Version=3, Sessions=1, DataTracksScrambled=0, CDTextLength=0, contiguous track numbers with complete A0/A1/A2 and per-track entries. Prove MODE 0/1/2 with independent owned sector fixtures; accept only modes actually proven. Require each track's INDEX1 to equal its entry PLBA, optional INDEX0 <= INDEX1, ordered nonoverlapping extents, correct first/last track and leadout equal to IMG sector count. Missing INDEX0 maps to no explicit pregap, not an invented two seconds. Reject Version2, multiple sessions, scrambling, unsupported indexes and semantic metadata that cannot be reconciled. Benign formatting tolerance must itself have tests.

**Companion recommendation:** Derive same-stem IMG and optional SUB from the selected descriptor, never arbitrary fields. Reuse canonical containment before opening contents. Include the symlink case, prefix sibling, traversal, absolute/drive names, case behavior, missing IMG, unreadable SUB, and short/long nonempty SUB. A missing optional companion differs from an existing unreadable or escaping companion. Check multiplication/length and signed PLBA before narrowing.

## Independent SUB Known Answer

**Derived vector, hand-computed from the documented grouping and existing callback consumer:** Input is 96 zero bytes except offsets 0=A0, 12=40, 23=01, 24=10, 62=20, 84=01 (hex values). Expected output is 96 zero bytes except offsets 0=80, 1=40, 2=80, 3=20, 7=01, 18=04, 95=40. Store these expectations literally; the generator must not call a production transpose routine.

For channel c in 0..7 and sample i in 0..95, output bit 7-c comes from input byte 12*c+floor(i/8), bit 7-(i mod 8). The asymmetric last-byte and third-byte values expose reversed sample/byte order.

[VERIFIED: src/cdrom_sector.c:181–200] Production Q extraction uses `(subcode[i] >> 6) & 1u` and shifts it by `7u - (i & 7u)`. Also add a distinct valid-Q/CRC fixture checked through GetlocP; the layout vector alone is intentionally not protocol-valid Q and proves no command behavior.

## Existing Integration Seams

[VERIFIED: host/disc_pbp.h:6–16] The current provider interface names are `schway__disc_pbp_open`, `schway__disc_pbp_close`, `schway__disc_pbp_toc`, `schway__disc_pbp_read_sector`; sector/subcode extents are `uint8_t sector[2352], uint8_t subcode[96]`. Mirror opaque ownership/error style; do not extend the public core ABI.

[VERIFIED: cmake/PBPFixtures.cmake:1–51] The CMake producer creates `${PROJECT_BINARY_DIR}/fixtures/disc_boot_pbp_source.bin` and `${PROJECT_BINARY_DIR}/fixtures/disc_boot_pbp_source.cue`; its target is `add_custom_target(schwaystation-pbp-source DEPENDS "${schway__pbp_source_bin}")`. It already has a one-data-track XA/FMV source. Reuse owned raw inputs rather than repeat external tool research/builds.

[VERIFIED: tests/disc/tests.cmake:1–430; tests/fuzz/tests.cmake:134–148] Provider/consumer and guest registration already use CTest setup fixtures to order source extent and parity checks before guest execution. Fuzz replay registration currently includes `fuzz.regress.pbp` and `FIXTURES_REQUIRED disc_pbp_preflight`. Extend those established seams and their exact inventories. The analogous ECM/CCD test names and files do not exist yet and must be created by the plans.

[VERIFIED: tests/compat/features.csv:1–3] Feature ledger header is `provider,profile,protection,peripheral,predicate,status,evidence,date`. Add truthful separate ECM and CCD evidence after actual provider/host/guest checks; parsing alone does not establish playability.

[VERIFIED: tests/cmake/workflows.cmake:12–47] Existing closure checks include `disc_registration`, CI/be32/wasm32 selection and Nightly fuzz arrays. Keep portable provider/replay tests selected on native, BE32 and applicable WASI paths. New fixture inputs need build-graph closure on every producing lane.

## Nightly Exact-Set Fix

[VERIFIED: scripts/classify-changes.sh:13–91; tests/cmake/ci_route_controls.cmake:25–48] Existing metadata recognition already checks exactly three distinct names with an explicit mode, but its positive control still expects `nightly_full=true`. The accepted mode is `release-metadata`; keys include `nightly_full` and `pbp_only`. Extend this existing branch; do not duplicate matching logic or invent a second classifier.

[VERIFIED: .github/workflows/nightly.yml:30–83] PR routing currently uses paginated/slurped files API data, a rename sentinel, count checks, and the classifier; scheduled/manual jobs default to full. Preserve those failure defaults. Plan complete-list trust checks, exact statuses and shape validation, count consistency and malformed/NUL/newline path negatives. A timeout, partial pages, renamed file, duplicate, unknown, extra, missing or mixed set must retain full Nightly. Keep Main and Release required checks separate.

[VERIFIED: .planning/phases/10-pbp-disc-images/10-CI-EFFICIENCY.md:163–190] Prior evidence identifies this exact release metadata false positive and distinct archive bytes in all 18 product archives. No broader CI deletion or matrix substitution is supported.

## Don't Hand-Roll

| Problem | Use instead | Reason |
|---|---|---|
| Independent guest disc/codec media | Existing pinned owned-media producers | Avoid reciprocal encoder/decoder tests |
| Path confinement | Existing canonical host seam | Lexical validation misses symlinks |
| TOC/identity/frontend paths | Existing shared disc callbacks and probe | Prevent format-specific core/identity behavior |
| CI routing | Existing classifier and controls | Preserve conservative fallbacks and cap |

These are design recommendations implementing the cited project boundaries; no package or source import is proposed.

## Common Pitfalls and Verification Order

- Treating every ECM modelled unit as a whole callback sector loses literal headers and shifts all following reads.
- A whole-file EDC pass can coexist with an incorrect LBA map. Compare complete reader bytes and TOC separately.
- Per-record indexing can exhaust RAM on one-byte literals; checkpoint by decoded distance and count work as well as output.
- A shared fixture transpose or ECC helper can make both sides agree on a wrong mapping. Keep independent vectors.
- Reusing an old provider's feature ledger claim is not evidence for the new provider.
- New inventory rows must be sorted before configure. Cheap extent/request/output counts, companions, workflow selectors and budget checks precede builds, emulator work and hosted CI.

[VERIFIED: AGENTS.md; .planning/AUTONOMOUS.md] Run focused preflights first, targeted native build/tests next, review/fixes before final verification, then existing required full native/endian/sanitizer/WASI/hosted lanes. Use act dry-run for routing; execute local act only if it adds coverage. No duplicate green broad suite for bookkeeping.

## Package Legitimacy Audit

Not applicable: this research proposes no external npm/pip/cargo packages and no new vendored runtime dependency. Exact provenance work remains required if execution discovers a genuine need for outside code; that would need a new evidence-supported scope amendment rather than silently importing ECM/GNU implementation source.

## Environment Availability

[VERIFIED: command -v act; command -v actionlint, 2026-11-21] Both tools are installed; hosted-only runners, artifact handoffs, permissions and release integrations still require GitHub evidence. Library docs/Context7 are unnecessary because this phase adds no library. GNU manual direct fetch timed out, but official-domain search returned its specification and explicit incompleteness statement. No forbidden source was opened.

## Assumptions Log and Open Questions

No training-only format assertion is promoted to a decision. Bounds and supported CCD fields above are explicit proposed implementation choices to be proven by tests. Public CCD documentation is incomplete; unsupported ambiguous variants are rejected under the locked policy, so no owner confirmation is needed. Literal-only ECM can prove the first vertical reader slice; modeled variants need their independent source vectors before admission.

[VERIFIED: .planning/config.json] Security enforcement and Nyquist validation are explicitly false, so their optional template sections are omitted. Hostile-input validation remains mandatory under project rules.

## Sources and Confidence

- Primary ECM author specification: https://sources.debian.org/src/ecm/1.00-1/format.txt/ — record expansion and termination facts, accessed 2026-11-21.
- Curated project-pinned PSX reference: https://psx-spx.consoledev.net/dev/ps1/cdr/cdromfileformats/diskimages/ — CCD/SUB layout and cross-check of literal headers, accessed 2026-11-21; project provenance pin remains controlling.
- GNU manual: https://www.gnu.org/software/ccd2cue/manual/ccd2cue.html — format is reverse-engineered and incomplete, official search extract, accessed 2026-11-21.
- In-repository source/read citations above — concrete present contracts, not hypothetical new filenames.

The confidence classifier returned LOW for unregistered provider labels webfetch/curated/official-docs; preserve that seam result rather than pretending it establishes a higher tier. Direct primary text and independent fixtures remain the acceptance basis. Source facts are CITED; design proposals are explicitly labelled. Research-store entries use the seam's LOW result. No package names were inferred.

<phase_requirements>
## Phase Requirements

| ID | Description | Research support |
|---|---|---|
| ECM-01 | The host decodes the documented ECM input subset with bounded random access, preserving sector bytes and rejecting invalid tables, offsets and truncated streams; a preflight confirms source extents and requested/produced sector counts before booting a guest. | Byte-stream reconstruction, bounded checkpoints, EDC/EOF validation, independent source parity and early counts |
| CCD-01 | The host parses the documented CCD descriptor subset and its confined IMG/SUB companions, preserving track modes, indexes, pregaps, subcode and first/last-sector reads; malformed or escaping companion paths are rejected before opening files outside the image directory. | Version3 single-session subset, reconciled indexes/PLBA/extents, canonical companions and asymmetric SUB vector |
</phase_requirements>

**Runtime/model:** Codex delegated researcher; inherited model identifier unavailable. No source implementation or tests were run; this artifact is planning evidence.
