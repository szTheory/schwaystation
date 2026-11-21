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
- **D-05:** Verify the exact standalone miniz source revision, file set, license grant, notice text, and per-file digest before copying it into the repository. Keep this source and its MIT notice distinct from libchdr's build-tree miniz copy. Pin the source to a public commit; preserve notices in every archive that links it and update `PROVENANCE.md` and `THIRD-PARTY-NOTICES.md`.
- **D-06:** Keep generated PBP bytes run-local. Derive them from owned source media and record input/tool/output identities. Before any long test or guest run, check fixture size and digest, directory/TOC bounds and ordering, track counts, compressed extents, requested versus decoded sector counts, and first/final boundary sectors. Compare supported sectors and TOC against the owned source media before guest execution.

### CI, quality gates, and iteration speed
- **D-07:** Fold TODOs 002, 005, 006, and 051 into this phase. Add the release-floor check because this phase first adds a statically linked PBP inflater to the packaged libretro core; implement the deferred tracked-source SPDX scan with the first tracked SPDX-bearing source; extend the existing vendored-file digest self-test for the exact new source; and route Nightly PR jobs conservatively. Update each todo's status/binding and correct stale phase text as appropriate.
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

**Primary recommendation:** Add a dedicated host-side PBP provider and use the upstream standalone miniz v3.1.2 inflater at public commit `77d0dce8627735138c51770d1799a1ef48f2117d`; accept only fixture-proven unencrypted PSISO layouts, validate all arithmetic/extents before use, and prove sector/TOC parity to owned source media. PBP format claims are medium confidence because the reference is reverse-engineered from homebrew. Keep full-block validation bounded to one `0x9300` byte output buffer and settle whether open-time validation is necessary for the chosen failure boundary before implementing it.

**Measured gate budget is a planning constraint:** the current counted set is exactly 3,901/3,901 lines. The original deferred release-floor seed is 134 lines and the original notices seed is 57 lines, before adapting and adding negative controls. The existing vendored check is only 31 lines and can be extended, but that does not create room for the other gates. Do not copy these seeds unchanged or raise the cap. The planner must identify a concrete, assertion-preserving consolidation supported by current code evidence; otherwise keep the binding gates as an explicit unresolved budget conflict rather than dropping coverage.

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
| Standalone miniz `tinfl` | v3.1.2, `richgel999/miniz` commit `77d0dce8627735138c51770d1799a1ef48f2117d` | Inflate raw DEFLATE PBP blocks into bounded memory. | The project’s stack research selects in-tree miniz so PBP remains available with CHD disabled and without platform zlib. The upstream tag’s `LICENSE` and the checked C/H file headers carry the MIT grant. |
| Existing `schway_disc_image` host interface | repository current head | Route PBP into runner and libretro without a core/API change. | `host/disc_image.h` is explicitly documented as the shared host-only provider; current dispatcher rejects `.pbp` and returns a new output only on a successful provider open. |

Standalone miniz files at that commit and SHA-256 values obtained from the GitHub API contents:

| File | SHA-256 |
|------|---------|
| `miniz_tinfl.c` | `2296ebd21ef9af5ebbefd5b454d4bb67d8fb4af2d24945e7fb32114374f75a76` |
| `miniz_tinfl.h` | `da4850920fdf09f8877d9affabe1ebba1852ce4b1ebbd121f2ba7a7ea8e57e5d` |
| `miniz_common.h` | `f1ba29821c8caef83585b328196aa346c5da7c82890a70a3d10d77c6cec0ed39` |
| `miniz.h` | `f0b67217bdd8e45b74eb9b7c2c7ae20ed6760a1d76f3276b93617039ebd082df` |

`LICENSE` must also be retained verbatim. It names RAD Game Tools and Valve Software (2013–2014) and Rich Geldreich and Tenacious Software LLC (2010–2014), and grants use, modification, distribution, sublicensing, and sale subject to retaining the copyright and permission notice. The standalone release is MIT; it is not the build-tree miniz copy under `pins/libchdr/`, whose checked project provenance identifies an Unlicense/public-domain grant. Keep the two provenance records distinct.

**Raw inflater API:** upstream `miniz_tinfl.h` documents `TINFL_FLAG_PARSE_ZLIB_HEADER` as the flag for zlib-wrapped input; with that flag absent, the input is raw DEFLATE. `TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF` is the mode for a known, complete output buffer. Use a freshly initialized decompressor per independent block, cap the output at exactly the expected block size, and reject incomplete input, incomplete output, unexpected extra output, or nonterminal inflater status. This API shape is verified in the pinned upstream source; the project-specific call must still be compile-tested.

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

Initialize the miniz inflater for one block, pass the complete compressed extent, use raw mode (do not set the zlib-header flag), cap the destination at `0x9300`, and require the decompressor to report completion with exactly the expected output and a bounded/acceptable input-consumption result. The source documentation warns that corrupted streams can yield more output than expected; stop immediately when the output limit is reached. Whether the parser validates every block during open or each block before exposing its sectors is still an implementation decision: early validation detects bad blocks before guest execution but adds one full decompression pass at open, while lazy validation is cheaper at open and still must fail at the media callback before returning invalid sector bytes. Plan tests must pin the selected behavior and bound the work.

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
| A1 | A run-local fixture can be generated from the existing owned CUE/BIN source without adding an uncleared tool dependency. | Architecture Patterns | Source/tool closure could block fixture production; planner must inspect the fixture route before assigning implementation. |
| A2 | Every accepted PSISO layout can validate all compressed blocks with one `0x9300` scratch buffer and a practical bounded open-time cost. | Raw block decode pattern | It may increase image-open latency or require a different lifecycle boundary; measure before accepting the tradeoff. |
| A3 | Only the four hashed miniz files plus upstream MIT `LICENSE` are needed after accounting for `miniz_export.h` generation and feature macros. | Standard Stack | The header includes `miniz_export.h`; confirm whether upstream CMake generates it and exactly what minimal configuration is required before source admission. |

## Open Questions

1. **Which exact PBP variants can the owned fixture producer generate?** The public reference has one CD-ROM data-track example but additional audio layouts remain unclear. The planner should use source media with known tracks and require exact TOC/sector parity; do not infer support from parser acceptance.
2. **What failure boundary is required for truncated blocks?** Full block validation at open ensures no compressed corruption remains for guest execution but scans the image once; per-block validation at sector callback avoids a full open pass but detects corruption only when that block is requested. Choose from requirement wording and measured bounded workload, then test the chosen behavior.
3. **Can all folded repository gates fit under 3,901 lines?** Current count is exactly 3,901. The original `floor.cmake` seed is 134 lines; `notices.cmake` seed is 57 lines. This establishes a concrete planning risk, not permission to delete assertions or raise the ceiling. The planner should show an assertion-preserving merge/reduction with exact line accounting or explicitly stop at the scope conflict.
4. **How is `miniz_export.h` produced?** The pinned miniz headers include it but it was not among the repository tree entries checked. Confirm upstream CMake generation and whether the project needs a generated header or a tiny project-owned compatibility header before pin/copy tasks.

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
- Standard stack: HIGH — exact upstream pin, file hashes, and MIT license checked via GitHub API; generated export header detail remains open.
- Architecture: HIGH — existing host dispatcher, media cap, CMake seam, and consumer call paths were inspected.
- PBP format: MEDIUM — one primary community reference, marked reverse-engineered; fixture parity must settle supported variants.
- Gate-budget fit: HIGH — current count and seed line counts were measured; whether the full fold can fit remains unresolved.

**Research date:** 2026-11-21
**Valid until:** Stable format/source pin; recheck only if the accepted PBP subset, upstream source, or gate inventory changes.
