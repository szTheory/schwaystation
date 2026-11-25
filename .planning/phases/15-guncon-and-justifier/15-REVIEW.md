---
phase: 15-guncon-and-justifier
reviewed: 2026-11-25T16:17:14Z
depth: deep
files_reviewed: 26
files_reviewed_list:
  - include/schwaystation.h
  - libretro/libretro.c
  - scripts/compat-doc.sh
  - src/frame.c
  - src/gpu.c
  - src/gpu.h
  - src/internal.h
  - src/pad.c
  - src/pad.h
  - tests/abi/inventory.txt
  - tests/abi/tests.cmake
  - tests/api/v10.c
  - tests/api/v9.c
  - tests/compat/features.csv
  - tests/compat/tests.cmake
  - tests/core/test_core.c
  - tests/core/test_state.c
  - tests/gpu/beam.c
  - tests/gpu/inventory.txt
  - tests/gpu/tests.cmake
  - tests/libretro/host.c
  - tests/libretro/inventory.txt
  - tests/libretro/tests.cmake
  - tests/sio/inventory.txt
  - tests/sio/pad_tests.c
  - tests/sio/tests.cmake
findings:
  critical: 2
  warning: 0
  info: 0
  total: 2
  necessity: 0
status: issues_found
---

# Phase 15: Code Review Report

**Reviewed:** 2026-11-25T16:17:14Z
**Depth:** deep
**Files Reviewed:** 26
**Status:** issues_found

## Summary

The review found two blockers. The new display-enable condition activates ordinary GPU beam, blanking, and timer scheduling during the OpenBIOS boot path, matching the reported frame-hash regression. State validation also accepts GunCon or Justifier records in multitap sockets, after which a serial poll indexes two-entry direct-gun arrays with the tap endpoint number. The latter is an out-of-bounds read reachable from a CRC-correct malformed save state.

## Critical Issues

### CR-01: Display enable changes the no-executable BIOS timing path

**Severity:** BLOCKER
**File:** `src/gpu.c:349-353`
**Issue:** The added `display_disabled == 0` term makes the full GPU beam path active as soon as guest software enables display, even when no executable is loaded and `display_mode` is zero. `schway__gpu_next` then schedules line, hblank, and Timer 0 dot events (`src/gpu.c:419-458`), and `schway__gpu_sync` applies beam edges, blank transitions, and timer updates (`src/gpu.c:461-518`). This changes BIOS instruction/interrupt timing for guests that only enabled the display. It is the source-level cause consistent with the supplied identical-invocation evidence: Phase 15 returns `1b819a89fbb391d76011340ee7a762e7583b878e18a79d8f12d4300dbdd79552`, while merge-base `f08fc08d40a1ebcb42708e0a31be3111e3eed711` returns the expected `df2704d7cf3f01ff70ab4534a443729bba77bf73e3488c836513f9f8b86a9372`.
**Fix:** Keep generic beam activation on the pre-change conditions for the no-executable BIOS path. Advance beam timing for gun sampling through an armed-sensor-specific path (or another narrowly scoped condition), rather than enabling all scanline and timer events merely because display output is enabled. Pin the supplied OpenBIOS invocation as a regression check and cover an armed gun with display mode zero separately.

### CR-02: Validated multitap state can route a gun poll into two-entry arrays

**Severity:** BLOCKER
**File:** `src/pad.c:584-588`
**Related:** `src/pad.c:642-679`, `src/pad.h:55-60`, `src/sio.c:402-405, 504-509, 559-561`
**Issue:** State validation only requires tap endpoints to be empty when multitap is disabled; with multitap enabled, it accepts any saved device ID, including GunCon and Justifier. Normal input validation rejects both gun types on multitap sockets (`src/frame.c:201-205`), so the state validator does not enforce the same invariant. After loading a CRC-correct state with `multitap[0] = 1` and a gun device at endpoint 2, a normal socket-1 poll reaches `poll_reply` with `port == 2`. The Justifier branch reads `gun_input[2]`, and the GunCon branch reads `gun_input[2]` and `gun_result_*[2]`; those arrays contain only two elements. This is an out-of-bounds read from externally supplied save-state data and can yield undefined behavior or disclose adjacent instance memory through controller response bytes.
**Fix:** Reject GunCon and Justifier device IDs for every saved tap endpoint regardless of the saved multitap flag, mirroring `schway_set_input`; also guard the gun-specific `poll_reply` branches against `port >= 2` as defense in depth. Add a malformed-state test that updates the state checksum and confirms rejection before a tap poll can run.

---

_Reviewed: 2026-11-25T16:17:14Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: deep_
