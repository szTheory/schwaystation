---
phase: 04-games-play-in-retroarch
plan: "17"
subsystem: card
tags: [memory-card, raw-import, atomic-save, host-helper]

# Dependency graph
requires:
  - phase: 04-games-play-in-retroarch
    provides: Core raw-card import contract and validated pending card transport
provides:
  - Shared host helper for bounded raw card import, read-only lint, and atomic persistence
  - Gating temporary-filesystem coverage for import and persistence failure paths
affects: [runner persistence, libretro saves, card imports]

# Actuals (#2632)
actuals:
  tokens: 6610
  tasks: 2
  commits: 3
commits: 3
plan_head_before: 798810fa2045905eb73b2b8b2114c6886533b4e6
plan_head_after: 3af8a803e9111184d0094d29794b7a327290a938

# Tech tracking
tech-stack:
  added: []
  patterns:
    - Keep card file access in a shared host helper outside the deterministic core
    - Validate into a temporary buffer and publish imports only after successful reads
    - Create exclusive sibling temp files, flush and sync them, then atomically replace the destination

key-files:
  created:
    - host/cards.c
    - host/cards.h
    - tests/card/files.c
  modified:
    - CMakeLists.txt
    - tests/card/tests.cmake
    - tests/card/inventory.txt
    - docs/runner.md

key-decisions:
  - Use exclusive sibling temp names and fail without mutation if all 64 candidates are occupied.

patterns-established:
  - Refused card imports leave the caller's output buffer unchanged.
  - Failed atomic saves preserve the existing destination and do not overwrite occupied temp names.

requirements-completed: []
coverage:
  - id: D1
    description: Short raw card images are FF-padded in memory and linted without rewriting their source file.
    requirement: GAME-04
    verification:
      - kind: unit
        ref: card.files.atomic_roundtrip (CTest)
        status: pass
    human_judgment: false
  - id: D2
    description: Exact-size cards import with clean lint, while empty, oversized, missing, or invalid-output requests preserve the previous buffer.
    requirement: GAME-04
    verification:
      - kind: unit
        ref: card.files.atomic_failures (CTest)
        status: pass
    human_judgment: false
  - id: D3
    description: Atomic saves write exact bytes and preserve the previous image on temp-name exhaustion, flush failure, or replacement failure.
    requirement: GAME-04
    verification:
      - kind: unit
        ref: card.files.atomic_roundtrip and card.files.atomic_failures (CTest)
        status: pass
    human_judgment: false

# Metrics
duration: 23min
completed: 2026-11-11
status: complete
---

# Phase 4 Plan 17: Raw Card File Helper Summary

**Raw card images now import with FF padding and read-only lint, while durable atomic saves preserve prior files on failure.**

## Performance

- **Duration:** 23min
- **Started:** 2026-11-11T09:51:41Z
- **Completed:** 2026-11-11
- **Tasks:** 2
- **Files modified:** 7

## Accomplishments

- Added a shared host helper that imports non-empty raw files up to 131,072 bytes, pads missing bytes with `FFh`, and reports header/checksum lint without rewriting the source.
- Added full-image atomic saves using exclusive sibling temp files, checked writes, `fsync` on POSIX or `_commit` on Windows, and atomic replacement.
- Registered temporary-filesystem tests for successful round trips, invalid imports, occupied temp names, and flush/replacement failures.

## Task Commits

1. **Task 1 RED:** `80360db` — register the failing raw card file test and capture intentional RED evidence.
2. **Task 1 GREEN:** `096905f` — implement bounded imports, lint, and durable atomic saves.
3. **Task 2:** `3af8a80` — cover exact-size imports and atomic failure edges.

## Files Created/Modified

- `host/cards.c`, `host/cards.h` — host-only raw import, lint, and atomic persistence helper.
- `CMakeLists.txt`, `tests/card/tests.cmake`, `tests/card/inventory.txt` — helper linkage and exact gating test registration.
- `tests/card/files.c` — temporary-filesystem success and refusal coverage.
- `docs/runner.md` — raw image and persistence contract.

## Decisions Made

- Kept host file access outside the deterministic core; the helper consumes the existing raw card contract.
- Kept lint advisory: malformed header/checksum data is reported without rejecting or rewriting the imported card.
- Used exclusive sibling temp names so an existing file is never replaced during temp creation.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking issue] Added the public and generated include paths to the new test target.**
- **Found during:** Task 1 RED setup
- **Issue:** The filesystem test included the host card header, which depends on the public header and generated version header.
- **Fix:** Added both include directories to the registered test target.
- **Files modified:** `tests/card/tests.cmake`
- **Verification:** Focused target build and test passed.
- **Committed in:** `096905f`

**2. [Rule 3 - Blocking issue] Made the test-only failure selector conversion explicit.**
- **Found during:** Task 1 GREEN
- **Issue:** The compiler rejected an enum-to-int conversion under the project's sign-conversion warnings.
- **Fix:** Added an explicit cast at the private save-helper boundary.
- **Files modified:** `host/cards.c`
- **Verification:** Focused target build and test passed.
- **Committed in:** `096905f`

No behavior or scope was added beyond the plan.

## Issues Encountered

The initial focused build exposed the missing generated-header include path and a signedness warning. Both were fixed before the passing implementation verification. No unresolved issues remain.

## TDD Gate Compliance

- **RED:** `ctest --test-dir build/ci -R '^card\.files\.atomic_roundtrip$' --no-tests=error --output-on-failure --output-junit build/ci/card-files-red.xml` exited 8. The named test ran and failed at the planned import-success assertion because the temporary helper stub returned `SCHWAY_HOST_CARD_ERR_IO` (3).
- **Machine evidence:** `gsd-tools check tdd-red-evidence build/ci/card-files-red-record.json --raw` returned `RED_EVIDENCE_OK`; the JUnit report contained one failing target test and no report errors.
- **GREEN:** Commit `096905f` followed the RED commit; the focused configure/build/test command passed.
- **Task 2:** Added regression cases for adjacent failure paths; this was test-only expansion and was not treated as a second RED cycle.

## Evidence

- `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-card-files && ctest --test-dir build/ci -R '^card\.files\.atomic_(roundtrip|failures)$' --no-tests=error --output-on-failure` — exit 0.
- `cmake --workflow --preset ci` — exit 0; all 448 CI tests and both release archive checks passed.
- `cmake --workflow --preset hygiene` — exit 0; all 23 hygiene checks passed.

## User Setup Required

None.

## Next Phase Readiness

The host helper and tests are ready for the dependent wrapper/name and runner persistence slices. GAME-04 remains partial until those planned consumers and process-level persistence controls land.

## Self-Check: PASSED

All declared files exist, the three task commits are ancestors of HEAD, the summary validator passed, and the working diff has no whitespace errors.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-11*
