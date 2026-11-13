---
phase: 04-games-play-in-retroarch
plan: "29"
subsystem: testing
tags: [retroarch, e2e, screenshots, cmake, runner]
requires:
  - phase: "04-54"
    provides: portable host and runner file handling used by the hosted integration path
  - phase: "04-55"
    provides: measured movie-path behavior used by the combined current-head run
provides:
  - Hosted BIOS-free RetroArch proof comparing two distinct frames against runner images
affects: [04-31, 04-32, phase-04-verification]
actuals:
  tokens: 32293
  tasks: 2
  commits: 67
plan_head_before: c0211f8836fe7dced46c5793f8b5ea659989986e
plan_head_after: 2f839fd29819d3d32a7ade8f0348db8fc6a1b1c5
tech-stack:
  added: []
  patterns:
    - Hosted headless RetroArch capture with independent runner reference images
    - One content session checks two distinct deterministic frame limits
key-files:
  created: []
  modified:
    - .github/workflows/ci.yml
    - tests/retroarch/run_retroarch.cmake
    - tests/retroarch/verify.py
    - tests/retroarch/test.cfg.in
    - docs/testing.md
    - tests/libretro/host.c
key-decisions:
  - "Use the existing hosted background driver with empty system/ and exact comparisons for frames 59 and 60."
  - "Keep local RetroArch execution stopped under the existing safety constraint; hosted automation supplies the required proof."
patterns-established:
  - "Compare decoded hosted screenshots to independently produced runner images, and require the two selected frames to differ."
requirements-completed: []
coverage:
  - id: D1
    description: The hosted RetroArch session uses an empty system directory, injects Cross at frame 60, captures distinct frames 59 and 60, and compares each pixel-for-pixel with its runner reference.
    requirement: GAME-07
    verification:
      - kind: e2e
        ref: https://github.com/szTheory/schwaystation/actions/runs/43404686338
        status: pass
    human_judgment: false
duration: "not measured; checkpointed across multiple sessions"
completed: 2026-11-13
status: complete
---

# Phase 4 Plan 29: Hosted RetroArch Content Proof Summary

**The hosted RetroArch session now proves two distinct BIOS-free frames against exact runner reference images.**

## Performance

- **Duration:** Not measured; this plan accumulated checkpointed source prerequisites and resumed acceptance across sessions, and active time cannot be isolated from retained timestamps.
- **Started:** Original active start was not retained.
- **Completed:** 2026-11-13
- **Tasks:** 2
- **Files modified:** 26 across the plan's retained implementation and diagnostic commits.
- **Runtime/model:** Codex/OpenGSD 1.20.0; gpt-6-luna xhigh.

## Accomplishments

- Wired the existing hosted RetroArch driver to run the pad-counter content with an empty system directory and deterministic Cross input at frame 60.
- Captured distinct frames 59 and 60 and compared each decoded image pixel-for-pixel with its corresponding runner image.
- Preserved the 300-line RetroArch test-code ceiling and the existing mismatch/dimension refusal controls.
- Removed the temporary Windows diagnostic workflow after the focused Windows portability result.

## Task Commits

Plan-specific commits accumulated across checkpointed work:

1. `3133c14` — add RetroArch replay round-trip diagnostic.
2. `41dd821` — retain hosted RetroArch replay probe.
3. `993d705` — run RetroArch after build-leg failures.
4. `f5e7e4b` — capture distinct RetroArch guest frames.
5. `67de19b` — document exact RetroArch frame mapping.
6. `547af36` — use pad-counter runner frames.
7. `501ed59` — align replay captures and use portable file opens.
8. `cfde4f6` — pin synchronous RetroArch replay startup.
9. `7014df2` — normalize Windows compatibility paths.
10. `ea3ec54` — bound the serialized BE32 movie lane.
11. `01b5406` — add temporary Windows path diagnostics.
12. `f41ffdb` — use secure CRT for diagnostic flag.
13. `f502338` — normalize synthetic config newlines.
14. `013ecf9` — normalize synthetic path expectation.
15. `8c3bdcb` — remove temporary Windows path probe.

The persisted plan ledger measures 67 commits from `c0211f8` through the recorded plan head because canonical acceptance was deferred while later phase plans advanced. The 15 commits listed above are the plan29-tagged implementation and diagnostic commits; the ledger count is not a claim that all 67 commits belong to plan29.

## Files Created/Modified

- `.github/workflows/ci.yml` — runs the hosted session and allows the real-host evidence job to execute after unrelated matrix failures while preserving aggregate failure behavior.
- `tests/retroarch/run_retroarch.cmake`, `tests/retroarch/test.cfg.in` — drive one deterministic content session and frame selection.
- `tests/retroarch/verify.py` — compare dimensions and decoded pixels with runner references.
- `tests/libretro/host.c`, `docs/testing.md` — provide the content/input seam and document hosted behavior.
- Compatibility, fixture, replay and runner tests plus temporary diagnostic configuration were updated during the retained Windows and timing investigations; the temporary path probe was removed.

## Decisions Made

- Kept the required integration on hosted background RetroArch; local launch remains stopped by the existing safety constraint.
- Required distinct images at frames 59 and 60, each independently compared to its runner result; zero-input replay equality is not used as acceptance evidence.

## Deviations from Plan

**Plan artifact metadata correction:** The plan named nonexistent `tests/retroarch/compare.c`. The implementation and successful hosted flow use the existing `tests/retroarch/verify.py`. The plan's artifact, context and task file declarations were corrected during closeout. This metadata change adds no behavior and requires no test rerun.

## Evidence

- Current exact-head run [43404686338](https://github.com/szTheory/schwaystation/actions/runs/43404686338), RetroArch job 131818379067: success on `2f839fd`; empty system directory, distinct frames 59 and 60, exact comparison against each runner reference, and deterministic Cross input at frame 60.
- The comparator's existing wrong-pixel and wrong-dimension controls remained registered. The existing aggregate RetroArch code budget remained 299/300 lines.

## User Setup Required

None. The required integration ran in hosted CI; no user UAT or local RetroArch launch was needed.

## Next Phase Readiness

This hosted content acceptance slice is complete. Plan 32 and phase verification remain; stop after phase verification before phase 4 ship or phase 5.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-13*

## Self-Check: PASSED

The summary file exists, and each of the 15 listed plan-specific commits is an ancestor of the recorded plan head.
