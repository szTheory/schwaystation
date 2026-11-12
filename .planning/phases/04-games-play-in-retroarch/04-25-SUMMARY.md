---
phase: 04-games-play-in-retroarch
plan: "25"
subsystem: runner
tags: [iso9660, runner, probe, compatibility]
requires:
  - phase: "04-41"
    provides: [compatibility ledger identity rules and protected judge]
  - phase: "04-01"
    provides: [registered CTest and byte-sorted inventory structure]
provides:
  - Bounded direct ISO9660 metadata extraction for runner identity probing
  - PS-X EXE validation and rcheevos MD5 reporting without booting or playing the disc
  - Registered synthetic identity, fallback and malformed-input coverage
affects: [04-49, phase-04-verification, GAME-06]
actuals:
  tokens: 10165
  tasks: 2
  commits: 3
plan_head_before: 1664413f8166a35ff36aceacfa5a28cae938507e
plan_head_after: a62b9e4470aa34d801e298d9aebd15ef629cde82
tech-stack:
  added: []
  patterns:
    - Host-only bounded ISO metadata parsing, outside the core determinism boundary
    - Hash canonical boot filename followed by the validated PS-X EXE bytes
key-files:
  created:
    - host/disc_probe.c
    - host/disc_probe.h
    - tests/compat/probe_tests.c
  modified:
    - runner/CMakeLists.txt
    - runner/probe.c
    - tests/compat/tests.cmake
    - tests/compat/inventory.txt
    - docs/runner.md
key-decisions:
  - "Probe direct raw ISO9660 metadata only; CUE and other track modes remain with plan 04-49."
  - "Report boot_status and play_status as unavailable; the probe never starts the core or submits sectors."
  - "Verify the synthetic track SHA-1 independently in CMake and use the existing MD5 implementation for production identity."
patterns-established:
  - "Raw ISO probes reject extents outside the volume and unsafe SYSTEM.CNF paths before reporting identity."
requirements-completed: []
coverage:
  - id: D1
    description: "The runner reports the exact boot identity for a bounded synthetic raw ISO without executing it."
    requirement: GAME-06
    verification:
      - kind: integration
        ref: "compat.probe.synthetic_identity"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci"
        status: pass
    human_judgment: false
  - id: D2
    description: "Missing SYSTEM.CNF fallback and malformed, missing or traversing boot inputs are handled with bounded refusals."
    requirement: GAME-06
    verification:
      - kind: integration
        ref: "compat.probe.iso_edges"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset hygiene"
        status: pass
    human_judgment: false
duration: 37min
completed: 2026-11-12
status: complete
---

# Phase 4 Plan 25: Bounded ISO Identity Probe Summary

**The runner now extracts a bounded raw ISO9660 boot identity and reports the exact synthetic serial and RetroAchievements MD5 without booting or playing the image.**

## Performance

- **Duration:** 37 minutes
- **Started:** 2026-11-12T05:05:05Z
- **Completed:** 2026-11-12T05:42:01Z
- **Tasks:** 2
- **Files modified:** 8

## Accomplishments

- Added a host-only raw ISO9660 parser bounded to 1 GiB, 2048-byte sectors, 64 descriptors, 64 KiB directories, 4096 directory records, 2047 ASCII bytes of `SYSTEM.CNF`, 16 path components and 63 bytes per component.
- Reused the core PS-X EXE validator and existing MD5 implementation; the synthetic `SWAY-00001` / `SWAY_000.01` identity matches `12481bed29c6c0d54a6f776ef5cec981`.
- Added a real-runner fallback and refusal suite for missing boot inputs, out-of-volume extents and parent-directory traversal. Disc reports explicitly say booting and playing are unavailable.
- Documented the supported probe surface. CUE parsing, track modes and disc execution remain outside this plan.

## Task Commits

1. **Task 1 RED: registered synthetic identity test** — `700ad86` (`test`)
2. **Task 1 GREEN: bounded ISO identity probe** — `30d0c8a` (`feat`)
3. **Task 2: fallback and refusal edge coverage** — `a62b9e4` (`fix`)

The separate plan metadata commit contains this summary and the state/roadmap updates.

## Files Created/Modified

- `host/disc_probe.c`, `host/disc_probe.h` — bounded direct ISO9660 boot metadata extraction outside the core.
- `runner/probe.c`, `runner/CMakeLists.txt` — report validated boot identity through the existing runner probe.
- `tests/compat/probe_tests.c`, `tests/compat/tests.cmake`, `tests/compat/inventory.txt` — generated fixtures and registered identity/refusal checks.
- `docs/runner.md` — raw ISO probe fields, limits and unavailable boot/play behavior.

## Decisions Made

- Production calculates the rcheevos MD5 from the canonical boot filename and validated executable bytes; CMake independently checks the generated track SHA-1 against `b7a1f733d515077a111f6d792d6549fde163f912`.
- A missing `SYSTEM.CNF` falls back to root `PSX.EXE`; if neither boot input exists, the runner refuses the image without outputting identity metadata.
- This slice recognizes direct raw ISO9660 images only. CUE and track mode handling remain with 04-49.

## Evidence

- Task 1 RED: `compat.probe.synthetic_identity` failed because the runner still treated the generated ISO as a PS-EXE; the failing registered test was committed before the implementation.
- Task 1 GREEN and tracer feedback: `cmake --preset ci && cmake --build --preset ci --target schwaystation-run schwaystation-test-probe && ctest --test-dir build/ci -R '^compat\\.probe\\.synthetic_identity$' --no-tests=error --output-on-failure` — exit 0 before and after the implementation commit.
- Task 2: `cmake --preset ci && cmake --build --preset ci --target schwaystation-run schwaystation-test-probe && ctest --test-dir build/ci -R '^compat\\.probe\\.(iso_edges|synthetic_identity)$' --no-tests=error --output-on-failure` — exit 0; both registered cases passed.
- Final `cmake --workflow --preset ci` — exit 0; all 490 CI tests and all 5 release checks passed.
- Final `cmake --workflow --preset hygiene` — exit 0; all 23 hygiene checks passed, including the approved 2300-line repository-check budget.
- `LC_ALL=C sort -c tests/compat/inventory.txt` and `git diff --check` — exit 0.

## Deviations from Plan

### Auto-fixed Issues

**1. Rule 3 — Added the standard declaration for the bounded seek check**
- **Found during:** Task 1 build
- **Issue:** The new offset guard uses `LONG_MAX`, which requires `<limits.h>`.
- **Fix:** Included the standard header and rebuilt with warnings as errors.
- **Files modified:** `host/disc_probe.c`
- **Verification:** Focused runner build and synthetic identity test passed.
- **Committed in:** `30d0c8a`

**2. Rule 1 — Corrected the synthetic test's multiline output match**
- **Found during:** Task 1 GREEN verification
- **Issue:** CMake's regex did not match the runner's line breaks, despite the expected fields being present.
- **Fix:** Asserted the required fields independently using tolerant patterns.
- **Files modified:** `tests/compat/tests.cmake`
- **Verification:** Synthetic identity test passed before and after the implementation commit.
- **Committed in:** `30d0c8a`

**3. Rule 1 — Classified malformed boot extents as invalid ISO metadata**
- **Found during:** Task 2 edge verification
- **Issue:** An out-of-volume extent was rejected but reported as an invalid boot path.
- **Fix:** Propagated malformed directory records as invalid ISO metadata and kept unsafe `SYSTEM.CNF` paths as metadata refusals.
- **Files modified:** `host/disc_probe.c`
- **Verification:** `compat.probe.iso_edges` passed with empty stdout and the expected refusal.
- **Committed in:** `a62b9e4`

**Total deviations:** 3 auto-fixed issues. All were needed to compile, verify or accurately classify the planned behavior.

## User Setup Required

None — no external service or local configuration is required.

## Next Phase Readiness

Plan 04-49 can build on the validated raw ISO metadata path to resolve CUE tracks and sector layouts. This plan does not claim disc boot or playback support.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-12*

## Self-Check: PASSED

- Summary file exists at the planned path.
- Task commits `700ad86`, `30d0c8a` and `a62b9e4` are ancestors of HEAD.
- The persisted plan ledger measures three commits from `1664413f8166a35ff36aceacfa5a28cae938507e` through `a62b9e4470aa34d801e298d9aebd15ef629cde82`.
