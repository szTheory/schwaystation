---
phase: 16-patches-cheats-and-compatibility-evidence
reviewed: 2026-11-26T20:54:42Z
depth: standard
files_reviewed: 40
files_reviewed_list:
  - .github/workflows/ci.yml
  - .github/workflows/nightly.yml
  - CMakeLists.txt
  - CMakePresets.json
  - cmake/DiscTargets.cmake
  - cmake/toolchains/wasm32-wasi.cmake
  - docs/maintaining.md
  - docs/patches.md
  - examples/sdl2/CMakeLists.txt
  - host/cheats.c
  - host/cheats.h
  - host/dat_inventory.c
  - host/dat_inventory.h
  - host/disc_bps.c
  - host/disc_ccd.c
  - host/disc_image.c
  - host/disc_image.h
  - host/disc_patch.c
  - host/disc_patch.h
  - include/schwaystation.h
  - libretro/CMakeLists.txt
  - libretro/libretro.c
  - libretro/schwaystation_libretro.info.in
  - quality.json
  - runner/main.c
  - scripts/compat-doc.sh
  - scripts/compat-ratchet.sh
  - scripts/library.sh
  - src/instance.c
  - tests/cmake/Capabilities.cmake
  - tests/cmake/capabilities_controls.cmake
  - tests/cmake/info_claims.cmake
  - tests/cmake/workflows.cmake
  - tests/compat/tests.cmake
  - tests/disc/patch.c
  - tests/disc/tests.cmake
  - tests/fuzz/tests.cmake
  - tests/hygiene/tests.cmake
  - tests/libretro/host.c
  - tests/runner/tests.cmake
findings:
  critical: 2
  warning: 3
  info: 0
  total: 5
  necessity: 0
status: issues_found
---

# Phase 16: Code Review Report

**Reviewed:** 2026-11-26T20:54:42Z
**Depth:** standard
**Files Reviewed:** 40
**Status:** issues_found

## Summary

Reviewed current HEAD `2b86f1d0c43e5fd225b3c4ae5994f245485b732f`. Coverage from the earlier 25-file review was reused for unchanged files. I reread the later implementation, build, test-reliability, workflow, and documentation changes, and added a targeted review of the WASI toolchain and CCD path because the latest hosted run exposed a runtime failure there. The three earlier findings remain below with their recorded fixed dispositions. Two current issues remain: the WASI runner traps while resolving CCD paths, and Windows compatibility tests cannot hash the DAT because `shasum` is assumed to exist.

## Narrative Findings (AI reviewer)

## Critical Issues

### CR-01: BPS staging prevents the WASI runner from compiling

**Severity:** BLOCKER
**File:** `host/disc_bps.c:107-135`
**Issue:** The previous implementation unconditionally used `tmpfile()` for BPS targets larger than 1 MiB, but the configured WASI libc does not provide that function. That prevented the WASI runner from compiling. This is fixed in the current tree: WASI now creates an exclusive file in the preopened working directory and unlinks it before staging data.
**Fix:** Fixed in the current code with the WASI-specific staging path. See `16-REVIEW-DISPOSITION.md`.

### CR-02: WASI CCD path resolution traps in the 128 KiB runner stack

**Severity:** BLOCKER
**File:** `cmake/toolchains/wasm32-wasi.cmake:5-8,17`; `host/disc_ccd.c:580`; `tests/runner/tests.cmake:1101,2362-2369`
**Issue:** The WASI toolchain links executables with a fixed 128 KiB stack, and the latest hosted WASI run traps with an out-of-bounds memory failure at `realpath(relative, NULL)` while `runner.disc.legacy` opens its supported CCD fixture. Earlier CCD probe cases passed, so this is a runtime failure in the deeper legacy runner path. Stack exhaustion is the leading hypothesis; the current test-only 256 KiB overrides do not apply to `schwaystation-run`. The crash prevents supported CCD loading and fails the required WASI lane.
**Fix:** Reproduce with a 256 KiB stack on the runner to confirm the cause. If that resolves the trap, raise the measured stack size in the toolchain baseline and remove the now-redundant 256 KiB target overrides in `tests/disc/tests.cmake`, `tests/fuzz/tests.cmake`, and `tests/runner/tests.cmake`. Re-measure the WASI runner and update the size bound from that producer. If the larger stack does not resolve it, replace or correct the WASI CCD path-resolution operation while retaining companion-path containment, then add a hosted regression for the CCD runner path.

## Warnings

### WR-01: Nightly fuzz workflow runs targets omitted from its build preset

**Severity:** WARNING
**File:** `CMakePresets.json:228-231`; `.github/workflows/nightly.yml:256-269`
**Issue:** The nightly fuzz loop includes the PPF and cheat replay executables, but the previous `fuzz` build preset omitted those targets, so a clean runner could not execute them. The current preset now builds both fuzzers and replay executables; the finding is retained with its fixed disposition.
**Fix:** Fixed by adding the four targets to the `fuzz` build preset. See `16-REVIEW-DISPOSITION.md`.

### WR-02: Capability importer attributes an unrelated JUnit failure to every selector

**Severity:** WARNING
**File:** `tests/cmake/Capabilities.cmake:184-194`
**Issue:** The previous importer used a report-wide failure flag, causing one failed testcase to mark every capability selector as failed. The current importer reads the matched testcase's own `status` attribute, leaving duplicates and skipped tests unknown, so unrelated results no longer contaminate each other. This is fixed in the current tree and retained with its disposition.
**Fix:** Fixed by assigning each selector from its own JUnit testcase status. See `16-REVIEW-DISPOSITION.md`.

### WR-03: Windows library compatibility checks depend on `shasum`

**Severity:** WARNING
**File:** `scripts/library.sh:25-27`; `tests/compat/tests.cmake:1666-1687`
**Issue:** The library inventory helper invokes only `shasum -a 256`. The Windows hosted compatibility tests run this helper in an environment without `shasum`, so the DAT digest is empty and inventory accounting fails in both Windows legs. This blocks the required test job even though the test runner and inventory code built successfully.
**Fix:** Use a cross-platform hash command available to this workflow, such as `cmake -E sha256sum`, or feature-detect `sha256sum` and `shasum` and fail explicitly if neither exists.

---

_Reviewed: 2026-11-26T20:54:42Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
