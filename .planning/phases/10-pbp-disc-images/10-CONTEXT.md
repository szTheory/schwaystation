# Phase 10: PBP disc images - Context

**Gathered:** 2026-11-20
**Status:** Ready for planning

<domain>
## Phase Boundary

Players can open explicitly verified, unencrypted PSISO PBP images through the existing host disc interface. The runner and libretro host expose the same deterministic sectors and clear media errors. Encrypted PGD and undocumented variants remain unsupported. Keep parsing, file access, and decompression in the host; do not change the deterministic core contract.

</domain>

<decisions>
## Implementation Decisions

### Supported format and errors
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

### Folded Todos
- **002 — release floor check:** The PBP inflater will be statically linked into packaged libretro builds, so inspect packaged platform floors, dependencies, and required exports against documented supported platforms.
- **005 — tracked-source SPDX scan:** This phase adds the first tracked SPDX-bearing source. Add the source-wide admitted-license and notice scan with its registered negative control, within the existing gate ceiling. Do not duplicate Phase 9's configure-time `.info`/linked-provider check.
- **006 — vendored-file digest check:** Extend the existing `libretro.vendored` gate and negative control to hash the newly admitted standalone inflater source; retain the existing file coverage.
- **051 — Nightly PR routing:** Apply a fail-closed changed-path classifier and its fast self-test, preserving full scheduled/manual coverage and relevant fuzz jobs.

### Reviewed Todos (not folded)
- **003 — pins file schema gate:** Its recorded binding is the first `pins.txt` row after the armips entry; that trigger predates this phase. A PBP source pin is not a new first-trigger. Do not rebind the stale todo without evidence that this phase owns the missing generic schema gate.
- **001 — CI required roll-up:** Nightly routing edits do not add a job to `ci.yml` or alter the required roll-up.
- **004 — single version source:** Phase 10 does not introduce a second machine-edited release version.
- **021 — actionlint local config:** This phase does not add an actionlint gate.
- **050 — release CI artifact reuse:** Already resolved with evidence and a prohibition on untested Main/Release substitution; keep it closed.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Scope, settled decisions, and operating rules
- `AGENTS.md` — licensing, host/core boundary, fixture rules, verification policy, gate budgets, branch/release rules, and CI/CD efficiency.
- `.planning/AUTONOMOUS.md` — fan-out and synthesis, adversarial review, phase boundaries, cheap preflights, `act`, and CI/release inspection.
- `.planning/PROJECT.md` — product scope and three deliverables.
- `.planning/REQUIREMENTS.md` — PBP-01 acceptance criteria and quality boundaries.
- `.planning/ROADMAP.md` — Phase 10 goal, success criteria, dependency, and canonical references.
- `.planning/STATE.md` — current phase state and Phase 9 measurements.
- `.planning/LESSONS.md` — prior CI, media-source completeness, and integration lessons; re-read during planning.
- `.planning/preparation/README.md` — index to the project's pinned reference material.
- `.planning/preparation/DECISIONS.md` — settled media helper, host boundary, profile, release, fixture, and dependency decisions.
- `.planning/preparation/ENGINEERING.md` — build, test, CI, release, source, and gate-budget practices.
- `.planning/preparation/CONFORMANCE.md` — fixture admission, sector parity, ledger, fuzz, and CI evidence rules.

### PBP format, host contract, and dependency provenance
- `.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md` §3 — PBP/PSISO observations and the shared disc-sector interface; the cited PSX-SPX material is reverse-engineered, so accept only fixture-proven variants.
- `.planning/preparation/HOSTS-AND-INTEGRATION.md` — runner and libretro provider integration and metadata claims.
- `.planning/research/STACK.md` — standalone miniz recommendation, dependency boundary, PBP compression, and archive notices.
- `.planning/research/FEATURES.md` — PBP supported subset, unsupported PGD/unknown layouts, and feature dependencies.
- `PROVENANCE.md` and `THIRD-PARTY-NOTICES.md` — approved source admission and distributable notice format.
- `ASSET_POLICY.md` and `tests/fixtures/manifest.txt` — generated media and committed-byte rules.

### Existing code, tests, and workflow seams
- `include/schwaystation.h` — shared `schway_disc` callback and TOC contract.
- `host/disc_image.c` and `host/disc_image.h` — image dispatch, host errors, and failure-atomic output behavior.
- `host/disc_chd.c`, `host/disc_chd.h`, and `host/media_limits.h` — separate provider, checked bounds, and reusable host-side patterns.
- `cmake/DiscFixtures.cmake` and `cmake/CHDFixtures.cmake` — owned fixture production and run-local generated media.
- `tests/disc/image.c`, `tests/disc/media.c`, `tests/disc/inventory.txt`, `tests/runner/tests.cmake`, and `tests/compat/features.csv` — parser, bounded media, exact inventory, consumer, and feature-ledger coverage.
- `libretro/schwaystation_libretro.info.in` and `docs/discs.md` — supported extension and user-facing media behavior.
- `tests/cmake/vendored.cmake`, `tests/cmake/notices.cmake`, and `tests/cmake/expect_fail.cmake` — existing digest/notice checks and negative-control convention.
- `.github/workflows/nightly.yml`, `.github/workflows/ci.yml`, and `.github/workflows/release.yml` — PR routing, required build lanes, and release packaging.
- `.planning/todos/pending/002-release-floor-gate.md`, `.planning/todos/pending/005-notices-gates.md`, `.planning/todos/pending/006-vendored-digest-gate.md`, and `.planning/todos/pending/051-nightly-pr-job-routing.md` — folded acceptance details.
- `.planning/todos/pending/050-release-ci-artifact-reuse.md` — resolved Main/Release reuse evidence and constraint.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `host/disc_chd.c` shows how an optional image format implements the same host-side disc interface without changing the core.
- `host/media_limits.h` and existing checked-size helpers provide bounds patterns for hostile media input.
- `tests/disc/image.c` and the disc inventory already contain explicit unsupported-format expectations that PBP support must update.
- `cmake/DiscFixtures.cmake` can provide owned source media; generated PBP output must remain in the build tree and be identity-checked.
- `tests/cmake/vendored.cmake` already hashes vendored sources and has a negative control; extend it instead of creating a parallel digest gate.

### Established Patterns
- The host opens files and supplies sectors; the deterministic core never reads a path or environment.
- Failed provider opens leave the current image/output unchanged. Media parsing is bounded and fuzzable.
- Disc inventories are sorted bytewise. Generated fixtures are checked before sector replay or guest execution.
- Existing CI runs the required platform matrix and the docs define which claims can be made from fixture or ledger evidence.

### Integration Points
- Route `.pbp` through `host/disc_image` to the new provider, then exercise it through runner and libretro.
- Update the libretro extension only when the supported build advertises PBP; preserve `.info` license accuracy and archive notices.
- Compare the PBP's TOC and raw sectors to its owned source before treating any variant as supported.
- Extend the existing test/build/fixture wiring so applicable native, ASan, wasm32, BE32, and release consumers use the same run-local bytes.
- Nightly classification is workflow-level PR routing only; scheduled/manual runs and Main/Release requirements remain complete.

</code_context>

<specifics>
## Specific Ideas

- The public PBP reference is partly reverse-engineered; unknown structures remain unsupported until an owned fixture proves their mapping.
- Verify the requested source track count against both decoded sectors and boundary reads before guest execution.
- Check the exact miniz revision, MIT notice, and per-file hash before source admission; keep the standalone PBP copy distinct from libchdr's build-tree copy.
- Preserve the current cap and all existing assertions; use measured before/after CI evidence to justify safe routing changes.

</specifics>

<deferred>
## Deferred Ideas

- Encrypted PGD, multi-disc switching, audio compression, and undocumented PSISO layouts stay unsupported.
- A generic `pins.txt` schema-gate rebind is deferred because its recorded first-row trigger was already crossed; handle it only with new scope evidence.
- Any broader Main/Release artifact handoff stays deferred pending a tested producer/consumer design and measured benefit.
- ECM/CloneCD, SBI/LibCrypt, profiles, peripherals, patch/cheat support, and milestone-wide DAT claims remain in their mapped later phases.

</deferred>

---

*Phase: 10-pbp-disc-images*
*Context gathered: 2026-11-20*
