---
phase: 01-a-test-frame-in-retroarch
plan: 01
subsystem: core, build, testing
tags: [c17, cmake, ctest, presets, sha256, test-card, state-visitor]

requires: []
provides:
  - "Complete phase-1 public header include/schwaystation.h (API v1)"
  - "Core from OBJECT libraries schwaystation_instance, schwaystation_testcard, schwaystation_state linked into STATIC schwaystation"
  - "Animated test card drawn into the instance's 1024 x 512 VRAM, scanned out by schway_get_video"
  - "Exact cycle accumulator, silent audio ring, state save/load/visit walk"
  - "schwaystation-host (XRGB8888) and schwaystation-run --frames N --hash"
  - "Presets dev, ci, ci-msvc, hygiene, asan, nofp, be32, wasm32, perf; schway_add_test; per-directory exact inventories"
  - "runner.hash pinned to the independently computed NTSC-U frame-60 hash"
affects: [01-02, 01-03, 01-04, 01-05, 01-06, 01-07, 01-08, 01-09, 01-10, 01-11]

actuals:
  tokens: 26509
  tasks: 3
  commits: 3
plan_head_before: 12c35cd7debd9cfa1a7ff580591711feea944e78
plan_head_after: 7bbdd97736b31e16d6c36e647508f801aeff1775

tech-stack:
  added: []
  patterns:
    - "One schway_core_options(target) applied to every core target (hidden, PIC, no stack protector, -mgeneral-regs-only under nofp)"
    - "Directory selections in tests/inventory/*.txt; names in tests/<dir>/inventory.txt; one shared reader tests/cmake/inventory.cmake"
    - "State walk modes SIZE, WRITE, VALIDATE, APPLY, RECORD, VISIT over one field list; VRAM as one array field"

key-files:
  created:
    - include/schwaystation.h
    - src/internal.h
    - src/instance.c
    - src/frame.c
    - src/testcard.c
    - src/state.c
    - src/state.h
    - src/crc32.c
    - host/xrgb8888.c
    - host/xrgb8888.h
    - runner/CMakeLists.txt
    - runner/main.c
    - runner/sha256.c
    - runner/sha256.h
    - runner/exit_status.h
    - CMakeLists.txt
    - CMakePresets.json
    - cmake/SchwaystationTests.cmake
    - cmake/packaging.cmake
    - cmake/version.h.in
    - cmake/schwaystation.pc.in
    - cmake/schwaystationConfig.cmake.in
    - cmake/toolchains/ppc-linux-gnu.cmake
    - cmake/toolchains/wasm32-wasi.cmake
    - tests/tests.cmake
    - tests/check.h
    - tests/testcard_pins.cmake
    - tests/cmake/inventory.cmake
    - tests/cmake/expect_fail.cmake
    - tests/cmake/check_junit.cmake
    - tests/inventory/ci.txt
    - tests/inventory/hygiene.txt
    - tests/inventory/nofp.txt
    - tests/inventory/wasm32.txt
    - "tests/{abi,nofp,core,runner,libretro,install,release,perf,policy,hygiene}/{tests.cmake,inventory.txt}"
  modified:
    - scripts/hygiene.sh

key-decisions:
  - "schway_video is 80 + 2P bytes (88 on 32-bit, 96 on 64-bit) with a trailing reserved1; schway_audio is 16 + 2P with no trailing field (it holds no u64); schway_config has an explicit reserved0 so the allocator sits at offset 8 on every target"
  - "State header is 20 bytes (magic, state format, total size, body CRC-32); size mismatch and out-of-range body fields give ERR_STATE_FIELD, bad magic ERR_STATE_MAGIC, other format ERR_STATE_VERSION, bad CRC ERR_STATE_CRC"
  - "The profile lock is set by the first run_frame and is not cleared by reset or load; the visitor reports only body fields (instance.frame, instance.region, instance.ticks, instance.tick_acc, gpu.vram)"
  - "schwaystation-host takes the core include paths instead of linking the core, so the runner links the core once"

patterns-established:
  - "Seeded files carry no header; each commit body carries the Seeded from line per path"
  - "Gate scripts open with a usage line and a Fault prevented line"

requirements-completed: [FRAME-01, FRAME-02]

coverage:
  - id: D1
    description: "Runner prints the pinned NTSC-U frame-60 card hash, equal to an independent model"
    requirement: FRAME-02
    verification:
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.hash"
        status: pass
      - kind: other
        ref: "python3 -I card_model.py (scratchpad one-off) exit 0; build/tracer/schwaystation-run --frames 60 --hash exit 0; both print be3538480b252f5bcfe7cb5b876a71c72d3795621b4278e405af61c425c06d71"
        status: pass
    human_judgment: false
  - id: D2
    description: "The card moves: frame 5 and frame 60 hash differently"
    requirement: FRAME-02
    verification:
      - kind: other
        ref: "build/tracer/schwaystation-run --frames 5 --hash vs --frames 60 --hash, differ, exit 0"
        status: pass
    human_judgment: false
  - id: D3
    description: "cmake --workflow --preset ci configures, builds and tests; the registered set equals the union of the directories ci.txt selects; every selected test ran once"
    requirement: FRAME-01
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci exit 0"
        status: pass
      - kind: integration
        ref: "cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake exit 0"
        status: pass
    human_judgment: false
  - id: D4
    description: "The inventory reader fails on an unsorted list, a name in two directories and a selected directory with no inventory.txt, and its union ignores selection order"
    requirement: FRAME-01
    verification:
      - kind: other
        ref: "cmake -DSEL=<scratch sel.txt> -DDIR=<scratch tests> -P <scratch driver including tests/cmake/inventory.cmake>: unsorted exit 1, duplicate exit 1, missing exit 1, reordered selection exit 0"
        status: pass
    human_judgment: false
  - id: D5
    description: "An empty selection configures and a test preset selecting nothing fails through noTestsAction error"
    requirement: FRAME-01
    verification:
      - kind: integration
        ref: "cmake --preset hygiene exit 0; ctest --preset hygiene exit 8 (non-zero, as required)"
        status: pass
    human_judgment: false
  - id: D6
    description: "Public header is the complete v1 surface with no deferred name; layouts pinned by _Static_assert; no floating point in src or include"
    requirement: FRAME-01
    verification:
      - kind: other
        ref: "grep -c schway_ include/schwaystation.h = 33; deferred-name grep count 0; grep -c _Static_assert src/internal.h = 73; float|double count 0"
        status: pass
    human_judgment: false
  - id: D7
    description: "No planning ID and no seed name in code, tests, config or scripts"
    verification:
      - kind: other
        ref: "git grep --untracked planning-ID scan over CMakeLists.txt CMakePresets.json cmake include src host runner tests scripts/hygiene.sh; test $? -eq 1 exit 0; cruft grep a=1 b=1 exit 0"
        status: pass
    human_judgment: false

duration: 13min
completed: 2026-10-20
status: complete
---

# Phase 1 Plan 01: CMake skeleton, public header and the test-card tracer Summary

**The animated PS1 test card is drawn into a 1024 x 512 VRAM, scanned out by `schway_get_video`, hashed by `schwaystation-run --frames 60 --hash` and pinned by `runner.hash` in `cmake --workflow --preset ci`, on a muhtari-style OBJECT-library build with per-directory exact test inventories.**

## Performance

- **Duration:** 13 min (wall time)
- **Started:** 2026-10-20T02:46:49Z
- **Completed:** 2026-10-20T02:59:32Z
- **Tasks:** 3
- **Files modified:** 55

## Accomplishments

- `include/schwaystation.h`: all 14 phase-1 functions, 11 types and 17 constants, C17 and C++17 guard, `SCHWAY_API_VERSION 1`, no deferred name.
- Core: `schway_create`/`destroy` (one allocation of the instance size, one free of the same size), profile with region check and lock, POWER reset, info rationals, `run_frame` on the exact accumulator (737/738 silent pairs NTSC), `get_video` from the display words, `get_audio` from a 4-frame ring, state save/load/visit sharing one walk, `schway_status_string` by `switch` over file-scope arrays.
- Pins computed independently first: `python3 -I card_model.py` (scratchpad, not committed) exit 0 printed NTSC-U frame 60 `be3538480b252f5bcfe7cb5b876a71c72d3795621b4278e405af61c425c06d71`, frame 5 `c3409c280e768d81ce09c5de0eb2d7e09e8d589bf740672b560b174ff583946c`, NTSC-J frame 60 `547d307f...`, PAL frame 60 `8b8f3d0a...`; the built runner printed the same four values (exit 0). They also equal research's candidates.
- Build skeleton: presets, toolchains, `schway_add_test` (timeout scale, `PASS_REGEX`), deferred exact-inventory check over the directory union, `check_junit.cmake` on the same union, every `tests/<dir>` pair and all four selections.

## Task Commits

1. **Task 1: Tracer - card to VRAM to get_video to the runner hash** - `3502d26` (feat)
2. **Task 2: Presets, toolchains, test registration and the per-directory inventories** - `c49653c` (feat)
3. **Task 3: The tracer's pinned test, the empty-preset control and the cruft cleanup** - `7bbdd97` (test)

## Evidence (command, exit status; run from the repository root)

- `cmake -S . -B build/tracer -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_COMPILE_WARNING_AS_ERROR=ON -DSCHWAYSTATION_BUILD_TESTS=OFF && cmake --build build/tracer && build/tracer/schwaystation-run --frames 60 --hash | grep -E '^frame 60 sha256 [0-9a-f]{64}$'`: exit 0 (re-run from a clean build directory for the tracer gate: exit 0).
- `cmake --preset dev && test -f build/dev/CTestTestfile.cmake`: exit 0.
- `cmake --workflow --preset ci`: exit 0 (final run from a clean `build/ci`: exit 0).
- `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake`: exit 0.
- `cmake --workflow --preset dev`: exit 0.
- `cmake --preset hygiene`: exit 0; `ctest --preset hygiene`: exit 8 (no test selected; `noTestsAction` is `error`; plan 01-06 adds the first hygiene test); `ctest --preset hygiene; test $? -ne 0`: exit 0.
- `sh scripts/hygiene.sh --tree`: exit 0.
- Scratch inventory copy, driver including `tests/cmake/inventory.cmake`, run through `cmake -P`: out-of-order line exit 1 naming the file; same name in two directories exit 1 naming the name; selected directory without `inventory.txt` exit 1 naming the missing file; reordered selection exit 0 with a sorted union. Nothing committed.
- Cruft grep: `git grep --untracked -nIi -e cogmaster -e muhtari -e 'master system' -e 7800 -e palette -- . ':!.planning' ':!PROVENANCE.md' ':!ASSET_POLICY.md' ':!THIRD-PARTY-NOTICES.md' ':!libretro/libretro.h'; a=$?; git grep --untracked -nIiw -e sms -- <same pathspecs>; b=$?; test "$a" -eq 1 && test "$b" -eq 1`: a=1, b=1, exit 0.
- Planning-ID scan: `git grep --untracked -nE -e 'D-[0-9]{2}' -e 'FRAME-[0-9]' -e 'XFER-[0-9]' -e 'DEC\.[0-9]' -- CMakeLists.txt CMakePresets.json cmake include src host runner tests scripts/hygiene.sh; test $? -eq 1`: exit 0.
- Gate-line ceiling: `test "$(cat tests/cmake/inventory.cmake tests/cmake/expect_fail.cmake tests/cmake/check_junit.cmake | wc -l)" -le 130`: exit 0 (113 lines). `wc -l scripts/*.sh tests/cmake/*.cmake .githooks/* | tail -1`: 924 total (baseline 812, minus 1 for the hygiene.sh line, plus 113).
- Presets `nofp`, `be32`, `wasm32`, `asan` and `perf` were not run here: GCC, qemu-ppc, wasmtime, clang-21 and Valgrind are absent on this host. They are first exercised by the hosted run in plan 01-14; no substitute was run.

**How the cruft grep reads D-27:** D-27 gives one `git grep -nIiE` over the whole tree. Every plan runs it with pathspec exclusions for `.planning`, `PROVENANCE.md`, `ASSET_POLICY.md`, `THIRD-PARTY-NOTICES.md` and the unmodified vendored `libretro/libretro.h` (files that must name the seeds or that this project may not edit), as a word match for `sms` (a substring match also hits words such as "mechanisms"), and with `--untracked` so new files not yet committed are scanned.

## Files Created/Modified

- `include/schwaystation.h` - the v1 public API.
- `src/internal.h` - instance struct, timing constants, 73 layout pins, `SCHWAY__<T>_SIZE_FIRST`.
- `src/instance.c`, `src/frame.c` - lifetime, profile, reset, info, status text; frame, video, audio.
- `src/testcard.c` - `schway__testcard_pixel` and the whole-card draw.
- `src/state.c`, `src/state.h`, `src/crc32.c` - the state walk, header, CRC-32.
- `host/xrgb8888.{c,h}` - mask, 5-to-8 bit replication, frame conversion.
- `runner/*` - `schwaystation-run` with `--frames`, `--hash`, `--region`, `--help`, `--version`; statuses 0, 1, 2.
- `CMakeLists.txt`, `CMakePresets.json`, `cmake/*` - build, presets, packaging, package config, toolchains, test registration.
- `tests/*` - inventory reader, JUnit check, negative-control runner, `check.h`, pins, selections, directory pairs, `runner.hash`.
- `scripts/hygiene.sh` - adaptation comment line removed.

## Decisions Made

See `key-decisions` above. All are within the plan's interface block and CONTEXT; none reopens a settled choice.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Empty inventory compared as unsorted**
- **Found during:** Task 2 (first `cmake --preset dev`)
- **Issue:** a comment-only `inventory.txt` leaves the sorted copy unset, and `if(NOT sorted STREQUAL lines)` compared the literal name "sorted" with an empty string, so every empty directory failed as unsorted.
- **Fix:** compare `"${sorted}"` with `"${lines}"`.
- **Files modified:** tests/cmake/inventory.cmake
- **Verification:** `cmake --preset dev` exit 0; `cmake --preset hygiene` exit 0; scratch reader checks above.
- **Committed in:** c49653c (Task 2 commit)

**2. [Rule 3 - Blocking] Duplicate core library on the runner's link line**
- **Found during:** Task 1 (first tracer build)
- **Issue:** `schwaystation-host` linked the core publicly and the runner linked it again; macOS ld warned "ignoring duplicate libraries".
- **Fix:** the host library takes the core's include directories privately instead of linking it; the runner links both once.
- **Files modified:** CMakeLists.txt
- **Verification:** rebuilt with no warning; tracer verify exit 0.
- **Committed in:** 3502d26 (Task 1 commit)

**3. [Note] Seed has no `runner/CMakeLists.txt`**
- The seed builds the runner from its top-level file; `runner/CMakeLists.txt` was written new as the plan specifies, and its commit body names only paths that exist at the seed pin.

---

**Total deviations:** 2 auto-fixed (1 bug, 1 blocking). **Impact:** both were needed for a clean configure and a warning-free build; no scope change.

## Issues Encountered

- `cmake/packaging.cmake` installs `THIRD-PARTY-NOTICES.md`, which plan 01-02 (same wave) adds. Configure and the `ci` workflow do not need it; the package step (plan 01-11) does.

## Known Stubs

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plans 01-03 to 01-08 append only to their own `tests/<dir>` pair and read the header as written.
- The `ci` and `ci-msvc` workflows are configure, build and test; plan 01-11 adds the package and `ci-release` steps.

---
*Phase: 01-a-test-frame-in-retroarch*
*Completed: 2026-10-20*

## Self-Check: PASSED
