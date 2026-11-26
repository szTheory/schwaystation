---
phase: 16-patches-cheats-and-compatibility-evidence
reviewed: 2026-11-26T22:08:14Z
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
  critical: 0
  warning: 0
  info: 0
  total: 0
  necessity: 0
status: clean
---

# Phase 16: Code Review Report

**Reviewed:** 2026-11-26T22:08:14Z
**Depth:** standard
**Files Reviewed:** 40
**Status:** clean

## Summary

Reviewed current HEAD `06bb5da9c5213b81c53d5de606a10af87f9b2a37`. Coverage from the earlier 25-file review was reused for unchanged files; I rechecked the implementation, build, test-reliability, workflow, budget, and inventory-emulator changes. The five original findings, the runner emulator finding, and the private-report helper emulator finding are all fixed. The focused inventory suite passed 3/3, and the controls script passed with a multi-part emulator argument. No findings remain on this head; prior findings are retained below as fixed review history. The corrected-head hosted BE32 run later passed both inventory tests and all 694 portable tests; downstream hash equality passed.

## Narrative Findings (AI reviewer)

No open findings remain on current HEAD. The entries below are retained from earlier review snapshots and are fixed in the current tree.

## Critical Issues

### CR-01: BPS staging prevents the WASI runner from compiling

**Severity:** BLOCKER
**File:** `host/disc_bps.c:107-135`
**Issue:** The previous implementation unconditionally used `tmpfile()` for BPS targets larger than 1 MiB, but the configured WASI libc does not provide that function. That prevented the WASI runner from compiling. This is fixed in the current tree: WASI now creates an exclusive file in the preopened working directory and unlinks it before staging data.
**Fix:** Fixed in the current code with the WASI-specific staging path. See `16-REVIEW-DISPOSITION.md`.

### CR-02: WASI CCD path resolution traps in the 128 KiB runner stack

**Severity:** BLOCKER
**File:** `cmake/toolchains/wasm32-wasi.cmake:5-8,17`; `host/disc_ccd.c:580`; `tests/runner/tests.cmake:1101,2362-2369`
**Issue:** The WASI toolchain's 128 KiB stack caused an out-of-bounds runtime trap at `realpath(relative, NULL)` in the `runner.disc.legacy` CCD path. Stack exhaustion was the leading hypothesis. This is fixed: the toolchain baseline is now 256 KiB, the narrower test overrides are removed, and local pinned Wasmtime evidence shows the legacy runner case passes at the new baseline.
**Fix:** Fixed by raising the WASI baseline stack to 256 KiB and removing redundant target-specific overrides. The WASI size measurement was also updated from the local producer; see `16-REVIEW-DISPOSITION.md` and `16-LOCAL-EVIDENCE.md`.

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
**Issue:** The library inventory helper previously invoked only `shasum -a 256`, which is unavailable in the Windows hosted environment. This is fixed: the helper now prefers `sha256sum` and falls back to `shasum`; the recorded native compatibility checks pass.
**Fix:** Fixed by selecting an available SHA-256 utility and preserving explicit failure handling if hashing fails. See `16-REVIEW-DISPOSITION.md`.

### WR-04: BE32 inventory tests invoke the target runner without its emulator

**Severity:** WARNING
**File:** `tests/compat/tests.cmake:1353-1354,1398-1399,1762-1768`
**Issue:** The hosted BE32 `compat.inventory.accounting` job failed because the test passed the big-endian runner through `RUN` and executed it directly from a CMake script. This is fixed in the current tree: the registrations pass the serialized emulator list, script mode decodes it, and every runner invocation in both inventory cases is prefixed with `${EMULATOR}`. The local focused selectors and multi-part emulator script-mode cases passed. Final CI confirmed both inventory cases pass under BE32 and the downstream hash comparison passes.
**Fix:** Fixed by transporting `CMAKE_CROSSCOMPILING_EMULATOR` to both script cases and using it for all runner invocations. A changed-head hosted BE32 run is still needed to verify the full cross-compiled test path; see the remaining warning below.

### WR-05: BE32 inventory controls invoke the test helper without its emulator

**Severity:** WARNING
**File:** `tests/compat/tests.cmake:1495-1496,1553-1554,1646-1651,1762-1768`
**Issue:** Before the latest correction, `schwaystation-test-library` was built as a target executable and therefore used the configured BE32 architecture, but `compat_inventory_controls` invoked it directly twice from CMake script mode. CMake did not automatically apply `CMAKE_CROSSCOMPILING_EMULATOR`, so the host could not run the BE32 ELF. This is fixed: both report-validation invocations now use the decoded `${EMULATOR}` list.
**Fix:** Fixed by prefixing both `${TEST_BINARY}` invocations with `${EMULATOR}` while preserving direct execution when the emulator list is empty. The controls script passed with a multi-part emulator argument, and the focused inventory suite passed 3/3.

---

_Reviewed: 2026-11-26T22:08:14Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
