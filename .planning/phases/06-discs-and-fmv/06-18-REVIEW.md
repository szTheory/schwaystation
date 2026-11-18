---
phase: 06-discs-and-fmv
reviewed: 2026-11-17T03:25:42Z
depth: standard
files_reviewed: 14
files_reviewed_list:
  - docs/hardware/cdrom.md
  - docs/libretro.md
  - include/schwaystation.h
  - libretro/libretro.c
  - libretro/schwaystation_libretro.info.in
  - src/disc.c
  - tests/cmake/info_claims.cmake
  - tests/cmake/info_claims_self_test.cmake
  - tests/core/disc_api.c
  - tests/fixtures/manifest.txt
  - tests/fixtures/src/disc_boot.asm
  - tests/libretro/host.c
  - tests/libretro/inventory.txt
  - tests/libretro/tests.cmake
findings:
  critical: 0
  warning: 0
  info: 0
  total: 0
  necessity: 0
status: clean
---

# Phase 6: Code Review Report

**Reviewed:** 2026-11-17T03:25:42Z
**Depth:** standard
**Files Reviewed:** 14
**Status:** clean

## Summary

Re-review of CR-01 confirms the no-media sequence is now implemented. Selecting an index at or above the image count removes the core medium using an empty TOC while preserving the open tray; the tray can then close with no disc attached. The API and hardware documentation describe this behavior. Focused core and libretro tests pass, including guest-visible GetTN no-media behavior after removal. No findings remain.

## Narrative Findings (AI reviewer)

No open findings.

### CR-01 Resolution: Tray removal now completes

The core accepts an empty TOC only through `schway_swap_disc` while the lid is open, clears the cached TOC, callbacks, attachment flag, and disc identity, and preserves the lid-open state. The libretro adapter invokes that path for both an out-of-range image index and removal of the selected playlist entry; it can then close the empty tray. The core API note and `docs/hardware/cdrom.md` document the contract. The core test checks cleared host state and disc reinsertion; the host test checks eject → remove → close and observes the guest's GetTN no-media error.

**Verification:** `ctest --test-dir build/ci -R '^(core\.disc_api|libretro\.host\.disc_control)$' --no-tests=error --output-on-failure` — exit 0, 2/2 passed.

### Follow-up: extension metadata matches host expectations

All three `retro_get_system_info` assertions in `tests/libretro/host.c` now expect `exe|psexe|cue|iso|m3u`, matching both `libretro.c` and the generated `.info` template. No extension-string mismatch remains.

---

_Reviewed: 2026-11-17T03:25:42Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
