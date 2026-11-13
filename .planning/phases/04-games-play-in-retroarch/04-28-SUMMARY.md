---
phase: 04-games-play-in-retroarch
plan: "28"
subsystem: libretro
tags: [controllers, dualshock, rumble, host-tests]
requires:
  - phase: 04
    provides: libretro host callbacks, guest test harness, runner reference execution
provides:
  - Per-port libretro controller selection and analog input configuration
  - Frame-latched input mapping with exact signed-axis conversion
  - Guest-driven DualShock rumble reports to both motors on both ports
  - Registered control, movie, guest-rumble tests and embedding documentation
affects: [libretro, controller-input, rumble, phase-04]
actuals:
  tokens: 20240.25
  tasks: 2
  commits: 4
tech-stack:
  added: []
  patterns:
    - One input snapshot per frame, followed by one guest frame execution
    - Independent family-specific state references for runner-equivalent guest movies
    - Real guest SIO motor commands drive captured libretro rumble reports
key-files:
  created: []
  modified:
    - libretro/libretro.c
    - tests/libretro/host.c
    - tests/libretro/tests.cmake
    - tests/libretro/inventory.txt
    - docs/embedding.md
key-decisions:
  - "Keep controller family selection distinct from DualShock digital/analog wire protocol mode."
  - "Use DualShock selection with digital wire mode for pad_counter movies because the pinned guest transaction consumes two payload bytes; prove the complete analog packet and axis endpoints with a separate real guest."
  - "Report strong and weak motor values for both ports once per successful frame; clear active outputs at reset and unload."
patterns-established:
  - "Controller movie comparisons retain separate serialized-state references for each controller family."
  - "Guest SIO transactions and independent core references verify host-visible rumble rather than interface registration alone."
requirements-completed: []
coverage:
  - id: D1
    description: Per-port device selection, defaults, absent input callback, analog option and frame-60 toggle behavior.
    requirement: GAME-03
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^libretro\\.input\\.controls$' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: Empty, press-at-60, three-edge, and held-input 300-frame pad_counter schedules for JOYPAD digital and DualShock selected with digital wire mode; compare complete guest frames, audio, counts, colors, and independent per-family state references.
    requirement: GAME-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^libretro\\.input\\.movies$' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
  - id: D3
    description: Real guest full DualShock analog transactions verify axis endpoints and both ports' strong/weak motor reports with exact scaling.
    requirement: GAME-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^libretro\\.rumble\\.guest$' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
  - id: D4
    description: Rumble maximum and zero values, repeated-frame reports, reset/unload clearing, and safe operation when the host lacks rumble support.
    requirement: GAME-03
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^libretro\\.rumble\\.controls$' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
duration: 92 min
completed: 2026-11-12
status: complete
commits: 4
plan_head_before: 11d4bf3cfacc33ab4fa2c5ec6f5842067dadd000
plan_head_after: 0c2f7644c439b1079d7b4ada361a4bab3d11d193
---

# Phase 4 Plan 28: Libretro Controllers and Rumble Summary

**Per-port controller mapping, frame-latched guest input and guest-driven rumble are covered by registered libretro tests.**

## Performance

- **Duration:** 92 min
- **Started:** 2026-11-12T13:42:57Z
- **Completed:** 2026-11-12 15:14:45 UTC
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments

- Added DualShock defaults, per-port NONE/JOYPAD/ANALOG selection, analog mode and configured toggle behavior. Input is sampled once per frame; signed axes -32768, 0, and 32767 map to 0, 128, and 255.
- Ran all four 300-frame pad_counter schedules independently under JOYPAD digital and DualShock selected with digital wire mode. Full video/audio and guest mailbox outcomes were compared to runner references; serialized-state expectations remain family-specific.
- Exercised a real guest DualShock transaction for the full analog response and both ports' strong/weak rumble commands. Added safe absent-interface behavior, exact motor scaling, repeated reports and lifecycle clearing.
- Registered the input/rumble control, movie and guest tests, sorted the libretro inventory, and documented the controller and rumble behavior in the embedding guide.

## Verification Evidence

- Valid TDD RED was recorded for libretro.rumble.guest before implementation; gsd_run check tdd-red-evidence reported RED_EVIDENCE_OK.
- Final focused cases passed individually: libretro.input.controls, libretro.input.movies, libretro.rumble.controls, and libretro.rumble.guest, each with --no-tests=error.
- Final cmake --workflow --preset ci exited 0: 501/501 CI checks passed and the release package preset passed 5/5. Final cmake --workflow --preset hygiene exited 0: 23/23 checks passed.
- git diff --check, byte-sorted tests/libretro/inventory.txt, and the pinned-root check passed. Workflow evidence is local; no hosted run URL was produced by this plan.

## Task Commits

1. **Task 1 RED:** 5d1dd82 — real guest rumble regression.
2. **Task 1 GREEN:** 32f58fe — controller layouts and rumble mapping.
3. **Task 2:** 2f5ba46 — controller and rumble controls.
4. **Task 2 regression alignment:** 0c2f764 — update legacy host references for the DualShock defaults.

## Files Modified

- libretro/libretro.c — controller selection, frame input mapping, option handling and rumble reporting/lifecycle.
- tests/libretro/host.c — actual guest movies, analog and rumble transaction checks, and host control cases.
- tests/libretro/tests.cmake — registered input and rumble cases.
- tests/libretro/inventory.txt — byte-sorted test inventory.
- docs/embedding.md — libretro controller configuration and rumble behavior.

## Decisions Made

- Controller family selection is separate from a DualShock device's digital or analog wire protocol. The pad_counter fixture's transaction reads only two payload bytes, so its DualShock movie uses digital wire mode; a distinct real guest test validates the full analog packet and axis endpoints. No guest SIO behavior or fixture was changed to manufacture coverage.
- Each family has an independent serialized-state reference even where complete frame, audio and mailbox outputs match.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Corrected guest-test expectations for the changed controller defaults**
- **Found during:** Task 2 full CI verification.
- **Issue:** Existing host, PAL host, and content lifecycle references assumed the previous digital default and did not select JOYPAD explicitly.
- **Fix:** Updated those tests to select JOYPAD where the old digital behavior was intended and aligned fallback expectations with the new DualShock default.
- **Files modified:** tests/libretro/host.c.
- **Verification:** The three affected cases passed individually, followed by final full CI and hygiene.
- **Committed in:** 0c2f764.

**2. [Rule 1 - Bug] Fixed the generated guest transaction fixture**
- **Found during:** Task 1 RED/GREEN.
- **Issue:** The initial guest program used an incorrect jump target and loaded from an unmapped response address, preventing the intended guest evidence from being meaningful.
- **Fix:** Corrected the generated program and its response-buffer base so the guest performed and checked the real SIO transaction.
- **Files modified:** tests/libretro/host.c.
- **Verification:** The corrected guest passed against the independent core reference and then against the libretro rumble path.
- **Committed in:** 5d1dd82 and 32f58fe.

**Total deviations:** 2 auto-fixed bugs. The acceptance scope and pinned guest fixture were preserved.

## Issues Encountered

The pad_counter guest does not consume a full analog payload, so full analog wire behavior is demonstrated by the separate real guest transaction test. All requested controller-family movie cases and the analog proof passed.

## User Setup Required

None.

## Next Phase Readiness

Plan 04-28 is complete. The broader GAME-03 requirement remains open for its other planned callers and behaviors; this summary does not mark the whole requirement complete.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-12*

## Self-Check: PASSED

- The summary file exists and records all five changed source, test and documentation files.
- All four task commits are ancestors of the recorded plan head.
- The measured plan commit count is four, from 11d4bf3cfacc33ab4fa2c5ec6f5842067dadd000 through 0c2f7644c439b1079d7b4ada361a4bab3d11d193.
