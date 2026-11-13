---
phase: 04-games-play-in-retroarch
plan: "21"
subsystem: testing
tags: [cmake, release, BIOS, firmware, archives]

requires:
  - phase: 04-01
    provides: Per-directory test registration and sorted inventory pattern.
  - phase: 04-38
    provides: Pinned DAT MD5/SHA-1 table and production BIOS identification path.
  - phase: 04-13
    provides: CI workflow used for final test and release verification.
provides:
  - Release scan of tracked files and every archive member for listed BIOS digests and ASSET_POLICY firmware markers.
  - Synthetic positive, clean, nested-member, malformed-archive, zero-member and production-key-isolation controls.
affects: [GAME-05, release archive scanning, BIOS metadata]

actuals:
  tokens: 6066
  tasks: 2
  commits: 2
commits: 2
plan_head_before: 7b1cb0a48dd890ae0a78587d723e151e31705ace
plan_head_after: ce2dcb13fc5a33c0e7ca1f1c3c56f1a17201cfa4

tech-stack:
  added: []
  patterns:
    - Extract archive members outside both the tracked tree and archive directories before scanning them.
    - Generate synthetic signatures and digest fixtures only in the build tree.

key-files:
  created:
    - tests/cmake/firmware_scan.cmake
  modified:
    - tests/release/tests.cmake
    - tests/release/inventory.txt
    - tests/built/bios_tests.c
    - tests/built/tests.cmake
    - tests/built/inventory.txt
    - THIRD-PARTY-NOTICES.md
    - docs/testing.md
    - .planning/todos/completed/013-firmware-scan-gate.md

key-decisions:
  - "Keep the scanner aligned with ASSET_POLICY by checking the existing release gate for firmware-size and content markers as well as DAT digests."
  - "Keep synthetic digest rows and signature fixtures in build-only test inputs; production identification remains limited to pinned rows."

requirements-completed: []
coverage:
  - id: D1
    description: "The release scanner checks tracked files and every release archive member, independent of suffix, for pinned BIOS digests and the existing firmware size/signature policy."
    requirement: GAME-05
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^release\\.firmware_scan\\.self_test$' --no-tests=error --output-on-failure"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci"
        status: pass
    human_judgment: false
  - id: D2
    description: "Nested matches and prohibited content are rejected; changed-byte input passes, while empty and malformed archives fail as infrastructure errors."
    requirement: GAME-05
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^release\\.firmware_scan\\.archive_edges$' --no-tests=error --output-on-failure"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset hygiene"
        status: pass
    human_judgment: false
  - id: D3
    description: "The production BIOS identification path leaves a synthetic-only digest unknown."
    requirement: GAME-05
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^built\\.bios\\.test_key_isolation$' --no-tests=error --output-on-failure"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci"
        status: pass
    human_judgment: false

duration: 44min
completed: 2026-11-11
status: complete
---

# Phase 04 Plan 21: Release Firmware Scan and Key Isolation Summary

**The release scanner now rejects DAT-matched firmware and the existing size/signature markers across tracked files and every archive member, with synthetic edge controls and production-key isolation.**

## Performance

- **Duration:** 44 min
- **Started:** 2026-11-11T15:11:00Z (approximate)
- **Completed:** 2026-11-11T15:55:01Z
- **Tasks:** 2
- **Files modified:** 9 task files
- **Runtime/model:** Codex, OpenGSD 1.20.0; sequential executor, gpt-6-luna xhigh

## Accomplishments

- Adapted the pinned MIT muhtari scanner seed to read this project's 17 production DAT MD5/SHA-1 rows and scan tracked files plus every extracted release archive member, regardless of suffix.
- Preserved the existing ASSET_POLICY scanner contract for exact 524,288-byte members, PS-X EXE, both CD001 offsets and every 32-byte Sony licence window. All marker comparisons use hex.
- Registered synthetic matching, changed-byte clean, nested-member, malformed-archive, zero-member and content-marker controls; the production BIOS binary confirms the synthetic key stays unknown.
- Closed pending todo 013 and updated the scanner notice and testing documentation. No real firmware bytes were used.

## Evidence

- `ctest --test-dir build/ci -R '^release\\.firmware_scan\\.archive_edges$' --no-tests=error --output-on-failure` — exit 0 after the per-member marker assertions were tightened.
- `cmake --workflow --preset ci` — exit 0; all CI and packaged-release tests passed.
- `cmake --workflow --preset hygiene` — exit 0; all hygiene checks, including the 1,850-line repository-check budget, passed.

## Task Commits

1. **Reject firmware digests in tracked files and release archives** — `4f9da20` (`feat`).
2. **Expand and document the proven path** — `ce2dcb1` (`feat`).

## Files Created/Modified

- `tests/cmake/firmware_scan.cmake` — tracked-file and archive-member scanning, with DAT digest and content-marker refusal.
- `tests/release/tests.cmake` and `tests/release/inventory.txt` — matching, clean, archive-edge and content-marker controls.
- `tests/built/bios_tests.c`, `tests/built/tests.cmake` and `tests/built/inventory.txt` — production-table isolation control.
- `THIRD-PARTY-NOTICES.md` — MIT seed attribution and adaptation notes.
- `docs/testing.md` — scanner scope and synthetic-control documentation.
- `.planning/todos/completed/013-firmware-scan-gate.md` — completed deferred scanner item.

## Decisions Made

- Kept existing ASSET_POLICY checks within `release.firmware_scan`; no additional gate was introduced.
- Generated digest keys and content-marker samples only in build-directory fixtures, leaving production recognition based on pinned DAT rows.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Existing contract] Preserved ASSET_POLICY release-scan checks**
- **Found during:** Task 2 (Expand and document the proven path)
- **Issue:** The digest-only implementation did not meet ASSET_POLICY.md §6, which also requires the same release scanner to reject BIOS-sized files, PS-X EXE and CD001 signatures, and Sony licence windows.
- **Fix:** Added those checks to the existing scanner and synthetic archive controls for each marker, without adding a gate.
- **Files modified:** `tests/cmake/firmware_scan.cmake`, `tests/release/tests.cmake`, `docs/testing.md`, `THIRD-PARTY-NOTICES.md`
- **Verification:** The archive-edge test and both final workflows passed.
- **Committed in:** `ce2dcb1`

**Total deviations:** 1 auto-fixed (Rule 2).
**Impact on plan:** Preserved the documented release-scan behavior within the already named gate.

## Issues Encountered

- The first archive-edge run used a stale match-message expression after the scanner began reporting specific content markers. The assertion was updated to verify each member/reason pair; the focused test then passed.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

This scanner slice is complete. GAME-05 remains open for the separately planned pinned Linux OpenBIOS work and other remaining requirement coverage.

## Self-Check: PASSED

The scanner and summary files exist, and both task commits are ancestors of the current plan head.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-11*
