---
phase: 15-guncon-and-justifier
plan: "04"
subsystem: input-and-sio
tags:
  - guncon
  - sio
  - serial-protocol
requires:
  - phase: 15-03
    provides:
      - Frame-latched gun input, per-port result slots, and fixed GPU sensor state
provides:
  - Direct-port GunCon 5A63h ID and six-byte 42h report over the existing SIO path
  - Active-low A, trigger, and B bits with independent X/Y halfwords
  - Deterministic unavailable, dark, offscreen, and reload coordinate replies
affects:
  - 15-05 GPU beam sampling
  - 15-06 Justifier IRQ10
  - 15-09 serialized continuation
actuals:
  tokens: 2727
  tasks: 2
  commits: 4
commits: 4
plan_head_before: beb778c3edce280d59dd6de0ad6f057e3405ba6e
plan_head_after: f49d7949f77042c054fc67a1741cf6b8a417295a
tech-stack:
  added: []
  patterns:
    - Gun reports reuse the existing cycle-scheduled SIO command, byte, and ACK state machine.
    - The report derives from the frame-latched input and the latest per-port beam observation.
key-files:
  created: []
  modified:
    - src/pad.c
    - tests/sio/pad_tests.c
    - docs/hardware/sio.md
key-decisions:
  - "Use PER.01's 5A63h ID, active-low button bits, X/Y byte order, and video-timing units."
  - "Choose X=0001h/Y=000Ah for unavailable or unsampled input and X=0001h/Y=0005h for explicit offscreen or reload; document these as model choices because PER.01 overlaps their interpretation."
patterns-established:
  - "GunCon polls return through the existing SIO byte scheduler, preserving per-byte ACK and final-byte no-ACK behavior."
  - "An invalid or unsampled frame-latched aim never reuses an earlier beam coordinate."
requirements-completed:
  - GUN-01
coverage:
  - id: D1
    description: Direct-port GunCon returns its documented ID and six-byte active-low button and coordinate report through SIO.
    requirement: GUN-01
    verification:
      - kind: unit
        ref: ctest --test-dir build/ci --no-tests=error -R '^sio[.]gun[.]guncon$' --output-on-failure
        status: pass
    human_judgment: false
  - id: D2
    description: Unavailable, unsampled, offscreen, reload, repeated, and aborted polls return stable coordinates without stale observations.
    requirement: GUN-01
    verification:
      - kind: unit
        ref: tests/sio/pad_tests.c#gun_guncon via sio.gun.guncon
        status: pass
    human_judgment: false
duration: 13m
completed: 2026-11-25
status: complete
---

# Phase 15 Plan 04: GunCon serial poll and deterministic light fallback

**The GunCon now returns its 5A63h report over cycle-scheduled SIO, with frame-stable buttons, beam coordinates, and explicit no-light/reload replies.**

## Performance

- **Duration:** About 13 minutes from the first recorded execution checkpoint; exact executor start was not captured.
- **Started:** 2026-11-25T04:46:57Z (first recorded execution checkpoint)
- **Completed:** 2026-11-25T04:59:50Z
- **Runtime/model:** Codex/OpenGSD 1.23.0; executor gpt-6-luna xhigh; primary model ID unavailable
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments

- Added the direct-port GunCon `63 5A` poll header and six-byte payload to the existing SIO device state machine. Button bits are active low; X and Y are little-endian halfwords from the latest per-port observation.
- Selected and documented stable `0001h/000Ah` unavailable/no-sample and `0001h/0005h` offscreen/reload replies. Frame-latch invalidation, repeats, and partial-poll aborts cannot reuse an old light observation.
- Preserved all per-byte ACK behavior and the final-byte no-ACK edge without changing the SIO transport.

## Verification Evidence

- Cheap preflight: `git diff --check` passed; `cmake --preset ci` exited 0; all four Task 1 selectors were present once; `policy.fixture_budget` passed 1/1.
- Task 1 RED: `./build/ci/tests/schwaystation-test-sio-pad gun_guncon` exited 1 with the expected missing GunCon ID/payload assertions. `gsd-tools check tdd-red-evidence .gsd/tdd-evidence/15-04/red.json --raw` returned `RED_EVIDENCE_OK` for `port 1 GunCon ID`.
- Task 1 GREEN: `cmake --build --preset ci --target schwaystation-test-sio-pad schwaystation-test-sio` exited 0. `sio.gun.guncon`, `sio.digital.poll_irq`, `sio.pad.final_no_ack`, and `sio.transport.abort` passed 4/4.
- Task 2 RED: the same TAP test exited 1 with the intended unavailable, dark, and reload coordinate mismatches. `gsd-tools check tdd-red-evidence .gsd/tdd-evidence/15-04/red2.json --raw` returned `RED_EVIDENCE_OK` for `unavailable sensor uses selected deterministic no-light response`.
- Task 2 GREEN: `cmake --build --preset ci --target schwaystation-test-sio-pad schwaystation-test-sio` exited 0. `sio.gun.guncon`, `sio.state.pending_byte`, `sio.state.pending_ack`, and `sio.transport.abort` passed 4/4.
- No broad workflow, hosted CI, `act`, Docker, or quality-metric producer was needed for this plan. The phase's integrated local and hosted gates remain outstanding.

## Task Commits

1. **Task 1 RED probe** — `c5a56cb` (`test(15-04): add failing GunCon poll vector`)
2. **Task 1 GREEN** — `d125465` (`feat(15-04): implement GunCon poll replies`)
3. **Task 2 RED probe** — `fadeff5` (`test(15-04): pin GunCon no-light and reload responses`)
4. **Task 2 GREEN** — `f49d794` (`feat(15-04): stabilize GunCon no-light replies`)

## Decisions Made

- Chose `000Ah` for unavailable input or no successful light sample and `0005h` for explicit offscreen/reload. The cited PER.01 notes overlap those conditions, so the values are documented as deterministic model choices rather than measured physical responses.
- Only the documented GunCon A, trigger, and B bits are exposed in the serial button halfword; unsupported bits remain released.

## Deviations from Plan

None - plan executed as written.

## Issues Encountered

- The first RED record serialization appended a literal escape and was rejected as unreadable JSON. It was regenerated from the unchanged TAP report, then the classifier returned `RED_EVIDENCE_OK`; no test rerun or source change was needed.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Plan 15-05 can now supply beam-derived X/Y observation values through the pad result slot. No physical-gun calibration or retail-game compatibility is claimed.

---
*Phase: 15-guncon-and-justifier*
*Completed: 2026-11-25*

## Self-Check: PASSED

- Summary file exists and coverage classification passed for both deliverables.
- Task commits `c5a56cb`, `d125465`, `fadeff5`, and `f49d794` are ancestors of the current HEAD.
- `git diff --check` passed.
