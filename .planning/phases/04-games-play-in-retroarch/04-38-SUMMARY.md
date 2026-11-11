---
phase: 04-games-play-in-retroarch
plan: "38"
subsystem: core-bios
tags: [C, CMake, BIOS, MD5, SHA-1, SHA-256, API-v4]

requires:
  - phase: 04-20
    provides: Copied exact-size BIOS storage and independent public SHA-256 identity.
provides:
  - Pinned DAT MD5 recognition with public BIOS status and region results.
  - Retained SHA-1 row metadata and source attribution without BIOS bytes.
  - Gating recognition, isolation, size-window and reset tests.
affects: [GAME-05, BIOS metadata, API-v4, firmware scanning]

actuals:
  tokens: 12695
  tasks: 2
  commits: 4
commits: 4
plan_head_before: 9cb3fda5f15c9744c2ba145522670b3d0292db17
plan_head_after: 0d169c5fa619dc744083fa4b7e209167fca17a81

tech-stack:
  added: []
  patterns:
    - Match BIOS bytes against instance-owned digest metadata without host I/O.

key-files:
  created:
    - src/bios_table.inc
  modified:
    - include/schwaystation.h
    - src/bios.c
    - src/internal.h
    - CMakeLists.txt
    - runner/CMakeLists.txt
    - tests/api/v4.c
    - tests/built/bios_tests.c
    - tests/built/tests.cmake
    - tests/built/inventory.txt
    - pins.txt
    - THIRD-PARTY-NOTICES.md
    - docs/embedding.md
    - ASSET_POLICY.md

key-decisions:
  - "Match pinned retail rows by their actual DAT MD5 values; retain paired SHA-1 metadata and keep public SHA-256 computed from the loaded image."
  - "Compile the single synthetic lookup row only in schwaystation-test-bios-synthetic; leave unknown images, including OpenBIOS, loadable."

patterns-established:
  - "Expose recognition status and region as append-only size-tagged API fields, preserving the original 40-byte prefix."
  - "Share pure hash objects with the core and runner without pulling host I/O into core objects."

requirements-completed: []
coverage:
  - id: D1
    description: "A loaded exact-size image reports its computed SHA-256 and is recognized with the pinned DAT region when its MD5 matches; unknown images remain loadable."
    requirement: GAME-05
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^built\\.bios\\.identity$' --no-tests=error --output-on-failure"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci"
        status: pass
    human_judgment: false
  - id: D2
    description: "Synthetic recognition keys stay out of production; the legacy 40-byte output prefix, undersized-output atomicity, and reset identity behavior are covered."
    requirement: GAME-05
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^built\\.bios\\.identity_edges$' --no-tests=error --output-on-failure"
        status: pass
      - kind: unit
        ref: "tests/api/v4.c; abi.api_v4 and abi.api_version in cmake --workflow --preset ci"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci"
        status: pass
    human_judgment: false

duration: 51min
completed: 2026-11-11
status: complete
---

# Phase 04 Plan 38: BIOS Identity from Pinned DAT Digests Summary

**The core now reports retail BIOS recognition from pinned DAT MD5 rows while keeping image SHA-256 independent and unknown firmware loadable.**

## Performance

- **Duration:** 51 min
- **Started:** 2026-11-11T14:18:36Z
- **Completed:** 2026-11-11T15:09:13Z
- **Tasks:** 2
- **Files modified:** 14
- **Runtime/model:** Codex / OpenGSD 1.20.0, GPT-6 Luna xhigh, sequential shared phase checkout.

## Accomplishments

- Added 17 pinned libretro `System.dat` digest pairs to a static table. The core matches MD5 and reports status and region; SHA-1 stays as source metadata, while SHA-256 remains computed from each loaded image.
- Added append-only BIOS status and region fields and a recognized flag to API v4 while preserving its original 40-byte output prefix.
- Kept the synthetic matching key in a separate test binary, verified production does not recognize it, and retained exact-size loading for unknown images.
- Shared the pure MD5 implementation with the core and runner, and documented the pinned source, raw-file SHA-256 and CC BY-SA 4.0 notice. No BIOS image bytes were added.

## Evidence

- **TDD RED:** The command ctest --test-dir build/ci -R '^built\.bios\.identity$' --no-tests=error --output-on-failure --output-junit bios-red.xml exited 8 on the intended UNKNOWN-versus-RECOGNIZED status and region assertions. `gsd_run check tdd-red-evidence build/ci/bios-red-evidence.json --raw` returned `RED_EVIDENCE_OK`; the JUnit report and semantic assessment are recorded in the uncommitted build evidence file.
- **GREEN:** The configured BIOS test binaries and API v4 caller built, and the focused `built.bios.identity` test passed. The post-commit tracer rerun also exited 0.
- **Expanded checks:** `built.bios.identity` and `built.bios.identity_edges` both passed. `runner.hash` and `exe.md5_rfc1321` passed after hash-object sharing.
- **Pinned source:** The command gh api repos/libretro/libretro-database/commits/12c21cf79f1e20eff6ac68b80875816c717d0a7a --jq .sha returned the pinned commit. The raw dat/System.dat SHA-256 is `57f83609a28122b0e5e0bfc6a1993a3b512580698be7dd2affb307913087544b`; an independent table comparison matched all 17 MD5/SHA-1 pairs and regions in `ASSET_POLICY.md`.
- **Final CI:** `cmake --workflow --preset ci` exited 0; all 463 tests and both release archive checks passed.
- **Final hygiene:** `cmake --workflow --preset hygiene` exited 0; all 23 repository and policy checks passed.
- All checks ran locally and headlessly; there is no hosted run URL for this slice.

## TDD Evidence

- **RED:** The API test compiled and the named target executed. It failed only because the pre-implementation getter returned UNKNOWN status and region for the synthetic digest row. The classifier accepted the fresh JUnit report.
- **GREEN:** Matching the loaded bytes against the isolated test row made the expected recognized status, region and public SHA-256 pass.
- **REFACTOR:** No separate cleanup was needed after the minimal implementation.

## Task Commits

1. **Task 1 RED:** `faac213` — add failing BIOS identity test.
2. **Task 1 GREEN:** `df35262` — recognize BIOS images from pinned DAT digests.
3. **Task 2:** `43efac0` — cover BIOS identity API edges.
4. **Rule 3 integration fix:** `0d169c5` — keep BIOS identity API symbols covered.

## Files Created/Modified

- `src/bios.c`, `src/bios_table.inc`, `include/schwaystation.h`, `src/internal.h` — MD5 recognition, per-region results and append-only API sizing.
- `CMakeLists.txt`, `runner/CMakeLists.txt` — shared pure hash implementations.
- `tests/api/v4.c`, `tests/built/bios_tests.c`, `tests/built/tests.cmake`, `tests/built/inventory.txt` — identity answers, isolated synthetic lookup and API edge coverage.
- `pins.txt`, `THIRD-PARTY-NOTICES.md`, `ASSET_POLICY.md`, `docs/embedding.md` — source pin, attribution, digest facts and public behavior.

## Decisions Made

- Retail recognition uses the pinned DAT MD5 facts; SHA-1 values are retained, and the API's SHA-256 is computed from the loaded bytes rather than inferred from the DAT.
- A recognized flag accompanies the status and region result. Unknown rows do not block exact-size BIOS loading.
- GAME-05 remains incomplete until its separate firmware archive and OpenBIOS plans finish.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Integration] Kept new BIOS API constants in the v4 compatibility caller.**
- **Found during:** Final CI.
- **Issue:** `abi.api_version` requires every new public API symbol to appear in a `tests/api/vK.c` caller; the first full CI run exited 8 because three new BIOS constants were not referenced there.
- **Fix:** Added numeric assertions for the recognized flag, recognized status and region-free region value in `tests/api/v4.c`.
- **Verification:** The focused `abi.api_version` guard and API v4 executable passed; the final full CI workflow passed.
- **Committed in:** `0d169c5`.

**Total deviations:** 1 auto-fixed integration issue. **Impact:** Kept the existing API version guard effective; no new scope or requirement was added.

## Issues Encountered

The first full CI run found the v4 caller omission described above. It was fixed and the complete workflow passed afterward. No unresolved issues remain.

## User Setup Required

None. No BIOS files, personal paths, external credentials or owner verification are required.

## Next Phase Readiness

Plan 04-38 is complete. The archive scanner in 04-21 and the pinned OpenBIOS build in 04-22 remain separate future slices. Retail firmware bytes were not used, so the table path is validated with an isolated synthetic key and the actual public digest facts are checked against their pinned source.

## Self-Check: PASSED

- SUMMARY file exists at the planned phase path.
- Task commits `faac213`, `df35262`, `43efac0`, and `0d169c5` are ancestors of HEAD.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-11*
