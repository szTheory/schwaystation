# Phase 1: A test frame in RetroArch - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-10-19
**Phase:** 01-a-test-frame-in-retroarch
**Areas discussed:** Test card content, Phase-1 API subset, Seed porting strategy
**Mode:** advisor (calibration `minimal_decisive`); one researcher per area under the owner's fan-out brief.

---

## Test card content

| Option | Description | Selected |
|--------|-------------|----------|
| Static pure-function card | Sibling precedent; hash never moves; an off-by-one frame and a broken state load would pass | |
| Pure-function card with a 32-bit frame-counter strip, redrawn every frame | Proves screenshot frame alignment, gives the state round trip real content, keeps the Ir/frame fixture nonzero | ✓ |

**User's choice:** the recommendation (create context).
**Notes:** Converter is bit replication `(c<<3)|(c>>2)` with bit 15 masked; a known-answer test breaks the circularity of a shared converter.

---

## Phase-1 API subset

| Option | Description | Selected |
|--------|-------------|----------|
| Consumer-minimal v1, appended per phase | 14 functions, each with a phase-1 caller; DEC.75 | ✓ |
| Whole v1.0 surface now with `ERR_UNSUPPORTED` stubs | Freezes ~25 signatures early; stub tests | |

**User's choice:** the recommendation.
**Notes:** Phase-1 audio is real silence at ticks/768 pairs per frame; the card lives in an internal VRAM array.

---

## Seed porting strategy

| Option | Description | Selected |
|--------|-------------|----------|
| Copy and rename from cogmaster's released phase 1, muhtari only for the OBJECT-library pattern | CI-proven skeleton; prune gates to the 1,500-line budget | ✓ |
| Re-derive every mechanism from the preparation pack | Discards sibling-proven release and e2e fixes | |

**User's choice:** the recommendation.
**Notes:** The researcher found cogmaster's phase 1 merged publicly (`ea3bd22e4280`); the orchestrator confirmed the `v0.1.0` release commit `f09ac97096ac` exists and differs only in version files, and pinned that.

---

## Claude's Discretion

- Visitor field path for the counter; exact OBJECT-library boundaries.
- Settled from the pack without asking: REQUIREMENTS over the ARCHITECTURE P1 row on scope; `tests/inventory/ci.txt` as the directory selection with per-directory inventories.

## Deferred Ideas

- Unnamed gates become one todo each (CONTEXT D-25); the `ci:` tail items stay in the tail step.
