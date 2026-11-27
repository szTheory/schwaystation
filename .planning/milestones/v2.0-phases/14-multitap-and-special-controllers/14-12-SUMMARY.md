---
phase: 14-multitap-and-special-controllers
plan: "12"
subsystem: libretro
tags: [libretro, input, mouse, flight-stick, negcon, jogcon]
requires:
  - phase: 14-11
    provides: compact libretro multitap topology and routing
provides:
  - Exact libretro mouse, flight, neGcon and Jogcon input mappings to serial replies
  - Public special-controller ID and input mapping documentation
affects: [libretro-input, phase-14-integration]
actuals:
  tokens: 7671
  tasks: 1
  commits: 2
commits: 2
plan_head_before: 5ff97ffba6fbcfe26fa47fd89ab72d654d604370
plan_head_after: 79e8c0cbc073bc004f7083cd4005a44943f424e6
tech-stack:
  added: []
  patterns:
    - Snapshot each frontend input value once per frame and pass integer values into the core
    - Test adapter mappings through actual serial endpoint replies
key-files:
  created: []
  modified:
    - libretro/libretro.c
    - tests/libretro/host.c
    - docs/controllers.md
key-decisions:
  - Accept only the exact published libretro device IDs for the supported special devices.
  - Map neGcon pressure controls and Jogcon dial bits with documented integer conversions and no response curves.
requirements-completed: []
coverage:
  - id: D1
    description: Mouse relative axes and buttons reach direct and tap serial endpoints with saturation and neutral reset.
    requirement: SIO-02
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^libretro[.]input[.](multitap|special|descriptors)$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: Flight, neGcon and Jogcon mappings reach serial replies; defaults remain intact and unsupported modes are refused.
    requirement: SIO-03
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^libretro[.]input[.](multitap|special|descriptors)$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D3
    description: Exact special device IDs and their host-side control mappings are documented.
    requirement: SIO-04
    verification:
      - kind: other
        ref: "git diff --check; libretro.input.descriptors"
        status: pass
    human_judgment: false
duration: 24min
completed: 2026-11-24
status: complete
---

# Phase 14 Plan 12: Special libretro controller mappings Summary

Special libretro mouse, flight-stick, neGcon and Jogcon values now pass through one frame snapshot into their serial replies, with exact device-ID support and documented integer mappings.

## Performance

- **Runtime/model:** Codex GSD executor, configured `gpt-6-luna` xhigh; runtime model ID unavailable.
- **Duration:** 24m wall time
- **Started:** 2026-11-24T10:25:25Z
- **Completed:** 2026-11-24T10:49:25Z
- **Tasks:** 1
- **Files modified:** 3

## Accomplishments

- Added exact device selection and snapshot mappings for mouse, flight stick, neGcon and Jogcon.
- Added direct and tap endpoint tests that check real serial replies, saturation, neutral reset, axis extremes, button masks, supported/refused modes, same-frame sampling and existing digital/DualShock defaults.
- Documented the accepted IDs, numeric subclass derivation and host control mappings.

## Task Commits

1. **Task 1 RED: special controller wire checks** — `f295c54` (test)
2. **Task 1 GREEN: special input mappings and docs** — `79e8c0c` (feat)

**Plan metadata:** pending closeout commit.

## Files Created/Modified

- `libretro/libretro.c` — exact special-device IDs and bounded integer input snapshots.
- `tests/libretro/host.c` — special callback values checked against serial endpoint replies.
- `docs/controllers.md` — published IDs and mapping table.

## Decisions Made

- Special analog subclasses are matched by exact ID; unknown subclasses remain unsupported.
- Frontend-provided integer axes are mapped without adding core-side response curves or deadzones.

## Verification

- `git diff --check` — exit 0.
- `LC_ALL=C sort -cu tests/libretro/inventory.txt` — exit 0; the sorted inventory did not change.
- CTest JSON preflight for `libretro.input.multitap|special|descriptors` — exit 0; all three selectors were present.
- `cmake --preset ci` — exit 0; reused `build/ci`.
- `cmake --build --preset ci --target schwaystation-test-libretro-host schwaystation_libretro` — exit 0.
- `ctest --preset ci --no-tests=error -R '^libretro[.]input[.](multitap|special|descriptors)$' --output-on-failure --output-junit "$PWD/.gsd/tdd-evidence/14-12/focused.xml"` (run from repository root) — exit 0; 3/3 passed.
- TDD RED evidence in ignored `.gsd/tdd-evidence/14-12/` classified `RED_EVIDENCE_OK` for the intended missing special-device mapping. The test-only direct serial poll helper made the mapping observable on wire.
- No broad suite, hosted CI, Act or Docker run; Plan 15 owns final reviewed-head aggregate verification.

## Issues Encountered

The first green focused run found a test instrumentation error: callback counts combined calls from different players. The test now scopes counts by player, device, index and ID; the focused trio then passed. This did not expose a product mapping defect.

## Deviations from Plan

None. The test-only counter correction kept the planned behaviors and assertions intact.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Plan 14-12 is complete. Continue with the next authorized Plan 14 slice; Plan 15 will run the final reviewed-head aggregate verification.

---
*Phase: 14-multitap-and-special-controllers*
*Completed: 2026-11-24*

## Self-Check: PASSED

The summary exists, both task commits are ancestors of HEAD, and the GSD coverage classifier accepted all three deliverables.
