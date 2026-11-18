---
phase: 04-games-play-in-retroarch
plan: "14"
subsystem: serial-io
tags: [C, SIO0, digital-pad, CTest]
requires:
  - phase: 04-games-play-in-retroarch
    plan: "01"
    provides: Per-directory test registration and exact CTest inventory.
  - phase: 04-games-play-in-retroarch
    plan: "02"
    provides: CPU MMIO, IRQ behavior, and validated instance state.
  - phase: 04-games-play-in-retroarch
    plan: "09"
    provides: Shared scheduler deadline ordering and dispatch.
provides:
  - Instance-owned SIO0 registers, RX FIFO, pending byte and ACK deadlines, and digital-pad transactions.
  - Deterministic save/load behavior for pending SIO work and per-port input state.
  - SIO0 edge tests and hardware timing notes for the digital poll slice.
affects: ["04-15", "04-16", SIO0, save-state, GAME-03]
actuals:
  tokens: 12588
  tasks: 3
  commits: 4
commits: 4
plan_head_before: 0c7b52c721067605c1f9239923aa7f2e621ed0fb
plan_head_after: d05e8b64d224c155b2dc187f40a96b22500c09c1
tech-stack:
  added: []
  patterns:
    - Per-instance SIO/pad state uses the existing fixed-width validate-then-apply state visitor.
    - Byte completion, device ACK, and ACK release remain separate scheduler deadlines.
key-files:
  created:
    - src/sio.c
    - src/sio.h
    - src/pad.c
    - src/pad.h
    - docs/hardware/sio.md
    - tests/sio/sio_tests.c
  modified:
    - CMakeLists.txt
    - src/bus.c
    - src/instance.c
    - src/internal.h
    - src/sched.c
    - src/state.c
    - tests/core/bus_unbuilt.c
    - tests/embed/CMakeLists.txt
    - tests/inventory/ci.txt
    - tests/sio/tests.cmake
    - tests/sio/inventory.txt
key-decisions:
  - Keep byte completion, pad ACK, and ACK release at distinct absolute ticks; do not reuse pad timing for cards.
  - Preserve API v4 and save-state format v4 while adding instance-owned SIO and pad fields.
  - Keep GAME-03 partial until DualShock, cards, host input, and integrated gameplay work land.
patterns-established:
  - SIO tests drive JOY MMIO and the instance scheduler at exact deadlines.
  - Peeks are checked against saved state to prove they do not consume or advance device state.
requirements-completed: []
coverage:
  - id: D1
    description: SIO0 digital polling returns the documented handshake and active-low button bytes with separate byte and pad-ACK timing and no final-byte ACK.
    requirement: GAME-03
    verification:
      - kind: unit
        ref: tests/sio/sio_tests.c#digital_poll_irq; sio.digital.poll_irq; sio.pad.final_no_ack
        status: pass
    human_judgment: false
  - id: D2
    description: Pending byte and ACK deadlines survive save/load, abort on reset or deselect, and keep both ports and instances independent.
    requirement: GAME-03
    verification:
      - kind: unit
        ref: tests/sio/sio_tests.c#state_pending_byte,state_pending_ack,transport_abort,independent_ports_instances
        status: pass
    human_judgment: false
  - id: D3
    description: Baud and character-length timing, FIFO widths and overflow, IRQ clear ordering, and pure peeks are deterministic.
    requirement: GAME-03
    verification:
      - kind: unit
        ref: tests/sio/sio_tests.c#baud_character_length,fifo_widths,fifo_overrun,irq_ack_order,peek_pure
        status: pass
    human_judgment: false
  - id: D4
    description: All registered SIO behavior passes the CI and hygiene workflows, including packaging and repository checks.
    requirement: GAME-03
    verification:
      - kind: other
        ref: cmake --workflow --preset ci
        status: pass
      - kind: other
        ref: cmake --workflow --preset hygiene
        status: pass
    human_judgment: false
duration: 47min
completed: 2026-11-10
status: complete
---

# Phase 4 Plan 14: Schedule SIO0 bytes and digital-pad replies

**SIO0 now schedules digital-pad bytes at baud-derived ticks, follows the separate pad ACK boundary, and restores pending transport state exactly.**

## Performance

- **Duration:** about 47 min
- **Started:** approximately 2026-11-10T19:11:03-05:00, after plan 04-10 closeout
- **Completed:** 2026-11-10T19:58:34-05:00
- **Tasks:** 3
- **Files modified:** 17 source, test, inventory, and documentation files

## Accomplishments

- Added an instance-owned SIO0 register and FIFO module and a per-port digital-pad state machine. The scheduled MMIO poll returns `FF/41/5A/buttons`; at baud `0088h`, bytes complete at 1,088 ticks, pad ACK asserts at 1,426, and the final reply byte has no ACK.
- Serialized pending byte/ACK/release deadlines and pad transaction state without changing API v4 or state format v4. Added exact save/load, abort, two-port, and two-instance tests.
- Registered FIFO-width and overrun, baud, IRQ-clear, final-byte, and pure-peek cases. Documented timing inputs, source strength, and the model-versus-measurement distinction in `docs/hardware/sio.md`.

## Task Commits

1. **Task 1 RED: digital SIO poll test** - `73e1f62` (`test`)
2. **Task 1 GREEN: scheduled SIO0 and digital pad** - `b1049c6` (`feat`)
3. **Task 2: state restore, abort, and isolation coverage** - `df22242` (`test`)
4. **Task 3: transport edge tests and timing notes** - `d05e8b6` (`test`)

**Plan metadata:** The closeout commit includes this summary and the GSD state updates.

## Files Created/Modified

- `src/sio.c`, `src/sio.h`, `src/pad.c`, and `src/pad.h` - Implement SIO0 registers, scheduled byte and ACK events, the RX FIFO, and digital-pad transactions.
- `src/bus.c`, `src/internal.h`, `src/instance.c`, `src/sched.c`, `src/state.c`, and `CMakeLists.txt` - Route and schedule SIO0, own its state per instance, and build its objects.
- `tests/sio/sio_tests.c`, `tests/sio/tests.cmake`, and `tests/sio/inventory.txt` - Exercise MMIO, timing, saved state, transport edges, FIFO behavior, IRQs, and peeks.
- `tests/core/bus_unbuilt.c`, `tests/embed/CMakeLists.txt`, and `tests/inventory/ci.txt` - Keep bus-range and embedded-object expectations aligned with the new SIO0 module.
- `docs/hardware/sio.md` - Record register, poll, ACK, FIFO, and source-confidence behavior.

## Decisions Made

- Byte completion, pad ACK assertion, and ACK release remain separate scheduler events. The future card ACK delay remains a distinct rule.
- SIO and pad state are instance-owned and pass through the existing state visitor, preserving the current serialized format and API revisions.
- GAME-03 remains incomplete until later plans add DualShock behavior, memory cards, host input, and integrated gameplay checks.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Preserve the flat I/O bus cost for built SIO0 registers**
- **Found during:** Task 1
- **Issue:** The first bus integration treated the SIO0 address range as an unbuilt expansion gap and bypassed the ordinary I/O access cost.
- **Fix:** Route only SIO0's implemented register window through the device handler, retain normal I/O cost, and leave gaps/SIO1 unbuilt.
- **Files modified:** `src/bus.c`, `tests/core/bus_unbuilt.c`
- **Verification:** `core.bus_unbuilt`, `embed.subdirectory`, and full CI passed.
- **Committed in:** `b1049c6`

### TDD Sequencing Adjustment

Task 2's required state/abort/independence behavior was already implemented in Task 1, while Task 2's file list contains only tests and inventories. The new behavior tests therefore passed before any Task 2 implementation work; inspection confirmed the existing code restored exact deadlines and canceled stale events. Task 2 was committed as regression coverage without a redundant source change or a fabricated RED result.

**Total deviations:** 1 auto-fixed bug and 1 TDD sequencing adjustment.
**Impact on plan:** The planned SIO0 scope is complete; the adjustment added coverage without changing established behavior or widening scope.

## Issues Encountered

- The first Task 1 test run exposed that I_STAT is write-zero-to-clear; the test was corrected to clear bit 7 by writing zero.
- A new Task 2 assertion initially expected three completed pad polls after two polls on one instance; it was corrected to the documented one-count-per-poll behavior.

## Evidence

- **Task 1 RED:** `ctest --test-dir build/ci -R '^sio\.digital\.poll_irq$' --no-tests=error --output-on-failure --output-junit build/ci/sio-red.junit.xml` — exit 8. The named test failed because RX stayed empty and JOY_RX_DATA returned zero before SIO0 existed. The machine-validated report is preserved at `build/ci/build/ci/sio-red.junit.xml`.
- **Task 1 GREEN:** `cmake --workflow --preset ci` — exit 0; 394/394 tests and both release-archive checks passed.
- **Task 2:** `ctest --test-dir build/ci -R '^sio\.(state\.pending_byte|state\.pending_ack|transport\.abort|pad\.independent_ports_instances)$' --no-tests=error --output-on-failure` — exit 0; 4/4 passed. Its full CI run exited 0 with 398/398 tests and both release checks passing.
- **Task 3 focused:** `ctest --test-dir build/ci -R '^sio\.' --no-tests=error --output-on-failure` — exit 0; all 11 SIO tests passed.
- **Final CI:** `cmake --workflow --preset ci` — exit 0; 404/404 tests and both release-archive checks passed.
- **Final hygiene:** `cmake --workflow --preset hygiene` — exit 0; all 23 checks passed. `git diff --check` also exited 0.
- No hosted run URL was produced. No owner UAT or setup is required.

## User Setup Required

None - no external service configuration is required.

## Next Phase Readiness

The SIO0 byte and digital-pad layer is ready for the next controller plan to add DualShock behavior. Card transactions remain unimplemented and must retain their separately sourced ACK timing. GAME-03 is still partial; no user action is needed for this plan.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-10*

## Self-Check: PASSED

- SUMMARY exists at the required path.
- Task commits `73e1f62`, `b1049c6`, `df22242`, and `d05e8b6` are ancestors of HEAD.
- SIO implementation, tests, inventory, and hardware documentation files exist.
