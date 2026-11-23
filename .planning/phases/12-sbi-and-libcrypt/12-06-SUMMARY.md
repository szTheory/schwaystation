---
phase: 12-sbi-and-libcrypt
plan: "06"
subsystem: testing
tags: [sbi, fuzzing, libfuzzer, cmake, nightly]
requires:
  - phase: 12-02
    provides: Bounded SBI sidecar parser
  - phase: 12-03
    provides: Selected-disc SBI integration
provides:
  - Bounded deterministic SBI parser replay across portable test lanes
  - Nightly libFuzzer campaign with an explicit seven-seed corpus and input cap
affects: [CRYPT-01, fuzz preset, Nightly fuzz job]
actuals:
  tokens: 4518
  tasks: 2
  commits: 1
commits: 1
plan_head_before: 538685db23d48db44c4f9e857b72f97c1d0918ac
plan_head_after: 38b4497a96d006147c5269bdd65cd89cd28aa8cd
tech-stack:
  added: []
  patterns:
    - Parser-specific deterministic seeds reused by replay and libFuzzer corpus generation
    - Existing CMake host-parser linking and portable fuzz inventory
key-files:
  created:
    - tests/fuzz/sbi.c
  modified:
    - CMakePresets.json
    - .github/workflows/nightly.yml
    - tests/cmake/workflows.cmake
    - tests/fuzz/inventory.txt
    - tests/fuzz/seeds.c
    - tests/fuzz/seeds.h
    - tests/fuzz/tests.cmake
key-decisions:
  - Bound SBI fuzz inputs and generated corpus files at the parser's 65,536-byte limit.
  - Keep seven distinct seeds, including a 57,355-byte 8,193-record limit case.
  - Add both SBI binaries to the explicit fuzz build preset so Nightly actually builds them.
requirements-completed: [CRYPT-01]
coverage:
  - id: D1
    description: The production SBI parser replays valid and malformed bounded inputs, and Nightly selects its mutation campaign.
    requirement: CRYPT-01
    verification:
      - kind: unit
        ref: ctest --preset ci --no-tests=error -R '^fuzz[.]regress[.]sbi$' --output-on-failure
        status: pass
      - kind: integration
        ref: ctest --test-dir build/phase12-06-fuzz-probe --no-tests=error -R '^fuzz[.]regress[.]sbi$' --output-on-failure
        status: pass
      - kind: other
        ref: cmake -DCASE=disc_registration -DROOT=. -P tests/cmake/workflows.cmake
        status: pass
    human_judgment: false
metrics:
  duration: 64min
  completed: 2026-11-23
status: complete
---

# Phase 12 Plan 06: SBI fuzz campaign Summary

**Seven bounded SBI seeds now exercise the production parser through portable replay and the Nightly libFuzzer campaign.**

## Performance

- **Duration:** 64 minutes elapsed from plan start through implementation and GSD closeout; active time was not separately measured.
- **Started:** 2026-11-23T06:54:23Z
- **Completed:** 2026-11-23T07:58:13Z
- **Tasks:** 2
- **Files modified:** 8
- **Runtime/model:** Codex GSD executor; configured `gpt-6-luna` xhigh; primary runtime model ID unavailable.

The changed-head native configure, focused build and replay took about one second combined. The separate Apple Clang 21 sanitizer configure/build/replay took about 11 seconds. Act's dry-run took about 30 seconds, mostly resolving the pinned upload-artifact action into the project-local cache. No hosted CI run occurred in this plan; the actual Ubuntu mutation campaign remains part of phase verification.

## Accomplishments

- Added seven deterministic cases for valid input, truncation, bad header, invalid format, invalid BCD, out-of-range LBA and the record-count limit. Replay checks the exact expected parser result, output bounds, sorted unique LBAs and unchanged output on failure.
- Registered portable replay and libFuzzer targets, the `fuzz.regress.sbi` inventory entry, and Nightly's expected count, 65,536-byte maximum and corpus-name checks.
- Extended the existing Nightly workflow control assertion and explicit fuzz build preset. The repository gate remains exactly 3,901/3,901 lines, with no new workflow routing.

## Test Evidence

| Command | Result |
| --- | --- |
| `LC_ALL=C sort -cu tests/fuzz/inventory.txt` | Exit 0; inventory is byte-sorted. |
| `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` | Exit 0; 3,901 lines, limit 3,901. |
| `cmake -DCASE=disc_registration -DROOT=. -P tests/cmake/workflows.cmake` | Exit 0; Nightly job and selector controls connected. |
| Nightly array and CMake fuzz-preset target-count preflights | Exit 0; 10 subsystem/count/length entries agree, and both SBI targets are present. |
| `cmake --preset ci` | Exit 0 in 0.8s. |
| `cmake --build --preset ci --target schwaystation-fuzz-sbi-replay` | Exit 0 in 0.2s. |
| `ctest --preset ci --no-tests=error -R '^fuzz[.]regress[.]sbi$' --output-on-failure` | Exit 0; 1/1 passed in 0.01s. |
| `build/ci/tests/schwaystation-fuzz-sbi-replay --write build/ci/tests/fuzz-work/sbi-seeds` | Exit 0; wrote all seven expected files, each at most 65,536 bytes; the record-bound seed is 57,355 bytes. |
| `cmake --preset fuzz -B build/phase12-06-fuzz-probe -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++` | Exit 0 in 7.6s using Apple Clang 21. |
| `cmake --build build/phase12-06-fuzz-probe --target schwaystation-fuzz-sbi-replay` | Exit 0 in 2.9s with the fuzz preset's address/undefined-behavior instrumentation. |
| `ctest --test-dir build/phase12-06-fuzz-probe --no-tests=error -R '^fuzz[.]regress[.]sbi$' --output-on-failure` | Exit 0; 1/1 passed in 0.12s under sanitizers. |
| `act -W .github/workflows/nightly.yml -j fuzz --dryrun -P ubuntu-26.04=ubuntu:latest --action-cache-path build/ci/phase12-06-act-cache` | Exit 0 in about 30s; parsed and traversed the Nightly fuzz job without creating containers or running its steps. |

The first Act dry-run attempt exited 1 at Act's interactive default-image chooser because its default cache path was outside the writable workspace. The explicit inert image mapping and project-local cache resolved it. A direct Apple Clang libFuzzer link probe exited 1 because this host lacks `libclang_rt.fuzzer_osx.a`; no compiler was installed, and no mutation run is claimed. Ubuntu Nightly installs the packaged libFuzzer runtime and remains the executable campaign environment.

## Task Commits

1. **Task 1: Add SBI fuzz campaign and portable seed replay** — `38b4497` (`feat(12-06): add bounded SBI fuzz campaign`). This commit also updates `CMakePresets.json`, which explicitly enumerates fuzz targets.
2. **Task 2: Preflight changed inventory selections and gate budget** — no separate file change or commit; the required inventory, selector, target-count and budget preflights passed.

## Files Created/Modified

- `tests/fuzz/sbi.c` — calls the production SBI parser and validates seed status, memory bounds and successful overlays.
- `tests/fuzz/seeds.c` and `tests/fuzz/seeds.h` — define seven named inputs and the 65,536-byte selected-corpus bound.
- `tests/fuzz/tests.cmake` and `tests/fuzz/inventory.txt` — register replay and libFuzzer targets on the existing host-parser path and portable inventory.
- `.github/workflows/nightly.yml` and `tests/cmake/workflows.cmake` — add corpus count, input bound, names and matching workflow-control assertions without changing routing.
- `CMakePresets.json` — include both SBI targets in the explicitly enumerated fuzz build preset.

## Decisions Made

- Reused the existing host parser library and the shared seed/replay interfaces; no new repository-check code or dependency was needed.
- Kept the full Nightly campaign length and all existing lanes, cases and assertions unchanged. The new campaign uses the parser's 65,536-byte bound.
- Ran Act's dry-run because it validates workflow parsing beyond the CMake text assertion. Did not run the full Act job: it would repeat every existing 60-second fuzz campaign, while the local sanitizer replay and required hosted campaign remain available.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing Critical Functionality] Add SBI targets to the fuzz build preset**

- **Found during:** Task 1 (Nightly target-count preflight)
- **Issue:** `CMakePresets.json` enumerates fuzz targets explicitly, so the Nightly `cmake --build --preset fuzz` command would omit the new SBI replay and mutation binaries unless the preset target list changed.
- **Fix:** Added both SBI target names to the existing fuzz build preset.
- **Files modified:** `CMakePresets.json`
- **Verification:** `cmake --list-presets=build`, the fuzz preset target-list check and the separate sanitizer replay build passed.
- **Committed in:** `38b4497`.

**Total deviations:** 1 auto-fixed (Rule 2).
**Impact on plan:** Required to make the Nightly campaign executable; no routing, gate lines or test coverage were removed.

## Issues Encountered

- The existing `build/fuzz` cache belongs to `/work`, not this checkout. It was left untouched; a fresh project-local fuzz build directory was used with the available Apple Clang 21 compiler.
- The local Apple Clang installation lacks its libFuzzer runtime, so the mutation campaign was not run locally. The configured Ubuntu Nightly job remains required to compile and run it.
- No Docker image, container, network or volume was created by the Act dry-run.

## Next Phase Readiness

- Plan 12-06 is complete and state can advance to Plan 12-07. Phase-level review and hosted CI still need to run after the remaining plan; no user action is required.

---
*Phase: 12-sbi-and-libcrypt*
*Completed: 2026-11-23*

## Self-Check: PASSED

- The summary and `tests/fuzz/sbi.c` exist, and task commit `38b4497a96d006147c5269bdd65cd89cd28aa8cd` is an ancestor of the current plan head.
- Coverage classification reports the deliverable as fully automated with passing evidence. `verify-summary` reports a known path-parser false positive for the plan's inventory command (`LC_ALL=C sort -cu tests/fuzz/inventory.txt`); the referenced inventory exists and its sorted preflight passed.
