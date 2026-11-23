# Phase 12: SBI and LibCrypt — Research

**Researched:** 2026-11-22  
**Domain:** SBI sidecars, CD-ROM Q position behavior, provider and CI integration  
**Confidence:** HIGH for documented SBI record layouts and current integration defects; implementation bounds need owned tests.

## Phase constraints

The owner-approved phase context is authoritative: `.planning/phases/12-sbi-and-libcrypt/12-CONTEXT.md` D-01–D-08. Keep SBI in the host provider, bind each overlay to its concrete image/TOC and M3U child, do not claim the format authenticates a disc, preserve sector delivery, and exercise read, seek, swap and GetlocP behavior. No public ABI change, PBP multi-disc support, retail-title boot claim, or CI routing change is in scope. Preserve all required CI lanes and the 3,901-line gate cap.

## Format facts

The project-pinned PSX reference, `PSX-SPU-CDROM-AND-MDEC.md` §2.7 (SCM.05), documents the SBI layout: a four-byte `SBI\0` file ID followed by records. Each record begins with the real absolute MSF address (3 bytes) and a format byte. Format 1 then has ten bytes of Q0–Q9 data; formats 2 and 3 each have three bytes for the wrong relative (Q3–Q5) or absolute (Q7–Q9) MSF. The SBI cannot store Q10–Q11 CRC. Therefore every listed sector must be exposed as bad-Q; do not use record Q data as a valid Q replacement. The file has no disc digest and cannot authenticate image pairing.

The same facts are independently visible in the [psx-spx SBI specification](https://psx-spx.consoledev.net/dev/ps1/cdr/cdromfileformats/diskimages/) (SBI section). It specifies the header and three record shapes, the missing CRC, and the controller behavior of retaining the most recent intact Q position. This is format documentation, not an identity mechanism. Records must be converted from the disc's absolute MSF to the provider's LBA with checked range arithmetic, then checked against that selected provider's TOC/map. Choose a finite record/file bound from existing hostile-input conventions and prove its rejection behavior with tests; the source does not define a safe parser cap.

## Code evidence and architecture

- `schway_disc.bad_q` is already the core/host seam; the core must not read files. `host/disc_image.c` owns concrete images and M3U children, so keep the overlay/callback with the child rather than in playlist-global state.
- `src/cdrom_sector.c:174–199` extracts Q and saves a valid-looking record before the caller applies the `bad_q` result. In the read path, a listed bad-Q sector can therefore replace the saved record even though `sector_advance` preserves `current_lba`.
- `src/cdrom_sector.c:314–337` completes seek using `sector_location` without consulting `bad_q`; a listed sector can alter the last accepted position there too.
- `src/disc.c:76–83` applies callbacks, TOC and identity but leaves the CD-ROM's cached `q_record_valid` intact. Reset cached Q state on a provider change so GetlocP cannot leak a prior disc's position.
- `tests/fixtures/src/disc_boot.sbi` is currently only a 116-byte `SBId` marker checked by `tests/disc/fixture.c`; it is not an SBI parser vector. Replace or supplement it with owned, independently constructed SBI records and update the fixture manifest if committed bytes are needed.
- Existing CMake inventories are exact and sorted. New rows must be sorted before configure. Run parser/input/output-count and selector preflights before builds, fuzz replay or guest command tests.

## Recommended implementation and verification order

1. Add the smallest documented SBI parser/provider slice with independent vectors for formats 1, 2 and 3, checked MSF-to-LBA conversion, duplicate and boundary behavior, malformed/truncated records, unsupported format, out-of-map records, absent sidecar, and failure-atomic open. Keep parsing bounded; no generic format framework or dependency.
2. Verify the per-image binding through two selected M3U children with overlapping numeric LBAs but different maps/overlays. Document that a semantically wrong SBI may still fit the selected map because SBI has no identity digest.
3. Fix the core's Q acceptance ordering in both read and seek paths and reset cached Q when applying a different disc. Keep bad-Q data delivery and normal target advancement unchanged. Add a guest-visible command sequence that establishes a good Q, swaps discs, reads a good sector and a valid-looking-Q bad sector, checks its data, then checks GetlocP returns the selected disc's prior-good Q.
4. Register `fuzz.sbi` plus deterministic seed replay in existing fuzz/disc paths; update the feature ledger only for provider binding, data availability and prior-good GetlocP predicates that actually pass.
5. Hoist the identical runner PCM capture once per region in `tests/libretro/tests.cmake`. Preserve all three regions, both callback comparisons per region, and file-size/hash assertions. This removes three repeated guest runs from the measured 18m52s BE32 `libretro.host` lane without reducing coverage.
6. Run cheap sorted-inventory, fixture extents/counts, selector wiring and line-budget checks, then targeted native configure/build/tests. Review source before final verification; run the full approved local and hosted lanes once on the changed head. Use `act` only if it adds executable coverage beyond local tests; use `act --dryrun` if workflow routing is changed (no routing change is proposed here).

## Adversarial findings and limits

- Moving only the `bad_q` callback earlier is insufficient: `sector_location` mutates both cached Q and derived position. Make acceptance atomic with the bad-Q decision.
- A same-disc test misses stale Q across swap. Keep the A→B swap in the guest-visible scenario and make B's prior-good position distinguishable from A's.
- SBI has no checksum of the disc and no CRC field. Provider/map binding prevents cross-child leakage, but cannot prove the user selected the semantically correct sidecar when its records map in range.
- Do not expand to M3S or multi-disc PBP. Neither is in the phase contract.
- Existing Main and Release matrices remain separate: prior measurement found differing archive bytes on their shared commit. The safe measured optimization in this phase is the duplicated libretro capture.

## Sources and runtime

- `.planning/preparation/README.md` directs CD-ROM/disc questions to `PSX-SPU-CDROM-AND-MDEC.md` §2.7, §3 and §5; SCM.05 supplies the project-pinned SBI facts.
- [psx-spx: Disk Images, SBI](https://psx-spx.consoledev.net/dev/ps1/cdr/cdromfileformats/diskimages/) — independent format and bad-CRC behavior cross-check.
- Repository source references above are current code evidence, not claims about hardware.

**Runtime/model:** Codex with delegated GSD researcher (`gpt-6-luna`, xhigh); parent checked format facts and code paths against the pinned prep notes and psx-spx. No source implementation or test was run during research.

## Phase requirements

| ID | Description | Research support |
|---|---|---|
| CRYPT-01 | Bind documented SBI cases to the selected provider/map and preserve data reads while GetlocP retains the previous good position; fuzz/replay malformed input and report only tested protection evidence. | SBI format/CRC limit, provider ownership, core Q-state defects, command-test sequence and bounded parser plan above. |
