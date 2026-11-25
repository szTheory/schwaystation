# Phase 16: Patches, cheats and compatibility evidence - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-11-25
**Phase:** 16-Patches, cheats and compatibility evidence
**Areas discussed:** patch identity and media mapping, cheat subset and serialization, compatibility accounting, experimental metadata, CI follow-up

---

[--auto] Selected all gray areas: patch identity and media mapping, cheat subset and serialization, compatibility accounting, experimental metadata, CI follow-up.

## Patch identity and media mapping

**Question:** How should supported patch formats identify their source and map into disc sectors?

| Option | Description | Selected |
|--------|-------------|----------|
| Trust format labels and whatever partial checks are present; accept ambiguous layouts | Smallest implementation, but can apply a patch to the wrong image or sector offsets | |
| Require explicit PPF1 source SHA-256, use native PPF2/3 checks and BPS source/target checks, and reject layouts without exact raw-sector mapping | Identity and sector mapping are checked before the staged result becomes visible | ✓ |
| Exclude PPF1 because it has no embedded source identity | Safer but falls short of the required PPF 1/2/3 family | |

**Agent selection:** Recommended option selected under the owner's standing autonomous authorization.
**Notes:** The primary PPF3 spec confirms there is no image-size field and its 1,024-byte block check is optional. PPF2 has the source-size and block checks. The implementation requires caller SHA-256 for every PPF version and applies all native checks; PPF block checks and BPS CRCs are not cryptographic identity. BPS transforms a byte stream; PPF is represented as ordered sparse records.

---

## Cheat subset and serialization

**Question:** Which GameShark codes and host state should Phase 16 support?

| Option | Description | Selected |
|--------|-------------|----------|
| Implement a broad set including conditional and stateful codes | More code coverage, with more ambiguous comparison and timing semantics | |
| Implement bounded unconditional `30` byte writes and `80` halfword writes; serialize/hash normalized ordered enabled records | Small deterministic subset using one checked RAM-write operation | ✓ |
| Leave cheat callbacks as no-ops and defer implementation | Avoids adding state/API behavior, but does not meet CHEAT-01 | |

**Agent selection:** Recommended option selected under the owner's standing autonomous authorization.
**Notes:** Malformed replacement input must preserve both prior configuration and memory. Disabling a code affects subsequent frames only.

---

## Compatibility accounting

**Question:** How should the pinned Redump inventory be represented?

| Option | Description | Selected |
|--------|-------------|----------|
| Commit and publish all source DAT rows | Makes each title visible but violates the project's no-bulk-redistribution policy | |
| Fetch the pinned DAT locally, account for every source entry in aggregate totals, and keep only allowed sparse evidence rows | Full accounting without distributing the bulk title dataset | ✓ |
| Report only the owned fixture titles | Avoids bulk data but does not account for the required source inventory | |

**Agent selection:** Recommended option selected under the owner's standing autonomous authorization.
**Notes:** The public report must distinguish inventory from playability and retain explicit unknowns.

---

## Experimental metadata

**Question:** What evidence can turn off `is_experimental`?

| Option | Description | Selected |
|--------|-------------|----------|
| Reuse profile-selection and protocol rows already in the ledger | Easier, but those rows do not establish required boot-to-play behavior | |
| Require fresh results for every committed required row and fail closed on missing, stale, duplicate, unknown or failing evidence | Keeps the metadata claim tied to measured required capabilities | ✓ |
| Keep the metadata experimental permanently | Safe if the matrix is incomplete, but does not evaluate the requirement's release condition | |

**Agent selection:** Recommended option selected under the owner's standing autonomous authorization.
**Notes:** DAT-wide playability is explicitly outside the claim. The metadata stays experimental whenever any required evidence is absent or fails. Main CI must protect `features.csv` in its sparse checkout and workflow-consistency assertion.

---

## CI follow-up

**Question:** How should the pending BE32 shard idea be treated?

| Option | Description | Selected |
|--------|-------------|----------|
| Split only `libretro.input.movies` based on the original measurement | Ignores the measured long `libretro.host` runtime | |
| Update the candidate to cover both long tests and require exact coverage plus measured critical-path gain before implementing a shard | Tests the complete candidate while preserving every gate | ✓ |
| Drop the follow-up without measuring it | Leaves a known candidate unresolved | |

**Agent selection:** Recommended option selected under the owner's standing autonomous authorization.
**Notes:** TODO052 remains pending without proof that splitting saves meaningful wall time after duplicate setup.

---

## the agent's Discretion

CLI option names, module decomposition, implementation limits within the preparation bounds, temporary storage mechanics, output formatting, fixtures, and exact test-file allocation remain implementation choices. These do not change supported semantics, required evidence, or the fail-closed claim policy.

## Deferred Ideas

No new future-phase feature was added. Unsupported GameShark types and ambiguous media mappings remain unsupported unless separately specified and proven.
