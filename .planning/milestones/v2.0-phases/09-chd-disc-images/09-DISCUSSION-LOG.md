# Phase 9: CHD disc images - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-11-19
**Phase:** 09-chd-disc-images
**Areas discussed:** CHD track and sector projection, fixture generation and cheap preflight, release packages/notices/compatibility ledger/CI efficiency

---

## CHD track and sector projection

| Option | Description | Selected |
|--------|-------------|----------|
| Parity-first supported subset | Map only layouts that fit the existing raw-sector, subcode, and two-index TOC contract byte-for-byte. | ✓ Recommended |
| Broad reconstruction | Synthesize sectors and infer broader gap, index, subcode, or session semantics. | |

**Auto selection:** Parity-first supported subset. Exclude CHD alignment padding from logical LBAs; distinguish stored and virtual pregaps from metadata; reject unrepresented semantics until covered.
**Notes:** The public disc callback and docs define the current contract. Advisor review found broad reconstruction adds silent byte-mismatch and off-by-one risks without a current requirement.

## Fixture generation and cheap preflight

| Option | Description | Selected |
|--------|-------------|----------|
| Audited pinned permissive generator | Generate a run-local CHD from owned CUE/BIN only after the complete pinned source and dependency closure passes license review. | ✓ Recommended |
| Opaque CHD blob | Commit or fetch a prebuilt CHD fixture. | |

**Auto selection:** Audited pinned permissive generator plus cheap structural and boundary checks before guest boot.
**Notes:** The pinned libchdr is a reader, not a generator. The existing owned tracks total 522,144 bytes, close to the cap; the generated CHD needs its own size check. MAME `chdman` entry files are BSD-3-Clause but its full closure is not yet cleared. Fixture security review confirmed that source completeness, decoder behavior, and guest completion are separate evidence.

## Release packages, notices, compatibility ledger, and CI efficiency

| Option | Description | Selected |
|--------|-------------|----------|
| CHD-enabled release artifacts; CMake default OFF | Ship player-facing CHD support after parity, retain opt-in builds, and test `.info` and archive notices in both configurations. | ✓ Recommended |
| CHD only in test builds | Keep published artifacts unable to open the newly verified format. | |

| Option | Description | Selected |
|--------|-------------|----------|
| Measure exact-SHA artifact reuse | Keep Main and Release gates until tested identity and runner-minute/critical-path evidence proves safe reuse. | ✓ Recommended |
| Reuse or skip a Release matrix without proof | Reduce work before showing artifact identity or coverage equivalence. | |

**Auto selection:** Package CHD-enabled releases after parity; keep all Main/Release lanes and checks unless measurement and tests prove a safe improvement.
**Notes:** The hosts/CI review found no evidence that the Phase 8 BE32 job durations alone establish total runner-minute or critical-path savings. Todo 005 is triggered by the non-MIT reader's notice requirements; todo 050 fits the new release capability and is folded into this phase.

## the agent's Discretion

- Private code boundaries, test details, supported metadata cases, and generator integration within the license and parity constraints.
- Prefer the smallest proven generator route. If license closure cannot be established, record that blocker and keep unrelated implementation moving.
- Make measured efficiency changes without removing lanes, tests, package checks, or required CI status.

## Deferred Ideas

- Broader CHD reconstruction, postgaps, multisession, and unsupported subcode modes await explicit byte-level coverage.
- Todo 006 remains bound to Phase 10. PBP, ECM/CloneCD, and SBI functionality remain in their roadmap phases.
- Cross-workflow artifact reuse proceeds only if Phase 9 exact-SHA identity testing demonstrates safety and meaningful measured savings.
