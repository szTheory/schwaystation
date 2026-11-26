---
phase: 16-patches-cheats-and-compatibility-evidence
reviewed: 2026-11-26T16:05:24Z
depth: standard
files_reviewed: 25
files_reviewed_list:
  - .github/workflows/ci.yml
  - .github/workflows/nightly.yml
  - CMakeLists.txt
  - cmake/DiscTargets.cmake
  - host/cheats.c
  - host/cheats.h
  - host/dat_inventory.c
  - host/dat_inventory.h
  - host/disc_bps.c
  - host/disc_image.c
  - host/disc_image.h
  - host/disc_patch.c
  - host/disc_patch.h
  - include/schwaystation.h
  - libretro/CMakeLists.txt
  - libretro/libretro.c
  - libretro/schwaystation_libretro.info.in
  - runner/main.c
  - scripts/compat-doc.sh
  - scripts/compat-ratchet.sh
  - scripts/library.sh
  - src/instance.c
  - tests/cmake/Capabilities.cmake
  - tests/cmake/info_claims.cmake
  - tests/cmake/workflows.cmake
findings:
  critical: 1
  warning: 2
  info: 0
  total: 3
  necessity: 0
status: issues_found
---

# Phase 16: Code Review Report

**Reviewed:** 2026-11-26T16:05:24Z
**Depth:** standard
**Files Reviewed:** 25
**Status:** issues_found

## Summary

Reviewed committed HEAD `d7ebc888a8f5a65ff900ce1f687bd46618ebe188` against merge base `b2a19abf8a53b7392241cc086ea531d76b31b3d6`. The review covered the patch and cheat C paths, runner and libretro integration, target linking and packaging, capability evidence, library tooling and CI wiring. A preset-equivalent WASI producer confirmed that `tmpfile()` prevents the runner from compiling with the pinned Clang 21.1.8 and wasi-libc configuration. The nightly workflow also invokes newly added fuzz targets that its explicit build preset does not build. JUnit failures are attributed too broadly across capability rows. The expected `is_experimental = true` state remains correct because the required boot-to-play evidence is unknown or absent; this is not a finding.

## Critical Issues

### CR-01: BPS staging prevents the WASI runner from compiling

**Severity:** BLOCKER
**File:** `host/disc_bps.c:111`
**Issue:** Targets larger than 1 MiB unconditionally call `tmpfile()`. In the configured WASI toolchain, wasi-libc marks this function unavailable and `-Werror` turns the diagnostic into a compile failure. This prevents the `wasm32` runner target from building, so the required WASI lane cannot complete. The parent independently reproduced the failure with Clang 21.1.8 and a preset-equivalent producer. The existing Wasmtime test wrapper preopens the build directory as guest `/`; a probe in that pinned configuration verified exclusive file creation followed by unlink and `fdopen` as a usable staging route.
**Fix:** Use a WASI-compatible file-backed staging opener in the preopened build directory, created with exclusive naming and unlinked once open, while preserving the existing bounded target size and cleanup behavior. Keep `tmpfile()` only on platforms where it is supported.

## Warnings

### WR-01: Nightly fuzz workflow runs targets omitted from its build preset

**Severity:** WARNING
**File:** `.github/workflows/nightly.yml:256-269`
**Issue:** The new `ppf` and `cheat` entries are added to the nightly loop, which immediately executes their replay programs. The normal branch builds with `cmake --build --preset fuzz`, but the `fuzz` build preset in `CMakePresets.json:201-227` explicitly lists targets and omits both new fuzzers and their replay executables. On a clean hosted runner, the loop therefore fails when it tries to execute the first missing replay binary; the fuzzer binaries later in the same loop are also missing.
**Fix:** Add `schwaystation-fuzz-ppf`, `schwaystation-fuzz-ppf-replay`, `schwaystation-fuzz-cheat`, and `schwaystation-fuzz-cheat-replay` to the `fuzz` build preset, or explicitly build those targets in the workflow before the loop.
**Disposition:** Seed only; this is CI/tooling code and does not add a product requirement under AGENTS.md Rule 7.

### WR-02: Capability importer attributes an unrelated JUnit failure to every selector

**Severity:** WARNING
**File:** `tests/cmake/Capabilities.cmake:176-208`
**Issue:** The importer sets one report-wide failed flag when it sees any `<failure>`, `<error>`, or `<skipped>` element. It then marks every uniquely matched capability selector as failed, even when that testcase itself passed and a different test failed. This makes the generated compatibility counts and failure evidence inaccurate and can make every capability appear to have failed because of one unrelated test. It fails closed for the experimental marker, so it cannot incorrectly clear that marker.
**Fix:** Inspect the body of each matched `<testcase>` and assign pass, fail, or skipped/unknown from that testcase's own result. Keep unmatched or duplicate selector names unknown.
**Disposition:** Seed only; this is evidence/test tooling and does not add a product requirement under AGENTS.md Rule 7.

---

_Reviewed: 2026-11-26T16:05:24Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
