# Phase 10: PBP disc images - Research

**Researched:** 2026-11-21
**Domain:** Bounded host-side PBP/PSISO parsing and raw DEFLATE; CMake, fixtures, and GitHub Actions routing
**Confidence:** MEDIUM — the upstream inflater pin/license are verified; PSISO format details are reverse-engineered and must be proven by owned fixtures.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** Accept only unencrypted PSISO layouts and track variants whose directory, TOC, sector mapping, and decoded bytes are demonstrated by owned fixtures. Start with the narrowest fixture-proven data-track layout; add another track mode only when its owned fixture proves the TOC and sector parity. Treat encrypted PGD, multi-disc switching, audio compression, unknown TOCs, and other undocumented layouts as unsupported.
- **D-02:** Separate malformed supported-layout data from valid-but-unsupported variants. Invalid offsets, arithmetic overflow, impossible counts/extents, inconsistent tables, and truncated blocks return a malformed-image result; encrypted or unrecognized layouts return unsupported. Both must fail through the host media boundary without partially replacing the selected image. Do not claim a guest boot for a format variant that has only parser coverage.
- **D-03:** Bound every PBP field, TOC entry, table, count, and allocation before narrowing or indexing. Confirm how compressed-block validation can detect truncation before guest execution while keeping memory bounded; use a fixed scratch area no larger than one decoded block rather than materializing the whole disc. The planner/researcher must settle the exact work bound and failure behavior from the implementation and fixtures.

### Provider and inflater
- **D-04:** Implement PBP as a separate host-only provider that plugs into the shared `schway_disc` contract and is called by the existing image dispatcher. Keep the core, public ABI, and guest-visible disc path unchanged. PBP must work with `SCHWAYSTATION_CHD=OFF` and without system zlib.
- **D-05:** Use the pinned modular inflater files `miniz_tinfl.c`, `miniz_tinfl.h`, and `miniz_common.h` under the upstream MIT grant. Exclude upstream `miniz.h`, whose stale Unlicense wording conflicts with the pin's MIT license record; supply the small project-owned compatibility header it needs and a generated export header. Keep these sources and notices distinct from libchdr's build-tree miniz copy, preserve their exact digests, and update `PROVENANCE.md` and `THIRD-PARTY-NOTICES.md`.
- **D-06:** Keep generated PBP bytes run-local. Derive them from owned source media and record input/tool/output identities. Before any long test or guest run, check fixture size and digest, directory/TOC bounds and ordering, track counts, compressed extents, requested versus decoded sector counts, and first/final boundary sectors. Compare supported sectors and TOC against the owned source media before guest execution.

### CI, quality gates, and iteration speed
- **D-07:** Fold TODOs 006 and 051 into this phase. Extend the existing vendored-file digest self-test and route Nightly PR jobs conservatively. Leave TODO 002 pending because its recorded first link/export trigger was already crossed in Phase 4; leave TODO 005 pending because the four pinned miniz files checked for this phase contain no SPDX identifiers and the owner chose to defer the generic scanner. Preserve the miniz license notice and archive checks required for this dependency.
- **D-08:** Keep the repository-check gate under the existing 3,901-line ceiling. No increase is authorized. Planning must account for all new counted helpers and negative controls, reuse existing gates where they cover the same fault, and preserve every requirement-named assertion. If the full folded scope cannot fit without moving/minifying gate logic or weakening checks, surface the concrete budget conflict before implementation instead of silently dropping acceptance.
- **D-09:** For Nightly pull requests, skip only the four benchmark jobs and `vectors-full` when a conservative classifier proves the change is PBP-only. Keep relevant fuzz campaigns. Unknown paths, shared source/tests, build or workflow changes, and routing changes run every Nightly job. Scheduled and manual runs retain the full existing workload. Confirm routing with `act --dryrun` and a hosted changed-head run; record raw runner time separately from wall-clock time and do not call raw runner seconds billing cost.
- **D-10:** Run cheap, relevant consistency checks before broad suites or hosted CI: sorted inventories, workflow selectors and job wiring, fixture identity and extents, and the current gate total. Reuse checks that already catch recurring faults; add a permanent preflight only when it prevents a named recurring fault. For each CI run, inspect critical-path time, duplicate work, iteration delay, and Actions cost while it is running and after completion. Preserve all required platforms, cases, assertions, and quality bounds.
- **D-11:** Preserve the separate required Main and Release checks and their full matrix. The Phase 9 evidence and TODO 050 prohibit assuming that one workflow's green result substitutes for the other or canceling/reusing work without a tested producer/consumer design. Do not reopen TODO 050 as part of Nightly routing.

### the agent's Discretion
- Choose exact fixture-proven PBP track variants, parser helper boundaries, fixed memory/work bounds, selectors, and plan waves after checking the primary format notes and current host implementation.
- Keep test code in the established disc, runner, and libretro seams; register every disc case in the sorted inventory and run it in the existing applicable native, sanitizer, wasm32, and big-endian lanes.
- Use one hosted changed-head CI run for coherent routing and code changes where practical, after the cheapest local preflights and any `act` check that adds coverage. Adjust provisional executor-chosen timeouts from measured workload only; preserve every case and explicit limit.

### Deferred Ideas (OUT OF SCOPE)
- Encrypted PGD, multi-disc switching, audio compression, and undocumented PSISO layouts stay unsupported.
- A generic `pins.txt` schema-gate rebind is deferred because its recorded first-row trigger was already crossed; handle it only with new scope evidence.
- Any broader Main/Release artifact handoff stays deferred pending a tested producer/consumer design and measured benefit.
- ECM/CloneCD, SBI/LibCrypt, profiles, peripherals, patch/cheat support, and milestone-wide DAT claims remain in their mapped later phases.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| PBP-01 | The host reads the explicitly tested unencrypted PSISO PBP layout and supported track variants with bounded directory/TOC parsing and deterministic sectors; encrypted PGD and undocumented layouts are rejected with a clear unsupported result. The inflater is in-tree permissive source with its notice and does not require system zlib. | The pinned PSX-SPX description supplies reverse-engineered header, TOC, and block observations; miniz v3.1.2 supplies a verified MIT-licensed raw inflater. Existing host dispatcher, media limits, CTest inventories, fixture producer, release notices, and optional CHD build are the integration seams. Fixtures must settle all accepted offsets, track mappings, parity, and malformed/truncated behavior. |
</phase_requirements>

## Project Constraints (from AGENTS.md)

- Reuse permissively licensed material only; verify the public commit and retain the upstream notice. Do not open the prohibited emulator source trees named in `AGENTS.md`.
- Keep file access, image parsing, and decompression in the host provider. The core receives the existing `schway_disc` callbacks and never reads files or environment state.
- Generated disc/PBP media stays in build storage; no game media bytes enter the repository. Keep the fixture bounded and record source/tool/output identity.
- Keep outputs deterministic across applicable platforms and test every behavior change through the existing CI preset.
- Repository checks are counted by the existing line-budget gate. Keep the complete shipping helper set at or below 3,901 lines; do not relocate or minify checks or remove assertions to make room.
- Run cheap fixture, selector, inventory, workflow, and budget checks before long suites/hosted CI. Use `act --dryrun` for routing and local `act` jobs only when they add coverage.
- Preserve all required platforms and Main/Release gates; record each hosted run's URL, exit status, critical path, and measured cost signals.
- Work on `phase/10-pbp-disc-images`, record runtime/model at each phase step, keep plans and tasks sized to the project’s 30-minute planning limit, and complete review before final verification.

## Summary

**Primary recommendation:** Add a dedicated host-side PBP provider and use the upstream standalone miniz v3.1.2 inflater at public commit `77d0dce8627735138c51770d1799a1ef48f2117d`; accept only the planned owned single-data-track PSISO subset after sector/TOC parity passes. Validate all indexed streams during open before publishing a reader, with one reusable `0x9300`-byte decoded buffer and checked logical/count limits (Plan 10-06). PBP format claims remain medium confidence because the reference is reverse-engineered; the later fixture checks establish behavior, not normative format coverage.

**Measured gate budget is resolved for the planned scope:** the current set is 3,901/3,901. Only TODOs 006 and 051 are folded. The preserving driver consolidation plus digest/routing/wiring additions measured in `10-GATE-BUDGET.md` project 3,899/3,901, including the obsolete Nightly path-filter assertion and PBP metadata-test mapping. The scratch quality, triage and routing controls passed; execution must reproduce the complete count and controls. No cap increase, assertion removal, relocation or generic SPDX scanner is authorized.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| PBP directory, TOC, extent, and sector parsing | Host media provider | Runner/libretro adapters | Paths and hostile bytes belong to the host; both frontends already consume the shared disc contract. |
| Raw DEFLATE | Host media provider | Build system | The inflater is a private static C source; the core and host platform should not need system zlib. |
| Generated PBP fixture and parity proof | Test/fixture producer | Existing CTest lanes | Build-local media can be regenerated from owned CUE/BIN sources and compared before any guest run. |
| PR-only Nightly routing | GitHub workflow | Fast routing self-test | A conservative classifier can avoid unrelated PR jobs without changing scheduled/manual coverage. |

## Standard Stack

### Core

| Component | Version / Pin | Purpose | Why this choice |
|-----------|---------------|---------|----------------|
| Standalone miniz `tinfl` | v3.1.2, `richgel999/miniz` commit `77d0dce8627735138c51770d1799a1ef48f2117d` | Inflate raw DEFLATE PBP blocks into bounded memory. | The project’s stack research selects in-tree miniz so PBP remains available with CHD disabled and without platform zlib. Use the modular inflater files under the root MIT grant; exclude `miniz.h`, whose stale Unlicense wording conflicts with the pin's MIT licensing record. |
| Existing `schway_disc_image` host interface | repository current head | Route PBP into runner and libretro without a core/API change. | `host/disc_image.h` is explicitly documented as the shared host-only provider; current dispatcher rejects `.pbp` and returns a new output only on a successful provider open. |

Standalone miniz files at that commit and SHA-256 values obtained from the GitHub API contents:

| File | SHA-256 |
|------|---------|
| `miniz_tinfl.c` | `2296ebd21ef9af5ebbefd5b454d4bb67d8fb4af2d24945e7fb32114374f75a76` |
| `miniz_tinfl.h` | `da4850920fdf09f8877d9affabe1ebba1852ce4b1ebbd121f2ba7a7ea8e57e5d` |
| `miniz_common.h` | `f1ba29821c8caef83585b328196aa346c5da7c82890a70a3d10d77c6cec0ed39` |
`LICENSE` must also be retained verbatim. It names RAD Game Tools and Valve Software (2013–2014) and Rich Geldreich and Tenacious Software LLC (2010–2014), and grants use, modification, distribution, sublicensing, and sale subject to retaining the copyright and permission notice. The selected standalone modular files are covered by this MIT grant; omit upstream `miniz.h`, which contains stale public-domain/Unlicense wording and is unnecessary for the low-level inflater. Provide a project-owned `miniz.h` shim and generate `miniz_export.h` through CMake's `GenerateExportHeader`. This source is not the build-tree miniz copy under `pins/libchdr/`, whose checked project provenance identifies a separate Unlicense/public-domain grant. Keep the two provenance records distinct.

**Raw inflater API:** upstream `miniz_tinfl.h` documents `TINFL_FLAG_PARSE_ZLIB_HEADER` as the flag for zlib-wrapped input; with that flag absent, the input is raw DEFLATE. `TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF` is the mode for a known, complete output buffer. Use a freshly initialized decompressor per independent block, cap the output at exactly the expected block size, and reject incomplete input, incomplete output, unexpected extra output, or nonterminal inflater status. The source also defines global `tinfl_*` symbols that collide with libchdr's separate miniz object when both features are linked. Apply compile-time aliases to the standalone object and matching caller declarations without editing upstream bytes; define `MINIZ_NO_MALLOC` and rename any remaining global inflater helpers. Test both CHD-OFF and CHD-ON link graphs.

### Verified standalone symbol closure

At the pinned source, `MINIZ_NO_MALLOC` still emits the following four global functions. Apply all four
compile definitions to the standalone source **and its caller declarations**; keep upstream files byte-exact.

| Upstream symbol (verbatim) | Private alias | Pinned source line |
|---|---|---:|
| `tinfl_decompress` | `schway__pbp_tinfl_decompress` | `miniz_tinfl.c:187` |
| `tinfl_decompress_mem_to_heap` | `schway__pbp_tinfl_decompress_mem_to_heap` | `miniz_tinfl.c:682` |
| `tinfl_decompress_mem_to_mem` | `schway__pbp_tinfl_decompress_mem_to_mem` | `miniz_tinfl.c:720` |
| `tinfl_decompress_mem_to_callback` | `schway__pbp_tinfl_decompress_mem_to_callback` | `miniz_tinfl.c:729` |

[VERIFIED: pinned `miniz_tinfl.c:177-187,682-778`, `miniz_tinfl.h:32-53,101-105`, source hash above; native object inspection 2026-11-21.]
The high-level helpers remain emitted; the no-malloc option does not remove them. `tinfl_decompressor_alloc`
and `tinfl_decompressor_free` at lines 760/768 are excluded by `#ifndef MINIZ_NO_MALLOC` at line 759.
`tinfl_clear_tree` at line 177 is `static`; `tinfl_init` is a header macro. `miniz_def_*` are declarations in
`miniz_common.h:80-82`, with no definitions or references in this selected no-malloc object.

The one-off compile used the three hash-verified upstream files and a private shim selecting
`MINIZ_NO_MALLOC`, `MINIZ_NO_STDIO`, `MINIZ_NO_TIME`, and portable zero values for
`MINIZ_HAS_64BIT_REGISTERS`, `MINIZ_USE_UNALIGNED_LOADS_AND_STORES`, and `MINIZ_LITTLE_ENDIAN`.
`nm -gjU` reported exactly the four upstream globals in the plain object and exactly the four aliases in
the prefixed object (Mach-O adds a leading underscore). `nm -u` showed only platform stack/memory helpers;
no allocator, zlib or unprefixed inflater reference remained. A relocatable link of plain plus prefixed objects
exited 0, demonstrating that the alias set removes duplicate definitions. This is an isolated symbol probe;
Plan 10-03 still verifies the actual CHD OFF/ON host link graphs.

Reproducible object commands, with the isolated verified source/shim under `build/planning/pbp-alias-proof`:

- `cc -std=c17 -O2 -DNDEBUG -Ibuild/planning/pbp-alias-proof -c build/planning/pbp-alias-proof/miniz_tinfl.c -o build/planning/pbp-alias-proof/plain.o` — exit 0.
- `cc -std=c17 -O2 -DNDEBUG -Ibuild/planning/pbp-alias-proof -Dtinfl_decompress=schway__pbp_tinfl_decompress -Dtinfl_decompress_mem_to_heap=schway__pbp_tinfl_decompress_mem_to_heap -Dtinfl_decompress_mem_to_mem=schway__pbp_tinfl_decompress_mem_to_mem -Dtinfl_decompress_mem_to_callback=schway__pbp_tinfl_decompress_mem_to_callback -c build/planning/pbp-alias-proof/miniz_tinfl.c -o build/planning/pbp-alias-proof/prefixed.o` — exit 0.
- `nm -gjU build/planning/pbp-alias-proof/plain.o` and `nm -gjU build/planning/pbp-alias-proof/prefixed.o` — exit 0, four globals each as listed above.
- `nm -u build/planning/pbp-alias-proof/prefixed.o` — exit 0, no allocator/zlib/unprefixed inflater reference.
- `cc -r build/planning/pbp-alias-proof/plain.o build/planning/pbp-alias-proof/prefixed.o -o build/planning/pbp-alias-proof/together.o` — exit 0; linker emitted an SDK text-stub warning, not a duplicate-symbol failure.

These are untracked planning-probe files, not an execution dependency or new repository gate. Execution uses
the alias table and CMake-generated export header specified in Plans 10-02/10-03. No hosted run URL applies.

### Supporting

| Component | Version | Purpose | When to use |
|-----------|---------|---------|-------------|
| `host/media_limits.h` | repository current head | Checked 64-bit additions/multiplications and image-size cap. | Apply to every PBP offset, table extent, block extent, and count conversion. |
| `cmake/DiscFixtures.cmake` | repository current head | Build-local owned source media. | Produce a PBP fixture from the existing owned track source; do not commit the generated PBP. |
| Existing CTest and CMake inventories | repository current head | Register provider, runner, libretro, fuzz, and fixture checks. | Extend the current inventories and consumers rather than inventing a parallel test path. |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Standalone miniz `tinfl` | System zlib | Rejected by locked decision: system zlib creates a platform link/install requirement and would make builds differ. |
| Host PBP provider | Add parsing/decompression to the core | Rejected: paths and hostile bytes must stay host-side; this would violate the public core boundary. |
| Full format permissiveness | Accept audio/multi-disc/unknown TOC variants | Rejected: PSX-SPX explicitly calls those layouts unknown; no owned fixture can currently justify their sector mapping. |

## Architecture Patterns

### Data flow

```text
PBP path → image dispatcher → host PBP parser → checked PSISO directory/TOC/table
                                      ↓
                                miniz raw inflater
                                      ↓
                           bounded sector read cache
                                      ↓
                    existing schway_disc callback → runner/libretro → core
```

### Recommended project structure

Follow `host/disc_chd.c` and `host/disc_chd.h` for the separate provider boundary. Wire the provider through the existing image dispatcher and target source lists. Add fixture creation beside `cmake/DiscFixtures.cmake`; use a local-only generator or an existing approved tool route only after its full source/license closure is checked. Add parser/replay cases to `tests/disc/image.c` and sorted `tests/disc/inventory.txt`, then cover the existing runner and libretro consumers. Do not create persistent generated game bytes.

### Bounds and failure pattern

Use `host/media_limits.h` checked arithmetic before narrowing. Its source states `#define SCHWAY__MEDIA_IMAGE_MAX_BYTES UINT64_C(1073741824)` and declares `static inline int schway__media_add_u64(...)` / `static inline int schway__media_mul_u64(...)` (`host/media_limits.h:7-22`). For each table or compressed block, prove `offset + length` is within the PSAR/file extent; check count-times-entry-size before iteration; bound the decompressed block to `0x9300` bytes from the pinned format description. Preserve failure-atomic open: the existing dispatcher clears the output initially, destroys a partial image on any backend error, and assigns it only after success (`host/disc_image.c:1263-1333`).

### Raw block decode pattern

Initialize the miniz inflater for one block, pass the complete compressed extent, use raw mode (do not set the zlib-header flag), cap the destination at `0x9300`, and require exact output and input consumption plus terminal success. Plan 10-06 resolves the lifecycle choice: validate every indexed stream once during open before publishing the handle, then retain at most one decoded-block cache. Bound physical input and logical decoded size by the existing 1 GiB limit, derive the checked block count/table limits from that size and the 16-sector block unit, and permit only the fixture-proven final-block padding. Reuse one decoded buffer; do not materialize the whole image or perform unbounded retries.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|------------|-------------|-----|
| Deflate decoding | New Huffman/DEFLATE implementation or platform-specific zlib linking | Pinned `tinfl` from miniz v3.1.2 | The repo already selected an in-tree permissive inflater; the decoder has bitstream/status edge cases. |
| Overflow/bounds checks | Per-parser ad hoc arithmetic | Existing shared media limit helpers | They already encode overflow-safe size arithmetic and the 1 GiB image cap. |
| Disc API | PBP-specific callback contract | Existing `schway_disc` interface | Runner, libretro and core already share this boundary. |
| Notice or digest system | New source/license or vendored-file gate | Existing notices and `libretro.vendored` gates | Phase 9 already has manifest notices and linked-provider `.info` checks; Phase 10 should extend those mechanisms. |

## Common Pitfalls

### PBP layout claims are not normative
**What goes wrong:** Accepting a TOC variant or audio/multi-disc mapping based on a sample from another PBP.
**Why:** The project-pinned PSX-SPX page says its PBP structure was reverse-engineered from homebrew DBALL.PBP and shows materially different TOC layouts in other samples.
**How to avoid:** Accept only layouts produced from owned source media whose TOC and every raw sector match; fail unknown layouts as unsupported.
**Warning signs:** Supporting a TOC pattern without a generator/fixture or allowing fallback interpretation after an unknown marker.

### Raw DEFLATE and size table mismatch
**What goes wrong:** Passing zlib mode, trusting compressed-size fields, or accepting a short/long decoded block.
**Why:** The PBP reference describes raw DEFLATE and 16-sector blocks that decode to 0x9300 bytes; miniz can return more output than the caller expects on corrupt data.
**How to avoid:** Use raw mode, validate each compressed span against the file and neighboring table entries, and require exact output size/status.
**Warning signs:** Output size is inferred from compressed bytes, the final block is treated the same as a full block without fixture evidence, or the input stream is accepted before reaching completion.

### TOC order and track encoding
**What goes wrong:** Treating every 10-byte TOC record as a canonical Q record or trusting record order.
**Why:** The reverse-engineered reference shows both CD-ROM and audio records and samples with unusual field placement/order; several records are explicitly uncertain.
**How to avoid:** Validate known A0/A1/A2 plus track records and BCD/MSF ranges only for fixture-proven variants; compare the normalized shared TOC to source CUE.
**Warning signs:** Unused zero entries are not checked, track count disagrees with the source, or data/audio mode is inferred from one byte without cross-checks.

### Expensive prevalidation and gate budget
**What goes wrong:** A complete-image inflate pass becomes unexpectedly slow, or new safety checks exceed the current budget and are then silently cut.
**Why:** The media cap allows up to 1 GiB of input, and the counted repository-check set is already at its maximum 3,901 lines.
**How to avoid:** Measure open-time validation on the generated fixture and the largest structurally valid test image; keep temporary memory at one block. Compute the gate total before adding helpers, reuse/consolidate only where the exact fault coverage remains, and stop planning if no evidence-backed reduction can preserve all assertions.
**Warning signs:** Guest execution starts before a required preflight, the count is checked only on changed files, a new helper sits outside the counted glob, or a limit is increased to make the plan pass.

## Code Examples

Verified upstream API facts: the raw-mode bit is selected by leaving `TINFL_FLAG_PARSE_ZLIB_HEADER` unset; `TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF` indicates the output buffer can hold the full decoded stream. See the [pinned miniz 3.1.2 inflater header](https://github.com/richgel999/miniz/blob/77d0dce8627735138c51770d1799a1ef48f2117d/miniz_tinfl.h) and [implementation](https://github.com/richgel999/miniz/blob/77d0dce8627735138c51770d1799a1ef48f2117d/miniz_tinfl.c). The final project call should be written and compile-tested with the exact decoded-size guard; no code was copied into the research artifact.

## State of the Art

The relevant upstream pin is miniz 3.1.2, released on 2026-07-01. Its release notes mention fixes to ZIP parsing and the `tinfl_decompress` zero-code-length loop; these are reasons to keep an exact source pin and not substitute a stale unrelated miniz copy. The repo’s older libchdr build-tree source is not the source to copy because its license basis is recorded separately.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | RESOLVED: Plan 10-04 selects an owned first-data-track CUE and a local Python fixture writer with deterministic encoding and an independent standard-library decode check. | Architecture Patterns | Generation, exact source/output counts and repeat-byte identity remain required implementation acceptance tests. |
| A2 | RESOLVED: Plan 10-06 chooses one full indexed-stream pass at open, one reusable `0x9300` output buffer, and checked 1 GiB logical/count limits. | Raw block decode pattern | Fixture latency is measured during execution; no alternative lifecycle choice remains open. |
| A3 | RESOLVED: the three verified files compile with a small private shim/export macro; the exact four aliases were confirmed by native object inspection and a combined relocatable link. | Standard Stack | Actual CMake export generation and CHD ON/OFF host links remain acceptance checks in 10-02/10-03, not unresolved design questions. |

## Open Questions (RESOLVED)

1. **Q1 — exact supported fixture variant — RESOLVED:** Plan 10-04 selects the owned disc_boot first MODE2/2352 data track and its equivalent single-track CUE, with deterministic raw/compressed PSISO fixture bytes. Plans 10-05/10-06 require exact TOC and all-sector parity before accepting that layout. CDDA/audio compression, PGD, multi-disc and unknown TOCs remain unsupported; no claim is made before those acceptance checks pass.
2. **Q2 — compressed corruption boundary — RESOLVED:** Plan 10-06 validates every indexed block during open before publishing the reader. Checked logical/count/table limits bound a single pass and one reusable `0x9300`-byte decoded buffer; exact stream completion/size and final-padding tests enforce the selected behavior before guests.
3. **Q3 — fixed gate budget fit — RESOLVED:** `10-GATE-BUDGET.md` records the measured preserving reductions and all digest/routing/wiring/metadata additions: 3,899/3,901 projected lines. The complete controls and count are required before/after implementation. Only TODOs 006/051 are folded; the generic SPDX scanner stays deferred and the ceiling is unchanged.
4. **Q4 — complete standalone symbol aliases — RESOLVED:** The pinned no-malloc object emits exactly `tinfl_decompress`, `tinfl_decompress_mem_to_heap`, `tinfl_decompress_mem_to_mem`, and `tinfl_decompress_mem_to_callback`. Alias them respectively to `schway__pbp_tinfl_decompress`, `schway__pbp_tinfl_decompress_mem_to_heap`, `schway__pbp_tinfl_decompress_mem_to_mem`, and `schway__pbp_tinfl_decompress_mem_to_callback` in the standalone object and caller declarations. Alloc/free helpers are preprocessed out; the other three high-level helpers remain emitted. The source/hash/object/link evidence is in the verified symbol-closure section above; Plans 10-02/10-03 apply and test this fixed set.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake | Build and test wiring | ✓ | 4.4.3 | — |
| C compiler | Host provider and test executables | ✓ | Apple clang 21.0.1 | — |
| `gh` | Public pin lookup and hosted CI evidence | ✓ | 2.101.0 | GitHub API over HTTPS |
| `act` | Routing dry-run and compatible local workflow jobs | ✓ | 0.2.87 | `act --dryrun` remains available if a job cannot run locally |
| Docker | `act` host | ✓ | 29.5.2 | — |

No package manager dependency is required for the inflater; no system zlib installation is permitted by the locked design.

## Validation Architecture

Omitted: `.planning/config.json` explicitly sets `workflow.nyquist_validation` to `false`. Existing project and phase requirements still require behavior tests through the existing CI preset.

## Security Domain

`.planning/config.json` explicitly sets `workflow.security_enforcement` to `false`; no ASVS checklist applies. The relevant media-security controls are already locked: checked arithmetic, bounded file/table/output lengths, fail-atomic open, fuzz/replay coverage, and rejection of encrypted/unknown layouts.

## Sources

### Primary / repository-pinned
- [PSX-SPX Disk Images, PBP section](https://psx-spx.consoledev.net/dev/ps1/cdr/cdromfileformats/diskimages/) — PBP offsets, PSISO sample, TOC records, sector block sizes, raw DEFLATE; explicitly reverse-engineered from homebrew, MEDIUM confidence.
- [miniz release 3.1.2](https://github.com/richgel999/miniz/releases/tag/3.1.2) and exact [commit `77d0dce8627735138c51770d1799a1ef48f2117d`](https://github.com/richgel999/miniz/commit/77d0dce8627735138c51770d1799a1ef48f2117d) — upstream release and public immutable pin, HIGH confidence.
- [Pinned miniz `LICENSE`](https://github.com/richgel999/miniz/blob/77d0dce8627735138c51770d1799a1ef48f2117d/LICENSE), [`miniz_tinfl.h`](https://github.com/richgel999/miniz/blob/77d0dce8627735138c51770d1799a1ef48f2117d/miniz_tinfl.h), and [`miniz_tinfl.c`](https://github.com/richgel999/miniz/blob/77d0dce8627735138c51770d1799a1ef48f2117d/miniz_tinfl.c) — MIT terms, raw-mode contract, status API, and inflater implementation, HIGH confidence.
- `.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md` §3 — project-pinned PBP facts and explicit confidence boundary.
- `.planning/research/STACK.md` — recommends standalone miniz for PBP and distinguishes it from libchdr’s build-tree copy.
- `PROVENANCE.md:58,208` and `THIRD-PARTY-NOTICES.md:791-795` — libchdr source closure and its separately licensed miniz copy.

### Existing implementation sources
- `host/disc_image.h:11-22` — exact existing error values include `SCHWAY_DISC_IMAGE_BAD_IMAGE = 4,` and `SCHWAY_DISC_IMAGE_UNSUPPORTED = 7,`.
- `host/media_limits.h:7-22` — exact shared 1 GiB image cap and checked add/multiply helpers.
- `host/disc_image.c:1263-1333` — failure-atomic dispatcher and current suffix/provider behavior.
- `cmake/DiscFixtures.cmake`, `cmake/CHDFixtures.cmake`, `host/disc_chd.c`, `tests/disc/image.c`, `tests/disc/inventory.txt`, `tests/runner/tests.cmake`, `tests/libretro/tests.cmake`, `.github/workflows/nightly.yml`, `tests/cmake/vendored.cmake`, and `tests/cmake/notices.cmake` — current implementation/test/workflow seams.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — exact upstream pin/hashes, MIT licensing, modular shim compilation and complete private symbol aliases checked; CMake export generation is assigned to 10-02.
- Architecture: HIGH — existing host dispatcher, media cap, CMake seam, and consumer call paths were inspected.
- PBP format: MEDIUM — one primary community reference, marked reverse-engineered; fixture parity must settle supported variants.
- Gate-budget fit: HIGH — exact preserving recipe projects 3,899/3,901 including routing/digest/wiring/metadata changes; execution must reproduce its counts and retained controls.

**Research date:** 2026-11-21
**Valid until:** Stable format/source pin; recheck only if the accepted PBP subset, upstream source, or gate inventory changes.
