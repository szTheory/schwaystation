# Phase 5: The sound matches the console - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in 05-CONTEXT.md; this log preserves the alternatives considered.

**Date:** 2026-11-13
**Phase:** 5-the-sound-matches-the-console
**Areas discussed:** SPU timing and DSP fidelity, PCM delivery and AV rate, Conformance evidence and oracle boundaries

---

## SPU timing and DSP fidelity

| Option | Description | Selected |
|--------|-------------|----------|
| One tick per 768 cycles | Matches the settled hardware sample period and keeps work bounded per sample. | ✓ |
| Eight-cycle RAM-slot model | Could add sub-sample phase detail, but current sources do not establish its starting phase and no pinned test requires it. | |

**User's choice:** Follow the existing sample-tick recommendation, with updates for newer primary noise and reverb evidence.
**Notes:** Three role-focused reviews found no contradictory evidence for DEC.36. The new psx-spx noise-clock update and SCPH-1001 reverb measurements were independently checked through the public GitHub API. Keep unverified ADSR edge quirks uncertain.

---

## PCM delivery and AV rate

| Option | Description | Selected |
|--------|-------------|----------|
| Keep the native PCM and current emulated-rate contract | int16 stereo at 44.1 kHz, exact 768-cycle samples with carried remainder, hash before host resampling, and unchanged emulated AV rate. | ✓ |
| Report an interlaced field rate | Change frame timing and sample accumulation with AV metadata if future evidence supports it. | |

**User's choice:** Preserve the current rate and PCM contract during Phase 5; do not make a metadata-only rate change.
**Notes:** The existing adapter already prefers batch delivery and supports single-sample fallback. A future interlace-rate change must update frame cadence, PCM counts, AV info, and host tests together.

---

## Conformance evidence and oracle boundaries

| Option | Description | Selected |
|--------|-------------|----------|
| Keep required gates and add checks for measured hardware constraints | Retain SOUND-01/02 and distinguish deterministic hashes, adapter equality, hardware transfer logs, and measured reverb/noise facts. | ✓ |
| Require a captured hardware PCM waveform | Directly validates waveforms only if a reproducible permitted digital capture already exists. None was found in the pinned SPU test tree. | |

**User's choice:** Keep the existing gate set and add plan-level checks for the newer primary noise and reverb facts, without claiming a complete silicon PCM oracle.
**Notes:** The pinned `spu/memory-transfer` log supplies seven hardware pass lines. Cross-lane hashes prove determinism; `libretro.host` proves delivery. Self-authored audio digests remain regression checks.

---

## The agent's Discretion

Private struct fields, helper boundaries, test file split, and plan waves remain implementation choices so long as source provenance, fixed-width state, bounded behavior, and all required checks are preserved. No additional dependency is recommended.

## Deferred Ideas

- CD-ROM, XA, CD-DA, and disc-driven audio remain in Phase 6.
- Revisit interlaced AV rates or remaining reverb/ADSR unknowns only with new evidence; no owner hardware capture is a Phase 5 gate.
- The todo matcher returned general keyword matches; none had an audio-scoped first caller in Phase 5, so no todo was folded.
