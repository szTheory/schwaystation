---
phase: 05-the-sound-matches-the-console
plan: "12"
subsystem: libretro audio
tags:
  - pcm
  - libretro
  - runner
requires:
  - phase: 05-the-sound-matches-the-console
    provides:
      - Native SPU PCM and runner audio hashing
      - Stub-booted nonzero tone fixture
provides:
  - Test-only runner PCM capture from the same drain used by --hash-audio
  - Batch and single callback equality checks for NTSC-J, NTSC-U and PAL
  - Reset, content reload, sample cadence and AV timing assertions
affects:
  - Phase 5 verification
  - libretro audio delivery
actuals:
  tasks: 2
  commits: 3
  plan_head_before: 39aa9c9e21e0aa4a437c52d9daaaf8a71975a9e0
  plan_head_after: c9fac3b77289723fde867bf2c3672671e84b9258
tech-stack:
  added: []
  patterns:
    - Compare canonical little-endian runner PCM to each frontend callback frame
    - Check carried frame sample counts against the rates reported by schway_get_info
key-files:
  created: []
  modified:
    - runner/main.c
    - tests/libretro/host.c
    - tests/libretro/tests.cmake
    - docs/embedding.md
key-decisions:
  - Keep runner PCM capture behind SCHWAY_RUN_TEST_HOOKS and compile the option out of the release runner.
  - Leave libretro/libretro.c unchanged because both callback paths already match the real runner stream in all tested regions.
  - Keep the power-reset check separate from content restart: a power reset erases the loaded executable, and a fresh content load is compared against the runner's first frame.
requirements-completed: []
coverage:
  - id: D1
    description: The test runner writes the same little-endian bytes consumed by --hash-audio; release runner rejects the test-only option.
    requirement: SOUND-01
    verification:
      kind: integration
      ref: "cmake --preset ci (0); cmake --build --preset ci --target schwaystation-run-test schwaystation-test-libretro-host (0); ctest --test-dir build/ci -R '^libretro\\.host$' --no-tests=error --output-on-failure (1/1 passed); release CLI with --test-audio-file exited 2 and reported unknown option"
      status: pass
    human_judgment: false
  - id: D2
    description: Batch and single callback bytes, stereo ordering and pair counts match runner PCM for NTSC-J, NTSC-U and PAL; reset and reload preserve stream boundaries.
    requirement: SOUND-02
    verification:
      kind: integration
      ref: "cmake --preset ci (0); cmake --build --preset ci --target schwaystation-test-libretro-host schwaystation-run-test (0); ctest --test-dir build/ci -R '^libretro\\.host($|\\.)' --no-tests=error --output-on-failure (2/2 passed)"
      status: pass
    human_judgment: false
  - id: D3
    description: Audio pair totals and libretro AV rates remain coupled to schway_get_info, including interlaced content.
    requirement: SOUND-02
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^libretro\\.video\\.geometry_edges$' --no-tests=error --output-on-failure (1/1 passed)"
      status: pass
    human_judgment: false
  - id: D4
    description: Embedding documentation describes modeled signed PCM, native rates, and the lack of host audio-device or resampling behavior.
    requirement: SOUND-01
    verification:
      kind: manual_procedural
      ref: "Reviewed docs/embedding.md Audio section against schway_get_audio, schway_get_info and the SPU frame producer."
      status: pass
    human_judgment: false
duration: "about 34 min wall from the persisted plan baseline; exact active time across restart is unavailable"
completed: 2026-11-14
status: complete
---

# Phase 05 Plan 12: Runner and libretro PCM equality Summary

**The runner's real PCM stream now gates libretro batch and single callback delivery in every region, with cadence and lifecycle checks tied to the core's native AV rates.**

## Performance

- **Duration:** About 34 minutes wall from the persisted plan baseline; the computer restart means exact active time is unavailable.
- **Started:** 2026-11-14T09:16:30Z (prior plan closeout baseline; Plan 05-12 began after it)
- **Completed:** 2026-11-14T09:50:40Z
- **Tasks:** 2
- **Files modified:** 4
- **Runtime/model:** Codex, OpenGSD 1.21.0; GPT-6 session (exact model ID not exposed)

## Accomplishments

- Added a `SCHWAY_RUN_TEST_HOOKS` PCM-file sink that serializes the same drained stereo pairs as `--hash-audio`; the release runner rejects the test-only option.
- Expanded the existing `libretro.host` integration to compare fullpath `spu_tone.exe` output byte for byte through batch and single callbacks in NTSC-J, NTSC-U and PAL.
- Checked nonzero asymmetric PCM, per-frame 737/738 or 886/887 counts, aggregate cadence, native AV rates, reset clearing, and a clean first frame after content reload.
- Checked that interlaced content keeps the frame rate reported by `schway_get_info`; updated the embedding audio contract to describe modeled signed PCM and native rate behavior.

## Task Commits

1. **Task 1 RED:** d51a623 — the callback stream comparison detected that a deliberately zeroed PCM reference differed from the real tone at byte 52.
2. **Task 1 GREEN:** f7b8b38 — added test-only PCM capture from the runner's single audio drain; the focused host test passed.
3. **Task 2:** c9fac3b — added the six region/callback combinations, lifecycle/cadence/AV checks, and embedding documentation; the existing libretro adapter passed, so no production adapter change was warranted.

## Files Created/Modified

- `runner/main.c` — added the test-only canonical PCM file sink behind `SCHWAY_RUN_TEST_HOOKS`.
- `tests/libretro/host.c` — compared each callback stream against runner PCM and asserted cadence, lifecycle, and interlaced AV behavior.
- `tests/libretro/tests.cmake` — drove the existing host integration across both callback modes and all three regions through the configured emulator.
- `docs/embedding.md` — documented modeled PCM, native timing, and host responsibilities.

## Decisions and Deviations

- No change to `libretro/libretro.c` was supported by the tests: both callback paths already preserve the real runner bytes and pair counts across all regions.
- The first reset assertion expected a power reset to restart the loaded executable. The repository's reset contract erases that executable, so the test now checks that reset does not replay its tone, then reloads content and compares its first frame with runner PCM.
- CMake script mode does not define `PROJECT_BINARY_DIR`; the test passes its build test directory explicitly to the script before creating PCM files.
- SOUND-01 and SOUND-02 remain pending for phase-level verification.
