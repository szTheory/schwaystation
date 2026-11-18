---
phase: 06-discs-and-fmv
plan: "08"
subsystem: discs
tags: [m3u, system-cnf, iso9660, c]
requires:
  - phase: 06-01
    provides: versioned host disc callbacks and swap contract
  - phase: 06-07
    provides: bounded host ISO/CUE sector provider
provides:
  - bounded M3U loading with immutable per-index disc descriptors
  - shared bounded SYSTEM.CNF parser and ISO boot metadata probe
  - disc provider format, containment, diagnostics, and lifetime documentation
affects: [phase-06-host-integration, phase-06-disc-fuzzing]
actuals:
  tokens: 16294
  tasks: 2
  commits: 3
commits: 3
plan_head_before: 126228df84ce78a93112954a69b12562393fecb4
plan_head_after: 38df10991370d7c22b352c7a4567808608207055
tech-stack:
  added: []
  patterns:
    - host-owned playlist children return stable descriptors until provider close
    - one allocation-free SYSTEM.CNF parser is shared by provider metadata and identity probing
key-files:
  created:
    - docs/discs.md
  modified:
    - host/disc_image.c
    - host/disc_image.h
    - host/disc_probe.c
    - tests/disc/image.c
    - tests/disc/tests.cmake
    - tests/disc/inventory.txt
key-decisions:
  - "Selection returns a prepared descriptor; only the host's later core swap changes live disc content."
  - "SYSTEM.CNF ISO version suffixes are stripped before resolving boot paths."
requirements-completed: []
coverage:
  - id: D1
    description: "Bounded M3U parsing accepts up to 32 contained ISO/CUE entries, rejects unsafe and unsupported entries, and releases already opened children on failure."
    requirement: DISC-02
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^disc\\.(image|playlist|cnf)' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: "The shared bounded SYSTEM.CNF parser validates boot and kernel metadata; the identity probe resolves the boot file or the documented PSX.EXE fallback."
    requirement: DISC-02
    verification:
      - kind: integration
        ref: "disc.cnf; ctest --test-dir build/ci -R '^compat\\.probe\\.iso_edges$' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
  - id: D3
    description: "Hosts can enumerate and prepare per-index descriptors while the provider's default descriptor remains unchanged until the host performs its core swap."
    requirement: DISC-04
    verification:
      - kind: unit
        ref: "disc.playlist; ctest --test-dir build/ci -R '^disc\\.(image|playlist|cnf)' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
duration: 35m
completed: 2026-11-15
status: complete
---

# Phase 6 Plan 08: Playlist and SYSTEM.CNF Summary

**Bounded M3U selection and a shared SYSTEM.CNF parser now cover safe host disc handoff and boot metadata.**

## Performance

- **Duration:** 35 minutes
- **Started:** 2026-11-15T18:00:23Z
- **Completed:** 2026-11-15T18:35:09Z
- **Tasks:** 2
- **Files modified:** 7
- **Runtime/model:** Codex/OpenGSD 1.20.0; executor gpt-6-luna xhigh

## Accomplishments

- Added contained M3U playlists with 32-entry and 64 KiB limits, bounded paths, CRLF/comments, non-recursive entries, stable index selection, and basename labels.
- Added one allocation-free, 2,047-byte ASCII SYSTEM.CNF parser to the host disc API and routed the identity probe through it; validated BOOT, TCB, EVENT, STACK, and the PSX.EXE fallback.
- Registered independent `disc.playlist` and `disc.cnf` tests and documented supported media, path containment, error details, descriptor lifetimes, and the absence of SBI parsing.

## Task Commits

1. **Task 1: Select contained m3u images and parse boot metadata** — `6d68972` (RED test), `f16cc1d` (implementation).
2. **Task 2: Register playlist/CNF edge cases** — `38df109` (fix, tests, inventory, documentation).

## Files Created/Modified

- `host/disc_image.c`, `host/disc_image.h` — playlist ownership/selection, diagnostics, and shared CNF parsing.
- `host/disc_probe.c` — uses the shared CNF parser while preserving probe fallback and identity behavior.
- `tests/disc/image.c`, `tests/disc/tests.cmake`, `tests/disc/inventory.txt` — generated hostile inputs and registered playlist/CNF selectors.
- `docs/discs.md` — host-facing format, containment, diagnostics, and ownership contracts.

## Decisions Made

- Playlist selection prepares a child descriptor without mutating the provider's default image; the host controls when the core receives a swap.
- ISO `;1` name versions are removed from CNF path components before directory lookup, while the reported boot name stays canonical.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking issue] Removed an obsolete probe helper after parser consolidation.**
- **Found during:** Task 1
- **Issue:** Replacing the probe's private parser left its path-component helper unused, and the warning-as-error build rejected the target.
- **Fix:** Removed the now-unused duplicate helper.
- **Files modified:** `host/disc_probe.c`
- **Verification:** `cmake --build --preset ci --target schwaystation-test-disc-image schwaystation-test-runner schwaystation-run` passed.
- **Committed in:** `f16cc1d`

**2. [Rule 1 - Bug] Normalized ISO version suffixes before resolving CNF boot paths.**
- **Found during:** Task 2
- **Issue:** A valid `SLUS_123.45;1` component was preserved for lookup, but the ISO directory probe compares against the canonical `SLUS_123.45` name and returned `NO_BOOT_EXE`.
- **Fix:** Store the version-free component for lookup and derive the canonical boot name from it.
- **Files modified:** `host/disc_image.h`, `tests/disc/image.c`
- **Verification:** `disc.cnf` and `compat.probe.iso_edges` passed.
- **Committed in:** `38df109`

**3. [Rule 1 - Bug] Rejected unsupported playlist entries before trying to resolve their paths.**
- **Found during:** Task 2
- **Issue:** A missing `.chd` listed in an M3U could be reported as a malformed/missing path instead of the same named unsupported-format result as direct and CUE-child entry paths.
- **Fix:** Classify unsupported extensions before canonical path resolution and assert consistent status/message behavior.
- **Files modified:** `host/disc_image.c`, `tests/disc/image.c`
- **Verification:** `disc.image.cue_reject` and the playlist selector passed.
- **Committed in:** `38df109`

**Total deviations:** 3 auto-fixed (1 blocking build issue, 2 correctness bugs).
**Impact on plan:** All changes were required to complete the planned build and shared parser behavior; no scope was removed.

## Issues Encountered

- The plan's `^runner\.probe` CTest selector has no matching test in this checkout. The required command still ran all eight `disc.image` tests; the actual `compat.probe.iso_edges` selector was also run and passed.
- The first CNF integration run correctly exposed the version-suffix lookup bug listed above; after the fix the full selected suite passed.

## Verification Evidence

- RED evidence: `ctest --test-dir build/ci -R '^disc\.playlist$' --no-tests=error --output-on-failure --output-junit /private/tmp/schway-06-08-red-playlist.xml` — exit 8, failing at the expected missing playlist-provider assertion; the recorded evidence validator returned `RED_EVIDENCE_OK`.
- Task 1: `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-disc-image schwaystation-test-runner && ctest --test-dir build/ci -R '^disc\.image|^runner\.probe' --no-tests=error --output-on-failure` — exit 0, eight selected tests passed.
- Task 2: `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-disc-image schwaystation-test-runner schwaystation-run && ctest --test-dir build/ci -R '^disc\.(image|playlist|cnf)' --no-tests=error --output-on-failure` — exit 0, ten selected tests passed.
- Probe integration: `ctest --test-dir build/ci -R '^compat\.probe\.iso_edges$' --no-tests=error --output-on-failure` — exit 0, one test passed.
- `git diff --check` and `LC_ALL=C sort -c tests/disc/inventory.txt` — exit 0.
- These were local checks; no hosted run URL applies to this plan.

## Known Stubs

None found in the files created or modified by this plan.

## Threat Flags

None beyond the planned bounded host file parsing and path-containment surface.

## Self-Check: PASSED

- Confirmed all eight plan files and the summary exist.
- Confirmed task commits `6d68972`, `f16cc1d`, and `38df109` are ancestors of the current HEAD.

## Next Phase Readiness

The provider now exposes stable per-index descriptors and bounded selection metadata for the next host integration plan. The host remains responsible for deciding when to swap the active core disc.

---
*Phase: 06-discs-and-fmv*
*Completed: 2026-11-15*
