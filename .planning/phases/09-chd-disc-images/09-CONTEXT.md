# Phase 9: CHD disc images - Context

**Gathered:** 2026-11-19
**Status:** Ready for planning

<domain>
## Phase Boundary

Players can open the documented, supported CHD v1–v5 subset with the same observable behavior as equivalent uncompressed discs. Keep parsing and decompression in the host provider, keep `SCHWAYSTATION_CHD` opt-in by default, and prove byte-level parity before enabling it in release artifacts. Reject layouts whose behavior cannot be mapped exactly to the existing disc callback. This phase also adds the shared media bounds/path checks and feature-evidence ledger contract needed by later media providers.

</domain>

<decisions>
## Implementation Decisions

### CHD projection and supported layouts
- **D-01:** Project CHD metadata onto the existing `schway_disc` contract: raw 2352-byte sectors, 96-byte subcode when available, and the existing two-index TOC fields. Map `index0` to pregap start, `index1` to program start, and leadout to the exclusive logical end; CHD alignment padding is not a logical LBA.
- **D-02:** Use validated metadata to distinguish stored and virtual pregaps. Preserve existing zero-subcode behavior when a track has no subcode so the core can synthesize Q. Reject unsupported sector modes, subcode encodings, nonzero postgaps, and multisession layouts until exact behavior is covered.
- **D-03:** Keep all parsing and libchdr calls host-side. Build the BSD-3-Clause libchdr closure statically behind `SCHWAYSTATION_CHD`; default remains OFF. Preserve `UNSUPPORTED` for CHD when disabled and make failed opens leave the active disc unchanged.

### Fixture, parser safety, and parity evidence
- **D-04:** Generate a run-local CHD from the existing owned CUE/BIN fixture. Before selecting a generator, audit the exact pinned source and compiled dependency closure for permissive licensing; the MAME `chdman` entry files have BSD-3-Clause headers but its complete closure is not yet cleared. Do not commit or fetch a prebuilt opaque CHD. Keep the generated CHD under the 512 KiB fixture cap and distribute identical bytes to all applicable hash lanes.
- **D-05:** Before any long guest run, cheaply check fixture size and hashes, bounded header/count/offset arithmetic, companion path confinement, modes, indexes, pregaps, leadout, extent counts, and boundary-sector reads through the final LBA. Then compare CHD and uncompressed identity, TOC, raw sectors, applicable subcode, and hashes on all eight applicable lanes.
- **D-06:** Add malformed-image regression and fuzz coverage for shared media bounds. Invalid input must not partially replace the active disc. Keep the compatibility ledger preflight strict about missing/duplicate keys and protect prior passing rows with dated allowances only.

### Release behavior and CI efficiency
- **D-07:** After parity passes, enable CHD in distributed release artifacts while retaining the default-OFF CMake build and the current release asset names. Verify ON/OFF `.info` extensions and archive notices; the published feature claim must match the shipped binary.
- **D-08:** Resolve todo 050 with exact-SHA workflow/artifact identity checks and measured runner-minutes and critical path. Keep both required Main and Release checks, all matrix lanes, and package validation unless measured savings and tests prove equivalent artifact reuse safe. Inspect each CI run while active and after completion; try `act` only where local execution adds coverage beyond the existing suite.
- **D-09:** Before expensive build, suite, guest, or hosted CI work, run the cheapest relevant existing consistency checks first. Add permanent preflight code only for a recurring fault named by a requirement; preserve phase gate budgets and every acceptance case.

### the agent's Discretion
- Choose private helper boundaries, exact supported metadata variants, fuzz duration, test selectors, plan waves, and build-tool integration while preserving byte parity, bounded parsing, and the host/core boundary.
- Select the smallest fixture-generation route only after the complete pinned license/build closure and reproducibility are established. If no route is cleared, keep the blocker concrete and continue independent Phase 9 work.
- Apply evidence-backed CI efficiency fixes within this phase; do not trade away lanes, assertions, required status checks, archive validation, or quality bounds.

### Folded Todos
- **005 — notices gates:** The new optional libchdr dependency makes SPDX and `.info` license-notice accuracy part of the CHD ON/OFF release contract. Fold the required remaining notice checks into the phase within the gate budget.
- **050 — release CI artifact reuse:** Phase 9 now changes release capabilities and directly evaluates Main/Release duplication. Measure exact-SHA identity, runner-minutes, and critical-path time, then either test a coverage-preserving handoff or record why separate runs remain justified.

### Reviewed Todos (not folded)
- **006 — vendored digest gate:** Bound to Phase 10's PBP provider, not Phase 9's CHD implementation. Keep it there.
- Other `todo.match-phase 9` candidates were generic keyword matches or belong to unrelated CPU, GTE, prior CI, or later media phases; none adds an acceptance criterion to MEDIA-01, MEDIA-02, CHD-01, or COMPAT-02.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Scope and project policy
- `AGENTS.md` — source/license policy, three deliverables, determinism, verification, budgets, branch and release rules, and CI/CD efficiency requirements.
- `.planning/AUTONOMOUS.md` — autonomous phase boundaries, role review, cheap preflights, `act`, and CI/release inspection.
- `.planning/PROJECT.md` — product and host/core architecture.
- `.planning/REQUIREMENTS.md` — MEDIA-01, MEDIA-02, CHD-01, and COMPAT-02 acceptance criteria.
- `.planning/ROADMAP.md` — Phase 9 scope, dependencies, success criteria, and canonical reference list.
- `.planning/LESSONS.md` — SW-CI-001 on measured workflow reuse and SW-ORACLE-002 on source/guest completeness checks.

### Settled behavior and technical references
- `.planning/preparation/README.md` — index of pinned, provenance-reviewed references; open the linked subsystem material before design.
- `.planning/preparation/DECISIONS.md` — DEC.42, DEC.43, DEC.46, DEC.69, DEC.78, DEC.87, DEC.91, and DEC.94.
- `.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md` — CD sector, TOC, subcode, and host media behavior.
- `.planning/preparation/ENGINEERING.md` — deterministic host/core boundary, CMake testing, fuzzing, and budgets.
- `.planning/preparation/CONFORMANCE.md` — media parity and cross-platform evidence practices.
- `.planning/research/STACK.md` — pinned libchdr and optional static build choices.
- `PROVENANCE.md` — allowed source reuse and notice requirements.
- `ASSET_POLICY.md` and `tests/fixtures/manifest.txt` — generated media limits and committed fixture policy.

### Existing media and CI seams
- `include/schwaystation.h` — public `schway_disc` callback and TOC contract.
- `host/disc_image.c` and `host/disc_image.h` — current host-side ISO/CUE/M3U provider and failure behavior.
- `docs/discs.md` — supported formats and user-facing media errors.
- `cmake/DiscFixtures.cmake` — owned deterministic disc fixture generation and digest checks.
- `tests/disc/image.c`, `tests/disc/tests.cmake`, and `tests/disc/inventory.txt` — provider tests and exact test inventory.
- `libretro/schwaystation_libretro.info.in` — frontend extensions and feature claims.
- `tests/compat/ledger.csv` and its registered ratchet checks — protected feature-evidence contract.
- `.planning/todos/pending/005-notices-gates.md` and `.planning/todos/pending/050-release-ci-artifact-reuse.md` — folded acceptance details.
- `.github/workflows/ci.yml` and `.github/workflows/release.yml` — required matrix and archive producer/consumer wiring.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `host/disc_image.c` is the host provider seam and already returns explicit unsupported/error results while protecting the output pointer on failure.
- `cmake/DiscFixtures.cmake` creates the deterministic CUE and track files that can feed a run-local CHD generator.
- `tests/disc/image.c` and its sorted inventory provide provider regression patterns; compatibility-ledger tests already enforce a protected ratchet.
- The `schway_disc` callback already supplies raw sectors and subcode to runner and libretro consumers, so CHD should normalize into this contract instead of creating a second guest path.

### Established Patterns
- No BIOS, disc, executable, memory-card, or state bytes are committed beyond the fixture manifest. Generated media is bounded and digest-checked.
- Filesystem access and media parsing stay in host modules; emulation remains deterministic, integer-only, and instance-owned.
- Run the changed native configure/build and focused selectors before broad CTest or hosted CI. Review source changes before final verification.
- Keep exact test inventories bytewise sorted. Use an existing preflight before long execution, and retain it permanently only when it prevents a recurring named fault.

### Integration Points
- `host/disc_image` must construct the same identity, TOC, sectors, subcode, and errors for equivalent CUE/BIN and CHD inputs.
- CMake options, presets, libchdr pin/notice, `.info` extensions, and archive assembly must agree on CHD enabled state.
- Every CI hash lane must consume the same run-local CHD bytes and report the same parity evidence; release packaging must validate the exact tested feature configuration.
- Future PBP, ECM, CloneCD, and SBI providers consume shared media checks and feature-ledger conventions without changing the guest-facing contract.

</code_context>

<specifics>
## Specific Ideas

- Compare boundary sectors including the final logical LBA before booting the guest; count and hash source units independently from guest-produced output.
- Compare ON/OFF `.info` claims and packaged notices. Preserve all existing release asset names.
- Treat Main and Release as distinct quality gates until exact-SHA artifact identity tests and measured savings justify reuse.
- Capture every CI outcome as command, exit status, and URL; do not commit logs or rerun an equivalent green full suite for bookkeeping.
</specifics>

<deferred>
## Deferred Ideas

- PBP parsing remains Phase 10; its vendored digest todo remains bound there.
- ECM/CloneCD remain Phase 11; SBI/LibCrypt remain Phase 12.
- Broader CHD sector reconstruction, unsupported metadata modes, postgaps, and multisession support stay out until byte-exact fixtures and requirements cover them.
- Any cross-workflow artifact reuse is deferred unless Phase 9's exact-SHA, artifact identity, and timing evidence proves it safe and worthwhile.

</deferred>

---

*Phase: 09-chd-disc-images*
*Context gathered: 2026-11-19*
