---
phase: 04-games-play-in-retroarch
plan: "23"
subsystem: runner
tags: [runner, bios, cli, cmake]

requires:
  - phase: 04-13
    provides: Per-instance BIOS loading and digest-based identity reporting.
  - phase: 04-39
    provides: A same-run, digest-checked OpenBIOS image for later runner boot checks.
provides:
  - The headless runner loads exact-size BIOS images, reports their identity, and executes reset code.
  - Registered CLI coverage for name-independent identity, unknown images, size boundaries, and selector refusals.
affects: [04-40, GAME-05, runner]

actuals:
  tokens: 6094
  tasks: 2
  commits: 4
commits: 4
plan_head_before: cb44ce4ef7590dbbc6f4273badeb0a0eda886f9b
plan_head_after: a70b173b25a1e6318d9e31869cbccf03521d3927

tech-stack:
  added: []
  patterns:
    - Generate synthetic BIOS images in the build tree and exercise them through the actual runner CLI.

key-files:
  created: []
  modified:
    - runner/main.c
    - tests/built/tests.cmake
    - tests/built/inventory.txt
    - tests/runner/tests.cmake
    - docs/runner.md

key-decisions:
  - Report SHA-256 and digest-table status from the loaded image; filenames never establish recognition.
  - Keep unknown exact-size images loadable and preserve the BIOS-free PS-EXE stub-kernel path.
  - Keep every synthetic firmware image under the build directory; no BIOS bytes enter the repository.

patterns-established:
  - CLI tests compare the runner's digest with an independent CMake SHA-256 and verify reset execution through the debug exit port.

requirements-completed: []
coverage:
  - id: D1
    description: The runner loads the copied synthetic BIOS, reports its independent digest and unknown flags, and executes its reset code.
    requirement: GAME-05
    verification:
      - kind: unit
        ref: "built.bios.runner"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci; exit 0"
        status: pass
    human_judgment: false
  - id: D2
    description: The runner keeps identity stable across a file rename, accepts another unknown image, and refuses bad sizes, missing files, duplicates, missing paths, and conflicting selectors.
    requirement: GAME-05
    verification:
      - kind: unit
        ref: "built.bios.runner_edges"
        status: pass
    human_judgment: false
  - id: D3
    description: Existing frozen API callers and the BIOS-free runner path remain valid.
    requirement: GAME-05
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci; ABI v1-v4 and BIOS-free runner tests passed"
        status: pass
    human_judgment: false

duration: 24 min
completed: 2026-11-11
status: complete
---

# Phase 04 Plan 23: Runner BIOS CLI Summary

**The headless runner loads exact-size BIOS images, reports their SHA-256 identity, and executes copied reset code.**

## Performance

- Duration: 24 minutes; started 2026-11-11T19:30:51Z, completed 2026-11-11T19:55:26Z.
- Tasks: 2; files modified: 5.
- Runtime/model: Codex / OpenGSD 1.20.0, gpt-6-luna xhigh.

## Accomplishments

- Added `--bios FILE`, exact-size bounded loading, digest-based identity output, and `--tty` execution of the BIOS reset path.
- Added synthetic CLI tests for copied-image execution, identity across renames, unknown images, invalid sizes, missing/duplicate arguments, and selector conflicts. Test images stay in `build/`.
- Updated runner documentation and retained the BIOS-free PS-EXE stub-kernel path.

## Task Commits

1. Task 1 RED: `0953fb4` — add the failing BIOS reset test.
2. Task 1 test correction: `3ce4ba6` — use the byte-wide debug port.
3. Task 1 GREEN: `91c4406` — add BIOS loading to the runner.
4. Task 2: `a70b173` — cover CLI edge cases.

## Files Created/Modified

- Created: none.
- Modified: `runner/main.c`, `tests/built/tests.cmake`, `tests/built/inventory.txt`, `tests/runner/tests.cmake`, `docs/runner.md`.

## Decisions Made

- Recognition follows the image digest, never its filename; exact-size unknown images remain loadable.
- `--bios` and `--exe` are mutually exclusive. GAME-05 remains open until plan 04-40 observes the separate OpenBIOS shell path.

## Deviations from Plan

**[Rule 1 - Bug] Corrected the synthetic reset probe's debug-port write (Task 1).** The initial word store could not exercise the byte-wide debug port; changed it to `sb` and enabled `--tty`. The corrected test produced accepted pre-implementation RED evidence and passed after implementation. File: `tests/built/tests.cmake`; commit: `3ce4ba6`.

## Evidence

- `ctest --test-dir build/ci -R '^built\.bios\.runner$' --no-tests=error --output-on-failure` — exit 0.
- `ctest --test-dir build/ci -R '^built\.bios\.runner_edges$' --no-tests=error --output-on-failure` — exit 0.
- `ctest --test-dir build/ci -R '^runner\.usage\.unknown$' --no-tests=error --output-on-failure` — exit 0.
- `cmake --workflow --preset ci` — exit 0; 471 regular tests and five release/package checks passed.
- `cmake --workflow --preset hygiene` — exit 0; 23 checks passed.
- Pre-implementation reset test rejected `--bios` as intended; `gsd-tools check tdd-red-evidence build/ci/tdd-red-04-23-task1.json --raw` — exit 0, `RED_EVIDENCE_OK`.

## User Setup Required

None. No real BIOS or OpenBIOS bytes were used.

## Next Phase Readiness

The synthetic runner path is ready; plan 04-40 owns the separate OpenBIOS shell observation.

## Self-Check: PASSED

The summary exists, all four task commits are ancestors of HEAD, and `verify-summary` reports no errors.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-11*
