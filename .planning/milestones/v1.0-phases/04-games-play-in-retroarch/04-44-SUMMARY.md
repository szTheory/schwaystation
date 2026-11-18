---
phase: 04-games-play-in-retroarch
plan: "44"
subsystem: libretro
tags: [libretro, memory, memory-cards, descriptors]
requires:
  - phase: 04-28
    provides: Libretro controller configuration and the established adapter lifecycle.
  - phase: 04-34
    provides: Host-owned memory-card import and atomic persistence helpers.
  - phase: 04-35
    provides: Canonical save basenames and save-directory path handling.
  - phase: 04-37
    provides: Core card spans and successful-write generation tracking.
provides:
  - Stable libretro SAVE_RAM and SYSTEM_RAM spans, plus RAM and scratchpad alias descriptors.
  - Card-two persistence to the canonical basename.1.mcr path after successful guest writes.
affects: [04-45, libretro metadata claims, frontend memory-card integration]
actuals:
  tokens: 9149
  tasks: 2
  commits: 5
commits: 5
plan_head_before: 8f4c713b2314f21968f0e11dc0a5cfcaa2c03139
plan_head_after: ae6a56d80dc0c6b76496032816b112baf2c60029
tech-stack:
  added: []
  patterns:
    - Expose stable spans from existing memory APIs and clear descriptors with adapter lifetime.
    - Persist host-owned card files atomically only when the guest write generation changes.
key-files:
  created: []
  modified:
    - libretro/libretro.c
    - tests/libretro/host.c
    - tests/libretro/tests.cmake
    - tests/libretro/inventory.txt
    - docs/embedding.md
key-decisions:
  - Keep card-one persistence owned by the frontend; the core exposes SAVE_RAM and never writes an .srm file.
  - Resolve card two from GET_SAVE_DIRECTORY and the canonical content basename, committing only after successful guest writes.
  - Build descriptors over existing RAM and scratchpad spans, with explicit aliases, holes, and an unmapped sentinel.
patterns-established:
  - Libretro memory spans are available only while an instance is loaded and are cleared on unload or deinit.
  - Descriptor alias resolution is checked independently at both endpoints, mirrors, selectors, disconnect bits, and holes.
requirements-completed: [GAME-04]
coverage:
  - id: D1
    description: Stable 128 KiB SAVE_RAM is exposed for card one, while card two reloads from and atomically persists to basename.1.mcr only after successful guest writes.
    requirement: GAME-04
    verification:
      - kind: integration
        ref: ctest:libretro.cards.persistence
        status: pass
      - kind: integration
        ref: ctest:libretro.cards.controls
        status: pass
    human_judgment: false
  - id: D2
    description: Libretro memory descriptors cover main RAM and scratchpad aliases, reject unrelated addresses, and clear with adapter lifecycle changes.
    requirement: GAME-04
    verification:
      - kind: integration
        ref: ctest:libretro.memory.aliases
        status: pass
      - kind: integration
        ref: ctest:libretro.memory.boundaries
        status: pass
    human_judgment: false
duration: 53min
completed: 2026-11-12
status: complete
---

# Phase 04 Plan 44: Persist libretro cards and resolve memory aliases Summary

**Libretro now exposes stable memory spans and maps aliases, while card two persists atomically through the host save directory.**

## Performance

- **Duration:** 53 minutes
- **Started:** 2026-11-12T15:18:29Z
- **Completed:** 2026-11-12T16:11:38Z
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments

- Exposed stable 128 KiB card-one SAVE_RAM and 2 MiB SYSTEM_RAM spans while loaded, without writing frontend-owned `.srm` files.
- Persisted card two at the content basename's `.1.mcr` path, loading fresh card contents and atomically saving only after a successful guest write.
- Registered independent controls for missing, malformed, unchanged, failed-persistence, alias, selector, hole, reset, unload, reload, and deinit behavior; documented the host contract.

## Evidence

- `ctest --test-dir build/ci --no-tests=error -R '^libretro\.cards\.persistence$' --output-on-failure` — exit 0, one case.
- `ctest --test-dir build/ci --no-tests=error -R '^libretro\.cards\.controls$' --output-on-failure` — exit 0, one case.
- `ctest --test-dir build/ci --no-tests=error -R '^libretro\.memory\.aliases$' --output-on-failure` — exit 0, one case.
- `ctest --test-dir build/ci --no-tests=error -R '^libretro\.memory\.boundaries$' --output-on-failure` — exit 0, one case.
- `ctest --test-dir build/ci --no-tests=error -R '^libretro\.host$' --output-on-failure` and the corresponding `^libretro\.host\.pal$` invocation — each exit 0.
- `cmake --workflow --preset ci` — exit 0; all 505 CI tests and all 5 release archive checks passed.
- `cmake --workflow --preset hygiene` — exit 0; all 23 checks passed.

## TDD Gate Compliance

- **RED:** The registered `libretro.cards.persistence` case failed on missing SAVE_RAM/card-two behavior; `libretro.memory.aliases` independently failed on the absent map. Both records passed `gsd_run check tdd-red-evidence`. An additional selector RED caught an incorrect RAM select mask at the final in-region byte.
- **GREEN:** The implementation exposed the stable memory spans, registered and cleared descriptors with instance lifetime, and persisted card two through the existing atomic helper. The persistence and alias cases passed separately.
- **REFACTOR:** Added failure and lifecycle controls, updated existing NTSC/PAL host expectations for the new memory contract, and documented behavior. Each required case, the full CI/release workflow, and hygiene passed.

## Task Commits

1. **Task 1 RED: card persistence** — `996941c` (test)
2. **Task 1 RED: memory aliases** — `699c013` (test)
3. **Task 1 GREEN: expose memory and persist card two** — `3df57f9` (feat)
4. **Task 2: add failure controls and lifecycle coverage** — `6418a77` (test)
5. **Compatibility correction: update legacy host memory expectations** — `ae6a56d` (test)

## Files Created/Modified

- `libretro/libretro.c` — exposes SAVE_RAM and SYSTEM_RAM, registers alias descriptors, and owns card-two adapter persistence.
- `tests/libretro/host.c` — exercises persistence, refusal/failure controls, map aliases, and adapter lifecycle.
- `tests/libretro/tests.cmake` — registers the four required host cases.
- `tests/libretro/inventory.txt` — lists the new cases in byte-sorted order.
- `docs/embedding.md` — documents memory, descriptor, and card ownership behavior.

## Decisions Made

- The frontend owns card-one `.srm` writes; the core only exposes its stable SAVE_RAM span.
- Card two uses `GET_SAVE_DIRECTORY` and the canonical content basename; a generation change gates the existing atomic save helper.
- Memory descriptors point into existing API spans and are reset with the adapter lifecycle. Unavailable host map registration does not prevent the core from loading.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Corrected RAM descriptor selection at the last in-region byte**
- **Found during:** Task 1 (memory alias RED)
- **Issue:** The initial selector allowed an address at the end of the 2 MiB span to resolve incorrectly.
- **Fix:** Extended the selector mask to exclude the complete in-region offset and retested endpoint, mirror, and hole behavior.
- **Files modified:** `libretro/libretro.c`, `tests/libretro/host.c`
- **Verification:** `libretro.memory.aliases` and the full CI workflow passed.
- **Committed in:** `3df57f9`

**2. [Rule 1 - Bug] Updated legacy host expectations for exposed memory spans**
- **Found during:** Final CI workflow
- **Issue:** Existing NTSC and PAL host checks still asserted that this release exposed no memory regions, so they failed after the planned APIs were added.
- **Fix:** Asserted the loaded card-one and main-RAM spans in both existing host paths.
- **Files modified:** `tests/libretro/host.c`
- **Verification:** Both focused host cases and the complete CI and hygiene workflows passed.
- **Committed in:** `ae6a56d`

**Total deviations:** 2 auto-fixed issues.
**Impact on plan:** Both corrections were needed to satisfy the planned alias contract and keep existing gating host tests aligned with the new behavior.

## Issues Encountered

The first complete CI run found the stale host assertions described above. The tests were corrected, the affected cases were rerun individually, and CI and hygiene were rerun after the final source/test change.

## User Setup Required

None — no external service configuration is required.

## Next Phase Readiness

The memory descriptors and card behavior are now exercised in the real in-process libretro host. Plan 04-45 can verify metadata claims against these functional interfaces.

## Self-Check: PASSED

- Summary file exists.
- All five task commits are ancestors of the current HEAD.
- Summary structure and referenced changed files passed `gsd_run verify-summary`.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-12*
