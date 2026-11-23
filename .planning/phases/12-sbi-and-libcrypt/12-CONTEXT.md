# Phase 12: SBI and LibCrypt - Context

**Gathered:** 2026-11-23
**Status:** Ready for planning

<domain>
## Phase Boundary

Add bounded, host-side SBI parsing and attach the parsed bad-Q sectors to the exact selected disc-image provider
and its LBA map. Preserve that association across M3U selection and disc replacement, and keep data-sector delivery
independent from bad-Q position handling. Make GetlocP retain the previous good Q position for listed sectors,
including after a disc swap, with a short deterministic CD-ROM command-sequence test. Add malformed-input fuzz and
seed replay coverage and record only the protection predicates actually exercised in the compatibility ledger.

This phase does not add protected-title boot claims, PBP multi-disc support, a new public ABI, or an SBI identity
format that the file itself cannot provide. Existing SBI bytes do not contain a disc digest; association is by the
selected image provider and its TOC/LBA map, and cannot authenticate a mistakenly paired sidecar with overlapping
valid LBAs.
</domain>

<decisions>
## Implementation Decisions

### SBI parsing and provider binding
- **D-01:** Keep SBI parsing in the host `schway_disc_image` provider. Store each parsed overlay with its concrete image object, use its canonical image path and TOC/LBA map as the association, and expose bad-Q through the existing callback. For M3U, each child image owns its own sidecar and callback so a numeric LBA from one disc cannot select another disc's record.
- **D-02:** Accept only the documented SBI record encodings that can be mapped to an in-range LBA with checked arithmetic. Every valid listed sector is bad-Q because SBI carries no CRC. Reject malformed, truncated, unsupported, or out-of-map sidecars before publishing the image; an absent sidecar means no bad-Q overlay. Do not claim that SBI can authenticate its own image pairing.
- **D-03:** Disc selection changes the provider, TOC, bad-Q callback, and available identity as one unit. Clear cached Q-position state when the core applies a different disc, so GetlocP cannot report a previous disc's Q record.

### CD-ROM bad-Q behavior and evidence
- **D-04:** A bad-Q sector must not replace either the last accepted Q record or the previous-good position in the sector-read or seek path. It still advances the read target and delivers valid sector data through the normal data FIFO/DMA path.
- **D-05:** Prove this through a short guest-visible command sequence, not only an internal helper: establish a good Q position, swap to a disc with a different TOC/LBA context, read a good sector and a listed bad-Q sector carrying valid-looking Q, verify the bad sector's data, then assert GetlocP returns the prior good Q on the selected disc. Retain selector and inventory checks before any long run.
- **D-06:** Exercise the malformed-input parser through `fuzz.sbi` and seed replay. Update the compatibility ledger only for SBI/provider binding, bad-Q data availability, and previous-good GetlocP behavior that the fixtures actually test; do not imply retail-title compatibility.

### Iteration and CI efficiency
- **D-07:** In `libretro.host`, capture the identical runner PCM once per region, then keep both batch and single libretro comparisons for every region and all existing file-size/hash assertions. This removes three redundant guest captures; the current BE32 `libretro.host` lane measured 18m52s.
- **D-08:** Keep the complete required Main, Nightly, and Release lanes and the existing 3,901-line repository-check cap. The measured duplicate Main/Release matrices remain separate because their artifacts differ and exact-SHA reuse has not been proven safe. Do not add a workflow bypass or broaden path filters within this SBI change; any further routing optimization needs a bounded, fail-closed design and its own measured tests.

### the agent's Discretion
- Choose parser helper boundaries, precise accepted SBI encodings and caps from the project-pinned format facts, and the smallest owned test vectors that independently prove each mapping.
- Keep optional sidecar loading failure-atomic; routine absent-sidecar behavior and malformed-sidecar errors must be clear in provider docs.
- Keep generated SBI bytes in the build tree unless a small committed seed is explicitly listed in `tests/fixtures/manifest.txt`.

### Reviewed Todos
- **051 — Route Nightly pull-request jobs by changed work:** reviewed and left pending. It is a Phase 10 workflow-routing item, and its complete route controls did not fit the locked Phase 11 line cap; this phase changes no Nightly routing.
</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Scope and settled behavior
- `.planning/ROADMAP.md` — Phase 12 goal, dependencies and success criteria.
- `.planning/REQUIREMENTS.md` — CRYPT-01 binding, data delivery, command-sequence, fuzz and ledger obligations.
- `.planning/AUTONOMOUS.md` — autonomous phase boundaries, fan-out, adversarial review, preflights and CI/CD efficiency.
- `AGENTS.md` — license, core/host boundary, deterministic behavior, test budgets, phase and release rules.
- `.planning/LESSONS.md` — especially SW-CI-001 and the source/guest-coverage checks in SW-ORACLE-002/003.
- `.planning/preparation/DECISIONS.md` — DEC.42, DEC.43, DEC.46 and the settled SBI bad-Q/GetlocP question in Open Questions.

### Disc format and CD-ROM behavior
- `.planning/preparation/README.md` — preparation index and source navigation.
- `.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md` §2.7 — documented SBI records, CRC limitation and LibCrypt behavior; §3 — disc provider boundary; §5 — expected test matrix.
- `.planning/preparation/HOSTS-AND-INTEGRATION.md` — host and M3U selection integration.
- `.planning/preparation/CONFORMANCE.md` — fixtures, bounded fuzzing, compatibility ledger and confidence limits.
- `docs/hardware/cdrom.md` — current sector, Q, GetlocP and disc-swap contracts.
- `docs/discs.md` — supported image providers, sidecar-facing documentation and current M3U/PBP limits.
- `include/schwaystation.h` — existing `schway_disc.bad_q`, identity and disc-swap contract.

### Existing code and tests
- `host/disc_image.c` and `host/disc_image.h` — host provider objects, sidecar integration point and M3U child selection.
- `src/cdrom_sector.c` and `src/disc.c` — Q acceptance, bad-Q delivery and disc-apply behavior.
- `tests/cdrom/sectors.c`, `tests/cdrom/tests.cmake`, and `tests/cdrom/inventory.txt` — sector command behavior and sorted selectors.
- `tests/disc/image.c`, `tests/disc/tests.cmake`, and `tests/disc/inventory.txt` — provider, playlist and fixture checks.
- `tests/fuzz/` and `tests/fuzz/inventory.txt` — fuzz target, replay and inventory conventions.
- `tests/compat/features.csv` — feature evidence ledger.
- `tests/libretro/tests.cmake` — current redundant capture in `libretro.host`.
- `.planning/todos/completed/050-release-ci-artifact-reuse.md` and `.planning/todos/pending/051-nightly-pr-job-routing.md` — existing Main/Release and Nightly efficiency evidence.
</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `schway_disc_image` already owns each concrete M3U child and returns that child's descriptor on selection; this is the natural lifetime for a per-image SBI overlay.
- `schway_disc.bad_q` already separates bad-Q queries from sector reads, so no public ABI change is required.
- `src/cdrom_sector.c` already keeps `current_lba` at the previous position when the callback reports bad-Q, and `tests/cdrom/sectors.c` has a focused bad-Q selector to extend.
- Existing CMake fixture, sorted-inventory, malformed-input, fuzz replay, and compatibility-ledger patterns cover the required seams.

### Established Patterns
- The host owns file access and parsing; the deterministic core consumes sectors and callbacks only.
- Provider open and replacement are failure-atomic; media parsing uses checked arithmetic and bounded extents.
- M3U selects a concrete child image. PBP is currently single-disc, and M3U entries currently accept CUE/ISO only.
- Review precedes final verification; run cheap fixture, selector, inventory and budget checks before the long matrix.

### Integration Points
- Attach sidecar state to each image descriptor and ensure selected-disc swaps replace its `bad_q` callback with the selected child's callback.
- Prevent bad-Q Q-record updates in both the sector delivery and seek paths; reset cached Q state on disc replacement.
- Exercise parser fixtures in the disc-provider tests, GetlocP/data delivery in the CD-ROM command tests, and the host interface in runner/libretro consumers.
- Hoist the repeated PCM capture in the existing libretro host test while preserving both callback comparisons and all checks.
</code_context>

<specifics>
## Specific Ideas

- Adversarial review found that `sector_location` currently records valid-looking Q before the bad-Q callback is applied, and seek completion does not consult bad-Q. Cover both paths.
- Disc swap currently changes the callback and TOC without clearing the saved Q record. Test disc A → disc B so the stale-position case is observable.
- An SBI file has no embedded identity checksum; bind it to the exact provider object and map but document that the format cannot detect a semantically wrong sidecar when its records fit the selected map.
- The measured PCM optimization should retain three regions, six comparisons, and every capture hash/size assertion while removing three runner invocations.
</specifics>

<deferred>
## Deferred Ideas

- No release-matrix or Nightly-routing changes in this phase. Main/Release artifact reuse remains unsupported by the measured differing archive digests in todo 050; the strict metadata Nightly shortcut remains under pending todo 051 and its gate-budget constraint.
- Multi-disc PBP and authenticated SBI-to-disc pairing remain out of scope; the current SBI format has no disc digest and the accepted PBP layout rejects multi-disc markers.

</deferred>

---

*Phase: 12-sbi-and-libcrypt*
*Context gathered: 2026-11-23*
