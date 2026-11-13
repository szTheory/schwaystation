---
phase: 04-games-play-in-retroarch
plan: "10"
subsystem: gpu-display
tags: [C, GPU, RGB24, runner, SHA-256, CTest]
requires:
  - phase: 04-01
    provides: Inventoried GPU test path and stable test-card display
  - phase: 04-03
    provides: GPU registers, VRAM and instance state
  - phase: 04-09
    provides: Beam timing and bounded field completion
provides:
  - Native bounded 15/24-bit display scanout from GPU display registers
  - Shared native video validation and RGB555/RGB24 host conversion
  - Native frame hashing, bounded PNG output and sanitized guest TTY text
affects: [04-27, 04-32, GPU, runner, libretro]
actuals:
  tokens: 12113
  tasks: 2
  commits: 4
commits: 4
plan_head_before: 741450eaf16b61d3bf7b707f8ab9a0a39ac42e34
plan_head_after: 2c42c1972ab9841362c45fdeda79ba5038d6a6a3
tech-stack:
  added: []
  patterns:
    - Native frame views are validated before host conversion or row traversal
    - RGB555 hash rows serialize as little-endian words; RGB24 rows hash packed bytes
key-files:
  created:
    - tests/gpu/display.c
    - runner/frame_hash.c
    - runner/frame_hash.h
    - runner/tty.h
  modified:
    - src/frame.c
    - src/gpu.c
    - host/xrgb8888.c
    - runner/main.c
    - tests/runner/test_png.c
    - tests/runner/test_runner.c
key-decisions:
  - Keep no-content test-card scanout and its pinned output separate from guest-programmed GPU scanout.
  - Use one bounded video validator for native frame hashing, PNG output and host conversion.
  - Defer libretro dynamic geometry notifications and user-facing runner documentation to their assigned later plans.
metrics:
  duration: 16min
  completed: 2026-11-10
status: complete
requirements-completed: []
coverage:
  - id: D1
    description: Native GPU scanout follows display-window width and field-height rules, wraps both encodings safely, and zeros disabled output.
    requirement: GAME-02
    verification:
      - kind: unit
        ref: tests/gpu/display.c#width_modes,native15_wrap,native24_wrap,interlace_and_off,malformed_windows; ctest -L ^gpu$
        status: pass
    human_judgment: false
  - id: D2
    description: Packed RGB24 converts to independent XRGB/decoded-PNG colors; native hashes include metadata and reject malformed frames.
    requirement: GAME-02
    verification:
      - kind: unit
        ref: tests/runner/test_png.c#rgb24,video_bounds,native_hash; runner.rgb24,runner.video_bounds,runner.native_hash
        status: pass
      - kind: other
        ref: cmake --workflow --preset ci (393 tests and 2 release checks)
        status: pass
    human_judgment: false
  - id: D3
    description: Runner PNG and hash paths share bounded frame validation, and guest TTY output replaces control and high bytes with '?' while preserving tab.
    requirement: GAME-02
    verification:
      - kind: unit
        ref: tests/runner/test_runner.c#tty_sanitize; runner.tty_sanitize
        status: pass
      - kind: other
        ref: cmake --workflow --preset hygiene (23 checks)
        status: pass
    human_judgment: false
---

# Phase 4 Plan 10: Native display windows and bounded RGB24 conversion

**GPU display registers now expose exact bounded native fields, while the runner hashes and presents RGB555/RGB24 with one shared bounds check.**

## Performance

- **Duration:** 16 min
- **Started:** 2026-11-10T18:50:00-05:00
- **Completed:** 2026-11-10T19:06:04-05:00
- **Tasks:** 2
- **Files modified:** 24 task source, test, inventory and todo paths

## Accomplishments

- Added native GPU scanout for guest display windows, including exact horizontal divider widths, native interlace field height, VRAM wrapping, packed RGB24 bytes, display-off zeros and invalid-window handling. The no-content test-card frame remains pinned.
- Added a bounded shared validator/converter and native frame hash support for RGB555 and packed RGB24. Hash and PNG paths validate before allocating or reading; independent colors, decoded PNG bytes, row padding, exact digest, metadata changes and malformed windows are covered.
- Filtered guest TTY output to printable ASCII and tab, replacing other bytes with `?`. Closed first-caller todos 029 and 032; plan 04-32 owns the user-facing runner documentation.

## Task Commits

1. **Task 1 RED: native display scanout cases** - `2e6905b` (test)
2. **Task 1 GREEN: bounded GPU scanout** - `d77b482` (feat)
3. **Task 2 RED: RGB24 and runner bounds regressions** - `4a95fb7` (test)
4. **Task 2 GREEN: native conversion and runner validation** - `2c42c19` (feat)

**Plan metadata:** The final closeout commit includes this summary and the GSD state updates.

## Files Created/Modified

- `src/frame.c`, `src/gpu.c`, `src/gpu.h`, `src/internal.h`, and `src/instance.c` - Build native scanout from guest display registers with instance-owned storage.
- `tests/gpu/display.c`, `tests/gpu/tests.cmake`, `tests/gpu/inventory.txt`, and `tests/core/test_core.c` - Verify exact dimensions, interlace height, VRAM wrapping, disabled output and metadata.
- `host/xrgb8888.c`, `host/xrgb8888.h`, `runner/frame_hash.c`, `runner/frame_hash.h`, `runner/main.c`, and `runner/tty.h` - Validate native frame extents, convert both encodings, hash native rows and filter terminal bytes.
- `runner/CMakeLists.txt`, `tests/runner/test_png.c`, `tests/runner/test_runner.c`, `tests/runner/tests.cmake`, and `tests/runner/inventory.txt` - Register independent RGB24, hash, bounds, PNG and TTY checks.
- `.planning/preparation/PSX-GPU-AND-GTE.md` and `docs/embedding.md` - Correct native field-height and video-view contracts.
- `.planning/todos/completed/029-runner-tty-byte-filter.md` and `.planning/todos/completed/032-print-hash-height-bound.md` - Record the first-caller fixes and their evidence.

## Decisions Made

- No-content test-card scanout remains independent of the guest GPU display registers so existing hashes stay unchanged.
- Native hashes include the 13 metadata words and tight rows: little-endian RGB555 words or packed RGB24 bytes. Host presentation stays outside the hash.
- The bounded native dimensions are 1024 by 512; host input pitch and output pitch are checked before row access.
- Libretro geometry notifications and user-facing runner docs remain with plans 04-27 and 04-32, respectively.

## Deviations from Plan

None. Small internal runner helpers isolate native hashing and TTY-byte filtering for direct independent tests.

## Verification

- Task 1 RED: `cmake --workflow --preset ci` — expected failure on native display assertions before scanout implementation.
- Task 1 GREEN: `cmake --workflow --preset ci && ctest --test-dir build/ci -L '^gpu$' --no-tests=error --output-on-failure` — exit 0; full CI and 51/51 GPU tests passed.
- Task 2 RED: `cmake --workflow --preset ci` — expected failures in the new RGB24, native hash, bounds and TTY cases before implementation.
- Task 2 GREEN: `ctest --test-dir build/ci -R '^runner\\.(rgb24|video_bounds|native_hash|tty_sanitize|xrgb8888|png)$' --no-tests=error --output-on-failure` — exit 0; 6/6 passed.
- Final: `cmake --workflow --preset ci` — exit 0; 393/393 tests and both release archive checks passed. Existing test-card hash and PNG pins remained green.
- Final: `cmake --workflow --preset hygiene` — exit 0; all 23 checks passed. `git diff --check` — exit 0.
- No hosted run URL was produced. No owner UAT is required.

## User Setup Required

None - no external service configuration is required.

## Next Phase Readiness

Plan 04-10's native scanout, runner conversion and bounds work is complete. GAME-02 remains partial until the later phase plans implement content-driven libretro geometry and the other gameplay deliverables. Plan 04-27 owns libretro geometry notifications; plan 04-32 owns the matching public documentation and final phase review/verification.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-10*

## Self-Check: PASSED

- SUMMARY exists at the required path.
- Task commits `2e6905b`, `d77b482`, `4a95fb7`, and `2c42c19` are ancestors of HEAD.
