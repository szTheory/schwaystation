# Phase 9: CHD disc images - Research

**Researched:** 2026-11-19
**Domain:** Host-side CHD decoding, bounded disc-image parsing, cross-platform parity, CI/release packaging
**Confidence:** HIGH for repository contracts and pinned reader guidance; MEDIUM for generator choice, which remains unresolved

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** Project CHD metadata onto the existing `schway_disc` contract: raw 2352-byte sectors, 96-byte subcode when available, and the existing two-index TOC fields. Map `index0` to pregap start, `index1` to program start, and leadout to the exclusive logical end; CHD alignment padding is not a logical LBA.
- **D-02:** Use validated metadata to distinguish stored and virtual pregaps. Preserve existing zero-subcode behavior when a track has no subcode so the core can synthesize Q. Reject unsupported sector modes, subcode encodings, nonzero postgaps, and multisession layouts until exact behavior is covered.
- **D-03:** Keep all parsing and libchdr calls host-side. Build the BSD-3-Clause libchdr closure statically behind `SCHWAYSTATION_CHD`; default remains OFF. Preserve `UNSUPPORTED` for CHD when disabled and make failed opens leave the active disc unchanged.
- **D-04:** Generate a run-local CHD from the existing owned CUE/BIN fixture. Before selecting a generator, audit the exact pinned source and compiled dependency closure for permissive licensing; the MAME `chdman` entry files have BSD-3-Clause headers but its complete closure is not yet cleared. Do not commit or fetch a prebuilt opaque CHD. Keep the generated CHD under the 512 KiB fixture cap and distribute identical bytes to all applicable hash lanes.
- **D-05:** Before any long guest run, cheaply check fixture size and hashes, bounded header/count/offset arithmetic, companion path confinement, modes, indexes, pregaps, leadout, extent counts, and boundary-sector reads through the final LBA. Then compare CHD and uncompressed identity, TOC, raw sectors, applicable subcode, and hashes on all eight applicable lanes.
- **D-06:** Add malformed-image regression and fuzz coverage for shared media bounds. Invalid input must not partially replace the active disc. Keep the compatibility ledger preflight strict about missing/duplicate keys and protect prior passing rows with dated allowances only.
- **D-07:** After parity passes, enable CHD in distributed release artifacts while retaining the default-OFF CMake build and the current release asset names. Verify ON/OFF `.info` extensions and archive notices; the published feature claim must match the shipped binary.
- **D-08:** Resolve todo 050 with exact-SHA workflow/artifact identity checks and measured runner-minutes and critical path. Keep both required Main and Release checks, all matrix lanes, and package validation unless measured savings and tests prove equivalent artifact reuse safe. Inspect each CI run while active and after completion; try `act` only where local execution adds coverage beyond the existing suite.
- **D-09:** Before expensive build, suite, guest, or hosted CI work, run the cheapest relevant existing consistency checks first. Add permanent preflight code only for a recurring fault named by a requirement; preserve phase gate budgets and every acceptance case.

### the agent's Discretion
- Choose private helper boundaries, exact supported metadata variants, fuzz duration, test selectors, plan waves, and build-tool integration while preserving byte parity, bounded parsing, and the host/core boundary.
- Select the smallest fixture-generation route only after the complete pinned license/build closure and reproducibility are established. If no route is cleared, keep the blocker concrete and continue independent Phase 9 work.
- Apply evidence-backed CI efficiency fixes within this phase; do not trade away lanes, assertions, required status checks, archive validation, or quality bounds.

### Deferred Ideas (OUT OF SCOPE)
- PBP parsing remains Phase 10; its vendored digest todo remains bound there.
- ECM/CloneCD remain Phase 11; SBI/LibCrypt remain Phase 12.
- Broader CHD sector reconstruction, unsupported metadata modes, postgaps, and multisession support stay out until byte-exact fixtures and requirements cover them.
- Any cross-workflow artifact reuse is deferred unless Phase 9's exact-SHA, artifact identity, and timing evidence proves it safe and worthwhile.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| MEDIA-01 | Supported providers agree on disc identity, TOC, sectors, subcode and explicit errors; failed opens do not replace active media. | Normalize CHD into the existing host callback and test against CUE/BIN. |
| MEDIA-02 | Readers validate bounds and confined paths, with malformed-input regression/fuzz tests and a cheap pre-guest inventory check. | Reuse provider checks and add checked metadata/extents plus preflight before long emulation. |
| CHD-01 | Optional pinned libchdr v1–v5 subset passes matching eight-lane parity, malformed-input tests and archive notices; default remains OFF until parity. | Use static host-side libchdr, exact fixture identity, ON/OFF metadata checks and package validation. |
| COMPAT-02 | Feature ledger adds provider evidence, preserves prior passes and preflights keys/counts before broad suites. | Keep new feature rows in the protected ledger and run its cheap ratchet preflight first. |
</phase_requirements>

## Summary

The project already has a host-only `schway_disc_image` provider, a raw-sector callback and a two-index TOC. Implement CHD as another provider that normalizes into that contract; do not add CHD parsing or file access to the emulation core. The accepted libchdr source is pinned at `47d794151b2b` and is built statically only when `SCHWAYSTATION_CHD=ON`; the option stays OFF by default until deterministic parity passes. [VERIFIED: `.planning/research/STACK.md:9-11,19-32`; `.planning/preparation/DECISIONS.md` DEC.43]

Use metadata to distinguish stored from virtual pregaps, exclude padded CHD frames from logical LBAs, and reject unsupported representations. A run-local CHD must come from the owned CUE/BIN fixture and a pinned generator whose complete compiled license closure is verified first. Existing advisor research found candidate generator entry files but did not clear a complete build closure; that audit is a plan gate, not an assumption. Never commit the CHD. [VERIFIED: `.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md:195,204`; `.planning/preparation/DECISIONS.md` DEC.91; generator clearance unresolved]

Before guest execution, run cheap checks on input counts, hashes, cap, paths, offsets, track metadata and first/last sectors. Then compare identity, TOC, sectors, applicable subcode and hashes across all eight applicable lanes. For CI optimization, first measure total runner-minutes and critical path by exact SHA; retain Main, Release, all matrix lanes and archive checks unless tested artifact equivalence demonstrates a safe, worthwhile reduction. [VERIFIED: `.planning/AUTONOMOUS.md` CI/release section; todo 050]

**Primary recommendation:** Plan in short, dependency-aware slices: clear the fixture generator license/build closure and media test contract first; implement bounded shared media checks and the host CHD backend next; then parity, fuzz, compatibility ledger and ON/OFF release wiring. Keep every expensive guest and hosted run behind the low-cost preflight.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| CHD metadata/path validation and file access | Host media provider | CMake build | Disc files are host supplied; the core must not read files or environment. |
| CHD hunk/codec decoding | Pinned libchdr, host-side static library | Host media provider | Avoid a custom CHD decoder and keep its code outside the deterministic core. |
| TOC, sector and subcode delivery | Existing host disc callback | Emulation core consumes data | Keep one contract shared by runner and libretro. |
| Fixture creation and parity evidence | Build/test tooling | CI matrix | Generate identical ephemeral fixture bytes and compare on each applicable platform. |
| Feature claims and package notices | CMake/libretro metadata and release workflow | Notice gate | Advertise CHD only in enabled builds and ship the licenses for the exact closure. |

## Project Constraints (from AGENTS.md)

- Reuse only permissively licensed source; audit exact pins and preserve notices. Never open the forbidden emulator sources.
- Keep the three deliverables (library, CLI runner, libretro core); filesystem and CHD parsing stay in host code.
- Do not commit CHD/disc/BIOS/state bytes; generate the CHD in build storage, keep it under 512 KiB, and add no fixture outside the manifest.
- The core remains integer-only, deterministic, instance-owned and bounded; host supplies sectors and subcode.
- Every behavior change needs gating CTest coverage; maintainers' details belong in `docs/`, not a long README or public-header prose.
- Verification is automation-first. Run recurring integration checks in CI; record one-off checks as command, exit status and run URL.
- Keep repository gates within the named absolute phase budget and real RetroArch code below 300 lines. Review code before final verification.
- Never add attempts/quota/publication ceremony or an unrequired gate. Keep plans below the 30-minute split threshold.
- Work on `phase/09-chd-disc-images`; never commit on `main`. Every phase ends with ship, merged main CI and published release.
- For each CI/CD run inspect live and completed critical path, duplicate work, iteration delay and Actions cost. Run cheap relevant preflights first; use `act` only when it adds local coverage and keep hosted-only required checks.
- Preserve all required matrix lanes, cases, assertions, quality bounds and package checks. Measure improvements; do not infer speedup from a single job duration.

## Standard Stack

### Core

| Component | Pin / license | Purpose | Guidance |
|-----------|---------------|---------|----------|
| `libchdr` | v0.3.0, approved commit `47d794151b2b64f6aba60304e6480f4f5a2bc24b`; BSD-3-Clause root license | CHD v1–v5 header/metadata parsing, hunk decoding and CD codecs | Static host-side dependency behind `SCHWAYSTATION_CHD`; keep bundled C codecs, raw-sector/subcode output and block CRC checks. Do not enable system zlib/zstd or optional C++ micro-flac. |
| In-tree `schway_disc_image` | Project code | Bounds, path confinement, metadata normalization, callbacks, errors | Extend this provider, preserving failure-atomic open behavior. |

### Dependency/notice closure

The project stack names bundled LZMA SDK 26.02 (public domain), miniz 3.1.2 (MIT), Zstandard 1.5.7 (select BSD-3 terms, not GPL-2), and dr_flac (select public-domain grant). Preserve libchdr's root license, separate `chd.h` BSD-3 attribution, miniz MIT and Zstandard BSD-3 notices in enabled archives. Reconcile the older `PROVENANCE.md` note that calls miniz/Zstandard vendored-copy verification incomplete against `.planning/research/STACK.md` before closing todo 005. [VERIFIED: `.planning/research/STACK.md:19,26-30`; discrepancy in `PROVENANCE.md:58`]

### Fixture generator

**No generator is cleared yet.** The pinned libchdr is a reader and cannot produce the parity fixture. Parent advisor research identified pinned MAME `chdman` and `libchdman-rs` as candidates; neither is approved until its complete compiled source/dependency closure is checked against AGENTS.md and output reproducibility is proven. Do not use a prebuilt opaque CHD or commit generated bytes. `cargo` was not present in this machine's PATH when checked, so a Rust-based route would need an explicit reproducible toolchain setup. [VERIFIED: project source index and `PROVENANCE.md:58`; local `command -v cargo` returned no path; candidate clearance unresolved]

## Architecture Patterns

### Data flow

```text
CHD path
  -> host path confinement and checked header/metadata validation
  -> pinned libchdr file callbacks and bounded hunk reads
  -> logical track/index/pregap/leadout normalization
  -> existing schway_disc TOC + 2352-byte sector + 96-byte subcode callbacks
  -> runner or libretro supplies the same data to the deterministic core
```

The public contract has `SCHWAY_DISC_TRACK_AUDIO`, `SCHWAY_DISC_TRACK_MODE1`, `SCHWAY_DISC_TRACK_MODE2`, a 99-track cap, `index0/index1/pregap/leadout` zero-based fields and `read_sector(... uint8_t sector[2352], uint8_t subcode[96])`. [VERIFIED: `include/schwaystation.h:250-270`; quoted source values]

### Recommended implementation shape

- Validate header sizes, track counts, metadata lengths, frame counts, offsets, arithmetic and allocation ceilings before opening/reading sectors. Reuse canonical-path and companion-path checks from the existing CUE/M3U provider.
- Stage a candidate image completely before the caller swaps it in; the open function currently sets `*out = NULL` before parsing and frees a failed candidate. [VERIFIED: `host/disc_image.c:1233-1288`; quoted behavior]
- Interpret `index0/index1` and pregap from validated track metadata. Treat padded CHD frames as storage alignment, not guest LBAs. Map only byte-exact sector modes and subcode encodings; missing subcode yields the provider's zero-fill behavior.
- Run a size/count/path/layout/boundary/hash preflight before any guest boot. Compare first and last sectors of each track, including final logical LBA, before a long integration run.
- Add CHD to `.info` extensions only when enabled; verify the corresponding release archive contains the selected reader and its full notices.

### Anti-patterns to Avoid

- Treating every CHD frame as a logical sector, which shifts LBAs where track padding is present.
- Inferring indexes or rebuilding raw sectors/subcode beyond the current interface without byte-level fixture evidence.
- Trusting nested metadata counts or offsets before checked arithmetic and limits; allowing an image companion path to escape its selected directory.
- Partially replacing an active image before the new provider has validated successfully.
- Building against host-installed codecs or enabling C++ micro-flac, which weakens reproducibility and adds an unfixed dependency path.
- Booting a guest before source unit counts, fixture identity, boundary sectors and expected output paths are checked.
- Reusing Main artifacts in Release based only on one slow job; compare SHA and artifact digests and measure aggregate runner cost and critical path.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|------|
| CHD headers, hunk compression, CD codecs and CRCs | A custom CHD decoder or ad hoc codec path | Approved pinned libchdr C backend | Versioned format and codec logic is complex and hostile-input facing. |
| Guest-facing media interface | A second CHD-only callback or core file reader | Existing `schway_disc` and `schway_disc_image` boundary | Runner/libretro share one stable media contract; the core never owns file paths. |
| Fixture source | Opaque committed or downloaded CHD | Build-time CHD generated from owned CUE/BIN with audited toolchain | Keeps source and output provenance reviewable and CI bytes reproducible. |
| Build codec dependencies | Host discovery of zlib/zstd/FLAC | Pinned bundled permissive dependencies and notices | Avoids platform-dependent builds and licensing drift. |

## Common Pitfalls

### CHD padding and pregaps
**What goes wrong:** TOC and sector LBAs disagree with the uncompressed image. **Why:** CHD storage pads track extents and metadata distinguishes stored/virtual gaps. **Prevention:** derive logical ranges from validated metadata; compare each track's first, program-start, and last sectors against CUE/BIN before boot.

### Raw subcode layout
**What goes wrong:** Q/P or RW bytes differ despite matching data sectors. **Why:** CHD metadata/compression does not guarantee the byte arrangement expected by the current callback. **Prevention:** implement only source formats that map byte-for-byte; zero-fill when absent; include exact subcode comparisons and hashes.

### Hostile counts/offsets/path names
**What goes wrong:** overflow, oversized allocation, out-of-range read or reading outside the selected directory. **Prevention:** checked arithmetic, explicit maxima, confined canonical paths, truncated/malformed fixtures, bounded fuzz runs and failure-atomic opens.

### License and archive drift
**What goes wrong:** linked binaries include an unapproved generator component or omit an indirect notice; `.info` claims CHD on an OFF build. **Prevention:** exact-pin closure audit before compiling, reconcile provenance, build ON/OFF metadata checks, and validate final archives' notices and feature claims.

### Slow feedback
**What goes wrong:** a long guest/matrix run discovers an unsorted inventory, absent fixture or wrong artifact selector. **Prevention:** run cap/hash/count/path/selector/inventory/budget checks first, then focused provider tests, then one changed-head hosted run. Inspect CI while running and after; do not repeat equivalent green suites for documentation.

## Code Examples

No new libchdr function names are prescribed here because the accepted source's exact callback declarations must be checked at the vendored pin during implementation. Use its file callback interface from the host provider; never expose a path or libchdr handle through the public emulation API. Source references for exact API declarations: `.planning/research/STACK.md:26` and the pinned `include/libchdr/chd.h` link in that file.

## Open Questions

1. **Which run-local CHD generator has a fully permissive compiled closure?** Existing advisor review found candidate pins but no cleared route. Resolve by auditing exact files and transitive compiled dependencies, then verify deterministic output; otherwise keep the concrete blocker and continue independent work.
2. **Which exact track modes and subcode encoding are represented by the owned fixture?** Inspect generated CUE/BIN and candidate CHD metadata before locking the subset. Unsupported representations must fail explicitly.
3. **Do Main and Release have safe same-SHA artifact identity and material savings?** Record per-job runner seconds/minutes, aggregate total, critical-path duration, SHA, artifact names/digests and producer/consumer validation. If SHA or coverage differs, preserve separate gates and close todo 050 with that evidence.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|-------------|-----------|---------|----------|
| CMake | Configure/build and CTest | Yes | 4.4.3 | — |
| C++ compiler | Candidate fixture tools only; runtime remains C | Yes | `c++` present | Select a C-only generator if practical |
| Rust/Cargo | Only if the audited fixture route requires it | No path observed | — | Avoid Rust tool or add a pinned reproducible setup in a plan |
| `act` | Optional local workflow replay | Yes | Not captured | Use `act --dryrun` for routing; retain hosted jobs for GitHub-only behavior |
| GitHub Actions/GitHub App | Required hosted matrix, release and publishing behavior | Hosted | — | No faithful local substitute for secrets, runner labels, attestations or release-app integration |

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | CMake + CTest |
| Config file | `CMakePresets.json`; subsystem inventory `tests/disc/inventory.txt` |
| Quick run command | `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-disc-image && ctest --preset ci --no-tests=error -R '^disc[.]image[.]' --output-on-failure` |
| Full suite command | `cmake --workflow --preset ci` |

The quick command exercises the current provider regression selectors. Add a CHD-on preset or an explicit build directory/option after `SCHWAYSTATION_CHD` is implemented; do not present the current default-OFF command as CHD coverage.

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| MEDIA-01 | CUE/BIN and CHD yield identical identity, TOC, sectors, applicable subcode and failure behavior. | integration | `ctest --preset ci --no-tests=error -R '^(disc[.]image[.]|disc[.]hash$)' --output-on-failure` | Existing provider tests; CHD parity cases to add |
| MEDIA-02 | Invalid counts/offsets/truncations/path escapes fail boundedly; inventory checks track/sector counts and boundary LBAs before guest boot. | unit/fuzz | `ctest --preset ci --no-tests=error -R '^disc[.]image[.]' --output-on-failure`; run the registered bounded CHD fuzz target once target name is confirmed | Existing CUE/ISO limits and path tests; CHD malformed/fuzz coverage to add |
| CHD-01 | Option ON opens the supported subset and produces the same hashes on eight applicable lanes; OFF remains dependency-free; archive notice and `.info` match build. | integration | `cmake --workflow --preset ci` plus required release rehearsal/package checks | CHD-specific gates to add |
| COMPAT-02 | New feature rows ratchet without deleting prior passes; missing/duplicate keys and counts fail before broad guest work. | unit | Run the existing compatibility-ledger preflight/selector before `cmake --workflow --preset ci`; retain its exact registered selector from CMake inventory | Ledger/ratchet exists; Phase 9 feature rows/preflight coverage to add |

### Sampling Rate

- **Per task:** sorted-inventory, pin/notice, fixture-size/hash and focused `disc.image` selectors as applicable.
- **Before guest boot:** one low-cost fixture/metadata/path/track-count/extent/boundary/hash preflight.
- **Per integrated slice:** CMake configure/build with CHD OFF and ON plus targeted CTest selectors.
- **Phase gate:** full required CI matrix and release archive rehearsal; verify all eight applicable hash lanes and package checks.
- Use `act` only if it adds a relevant local workflow test beyond the direct local suite; `act --dryrun` can check routing, not hosted permissions, tokens, attestations or release App behavior.

### Wave 0 Gaps

- CHD-enabled target/preset and exact pinned reader closure.
- License-cleared reproducible run-local CHD fixture generator and output-cap check.
- CHD structural preflight, parity selectors, malformed corpus/fuzz target, and feature-ledger rows.
- ON/OFF `.info` and archive-notice validation for the CHD feature.

## Security Domain

`security_enforcement` is explicitly false in `.planning/config.json`; the universal ASVS checklist is not active. Hostile input still remains in scope by MEDIA-02. Threats to address in the plan: untrusted metadata causing integer overflow or oversized allocation; path escape via image metadata; malformed hunk reads; partial active-disc replacement; and license/notice mismatch. Mitigations must cite bounded checks, confined paths, staged open, fuzz/regression tests, and archive validation.

## Sources

### Primary project sources (HIGH confidence for project requirements/contracts)
- `.planning/REQUIREMENTS.md` — MEDIA-01, MEDIA-02, CHD-01, COMPAT-02.
- `.planning/preparation/DECISIONS.md` — DEC.42/43/46/69/78/87/91/94.
- `.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md` — SCM.13, CHD v5 metadata and host disc interface.
- `.planning/research/STACK.md` — approved libchdr pin, static C backend and notice choices.
- `include/schwaystation.h`, `host/disc_image.c`, `tests/disc/tests.cmake`, `.github/workflows/release.yml` — current integration seams.
- `AGENTS.md`, `.planning/AUTONOMOUS.md`, `.planning/LESSONS.md`, and todos 005/050 — execution, security, CI and release constraints.

### Pinned upstream documentation (CITED; closure must still be verified when fetched)
- [libchdr approved source tree](https://github.com/rtissera/libchdr/tree/47d794151b2b64f6aba60304e6480f4f5a2bc24b) — reader and API.
- [libchdr CMake options at the approved pin](https://github.com/rtissera/libchdr/blob/47d794151b2b64f6aba60304e6480f4f5a2bc24b/CMakeLists.txt) — backend and dependencies.
- [libchdr license at the approved pin](https://github.com/rtissera/libchdr/blob/47d794151b2b64f6aba60304e6480f4f5a2bc24b/LICENSE.txt) — root BSD-3-Clause terms.
- [Pinned MAME `chdman` source tree](https://github.com/mamedev/mame/tree/96df6a708bdd2bcfa8a8a75a2330e0a6b806ac86) — candidate fixture tool only; do not use until its full compiled closure is cleared.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | The current owned fixture can be converted to a CHD below 512 KiB without changing track coverage. | Fixture generator | An oversized/incomplete fixture forces another owned test input or a different compression route. |
| A2 | The generator's exact build closure can be made permissive and reproducible on all required build hosts. | Standard Stack | Could block parity or violate the no-GPL source policy. |

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH for project-approved libchdr pin/build boundary; LOW for fixture generator until closure audit.
- Architecture: HIGH; follows existing public callback and host provider.
- Pitfalls/validation: HIGH; requirements and tests explicitly call out bounds, paths, fuzzing, preflight and cross-platform hashes.

**Research date:** 2026-11-19
**Valid until:** 2026-12-19 for pinned local architecture; recheck upstream source details at exact pin before vendoring.
