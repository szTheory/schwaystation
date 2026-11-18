---
phase: 04-games-play-in-retroarch
plan: "27"
subsystem: libretro
tags: [libretro, ps-exe, bios, lifecycle]
requires:
  - phase: 04-15
    provides: Deterministic digital-pad input for the adapter
  - phase: 04-37
    provides: The pad_counter fixture and its guest-visible controls
  - phase: 04-40
    provides: A built and inspected OpenBIOS integration baseline
provides:
  - Bounded fullpath PS-EXE loading in libretro through the real-MIPS stub
  - Explicit no-content BIOS selection through the existing BIOS loader
  - Atomic invalid-content refusal and load, unload, and reload coverage
affects: [04-24, 04-29, 04-43]
actuals:
  tokens: 7713
  tasks: 2
  commits: 4
  plan_head_before: 7dd06e974b628fdadfccec30ff84126bb74fe61a
  plan_head_after: aba2b7b03da6472237938c393ce86dc3178e0407
tech-stack:
  added: []
  patterns:
    - Read bounded host-selected images before replacing the active core instance
    - Keep the no-content BIOS option separate from the PS-EXE stub path
key-files:
  created: []
  modified:
    - libretro/libretro.c
    - libretro/schwaystation_libretro.info.in
    - src/exe.c
    - tests/exe/exe_tests.c
    - tests/libretro/host.c
    - tests/libretro/inventory.txt
    - tests/libretro/tests.cmake
    - docs/embedding.md
key-decisions:
  - "Default no-content loads keep the built-in test card; only an explicit BIOS option reads from the frontend system directory."
  - "PS-EXE content always uses the real-MIPS stub, even when a BIOS option is selected."
  - "Validate malformed EXE headers before refusing valid late loads, preserving frozen API error statuses."
requirements-completed: []
coverage:
  - id: D1
    description: Fullpath pad_counter PS-EXE content runs without a BIOS, and guest input changes its visible output.
    requirement: GAME-07
    verification:
      - kind: integration
        ref: tests/libretro/tests.cmake#libretro.content.no_bios
        status: pass
    human_judgment: false
  - id: D2
    description: The BIOS option defaults to the test card, loads an exact-size selected image only for no-content, and refuses missing or truncated images without replacing active state.
    requirement: GAME-07
    verification:
      - kind: integration
        ref: tests/libretro/tests.cmake#libretro.content.lifecycle
        status: pass
    human_judgment: false
  - id: D3
    description: Guest display geometry changes notify the frontend, while invalid and late EXE loads preserve state and existing API error results.
    requirement: GAME-02
    verification:
      - kind: integration
        ref: tests/libretro/tests.cmake#libretro.content.no_bios
        status: pass
      - kind: unit
        ref: tests/exe/tests.cmake#exe.after_first_frame and tests/api/v2.c#schway_load_exe statuses
        status: pass
    human_judgment: false
duration: "at least 20 minutes; exact executor start was not retained after context compaction"
completed: 2026-11-12
status: complete
---

# Phase 4 Plan 27: Load BIOS-free fullpath PS-EXEs in libretro Summary

**The libretro core now runs fullpath PS-EXEs through its real-MIPS stub, with a separate opt-in BIOS path for no-content boots.**

## Performance

- **Duration:** At least 20 minutes; setup began before the earliest measurable task commit, and the executor start timestamp was unavailable after context compaction.
- **Started:** Before 2026-11-12T02:45:31Z; exact time unavailable.
- **Completed:** 2026-11-12T03:06:27Z
- **Runtime/model:** Codex, OpenGSD 1.20.0; gpt-6-luna xhigh.
- **Tasks:** 2
- **Files modified:** 8

## Accomplishments

- Fullpath `.exe` content is read and validated before the active instance is replaced; PS-EXEs always boot through the real-MIPS stub.
- No-content boot keeps the test card by default. An explicit BIOS option reads an exact-size image from the frontend system directory and passes it to `schway_load_bios`; missing or wrong-sized choices refuse without fallback.
- The registered lifecycle test checks BIOS alternatives, state-preserving content failures, execution with a BIOS option selected, unload/reload, and return to no-content.
- Valid late EXE loads are refused after ticks, frames, or an earlier load, while malformed files retain the frozen API's validation error statuses.

## Task Commits

- Task 1 RED test commit: `b5f89aa` (amended from the original `638c2fa` to correct test code).
- Task 1 GREEN: `315262a`.
- Task 2: `1644ff3`.
- Compatibility fix discovered during full CI: `aba2b7b`.

## Files Created/Modified

Created: none.

- `libretro/libretro.c` — bounded EXE and BIOS reads, no-content selector, stub boot, input mapping, and geometry notification.
- `libretro/schwaystation_libretro.info.in` — advertises EXE content.
- `src/exe.c` — refuses valid late EXE loads without changing the instance and preserves validation error precedence.
- `tests/exe/exe_tests.c` — invalid-load and late-load state controls.
- `tests/libretro/host.c` — real in-process content, BIOS selection, refusal, and lifecycle assertions.
- `tests/libretro/tests.cmake`, `tests/libretro/inventory.txt` — registered lifecycle coverage and exact inventory.
- `docs/embedding.md` — documents the adapter's content and BIOS behavior.

## TDD Gate Compliance

The original pre-production run of `ctest --test-dir build/ci -R '^libretro\.content\.no_bios$' --no-tests=error --output-on-failure --output-junit build/ci/04-27-task1-red.xml` exited 8 on the behavioral assertion `p_load_game(&game) == 1` (actual 0): the old adapter rejected all content. `gsd_run check tdd-red-evidence build/ci/04-27-task1-red-evidence.json --raw` classified the target as RED and returned its expected failing-test status. The RED commit was later amended to correct a test initializer and pixel packing expectation; that amended snapshot was not independently rerun against pre-production code. The RED claim here is limited to the original observed loader assertion.

## Evidence

- Task 1 tracer: `ctest --test-dir build/ci -R '^libretro\.content\.no_bios$' --no-tests=error --output-on-failure` — exit 0.
- Task 2 focused coverage: `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-libretro-host schwaystation-fixtures && ctest --test-dir build/ci -R '^libretro\.content\.lifecycle$' --no-tests=error --output-on-failure` — exit 0.
- Changed libretro modes: `ctest --test-dir build/ci -R '^(libretro\.host|libretro\.host\.pal|libretro\.content\.no_bios|libretro\.content\.lifecycle)$' --no-tests=error --output-on-failure` — exit 0, 4 tests.
- Compatibility correction: `ctest --test-dir build/ci -R '^(abi\.api_v2|exe\.error_changes_nothing|exe\.after_first_frame|libretro\.content\.no_bios|libretro\.content\.lifecycle)$' --no-tests=error --output-on-failure` — exit 0, 5 tests.
- Final full CI: `cmake --workflow --preset ci` — exit 0; all 482 CI tests and all 5 release checks passed.
- Final hygiene: `cmake --workflow --preset hygiene` — exit 0; all 23 checks passed.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Preserve EXE validation status precedence**

- **Found during:** Final full CI.
- **Issue:** The new late-load guard returned `ERR_ARGUMENT` before validation, breaking frozen API v2 callers that expect invalid and out-of-range statuses.
- **Fix:** Validate non-null, non-empty EXE input first; refuse an otherwise valid load after ticks, frames, or an existing load before writing RAM.
- **Files modified:** `src/exe.c`.
- **Verification:** `abi.api_v2`, `exe.error_changes_nothing`, and `exe.after_first_frame` passed, followed by the complete CI workflow.
- **Committed in:** `aba2b7b`.

**Total deviations:** 1 auto-fixed issue.

## Issues Encountered

The first full CI run found the API v2 status-precedence regression described above. The focused regression set and one final full CI run passed after the fix. No hosted RetroArch run was needed for this local slice; hosted screenshot comparison remains with the later integration plan.

## Threat Flags

| Flag | File | Description |
|---|---|---|
| threat_flag: filesystem-read | `libretro/libretro.c` | The adapter reads a frontend-selected BIOS path; it rejects absolute paths and dot segments, bounds the read to the BIOS size plus one byte, and refuses missing or wrong-sized images. |

## User Setup Required

None.

## Next Phase Readiness

The local libretro content path is ready for the later hosted RetroArch screenshot comparison. Full native geometry/rate coverage remains assigned to its dependent plan. Plan 04-24 is still incomplete at its existing budget checkpoint and was not changed here.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-12*

## Self-Check: PASSED

The summary and all eight modified files exist; all four task commits are ancestors of HEAD.
