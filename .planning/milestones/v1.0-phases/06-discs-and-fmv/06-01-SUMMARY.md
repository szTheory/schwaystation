---
phase: 06-discs-and-fmv
plan: "01"
subsystem: infra
tags: [cmake, c, cdrom, mdec, disc-image, inventories]
requires: []
provides:
  - Top-level CD-ROM and MDEC object targets included in the shipped core archive.
  - A host-only disc-image library linked by the runner and libretro targets.
  - Empty sorted inventories for the CD-ROM, MDEC, and host-disc test directories.
affects: [phase-6-disc, phase-6-mdec, phase-6-host-integration]
actuals:
  tokens: 1179
  tasks: 2
  commits: 2
commits: 2
plan_head_before: 81ef31aeba2f758cc02a50e98dd3b1389ac97d4f
plan_head_after: 7ed3b5aa2dcae3f42e89ca5f523eefd28ba3f5f0
tech-stack:
  added: []
  patterns:
    - Per-chip object targets feed the production core archive.
    - Host filesystem support remains in a separate static library.
key-files:
  created:
    - cmake/DiscTargets.cmake
    - src/cdrom.c
    - src/mdec.c
    - host/disc_image.c
    - tests/cdrom/inventory.txt
    - tests/mdec/inventory.txt
    - tests/disc/inventory.txt
  modified:
    - CMakeLists.txt
key-decisions:
  - "Top-level deliverables use per-chip object targets; embedded builds compile the same sources directly into the library to preserve the add_subdirectory target contract."
  - "Inventories remain empty until their first behavior callers are implemented in later plans."
requirements-completed: []
coverage:
  - id: D1
    description: Top-level library, runner, and libretro builds include the CD-ROM and MDEC production objects.
    requirement: DISC-01
    verification:
      - kind: integration
        ref: "cmake --preset ci && cmake --build --preset ci --target schwaystation schwaystation-run schwaystation_libretro && ctest --test-dir build/ci -R '^abi\\.|^install\\.|^embed\\.' --no-tests=error --output-on-failure; exit 0"
        status: pass
    human_judgment: false
  - id: D2
    description: The host disc-image library links into runner and libretro and stays outside the core archive.
    requirement: DISC-02
    verification:
      - kind: integration
        ref: "Archive/link inspection with ar and Ninja; exit 0 (CD/MDEC members present, disc_image absent from the core archive and linked by both hosts)."
        status: pass
    human_judgment: false
  - id: D3
    description: Empty sorted test inventories exist for CD-ROM, MDEC, and host-disc tests.
    requirement: DISC-03
    verification:
      - kind: other
        ref: "LC_ALL=C sort -c tests/cdrom/inventory.txt tests/mdec/inventory.txt tests/disc/inventory.txt; exit 0"
        status: pass
      - kind: integration
        ref: "cmake --preset ci && cmake --build --preset ci --target schwaystation && ctest --test-dir build/ci -R '^policy\\.' --no-tests=error --output-on-failure; exit 0"
        status: pass
    human_judgment: false
duration: 12min
completed: 2026-11-15
status: complete
---

# Phase 06 Plan 01: Build and inventory the disc subsystem skeleton Summary

**CD-ROM and MDEC production build targets now ship in the core archive, with a separate host disc-image library and exact subsystem inventory files.**

## Performance

- **Duration:** 12 min
- **Started:** 2026-11-15T15:32:21Z
- **Completed:** 2026-11-15T15:44:41Z
- **Tasks:** 2
- **Files modified:** 8
- **Runtime/model:** Codex, OpenGSD 1.20.0; executor gpt-6-luna xhigh

## Accomplishments

- Added production CD-ROM and MDEC object targets to the top-level core archive and built the library, runner, and libretro deliverables.
- Added a separate host disc-image target and confirmed it links to both hosts without entering the core archive.
- Added empty, bytewise-sorted inventories for the CD-ROM, MDEC, and host-disc test areas; later plans will register behavior tests with their first callers.

## Task Commits

1. **Task 1: Trace the three deliverables through the new build targets** - `f65d4e8` (`feat`)
2. **Task 2: Create every new per-directory test inventory** - `7ed3b5a` (`test`)

## Files Created/Modified

- `CMakeLists.txt` - Includes the disc targets in the core archive and links the host helper into runner and libretro.
- `cmake/DiscTargets.cmake` - Defines the per-chip core targets and host-only disc-image target.
- `src/cdrom.c`, `src/mdec.c` - Compileable translation units reserved for later device implementations.
- `host/disc_image.c` - Compileable host-only translation unit reserved for later image-provider implementation.
- `tests/cdrom/inventory.txt`, `tests/mdec/inventory.txt`, `tests/disc/inventory.txt` - Empty exact inventories for later test registration.

## Decisions Made

- Top-level builds expose per-chip object targets; embedded builds compile those same source files directly into the library so the existing `add_subdirectory` target contract remains intact.
- The inventories contain no future test names; each later behavior plan registers its real tests with the first caller.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Preserved the embedding consumer's target contract**
- **Found during:** Task 1 (Trace the three deliverables through the new build targets)
- **Issue:** The first tracer run failed `embed.subdirectory`: the embedding consumer permits the existing library and object targets but detected the new CD-ROM and MDEC targets.
- **Fix:** Limit the new object targets to top-level builds. When embedded with `add_subdirectory`, compile the same translation units directly into the library so the consumer still gets the code without extra directory targets.
- **Files modified:** `CMakeLists.txt`
- **Verification:** The tracer command passed after the adjustment and again after the task commit; both runs selected and passed 19 checks.
- **Committed in:** `f65d4e8`.

**Total deviations:** 1 auto-fixed (Rule 3 - blocking issue).
**Impact on plan:** Preserved the existing embed contract while meeting the shipped top-level target requirements; no behavior or dependencies were added.

## Verification Evidence

All commands ran from the repository root. Local runs have no hosted run URL.

| Check | Result |
|---|---|
| `cmake --preset ci && cmake --build --preset ci --target schwaystation schwaystation-run schwaystation_libretro && ctest --test-dir build/ci -R '^abi\\.|^install\\.|^embed\\.' --no-tests=error --output-on-failure` | First run exit 8 due to the embed target contract; after the CMake adjustment, exit 0 before commit and exit 0 at the post-commit tracer gate (19/19 selected checks each time). |
| `cmake --preset ci && cmake --build --preset ci --target schwaystation && ctest --test-dir build/ci -R '^policy\\.' --no-tests=error --output-on-failure` | Exit 0 (4/4 selected checks). |
| `LC_ALL=C sort -c tests/cdrom/inventory.txt tests/mdec/inventory.txt tests/disc/inventory.txt` | Exit 0. |
| `ar -t build/ci/libschwaystation.a`; `ninja -C build/ci -t commands schwaystation-run`; `ninja -C build/ci -t commands schwaystation_libretro` with shell guards | Exit 0; CD-ROM and MDEC objects were present in the core archive, and `disc_image` was linked only by runner and libretro. |

## Issues Encountered

No unresolved issues.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Plan 06-02 can add the consumer-minimal disc callback API. DISC-01 through DISC-03 remain pending because this plan establishes build targets and inventories but does not implement device or image behavior.

---
*Phase: 06-discs-and-fmv*
*Completed: 2026-11-15*

## Self-Check: PASSED

All planned files and both task commits exist; the plan ledger measures two task commits from the recorded base.
