---
phase: 15-guncon-and-justifier
plan: "03"
subsystem: input-and-gpu
tags:
  - guncon
  - justifier
  - frame-latch
  - gpu-beam
requires:
  - phase: 15-02
    provides:
      - Validated API v10 input records with explicit gun validity
provides:
  - Two independent direct-port gun records latched at frame start
  - Pad-owned per-port protocol enable and latest observation result slots
  - Fixed GPU-owned sensor event state with inactive-deadline initialization
affects:
  - 15-04 GunCon protocol
  - 15-05 GPU beam sampling
  - 15-06 Justifier IRQ10
  - 15-09 serialized continuation
actuals:
  tokens: 3707
  tasks: 2
  commits: 4
commits: 4
plan_head_before: e869a7a31b54b42c61233f3cc83365dfc739cb16
plan_head_after: c0af4f13a99f889834384e7e32f9f3f03d6fa865
tech-stack:
  added: []
  patterns:
    - The validated host gun snapshot is copied as one per-port record only at frame start.
    - Pad-owned invalidation clears the associated GPU event and restores UINT64_MAX as its inactive deadline.
key-files:
  created: []
  modified:
    - src/pad.h
    - src/pad.c
    - src/gpu.h
    - src/gpu.c
    - tests/sio/pad_tests.c
key-decisions:
  - Keep pending host input outside serialized state; Plan 09 will serialize only latched hardware-visible fields.
  - Store beam scheduling in the GPU instance and protocol/result state in the pad instance.
patterns-established:
  - Frame-start snapshots keep aim, validity, trigger and device topology coherent.
  - Per-port sensor records use fixed-width fields and an explicit inactive deadline.
requirements-completed:
  - GUN-01
  - GUN-02
  - SIO-05
coverage:
  - id: D1
    description: Both direct-port gun records latch atomically at frame start, including valid center aim, trigger state and independent port and instance values.
    requirement: SIO-05
    verification:
      - kind: unit
        ref: ctest --test-dir build/ci --no-tests=error -R '^sio[.]gun[.]latch$' --output-on-failure
        status: pass
    human_judgment: false
  - id: D2
    description: Device replacement, missing gun validity, v9-sized input and reset clear stale pad results and GPU sensor events without clearing another direct port.
    requirement: GUN-02
    verification:
      - kind: unit
        ref: ctest --test-dir build/ci --no-tests=error -R '^sio[.]gun[.]latch$' --output-on-failure
        status: pass
    human_judgment: false
  - id: D3
    description: Each GPU instance reserves bounded fixed-width sensor timing for two ports and initializes event deadlines to the inactive sentinel.
    requirement: GUN-01
    verification:
      - kind: unit
        ref: ctest --test-dir build/ci --no-tests=error -R '^sio[.]gun[.]latch$' --output-on-failure
        status: pass
    human_judgment: false
duration: 13m 25s
completed: 2026-11-24
status: complete
---

# Phase 15 Plan 03: Frame-latched gun input and bounded beam state

**GunCon and Justifier snapshots now latch atomically at frame start, with bounded per-port GPU sensor slots and stale-shot invalidation.**

## Performance

- **Duration:** 13m 25s
- **Started:** 2026-11-25T04:28:06Z
- **Completed:** 2026-11-25T04:41:31Z
- **Runtime/model:** Codex/OpenGSD 1.23.0; executor gpt-6-luna xhigh; primary model ID unavailable
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments

- Added per-port latched API v10 gun records, result coordinates/validity and Justifier enable state; a complete record changes only at frame start.
- Verified valid `(0, 0)` aim, same-frame replacement deferral, v9 input compatibility, per-port/instance independence and cleanup on device changes, missing aim and reset.
- Defined two fixed-width GPU sensor records carrying armed/consumed state, aim, target beam position, deadline and conversion phase. Reset and snapshot invalidation set deadlines to `UINT64_MAX`.

## TDD Evidence

- Task 1 RED: `./build/ci/tests/schwaystation-test-sio-pad gun_latch` exited 1 on the missing per-port pad storage assertion. `gsd-tools check tdd-red-evidence .gsd/tdd-evidence/15-03/red.json --raw` accepted it as `RED_EVIDENCE_OK`. The GREEN build and focused API/pad selector set passed.
- Task 2 RED: the same binary exited 1 with 21/22 TAP assertions passing; the sole failure was the missing GPU sensor storage assertion. `gsd-tools check tdd-red-evidence .gsd/tdd-evidence/15-03/gpu-red.json --raw` accepted it as `RED_EVIDENCE_OK`.
- Final focused verification: `cmake --preset ci` exited 0; `cmake --build --preset ci --target schwaystation-test-sio-pad` exited 0; `ctest --test-dir build/ci --no-tests=error -R '^sio[.]gun[.]latch$' --output-on-failure` passed 1/1. The earlier targeted set containing `sio.gun.latch`, `sio.pad.input_latch`, `sio.pad.independent_ports_instances` and `abi.api_v10` passed 4/4. `policy.fixture_budget` passed 1/1 and the SIO inventory and frozen API offsets passed their cheap preflights.
- No full workflow, hosted CI, `act` run or Docker resource was needed for this plan; full local and hosted gates remain part of phase closeout.

## Task Commits

1. **Task 1 RED probe** — `3e9af4a` (`test(15-03): add failing gun latch state probe`)
2. **Task 1 GREEN** — `fd627ab` (`feat(15-03): latch direct-port gun input`)
3. **Task 2 RED probe** — `f275820` (`test(15-03): add failing GPU sensor storage probe`)
4. **Task 2 GREEN** — `c0af4f1` (`feat(15-03): reserve bounded gun sensor state`)

## Files Modified

- `src/pad.h` and `src/pad.c` — per-port latched input/result and enable state; frame-start, reset and device-change invalidation.
- `src/gpu.h` and `src/gpu.c` — fixed GPU sensor-event records and inactive deadline initialization.
- `tests/sio/pad_tests.c` — gating TDD probe for snapshot boundaries, compatibility, port isolation and stale-state clearing.

## Decisions Made

- Kept host-pending input outside state serialization; Plan 09 adds only the hardware-visible latched continuation fields.
- Kept beam scheduling state in the GPU and protocol/result state in the pad module; the pad invalidation path clears both owners through the current instance.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Renamed a shadowing frame-start local**
- **Found during:** Task 1 focused build
- **Issue:** Strict `-Wshadow -Werror` rejected reuse of `device` for the direct-port selection and socket loop.
- **Fix:** Renamed the outer local to `input_device`.
- **Files modified:** `src/pad.c`
- **Verification:** The targeted build and focused selector set passed after the fix.
- **Committed in:** `fd627ab`

**Total deviations:** 1 auto-fixed (Rule 3 - Blocking)
**Impact on plan:** This was a local compile fix with no behavior or scope change.

## Issues Encountered

The initial targeted build stopped on the shadowing diagnostic; the first corrective edit resolved it, and the rerun completed successfully.

## User Setup Required

None - no external configuration is required.

## Next Phase Readiness

Plans 15-04 and 15-05 can consume the shared latched records and GPU event contract. The phase still needs serial protocol, real beam sampling, IRQ ordering, host mapping, serialization, code review and the final local/hosted gates. Synthetic results do not establish retail-game compatibility or physical threshold calibration.

---
*Phase: 15-guncon-and-justifier*
*Completed: 2026-11-24*

## Self-Check: PASSED

The two RED probes passed the GSD evidence classifier, both implementation commits are ancestors of the plan head, the final focused test passed, and the task-commit count matches the persisted plan ledger.
