---
phase: 13-regions-console-revisions-and-playback
fixed_at: 2026-11-23T22:10:33Z
review_path: .planning/phases/13-regions-console-revisions-and-playback/13-REVIEW.md
iteration: 1
findings_in_scope: 2
fixed: 2
skipped: 0
status: all_fixed
---

# Phase 13: Code Review Fix Report

**Fixed at:** 2026-11-23T22:10:33Z
**Source review:** `.planning/phases/13-regions-console-revisions-and-playback/13-REVIEW.md`
**Iteration:** 1

**Summary:**
- Findings in scope: 2
- Fixed: 2
- Skipped: 0

## Fixed Issues

### CR-01: Legacy GPU override makes a valid state unloadable

**Files modified:** `src/instance.c`, `src/internal.h`, `src/state.c`, `tests/core/test_state.c`
**Commit:** `b9fc8c7`
**Disposition:** fixed: requires human verification
**Applied fix:** State validation now checks that the saved board identifier and profile flags are individually valid while preserving the documented legacy GPU-v0 override. Added an old-sized-profile save/load/save byte-exact round-trip regression; explicit invalid board/GPU selections remain rejected by the profile API.

### WR-01: Auto region and fast-boot eligibility use the first playlist disc

**Files modified:** `libretro/libretro.c`, `tests/libretro/host.c`, `tests/libretro/tests.cmake`, `cmake/DiscFixtures.cmake`, `tests/fixtures/disc_boot_playlist.xml`, `tests/fixtures/manifest.txt`
**Commit:** `2247086`
**Disposition:** fixed: requires human verification
**Applied fix:** Resolve the validated initial playlist index before probing identity, region and boot-executable eligibility, then reuse that same index when opening the disc. Added deterministic tests where the selected member differs from member zero in region and boot eligibility.

## Verification

Checks ran in the main checkout because `workflow.use_worktrees` is disabled. The full CI workflow was not run; Phase 13 calls for one full workflow run after review fixes.

- `git diff --check` — exit 0.
- `LC_ALL=C sort -cu tests/core/inventory.txt && LC_ALL=C sort -cu tests/libretro/inventory.txt` — exit 0.
- SHA-256 of `tests/fixtures/disc_boot_playlist.xml` matched its entry in `tests/fixtures/manifest.txt` — exit 0.
- `cmake -DROOT=. -DLIMIT=3901 '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -P tests/cmake/line_budget.cmake` — exit 0; budget remains 3901/3901.
- `ctest --preset ci --show-only=json-v1 -R '^libretro[.]host[.]disc_control$|^libretro[.]content[.]boot$'` — exit 0; selected both targets and their two required fixture prerequisites.
- `cmake --preset ci` — exit 0.
- `cmake --build --preset ci --target schwaystation-test-libretro-host schwaystation_libretro schwaystation-libretro-card-test` — exit 0.
- `ctest --preset ci --no-tests=error -R '^libretro[.]host[.]disc_control$|^libretro[.]content[.]boot$' --output-on-failure` — exit 0; 4/4 tests passed including fixture prerequisites.

The first focused CTest attempt exited 8 because the host's mode guard still expected the old argument count. The guard and usage text were updated, and the same focused CTest command then passed 4/4.

---

_Fixed: 2026-11-23T22:10:33Z_
_Fixer: the agent (gsd-code-fixer)_
_Iteration: 1_
