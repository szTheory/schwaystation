---
phase: 06-discs-and-fmv
plan: "10"
subsystem: cdrom-audio
tags: [cdda, atv, spu, pcm, state]
dependency_graph:
  requires: [06-05, 06-09]
  provides: [cdda-native-mixer, cdda-bounded-queue]
  affects: [06-33, 06-12, 06-16]
actuals:
  tokens: 10001.25
  tasks: 1
  commits: 3
  plan_head_before: e81e0b1b18cd1ba90fb79c6cb0ad2e1c3df46efc
  plan_head_after: dfaa5711ad799a41e41b7432710c36c65f3c0453
tech-stack:
  added: []
  patterns: [bounded-cd-audio-queue, native-spu-tick-mix]
key-files:
  created: [src/cdrom_audio.c, .planning/phases/06-discs-and-fmv/06-33-PLAN.md]
  modified: [CMakeLists.txt, cmake/DiscTargets.cmake, .planning/ROADMAP.md, .planning/phases/06-discs-and-fmv/06-10-PLAN.md, .planning/phases/06-discs-and-fmv/06-12-PLAN.md, src/cdrom.c, src/cdrom.h, src/cdrom_sector.c, src/cdrom_state.c, src/internal.h, src/spu.c, src/spu.h, tests/cdrom/audio.c, tests/cdrom/tests.cmake, tests/cdrom/inventory.txt, tests/core/test_state.c, docs/embedding.md, docs/hardware/cdrom.md, docs/hardware/spu.md]
key-decisions:
  - Keep CDDA storage bounded to 1,176 stereo pairs and drop newest pairs on overrun.
  - Apply staged ATV coefficients atomically and serialize pending and active matrices.
  - Split the unstarted XA mixing task into dependent Plan 06-33 at the 30-minute bound.
metrics:
  duration: "at least 49m47s wall lower bound"
  completed_date: 2026-11-16
status: complete
requirements-completed: []
coverage:
  - id: D1
    description: CDDA sectors reach host PCM through the native SPU tick with ATV, AVOL, capture, underflow, and bounded overrun controls.
    requirement: DISC-02
    verification:
      kind: integration
      ref: >-
        cmake --preset ci && cmake --build --preset ci --target schwaystation-test-cdrom-audio schwaystation-test-state && ctest --test-dir build/ci -R '^disc\.audio|^core\.state\.' --no-tests=error --output-on-failure (exit 0, 9/9)
      status: partial
    human_judgment: false
  - id: D2
    description: CDDA source-sector reads and native PCM cadence are independently checked at 1x and 2x.
    requirement: DISC-01
    verification:
      kind: integration
      ref: >-
        ctest --test-dir build/ci -R '^disc\.audio' --no-tests=error --output-on-failure (exit 0, 1/1)
      status: partial
    human_judgment: false
---

# Phase 6 Plan 10: Feed CD-DA through the native SPU output Summary

**CDDA sector samples enter the bounded ATV-routed queue and reach native 44.1-kHz PCM, with state format 13 preserving the matrix and queue.**

Runtime/model: Codex/OpenGSD 1.20.0; executor gpt-6-luna xhigh. Wall lower bound from the durable start marker `2026-11-16T05:32:11Z` to closeout: at least 49m47s. Exact active time was not recorded. The focused review was clear. No hosted run applies.

## Performance

- **Duration:** at least 49m47s wall lower bound
- **Started:** 2026-11-16T05:32:11Z
- **Completed:** 2026-11-16T06:21:58Z
- **Tasks:** 1
- **Files modified:** 21

## Completed work

- Added a fixed two-sector CDDA queue, signed little-endian stereo decode, staged/applied ATV routing, deterministic clipping, underflow silence, and drop-newest overrun behavior.
- Mixed CD input at the existing 768-cycle SPU tick through AVOL, direct/reverb routing, capture, and CD-ROM mute handling. The sector probe checks raw `0x1234/0xFEDC` input and signed `4659/-292` host PCM at both drive speeds.
- Added independent source-LBA/read and native output counters at 1x and 2x; three scheduled 2x source sectors fill the queue and demonstrate later source and explicit samples being dropped.
- Bumped state format and behavior revision to 13, serialized ATV state, tightened audio-count validation to 1,176, and updated state round-trip/reset controls and hardware documentation.
- Extracted the CDDA path into `src/cdrom_audio.c`; `src/cdrom.c` remains at 1,196 lines.

## Gate Evidence

The valid RED command was `ctest --test-dir build/ci -R ^disc\\.audio\\.cdda$ --no-tests=error --output-junit "$PWD/build/ci/06-10-task1-red.xml"`. It exited 8 on the intended queue-consumption assertion (`1 != 0`); the target ran and failed for the planned behavior. The persisted record was classified `RED_EVIDENCE_OK`.

Final focused command:

```sh
cmake --preset ci && cmake --build --preset ci --target schwaystation-test-cdrom-audio schwaystation-test-state && ctest --test-dir build/ci -R '^disc\.audio|^core\.state\.' --no-tests=error --output-on-failure
```

Exit status: 0. All 9 selected tests passed (1 CD audio test and 8 core state tests). `git diff --check` passed, and the production CD-ROM controller source is below its 1,200-line limit.

## Task Commits

1. **Task 1 RED:** `658a4b5` (`test(06-10): add failing CD audio tick control`).
2. **Task 1 implementation:** `a8e500d` (`feat(06-10): route CDDA through native SPU`).
3. **Bounded follow-up split:** `dfaa571` (`docs(06-33): split XA audio integration`).

## Deviations from Plan

- The original XA mixing task exceeded the 30-minute execution bound before implementation. Its acceptance was preserved in dependent Plan 06-33 and Plan 06-12 now waits for that follow-up.
- The CD audio functions were extracted to `src/cdrom_audio.c` to keep `src/cdrom.c` under the project line cap.
- State continuity tests now wake the SPU path when their fixture seeds a nonempty CD queue, matching the production enqueue behavior.

## Remaining Acceptance

XA integration, mixed XA/CDDA/voice/reverb controls, and XA mute progression remain in Plan 06-33. Aggregate DISC-01 and DISC-02 acceptance remains pending the later reference guests and owned-disc caller.

## Self-Check: PASSED

- The Plan 06-10 summary exists and the task, split, and implementation commits are reachable from `plan_head_after`.
- The final focused gate passed 9/9 tests.
- The XA mixing acceptance remains in Plan 06-33; no acceptance was removed.

## Next Plan Readiness

Plan 06-33 is the next Wave 9 plan. Plan 06-12 depends on it, keeping the XA integration ahead of the MDEC oracle work.

---
*Phase: 06-discs-and-fmv*
*Completed: 2026-11-16*
