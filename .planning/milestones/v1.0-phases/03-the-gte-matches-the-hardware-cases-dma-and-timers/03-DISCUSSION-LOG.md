# Phase 3: The GTE matches the hardware cases; DMA and timers - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md; this log preserves the alternatives considered.

**Date:** 2026-11-09
**Phase:** 3-The GTE matches the hardware cases; DMA and timers
**Areas discussed:** GTE core and vectors, DMA, root counters, integration/API/state, fixtures and CI, gates/docs/provenance

**Method:** the owner's standing fan-out instruction (`.planning/AUTONOMOUS.md`) replaced interactive questions.
Six advisor agents ran in parallel, one per area, each through every role lens with an adversarial pass; the
recommendations were synthesized and locked without asking. Conflicts between reports are resolved in CONTEXT.md
D-30a to D-37a.

---

## GTE implementation source (A)

| Option | Description | Selected |
|--------|-------------|----------|
| ISC seed from ares, ported to integer C | Passed 1150/1150 in the advisor's port; reuse over rewrite | ✓ |
| Write fresh from psx-spx | Independent, but re-derives what a permissive seed already gives | |

**Notes:** independence is kept in `tests/gte/ref.c`, written from psx-spx and the vectors without the seed.

## DMA scheduling and stall (B, E)

| Option | Description | Selected |
|--------|-------------|----------|
| Data-atomic units, time-evented, freeze-until stall | Keeps the one-instruction overrun bound | ✓ |
| One-instruction stall charge with a word cap | Breaks the 1,024 margin for a 64K-word OTC | |
| Run to completion per CHCR write | 65,535 x 65,535 words is a hang; IRQ timing wrong | |
| Per-word interleave | 33.8M events a second | |

**Notes:** the 1000h guard is a yield (hardware `chain-looping` never finishes), not an abort.

## Root counters (C)

| Option | Description | Selected |
|--------|-------------|----------|
| Lazy closed-form catch-up plus derived deadline | O(1), bounded, no hot-loop cost | ✓ |
| Per-step ticking | Three counters of work per cycle | |
| Full event queue | Three devices do not need one | |

**Notes:** a hardware NTSC timer row was rejected: the log's 71,4xx sysclk/8 frame is a PAL-crystal console forced
to NTSC, 0.9% off our frame. All P3 timer rows are `doc`.

## Integration and state (E)

| Option | Description | Selected |
|--------|-------------|----------|
| Derived `next_deadline`, normalized timer view in the state, one format bump | Hash independent of catch-up cadence | ✓ |
| Stored deadline and `last_tick` | Hostile state becomes an event storm; hash depends on cadence | |

**Notes:** one additive public change (`SCHWAY_FAULT_DMA`, API 3) and one runner flag (`--hash-state`).

## Fixtures and CI (D)

| Option | Description | Selected |
|--------|-------------|----------|
| 30 verbatim table cases, no invented corners, `hw` only from MIT hardware artefacts | Honest provenance | ✓ |
| Eight extra corner cases labelled `hw` | No hardware source | |

**Notes:** no new CI job; armips unity build is the measured speed-up (51.2 to 17.5 CPU-s).

## Gates, docs, provenance (F)

| Option | Description | Selected |
|--------|-------------|----------|
| One `policy.notices` gate of at most 30 lines; ratchet 1520 to 1483 | Names the fault; todo 005 trimmed | ✓ |
| Full 57-line notices seed port | No phase-3 caller for the SPDX and `.info` checks | |

---

## Claude's Discretion

Container layout of `gte_cases.inc`, case selection, state-struct field names, DMA unit enum, plan order in a wave.

## Deferred Ideas

Dot-clock and hblank records (phase 4), nugget and ps1-tests nightly lanes, GTE write-to-use latency, PGXP,
CPU-bound perf fixture, DICR bit 15 and bit 6, SPDX scan and `.info` check.
