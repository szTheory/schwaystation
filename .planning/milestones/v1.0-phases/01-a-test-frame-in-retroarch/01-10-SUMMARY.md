---
phase: 01-a-test-frame-in-retroarch
plan: 10
subsystem: libretro, testing
tags: [libretro, adapter, xrgb8888, audio, core-options, serialize, cmake, ctest]
status: complete

requires:
  - phase: 01-02
    provides: "the vendored libretro/libretro.h"
  - phase: 01-03
    provides: "schway_get_info rationals, schway_get_audio silent pairs, the state API"
  - phase: 01-04
    provides: "schwaystation-host with schway_host_xrgb8888_frame, shared with the runner"
  - phase: 01-05
    provides: "abi gates on the same schwaystation archive the module links"
  - phase: 01-08
    provides: "install components and packaging with a libretro component"
provides:
  - "SCHWAYSTATION_BUILD_LIBRETRO (default PROJECT_IS_TOP_LEVEL, OFF for WASI) and the MODULE target schwaystation_libretro (no lib prefix, .dylib on macOS), exporting only the 25 retro_* entry points"
  - "Generated build/<preset>/libretro/schwaystation_libretro.info with display_version from version.txt; install component libretro (cores/, info/)"
  - "Core option schwaystation_region (ntsc-u default, ntsc-j, pal) through SET_CORE_OPTIONS_V2 or SET_VARIABLES"
  - "CTests libretro.host and libretro.host.pal (labels libretro, portable)"
affects: [01-11, 01-12, 01-13, 01-14]

actuals:
  tokens: 11275
  tasks: 2
  commits: 2
plan_head_before: f270f0e8cdf2a71acee6205097fa1011ce3739a7
plan_head_after: c84e837a13839d552c9a8b92e6fb35eacb5aabc7

tech-stack:
  added: []
  patterns:
    - "In-process libretro host test: dlopen or LoadLibraryA, all 25 entry points resolved first, a reference schway instance stepped frame for frame beside the module"
    - "One host binary, two modes on the command line (ntsc-u: core options v2 and batch audio; pal: no options version and single-sample audio), so both fallbacks run"

key-files:
  created:
    - libretro/CMakeLists.txt
    - libretro/libretro.c
    - libretro/schwaystation_libretro.info.in
    - tests/libretro/host.c
  modified:
    - CMakeLists.txt
    - tests/libretro/tests.cmake
    - tests/libretro/inventory.txt

key-decisions:
  - "The instance is created in retro_load_game and destroyed in retro_unload_game and retro_deinit. A changed region option destroys it and creates a new one, because a profile is locked after the first frame"
  - "SET_SYSTEM_AV_INFO is sent only when GET_VARIABLE_UPDATE reports a region different from the running one. At load the option sets the new instance's profile, and the frontend reads the AV info after load returns"
  - "The library version reaches retro_get_system_info as a compile definition from PROJECT_VERSION, which comes from version.txt, as in the seed. The generated version header holds only numeric macros"
  - "The .info claims core_options true and core_options_version 2.0, because libretro.host checks the option. savestate stays false, even though retro_serialize is wired and tested in-process"
  - "Max geometry is 640 x 480 (NTSC-U, NTSC-J) or 640 x 576 (PAL), defined in the adapter. The frame buffer is allocated once per instance at 640 x 576"

requirements-completed: [FRAME-03]

coverage:
  - id: D1
    description: "libretro.host loads the built module, answers SET_SUPPORT_NO_GAME and SET_PIXEL_FORMAT, refuses content, loads with no game and runs 60 frames, each equal byte for byte to schway_host_xrgb8888_frame of a reference instance's schway_get_video"
    requirement: FRAME-03
    verification:
      - kind: integration
        ref: "tests/libretro/tests.cmake#libretro.host"
        status: pass
    human_judgment: false
  - id: D2
    description: "retro_get_system_av_info reports fps 53693175.0 / 897619.0 (NTSC) or 53203425.0 / 1069484.0 (PAL) and sample_rate 44100.0, checked with exact double equality; base 320 x 240, max 640 x 480 or 640 x 576, aspect exactly 4.0f / 3.0f"
    requirement: FRAME-03
    verification:
      - kind: integration
        ref: "tests/libretro/tests.cmake#libretro.host"
        status: pass
      - kind: integration
        ref: "tests/libretro/tests.cmake#libretro.host.pal"
        status: pass
    human_judgment: false
  - id: D3
    description: "No float or double in the core; the doubles and the float exist only in libretro/libretro.c"
    requirement: FRAME-03
    verification:
      - kind: other
        ref: "grep -E -c 'float|double' src/*.c src/*.h include/schwaystation.h sums to 0; grep -c double libretro/libretro.c prints 2"
        status: pass
    human_judgment: false
  - id: D4
    description: "Each retro_run forwards exactly the pairs the reference instance drains for that frame (737 or 738 NTSC, 886 or 887 PAL), through audio_batch, or pair by pair when only the single-sample callback is set"
    requirement: FRAME-03
    verification:
      - kind: integration
        ref: "tests/libretro/tests.cmake#libretro.host"
        status: pass
      - kind: integration
        ref: "tests/libretro/tests.cmake#libretro.host.pal"
        status: pass
    human_judgment: false
  - id: D5
    description: "schwaystation_region registers through SET_CORE_OPTIONS_V2 when GET_CORE_OPTIONS_VERSION reports 2, else through SET_VARIABLES. It is read at load. An unchanged update keeps the instance. A changed region recreates the instance from power-on and sends SET_SYSTEM_AV_INFO once with the new rates, and retro_get_region follows"
    requirement: FRAME-03
    verification:
      - kind: integration
        ref: "tests/libretro/tests.cmake#libretro.host"
        status: pass
      - kind: integration
        ref: "tests/libretro/tests.cmake#libretro.host.pal"
        status: pass
    human_judgment: false
  - id: D6
    description: "retro_reset is schway_reset POWER. retro_serialize_size equals schway_state_size, and the state matches the reference's bytes. A serialize at frame 30, run to 60, unserialize, run to 60 sequence reproduces the 60th frame and each frame's audio. A short buffer and a damaged state are refused"
    requirement: FRAME-03
    verification:
      - kind: integration
        ref: "tests/libretro/tests.cmake#libretro.host"
        status: pass
      - kind: integration
        ref: "tests/libretro/tests.cmake#libretro.host.pal"
        status: pass
    human_judgment: false
  - id: D7
    description: "The module exports only retro_* symbols and links the same schwaystation archive the abi tests check"
    requirement: FRAME-03
    verification:
      - kind: other
        ref: "nm -g build/ci/libretro/schwaystation_libretro.dylib, defined T/D/B/S/C names piped to grep -v -E '^_?retro_' exits 1 (25 names, all retro_)"
        status: pass
    human_judgment: false
  - id: D8
    description: "The generated .info claims supports_no_game true, savestate false and is_experimental true, and lists no content extension; it installs to info/ and the module to cores/ in the libretro archive"
    requirement: FRAME-07
    verification:
      - kind: other
        ref: "grep -c 'savestate = \"false\"' build/ci/libretro/schwaystation_libretro.info prints 1; cpack --preset ci exit 0 and the libretro zip lists cores/schwaystation_libretro.dylib and info/schwaystation_libretro.info"
        status: pass
    human_judgment: false
  - id: D9
    description: "Both libretro tests are inventoried and ran exactly once in the ci preset"
    requirement: FRAME-03
    verification:
      - kind: integration
        ref: "cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake exit 0 (77 tests ran once each)"
        status: pass
    human_judgment: false

duration: 12min
completed: 2026-10-20
---

# Phase 1 Plan 10: The libretro Adapter and In-Process Host Summary

The `schwaystation_libretro` core now boots to the test card with no content. It delivers XRGB8888 through the runner's shared converter, sends each frame's exact silent pairs, and reports the core's exact rate rationals. It has a region core option, a power-cycle reset and serialize wired to the state API. A dlopen host checks all of this against a reference instance, for NTSC and PAL.

## Performance

- **Duration:** about 12 min wall time (2026-10-20T15:40:08Z to about 15:52Z)
- **Started:** 2026-10-20T15:40:08Z
- **Completed:** 2026-10-20
- **Tasks:** 2
- **Files modified:** 7 (4 created, 3 modified)

## Accomplishments

- Top-level CMake has `SCHWAYSTATION_BUILD_LIBRETRO`, default `PROJECT_IS_TOP_LEVEL` and OFF for WASI. `schwaystation-host` builds when the runner or libretro is on, and `add_subdirectory(libretro)` runs when libretro is on.
- `libretro/libretro.c` covers the 25 entry points:
  - Video goes through `schway_host_xrgb8888_frame`, and audio drains `schway_get_audio`.
  - The `schwaystation_region` option uses v2 or SET_VARIABLES, and a changed region recreates the instance.
  - Reset and serialize go through the core API. The adapter resets every static in `retro_deinit`.
- `tests/libretro/host.c` steps a reference `schway` instance beside the module. Every frame's pixels and audio pairs must match it. It also runs:
  - the exact AV-info equalities
  - the serialize, run, unserialize, run replay
  - refused content
  - an unchanged update and a region change
  - a deinit and reinit

## Task Commits

1. **Task 1: no-content boot, XRGB8888 frame and AV info, in-process compare** - `aebaed6` (feat)
2. **Task 2: batch audio, the region option, reset and serialize, the PAL case** - `c84e837` (feat)

## Files Created/Modified

- `CMakeLists.txt`: the libretro option, the host library condition, `add_subdirectory(libretro)`
- `libretro/CMakeLists.txt`: the MODULE target, the version definition, the `.info` configure, the install rules (top-level only)
- `libretro/libretro.c`: the adapter
- `libretro/schwaystation_libretro.info.in`: the core information
- `tests/libretro/host.c`: the in-process frontend
- `tests/libretro/tests.cmake`, `tests/libretro/inventory.txt`: `libretro.host`, `libretro.host.pal`

## Evidence (commands from the repository root, exit status)

- `cmake --workflow --preset ci`: exit 0 (77 tests, 100% passed), run after both task commits
- `ctest --test-dir build/ci -R '^libretro\.host$' --output-on-failure` (tracer gate re-run): exit 0
- `ctest --test-dir build/ci -L libretro --output-on-failure`: exit 0 (2 tests)
- `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake`: exit 0
- `cmake --workflow --preset hygiene`: exit 0
- `cpack --preset ci`: exit 0. The libretro zip holds `cores/schwaystation_libretro.dylib` and `info/schwaystation_libretro.info`.
- `nm -g build/ci/libretro/schwaystation_libretro.dylib`, defined names, `grep -v -E '^_?retro_'`: exit 1, so no name lacks the prefix (25 names)
- Cruft grep, the plan's two `git grep --untracked` commands with `test "$a" -eq 1 && test "$b" -eq 1`: a=1, b=1, exit 0
- Planning-ID scan `git grep --untracked -nE -e 'D-[0-9]{2}' -e 'FRAME-[0-9]' -e 'XFER-[0-9]' -e 'DEC\.[0-9]' -- CMakeLists.txt libretro tests/libretro ':!libretro/libretro.h'; test $? -eq 1`: exit 0
- Mutation checks, each run once against the libretro tests. Each mutant compiled and failed `ctest -L libretro` with exit 8:
  - dropping SET_SYSTEM_AV_INFO
  - a no-op `retro_unserialize`
  - a no-op `retro_reset`
  - fps rounded through float
  - one pair fewer per batch
  - The batch mutant fails only `libretro.host`, as expected: the pal mode uses single samples.
- `cmake --workflow --preset dev`: exit 8, on `abi.undefined` only. This failure was already there before this plan; see Issues Encountered.

## Decisions Made

See `key-decisions` in the frontmatter.

## Deviations from Plan

### Auto-fixed Issues

None.

### Plan interpretation

- **The version string comes from a compile definition.** `SCHWAYSTATION_LIBRETRO_VERSION="${PROJECT_VERSION}"` is set in `libretro/CMakeLists.txt`, as the seed does. The plan asked for "the version from the generated header", but `schwaystation_version.h` holds numeric macros with a `u` suffix. Both come from `version.txt`, and the host checks the string against `schway_get_versions`.
- **SET_SYSTEM_AV_INFO at load.** The plan says "a change read at load ... sends SET_SYSTEM_AV_INFO". At load, the option sets the new instance's profile, and no SET_SYSTEM_AV_INFO is sent. libretro.h line 1370 describes that call as a change to AV info the frontend already holds, and the frontend reads it after `retro_load_game` returns.
- **Extra `.info` claims.** The `.info` adds `core_options = "true"` and `core_options_version = "2.0"`. The plan's field list did not name them, but the option exists and is tested.
- **Vendored header citation.** The adapter's header comment cites the vendored libretro.h as libretro-common, pinned in PROVENANCE.md. The seed's "RetroArch v1.22.2" wording was dropped because the pin is `a3194c18ba65`.

### Package-file and src defects

None. Plans 01-05 and 01-08 record no defect for this plan, so `cmake/` and `src/` were not touched, and `include/schwaystation.h` was not edited.

## Issues Encountered

- `cmake --workflow --preset dev` (Debug, AppleClang on macOS) fails `abi.undefined`: `libschwaystation.a` references `memcpy_chk` and `memset_chk`. A Debug build of the pre-plan commit `f270f0e`, exported with `git archive`, fails the same way, so this plan did not cause it. The fix belongs in `schway_core_options` or `tests/cmake/abi.cmake`, which this plan does not touch. It is logged in `deferred-items.md`. The `ci` preset passes.
- `clang-format` is not installed on this host, so the format check was not run.
- These were not run locally and run on their CI lanes:
  - `be32`: the host runs through `CMAKE_CROSSCOMPILING_EMULATOR` and dlopens under qemu-user
  - `asan`
  - `ci-msvc`: `LoadLibraryA` and dllexport

## Known Stubs

None that block the goal. `retro_cheat_*`, `retro_set_controller_port_device` and the memory-region getters are intentional no-ops in this release, as the seed has them. The tests call them and check the values they return.

## Threat Flags

| Flag | File | Description |
|------|------|-------------|
| threat_flag: untrusted-input | libretro/libretro.c | `retro_unserialize` passes frontend bytes to `schway_load_state`, which checks magic, format, size and CRC before applying anything. The host test feeds it a damaged state and checks that it is refused. |

## Requirement Status

`requirements-completed` lists FRAME-03, this plan's in-process proof for NTSC and PAL. The REQUIREMENTS.md boxes for FRAME-03 and FRAME-07 stay open because later plans still declare them: 01-14 declares FRAME-03, and 01-11, 01-12, 01-13 and 01-14 declare FRAME-07. Plan 01-12's hosted RetroArch run loads the module built here.

## Next Phase Readiness

The module RetroArch loads in plan 01-12 exists and is packaged in the libretro component. Ready for the next plan in phase 1.

## Self-Check: PASSED

- FOUND: libretro/CMakeLists.txt, libretro/libretro.c, libretro/schwaystation_libretro.info.in, tests/libretro/host.c
- FOUND: aebaed6, c84e837 (ancestors of HEAD)
