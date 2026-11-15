---
phase: 05-the-sound-matches-the-console
plan: "19"
subsystem: SPU documentation and evidence
tags:
  - SPU
  - hardware-docs
  - PCM
  - provenance
requires:
  - phase: 05-the-sound-matches-the-console
    provides:
      - Implemented SPU voice, noise, DMA4, capture, reverb, state and PCM delivery paths
provides:
  - Maintained SPU hardware contract aligned with implemented behavior and pinned sources
  - Separate descriptions of hardware observations, model regressions and host-delivery checks
affects:
  - Phase 5 final review and verification
  - SPU documentation and later audio evidence updates
actuals:
  tasks: 2
  commits: 2
tech-stack:
  added: []
  patterns:
    - Keep source-backed SPU facts and current model behavior in the maintained hardware guide.
    - Describe each audio check by the claim it establishes and the boundary it does not establish.
key-files:
  created:
    - .planning/phases/05-the-sound-matches-the-console/05-19-SUMMARY.md
  modified:
    - docs/hardware/spu.md
    - .planning/preparation/PSX-SPU-CDROM-AND-MDEC.md
    - .planning/preparation/ARCHITECTURE.md
    - docs/testing.md
    - PROVENANCE.md
    - src/spu.c
    - src/spu_envelope.c
    - src/spu_reverb.c
    - src/spu_voice.c
key-decisions:
  - Keep the maintained hardware guide as the current implementation contract and reduce superseded SPU preparation detail to source-linked pointers.
  - Keep transfer hardware evidence, measured constraints, deterministic PCM regressions and runner/libretro delivery equality as separate claims.
  - Retain the unresolved ADSR edge behavior and exact per-stage reverb rounding; do not add Phase 6 CD audio behavior or profile switches.
requirements-completed: []
coverage:
  - id: D1
    description: The SPU guide and preparation references describe the implemented registers, sample order, DMA4, state ownership, fixed-point voice/reverb paths and unresolved precision points with exact source pins.
    requirement: SOUND-01
    verification:
      kind: other
      ref: "cmake --workflow --preset hygiene (exit 0; 23/23 passed)"
      status: pass
    human_judgment: false
  - id: D2
    description: Testing and provenance docs distinguish the seven hardware transfer cases, pinned noise/reverb evidence, authored digests and CPU-free replay, runner/libretro PCM equality, and video-only RetroArch smoke.
    requirement: SOUND-02
    verification:
      kind: other
      ref: "cmake --workflow --preset hygiene (exit 0; 23/23 passed)"
      status: pass
    human_judgment: false
duration: "about 30 minutes wall across restart; exact active time unavailable"
completed: 2026-11-14
status: complete
---

# Phase 05 Plan 19: SPU contract and evidence boundaries Summary

**The maintained SPU guide now matches the implemented sound path, while testing and provenance docs distinguish hardware evidence from deterministic and host-delivery checks.**

## Performance

- **Duration:** About 30 minutes wall across restart; exact active time unavailable.
- **Started:** 2026-11-14T10:10Z (estimated from the prior session handoff).
- **Completed:** 2026-11-14T10:40:49Z.
- **Tasks:** 2.
- **Files modified:** 9.
- **Runtime/model:** Codex, OpenGSD 1.21.0; GPT-6 session (exact model ID not exposed).

## Accomplishments

- Rewrote the SPU guide around implemented behavior, including sample event order, active DMA4/FIFO handling, capture/IRQA, state ownership, native PCM delivery and the unresolved ADSR/reverb details.
- Reduced repeated SPU research notes to pointers while retaining the primary source IDs, exact noise/reverb commit pins and open hardware questions; corrected architecture references that still described a live CD audio queue or push-time audio hash.
- Added an evidence table that separates the seven named transfer cases, noise/reverb source facts and measurements, authored PCM and impulse digests, CPU-free fixture equality, runner/libretro delivery and video-only RetroArch smoke.
- Recorded Gaussian, BRR predictor and reverb FIR factual data in provenance, with exact source pins and explicit confirmation that no ares implementation code or table was copied.

## Task Commits

1. **Trace implemented SPU facts into maintained hardware references** — `0175d1f` (`docs(05-19): align SPU docs with implementation`).
2. **Expand testing and provenance documentation without conflating evidence** — `5a4c7b0` (`docs(05-19): clarify SPU test evidence`).

## Checks Run

- `cmake --workflow --preset hygiene` — exit 0, 23/23 passed after the final task commit.
- The initial hygiene run exited 8 because `hygiene.no_float` interpreted section numbers and a decimal notation in existing SPU comments as floating-point literals. The affected source comments were rewritten as document citations/words without changing code behavior; subsequent hygiene runs passed.
- The recurring hosted RetroArch job and its exact PR-head result remain for ship-time confirmation. No hosted run URL is claimed here.

## Files Created/Modified

- `docs/hardware/spu.md` — current implementation contract and evidence limits.
- `.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md` — concise SPU pointers, preserved source pins and unresolved questions; Phase 6 audio marked as planned.
- `.planning/preparation/ARCHITECTURE.md` — corrected Phase 5 audio, state, dependency and hashing descriptions.
- `docs/testing.md` — SPU evidence matrix and hosted smoke boundary.
- `PROVENANCE.md` — imported factual data and permissive-code distinctions.
- `src/spu.c`, `src/spu_envelope.c`, `src/spu_reverb.c`, `src/spu_voice.c` — comment-only hardware citations/notation updates required for the integer-only hygiene scan.

## Deviations

- Fixed a false-positive `hygiene.no_float` failure by replacing numeric section citations and a decimal phrase in four source comments with document citations and words. No executable code changed. The full hygiene workflow passed after the fix and after both task commits.

## Issues Encountered

- No remaining issues. The implementation has no full silicon PCM waveform oracle, and the documentation keeps that limit explicit.

## Next Phase Readiness

- Plan 05-19 is complete. Plan 05-15 remains; it performs the final code review and required CI wiring before phase verification.
- The required phase boundary is unchanged: finish Phase 5 review and verification, then stop before `$gsd-ship 5`. The exact hosted run head, result and URL are confirmed at ship.

## Self-Check: PASSED

- All plan output files exist, the two task commits are on this branch, both documentation tasks are complete, and the required hygiene workflow exits 0.
- Phase requirements SOUND-01 and SOUND-02 remain pending because Plan 05-15 is the last declaring plan.

---
*Phase: 05-the-sound-matches-the-console*
*Completed: 2026-11-14*
