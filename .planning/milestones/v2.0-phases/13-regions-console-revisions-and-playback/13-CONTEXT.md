# Phase 13: Regions, console revisions and playback - Context

**Gathered:** 2026-11-23
**Status:** Ready for planning

<domain>
## Phase Boundary

Deliver the Phase 13 machine-profile, BIOS-shell boot and bounded FMV behavior in the library, runner and
libretro core. Keep region selection independent from board revision and title identity; support every documented
legal board/GPU pairing; keep the normal BIOS path as the default and add an opt-in BIOS-backed shell shortcut;
and add targeted MDEC evidence without extending the accepted v1.0 hardware-accuracy claims.

</domain>

<decisions>
## Implementation Decisions

### Machine profiles and compatibility identity

- **D-01:** Add board identity through the existing size-tagged, append-only profile API. Board selection supplies
  its documented derived GPU, SPU/CD firmware defaults, RAM-control boot value and port flags; reject explicit
  board/GPU mismatches. The supported board set is `pu7`, `pu8_early`, `pu8_late`, `pu18`, `pu20`, `pu22`,
  `pu23`, `pm41` and `pm41_2`. The legal GPU behavior pairs are `pu7/v0`, `pu8_early/v0`, and `v2` for every
  other board (PM-41(2)'s CXD9500 is represented by the documented v2 behavior). Use `pu18` as the new default
  because it matches the settled v2 GPU and vC2 CD firmware defaults. Keep callers using older profile sizes,
  including the existing `SCHWAY_PROFILE_GPU_V0` flag, behavior-compatible. Update ABI, behavior and state
  versions when required; serialize, validate and hash behavior-affecting board identity. — **Reversibility:**
  costly — the size-tagged public struct, serialized state and compatibility matrix are downstream contracts.
- **D-02:** Keep console region, board revision and title serial as separate values. In host `auto` mode, infer
  region from the serial prefix, then the disc licence string, then the EXE marker; report disagreements as lint,
  and let an explicit region override win. Default runner and libretro selection to `auto`, falling back to the
  current NTSC-U behavior when no content identity supplies a region. Pass a concrete region to the core; do not
  add file or identity detection to the deterministic core. Preserve the existing title-ledger `region` column.
- **D-03:** Record only profile predicates actually exercised in the existing feature ledger. Keep its schema and
  encode new profiles canonically as `<region>.<board>.<gpu>`, with lower-case board tokens and hyphens for
  underscore-separated names (for example `pal.pm41-2.v2`). Keep existing region-only rows unchanged. Do not
  imply a title boot or play result from a profile-selection test.

### BIOS boot shortcut

- **D-04:** Keep full BIOS boot as the default. Add `full|fast` selection to the runner and the documented
  libretro boot option. `fast` remains opt-in, requires a supplied BIOS and eligible disc executable, and hooks
  only the documented kernel shell entry at `0x80030000` after normal BIOS initialization. Keep standalone
  `--exe` on its existing BIOS-free stub path; it is not a BIOS fast-boot case. Do not add HLE, bypass BIOS
  initialization or change emulated clocks and timing. Invalid combinations fail before execution.

### MDEC evidence and iteration

- **D-05:** Keep the exact v1.0 4-bit output, frozen deterministic 8-bit baseline and 4,915,200-cycle cap.
  Reuse the source-unit preflight before every FMV guest run and report compressed-source coverage, decoder output,
  guest completion and displayed-image comparison as separate results. Add only bounded edge regressions backed by
  the pinned inputs and current accepted evidence; do not claim exact 8-bit hardware output or complete external
  frame/movie conformance.
- **D-06:** Before expensive work, run the cheapest relevant inventory, tuple-count, selector, fixture-extent,
  MDEC source-coverage and 3,901-line repository-check-budget controls. Sort exact inventories before configure.
  Reuse the existing local workflow evidence and use `act --dryrun` for routing only when workflow wiring changes;
  run `act` jobs only when they add coverage beyond local tests. Inspect each hosted CI/CD run while active and
  after completion for critical-path time, duplicate work, iteration delay and raw runner cost, preserving every
  required job, platform, case and assertion. Any Docker resources created for this work must use identifiable
  schwaystation/plan names and be removed individually at task close; no global prune.
- **D-07:** Do not fold unrelated todo matches based only on the current keyword matcher: its 29 Phase 13 results
  all carry the same generic score. Keep TODO 049 bound to its Phase 7 runner/CI review, TODO 051 bound to Phase
  10 Nightly routing, TODO 047 bound to the Phase 6 CD-ROM DMA caller, and TODO 048 as a test-maintenance seed.
  None changes a Phase 13 requirement or is folded into this phase.

### the agent's Discretion

- Choose internal board tables, enum spellings, helper boundaries and exact fixed-point tests from the canonical
  hardware facts; do not expand into undocumented silicon variants.
- Preserve legacy size-tagged callers and the old GPU flag while rejecting illegal tuples on explicit new board
  profiles. The planner must check compatibility tests and state versioning before choosing the internal encoding.
- Choose the smallest owned, deterministic boot and MDEC fixtures that cover every named result; keep guest work
  bounded and leave generated data in the build tree.

### Reviewed Todos

- **047 — CD-ROM DMA delay cost:** remains bound to the first CD-ROM DMA device plan in Phase 6; Phase 13 adds
  profile identity but does not change the already-built CD-ROM DMA delay selection.
- **048 — Split long GPU pixel test cases:** remains a test-maintenance seed. Phase 13 adds profile and timing
  cases but does not require restructuring the existing pixel-test helpers.
- **049 — Review runner/checkpoint repeats:** remains bound to its Phase 7 CI review; this phase's runner changes
  add new profile and boot coverage rather than repeating identical runner/checkpoint outputs.
- **051 — Nightly PR job routing:** remains a Phase 10 workflow item; Phase 13 does not edit Nightly routing.

</decisions>

<canonical_refs>
## Canonical References

### Scope, decisions and operating constraints

- `.planning/ROADMAP.md` — Phase 13 success criteria and phase dependency boundaries.
- `.planning/REQUIREMENTS.md` — PROFILE-01, PROFILE-02, BOOT-01, MDEC-01 and the shared test/budget contract.
- `.planning/AUTONOMOUS.md` — discuss, review, CI efficiency, cheap preflights, `act` and Docker guidance.
- `AGENTS.md` — API compatibility, determinism, test budgets, branch/release policy and evidence format.
- `.planning/LESSONS.md` — especially SW-ORACLE-002/003 and SW-CI-001.
- `.planning/preparation/DECISIONS.md` — DEC.04, DEC.17, DEC.27, DEC.64, DEC.75 and DEC.83.
- `.planning/preparation/README.md` — source index and required subsystem reading routes.

### Hardware and host behavior

- `.planning/preparation/PSX-SYSTEM.md` §§7–9 — BIOS shell entry, region precedence, board table, derived defaults
  and power-on state.
- `.planning/preparation/PSX-GPU-AND-GTE.md` §1.1 — v0/v2 GPU behavior differences and revision evidence.
- `.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md` §4 — MDEC source/output behavior and existing evidence limits.
- `.planning/preparation/HOSTS-AND-INTEGRATION.md` §§3, 6 — libretro region/boot options and runner command contract.
- `.planning/preparation/CONFORMANCE.md` — compatibility evidence schema, confidence and fixture rules.
- `.planning/milestones/v1.0-MILESTONE-AUDIT.md` — accepted v1.0 MDEC boundary.

### Existing implementation and tests

- `include/schwaystation.h` — size-tagged `schway_profile` and public profile fields.
- `src/instance.c`, `src/state.c`, `src/internal.h` — profile validation, state serialization and version values.
- `runner/main.c`, `runner/probe.c`, `host/disc_probe.c` — runner options and host-side content identity/region inputs.
- `libretro/libretro.c` — core options, content setup, region and boot integration.
- `cmake/MDECReferences.cmake`, `tests/mdec/` — pinned source-unit preflight and bounded guest/model checks.
- `tests/compat/features.csv`, `tests/cmake/compat-ratchet.cmake`, `docs/compatibility.md` — feature rows, protected
  schema and generated compatibility guidance.
- `tests/*/tests.txt`, `tests/CTestTestfile.cmake` — exact subsystem test inventories and registration patterns.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets

- PAL integer clock ratios, 314-line frame timing and audio cadence already exist and provide regression baselines.
- `schway_set_profile` validates profiles and locks them after execution starts; state and runner hashes already
  cover profile-dependent state.
- Runner/libretro region selectors and the host disc probe provide integration points; extend them without moving
  media reads or identity detection into the core.
- Existing MDEC preflight gates the pinned 4-bit and deterministic 8-bit paths and already enforces the cycle cap.

### Established Patterns

- Public C structs are size-tagged and append-only; new behavior must preserve older sizes and caller-owned data.
- Behavior-affecting state has explicit validation, serialization and deterministic-hash tests.
- Compatibility feature rows are keyed by provider/profile/protection/peripheral/predicate and checked by the
  protected ratchet; title rows retain their separate serial/MD5 identity and region.
- Host-specific input and file handling stay outside the integer-only core.

### Integration Points

- Region and board selection must agree across the library profile, runner flags, libretro options, state format,
  hashes and compatibility feature rows.
- The BIOS shell shortcut joins BIOS loading, selected disc/EXE identity and the kernel entry hook; ordinary BIOS
  boot and the existing standalone EXE stub remain separate tested paths.
- Phase 13 MDEC cases join the pinned source preflight, guest/model selectors and explicit result reporting.

</code_context>

<specifics>
## Specific Ideas

- Canonical feature-profile values use `ntsc-j.pu8-early.v0`, `ntsc-u.pu23.v2` and `pal.pm41-2.v2` forms; only
  predicates actually tested are recorded.
- The BIOS shortcut is intentionally narrow: initialize through the supplied BIOS, intercept the documented
  shell entry, then launch eligible disc content; never synthesize a BIOS or shortcut the normal path by default.
- A complete source scan means only that compressed source bytes are present. Decoder output, guest progress,
  display comparison and hardware fidelity remain independent claims.
- Before a long local or hosted run, check the selected cases, inputs, generated counts, sorted inventories and
  budgets first; inspect every CI/CD run for measured, safe iteration-time savings.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within Phase 13 scope. Exact 8-bit MDEC hardware matching and full frame/movie
conformance remain outside v2.0 under the accepted v1.0 evidence boundary.
</deferred>

---

*Phase: 13-regions-console-revisions-and-playback*
*Context gathered: 2026-11-23*
