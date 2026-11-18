---
phase: 01-a-test-frame-in-retroarch
plan: 09
subsystem: runner, testing
tags: [runner, bench, percentiles, signals, revision, cmake, ctest, docs]
status: complete

requires:
  - phase: 01-04
    provides: "schwaystation-run with --frames, --hash, --png, --region, the exit_status.h table and the runner test file"
provides:
  - "schwaystation-run --bench N: frames, nearest-rank p50 to p99.999, max_us and max_rss_kib, one whole number per line"
  - "Clean stop on SIGINT and SIGTERM after a whole frame, exit 0"
  - "--version prints the version and the configure-time Git revision from a generated header (runner only, never the core)"
  - "tests/cmake/revision.cmake: build.revision_matches_head and its self-test"
  - "runner.bench.percentile, runner.bench.format, runner.bench.bytes, runner.stop, runner.exit.failure, runner.version"
  - "SCHWAYSTATION_RUNNER_SOURCES and SCHWAYSTATION_RUNNER_INCLUDE for a second runner built by the tests"
  - "docs/runner.md"
affects: [01-10, 01-12, 01-13, 01-14, 01-15]

actuals:
  tokens: 10300
  tasks: 3
  commits: 3
plan_head_before: e84bc78504fb697eefb6c15a0271ad3c0eb173a2
plan_head_after: 58792b90cb72d02869e802a9fde67863fb9960d4

tech-stack:
  added: []
  patterns:
    - "Runner-only host reads (monotonic clock, peak RSS, signals) in one small file each; the core reads none"
    - "A second runner built from the same source list with a compile-time hook (SCHWAY_RUN_RAISE_AT) that raises SIGTERM inside a chosen frame"
    - "Configure-time revision in a generated header, checked against git rev-parse HEAD at test time"

key-files:
  created:
    - runner/bench.c
    - runner/bench.h
    - runner/clock.c
    - runner/clock.h
    - runner/rss.c
    - runner/rss.h
    - runner/stop.c
    - runner/stop.h
    - tests/runner/test_runner.c
    - tests/cmake/revision.cmake
    - docs/runner.md
  modified:
    - runner/main.c
    - runner/CMakeLists.txt
    - runner/exit_status.h
    - tests/runner/tests.cmake
    - tests/runner/inventory.txt

key-decisions:
  - "The --bench count shares the --frames limit of 1 to 1000000; the seed's larger buffer cap stays in bench.h as the arithmetic bound runner.bench.bytes pins"
  - "runner.stop raises SIGTERM inside frame 5, not the seed's frame 3, so the expected hash is the existing frame-5 card pin"
  - "The revision is read with git rev-parse at every configure, never at build time, so a commit after a configure leaves it stale and build.revision_matches_head names both revisions; outside a Git checkout it is unknown"
  - "runner.version uses a tolerant regex accepting a hex revision or unknown"

requirements-completed: [FRAME-05, FRAME-09]

coverage:
  - id: D1
    description: "--bench N prints frames then nearest-rank p50_us, p90_us, p99_us, p99.9_us, p99.99_us, p99.999_us, max_us and max_rss_kib in that order, one whole number each"
    requirement: FRAME-09
    verification:
      - kind: unit
        ref: "tests/runner/tests.cmake#runner.bench.percentile"
        status: pass
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.bench.format"
        status: pass
      - kind: other
        ref: "build/ci/schwaystation-run --bench 600 piped to grep -E -c for the eight value lines prints 8, exit 0"
        status: pass
    human_judgment: false
  - id: D2
    description: "A sample count whose buffer would wrap, and --bench 0, negative or missing, exit 2 before anything runs"
    requirement: FRAME-09
    verification:
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.bench.bytes"
        status: pass
    human_judgment: false
  - id: D3
    description: "The clock and RSS reads live in the runner; no core source calls clock_gettime, getrusage or QueryPerformance"
    requirement: FRAME-09
    verification:
      - kind: other
        ref: "grep -E -c 'clock_gettime|getrusage|QueryPerformance' src/*.c prints 0 for every file"
        status: pass
    human_judgment: false
  - id: D4
    description: "SIGINT or SIGTERM ends the run after the current frame, prints the hash or bench report for the frames that ran, and exits 0"
    requirement: FRAME-09
    verification:
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.stop"
        status: pass
    human_judgment: false
  - id: D5
    description: "An unwritable --png path exits 1 with a cannot-write message"
    requirement: FRAME-09
    verification:
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.exit.failure"
        status: pass
    human_judgment: false
  - id: D6
    description: "--version prints schwaystation-run, the version and the 40-hex revision embedded at configure"
    requirement: FRAME-05
    verification:
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.version"
        status: pass
    human_judgment: false
  - id: D7
    description: "build.revision_matches_head fails, naming both revisions, when the embedded revision differs from git rev-parse HEAD; its self-test forces forty zeros and expects exit 1 and the mismatch message"
    requirement: FRAME-05
    verification:
      - kind: integration
        ref: "tests/runner/tests.cmake#build.revision_matches_head"
        status: pass
      - kind: integration
        ref: "tests/runner/tests.cmake#build.revision_matches_head.self_test"
        status: pass
    human_judgment: false
  - id: D8
    description: "docs/runner.md lists every phase-1 flag with an example, the hash definition, the bench report and the exit statuses 0, 1 and 2, with no planning IDs"
    requirement: FRAME-09
    verification:
      - kind: other
        ref: "grep -E -c 'D-[0-9]|FRAME-|XFER-|DEC\\.' docs/runner.md prints 0; the doc is under 61440 bytes and mentions --bench"
        status: pass
    human_judgment: false
  - id: D9
    description: "Every runner test is inventoried and ran exactly once in the ci preset"
    requirement: FRAME-05
    verification:
      - kind: other
        ref: "cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake exits 0 (75 tests)"
        status: pass
    human_judgment: false

duration: about 11 h 30 min wall, including a checkpoint interruption
completed: 2026-10-20
---

# Phase 1 Plan 09: Runner bench, stop, version and revision gate Summary

**`schwaystation-run --bench N` prints nearest-rank frame-time tails to p99.999 plus peak RSS from runner-only
clock and RSS reads; SIGINT and SIGTERM stop after a whole frame with exit 0; `--version` carries the
configure-time Git revision, and `build.revision_matches_head` catches a stale configure by name.**

## Performance

- **Started:** 2026-10-20T04:07:43Z
- **Completed:** 2026-10-20T15:38Z
- **Wall time:** about 11 h 30 min. Most of it was the checkpoint interruption: the first executor stopped at a
  blocking-human checkpoint after the Claude Code auto-mode classifier inconsistently denied Write and Edit calls
  porting the seed's runner code. The owner approved the port in session and this continuation finished all three
  tasks; the active work after the resume took well under an hour.
- **Tasks:** 3
- **Files modified:** 16 (11 created, 5 modified)

## Accomplishments

- `runner/bench.{c,h}`, `clock.{c,h}`, `rss.{c,h}` and `stop.{c,h}` ported with renames; `main.c` times only
  `schway_run_frame`, prints the nine-line report, installs the signal handlers after the command line is valid
  and leaves the frame loop between whole frames when a stop is requested.
- `--frames N or --bench N is required` usage message; `--bench 0`, `-1`, missing, `1000001` and above exit 2.
- `runner/CMakeLists.txt` runs `git rev-parse HEAD` at configure (falls back to `unknown`) into
  `build/<preset>/runner/generated/revision.h`; `--version` prints `schwaystation-run 0.0.0 <revision>`. The
  revision never reaches the core or `schway_get_versions`.
- `tests/cmake/revision.cmake` (26 lines) compares the printed revision with HEAD; `-DREVISION=` overrides it for
  the self-test.
- The `--help` exit table now names a signal stop under 0 and an unwritten file under 1.
- `docs/runner.md` for users: synopsis, options table, examples, frame hash, PNG output, bench report, stopping,
  exit status.

## Task Commits

1. **Task 1: Tracer - `--bench N` from the host clock to the printed percentile report** - `d9d164b` (feat)
2. **Task 2: Signals, the failure status, `--version` and the revision gate** - `015f25b` (feat)
3. **Task 3: docs/runner.md** - `58792b9` (docs)

The tracer gate re-ran `cmake --workflow --preset ci` and the `--bench 600` line count (both exit 0) before Task 2.

## Evidence

All commands from the repository root:

| Command | Exit |
|---------|------|
| `cmake --workflow --preset ci` (after Task 3; 75 tests) | 0 |
| `cmake --workflow --preset hygiene` | 0 |
| `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake` | 0 (75 tests ran once each) |
| `test "$(build/ci/schwaystation-run --bench 600 \| grep -E -c '^((p50\|p90\|p99\|p99\.9\|p99\.99\|p99\.999\|max)_us\|max_rss_kib) [0-9]+$')" -ge 8` | 0 |
| `ctest --test-dir build/ci -L bench` | 0 (3 tests) |
| `grep -E -c 'clock_gettime\|getrusage\|QueryPerformance' src/*.c` | prints 0 for every file |
| `build/ci/schwaystation-run --bench 0; test $? -eq 2` | 0 |
| `ctest --test-dir build/ci -R '^(runner\.(stop\|exit\.failure\|version)\|build\.revision_matches_head)' --output-on-failure` | 0 (5 tests) |
| `build/ci/schwaystation-run --version \| grep -E -c '^schwaystation-run [0-9]+\.[0-9]+\.[0-9]+ [0-9a-f]{40}$'` | 0, prints 1 |
| `ctest --test-dir build/ci -N -R '^build\.revision_matches_head' \| grep -c 'Test #'` | 0, prints 2 |
| `test "$(wc -c < docs/runner.md)" -lt 61440 && grep -c -e '--bench' docs/runner.md` | 0, prints 6 |
| `grep -E -c 'D-[0-9]\|FRAME-\|XFER-\|DEC\.' docs/runner.md` | 1, prints 0 |
| `test "$(cat tests/cmake/revision.cmake \| wc -l)" -le 30` | 0 (26 lines) |
| `wc -l scripts/*.sh tests/cmake/*.cmake .githooks/* \| tail -1` | 0, prints 1170 total |
| Planning-ID scan `git grep --untracked -nE -e 'D-[0-9]{2}' -e 'FRAME-[0-9]' -e 'XFER-[0-9]' -e 'DEC\.[0-9]' -- runner tests/runner tests/cmake/revision.cmake` | 1 (no match; baseline at HEAD also 1) |
| Cruft grep (both `git grep` calls of the plan's Task 3 verify) | a=1, b=1 |

## Decisions Made

See `key-decisions` in the frontmatter.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing wording] The exit table did not describe the new 0 and 1 cases**
- **Found during:** Task 2
- **Issue:** `runner/exit_status.h` (not in this plan's files list, but the 01-04 summary left its wording to this
  plan) said 0 meant "every frame ran" and 1 "memory ran out or the core failed", which no longer covered a signal
  stop or an unwritten file.
- **Fix:** 0 now reads "every frame ran, or SIGINT or SIGTERM ended the run after a whole frame"; 1 reads "a file
  could not be written, memory ran out or the core failed". `docs/runner.md` repeats the table.
- **Files modified:** runner/exit_status.h
- **Commit:** 015f25b

**2. [Rule 1 - Bug] Self-test regex broke on CMake's message wrapping**
- **Found during:** Task 2
- **Issue:** CMake wrapped the mismatch message between "configured at" and the zeros, so the first regex never
  matched.
- **Fix:** The regex allows a break between words (`configured at[^/]*0000...`), as the other self-tests do;
  `find_package(Git REQUIRED QUIET)` drops the status noise.
- **Files modified:** tests/runner/tests.cmake, tests/cmake/revision.cmake
- **Commit:** 015f25b

**Total deviations:** 2 auto-fixed. **Impact:** none on scope.

## Issues Encountered

- The checkpoint interruption described under Performance. No tool call was denied in this continuation.
- Windows (`QueryPerformanceCounter`, `K32GetProcessMemoryInfo`), the wasm32 lane (`max_rss_kib 0`, emulated
  signals already in the toolchain file) and the big-endian lane were not run locally; they run in CI.
- After a commit, an unconfigured `build/ci` fails `build.revision_matches_head` until the next configure. That is
  the gate working; `cmake --workflow --preset ci` configures first.

## Next Phase Readiness

FRAME-05 and FRAME-09 stay open in REQUIREMENTS.md: plans 01-12 to 01-15 also declare them. Plan 01-10 runs next on
the same tree.

## Self-Check: PASSED

- FOUND: runner/bench.c, runner/bench.h, runner/clock.c, runner/clock.h, runner/rss.c, runner/rss.h,
  runner/stop.c, runner/stop.h, tests/runner/test_runner.c, tests/cmake/revision.cmake, docs/runner.md
- FOUND: d9d164b, 015f25b, 58792b9 (ancestors of HEAD)
