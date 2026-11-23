# Phase 11: ECM and CloneCD images - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md; this log preserves the alternatives considered.

**Date:** 2026-11-21
**Phase:** 11-ECM and CloneCD images
**Areas discussed:** ECM access and byte mapping, CloneCD descriptor/companion scope, SUB ordering, CI iteration and gate budget

---

## ECM access and byte mapping

| Option | Description | Selected |
|---|---|---|
| Decode to a temporary full raw image | Simple reads but needs host writes, disk space and cleanup; conflicts with hosts without file access and memory limits. | |
| Rescan from the start for each sector | Minimal index memory but repeated reads can make guest execution proportional to the whole file. | |
| Validate once and use a bounded index/checkpoints | Keeps all input validation before guest execution, supports random reads and bounds index/replay work. | ✓ |

**User's choice:** Follow the best-supported recommendation under autonomous authorization.
**Notes:** Keep exact decoded-byte parity as the evidence boundary. ECM type mapping and TOC policy must be grounded in its author spec and owned fixtures; fail closed on unproven layouts.

---

## CloneCD descriptor and companion scope

| Option | Description | Selected |
|---|---|---|
| Accept broad inferred CCD variants | More files may open, but undocumented values and missing indexes can silently shift tracks. | |
| Accept a fixture-proven subset and reject ambiguity | Reconcile track and TOC metadata, preserve explicit indexes and pregaps, and use confined same-stem companions. | ✓ |
| Treat `.img` as one raw track and ignore `.ccd` | Simpler, but loses required track, index, pregap and subcode behavior. | |

**User's choice:** Follow the best-supported recommendation under autonomous authorization.
**Notes:** Never synthesize a default two-second pregap for missing INDEX 0. Validate all companion paths before opening.

---

## SUB ordering

| Option | Description | Selected |
|---|---|---|
| Copy `.sub` bytes directly to the callback | Avoids conversion but conflicts with the project-pinned reference's channel-major description and the callback's bit-interleaved representation. | |
| Convert channel-major P–W groups to callback layout | Matches the project-pinned PSX reference and consumer representation; requires an independent known-answer vector. | ✓ |
| Omit SUB support | Avoids conversion uncertainty but loses the required optional subcode behavior. | |

**User's choice:** Follow the best-supported documented format and prove it with a known-answer test.
**Notes:** General CloneCD documentation is incomplete; retain the source confidence boundary.

---

## CI iteration and gate budget

| Option | Description | Selected |
|---|---|---|
| Keep current broad Nightly trigger | Safe but wastes a measured run for release-metadata-only PRs. | |
| Skip PR Nightly only for the exact complete generated-metadata file set | Addresses a reproduced false positive while every mixed, incomplete or unknown file set retains full Nightly. | ✓ |
| Broaden file exclusions or remove release matrix jobs | Could hide regressions or weaken required coverage; no evidence supports it. | |

**User's choice:** Follow the narrow exact-set recommendation. The owner separately selected the existing 3,901-line cap and explicitly deferred the generic SPDX scanner.
**Notes:** Main CI and Release CI remain separate because their 18 product archives differ. Keep scheduled/manual Nightly and all test/platform cases unchanged.

---

## the agent's Discretion

- Select a bounded index representation, fixture format, parser helper boundaries, and precise supported fields
  within the acceptance criteria. Resolve routine mapping questions using sources and owned vectors.
- Add permanent preflights only when they guard a recurring failure. Reuse the existing sorted inventory,
  path checks, route checks and budget gate.
- Keep code review before final verification, and batch related repairs before one changed-head hosted run.

## Execution re-review: Nightly route budget (2026-11-22)

New cap evidence made the originally selected D-08 route infeasible within the locked repository-check ceiling.
The existing counted set is 3,898/3,901 lines, leaving three lines; the route-control additions alone were
measured at 30 lines, for 3,928 before adding classifier or workflow-boundary controls. Two independent
read-only audits found no safe existing assertion to remove and no measured 27-line consolidation that retained
all cases. The parent reviewed the alternate options adversarially: raising the cap violates D-09; deleting or
relaxing controls weakens the fail-closed boundary; minifying or moving gate code violates the project policy;
the existing full-Nightly fallback is safe and keeps all coverage.

**Decision D-12:** Defer only the generated-release-metadata Nightly shortcut to existing pending todo 051.
Keep all current routes, cases, platform jobs and Main/Release separation unchanged. Plan 11-02 now records this
evidence and verifies the existing classifier behavior; this does not complete or waive ECM-01 or CCD-01.

## Deferred Ideas

- Generic SPDX scanner; broader CI/CD redesign; undocumented CCD variants; any ECM sector layout without proven
  byte parity.
