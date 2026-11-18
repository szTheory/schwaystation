---
phase: 06-discs-and-fmv
plan: "14"
reviewed: 2026-11-16T20:02:52Z
depth: standard
files_reviewed: 20
files_reviewed_list:
  - .github/workflows/ci.yml
  - CMakePresets.json
  - PROVENANCE.md
  - pins.txt
  - cmake/DiscFixtures.cmake
  - cmake/Mkpsxiso.cmake
  - cmake/Psxavenc.cmake
  - src/cdrom.c
  - src/cdrom.h
  - src/cdrom_sector.c
  - src/cdxa.h
  - tests/cmake/fixture_budget.cmake
  - tests/fixtures/disc_boot.xml
  - tests/fixtures/inventory.txt
  - tests/fixtures/manifest.txt
  - tests/fixtures/src/disc_boot.cnf
  - tests/fixtures/tests.cmake
  - tests/hygiene/tests.cmake
  - tests/policy/inventory.txt
  - tests/policy/tests.cmake
  - tools/gen_disc.py
findings:
  critical: 1
  warning: 2
  info: 0
  total: 3
  necessity: 0
status: issues_found
---

# Plan 06-14: Code Review

**Reviewed:** 2026-11-16T20:02:52Z
**Depth:** standard
**Files Reviewed:** 20
**Status:** issues_found

## Summary

Reviewed the pinned tool integration, media generation and handoff, cue splitting and PVD normalization, fixture caps, workflow consumers, and source provenance. The Linux producer path and generated-media checks are internally connected, but the current Windows executable path prevents the new fixture build from working under the repository's Ninja-based MSVC preset. Two additional portability and stale-pin-cache issues need correction.

## Critical Issues

### CR-01: Windows build looks for mkpsxiso in the wrong directory

**Severity:** BLOCKER
**File:** `cmake/Mkpsxiso.cmake:71-72`; `CMakePresets.json:8,32-39`
**Issue:** The `ci-msvc` preset inherits the single-configuration `Ninja` generator. `ExternalProject_Add` uses that generator and sets `CMAKE_BUILD_TYPE=Release`, which places `mkpsxiso.exe` directly in `mkpsxiso-build/`. The Windows branch instead records `mkpsxiso-build/Release/mkpsxiso.exe`; `DiscFixtures.cmake:76` invokes that nonexistent path while generating the disc. Thus both Windows CI legs fail when building the fixture.
**Fix:** Point the Ninja build at `${schway__mkpsxiso_bin}/mkpsxiso.exe`, or explicitly select a multi-configuration generator and derive the executable path from its configuration.

## Warnings

### WR-01: Local MSVC media-tool target uses Unix-only compiler and linker inputs

**Severity:** WARNING
**File:** `cmake/Psxavenc.cmake:110-125`; `CMakePresets.json:32-39`
**Issue:** When no run-local media artifact is supplied, fixture configuration includes the native psxavenc target and its generated media command builds that target. The target unconditionally passes `-ffast-math` and links `m`; the repository's MSVC configuration uses `cl`, which does not accept that option and has no Unix `m` library. A local Windows `ci-msvc` build therefore cannot produce the disc fixture even when FFmpeg is installed.
**Fix:** Select supported compiler-specific floating-point options and link `m` only on platforms that provide it, or make the local encoder's supported toolchains explicit and provide a Windows media-generation path.

### WR-02: Changed mkpsxiso pins can reuse stale extracted sources

**Severity:** WARNING
**File:** `cmake/Mkpsxiso.cmake:30-46,61-66`
**Issue:** The archive is rechecked against the current pin digest, but extraction is skipped whenever the stable destination already contains its marker file. If the mkpsxiso or submodule pin changes while reusing a build directory, the new archive can be downloaded and verified while CMake still builds the old extracted source at the fixed destination. This silently violates the pin recorded in `pins.txt`.
**Fix:** Key extraction directories by the pinned commit or archive digest, or invalidate and re-extract the destination whenever its archive digest differs from the one previously extracted.

---

_Reviewed: 2026-11-16T20:02:52Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
