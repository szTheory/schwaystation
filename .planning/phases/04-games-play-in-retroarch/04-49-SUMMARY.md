---
phase: 04-games-play-in-retroarch
plan: "49"
subsystem: runner
tags: [cue, disc-probe, compatibility]
requires:
  - phase: "04-25"
    provides: [bounded raw ISO identity probe]
  - phase: "04-01"
    provides: [registered CTest and byte-sorted inventory structure]
provides:
  - Host-only bounded CUE parsing and sector-layout handling for runner identity probing
  - Stable synthetic disc identity across supported CUE sector representations
  - Registered CUE parser, path, timestamp, companion and sheet-size controls
affects: [04-26, phase-04-verification, GAME-06]
actuals:
  tokens: 14512
  tasks: 2
  commits: 3
plan_head_before: f2f471f09c889fe0b89b58d69d0682d181afecd7
plan_head_after: 3633e27267fbad55d6e099cd96c79b694fe233e0
tech-stack:
  added: []
  patterns:
    - Bounded CUE parsing in the host-only probe, outside the core determinism boundary
    - Logical ISO reads mapped to 2048-byte user sectors in supported CUE tracks
key-files:
  created: []
  modified:
    - host/disc_probe.c
    - host/disc_probe.h
    - runner/probe.c
    - tests/compat/probe_tests.c
    - tests/compat/tests.cmake
    - tests/compat/inventory.txt
    - docs/runner.md
key-decisions:
  - "Keep CUE parsing in the runner's host-only probe; it does not execute a disc or add CD-ROM behavior."
  - "Cap each CUE sheet at 64 KiB, tracks and total indexes at 99, relative FILE names at 255 bytes, and companion image bytes together at 1 GiB."
  - "Preserve boot_status and play_status as unavailable because the probe only reports identity."
patterns-established:
  - "Equivalent 2048, 2352 and 2336-byte CUE data layouts use the same ISO identity reader."
requirements-completed: []
coverage:
  - id: cue-identity
    description: "Supported CUE/BIN sector layouts and track 99 preserve the established synthetic boot identity."
    requirement: GAME-06
    verification:
      - kind: integration
        ref: "compat.probe.cue_modes"
        status: pass
    human_judgment: false
  - id: cue-boundaries
    description: "CUE timestamp, mode, track, index, path, companion, name-length and sheet-size boundaries produce the expected identity or refusal through the runner."
    requirement: GAME-06
    verification:
      - kind: integration
        ref: "compat.probe.cue_edges"
        status: pass
    human_judgment: false
  - id: raw-iso-regression
    description: "The shared reader preserves prior raw ISO identity and malformed-input behavior."
    requirement: GAME-06
    verification:
      - kind: integration
        ref: "compat.probe.synthetic_identity and compat.probe.iso_edges"
        status: pass
    human_judgment: false
duration: 38min
completed: 2026-11-12
status: complete
---

# Phase 4 Plan 49: Bounded CUE Track Identity Probe Summary

**The host-only runner probe now resolves supported CUE/BIN sector layouts to the existing canonical identity and refuses malformed, oversized or escaping sheet input.**

## Performance

- **Duration:** 38 minutes
- **Started:** 2026-11-12T05:46:30Z
- **Completed:** 2026-11-12T06:24:08.881Z
- **Tasks:** 2
- **Files modified:** 7
- **Runtime/model:** Codex OpenGSD 1.20.0, gpt-6-luna xhigh

## Accomplishments

- Added bounded CUE sheet parsing for up to 99 tracks and indexes, relative `FILE` names up to 255 bytes, and supported 2048, 2352, 2336 and AUDIO layouts.
- Preserved the exact synthetic `SWAY-00001` / `SWAY_000.01` identity and MD5 across equivalent data layouts, including data track 99.
- Added actual-runner edge coverage for maximum and invalid MSF, unsupported modes, track and index errors, path traversal, missing companions, 255/256-byte names, and the 64 KiB sheet boundary.
- Kept probe output identity-only: disc boot and play status remain `unavailable`; no disc sectors enter the emulated console.

## Task Commits

1. **Task 1 RED: registered CUE identity test** — `8b9a87d` (`test`)
2. **Task 1 GREEN: bounded CUE track identity probe** — `5cae148` (`feat`)
3. **Task 2: CUE boundary tests and documentation** — `3633e27` (`feat`)

Plan summary and state/roadmap bookkeeping are recorded in a separate documentation commit.

## Files Created/Modified

- `host/disc_probe.c`, `host/disc_probe.h` — bounded CUE parser and physical-sector to ISO user-data mapping.
- `runner/probe.c` — routes CUE and raw ISO inputs through the host-only identity reader.
- `tests/compat/probe_tests.c`, `tests/compat/tests.cmake`, `tests/compat/inventory.txt` — generated fixtures and registered identity/refusal controls.
- `docs/runner.md` — supported CUE layouts, limits and identity-only behavior.

## Decisions Made

- CUE parsing remains a host-side `--probe` feature. The core does not read files, receive disc sectors or execute disc content.
- The 1 GiB cap applies to companion image files; the CUE sheet has its independent 64 KiB cap.
- CUE input does not claim boot or play status; both remain unavailable.

## Evidence

- Task 1 RED: registered `compat.probe.cue_modes` failed because the runner still rejected the generated CUE input; the captured CTest/JUnit evidence passed the GSD RED evidence validator before the test commit.
- Task 1 GREEN: `cmake --preset ci && cmake --build --preset ci --target schwaystation-run schwaystation-test-probe && ctest --test-dir build/ci -R '^compat\.probe\.cue_modes$' --no-tests=error --output-on-failure` — exit 0; the real runner reported the exact canonical identity.
- Raw ISO regressions: `ctest --test-dir build/ci -R '^compat\.probe\.(synthetic_identity|iso_edges)$' --no-tests=error --output-on-failure` — exit 0; both prior registered cases passed.
- Task 2: `ctest --test-dir build/ci -R '^compat\.probe\.cue_edges$' --no-tests=error --output-on-failure` — exit 0; all CUE edge scenarios passed through `schwaystation-run --probe`.
- Final `cmake --workflow --preset ci` — exit 0; all 492 CI tests and all 5 release checks passed. Run URL: not applicable to this local workflow.
- Final `cmake --workflow --preset hygiene` — exit 0; all 23 hygiene checks passed, including the approved 2,300-line repository-check budget. Run URL: not applicable to this local workflow.
- `LC_ALL=C sort -c tests/compat/inventory.txt` and `git diff --check` — exit 0.

## Deviations from Plan

### Auto-fixed Issues

**1. Rule 3 — Added standard declarations required by the CUE fixture generator**
- **Found during:** Task 1 RED build
- **Issue:** The generated fixture helper used allocation routines without including their standard declarations.
- **Fix:** Included `<stdlib.h>`, then rebuilt and produced a genuine registered RED result.
- **Files modified:** `tests/compat/probe_tests.c`
- **Verification:** The RED test compiled, failed for the intended unsupported CUE behavior, and passed machine RED-evidence validation.
- **Committed in:** `8b9a87d`

**2. Timing — Plan duration exceeded its 30-minute split checkpoint**
- **Found during:** Final plan closeout
- **Issue:** Dispatch-to-closeout took 35 minutes against the 22–28 minute forecast; the split checkpoint passed before the plan was complete.
- **Fix:** Completed the authorized scope and recorded the actual duration for sizing follow-up.
- **Files modified:** None beyond the planned files.
- **Verification:** Both required closeout workflows passed; no acceptance item was omitted.

**Total deviations:** 1 implementation auto-fix and 1 timing deviation. The implementation correction was necessary to compile the fixture generator; the timing overrun is recorded for future sizing.

## User Setup Required

None.

## Self-Check: PASSED

- All seven files listed under `key-files.modified` exist.
- Task commits `8b9a87d`, `5cae148` and `3633e27` are ancestors of the recorded plan head.
- The working tree was clean after the task commits and before plan metadata updates.

