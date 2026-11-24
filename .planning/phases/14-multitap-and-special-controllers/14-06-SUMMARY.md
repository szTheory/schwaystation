---
phase: 14-multitap-and-special-controllers
plan: "06"
subsystem: sio-special-controllers
tags: [c, sio, dual-analog-flight, jogcon]
requires:
  - phase: 14-multitap-and-special-controllers
    provides: Frame-latched controller endpoints and both tap polling methods
provides:
  - Dual Analog flight ID 5A53 with frame-latched buttons and four raw axes
  - Jogcon native ID 5AE3 payload and bounded refusal of unsupported commands
  - Public input and hardware documentation for the implemented Jogcon fields and evidence limits
affects: [phase-14-controller-host-mapping, sio-state]
actuals:
  tokens: 6546
  tasks: 2
  commits: 4
tech-stack:
  added: []
  patterns:
    - Controller-specific fixed poll lengths and payload switches over existing frame-latched integer state
    - Unsupported serial commands terminate with FF and no ACK before changing device or rumble state
key-files:
  created: []
  modified:
    - src/pad.c
    - tests/sio/pad_tests.c
    - include/schwaystation.h
    - docs/embedding.md
    - docs/hardware/sio.md
key-decisions:
  - "Dual Analog flight uses the existing axis order and ID 5A53 without redirecting DIGITAL or DUALSHOCK behavior."
  - "Jogcon supports only its documented native 42h payload; the unknown dial bit is cleared and dial interpretation is not inferred."
  - "Jogcon direction flags mean turned left/right; software configuration and feedback commands are refused with FF and no ACK."
patterns-established:
  - "Special-pad command support stays device-scoped; unsupported commands do not inherit DualShock configuration or rumble behavior."
requirements-completed: [SIO-03]
coverage:
  - id: D1
    description: Dual Analog flight returns ID 5A53, active-low buttons, and frame-latched right-X/right-Y/left-X/left-Y bytes on direct ports and both tap polling methods while digital and DualShock paths remain compatible
    requirement: SIO-04
    verification:
      - kind: integration
        ref: "sio.special.flight; focused six-test selector command below exited 0"
        status: pass
      - kind: integration
        ref: "sio.tap.poll_methods and existing DualShock selectors; focused six-test selector command below exited 0"
        status: pass
    human_judgment: false
  - id: D2
    description: Jogcon returns its native digital-button, raw dial, and left/right flag payload on direct and tap endpoints; save/load, repeated polls, reset, device change, and unsupported commands stay bounded and deterministic
    requirement: SIO-03
    verification:
      - kind: integration
        ref: "sio.special.jogcon; focused four-test selector command below exited 0"
        status: pass
    human_judgment: false
metrics:
  duration: 19m23s
  completed: 2026-11-24
  status: complete
commits: 4
plan_head_before: c9ab3affa5ece913610516629967623384a34030
plan_head_after: 4123d908be33640ccf619c76b61af42637b265d6
---

# Phase 14 Plan 06: Dual Analog Flight and Bounded Jogcon Summary

**Dual Analog flight and Jogcon now report distinct documented poll payloads while existing digital and DualShock behavior stays unchanged.**

## Performance

- **Duration:** 19m23s wall time through state recording (05:57:02–06:16:25 UTC); final metadata-commit time was not separately measured.
- **Runtime/model:** Codex GSD executor, configured `gpt-6-luna` xhigh; primary runtime model ID unavailable.
- **Tasks:** 2; **plan commits:** 4; **implementation files modified:** 5.

## Accomplishments

- Added Dual Analog flight ID `5A53` with active-low buttons and the frame-latched right-X, right-Y, left-X, left-Y bytes. Tests cover all four axes at 0/128/255, every button bit, both direct ports, tap Method 2 and fixed Method 1 records, frame latching, reset, and refusal of unsupported configuration commands.
- Added native Jogcon ID `5AE3` with digital buttons, raw dial halfword bit 0 cleared, and only the documented left/right direction bits. Tests cover signed-looking raw patterns without assigning their physical meaning, repeated polls, saved-state restoration, frame latching, reset/device change, both tap methods, and every listed unsupported command plus representative other commands.
- Unsupported Jogcon commands return `FF` with no ACK and preserve the device, latched controls, mode/configuration and rumble fields. Digital-pad selection remains the supported digital mode; no software mode switch or motor behavior is invented.
- Corrected the public `jogcon_flags` field descriptions and documented the exact supported/rejected command table and PER.01 evidence limits.

## Verification Evidence

All commands ran locally from the repository root. Changed paths were checked with `git diff --name-only | LC_ALL=C sort -cu`, `git diff --check` passed, and `cmake --preset ci` configured successfully. Exact registered selectors were asserted before target builds.

Task 1 RED: `ctest --test-dir build/ci --no-tests=error -R '^sio[.]special[.]flight$' --output-junit "$PWD/.gsd/tdd-evidence/14-06/task1-red.xml" --output-on-failure` exited 8 on the expected `41h` versus `53h` ID assertion; `gsd-tools check tdd-red-evidence .gsd/tdd-evidence/14-06/task1-red.json --raw` exited 0 with `RED_EVIDENCE_OK`. The report path is shown with `$PWD` to keep the repository location portable. After implementation, the exact six-test selection `^(sio[.]special[.]flight|sio[.]pad[.](config_commands|analog_button_edges|old_rumble|watchdog_boundary)|sio[.]tap[.]poll_methods)$` passed 6/6 in 0.01s, after both SIO test targets built successfully.

Task 2 RED: `ctest --test-dir build/ci --no-tests=error -R '^sio[.]special[.]jogcon$' --output-junit "$PWD/.gsd/tdd-evidence/14-06/task2-red.xml" --output-on-failure` exited 8 on the expected `41h` versus `E3h` ID assertion; the same checker exited 0 with `RED_EVIDENCE_OK`. After implementation, the exact four-test selection `^(sio[.]special[.](flight|jogcon)|sio[.]tap[.]poll_methods|sio[.]pad[.]config_commands)$` passed 4/4 in 0.04s, and both targeted SIO test targets built successfully.

The full `cmake --workflow --preset ci` suite was not run: workflow wiring did not change, and the required local serial and compatibility selectors passed. No Act or hosted run added coverage for this code slice; no Docker resources were created. There is no hosted run URL for this plan.

## Task Commits

1. **Task 1: Add the flight-mode payload while retaining old pad modes** — `bc4d8e1` (RED), `4bae1bb` (GREEN).
2. **Task 2: Implement native Jogcon input and explicit unsupported modes** — `504b9e6` (RED), `4123d90` (GREEN).

The measured plan commit count is four from `c9ab3affa5ece913610516629967623384a34030` through `4123d908be33640ccf619c76b61af42637b265d6`.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Corrected the flight test's active-low high-byte vectors**
- **Found during:** Task 1 focused GREEN run
- **Issue:** Two expected high-byte values complemented the button bit in the low byte, making correct replies appear wrong.
- **Fix:** Compute the active-low 16-bit value before splitting it into payload bytes.
- **Files modified:** `tests/sio/pad_tests.c`
- **Verification:** The complete six-test Task 1 selector passed after correction.
- **Committed in:** `4bae1bb`

**2. [Rule 1 - Bug] Corrected stale public Jogcon flag semantics**
- **Found during:** Task 2 source and API documentation review
- **Issue:** The public header and embedding guide described flags as clockwise and dial-valid, contradicting pinned PER.01's left/right direction flags and this plan's input contract.
- **Fix:** Document bit 0 as turned left and bit 1 as turned right; describe the raw dial halfword and reserved wire bit consistently.
- **Files modified:** `include/schwaystation.h`, `docs/embedding.md`, `docs/hardware/sio.md`
- **Verification:** The four-test Task 2 selector passed with independent 0/1/2/3 direction vectors.
- **Committed in:** `4123d90`

**Total deviations:** 2 auto-fixed. Both were directly required to make the test vectors and public input contract accurate.

## Issues Encountered

- The first Task 1 GREEN run exposed the test-vector issue above; corrected before accepting the pass.
- The first Task 2 test build found duplicate local variable names in the new test function. Renaming the Method 1 values restored the build before RED evidence was captured.
- The first CTest RED report used a relative JUnit path, which CTest placed under `build/ci`; the target was rerun with an absolute ignored evidence path. Only that fresh report was classified.

## Requirements and Limits

- **SIO-03 is complete:** together with Plan 14-05's neGcon slice, the documented neGcon/Jogcon controls have deterministic serial tests and unsupported Jogcon software modes are refused.
- **SIO-04 remains incomplete:** the core input boundary and serial flight payload are covered here, but the planned libretro mapping is owned by a later plan.
- No real-title gameplay, Jogcon dial interpretation, or force-feedback behavior is claimed.

## Next Phase Readiness

Plan 14-06 is complete. Plan 14-07 remains the next authorized phase slice; this executor did not start it or phase verification.

---
*Phase: 14-multitap-and-special-controllers*
*Completed: 2026-11-24*

## Self-Check: PASSED

- Summary file exists at `.planning/phases/14-multitap-and-special-controllers/14-06-SUMMARY.md`.
- Task commits `bc4d8e1`, `4bae1bb`, `504b9e6`, and `4123d90` are ancestors of HEAD.
- The focused four-test selector passed; the worktree was clean after the implementation commit.
