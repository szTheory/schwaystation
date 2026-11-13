---
phase: 04-games-play-in-retroarch
plan: "43"
subsystem: libretro
tags: [libretro, native-video, rgb555, rgb24, geometry]
requires:
  - phase: 04-27
    provides: BIOS-free PS-EXE content loading and initial geometry notifications
provides:
  - Native RGB555 and packed RGB24 frames converted for libretro callbacks
  - Geometry and rate callback coverage for native display mode changes
affects: [phase-04-retroarch, GAME-02, GAME-07]
actuals:
  tokens: 4225
  tasks: 2
  commits: 3
  plan_head_before: 6a5244dbfa2346f36fbba26613d8045ccc75fcf4
  plan_head_after: 38f983e730508fe76cb60ae559414cb726e3c86c
tech-stack:
  added: []
  patterns:
    - Fixed-size native video storage and shared bounded pixel conversion
key-files:
  created: []
  modified:
    - libretro/libretro.c
    - tests/libretro/host.c
    - tests/libretro/tests.cmake
    - tests/libretro/inventory.txt
    - docs/embedding.md
key-decisions:
  - "Use the existing bounded shared converter for both RGB555 and packed RGB24 before host callbacks."
  - "Send geometry callbacks only for dimension changes and AV-info callbacks only for rate changes."
patterns-established:
  - "Native video acceptance tests use generated first-party PS-EXEs with independently checked pixels."
requirements-completed: []
coverage:
  - id: D1
    description: "The libretro adapter converts native RGB555 and packed RGB24 frames and reports dimensions and rate changes at the appropriate callbacks."
    requirement: GAME-02
    verification:
      - kind: integration
        ref: tests/libretro/tests.cmake#libretro.video.geometry
        status: pass
    human_judgment: false
  - id: D2
    description: "Repeated modes, display toggles, size and depth changes, interlaced fields, moved windows, and PAL rate changes preserve expected pixels and callback counts."
    requirement: GAME-02
    verification:
      - kind: integration
        ref: tests/libretro/tests.cmake#libretro.video.geometry_edges
        status: pass
    human_judgment: false
metrics:
  duration: 31min
  completed: 2026-11-12
  status: complete
---

# Phase 04 Plan 43: Native libretro video geometry summary

**Native RGB555 and RGB24 frames now reach libretro with correct pixels and mode-change notifications.**

## Performance

- **Duration:** 31 minutes
- **Started:** 2026-11-12T04:32:21Z
- **Completed:** 2026-11-12T05:03:36Z
- **Tasks:** 2
- **Files modified:** 5
- **Runtime/model:** Codex with OpenGSD 1.20.1, `gpt-6-luna` xhigh

## Accomplishments

- Added a real registered RGB24 guest test that exposed the adapter's prior 16-bit-only rejection, then verified independent native and converted pixel values after the fix.
- Converted valid native 15-bit and packed 24-bit video through the existing bounded converter before delivering frames; invalid frames do not update geometry or reach callbacks.
- Added registered edge coverage for repeated modes, display off/on, width/height/depth changes, interlaced fields, moved windows, and rate changes. Documented conversion and callback behavior.

## Evidence

- The registered `libretro.video.geometry` test failed against the old adapter; `gsd_run check tdd-red-evidence build/ci/04-43-task1-red-evidence.json --raw` classified the failing target as valid RED evidence (exit 0).
- Task 1 verification: `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-libretro-host schwaystation-run schwaystation-fixtures && ctest --test-dir build/ci -R '^libretro\.video\.geometry$' --no-tests=error --output-on-failure` (exit 0).
- Task 2 verification: `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-libretro-host schwaystation-run schwaystation-fixtures && ctest --test-dir build/ci -R '^libretro\.video\.geometry_edges$' --no-tests=error --output-on-failure` (exit 0).
- Final `cmake --workflow --preset ci` (exit 0; 488 CI tests and 5 release checks passed) and `cmake --workflow --preset hygiene` (exit 0; 23 checks passed).
- `git diff --check` (exit 0). No hosted RetroArch run or captured hardware oracle was claimed.

## Task Commits

1. **Task 1 RED:** `b66300d` — register the failing RGB24 geometry delivery test.
2. **Task 1 GREEN:** `e0396a4` — convert native RGB24 libretro frames.
3. **Task 2:** `38f983e` — cover native libretro geometry edges.

## Files Created/Modified

- `libretro/libretro.c` — validate and convert native video before callbacks; limit geometry and rate notifications to their respective changes.
- `tests/libretro/host.c` — generated PS-EXE guest and independent pixel/callback assertions.
- `tests/libretro/tests.cmake` and `tests/libretro/inventory.txt` — register and inventory both gating test cases.
- `docs/embedding.md` — document native frame conversion and callback behavior.

## Decisions Made

- Kept storage bounded to native maxima and reused the shared host video converter.
- Left the existing core hashes and no-content test-card pins untouched.
- The broader GAME-02 and GAME-07 requirements remain incomplete; this plan verifies only the libretro native-video slice.

## Deviations from Plan

None. The expected XRGB8888 red-channel value in the new test was corrected during Task 1 before the tracer verification passed.

## Issues Encountered

The initial independent RGB24 pixel expectation placed the red component in the wrong XRGB8888 byte. The host assertion was corrected to match the documented converter output, then the tracer and expanded edge tests passed.

## Next Phase Readiness

The native video callback path and its mode-change regressions are ready for the remaining RetroArch integration plans. No owner verification is needed for this slice.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-12*

## Self-Check: PASSED

- SUMMARY file exists.
- Task commits `b66300d`, `e0396a4`, and `38f983e` are ancestors of the current HEAD.
