---
phase: 01-a-test-frame-in-retroarch
plan: 08
subsystem: testing, install, packaging
tags: [cmake, ctest, install, find_package, pkg-config, add_subdirectory, embedding]
status: complete

requires:
  - phase: 01-01
    provides: "install rules, the CMake package (SameMinorVersion), the .pc file, schway_add_test and the per-directory inventories"
  - phase: 01-02
    provides: "the vendored libretro header, notices and removed setup script the cruft grep runs over"
provides:
  - "install.stage (FIXTURES_SETUP install_staged) installing the library component into <build>/stage"
  - "install.consumer, install.consumer.dll_crt and install.consumer.cxx: tests/consumer built with find_package against the stage, each in its own build directory"
  - "embed.subdirectory: tests/embed embeds the source tree with tests, runner and libretro off and runs one frame"
  - "tests/consumer/{CMakeLists.txt,main.c,main.cpp} and tests/embed/CMakeLists.txt"
affects: [01-10, 01-14]

actuals:
  tokens: 2600
  tasks: 2
  commits: 2
plan_head_before: f12f922fcb5c9556fdedaed6dca9dd9b3d861c9b
plan_head_after: eaa68cf56371d65633d93955a4300fef21857607

tech-stack:
  added: []
  patterns:
    - "One CMake file is both the embedding project and, in script mode, the post-build check that reads the subdirectory's generated cmake_install.cmake for a file(INSTALL rule"
    - "Consumer rows are name|build dir|MSVC runtime|program|expected text, expanded by one foreach into ctest --build-and-test tests"

key-files:
  created:
    - tests/consumer/CMakeLists.txt
    - tests/consumer/main.c
    - tests/consumer/main.cpp
    - tests/embed/CMakeLists.txt
  modified:
    - tests/install/tests.cmake
    - tests/install/inventory.txt

key-decisions:
  - "embed.subdirectory accepts exactly the targets schwaystation, schwaystation_instance, schwaystation_state and schwaystation_testcard: the library is built from per-module object libraries, so the seed's single-target check would always fail"
  - "embed.subdirectory also fails when the embedded tree adds an install rule, checked after the build against the subdirectory's cmake_install.cmake, so the must-have 'no install rule appears' is tested rather than assumed"
  - "Install and embed tests carry only the install label, as in the seed, so the be32 and wasm32 lanes (which run only portable tests) register them without running a cross-compiled consumer"

requirements-completed: [FRAME-08]

coverage:
  - id: D1
    description: "install.stage installs both headers, the static library and the package into build/ci/stage, and install.consumer builds tests/consumer with find_package against it and prints the api 1, 320x240 line"
    requirement: FRAME-08
    verification:
      - kind: integration
        ref: "tests/install/tests.cmake#install.stage"
        status: pass
      - kind: integration
        ref: "tests/install/tests.cmake#install.consumer"
        status: pass
    human_judgment: false
  - id: D2
    description: "install.consumer.cxx builds a C++17 program against the installed header; install.consumer.dll_crt builds the C consumer on the DLL C runtime"
    requirement: FRAME-08
    verification:
      - kind: integration
        ref: "tests/install/tests.cmake#install.consumer.cxx"
        status: pass
      - kind: integration
        ref: "tests/install/tests.cmake#install.consumer.dll_crt"
        status: pass
    human_judgment: false
  - id: D3
    description: "embed.subdirectory builds a C-only project that pulls in the source tree with add_subdirectory, gets only the library and its alias, no C++, no tests and no install rule, and runs one frame"
    requirement: FRAME-08
    verification:
      - kind: integration
        ref: "tests/install/tests.cmake#embed.subdirectory"
        status: pass
      - kind: other
        ref: "cmake -DINSTALL_SCRIPT=build/ci/cmake_install.cmake -P tests/embed/CMakeLists.txt exits 1 (the top-level tree has install rules); against build/ci/embed/schwaystation/cmake_install.cmake it exits 0"
        status: pass
    human_judgment: false
  - id: D4
    description: "The consumers require the install_staged fixture and build in their own directories, so parallel CTest from an empty stage passes"
    requirement: FRAME-08
    verification:
      - kind: other
        ref: "ctest --test-dir build/ci -L install -j 4 exits 0, run once with the stage and consumer build directories removed first"
        status: pass
    human_judgment: false
  - id: D5
    description: "Each install and embed test runs exactly once in the ci preset, and the five are the install inventory"
    requirement: FRAME-08
    verification:
      - kind: other
        ref: "cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake exits 0"
        status: pass
    human_judgment: false

duration: 6 min
completed: 2026-10-20
---

# Phase 1 Plan 08: Install, consumer and embed tests Summary

**The installed package is built against from C (static and DLL C runtime) and C++ with find_package, and the source tree embeds through add_subdirectory with only the library: five install-labelled CTests, one fixture.**

## Performance

- **Duration:** about 6 min
- **Started:** 2026-10-20T03:59:57Z
- **Completed:** 2026-10-20T04:05Z
- **Tasks:** 2
- **Files modified:** 6 (4 created, 2 modified)

## Accomplishments

- `install.stage` installs the `library` component into `build/<preset>/stage` and sets up `install_staged`.
- `install.consumer` builds `tests/consumer` with `find_package(schwaystation CONFIG REQUIRED)` against the stage and
  prints `consumer: api 1, one frame 320x240`; where pkg-config exists (not here, on macOS) the same build also
  links `consumer_pc` from `schwaystation.pc` alone.
- `install.consumer.cxx` prints `consumer_cxx: api 1`; `install.consumer.dll_crt` builds the C consumer with
  `CMAKE_MSVC_RUNTIME_LIBRARY` set to the DLL runtime.
- `embed.subdirectory` fails on C++ being enabled, any target besides the library and its object libraries, a
  missing alias, registered tests, or (after the build) an install rule; then runs the consumer's frame.

## Task Commits

1. **Task 1: Tracer - install.stage to install.consumer** - `ca6d550` (test)
2. **Task 2: C++ consumer, DLL-runtime consumer and the embed** - `eaa68cf` (test)

The tracer gate re-ran `ctest --test-dir build/ci -R '^install\.(stage|consumer)$'` (exit 0) before Task 2.

## Evidence

All commands from the repository root:

| Command | Exit |
|---------|------|
| `cmake --workflow --preset ci` (after both commits; 67 tests) | 0 |
| `cmake --workflow --preset hygiene` (15 tests) | 0 |
| `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake` | 0 |
| `ctest --test-dir build/ci -R '^install\.(stage\|consumer)$' --output-on-failure` | 0 |
| `ctest --test-dir build/ci -N -L install \| grep -c 'Test #'` | 0, prints 5 |
| `ctest --test-dir build/ci -L install -j 4` (once warm, once with stage and consumer dirs removed) | 0, 0 |
| `test -f build/ci/stage/include/schwaystation.h && test -f build/ci/stage/include/schwaystation_version.h` | 0 |
| `grep -c 'SameMinorVersion' CMakeLists.txt` | 0, prints 1 |
| `grep -c 'src/' tests/consumer/main.c tests/consumer/CMakeLists.txt tests/consumer/main.cpp tests/embed/CMakeLists.txt` | prints 0 for each |
| Planning-ID scan `git grep --untracked -nE -e 'D-[0-9]{2}' -e 'FRAME-[0-9]' -e 'XFER-[0-9]' -e 'DEC\.[0-9]' -- tests/install tests/consumer tests/embed` | 1 (no match; baseline at HEAD also 1) |
| Cruft grep (both `git grep` calls of the plan's Task 2 verify) | a=1, b=1 |

## Decisions Made

See `key-decisions` in the frontmatter: the embed target check names the library's object libraries, the embed
also checks for install rules, and the install tests stay off the cross lanes' `portable` filter as in the seed.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] The seed's embed target check could never pass here**
- **Found during:** Task 2
- **Issue:** The seed required `BUILDSYSTEM_TARGETS` to equal the one library target; this library is built from
  three object libraries plus the archive.
- **Fix:** Sort the list and compare with the four names.
- **Files modified:** tests/embed/CMakeLists.txt
- **Commit:** eaa68cf

**2. [Rule 2 - Missing check] Install rules in an embedding build were not tested**
- **Found during:** Task 2
- **Issue:** The plan's must-have says no install rule of this project appears in the embedding build; the seed
  checked targets and tests only.
- **Fix:** A post-build step runs the same file in script mode over the subdirectory's `cmake_install.cmake` and
  fails on `file(INSTALL`. Verified once to exit 1 against the top-level build's script.
- **Files modified:** tests/embed/CMakeLists.txt
- **Commit:** eaa68cf

**Total deviations:** 2 auto-fixed (1 bug, 1 missing check). **Impact:** none on scope; both inside the plan's files.

## Defects for plan 01-10

None. No package-file defect appeared; `CMakeLists.txt` and `cmake/` were not edited.

## Issues Encountered

- pkg-config is not installed on this host, so the `consumer_pc` build path did not run locally; it runs on CI legs
  that have pkg-config.
- The ci-msvc leg (where `install.consumer.dll_crt` differs from `install.consumer`) was not run locally; it runs in
  CI.

## Next Phase Readiness

FRAME-08 stays open in REQUIREMENTS.md: plan 01-14 also declares it. Ready for the next plan in phase 1.

## Self-Check: PASSED

- FOUND: tests/consumer/CMakeLists.txt, tests/consumer/main.c, tests/consumer/main.cpp, tests/embed/CMakeLists.txt
- FOUND: ca6d550, eaa68cf (ancestors of HEAD)
