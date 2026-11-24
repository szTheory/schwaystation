---
phase: 14-multitap-and-special-controllers
plan: "08"
subsystem: memory-card-persistence
tags: [memory-card, host-files, atomic-save, multitap]
requires:
  - phase: 14-multitap-and-special-controllers
    provides: One-based card port/socket API and atomic host card writer
provides:
  - Stable socket-aware card filename helpers preserving legacy A paths
  - Eight-socket atomic save and isolation regression vectors
  - Documented filename table and save-state persistence boundary
affects: [runner, libretro, memory-card, saves]
actuals:
  tokens: 6510
  tasks: 2
  commits: 3
commits: 3
plan_head_before: a3a1c9af3d2843d118ebeb23c204b0865bdf037e
plan_head_after: 68e1c43dadbc9ea710f926f759a83636dc324a65
tech-stack:
  added: []
  patterns: [one-based socket path mapping, validate-before-write path construction]
key-files:
  created:
    - docs/saves.md
  modified:
    - host/cards.c
    - host/cards.h
    - tests/card/files.c
    - docs/runner.md
key-decisions:
  - Keep port 1/socket A as `.srm` and port 2/socket A as `.1.mcr` for compatibility.
  - Use fixed suffixes for the other six physical port/socket coordinates, independent of tap configuration.
  - Keep card image loading and flushing in host adapters; card images and generations remain outside ordinary save states.
patterns-established:
  - Socket path helpers validate identity, coordinates, and complete output capacity before writing caller buffers.
  - Atomic persistence tests verify each endpoint's bytes independently under successful and injected-failure writes.
requirements-completed: []
coverage:
  - id: D1
    description: All eight one-based physical port/socket coordinates map to unique stable names, with unchanged A aliases.
    requirement: SIO-01
    verification:
      - kind: unit
        ref: "ctest card.files.tap_paths"
        status: pass
    human_judgment: false
  - id: D2
    description: Invalid coordinates, unsafe or oversized components, and short buffers reject without changing output.
    requirement: SIO-01
    verification:
      - kind: unit
        ref: "ctest card.files.tap_paths"
        status: pass
    human_judgment: false
  - id: D3
    description: Eight card images save and reload independently; a selected socket update leaves the other seven unchanged.
    requirement: SIO-01
    verification:
      - kind: unit
        ref: "ctest card.files.tap_paths"
        status: pass
    human_judgment: false
  - id: D4
    description: Flush and rename failures preserve the selected old file, remove temporary files, and leave other sockets unchanged.
    requirement: SIO-01
    verification:
      - kind: unit
        ref: "ctest card.files.tap_paths and card.files.atomic_failures"
        status: pass
    human_judgment: false
# Metrics
duration: 16m21s
completed: 2026-11-24
status: complete
---

# Phase 14 Plan 08: Stable Card Paths and Persistence Summary

**Socket-aware paths preserve both legacy A filenames and give each added tap card a distinct atomic save file.**

## Performance

- **Duration:** 16m21s wall time (active time not separately measured)
- **Started:** 2026-11-24T06:55:13Z
- **Completed:** 2026-11-24T07:11:34Z
- **Tasks:** 2
- **Files modified:** 5
- **Runtime/model:** Codex GSD executor, configured `gpt-6-luna` xhigh; primary runtime model ID unavailable

## Accomplishments

- Added host helpers that map one-based port/socket coordinates to stable suffixes. Port 1/socket A remains `.srm`, port 2/socket A remains `.1.mcr`, and the other sockets use `.p1b.mcr` through `.p2d.mcr`.
- Validated all eight paths, basename and digest identity routes, directory separators, component limits, invalid coordinates, exact buffer capacity and no-partial-output behavior.
- Wrote and reloaded eight distinct raw card images, changed one socket while checking the other seven, and injected flush and rename failures to prove the old selected file and unrelated sockets remain intact.
- Added [docs/saves.md](../../../docs/saves.md) and linked it from the runner guide. The doc records the suffix table, host-adapter ownership and DEC.67 save-state boundary.

## Task Commits

1. **Task 1 RED:** `ab475ee` — add socket card path vectors and failing helper scaffold.
2. **Task 1 GREEN:** `3faadf8` — map socket-aware card filenames.
3. **Task 2:** `68e1c43` — test independent atomic socket saves and document naming.

## Verification Evidence

- `LC_ALL=C sort -cu tests/card/inventory.txt` — exit 0; no inventory changes were needed.
- `git diff --check` — exit 0.
- `cmake --preset ci` — exit 0.
- Selector preflights confirmed `card.files.tap_paths`, `card.files.wrappers`, `card.files.wrapper_edges`, `card.files.atomic_roundtrip`, and `card.files.atomic_failures` — exit 0.
- Focused `schwaystation-test-card-files` build — exit 0.
- RED command `ctest --preset ci --no-tests=error -R '^card[.]files[.]tap_paths$' --output-junit .gsd/tdd-evidence/14-08/task1-red.xml --output-on-failure` — exit 8 on the expected helper-success assertions; `gsd_run check tdd-red-evidence .gsd/tdd-evidence/14-08/task1-red.json --raw` returned `RED_EVIDENCE_OK`. The temporary return-error helper definitions let the exact registered target compile and fail on assertions; GREEN replaced them with the implementation.
- Task 1 focused `tap_paths|wrappers|wrapper_edges` selection — 3/3 passed.
- Task 2 focused `tap_paths|atomic_roundtrip|atomic_failures` selection — 3/3 passed.
- Final combined `tap_paths|wrappers|wrapper_edges|atomic_roundtrip|atomic_failures` selection — 5/5 passed in 0.03 seconds.
- Temporary card directories were removed. No workflow wiring changed, so no broad suite, Act, hosted CI, or Docker run was needed.

## Decisions Made

- The filename helper uses the same one-based port/socket constants as the host-visible card API.
- Existing two-path basename and identity helpers retain their output. The new helper uses the same identity validation but selects one socket suffix.
- This plan defines and tests additional paths; adapter-specific loading and flushing remain in the runner/libretro plans.

## Deviations from Plan

**Minor documentation addition:** linked the new save-path guide from `docs/runner.md` so the suffix table is discoverable beside the existing runner card documentation. This adds no runtime behavior.

## Issues Encountered

The `tap_paths` CTest registration already existed, but its test dispatcher did not yet handle the selector. The RED commit added the target assertions and temporary return-error definitions so the exact target could execute and fail for the intended reason. The GREEN commit replaced the scaffold with the validated path construction.

## User Setup Required

None.

## Next Phase Readiness

Plan 14-08 is complete. Plan 14-09 remains outside this execution scope.

---
*Phase: 14-multitap-and-special-controllers*
*Completed: 2026-11-24*

## Self-Check: PASSED

- SUMMARY exists at the expected phase path.
- All three task commits are ancestors of HEAD.
- The plan ledger measures three commits from `a3a1c9a` through `68e1c43`.
