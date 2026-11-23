# Phase 13: Regions, console revisions and playback - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-11-23
**Phase:** 13-regions-console-revisions-and-playback
**Areas discussed:** machine-profile identity and compatibility tuples, BIOS-shell shortcut, bounded FMV evidence and CI iteration

---

## Machine-profile identity and compatibility tuples

| Option | Description | Selected |
|--------|-------------|----------|
| Keep region-only profiles and infer board from title/BIOS | Minimal public API change, but conflicts with DEC.04 and would conflate title identity with console hardware. | |
| Append board identity, derive documented hardware defaults, validate legal GPU pairs, and keep region separate | Matches DEC.04/DEC.17; preserves title ledger identity; uses the existing feature `profile` key for exercised tuples. | ✓ |
| Redesign compatibility data with separate board/GPU columns or a new tuple file | Improves independent queries but expands ratchet, fixture and report migration without a current consumer need. | |

**Auto-selected:** Append board identity; default new profiles to PU-18, preserve old size-tagged callers, and encode new feature profiles as `<region>.<board>.<gpu>`.
**Notes:** The profile/product and architecture advisors independently favored the existing feature-ledger field. The CI/evidence review highlighted the title ledger's region-only field; the adversarial synthesis kept it unchanged because title identity and feature predicates are distinct ledgers. Validate the exact supported tuple set against PSX-SYSTEM §8 and GPU §1.1 before planning.

## BIOS-shell shortcut

| Option | Description | Selected |
|--------|-------------|----------|
| Make the shortcut default or synthesize BIOS behavior | Faster startup, but changes ordinary boot and violates the no-HLE/user-BIOS contract. | |
| Keep full boot default; opt in to a BIOS-backed hook at the documented shell entry | Preserves the normal path and kernel initialization while skipping only the eligible shell handoff. | ✓ |
| Apply the shortcut to standalone PS-EXE stub loading | Reuses an existing executable path, but does not exercise the BIOS shell handoff. | |

**Auto-selected:** Full BIOS boot remains default; `fast` requires supplied BIOS and eligible disc executable; standalone EXE remains on the existing stub path.
**Notes:** The decision follows the pinned system boot notes and host integration contract. Invalid combinations must fail before guest execution.

## Bounded FMV evidence and CI iteration

| Option | Description | Selected |
|--------|-------------|----------|
| Treat complete source coverage or decoder progress as full FMV conformance | Collapses distinct evidence and repeats the earlier source/output reporting mistake. | |
| Keep separate source, decoder, guest, display and fidelity results; add focused bounded regressions | Preserves the v1.0 scope and makes the first failed layer visible before expensive runs. | ✓ |
| Expand Phase 13 into full movie/frame hardware conformance | Requires evidence explicitly outside the v2.0 milestone boundary. | |

**Auto-selected:** Keep the 4-bit oracle, 8-bit deterministic baseline and 4,915,200-cycle cap; run the existing source-unit preflight before guests and add no full-conformance claim.
**Notes:** CI/evidence advisor recommended cheap preflights and preserving hosted-only platform/artifact checks. The current 29 todo matcher results all scored 0.6 with generic reasons; only plausible runner/GPU/CD/CI items were checked and all remain bound outside Phase 13.

## Trade-off synthesis and adversarial pass

- **Product:** `auto` region selection follows the title while board selection follows the emulated console; explicit overrides remain available. A compatibility row only describes the predicate its fixture actually runs.
- **Security:** Parse and validate region/board/boot inputs on the host boundary, reject illegal pairs and malformed option values before guest execution, and keep the core free of file, environment and wall-clock reads.
- **Architecture:** Board identity is the source for documented defaults; serialize and hash behavior-affecting values. Preserve legacy struct sizes and the existing GPU flag while validating explicit new tuples.
- **DevOps/CI:** Cheap selector, input, tuple-count, sorted-inventory and budget checks precede long suites. Keep required matrices and inspect measured CI/CD timings; use `act` only when it adds local evidence.
- **PS1 hardware:** Use the nine documented board identities and v0/v2 behavior distinctions; do not infer console board from game serial. Region controls its own timing crystal and CD region behavior.
- **MDEC:** Source completeness, produced blocks, guest completion, display comparison and fidelity remain separate. The cap and accepted output baselines do not move.
- **C/toolchain:** Use integer-only, bounded, size-tagged API changes and assert cross-platform deterministic state/hash behavior.
- **Adversarial checks:** The feature ledger accepts dots, hyphens and plus but not underscores, so encode underscore board names with hyphens and test canonical values. PM-41(2) uses the CXD9500 but is represented by v2 behavior. A legacy GPU-v0 profile must not force an invalid explicit board pair. `fast` must not bypass BIOS initialization or apply to the separate stub EXE route. Never describe complete MDEC input as complete decoder or display output.

## the agent's Discretion

- Internal enums, lookup tables, host option plumbing, bounded owned fixture construction, and the exact legacy-size compatibility representation are for the planner/executor, within the locked API and behavior boundaries.

## Reviewed Todos

- TODO 047 remains attached to Phase 6's CD-ROM DMA delay behavior.
- TODO 048 remains a GPU test-maintenance seed, not a Phase 13 requirement.
- TODO 049 remains attached to Phase 7's runner/checkpoint CI review.
- TODO 051 remains attached to Phase 10's Nightly routing.

## Deferred Ideas

- Exact 8-bit MDEC hardware output and full external frame/movie conformance remain outside v2.0.

