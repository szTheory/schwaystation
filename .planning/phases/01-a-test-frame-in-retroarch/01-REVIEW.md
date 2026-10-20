---
phase: 01-a-test-frame-in-retroarch
reviewed: 2026-10-20
depth: standard
depth_source: config
files_reviewed: 126
files_reviewed_list:
  - .github/workflows/ci.yml
  - .github/workflows/nightly.yml
  - .github/workflows/release.yml
  - .release-please-manifest.json
  - CMakeLists.txt
  - CMakePresets.json
  - PROVENANCE.md
  - README.md
  - THIRD-PARTY-NOTICES.md
  - cmake/SchwaystationTests.cmake
  - cmake/packaging.cmake
  - cmake/schwaystation.pc.in
  - cmake/schwaystationConfig.cmake.in
  - cmake/toolchains/ppc-linux-gnu.cmake
  - cmake/toolchains/wasm32-wasi.cmake
  - cmake/version.h.in
  - docs/embedding.md
  - docs/maintaining.md
  - docs/runner.md
  - docs/testing.md
  - host/xrgb8888.c
  - host/xrgb8888.h
  - include/schwaystation.h
  - libretro/CMakeLists.txt
  - libretro/libretro.c
  - libretro/libretro.h
  - libretro/schwaystation_libretro.info.in
  - pins.txt
  - release-please-config.json
  - runner/CMakeLists.txt
  - runner/bench.c
  - runner/bench.h
  - runner/clock.c
  - runner/clock.h
  - runner/crc32.c
  - runner/crc32.h
  - runner/exit_status.h
  - runner/main.c
  - runner/png.c
  - runner/png.h
  - runner/rss.c
  - runner/rss.h
  - runner/sha256.c
  - runner/sha256.h
  - runner/stop.c
  - runner/stop.h
  - scripts/apply-repo-settings.sh
  - scripts/check-title.sh
  - scripts/classify-changes.sh
  - scripts/fetch-pin.sh
  - scripts/hygiene.sh
  - scripts/publish.sh
  - src/crc32.c
  - src/frame.c
  - src/instance.c
  - src/internal.h
  - src/state.c
  - src/state.h
  - src/testcard.c
  - tests/abi/clock_fixture.c
  - tests/abi/global_fixture.c
  - tests/abi/inventory.txt
  - tests/abi/libc_fixture.c
  - tests/abi/symbol_fixture.c
  - tests/abi/tests.cmake
  - tests/alloc.h
  - tests/api/v1.c
  - tests/check.h
  - tests/cmake/abi.cmake
  - tests/cmake/api_version.cmake
  - tests/cmake/archive_contents.cmake
  - tests/cmake/budgets.cmake
  - tests/cmake/check_junit.cmake
  - tests/cmake/expect_fail.cmake
  - tests/cmake/hashes.cmake
  - tests/cmake/install_line.cmake
  - tests/cmake/inventory.cmake
  - tests/cmake/line_budget.cmake
  - tests/cmake/no_float.cmake
  - tests/cmake/no_skip.cmake
  - tests/cmake/perf_compare.cmake
  - tests/cmake/revision.cmake
  - tests/cmake/workflows.cmake
  - tests/consumer/CMakeLists.txt
  - tests/consumer/main.c
  - tests/consumer/main.cpp
  - tests/core/inventory.txt
  - tests/core/test_core.c
  - tests/core/test_state.c
  - tests/core/tests.cmake
  - tests/embed/CMakeLists.txt
  - tests/header/header.c
  - tests/header/header.cpp
  - tests/hygiene/fixtures/titles.txt
  - tests/hygiene/inventory.txt
  - tests/hygiene/tests.cmake
  - tests/install/inventory.txt
  - tests/install/tests.cmake
  - tests/inventory/ci.txt
  - tests/inventory/hygiene.txt
  - tests/inventory/nofp.txt
  - tests/inventory/wasm32.txt
  - tests/libretro/host.c
  - tests/libretro/inventory.txt
  - tests/libretro/tests.cmake
  - tests/nofp/inventory.txt
  - tests/nofp/nofp_fixture.c
  - tests/nofp/tests.cmake
  - tests/perf/allow.txt
  - tests/perf/inventory.txt
  - tests/perf/tests.cmake
  - tests/policy/inventory.txt
  - tests/policy/tests.cmake
  - tests/release/inventory.txt
  - tests/release/tests.cmake
  - tests/retroarch/bmp.c
  - tests/retroarch/compare.c
  - tests/retroarch/flip.cmake
  - tests/retroarch/run_retroarch.cmake
  - tests/retroarch/test.cfg.in
  - tests/runner/inventory.txt
  - tests/runner/test_png.c
  - tests/runner/test_runner.c
  - tests/runner/tests.cmake
  - tests/testcard_pins.cmake
  - tests/tests.cmake
findings:
  critical: 1
  warning: 1
  info: 0
  total: 2
status: issues_found
---

# Phase 1: Code Review Report

**Depth:** standard, performed inline in Codex.
**Source revision:** c078b3127f0128460b56070ce1f5f24559d18f85.
**Scope:** SUMMARY key files cross-checked against the phase task-commit union;
base `82d61d2d3d657f5fa1e452d199c6a7f7bcc7d77f`. The deleted `scripts/setup-release-app.sh`
was excluded. No structural pre-pass or external reviewer lane was requested.
The vendored libretro header was checked for its notice, recorded digest and the
API contracts used by the adapter, rather than audited as an upstream implementation.

## Summary

One core correctness defect needs correction: a successfully loaded state can
advance beyond the loader's permitted counter range, after which a successful
save cannot be loaded. One test-reliability warning is a tooling seed under
AGENTS rule 7, not a new phase requirement or a verification gate. No source fixes
were requested or applied.

## Narrative Findings (AI reviewer)

## Critical Issues

### CR-01: A loaded boundary state advances into an unloadable save

**Classification:** BLOCKER (state round-trip correctness).
**File:** `src/frame.c:47`; related bounds at `src/state.c:22` and
`src/state.c:58`.
**Issue:** The loader accepts `ticks == 2^62` and
`frame == STATE_FRAME_MAX`, but `schway_run_frame` increments those
counters without checking the same limits. Loading an accepted state with ticks
at the limit, running one frame and saving all return OK; loading the newly
saved bytes returns ERR_STATE_FIELD. A crafted, CRC-valid state reaches this
in one frame; ordinary runs would take an impractically long time. This is a
loss of resumability, not a demonstrated memory corruption.

**Reproduction:** In an otherwise valid freshly saved NTSC-U state, set the
little-endian ticks field (offset 32, eight bytes) to `UINT64_C(1) << 62`
and recompute the body CRC. Load it, run one frame, save, then load the saved
bytes. Observed statuses: `load_before=0 run=0 save=0 load_after=10`.

**Fix:** Make the run and load limits one shared invariant. Compute the next
tick count and frame count before changing the instance; when either would
exceed the serializable range, return a documented non-success status without
changing the instance or output. Keep the bounded run contract. Do not merely
reduce the load limit by one frame, which only delays the defect. Add a core
regression that loads states at and immediately below both limits, then verifies
that each run either refuses without mutation or produces a reloadable save.
Document the refusal in `docs/embedding.md`.

## Warnings

### WR-01: PASS_REGEX can turn a command failure into a passing test

**Classification:** WARNING.
**Disposition:** Tooling seed under AGENTS rule 7; excluded from phase acceptance
and the immediate core fix scope. Address with the next test-helper change.
**File:** `cmake/SchwaystationTests.cmake:27`; current users include
`tests/runner/tests.cmake:150` and `tests/install/tests.cmake:58`.
**Issue:** The helper implements PASS_REGEX using CTest's
PASS_REGULAR_EXPRESSION property. Matching output overrides an ordinary nonzero
exit code. A runner that prints the expected hash and later exits 1, or a consumer
that prints its success marker before an ordinary failure, can pass these checks.
The JUnit inventory check sees CTest's passed result and cannot recover the
original failure. Other checks independently cover some runner output, so this
does not establish that all CI could miss the same fault.

**Reproduction:** A scratch CTestTestfile.cmake with the following contents
passes with CTest exit 0, although the child exits 1:

```cmake
add_test(probe sh "-c" "echo expected; exit 1")
set_tests_properties(probe PROPERTIES PASS_REGULAR_EXPRESSION "expected")
```

**Fix:** For callers needing an output assertion, wrap execution in a CMake
script that checks both exit 0 and the tolerant regex, using the existing
`tests/cmake/expect_fail.cmake` with CODE=0 where appropriate. Leave CTest to
check the wrapper's exit status; do not attach PASS_REGULAR_EXPRESSION to it.

## Evidence and limits

All commands were run from the repository root. Local checks have no hosted run
URL (not applicable); no logs or state bytes are committed.

- `node <GSD install>/bin/gsd-tools.cjs check evaluation-scope --phase 01 --raw`:
  exit 0, resolved task-commit scope; combined scope has 126 existing files.
- `cc -std=c17 -Iinclude -Isrc -Ibuild/ci/generated /tmp/schway-review-boundary.c build/ci/libschwaystation.a -o /tmp/schway-review-boundary && /tmp/schway-review-boundary`:
  exit 0; the one-off harness asserted the CR-01 status sequence above.
- `ctest --test-dir <scratch directory> --output-on-failure` on the WR-01
  fixture: exit 0, one passed test despite the child's exit 1.
- `cmake --workflow --preset ci`: exit 0; 77 tests and two archive tests passed,
  and the three deliverables were packaged.
- `cmake --workflow --preset hygiene`: exit 0; 21 tests passed.
- `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake`:
  exit 0; all 77 selected tests ran once.
- `cmake -DJUNIT=build/ci/junit-release.xml -DINVENTORY=tests/inventory/ci.txt -DLABELS=release -P tests/cmake/check_junit.cmake`:
  exit 0; both selected archive tests ran once.
- `cmake -DJUNIT=build/hygiene/junit.xml -DINVENTORY=tests/inventory/hygiene.txt -P tests/cmake/check_junit.cmake`:
  exit 0; all 21 selected tests ran once.
- `shasum -a 256 libretro/libretro.h`: exit 0; matches the recorded
  `036ddffc995ad116c3081ca0a7515d09c2a116f3483e4f5012fe0c29a885da76`.
- `git diff --check`: exit 0.

The documented fork release-input policy in `docs/maintaining.md` deliberately
rejects a fork PR until it is rehearsed on a repository branch; it is not a finding.
The already recorded Windows DLL-runtime consumer todo is not duplicated here.
This review does not claim verification of hosted release publication, all target
platforms, or a new real RetroArch run.

**Next command:** `$gsd-code-review 1 --fix`, addressing CR-01; keep WR-01 as
the tooling seed described above.
