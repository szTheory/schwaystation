---
phase: 04-games-play-in-retroarch
plan: "42"
subsystem: runner
tags: [disc-probe, sha1, metadata, privacy, compatibility]
requires:
  - phase: "04-49"
    provides: [bounded CUE and physical track parsing]
  - phase: "04-01"
    provides: [registered CTest and byte-sorted inventory structure]
provides:
  - Independent streaming SHA-1 for physical first-track identity
  - Exact serial, boot MD5, track SHA-1, DAT title and ledger join for disc identity output
  - Production redaction and bounded malformed-input controls for private disc and executable data
affects: [04-26, phase-04-verification, GAME-06]
actuals:
  tokens: 19408
  tasks: 2
  commits: 3
commits: 3
plan_head_before: b00a828067b17a096be52c97ca99bb0179ddf385
plan_head_after: c3b09a69127b15e4225e4066b0458dd9cc83305f
tech-stack:
  added: []
  patterns:
    - Independently hash physical track bytes in the host-side probe, outside the deterministic core
    - Reveal disc identity only after an exact DAT and metadata-ledger join
    - Keep synthetic metadata switches isolated to the non-installed test runner
key-files:
  created:
    - host/sha1.c
    - host/sha1.h
  modified:
    - CMakeLists.txt
    - docs/runner.md
    - host/disc_probe.c
    - host/disc_probe.h
    - runner/CMakeLists.txt
    - runner/main.c
    - runner/probe.c
    - runner/probe.h
    - tests/compat/inventory.txt
    - tests/compat/probe_tests.c
    - tests/compat/tests.cmake
    - tests/exe/tests.cmake
    - tests/fixtures/tests.cmake
    - tests/runner/tests.cmake
key-decisions:
  - "Expose a disc serial, boot MD5, physical track-1 SHA-1 and DAT title only when all fields match a passing metadata.identity ledger row."
  - "Keep boot_status and play_status unknown; the probe does not execute a disc."
  - "Use the pinned libretro-database PlayStation DAT only as an attributed title source; do not vendor it or add a package."
  - "Keep test metadata arguments in a dedicated non-installed test runner and require an isolated root and marker file."
patterns-established:
  - "Unlisted disc and bare executable names and digests are redacted in the production runner."
  - "Hostile paths and malformed files produce generic input errors without echoing the path."
requirements-completed: []
coverage:
  - id: full-disc-identity
    description: "The runner reports a disc serial, boot MD5, independent physical track-1 SHA-1 and DAT title only for an exact passing metadata.identity join; boot and play remain unknown."
    requirement: GAME-06
    verification:
      - kind: integration
        ref: "compat.probe.full_identity"
        status: pass
      - kind: integration
        ref: "compat.probe.sha1_vectors"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci"
        status: pass
    human_judgment: false
  - id: bounded-private-probe
    description: "Malformed CUE, ISO, metadata and executable inputs are refused within bounded parsing limits, while unmatched names, paths and hashes stay private."
    requirement: GAME-06
    verification:
      - kind: integration
        ref: "compat.probe.malformed"
        status: pass
      - kind: integration
        ref: "compat.probe.cue_edges and compat.probe.iso_edges"
        status: pass
      - kind: other
        ref: "cmake --workflow --preset hygiene"
        status: pass
    human_judgment: false
duration: 40min
completed: 2026-11-12
status: complete
---

# Phase 4 Plan 42: Full Disc Identity Join and Privacy Summary

**The host-side runner probe now reveals a disc identity only after serial, RetroAchievements MD5, independent physical track SHA-1, DAT title and a passing metadata ledger row all match.**

## Performance

- **Duration:** 40 minutes
- **Started:** 2026-11-12T06:25:29Z
- **Completed:** 2026-11-12T07:05:14Z
- **Tasks:** 2
- **Files modified:** 16
- **Runtime/model:** Codex OpenGSD 1.20.0, gpt-6-luna xhigh

## Accomplishments

- Added an owned streaming integer SHA-1 implementation with standard known-answer vectors and used it to hash the physical bytes of track 1 independently of the probe's MD5.
- Joined the synthetic disc's exact serial, boot MD5, independently measured track SHA-1, pinned DAT title and `metadata.identity` ledger row. A complete match reports the metadata date; boot and play remain `unknown`.
- Redacted unlisted disc identities and bare executable names and digests in the production runner. Generic error labels also prevent malformed executable paths from appearing in output.
- Added registered controls for truncated, overlapping and looped ISO metadata, malformed CUE input, absent and unsupported media, invalid extents, wrong or missing join fields, and an all-unavailable ledger. The exact full-identity case rejects redacted or all-unknown output.
- Kept the DAT unvendored, documented its pinned source and CC-BY-SA attribution, and restricted fixture metadata arguments to a dedicated non-installed test runner.

## Task Commits

1. **Task 1 RED: require exact metadata join** — `b81268e` (`test`)
2. **Task 1 GREEN: hash and join complete disc identity** — `bb919f9` (`feat`)
3. **Task 2: redact unmatched identities and expand hostile-input coverage** — `c3b09a6` (`feat`)

The separate plan metadata commit records this summary and the state/roadmap updates.

## Evidence

- Task 1 RED was a registered failure because the runner did not yet produce the complete synthetic identity join; `gsd_run check tdd-red-evidence build/ci/04-42-red.json --raw` accepted the captured failure before the implementation commit.
- The full-identity test and SHA-1 vectors passed through the real test runner; the independent track expectation is computed with CMake `file(SHA1)`.
- Focused parser, privacy and malformed-input retest — 11 selected `exe`, `fixtures`, `runner` and `compat.probe` tests — exit 0.
- `cmake --workflow --preset ci` — exit 0; all 495 CI tests and all 5 release archive checks passed.
- `cmake --workflow --preset hygiene` — exit 0; all 23 hygiene checks passed, including the approved repository-check line budget.
- The compatibility inventory remained byte-sorted; its CI inventory checks passed.

## Deviations from Plan

### Auto-fixed Issues

**1. Rule 1 — Reject `--region` when combined with `--probe`**
- **Found during:** Task 1 integration checks
- **Issue:** `--probe FILE --region pal` was accepted by the parser and attempted to open the file instead of returning a usage error.
- **Fix:** Track whether `--region` was explicitly supplied and reject it in both production and test runner probe modes.
- **Files modified:** `runner/main.c`
- **Verification:** `exe.probe_usage` passed.
- **Committed in:** `c3b09a6`

**2. Rule 1 — Suppress paths from malformed executable errors**
- **Found during:** Task 2 integration checks
- **Issue:** Invalid `--exe` contents still echoed the absolute path even though probe read failures used a generic label.
- **Fix:** Route shared runner file refusals through the generic `input` label and update the regression expectation.
- **Files modified:** `runner/probe.c`, `runner/probe.h`, `tests/runner/tests.cmake`
- **Verification:** `runner.exe.missing` and `exe.probe_refusals` passed.
- **Committed in:** `c3b09a6`

**3. Timing — Completed beyond the 30-minute forecast**
- **Found during:** Task 2 integration checks
- **Issue:** The complete plan took 40 minutes including source work, workflow checks and closeout.
- **Resolution:** Finished the approved acceptance scope and recorded the actual duration. No requirements or verification steps were dropped.

**Total deviations:** 2 correctness fixes and 1 timing deviation.

## User Setup Required

None.

## Self-Check: PASSED

- All source, test and documentation files listed above exist.
- Task commits `b81268e`, `bb919f9` and `c3b09a6` are ancestors of `plan_head_after`.
- The CI, release archive and hygiene workflows passed against the committed implementation.
