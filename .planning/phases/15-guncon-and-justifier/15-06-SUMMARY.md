---
phase: 15-guncon-and-justifier
plan: "06"
subsystem: sio-gpu
tags: [justifier, irq10, gpu-beam, timers, sio]
requires:
  - phase: 15-04
    provides: [Justifier SIO protocol and sensor-enable state]
  - phase: 15-05
    provides: [bounded beam sample events and live VRAM sampling]
provides:
  - Direct-port Justifier ID, button, enable, abort, reset, and device-change behavior
  - IRQ10 on one enabled bright Justifier beam event after Timer0/Timer1 synchronize
  - Deterministic no-hit, same-tick, rearm, split-budget, port, and instance controls
affects: [15-07-PLAN.md, 15-09-PLAN.md]
actuals:
  tokens: 4240
  tasks: 2
  commits: 4
commits: 4
plan_head_before: f3bc589
plan_head_after: 8b70d91
tech-stack:
  added: []
  patterns: [transaction-bound peripheral enable, beam-timed hardware IRQ]
key-files:
  created: []
  modified: [src/pad.c, tests/sio/pad_tests.c, docs/hardware/sio.md, src/gpu.c, tests/gpu/beam.c]
key-decisions:
  - "Raise IRQ10 only after an enabled Justifier produces a bright beam sample; preserve the existing timer-to-GPU scheduler order."
  - "Reuse the existing serialized small-byte staging lane for enable continuation; no state-format revision or timers.c change was needed."
patterns-established:
  - "Exercise peripheral enable and IRQ timing through existing SIO, GPU, timer, and bus paths."
  - "Use one focused IRQ selector for tied-event, no-hit, rearm, port, and instance controls."
requirements-completed: [GUN-01]
coverage:
  - id: D1
    description: "A direct-port Justifier reports ID 31h and active-low buttons; its sensing enable commits only on a complete command and clears on abort, reset, or device change."
    requirement: GUN-01
    verification:
      - kind: unit
        ref: "tests/sio/pad_tests.c#sio.gun.justifier"
        status: pass
    human_judgment: false
  - id: D2
    description: "Only an enabled bright Justifier sample raises IRQ10 at the beam event after Timer0/Timer1 catch up; no-hit and tied-event behavior is deterministic across budgets, ports, and instances."
    requirement: GUN-01
    verification:
      - kind: unit
        ref: "tests/gpu/beam.c#gpu.gun.irq_order"
        status: pass
      - kind: unit
        ref: "tests/gpu/beam.c#gpu.gun.sample, gpu.beam.equal_timer_beam_dma"
        status: pass
      - kind: unit
        ref: "tests/sio/pad_tests.c#sio.gun.justifier, sio.gun.guncon, sio.transport.abort"
        status: pass
    human_judgment: false
duration: "about 3h 52m wall time; active time not isolated"
completed: 2026-11-25
status: complete
---

# Phase 15 Plan 06: Justifier IRQ10 Summary

**Justifier polling and beam-timed IRQ10 now use the existing SIO, GPU, and timer scheduling paths.**

## Performance

- **Duration:** About 3h 52m wall time through task implementation commits; active time was not isolated.
- **Start reference:** Plan 15-05 closeout checkpoint at 2026-11-25T08:31:06Z; Plan 15-06 start was not separately recorded.
- **Implementation committed:** 2026-11-25T12:23:02Z.
- **Tasks:** 2.
- **Files modified:** 5.
- **Runtime/model:** Codex GSD executor, gpt-6-luna xhigh; primary session model ID unavailable.

## Accomplishments

- Added direct-port Justifier ID 31h, button mapping, and complete-command enable handling through the existing pad FSM. Partial abort, reset, and device changes leave sensing disabled and clear stale shot state.
- Raised IRQ10 from the existing GPU beam callback only for an enabled Justifier after a bright sample. The existing scheduler synchronizes Timer0 and Timer1 before the callback, so `timers.c` and global event order did not need changes.
- Added gating checks for IRQ and timer values at the event, tied enable/VRAM writes, consume/clear/rearm behavior, disabled/dark/offscreen/reload/missing aim, split/whole budgets, both ports, and independent instance IRQ status.

## Task Commits

1. **Implement Justifier replies and sensor enable** — `129ed94` (RED), `77c458a` (implementation).
2. **Synchronize exact shot position before raising IRQ10** — `e9baa70` (RED), `8b70d91` (implementation).

**Plan base:** `f3bc589`.

## Files Modified

- `src/pad.c` and `tests/sio/pad_tests.c` — Justifier serial ID, controls, enable, abort/reset, and device-change behavior.
- `src/gpu.c` and `tests/gpu/beam.c` — conditional IRQ10 at the bright beam event and timing/no-hit/isolation coverage.
- `docs/hardware/sio.md` — documented the modeled IRQ10/timer observation behavior and scope.

## Verification Evidence

- Cheap preflight: `cmake --preset ci`, exit 0 (0.9s); four required selectors were present exactly once; `abi.api_v9` and `policy.fixture_budget` passed 2/2.
- Task 1 RED: `gun_justifier` produced 8 TAP assertions with the expected missing ID/button/enable behaviors; `tdd-red-evidence` accepted the target assertion. The pad target built and the Justifier, GunCon, abort, and input-latch selectors passed 4/4.
- Task 2 RED: `gpu.gun.irq_order` had exactly the expected missing-IRQ10 assertion while its Timer0/Timer1-at-event assertions passed; `tdd-red-evidence` accepted the target assertion.
- Task 2 GREEN: the focused GPU/SIO targets built successfully; `gpu.gun.irq_order`, `gpu.gun.sample`, `gpu.beam.equal_timer_beam_dma`, and `sio.gun.justifier` passed 4/4 after the final timing and isolation cases were added.
- `git diff --check` passed. No workflow changed, so hosted CI and `act` were not run. No Docker resources were created.

## Decisions and Deviations

- The beam callback already synchronizes Timer0/Timer1 before sampling. No `timers.c` change was needed, avoiding a global timer-order change.
- The enable state uses the existing serialized small-byte staging lane; this plan adds no serialized fields or state-format revision.
- Test assertions use the existing internal bus and per-instance state to observe the IRQ handler timestamp without adding a test harness or public API.
- The Task 1 RED and implementation and Task 2 RED and implementation were committed separately. No other plan scope was added.
- Plan duration substantially exceeded the low-confidence 20–28 minute estimate. The main delay was waiting for checkpoints and reconciling beam-event ordering; future work should use the already-proven Plan 15-05 scheduler interfaces and a focused first selector before expanding cases.

## Evidence Boundary

These synthetic tests establish the modeled Justifier protocol and interrupt timing. They do not establish calibrated physical-gun behavior or retail-game compatibility.

## Next Plan Readiness

The Justifier serial and IRQ10 paths are ready for Plan 15-07's libretro input mapping. Phase 15 still has Plans 15-07 through 15-10, followed by phase review, verification, and release.

## Self-Check: PASSED

- The summary is at the planned path.
- Task commits `129ed94`, `77c458a`, `e9baa70`, and `8b70d91` are ancestors of HEAD.
- Coverage classification passed 2/2 deliverables with no errors.
- The required focused GPU/SIO selector set passed 4/4; `git diff --check` passed.

---
*Phase: 15-guncon-and-justifier*
*Completed: 2026-11-25*
