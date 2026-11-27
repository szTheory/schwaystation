# Phase 14: Multitap and special controllers - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-11-23
**Phase:** 14-multitap-and-special-controllers
**Areas discussed:** Public input compatibility, multitap cards and saves, controller protocols and host mapping, verification and iteration

---

## Public input compatibility

| Option | Description | Selected |
|--------|-------------|----------|
| Append API v9 fields | Preserve the 28-byte v8 prefix, add fields for extended controllers, and safely default absent trailing fields. | ✓ |
| Replace the input layout | Redesign all input fields around eight controller slots in one change. This would require callers to migrate and would invalidate the released layout. | |

**Selection:** Recommended additive API v9 path accepted under the owner's standing autonomous authorization.
**Notes:** The current setter copies the full struct; the extension must first copy only bytes covered by the caller's size tag.

---

## Multitap cards and saves

| Option | Description | Selected |
|--------|-------------|----------|
| Support all eight cards | Model four card sockets per tap on both ports; add new stable public identifiers and accessors while preserving the existing card 1/2 meanings. Persist each card separately. | ✓ |
| Support only one card per tap | Keep the existing two-card API and omit independent persistence for the other six sockets. This would leave SIO-01 incomplete. | |
| Renumber all cards 1–8 | Flatten sockets into one numbering scheme, changing the existing meaning of card 2. This would break released callers. | |

**Selection:** Recommended full eight-card path accepted under the owner's standing autonomous authorization.
**Notes:** Persistent card bytes and generation counters remain outside save states; loading a state restores only in-flight card protocol state.

---

## Controller protocols and host mapping

| Option | Description | Selected |
|--------|-------------|----------|
| Implement documented serial modes | Use the pinned IDs and payloads, frame-latched integer inputs, official libretro device identifiers, and explicit refusal for unsupported modes. | ✓ |
| Infer missing Jogcon behavior | Add plausible but uncited serial and force-feedback behavior. This would create an unsupported compatibility claim. | |

**Selection:** Keep implementation within documented protocol evidence; report unsupported modes explicitly.
**Notes:** The host reference supplies raw axes; response curves and deadzones stay host-side. Dual Analog flight mode uses the documented analog payload.

---

## Verification and iteration

| Option | Description | Selected |
|--------|-------------|----------|
| Direct focused vectors | Add short SIO/card and libretro cases, sort inventories, configure first, then run targeted selectors and required full gates. | ✓ |
| Reuse long guest/movie tests for basic inputs | Add serial/mapping assertions to already long host and movie scenarios, increasing iteration cost without adding matching coverage. | |
| Add `act` and workflow routing checks | Run them despite no workflow change; they would duplicate local checks without adding coverage. | |

**Selection:** Reuse focused suites and cheap configure preflight; preserve hosted coverage.
**Notes:** Do not change test parallelism or remove required gates without measured evidence.

---

## Decided autonomously

[auto] [Protocol coverage] — Q: "Which tap access paths and empty-slot cases are in scope?" → Selected: "Both controller poll methods, card addressing, both physical ports, empty tap/slot behavior, later-slot padding, ACK timing and no-ACK final bytes." (reversible technical — these cases are specified by the pinned hardware reference and are cheap to verify with short vectors)

[auto] [Jogcon boundary] — Q: "How much undocumented Jogcon behavior should be modeled?" → Selected: "Limit behavior to documented integer controls and explicitly refuse unsupported modes; do not infer force feedback." (reversible technical — a narrow implementation can be extended when primary evidence appears)

[auto] [Test placement] — Q: "Where should controller checks run?" → Selected: "Extend the existing SIO/card and libretro inventories with direct protocol/mapping tests." (reversible technical — existing focused test suites provide the same coverage at lower iteration cost)

## the agent's Discretion

- Internal enum names, exact per-socket sidecar suffixes, compact integer input fields and focused task boundaries, within the contracts recorded in CONTEXT.md.

## Deferred Ideas

- Light guns remain in Phase 15; SIO1 link support, PS2 peripherals, PocketStation execution and undocumented Jogcon force feedback remain out of scope.
- No CI workflow or matrix change is justified by this phase's current evidence.
