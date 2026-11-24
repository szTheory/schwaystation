---
phase: 14-multitap-and-special-controllers
plan: "03"
subsystem: sio
tags:
  - multitap
  - controllers
  - save-state
dependency-graph:
  requires:
    - phase: 14-02
      provides: API v9 input snapshots and eight controller endpoints
  provides:
    - Per-port Method 1 and Method 2 controller routing
    - Serialized Method 1 response history and in-flight SIO routing
  affects:
    - SIO
    - state serialization
actuals:
  tokens: 8533
  tasks: 2
  commits: 3
commits: 3
plan_head_before: f04135de355a4982667309a55e978d439591c9de
plan_head_after: 8b876d085a92b2e03639bdf7f0b3038bd883ea4e
tech-stack:
  added: []
  patterns:
    - Scheduled SIO dispatch resolves each tap socket to an existing pad byte FSM
    - Each physical port saves its own request and previous-response history
key-files:
  created: []
  modified:
    - src/sio.h
    - src/sio.c
    - tests/sio/sio_tests.c
    - docs/hardware/sio.md
key-decisions:
  - Preserve PER.01 Method 1 semantics: REQ in the third transmitted byte changes the next access only.
  - Keep Method 1 response history independent for each physical port and retain full Method 2 endpoint lengths.
  - Use the existing pad FSM for socket data and pad payload side effects.
patterns-established:
  - Tap routing remains in the SIO owner and resolves socket indices through the existing pad endpoint map.
  - Tap history is part of saved protocol state; controller data and persistent card bytes remain outside SIO state.
requirements-completed: []
coverage:
  - id: tap-methods
    description: Both ports route all four controllers using Method 1 long records and Method 2 individual polls, including a complete analog configuration response.
    requirement: SIO-01
    verification:
      - kind: unit
        ref: "ctest --preset ci --no-tests=error -R '^sio[.]tap[.]poll_methods$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: tap-history
    description: REQ history, the four-byte short response, missing-slot padding, empty Slot A, and reset/deselect/port-switch aborts preserve bounded transaction behavior.
    requirement: SIO-01
    verification:
      - kind: unit
        ref: "ctest --preset ci --no-tests=error -R '^sio[.]tap[.]history_edges$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: tap-timing-state
    description: Pending tap bytes and ACKs restore at the same deadlines; the final byte raises no later ACK or IRQ.
    requirement: SIO-01
    verification:
      - kind: unit
        ref: "ctest --preset ci --no-tests=error -R '^sio[.](tap[.](history_edges|poll_methods)|digital[.]poll_irq|state[.]pending_(byte|ack)|transport[.]abort)$' --output-on-failure"
        status: pass
    human_judgment: false
metrics:
  duration: 31m31s
  completed: 2026-11-24
status: complete
---

# Phase 14 Plan 03: Multitap Controller Access Summary

Two independently configured taps now route all four controller sockets through both documented SIO methods, preserve PER.01 request history, and resume saved transactions at their scheduled deadlines.

## Performance

- **Runtime/model:** Codex GSD executor; configured `gpt-6-luna` xhigh; primary runtime model ID unavailable.
- **Duration:** 31m31s, measured from 2026-11-24T04:01:30Z through 2026-11-24T04:33:01Z.
- **Tasks:** 2 complete.
- **Files modified:** 4.

## Accomplishments

- Added Method 2 socket addressing and Method 1 request/response history independently on both physical ports. Long replies contain four fixed eight-byte records, and repeated REQ=1 alternates long replies with the documented four-byte short reply.
- Kept pad IDs, button bytes, analog payloads, and device-specific poll lengths on the existing pad FSMs. Empty Slot A aborts on the first byte; missing later sockets return eight `FF` bytes.
- Serialized each port's request history and in-flight route. Reset, deselect, and port switching cancel tap routing and pending transport events.
- Documented the pinned PER.01 headings and added protocol, empty-slot, abort, saved-state, and deadline vectors.

## Task Commits

1. **Task 1 RED:** `f9b8d11` — add failing tap poll and request-history assertions.
2. **Task 1 GREEN:** `7f0b42c` — route both controller access methods and add per-port SIO history state.
3. **Task 2:** `8b876d0` — cover empty slots, aborts, deadlines and saved history; document the protocol.

The final SUMMARY/STATE/ROADMAP metadata commit is recorded separately after bookkeeping.

## Check Results

- Cheap preflight passed: `git diff --check`, the byte-sorted SIO inventory check, and exact selector assertions found all required tests before the targeted build.
- Task 1 RED: `ctest --preset ci --no-tests=error -R '^sio[.]tap[.]poll_methods$' --output-on-failure --output-junit .gsd/tdd-evidence/14-03/task1-red.xml` exited 8 on the missing Method 2 routes and missing next-transaction long reply. `gsd_run check tdd-red-evidence .gsd/tdd-evidence/14-03/task1-red.json --raw` returned `RED_EVIDENCE_OK`; the report and semantic assessment are retained in ignored local evidence.
- Task 1 GREEN and tracer recheck: configure, selector check, focused build and `sio.tap.poll_methods` all exited 0. The post-commit tracer run found the selector and reported the target passed.
- Final focused run: `ctest --preset ci --no-tests=error -R '^sio[.](tap[.](history_edges|poll_methods)|digital[.]poll_irq|state[.]pending_(byte|ack)|transport[.]abort)$' --output-on-failure` exited 0 with 6/6 tests passing in 1.26 seconds.
- No hosted CI, Act, or Docker run was needed for this plan: workflow files were unchanged, and the phase's integrated hosted gates are reserved for the reviewed final phase head.

## Requirement Status

SIO-01 remains pending at phase level because it also covers tap memory-card routing and persistence through the host save lifecycle. This plan's controller-routing slice and its automated evidence are covered above; it does not mark the broader phase requirement complete.

## Deviations from Plan

The Method 1 history, empty-slot handling, abort cleanup, and serialization share the same per-port SIO routing state. Those implementation pieces landed with Task 1's integrated router; Task 2 added the edge vectors and hardware documentation against that complete state machine. Task 2 therefore had no separate production GREEN commit. Its first expanded test run exposed a test setup assumption that the router becomes active when JOY_DATA is written; routing correctly begins when the scheduled byte completes. The assertion was corrected and the final target passed. No valid RED evidence was claimed for that test-only mistake.

## Issues Encountered

- The initial Task 2 timing assertion expected route activation before the 1,088-tick byte event. It was corrected to inspect the pending-byte state before the deadline and the active route after completion.
- No TODO, FIXME, placeholder, skipped test, unrun verification, or new security surface remains in the changed files.

## Next Phase Readiness

Plan 14-04 can build on the per-port SIO routing state. The full Phase 14 review, integrated verification, and hosted release gates remain for their planned final orchestration step.

## Self-Check: PASSED

- Summary file exists at the required phase path.
- The three plan commits are ancestors of `plan_head_after` and match the measured ledger count.
- Final focused local SIO selectors passed 6/6; no tracked or untracked task outputs remain.

---
*Phase: 14-multitap-and-special-controllers*
*Completed: 2026-11-24*
