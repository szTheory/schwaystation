# Phase 11: ECM and CloneCD images - Context

**Gathered:** 2026-11-21
**Status:** Ready for planning

<domain>
## Phase Boundary

Add fixture-proven ECM and CloneCD `.ccd`/`.img`/optional `.sub` readers to the existing host disc-image
helper. The runner and libretro must receive the same sector, TOC, index, pregap, and subcode behavior through
the existing `schway_disc` contract. Preserve content identity and the deterministic core boundary. Reject
malformed, ambiguous, out-of-bounds, or unsupported inputs before publishing a reader or opening a companion
outside the selected image directory. Record evidence for both providers in the compatibility feature ledger.

This phase does not change the public ABI, add a second core path, guess undocumented CloneCD metadata, or
introduce generic licence-scanning infrastructure. Keep the approved 3,901-line repository-check ceiling.
</domain>

<decisions>
## Implementation Decisions

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

### Trade-offs by role

| Lens | Benefits and trade-offs | Failure modes, patterns and lessons |
|---|---|---|
| Product and end user | Users can reuse compressed archives while seeing one stable disc contract and content identity. | A readable header is not proof of bootable media. Call a claim supported only when the source bytes, TOC and guest-facing path are verified. |
| CD-ROM domain | Explicit mode/index/pregap and subcode mapping preserves the fields the drive consumes. | ECM lacks a TOC; CCD evidence is partly reverse-engineered. Do not infer indexes, track modes or subcode order from filenames or common practice. |
| Architecture | Format parsing remains in the host helper; runner and libretro reuse the same provider. | A format-specific core path or public ABI extension multiplies behavior and compatibility obligations without adding needed information. |
| C and toolchain | Checked integer arithmetic, bounded allocations and generated fixtures fit the portable C17 project. | Narrowing before validation, signed LBA conversion, overflow and endianness can produce host-specific sector maps. Run focused native/BE32/WASM consumers. |
| Security and fuzzing | Open-time validation and canonical companion confinement bound hostile records and text input before guest boot. | Lexical path checks alone miss symlink escapes; full-file rescans per guest read create a denial-of-service path. Reuse current confinement and fuzz seams. |
| Performance and WASM | Sparse/bounded index avoids whole-image RAM growth and keeps repeat reads practical under the existing media/memory limits. | Index every tiny record or decode the prefix on every sector can consume memory or make emulation crawl. Measure actual owned fixtures and cap per-read work. |
| Frontend integrator | The unchanged callback lets both hosts receive equivalent bytes and optional subcode behavior. | Do not add file I/O to the core or a second image lifecycle that behaves differently across hosts. |
| DevOps and CI | A proven metadata-only Nightly exception removes a measured irrelevant run while all code changes retain full tests. | Broad path filters can silently hide regressions. The route must fail closed and have negative controls before a hosted run. |
| Licensing and maintenance | The ECM author spec and project-pinned CloneCD reference are enough to implement from format facts without importing another runtime dependency. | No GPL emulator source may be opened or copied. If future code is vendored, audit the exact pin and retain its notice. |

### Adversarial pass

- Type-0 spans can be unaligned and can straddle requested sectors; compute decoded byte positions, not record
  counts as LBAs. A global EDC pass does not excuse a broken sector map.
- A sparse index is useful only if decode-forward work is capped even for highly fragmented valid streams.
  Keep open-time validation separate from per-read bounds and test repeated and reverse-order reads.
- If type 2/3 records omit bytes needed by the callback, do not fill them from an assumed standard unless the
  specification and independent byte vectors prove the rule. Reject unproven layouts clearly.
- CCD PLBA, indexes, track mode and image offset can disagree. Fail closed before open; do not turn a conflict
  into a plausible but shifted TOC. Missing INDEX 0 has no safe inferred duration.
- A `.sub` transpose can be wrong in either direction and still look plausible. Use a hand-computed channel
  vector and verify Q extraction through the production callback path.
- A safe-looking companion filename may be a symlink to outside the selected directory. Canonical confinement
  must happen before reading, with tests for traversal, absolute/drive paths and symlink escapes.
- The Nightly exception is valid only for the exact complete generated metadata set. Unknown path-list states,
  partial API results and mixed changes must retain full Nightly. Scheduled and manual runs remain full.
- Keep the gate ceiling, all existing assertions, Main and Release lanes, and the generic scanner deferral.
  The measured release matrix is a reason to fix the narrow Nightly false positive, not a reason to remove a
  required release matrix.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Scope and decisions
- `AGENTS.md` — deliverables, host/core boundary, fixtures, line cap, commits and required phase release rules.
- `.planning/AUTONOMOUS.md` — autonomous phase boundaries, fan-out, adversarial review, CI and release efficiency.
- `.planning/PROJECT.md` — product objective and supported deliverables.
- `.planning/REQUIREMENTS.md` — ECM-01, CCD-01 and gate budget.
- `.planning/ROADMAP.md` — Phase 11 goal, success criteria and canonical references.
- `.planning/STATE.md` — Phase 10 release evidence and Phase 11 recommendation.
- `.planning/LESSONS.md` — prior media, test, fixture and CI integration lessons; re-read during planning.
- `.planning/phases/09-chd-disc-images/09-CONTEXT.md` — shared provider, media preflight, feature ledger and path confinement patterns.
- `.planning/phases/10-pbp-disc-images/10-CONTEXT.md` — bounds, active-reader preservation, fixture preflight, scanner deferral and CI rules.
- `.planning/phases/10-pbp-disc-images/10-CI-EFFICIENCY.md` — measured current-main/release workload and exact metadata-only Nightly issue.

### Domain evidence and interfaces
- `.planning/preparation/README.md` — preparation index; read before other preparation sources.
- `.planning/preparation/BRIEF.md` — source tiers, citation style, licensing and research rules.
- `.planning/preparation/DECISIONS.md` — DEC.42/43 host provider and `schway_disc` boundaries; do not reopen without evidence.
- `.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md` §3 — image layouts, TOC, sector and subcode contracts; SCM.05 and SCM.32.
- `.planning/preparation/HOSTS-AND-INTEGRATION.md` — shared runner/libretro helper and host file access.
- `.planning/preparation/CONFORMANCE.md` — fixture admission, source coverage, ledger and fuzz evidence.
- `.planning/preparation/ENGINEERING.md` — build presets, tests, CI, release and absolute gate budgets.
- `.planning/preparation/PROVENANCE.md` and `PROVENANCE.md` — permitted sources, pin and notice requirements.
- `.planning/ASSET_POLICY.md` — build-local generated media and fixture limits.
- `.planning/research/FEATURES.md` — v2.0 ECM and CCD expected user behavior and evidence boundaries.
- [ECM Format Specification v1.0](https://sources.debian.org/src/ecm/1.00-1/format.txt/) — Neill Corlett's primary record/terminator specification, recorded as SCM.32.
- [PSX-SPX CloneCD image reference](https://psx-spx.consoledev.net/dev/ps1/cdr/cdromfileformats/diskimages/) — curated raw IMG/SUB sizes, channel-major P–W description, optional SUB and CCD field notes; not a complete official standard.
- [GNU ccd2cue manual](https://www.gnu.org/software/ccd2cue/manual/ccd2cue.html) — reverse-engineered, explicitly incomplete CCD field and index interpretation.

### Existing implementation and checks
- `include/schwaystation.h` — public disc callback and TOC/index fields; this phase should not change the ABI.
- `host/disc_image.c` — dispatcher, existing CUE source handling, companion-path checks and active-image replacement behavior.
- `host/disc_pbp.c` and `host/media_limits.h` — open-time extent validation, bounded provider pattern and media limits.
- `src/cdrom_sector.c` — consumer of callback subcode and Q-channel bit placement.
- `tests/disc/image.c` and `tests/disc/tests.cmake` — provider parity, failure preservation and sorted test inventory.
- `tests/compat/features.csv` — protected feature evidence ledger.
- `scripts/classify-changes.sh` and its test — Nightly route classifier and current fast-route assertions.
- `.github/workflows/nightly.yml`, `.github/workflows/ci.yml`, `.github/workflows/release.yml` — routing, required matrices and release behavior.
- `docs/media-testing.md` and `docs/maintaining.md` — media preflight and maintainer verification guidance.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `host/disc_image.c` routes file-backed images through one host seam and already has path confinement and
  failure-preservation behavior.
- `host/disc_pbp.c` validates full media extents on open and offers a host-only bounded reader pattern.
- `host/media_limits.h` centralizes media size constraints and checked integer helpers.
- `src/cdrom_sector.c` consumes the callback's raw sector and interleaved P–W subcode representation.
- `tests/disc/image.c`, the sorted inventory, the production-parser fuzz targets and the compatibility ledger
  already exercise the actual provider interfaces.

### Established Patterns
- Validate all table and companion extents before publishing a provider.
- Keep source and decoded byte/sector counts distinct; check the caller's request and guest output before
  starting long guest work.
- Use owned generated fixtures; never add commercial disc, BIOS, executable or private host data.
- Compare reader bytes, TOC and identity against the uncompressed source; generated fixture and expected data
  must not use the same unchecked implementation for both sides.
- Preserve LF, sort exact inventories bytewise, and run cheap path/extent/selector/count/budget checks before
  broad native, fuzz, hosted or emulation workloads.

### Integration Points
- Register `.ecm` and `.ccd` routing in the existing host dispatcher; allow ECM-backed CUE files only where the
  same source/track rules are proven.
- Feed both runner and libretro from the existing `schway_disc` callbacks; keep core and public header unchanged.
- Add exact sorted tests and compatibility-ledger rows for each provider, with malformed and path-escape cases.
- Wire the conservative metadata-only Nightly case into the existing classifier without changing scheduled or
  manual runs, main CI, release CI or their required checks.
</code_context>

<specifics>
## Specific Ideas

- ECM record bytes are not equivalent to sectors; exercise first/last, repeated, reverse and interleaved random
  reads against an independently generated raw image.
- For CloneCD, include explicit track/index/pregap evidence and channel-major SUB bytes with visible distinct
  bit patterns in P and Q so the transpose direction is unambiguous.
- Preflight descriptor extent, `.img` sector count, `.sub` byte count, requested sectors, generated guest reads,
  workflow selectors and line budget before long tests or hosted CI.
- Record the Nightly routing check's exit status before dispatching hosted CI; use `act --dryrun` for selector
  routing, then retain hosted required checks for GitHub-only behavior.
</specifics>

<deferred>
## Deferred Ideas

- Generic tracked-source SPDX scanner: explicitly deferred by the owner; retain exact dependency notice and
  provenance checks within the current ceiling.
- Broader Nightly or release workflow redesign: no evidence supports dropping platform coverage, merging Main
  and Release checks, or changing required lanes in this phase.
- Unsupported/undocumented CCD versions, track modes, arbitrary companion-path syntax, and inferred pregaps:
  defer until primary evidence and an independent fixture establish exact behavior.
- ECM variants that cannot be mapped byte-for-byte to the 2352-byte callback: report unsupported until that
  mapping has evidence; no public ABI expansion is in scope.

</deferred>

---

*Phase: 11-ecm-and-clonecd-images*
*Context gathered: 2026-11-21*
