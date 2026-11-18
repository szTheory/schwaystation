---
phase: 04-games-play-in-retroarch
plan: "15"
subsystem: serial-io
tags: [C, API-4, SIO0, DualShock, rumble]
requires:
  - phase: 04-games-play-in-retroarch
    plan: "14"
    provides: Scheduled SIO0 byte delivery and digital controller transactions.
  - phase: 04-games-play-in-retroarch
    plan: "03"
    provides: API v4 base types and the validated state visitor.
provides:
  - API v4 host input, frame lag, and normalized per-port rumble reports.
  - Frame-latched digital and analog DualShock input through SIO0.
  - DualShock configuration commands, both rumble protocols, and the exact watchdog.
  - Tests and embedding guidance for input, polling, protocol state, and result compatibility.
affects: ["04-12", "04-16", libretro host, runner inputs, save-state]
actuals:
  tokens: 13904
  tasks: 2
  commits: 4
commits: 4
plan_head_before: f8f0a078b922580503dc36aa30233ac7009c8ffd
plan_head_after: deb3e46a70c5c9a42eae77d970a9c32426f4d839
tech-stack:
  added: []
  patterns:
    - Host input is queued outside saved state and latched once at a frame boundary.
    - Latched controller and in-flight SIO protocol fields use the existing state visitor.
key-files:
  created:
    - tests/sio/pad_tests.c
  modified:
    - include/schwaystation.h
    - src/frame.c
    - src/internal.h
    - src/pad.c
    - src/pad.h
    - src/sio.c
    - tests/api/v4.c
    - tests/sio/tests.cmake
    - tests/sio/inventory.txt
    - docs/embedding.md
key-decisions:
  - Keep the approved API v4 and append input and result fields without changing the frozen v1-v3 callers.
  - Keep pending host input outside save states while saving latched input, protocol phases, mappings, watchdog, and rumble state.
patterns-established:
  - A completed 42h SIO poll is the sole source of per-frame non-lag status; peeks do not count.
  - Apply a staged 4Dh actuator map only after all six mapping bytes arrive.
requirements-completed: []
coverage:
  - id: D1
    description: API v4 accepts frame-latched host input, returns lag and per-port rumble, and preserves earlier caller sizes.
    requirement: GAME-03
    verification:
      - kind: unit
        ref: "abi.api_v1, abi.api_v2, abi.api_v3, abi.api_v4, and sio.pad.input_latch in cmake --workflow --preset ci"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci (exit 0; 413 checks and 2 release-package checks)"
        status: pass
    human_judgment: false
  - id: D2
    description: DualShock 43h-47h, 4Ch, and 4Dh commands return their documented payloads and apply mode, lock, selector, and mapping changes.
    requirement: GAME-03
    verification:
      - kind: unit
        ref: "sio.pad.config_commands via ctest --test-dir build/ci -R '^sio\\.pad\\.'"
        status: pass
    human_judgment: false
  - id: D3
    description: Analog-button rising edges respect the configuration lock, legacy 0140h rumble turns on and 0000h turns off, and the watchdog expires at exactly 33,868,800 CPU ticks.
    requirement: GAME-03
    verification:
      - kind: unit
        ref: "sio.pad.analog_button_edges, sio.pad.old_rumble, and sio.pad.watchdog_boundary via ctest --test-dir build/ci -R '^sio\\.pad\\.'"
        status: pass
    human_judgment: false
  - id: D4
    description: A partial actuator-map transaction resumes after a split run and save/load without changing its staged bytes.
    requirement: GAME-03
    verification:
      - kind: unit
        ref: "sio.pad.state_split_config via ctest --test-dir build/ci -R '^sio\\.pad\\.'"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci (exit 0; 413 checks and 2 release-package checks)"
        status: pass
    human_judgment: false

duration: 58min
completed: 2026-11-10
status: complete
---

# Phase 4 Plan 15: Host input and DualShock summary

**API v4 input now reaches timed SIO polls, with complete DualShock configuration, rumble reporting, and watchdog behavior.**

## Performance

- **Duration:** 58 min
- **Started:** 2026-11-11T02:47:11Z
- **Completed:** 2026-11-11T03:45:34Z
- **Tasks:** 2
- **Files modified:** 11

## Accomplishments

- Added `schway_input` and `schway_set_input`, latching two-port input at frame start and preserving the API v4 result layout for older-sized output buffers.
- Implemented digital and analog poll payloads, the full DualShock config command table, staged 4Dh actuator mappings, legacy rumble, and the exact emulated-time watchdog.
- Saved latched controller/protocol state while leaving pending host input outside ordinary states; added tests for state continuation and documented the input/result contract.

## Task Commits

1. **Task 1: Trace host input through a DualShock poll** — RED `720494e`, GREEN `cef192e`.
2. **Task 2: Complete configuration, old rumble and watchdog edges** — RED `c699ceb`, GREEN `deb3e46`.

## Files Created/Modified

- `include/schwaystation.h`, `src/internal.h` — API v4 input type, device constants, and appended run-result fields with layout assertions.
- `src/frame.c` — input validation/queueing, frame-boundary latching, watchdog advancement, and lag/rumble output.
- `src/pad.c`, `src/pad.h`, `src/sio.c` — per-port digital/analog controller protocol, config commands, rumble, and serialized protocol state.
- `tests/api/v4.c` — exercises all new public names and verifies the old result-size canary.
- `tests/sio/pad_tests.c`, `tests/sio/tests.cmake`, `tests/sio/inventory.txt` — timed-SIO and watchdog acceptance coverage.
- `docs/embedding.md` — input/result and DualShock protocol contracts; retains the published DMA fault `addr`/`opcode` exception.

## Decisions Made

- Kept the approved API at version 4 and preserved frozen v1-v3 callers.
- Stored host-pending input outside ordinary save states, while serializing latched input and controller protocol state needed to resume execution.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] A 01h data byte could restart an active pad transaction**
- **Found during:** Task 1, after the first GREEN implementation.
- **Issue:** The transaction start byte was recognized while a payload was active, so command 43h's enter-config byte was consumed as a new start.
- **Fix:** Recognize the start byte only while the controller protocol is idle.
- **Files modified:** `src/pad.c`.
- **Verification:** `sio.pad.input_latch` passed in the task verification run.
- **Committed in:** `cef192e`.

**2. [Rule 3 - Blocking gate] The no-float scanner matched section numbers in a source comment**
- **Found during:** Task 2 hygiene verification.
- **Issue:** The citation `2.1-2.4` was interpreted as floating-point syntax.
- **Fix:** Keep the preparation document citation and remove the numeric section range from the source comment.
- **Files modified:** `src/pad.c`.
- **Verification:** `cmake --workflow --preset hygiene` passed all 23 checks.
- **Committed in:** `deb3e46`.

## Evidence

- Task 1 RED: `ctest --test-dir build/ci -R '^sio\\.pad\\.input_latch$' --no-tests=error --output-on-failure --output-junit plan04-15-task1-red.xml` exited 8. The target ran through SIO MMIO and returned ID 41h instead of 73h; `check tdd-red-evidence` returned `RED_EVIDENCE_OK`.
- Task 1 GREEN and tracer gate: `cmake --workflow --preset ci` exited 0 (408 checks and 2 release-package checks), then the ABI/SIO label run exited 0 (28 checks).
- Task 2 RED: the five pad acceptance tests exited 8; the named `sio.pad.config_commands` test failed on the planned config response. The analog-lock and old-rumble cases also failed; watchdog-boundary and state-continuation cases passed. `check tdd-red-evidence` returned `RED_EVIDENCE_OK`.
- Pad/SIO run: `ctest --test-dir build/ci -R '^sio\\.pad\\.' --no-tests=error --output-on-failure` exited 0 (8 checks).
- Full CI: `cmake --workflow --preset ci` exited 0 (413 checks and 2 release-package checks).
- Hygiene: `cmake --workflow --preset hygiene` exited 0 (23 checks).
- All checks ran headlessly; there are no hosted run URLs for these local commands.

## TDD Gate Compliance

- Task 1 has a committed RED test (`720494e`) followed by the implementation commit (`cef192e`). The target failed on the expected digital ID and absent analog payload, then passed after the frame latch and SIO implementation.
- Task 2 has a committed RED test (`c699ceb`) followed by the implementation commit (`deb3e46`). The named config test failed on missing command 44h behavior; config, edge/lock, legacy rumble, watchdog, and save/load tests passed after implementation.
- No separate refactor commit was needed.

## Issues Encountered

The first full CI run caught missing documentation and API-v4 test references for the newly added public names; both were added before the Task 1 GREEN commit. The first hygiene run caught the comment citation described above and passed after that correction.

## User Setup Required

None. All verification is local and automated.

## Next Phase Readiness

The API and SIO controller contracts are ready for the remaining phase consumers. GAME-03 remains pending because its runner fixture and integrated host behaviors are still scheduled in other plans.

---

*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-10*

## Self-Check: PASSED

- SUMMARY.md and the new SIO pad test file exist.
- All four task commits are ancestors of the current phase branch head.
- Measured plan commits: 4; `plan_head_before` and `plan_head_after` match the saved ledger and current task head.
