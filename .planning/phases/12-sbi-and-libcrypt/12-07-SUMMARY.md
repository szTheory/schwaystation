---
phase: 12-sbi-and-libcrypt
plan: "07"
subsystem: testing
tags: [libretro, pcm, ci-efficiency, cmake]
requires:
  - phase: 12-01
    provides: Runner audio capture and libretro host comparison paths
provides:
  - One verified runner PCM capture per region reused by both libretro callback comparisons
affects: [CRYPT-01, libretro host test, CI iteration time]
actuals:
  tokens: 747
  tasks: 1
  commits: 1
commits: 1
plan_head_before: db9d747d965b7f3d5964c34cf76fd2d42f5ebd5c
plan_head_after: 57689ce90cf3459aab15ff984a188d18928438a6
tech-stack:
  added: []
  patterns:
    - Share an identical deterministic runner capture across callback-mode comparisons
key-files:
  created: []
  modified:
    - tests/libretro/tests.cmake
key-decisions:
  - Capture once per region because runner arguments and PCM inputs are identical across callback modes.
  - Preserve six callback comparisons and every existing file-size, alignment, hash and exit assertion.
requirements-completed: [CRYPT-01]
coverage:
  - id: D1
    description: Each of three regions uses one verified runner PCM capture for both batch and single callback comparisons.
    requirement: CRYPT-01
    verification:
      - kind: integration
        ref: ctest --preset ci --no-tests=error -R '^libretro[.]host$' --output-on-failure
        status: pass
    human_judgment: false
metrics:
  duration: 8min
  completed: 2026-11-23
status: complete
---

# Phase 12 Plan 07: Reuse libretro runner PCM Summary

**One runner PCM capture per region now feeds both callback comparisons while preserving every existing check.**

## Performance

- **Duration:** 8 minutes elapsed from plan start through implementation, focused verification and closeout; active time was not separately measured.
- **Started:** 2026-11-23T08:02:00Z
- **Completed:** 2026-11-23T08:10:24Z
- **Tasks:** 1
- **Files modified:** 1
- **Runtime/model:** Codex GSD executor; configured `gpt-6-luna` xhigh; primary runtime model ID unavailable.

The source count drops runner captures from six to three while keeping all six host callback comparisons. The prior BE32 `libretro.host` measurement was about 18m52s. This focused native run took 131.00s, which is not a comparable host result; no hosted time savings are claimed until phase CI measures the changed head.

## Accomplishments

- Hoisted runner PCM capture and integrity checks outside the callback loop, once for each of `ntsc-j`, `ntsc-u` and `pal`.
- Reused each region's verified PCM path for both `batch` and `single` host comparisons. All runner inputs, mailbox guards, file existence/size/alignment checks, SHA-256 comparisons, host exit checks and the separate baseline host call remain.
- Confirmed the invocation change before and after editing: runner captures 6→3; callback comparisons remain 6; baseline host call remains 1.

## Test Evidence

| Command | Result |
| --- | --- |
| Before/after source-count and assertion preflight | Exit 0 after narrowing the host-call selector; captures 6→3, comparisons 6→6, baseline call 1 retained, runner arguments and all assertion counts unchanged. |
| `git diff --check` | Exit 0. |
| `cmake --preset ci` | Exit 0 in 4.6s. |
| `cmake --build --preset ci --target schwaystation-test-libretro-host schwaystation-run` | Exit 0 in 5.2s. |
| `ctest --preset ci --no-tests=error -R '^libretro[.]host$' --output-on-failure` | Exit 0; 1/1 passed in 131.00s (131.06s wall). |

The plan's pre-edit probe initially counted the separate baseline host invocation as a callback comparison and exited 1. I narrowed it to the `audio-equality-${callback}` call before editing; the corrected count is six callback comparisons plus one distinct baseline call.

No hosted CI or Act run was started because this change does not alter workflow routing and the focused host test runs locally. No Docker resources were created. The 131-second local result demonstrates the focused test still passes; it is not used to claim a hosted critical-path reduction.

## Task Commits

1. **Task 1: Capture runner PCM once per region and reuse it for both callbacks** — `57689ce` (`perf(12-07): reuse runner PCM per region`).

## Files Created/Modified

- `tests/libretro/tests.cmake` — moves runner capture and PCM integrity checks to the per-region scope; keeps both callback comparisons inside the callback loop.

## Decisions Made

- Reused one PCM file per region because both callback comparisons use identical runner input arguments; output path is now region-scoped.
- Kept all six callback comparisons and all existing integrity assertions, including the independent baseline host invocation.

## Deviations from Plan

None. The initial source-count query was corrected before editing and did not alter the source change or measured counts.

## Issues Encountered

- The first count probe included a separate baseline host call; the corrected probe distinguished it from the six callback comparisons before any source edit.
- No comparable hosted run was performed, so CI time savings remain unmeasured.

## Next Phase Readiness

- All seven Phase 12 plans are implemented. Phase-level review, verification and required hosted CI remain; no ship or next phase was started.

---
*Phase: 12-sbi-and-libcrypt*
*Completed: 2026-11-23*

## Self-Check: PASSED

- The summary and modified test file exist; task commit `57689ce90cf3459aab15ff984a188d18928438a6` is an ancestor of the current plan head.
- The preflight and focused integration test passed, and `git diff --check` passed.
