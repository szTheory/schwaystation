---
phase: 06-discs-and-fmv
plan: "07"
subsystem: host-disc
tags: [c, cue, iso, disc, cmake, ctest]
requires:
  - phase: "06-01"
    provides: Production disc-image target and per-directory test inventories.
  - phase: "06-02"
    provides: Public disc callbacks and copied TOC contract.
provides:
  - Host-only ISO and cue/bin sector provider with raw-sector normalization and TOC metadata.
  - Bounded cue parsing for MODE1/2048, MODE2/2352 and AUDIO tracks, with path containment.
  - Disc image behavior tests registered in the supported CMake test lanes.
affects: [06-04, 06-08, 06-11, 06-15, 06-16, 06-17, 06-19, 06-20]
actuals:
  tokens: 20831
  tasks: 3
  commits: 6
commits: 6
plan_head_before: dafc10f04e9733e2ae3baaf1d0dc6847035e5745
plan_head_after: 3a433960a0ec224792d0b9f2d1c622b7d70815dd
tech-stack:
  added: []
  patterns:
    - Host file access and disc parsing stay outside the deterministic core.
    - Cue and ISO data are served through one bounded provider instance and the public sector callback.
key-files:
  created:
    - host/disc_image.c
    - host/disc_image.h
    - tests/disc/image.c
    - tests/disc/tests.cmake
    - tests/disc/inventory.txt
  modified:
    - cmake/DiscTargets.cmake
    - tests/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/asan.txt
    - tests/inventory/wasm32.txt
decisions:
  - Keep ISO/cue parsing and all host file handles in a host-only provider; do not add file or clock access to the core.
  - Model INDEX 00 as physical file content and PREGAP as synthetic silence, preserving their distinct sector offsets.
  - Reject unsupported image extensions with a named planned-v2.0 result instead of partial format sniffing.
metrics:
  duration: "not measured"
  completed: 2026-11-15
status: complete
requirements-completed: []
coverage:
  - id: D1
    description: The host provider reads ISO and supported cue/bin sectors through the public disc callbacks, builds valid TOCs, and rejects tested size, offset, format, and path attacks.
    requirement: DISC-02
    verification:
      kind: unit
      ref: "cmake --preset ci && cmake --build --preset ci --target schwaystation-test-disc-image && ctest --test-dir build/ci -R '^disc\\.image|^policy\\.' --no-tests=error --output-on-failure; exit 0, 12/12 selected tests passed locally; no hosted run URL applies"
      status: pass
    human_judgment: false

---

# Phase 6 Plan 07: Bounded ISO and Cue Image Provider Summary

**ISO and cue/bin images now share a bounded host-only sector provider that supplies normalized raw sectors and validated TOCs.**

## Performance

- **Duration:** Not measured; the executor start timestamp was unavailable.
- **Started:** Not captured.
- **Completed:** 2026-11-15
- **Tasks:** 3
- **Files modified:** 10
- **Runtime/model:** Codex, OpenGSD 1.20.0; executor gpt-6-luna xhigh.
- **Actuals:** 83,327 diff bytes / 4 = 20,831 estimate-scale tokens; six measured task commits.

## Accomplishments

- Added a host-only provider that supplies ISO and cue/bin sectors through the existing public disc callback contract. Cooked ISO sectors are normalized to raw Mode 1 layout.
- Added bounded cue support for MODE1/2048, MODE2/2352, AUDIO, multiple binary files, physical INDEX 00, synthetic PREGAP, validated MSF offsets, and canonical child-path containment.
- Added temporary build-directory test images covering sector data, TOCs, EOF behavior, parser limits, unsupported types, and hostile paths. The tests run through the normal CTest graph and ci/asan/wasm32 inventories; cdrom and mdec inventories remain registered for later callers.

## Verification Evidence

All verification ran from the repository root in the background. These were local runs, so no hosted run URL applies.

| Command | Result |
|---|---|
| `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-disc-image && ctest --test-dir build/ci -R '^disc\.image' --no-tests=error --output-on-failure` | Task 1: exit 0; ISO sector test passed. Task 2: exit 0; all 8 `disc.image.*` tests passed. |
| `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-disc-image && ctest --test-dir build/ci -R '^disc\.image|^policy\.' --no-tests=error --output-on-failure` | Task 3: exit 0; all 12 selected disc and policy tests passed. |
| `git diff --check` | Exit 0 before task commits. |
| Global lane inventory `LC_ALL=C sort -c` checks | Exit 0 for ci, asan, and wasm32 inventories. |

## Task Commits

1. **Task 1: Read one ISO sector through the public disc callbacks** — `d07ea4e` (test, RED), `15a7c6a` (feat, GREEN).
2. **Task 2: Expand cue layouts and path containment** — `24951b3` (test, RED), `82d767b` (test, RED boundary coverage), `bc82a58` (feat, GREEN).
3. **Task 3: Register supported lane inventories for the new callers** — `3a43396` (chore).

## TDD Evidence

- Task 1's initial callback test failed against the provider scaffold (CTest exit 8); the format-based RED evidence classifier accepted `build/ci/tdd-red-06-07-task1.json`. The completed ISO provider then passed the named task check.
- Task 2's cue mode test failed against the ISO-only provider (CTest exit 8); the classifier accepted `build/ci/tdd-red-06-07-task2.json`. The index/cap boundary test also failed against the incomplete provider (CTest exit 8); the classifier accepted `build/ci/tdd-red-06-07-task2-index.json`. The completed cue provider passed all eight disc image tests.
- RED evidence files remain ignored build output and were not committed.

## Files Created/Modified

- `host/disc_image.c`, `host/disc_image.h` — bounded host provider and its opaque lifetime API.
- `tests/disc/image.c` — generated ISO/cue/bin inputs and provider callback tests.
- `tests/disc/tests.cmake`, `tests/disc/inventory.txt`, `tests/tests.cmake` — register the disc image test target.
- `cmake/DiscTargets.cmake` — expose the public and generated include paths required by the provider consumer.
- `tests/inventory/ci.txt`, `tests/inventory/asan.txt`, `tests/inventory/wasm32.txt` — register disc, CD-ROM, and MDEC inventories in supported lanes.

## Decisions Made

- Keep image files and parsing in the host target; the emulated core receives sectors only through callbacks.
- Preserve the distinction between physical INDEX 00 sectors and synthetic PREGAP sectors.
- Name unsupported CHD/PBP/ECM/CCD formats explicitly and reject them without sniffing.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking issue] Added public and generated include paths to the provider target.**
- **Found during:** Task 1.
- **Issue:** The existing provider target did not expose the installed public API or generated version header to its new source and test consumer, so the target could not compile.
- **Fix:** Added the host, public include, and generated include directories to the target's PUBLIC include paths.
- **Files modified:** `cmake/DiscTargets.cmake`.
- **Verification:** The focused configure/build/CTest command passed after the fix.
- **Committed in:** `d07ea4e`.

**2. [Plan sequencing] Registered the disc test target and its disc inventory during Task 1.**
- **Found during:** Task 1.
- **Issue:** The first TDD RED check required CTest to discover the new target and test before the implementation was complete; those registrations were listed under Task 3.
- **Fix:** Added the minimum disc target and normal test graph registration needed to run RED evidence. Task 3 completed the global lane registrations and sorted inventories.
- **Files modified:** `tests/disc/tests.cmake`, `tests/disc/inventory.txt`, `tests/tests.cmake`, and `tests/inventory/ci.txt`, `tests/inventory/asan.txt`, `tests/inventory/wasm32.txt`.
- **Verification:** The first RED check selected the ISO test and failed at the expected unimplemented callback; the final Task 3 command passed all 12 selected tests.
- **Committed in:** `d07ea4e`, with remaining lane registration in `3a43396`.

**Total deviations:** 2 (one Rule 3 blocking fix and one task sequencing adjustment).
**Impact on plan:** Both changes supported the planned provider and its TDD checks; no acceptance scope was removed.

## Issues Encountered

- The initial build surfaced missing target include directories and an integer conversion warning; both were corrected before implementation verification.
- Cue parser tests exposed safe-name termination and MSF digit-pair parsing errors. These were corrected inline, then the complete focused suite passed.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

The provider is ready for Plan 06-08 playlist and SYSTEM.CNF integration and for the later runner, libretro, and CD-ROM consumers. Requirement DISC-02 remains pending because m3u parsing, fuzz replay, disc boot, and the full cross-plan acceptance are not included here.

---
*Phase: 06-discs-and-fmv*
*Completed: 2026-11-15*

## Self-Check: PASSED

The summary file exists and all six task commits are ancestors of the plan head recorded above.
