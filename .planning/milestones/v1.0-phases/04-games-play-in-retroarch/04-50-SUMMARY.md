---
phase: 04-games-play-in-retroarch
plan: "50"
subsystem: bus
tags: [EXP2, OpenBIOS, C, CMake, TDD]

requires:
  - phase: 04-23
    provides: Explicit BIOS loading through the runner reset path.
  - phase: 04-39
    provides: Pinned OpenBIOS build and same-run artifact handoff.
provides:
  - Exact EXP2 POST byte-write compatibility with existing port timing and gap faults.
  - Registered regression coverage and source-grounded bus documentation.
affects: [04-40, bus, GAME-05]

actuals:
  tokens: 2632
  tasks: 2
  commits: 2
commits: 2
plan_head_before: bd5a350879b4e4fbc0fac2cdcd854164c91ff1a5
plan_head_after: f28443b0e1756c3e94866b5dad411812be86986a

tech-stack:
  added: []
  patterns:
    - Exact source-identified bus exception routed through existing EXP2 timing.

key-files:
  created: []
  modified:
    - src/bus.c
    - tests/bus/bus_tests.c
    - tests/bus/tests.cmake
    - tests/bus/inventory.txt
    - docs/hardware/bus.md

key-decisions:
  - Accept only width-one writes to physical 1F802041h as ignored POST status, preserving reads, wider accesses, neighboring faults, and debug-port behavior.

patterns-established: []
requirements-completed: []
coverage:
  - id: D1
    description: The source-identified POST byte write succeeds with normal EXP2 cost and no stored status or debug side effects; refusal controls retain EXP2 faults.
    requirement: GAME-05
    verification:
      - kind: unit
        ref: ctest bus.exp2_post and bus.exp_regions; focused RED/GREEN commands recorded below
        status: pass
      - kind: integration
        ref: cmake --workflow --preset ci; 472 tests and five release checks passed
        status: pass
      - kind: integration
        ref: cmake --workflow --preset hygiene; 23 checks passed
        status: pass
    human_judgment: false

duration: about 17 minutes
completed: 2026-11-11
status: complete
---

# Phase 04 Plan 50: POST Byte Compatibility Summary

**The bus accepts OpenBIOS' exact POST byte write through normal EXP2 timing while retaining the surrounding fault boundaries.**

## Performance

- **Duration:** about 17 minutes from the pre-execution plan-check head through the final verification run.
- **Started:** approximately 2026-11-11T20:31:32Z, at the clean execution base.
- **Completed:** 2026-11-11T20:48:46Z, after CI and hygiene completed.
- **Tasks:** 2.
- **Files modified:** 5.
- **Runtime/model:** Codex / OpenGSD 1.20.0, gpt-6-luna xhigh, sequential shared phase checkout.

## Accomplishments

- Accepted only width-one writes to physical `1F802041h`; they use the existing EXP2 byte-write cost and leave fault, unbuilt, TTY, exit, and saved-status state unchanged.
- Added a registered bus regression for `0Fh`, `0Eh`, byte endpoints, physical/KSEG aliases, both debug-port flag settings, reads, widths 2/4, aligned accesses covering POST, and adjacent gaps. Existing debug-port controls remain in `bus.exp_regions`.
- Documented the narrow model with pinned MIT OpenBIOS source links. This is source evidence for the firmware write, not a hardware measurement or shell-boot acceptance.

## TDD Cycle

- **RED:** `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-bus && ctest --test-dir build/ci -R '^bus\.(exp2_post|exp_regions)$' --no-tests=error --output-on-failure --output-junit build/ci/04-50-red.xml` exited 8 because `bus.exp2_post` observed the missing success and EXP2 charge. `bus.exp_regions` passed. The native `check tdd-red-evidence` returned `RED_EVIDENCE_OK`; semantic inspection confirmed the named test failed on the planned assertion.
- **GREEN:** The same configure/build sequence followed by the focused CTest command without JUnit output exited 0; both registered cases passed.
- **REFACTOR:** None; the exact condition uses the existing dispatch and charge path.

## Evidence

- Focused GREEN: `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-bus && ctest --test-dir build/ci -R '^bus\.(exp2_post|exp_regions)$' --no-tests=error --output-on-failure` — exit 0.
- Documentation check: `git diff --check HEAD` — exit 0.
- Full closeout: `cmake --workflow --preset ci` — exit 0; all 472 tests and five release checks passed.
- Hygiene closeout: `cmake --workflow --preset hygiene` — exit 0; all 23 checks passed. The whole-tree scans completed in 170.44 seconds under the existing four-times timeout scale.

## Task Commits

1. **Task 1 RED:** `8b0d614` — add the failing OpenBIOS POST bus regression.
2. **Task 1 GREEN:** `f28443b` — model the ignored POST byte and document its source basis.

Task 2 confirmed the narrow implementation and documentation contract; it required no additional file change. The two task commits are measured from the recorded plan head.

## Files Created/Modified

- Modified: `src/bus.c`, `tests/bus/bus_tests.c`, `tests/bus/tests.cmake`, `tests/bus/inventory.txt`, and `docs/hardware/bus.md`.
- Created: `.planning/phases/04-games-play-in-retroarch/04-50-SUMMARY.md`.

## Decisions Made

- The pinned OpenBIOS source identifies the byte write; the core models it as ignored diagnostic status, with no readable or saved status value.
- `GAME-05` remains pending as a whole until its shell and portable-frame acceptance is completed.

## Deviations from Plan

None — the planned address-specific compatibility slice and acceptance boundaries were followed.

## Issues Encountered

- CTest resolved the relative JUnit output path under `build/ci`; the unmodified report was located there and accepted by the native RED classifier. No source changes were needed.

## User Setup Required

None.

## Next Phase Readiness

Plan 04-40 can resume to reinspect the actual OpenBIOS boot, diagnose any next concrete fault, and retain its independent shell-frame and same-run portable-hash acceptance. This plan does not claim shell success or complete `GAME-05`; no owner action is needed.

## Self-Check: PASSED
