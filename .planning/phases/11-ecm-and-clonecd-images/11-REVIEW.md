---
phase: 11-ecm-and-clonecd-images
reviewed: 2026-11-22T20:28:55Z
depth: deep
files_reviewed: 40
files_reviewed_list:
  - .github/workflows/ci.yml
  - .github/workflows/nightly.yml
  - CMakePresets.json
  - cmake/DiscFixtures.cmake
  - cmake/DiscTargets.cmake
  - cmake/LegacyDiscFixtures.cmake
  - examples/sdl2/CMakeLists.txt
  - host/disc_ccd.c
  - host/disc_ccd.h
  - host/disc_ecm.c
  - host/disc_ecm.h
  - host/disc_image.c
  - host/disc_probe.c
  - libretro/CMakeLists.txt
  - libretro/libretro.c
  - tests/cmake/info_claims.cmake
  - tests/cmake/info_claims_self_test.cmake
  - tests/cmake/workflows.cmake
  - tests/compat/features.csv
  - tests/compat/tests.cmake
  - tests/disc/ccd.c
  - tests/disc/ecm.c
  - tests/disc/fixture.c
  - tests/disc/image.c
  - tests/disc/inventory.txt
  - tests/disc/legacy.c
  - tests/disc/tests.cmake
  - tests/fixtures/manifest.txt
  - tests/fuzz/ccd.c
  - tests/fuzz/ecm.c
  - tests/fuzz/inventory.txt
  - tests/fuzz/replay_main.h
  - tests/fuzz/tests.cmake
  - tests/libretro/host.c
  - tests/libretro/inventory.txt
  - tests/libretro/tests.cmake
  - tests/runner/inventory.txt
  - tests/runner/tests.cmake
  - tools/gen_ccd.py
  - tools/gen_ecm.py
findings:
  critical: 0
  warning: 1
  info: 0
  total: 1
  necessity: 0
status: issues_found
---

# Phase 11: Code Review Report

**Reviewed:** 2026-11-22T20:28:55Z
**Depth:** deep
**Files Reviewed:** 40
**Status:** issues_found

## Summary

Reviewed the Phase 11 source and integration diff against its ECM/CCD requirements and supported-format decisions. The parser, bounded read paths, companion confinement, consumer wiring, and CI routing did not yield a confirmed blocker. One user-facing metadata description is now incomplete: the advertised extension list includes ECM and CCD, but the core description omits them. No tests were run as part of this read-only review.

## Warnings

### WR-01: Libretro description omits the newly supported image formats

**Severity:** WARNING

**File:** `libretro/CMakeLists.txt:27-34`

**Issue:** `SCHWAYSTATION_INFO_EXTENSIONS` now advertises `.ecm` and `.ccd`, but both CHD-off and CHD-on `SCHWAYSTATION_INFO_DESCRIPTION` strings still enumerate only CUE/ISO/M3U and PBP (plus CHD in the latter branch). The generated RetroArch `.info` therefore advertises the formats in one field while omitting them from its user-facing description.

**Fix:** Update both description strings to mention ECM and CCD among the supported disc image formats.

**Resolution:** Both CHD-off and CHD-on descriptions now name ECM (`.ecm`) and CloneCD (`.ccd`). The metadata guard checks the description, and its self-test rejects a description that omits CloneCD. The CI preset configure and focused metadata selector passed after the fix.

---

_Reviewed: 2026-11-22T20:28:55Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: deep_

## Follow-up: Nightly fixture-closure correction

**Reviewed:** 2026-11-22T21:23:00Z
**Files:** `.github/workflows/nightly.yml`, `tests/cmake/workflows.cmake`
**Result:** No new findings.

The Nightly fuzz job now builds `schwaystation-test-disc-legacy` alongside its legacy fixture producers before the closure CTest selection (`.github/workflows/nightly.yml:212-243`). The two `disc.image.legacy_*` tests invoke that executable, and the ECM/CCD replay tests require the legacy preflight/parity fixtures; building the target before CTest resolves the recorded `Unable to find executable` failure without removing a case or changing the campaign. The existing workflow assertion now requires the target on the fixture-build command, so deleting it will fail the `disc_registration` preflight (`tests/cmake/workflows.cmake:28`). The PBP-only route skips this closure test and does not need the legacy executable.

The orchestrator's targeted local evidence reports the workflow preflight passed, the isolated AppleClang 21 fuzz closure build completed, and the exact four CTest closure cases passed. I did not rerun tests during this read-only review. No security, correctness or robustness issue remains in these two changes.

_Follow-up reviewed: 2026-11-22T21:23:00Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_

## Follow-up: POSIX symlink declaration

**Reviewed:** 2026-11-22T21:28:16Z
**File:** `tests/disc/ccd.c`
**Result:** No findings.

The `_POSIX_C_SOURCE` feature-test macro is defined before every header and only for non-Windows, non-WASI builds where it is unset. That exposes `symlink()` on strict-C17 glibc builds and matches the existing guards around `<unistd.h>` and symlink-dependent tests. Windows and WASI compilation paths remain unchanged. No tests were rerun during this read-only review.

_Follow-up reviewed: 2026-11-22T21:28:16Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
