# Phase 16: Patches, cheats and compatibility evidence - Context

**Gathered:** 2026-11-25
**Status:** Ready for planning

<domain>
## Phase Boundary

Deliver the Phase 16 requirements: runner-side PPF 1/2/3 and BPS patching over supported raw sector reads; bounded deterministic GameShark writes for the runner and libretro host through a narrow core RAM-write operation; a pinned Redump DAT inventory report with pass/fail/unknown accounting; and a committed required-capability matrix that controls the libretro experimental metadata claim. Preserve the source image, reject invalid input without partial effects, and keep every untested title or required capability unknown. DAT inventory is not a playability claim.

</domain>

<decisions>
## Implementation Decisions

### Patch identity and media mapping
- **D-01:** Keep patch parsing and transformation in the host. PPF addresses use an explicit raw 2352-byte track-file mapping; initially reject compressed, multi-track, virtual-pregap, synthesized-sector, or otherwise ambiguous layouts unless implementation proves an exact mapping. Validate the entire patch and staged result before patched sectors become visible, and preserve patch-record order for overlaps. PPF2 source-size and block checks and the optional PPF3 1,024-byte block check are format checks only; PPF3 has no source-size field and its block check may be absent. Require a caller-supplied source SHA-256 for PPF1 and whenever a format check is absent or insufficient for the identity claim; report exactly which checks were performed and never describe CRC/block checks as cryptographic identity. BPS source/target/patch CRCs are likewise non-cryptographic. Reject BPS size changes that would change the source sector geometry. Never redefine `schway_disc.identity_sha256`, which currently identifies the first 16 data sectors plus serial.
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

### the agent's Discretion
The planner may choose module/file boundaries, CLI spelling, concrete line/entry caps within the preparation bounds, temporary target storage, report formatting, fixture design, and the test decomposition. It may refine the supported raw-track subset where current provider behavior proves a narrower or broader safe mapping. It must preserve the decisions above and the project's deterministic, bounded, no-partial-application contracts.

### Folded Todos
- **TODO052 — Isolate the serial BE32 movie test from the main test shard:** its original Phase 15 note predates the observed `libretro.host` runtime. Update the evidence to include both tests and evaluate the full candidate together in this phase; keep it pending unless the complete shard contract is proven and measured.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Requirements and settled choices
- `.planning/ROADMAP.md` §Phase 16 — phase goal, requirements and success criteria.
- `.planning/REQUIREMENTS.md` — `PATCH-01`, `CHEAT-01`, `COMPAT-01`, `COMPAT-03`, plus phase budgets and test constraints.
- `.planning/preparation/DECISIONS.md` — DEC.65 host-side patch/cheat boundary; DEC.69 metadata claims tied to tests; DEC.75 append-only public API; DEC.77 integer-only deterministic core.
- `.planning/AUTONOMOUS.md` — autonomous authorization, research, CI and delivery preferences.
- `.planning/LESSONS.md` — imported lessons to check at each phase step.

### Patch and cheat behavior
- `.planning/preparation/README.md` — preparation index and source/licensing rules.
- `.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md` §6 — patch/cheat behavior, identity, hostile-input bounds and phase scope.
- `.planning/preparation/HOSTS-AND-INTEGRATION.md` — runner and libretro integration boundaries.
- `PROVENANCE.md` — allowed references and provenance.
- `include/schwaystation.h` — current disc identity, state and public memory API contracts.
- `host/disc_image.c`, `runner/main.c`, `src/instance.c`, `libretro/libretro.c` — sector loading, runner lifecycle, checked memory access and libretro frame/state/cheat hooks.
- PPF format primary source: [PPF format repository](https://github.com/meunierd/ppf), including the author's [PPF3 specification](https://raw.githubusercontent.com/meunierd/ppf/master/ppfdev/PPF3.txt).
- BPS format primary source: [BPS File Format Specification](https://floating.muncher.se/byuu/bps/bps_spec.html).
- GameShark write types: project-pinned preparation source `.planning/preparation/PSXDEV-EXTRACTS.md`; reference page [PSX cheat devices](https://psx-spx.consoledev.net/cheatdevices/).

### Compatibility, data and CI
- `.planning/preparation/CONFORMANCE.md` §3 — compatibility ledger, evidence rules, fixture coverage and generated report.
- `ASSET_POLICY.md` and `PROVENANCE.md` — DAT licensing and no-bulk-redistribution policy.
- `scripts/library.sh` and root `pins.txt` — pinned DAT acquisition and local accounting path.
- `docs/compatibility.md`, `tests/compat/compat_tests.c`, `tests/compat/tests.cmake` — existing reports, ledger tests and current hard-coded matrix behavior.
- `libretro/schwaystation_libretro.info.in` and `tests/cmake/info_claims.cmake` — current experimental and cheat claims and their guard.
- `.github/workflows/ci.yml` and `tests/cmake/workflows.cmake` — protected compatibility sparse checkout and its workflow contract; include `tests/compat/features.csv` in both.
- `.planning/todos/pending/052-be32-movie-test-shard.md` — CI shard candidate and required proof.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `host/disc_image.c` already maps supported disc providers to sector reads; preserve each provider's current source and virtual-pregap distinctions.
- `schway_peek` in `src/instance.c` already validates physical memory offsets, widths, alignment and bounds without side effects; use the same conventions for the new write operation.
- `retro_run` queues input before `schway_run_frame`, giving the libretro host a frame-start point for ordered cheat writes. The current `retro_cheat_reset` and `retro_cheat_set` hooks are no-ops.
- `scripts/library.sh` already downloads and digest-checks the pinned DAT. The compatibility ledger and ratchet provide a starting point, but current profile rows are explicitly not boot/play proof.

### Established Patterns
- Core modules use fixed-width integers, instance-owned state, checked public structs, and validate-then-apply behavior.
- The source disc image remains host-owned; public `schway_disc.identity_sha256` describes only the first 16 data sectors and serial.
- Compatibility evidence is sparse and evidence-linked; unknown is valid, synthetic feature rows do not prove retail playability, and protected passes cannot disappear without a dated allowance.
- CI uses a cheap consistency preflight before long guest suites; Main and hosted-only integration evidence remain required where configured.

### Integration Points
- Runner option parsing and frame loop in `runner/main.c`; patched sector callback through `host/disc_image.c` and the existing disc callback in `include/schwaystation.h`.
- Core checked RAM access in `include/schwaystation.h` and `src/instance.c`.
- Libretro `retro_run`, cheat callbacks, serialization functions and `.info` claims in `libretro/libretro.c` and `libretro/schwaystation_libretro.info.in`.
- DAT accounting/report generation and fail-closed matrix generation in `scripts/library.sh`, compatibility docs and `tests/compat`.

</code_context>

<specifics>
## Specific Ideas

Use the canonical PPF and BPS formats only within a documented source-layout subset. For GameShark, the initial subset is the familiar unconditional `30` byte and `80` halfword constant writes. Keep every claim tied to the exact measured run/configuration that supports it; CRC or a profile selection test must not be described as cryptographic identity or playability.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 16-Patches, cheats and compatibility evidence*
*Context gathered: 2026-11-25*
