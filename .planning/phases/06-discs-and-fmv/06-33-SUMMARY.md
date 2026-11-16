---
phase: 06-discs-and-fmv
plan: "33"
subsystem: cdrom-audio
tags: [xa, cdda, spu, state, pcm]
requires:
  - phase: 06-09
    provides: XA ADPCM decoding and integer resampling with serialized history
  - phase: 06-10
    provides: Native CDDA queue and SPU sample-tick integration
provides:
  - Accepted XA sector decode into a bounded native audio queue
  - Serialized XA queue state in format and behavior revision 14
  - Exact scheduled cadence and equal-deadline packet capacity controls
affects: [06-34, 06-16, DISC-02]
actuals:
  tokens: 10361.5
  tasks: 1
  commits: 2
  plan_head_before: 768d997fad311d5514492fdeb9b39cc33c2b6d43
  plan_head_after: e1ef7c79391840dd9b349723f5d76b35cea52e05
tech-stack:
  added: []
  patterns:
    - Keep XA packet storage separate from the smaller bounded CDDA queue.
    - Keep decoder output scratch in instance storage, off the sector-event stack.
key-files:
  created:
    - .planning/phases/06-discs-and-fmv/06-34-PLAN.md
  modified:
    - src/cdrom_sector.c
    - src/cdrom_audio.c
    - src/cdrom_state.c
    - src/cdrom.h
    - src/internal.h
    - src/spu.c
    - tests/cdrom/audio.c
    - tests/core/test_state.c
    - docs/embedding.md
    - docs/hardware/cdrom.md
    - docs/hardware/spu.md
key-decisions:
  - "Keep XA audio in a separate ring with 9,408 packet frames plus one equal-deadline reserve pair; the existing CDDA queue remains at 1,176 pairs."
  - "Preserve XA queue contents and cursors in state format 14 and keep decode scratch instance-owned."
patterns-established:
  - "Drain one CDDA and one XA pair at the native SPU tick, sum in signed 32-bit space, then capture and mix."
requirements-completed: []
coverage:
  - id: D1
    description: Accepted XA sectors reach native PCM through the bounded ring; CDDA remains an independent source.
    requirement: DISC-02
    verification:
      - kind: integration
        ref: ctest --test-dir build/ci -R '^disc\.audio\.xa$|^disc\.audio\.cdda$' --no-tests=error --output-on-failure
        status: pass
    human_judgment: false
  - id: D2
    description: The 37.8-kHz stereo and 18.9-kHz mono interleaves drain at native cadence, and a tied CD-ROM/SPU event accepts the full packet with one older pair pending.
    requirement: DISC-02
    verification:
      - kind: integration
        ref: ctest --test-dir build/ci -R '^disc\.audio\.xa$' --no-tests=error --output-on-failure
        status: pass
    human_judgment: false
  - id: D3
    description: Muted XA continues decoding, queueing and capture; mode-disabled XA falls through to data delivery; reset, lid opening and separate instances preserve their boundaries.
    requirement: DISC-02
    verification:
      - kind: integration
        ref: ctest --test-dir build/ci -R '^disc\.audio\.xa$' --no-tests=error --output-on-failure
        status: pass
    human_judgment: false
  - id: D4
    description: XA queue bytes and cursors round-trip and invalid queue counts are refused under state format 14.
    requirement: DISC-02
    verification:
      - kind: unit
        ref: ctest --test-dir build/ci -R '^core\.state\.' --no-tests=error --output-on-failure
        status: pass
    human_judgment: false
metrics:
  duration: "at least 1h22m00s wall lower bound"
  completed: 2026-11-16
  commits: 2
status: complete
---

# Phase 6 Plan 33: Route decoded XA audio through the native SPU output Summary

**Accepted XA packets now reach the native SPU mixer through serialized bounded storage, with CDDA behavior and its two-sector queue preserved.**

## Performance

- **Runtime/model:** Codex/OpenGSD 1.20.0; executor gpt-6-luna xhigh.
- **Duration:** At least 1h22m00s wall lower bound from the first durable Plan 06-33 RED report (`2026-11-16T06:29:28Z`) through summary preparation (`2026-11-16T07:51:28Z`). The plan began before that RED artifact, so actual wall time is longer; an exact start marker was not captured.
- **Tasks:** 1/1 bounded task complete.
- **Files modified:** 20.
- **Hosted run:** None; local focused checks only.

## Completed work

- Routed accepted XA sectors through the existing decoder/resampler into a separate XA PCM ring. The 9,408-frame maximum packet fits with one reserve pair for the equal-deadline CD-ROM-before-SPU event; the CDDA queue remains capped at 1,176 pairs.
- Mixed one XA and one CDDA sample on each native SPU tick, summing in signed 32-bit space before capture and output processing. Decode scratch is instance-owned to avoid a large sector-event stack buffer.
- Bumped behavior and state format to 14; serialized and validated XA queue contents, head and count. Updated state-version controls and embedding/CD-ROM/SPU documentation.
- Added registered XA controls for native output, exact 1/8 stereo and 1/32 mono cadence, same-deadline full-packet acceptance, mute progression, mode-disabled data fallback, reset/lid clearing and instance isolation.
- Kept aggregate DISC-02 coverage partial until the follow-up verifies mode-enabled file/channel mismatch and XA/CDDA/voice/reverb mixing.

## Gate Evidence

The RED command was `ctest --test-dir build/ci -R '^disc\.audio\.xa$' --no-tests=error --output-on-failure --output-junit "$PWD/build/ci/06-33-task1-red.xml"`. It exited 8 on the intended native-audio assertion: the scheduled accepted XA sector produced no nonzero host PCM (`nonzero` was 0, expected 1). The target executed and failed on the planned behavior. The persisted record `build/ci/06-33-task1-red-evidence.json` was classified `RED_EVIDENCE_OK`.

The final focused command was:

```sh
cmake --preset ci && cmake --build --preset ci --target schwaystation-test-cdrom-audio schwaystation-test-spu schwaystation-test-state && ctest --test-dir build/ci -R '^disc\.audio|^spu\.model$|^core\.state\.' --no-tests=error --output-on-failure
```

Exit status: 0. All 12 selected tests passed: the three registered CD audio selectors, `spu.model`, and eight core state tests. `git diff --check` and the sorted CD-ROM inventory check passed.

## Task Commits

1. **Task 1 RED:** `1fcaec1` (`test(06-33): register failing XA audio output control`).
2. **Task 1 implementation:** `e1ef7c7` (`feat(06-33): route XA audio into native SPU output`). This also records the dependent Plan 06-34 split and keeps Plan 06-12 independent of XA audio.

## Deviations from Plan

### Correctness adjustment

The XA decoder can produce a 9,408-frame packet, while the established CDDA queue is limited to 1,176 pairs. A separate maximum-packet XA ring prevents valid packets from being truncated. The equal-deadline test establishes one additional reserve pair because the CD-ROM sector event runs before the same-deadline SPU sample event. State format and behavior revision were advanced to 14 for the new serialized queue.

### Bounded follow-up split

The remaining mode-enabled file/channel mismatch and XA/CDDA/voice/reverb combined controls were moved to Plan 06-34 at the 30-minute execution bound. Plan 06-16 now depends on both audio plans. Plan 06-12's MDEC oracle work remains independent and does not depend on XA audio. No acceptance was removed.

## Next Phase Readiness

The bounded XA route, ring state and cadence controls are complete. Plan 06-34 is next for the deferred mismatch and mixed-source controls. DISC-02 remains pending aggregate Phase 6 acceptance and the owned-disc audio caller.

## Self-Check: PASSED

- Summary file exists.
- Dependent Plan 06-34 file exists.
- RED commit `1fcaec1` and implementation commit `e1ef7c7` are ancestors of the current branch.
- `git diff --check` passes.

---
*Phase: 06-discs-and-fmv*
*Completed: 2026-11-16*
