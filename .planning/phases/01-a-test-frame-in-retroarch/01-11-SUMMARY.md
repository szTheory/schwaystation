---
phase: 01-a-test-frame-in-retroarch
plan: 11
subsystem: packaging, release, docs
tags: [cpack, zip, release, archive-contents, install-line, readme, retroarch]
status: complete

requires:
  - phase: 01-07
    provides: "docs.budgets"
  - phase: 01-08
    provides: "install components library, runner, libretro; cmake/packaging.cmake with version-free CPack ZIP names; the ci and ci-msvc package presets and the ci-release and ci-msvc-release test presets"
  - phase: 01-09
    provides: "the runner in the runner component"
  - phase: 01-10
    provides: "the libretro module and its .info in the libretro component"
provides:
  - "The ci and ci-msvc workflow presets: configure, build, test, package, then the release-labelled test preset"
  - "Per leg: schwaystation-library-<os>-<arch>.zip, schwaystation-runner-<os>-<arch>.zip, schwaystation-libretro-<os>-<arch>.zip, no version and no top-level folder, each with LICENSE and THIRD-PARTY-NOTICES.md"
  - "CTests release.archive_contents and release.archive_contents.self_test (label release)"
  - "tests/cmake/install_line.cmake (README, BASE_URL, FLIP, HOME_DIR) and the README's single macOS arm64 install line"
affects: [01-12, 01-13, 01-14]

actuals:
  tokens: 3300
  tasks: 2
  commits: 2
plan_head_before: 7ba1b56ec59afd6bcfcd27c122e5787a21674920
plan_head_after: e521dfe11291b7214e5c0e35b8e8ee75d44e9a6e

tech-stack:
  added: []
  patterns:
    - "A release test reads the archives the package step wrote, so it runs in a second test preset after packaging and the first preset excludes its label"
    - "The README install line is extracted by regex and run under sh with HOME set to a scratch directory and only the download base swapped for file://"

key-files:
  created:
    - tests/cmake/archive_contents.cmake
    - tests/cmake/install_line.cmake
  modified:
    - CMakePresets.json
    - tests/release/tests.cmake
    - tests/release/inventory.txt
    - README.md

key-decisions:
  - "The install line keeps the seed's form: mkdir, a mktemp directory, curl of the zip and SHA256SUMS, shasum -a 256 -c --ignore-missing, then tar -xf of cores and info into ~/Library/Application Support/RetroArch, so a damaged download installs nothing"
  - "The README also gives the manual install for other systems and the core_info.cache note, and stays at 3726 bytes"
  - "release.floor is not ported; only the two archive-contents tests carry the release label"

requirements-completed: []

coverage:
  - id: D1
    description: "cmake --workflow --preset ci configures, builds, tests, packages and runs the ci-release preset, writing the three version-free macOS arm64 zips"
    requirement: FRAME-01
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci exit 0 (77 tests, then 2 release tests); ls of the three build/ci/schwaystation-*-macos-arm64.zip exit 0"
        status: pass
    human_judgment: false
  - id: D2
    description: "Every release-labelled test ran exactly once, and every other inventoried test ran exactly once in the first test step"
    requirement: FRAME-01
    verification:
      - kind: integration
        ref: "cmake -DJUNIT=build/ci/junit-release.xml -DINVENTORY=tests/inventory/ci.txt -DLABELS=release -P tests/cmake/check_junit.cmake exit 0; the same with build/ci/junit.xml and -DEXCLUDE=release exit 0"
        status: pass
    human_judgment: false
  - id: D3
    description: "release.archive_contents checks each zip holds exactly its expected members with LICENSE and THIRD-PARTY-NOTICES.md at the root, byte-identical to the tree's, and no source or build path"
    requirement: FRAME-06
    verification:
      - kind: integration
        ref: "tests/release/tests.cmake#release.archive_contents"
        status: pass
    human_judgment: false
  - id: D4
    description: "A libretro zip without THIRD-PARTY-NOTICES.md fails the archive check, naming the missing member"
    requirement: FRAME-06
    verification:
      - kind: integration
        ref: "tests/release/tests.cmake#release.archive_contents.self_test"
        status: pass
    human_judgment: false
  - id: D5
    description: "The libretro zip lists LICENSE, THIRD-PARTY-NOTICES.md, cores/schwaystation_libretro.dylib and info/schwaystation_libretro.info"
    requirement: FRAME-06
    verification:
      - kind: other
        ref: "unzip -l build/ci/schwaystation-libretro-macos-arm64.zip piped to grep -E -c on the four names prints 4"
        status: pass
    human_judgment: false
  - id: D6
    description: "The single README install line installs the core and info file into a scratch HOME's RetroArch directory from a file:// base holding this leg's zip"
    requirement: FRAME-07
    verification:
      - kind: e2e
        ref: "cmake -DREADME=README.md -DBASE_URL=file://$PWD/build/ci/ -P tests/cmake/install_line.cmake exit 0"
        status: pass
    human_judgment: false
  - id: D7
    description: "With the zip's last byte flipped the install line exits non-zero and installs nothing"
    requirement: FRAME-07
    verification:
      - kind: e2e
        ref: "cmake -DREADME=README.md -DBASE_URL=file://$PWD/build/ci/ -DFLIP=1 -P tests/cmake/install_line.cmake exit 0 (the line exited 1)"
        status: pass
    human_judgment: false
  - id: D8
    description: "README holds exactly one line beginning mkdir -p ~/Library/Application, stays under 16384 bytes and within docs.budgets"
    requirement: FRAME-07
    verification:
      - kind: other
        ref: "grep -c '^mkdir -p ~/Library/Application' README.md prints 1; wc -c is 3726; ctest --test-dir build/ci -R '^docs\\.budgets$' --no-tests=error exit 0"
        status: pass
    human_judgment: false

duration: 4min
completed: 2026-10-20
---

# Phase 1 Plan 11: Release Archives and the README Install Line Summary

The `ci` workflow now packages each leg's three version-free zips and checks their contents in a release-labelled test step. The README's one macOS arm64 install line is proven locally: it installs the core where RetroArch finds it, and it refuses a tampered archive.

## Performance

- **Duration:** about 4 min wall time (2026-10-20T15:52:20Z to 15:55:43Z, before the SUMMARY)
- **Started:** 2026-10-20T15:52:20Z
- **Completed:** 2026-10-20
- **Tasks:** 2
- **Files modified:** 6 (2 created, 4 modified)

## Accomplishments

- The `ci` and `ci-msvc` workflow presets run five steps: configure, build, test, package, and then `ci-release` or `ci-msvc-release`.
- `release.archive_contents` checks the library, runner and libretro zips. Each must hold exactly its members, with no top-level folder. The notices must be byte-identical to the tree's, and no member may contain a source or build path. Its self test zips a fixture without the notices file and expects `lacks: THIRD-PARTY-NOTICES.md`.
- The README install section has the single line, a sentence on RetroArch 1.22.2 (Metal) and `info/core_info.cache`, and the manual install for other systems.
- `tests/cmake/install_line.cmake` runs that line verbatim with a scratch HOME. It also covers the flipped-byte control.

## Task Commits

1. **Task 1: package step to three zips to release.archive_contents** - `189669f` (feat)
2. **Task 2: the README install line and its local proof** - `e521dfe` (feat)

## Files Created/Modified

- `CMakePresets.json`: package and release-test steps in the `ci` and `ci-msvc` workflows
- `tests/cmake/archive_contents.cmake`: the archive check (created in Task 1; blank lines trimmed in Task 2 to meet the line budget)
- `tests/release/tests.cmake`, `tests/release/inventory.txt`: the two release tests
- `tests/cmake/install_line.cmake`: the install-line runner
- `README.md`: the install section

## Evidence (commands from the repository root, exit status)

- `cmake --workflow --preset ci`: exit 0, after each task (77 tests, then 2 release tests, 100% passed)
- `ls build/ci/schwaystation-library-macos-arm64.zip build/ci/schwaystation-runner-macos-arm64.zip build/ci/schwaystation-libretro-macos-arm64.zip`: exit 0
- `cmake -DJUNIT=build/ci/junit-release.xml -DINVENTORY=tests/inventory/ci.txt -DLABELS=release -P tests/cmake/check_junit.cmake`: exit 0 (2 tests ran once each). This was also the tracer gate re-run.
- `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake`: exit 0 (77)
- `unzip -l build/ci/schwaystation-libretro-macos-arm64.zip | grep -E -c '(^| )(LICENSE|THIRD-PARTY-NOTICES\.md|cores/schwaystation_libretro\.dylib|info/schwaystation_libretro\.info)$'`: printed 4
- `grep -c 'release.floor' tests/release/tests.cmake`: printed 0
- `cmake -DREADME=README.md "-DBASE_URL=file://$PWD/build/ci/" -P tests/cmake/install_line.cmake`: exit 0
- `cmake -DREADME=README.md "-DBASE_URL=file://$PWD/build/ci/" -DFLIP=1 -P tests/cmake/install_line.cmake`: exit 0 (shasum refused the copy, so the line exited 1 and installed nothing)
- `grep -c '^mkdir -p ~/Library/Application' README.md`: printed 1. `wc -c < README.md`: 3726, under 16384.
- `ctest --test-dir build/ci -R '^docs\.budgets$' --no-tests=error`: exit 0
- Plan line ceiling: `cat tests/cmake/archive_contents.cmake tests/cmake/install_line.cmake | wc -l` printed 174, within the limit of 175. The gate-line total `wc -l scripts/*.sh tests/cmake/*.cmake .githooks/* | tail -1` printed 1344, within the `policy.gate_budget` limit of 1500.
- Cruft grep (the plan's two `git grep --untracked` commands): a=1, b=1, exit 0
- Planning-ID scan `git grep --untracked -nE -e 'D-[0-9]{2}' -e 'FRAME-[0-9]' -e 'XFER-[0-9]' -e 'DEC\.[0-9]' -- CMakePresets.json cmake/packaging.cmake tests/release tests/cmake/archive_contents.cmake tests/cmake/install_line.cmake README.md`: exit 1, so there are no matches
- `cmake --workflow --preset hygiene`: exit 0 (15 tests)

## Decisions Made

See `key-decisions` in the frontmatter.

## Deviations from Plan

### Auto-fixed Issues

None.

### Plan interpretation

- Plan 01-08 had already done the packaging part of Task 1:
  - `cmake/packaging.cmake`: per-component ZIPs, version-free names, LICENSE and notices in every component, no top-level directory
  - the `ci` and `ci-msvc` package presets
  - the `ci-release` and `ci-msvc-release` test presets, with the include label `^release$` and JUnit to `build/<preset>/junit-release.xml`

  The file already matched the seed apart from renames, so this plan left it unchanged. This plan added only the two workflow steps and the release tests.
- To meet the 175-line ceiling, I trimmed comment and blank lines in the two gate files. No command changed.

## Issues Encountered

None. The known `dev`-preset `abi.undefined` failure on macOS Debug is still in deferred-items.md. This plan did not touch it.

## Known Stubs

None.

## Next Phase Readiness

Plans 01-12 and 01-13 add the ci.yml step that runs the install line on the macOS arm64 leg, and the release workflow's `install-line-latest` job. FRAME-01, FRAME-06 and FRAME-07 stay open in REQUIREMENTS.md, because later plans in the phase still declare them.

## Self-Check: PASSED

- FOUND: tests/cmake/archive_contents.cmake, tests/cmake/install_line.cmake, README.md install line
- FOUND: 189669f, e521dfe (ancestors of HEAD)
