---
phase: 13-regions-console-revisions-and-playback
reviewed: 2026-11-23T21:52:33Z
depth: standard
files_reviewed: 43
files_reviewed_list:
  - cmake/MDECReferences.cmake
  - docs/compatibility.md
  - docs/embedding.md
  - docs/libretro.md
  - docs/runner.md
  - host/disc_probe.c
  - host/disc_probe.h
  - include/schwaystation.h
  - libretro/CMakeLists.txt
  - libretro/libretro.c
  - runner/CMakeLists.txt
  - runner/main.c
  - runner/probe.c
  - runner/region.c
  - runner/region.h
  - scripts/compat-doc.sh
  - src/bus.c
  - src/cdrom.c
  - src/cpu.c
  - src/instance.c
  - src/internal.h
  - src/state.c
  - tests/abi/inventory.txt
  - tests/abi/tests.cmake
  - tests/api/v8.c
  - tests/cdrom/inventory.txt
  - tests/cdrom/model.c
  - tests/cdrom/tests.cmake
  - tests/cmake/compat-ratchet.cmake
  - tests/compat/features.csv
  - tests/compat/inventory.txt
  - tests/compat/tests.cmake
  - tests/core/inventory.txt
  - tests/core/test_core.c
  - tests/core/test_state.c
  - tests/core/tests.cmake
  - tests/libretro/host.c
  - tests/libretro/inventory.txt
  - tests/libretro/tests.cmake
  - tests/mdec/color_reference.c
  - tests/nightly/inventory.txt
  - tests/runner/inventory.txt
  - tests/runner/test_region_probe.c
  - tests/runner/tests.cmake
findings:
  critical: 1
  warning: 1
  info: 0
  total: 2
  necessity: 0
status: issues_found
---

# Phase 13: Code Review Report

**Reviewed:** 2026-11-23T21:52:33Z
**Depth:** standard
**Files Reviewed:** 43
**Status:** issues_found

## Summary

The review found one state compatibility defect and one playlist-selection defect. Both affect supported user flows. No tests were run as requested for this review. The repository necessity rubric was read; no necessity finding was supported by the reviewed code.

## Critical Issues

### CR-01: Legacy GPU override makes a valid state unloadable

**Classification:** BLOCKER
**File:** src/state.c:112
**Issue:** A caller using an older profile size, or a v8 profile with board zero, is explicitly allowed to keep the current board while using SCHWAY_PROFILE_GPU_V0. The new state validator instead requires the saved GPU flag to match that board's default. A fresh PU-18 instance configured with an old-sized profile and GPU_V0 is accepted by schway_set_profile (src/instance.c:424-449; this case is also exercised in tests/core/test_core.c:365-377), but its state contains board=PU18 and GPU_V0, so loading that same state fails this check. This breaks save-state round trips for a documented compatibility path.
**Fix:** Preserve the legacy override in state validation. For example, encode whether the board/GPU tuple was selected through the new explicit-board field and enforce pairing only for explicit board selections; otherwise validate that the board ID and profile flags are individually valid.

## Warnings

### WR-01: Auto region and fast-boot eligibility use the first playlist disc

**Classification:** WARNING
**File:** libretro/libretro.c:1592-1608, 1655-1668
**Issue:** The disc identity and boot-executable eligibility are probed from prepared_discs[0], but the initial disc may be a different playlist entry selected through lr_disk_set_initial_image and opened later as initial. Consequently, auto region can be chosen from another disc, and fast mode can accept or reject based on another entry's boot executable. This can run the selected title under the wrong region or incorrectly apply the BIOS shell shortcut.
**Fix:** Resolve and validate the initial playlist index/path before probing. Use that selected image for serial, licence, EXE-region and fast-boot eligibility, while retaining disc-specific handling for later swaps. Add a playlist test where the chosen initial member differs in region and boot eligibility from member zero.

---

_Reviewed: 2026-11-23T21:52:33Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
