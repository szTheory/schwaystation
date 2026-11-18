---
phase: 04-games-play-in-retroarch
plan: "20"
subsystem: core-bios
tags: [C, CMake, BIOS, SHA-256, save-state]

requires:
  - phase: 04-02
    provides: Stub BIOS window and reset-vector execution path.
  - phase: 04-34
    provides: API v4 instance and state-validation baseline.
  - phase: 04-36
    provides: Runner and integration-test interfaces.
provides:
  - Exact-size host BIOS loading into instance-owned storage, booted at BFC00000h.
  - Public SHA-256 image identity and save-state identity validation without serializing BIOS bytes.
  - Gating BIOS copy, reset, refusal, and matching/mismatching state coverage.
affects: [GAME-05, BIOS loading, API-v4, save-states, runner]

actuals:
  tokens: 8257
  tasks: 2
  commits: 4
commits: 4
plan_head_before: 278295b0eb57dfd24ddbfa5c4884fdbc2ba9b00c
plan_head_after: 0383228858d3577189a095caf337b61a09411d09

tech-stack:
  added: []
  patterns:
    - Host firmware is copied into per-instance memory; state stores its identity, not its bytes.

key-files:
  created:
    - src/bios.h
  modified:
    - include/schwaystation.h
    - src/bios.c
    - src/bus.c
    - src/bus.h
    - src/frame.c
    - src/instance.c
    - src/internal.h
    - src/state.h
    - runner/CMakeLists.txt
    - tests/api/v4.c
    - tests/built/bios_tests.c
    - tests/fixtures/tests.cmake
    - docs/embedding.md
    - ASSET_POLICY.md

key-decisions:
  - "Save states record whether a BIOS is loaded and its SHA-256 identity, never its bytes; a mismatch is rejected before mutation."
  - "Public SHA-256 identifies every loaded image; retail recognition stays separate and uses the DAT MD5/SHA-1 facts in plan 04-38."

requirements-completed: []
coverage:
  - id: D1
    description: "An exact-size synthetic BIOS is copied, fetched from reset vector BFC00000h, and reported with its independently checked SHA-256."
    requirement: GAME-05
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^built\\.bios\\.synthetic_rom$' --no-tests=error --output-on-failure"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci"
        status: pass
    human_judgment: false
  - id: D2
    description: "NULL, wrong-size, and late loads are refused without changing the current BIOS bytes or reported identity."
    requirement: GAME-05
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^built\\.bios\\.copy_edges$' --no-tests=error --output-on-failure"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci"
        status: pass
    human_judgment: false
  - id: D3
    description: "Power reset retains the BIOS; save-state restore accepts the same identity and atomically rejects a different identity without serializing BIOS bytes."
    requirement: GAME-05
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^built\\.bios\\.copy_edges$' --no-tests=error --output-on-failure"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci"
        status: pass
    human_judgment: false
  - id: D4
    description: "The real API v4 caller reports a known-answer BIOS digest while frozen earlier API callers and runner digest behavior remain valid."
    requirement: GAME-05
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^abi\\.api_v4$' --no-tests=error --output-on-failure"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci"
        status: pass
    human_judgment: false

duration: 40min
completed: 2026-11-11
status: complete
---

# Phase 04 Plan 20: Copied BIOS Execution and SHA-256 Identity Summary

**Hosts can load a copied 512 KiB BIOS that executes from the reset vector, report its SHA-256, and safely match that identity when loading state.**

## Performance

- **Duration:** 40 min
- **Started:** 2026-11-11T12:46:23Z
- **Completed:** 2026-11-11T13:26:12Z
- **Tasks:** 2
- **Files modified:** 21
- **Runtime/model:** Codex / OpenGSD 1.20.0, GPT-6 Luna xhigh, sequential shared phase checkout.

## Accomplishments

- Added an exact-size BIOS loader that copies caller bytes into the instance, keeps the image across power reset, and begins CPU execution at BFC00000h.
- Exposed the loaded flag and SHA-256 through API v4. The identity is independent from retail MD5/SHA-1 recognition, and unknown exact-size images remain loadable.
- Added state fields for BIOS presence and digest; state validation rejects a different image before mutating the target and never serializes firmware bytes.
- Added synthetic BIOS coverage for known-answer hashing, reset-vector fetch, source mutation/free, invalid and late load refusal, reset retention, and save-state identity. The tests contain runtime-constructed instructions only; no BIOS bytes were committed.

## Evidence

- TDD RED: the registered `built.bios.synthetic_rom` test failed against the committed scaffold on the expected empty flags, SHA-256, BIOS span, and RAM marker assertions. `check tdd-red-evidence` returned `RED_EVIDENCE_OK` for `build/ci/bios-red-evidence.json`.
- Focused verification: `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-bios schwaystation-test-api-v4 && ctest --test-dir build/ci -R '^(built\\.bios\\.(synthetic_rom|copy_edges)|abi\\.api_v4)$' --no-tests=error --output-on-failure` — exit 0.
- Final full verification: `cmake --workflow --preset ci` — exit 0; all 459 CI tests and both release archive checks passed.
- Final hygiene: `cmake --workflow --preset hygiene` — exit 0; all 23 repository and policy checks passed.
- The focused recheck for the two state goldens, embedding build, and API-name documentation check passed after their updates.

## TDD Gate Compliance

- **RED:** The synthetic reset-vector test produced a real target failure before implementation; the evidence checker accepted the captured report.
- **GREEN:** The copied BIOS implementation passed the reset-vector, copy-edge, and API v4 tests, followed by full CI and hygiene.

## Task Commits

1. **Task 1 RED:** `df4f4bd` — add failing BIOS identity and reset-vector test.
2. **Task 1 GREEN:** `1b800e2` — execute copied BIOS images and report SHA-256.
3. **Task 2:** `c31a7a6` — cover BIOS copy, refusal, reset, and state identity.
4. **Integration fixes:** `0383228` — align v4 state goldens, embed target expectations, and API docs.

## Decisions Made

- Ordinary save states carry only the loaded flag and SHA-256 identity; BIOS bytes remain instance-owned host input.
- The API reports image identity without claiming retail recognition. The DAT-based MD5/SHA-1 recognition slice remains in plan 04-38.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Integration] Wired the new test directory into ordinary CI.**
- **Found during:** Task 1.
- **Issue:** `tests/built` was absent from the central CI inventory, and its test target did not include the shared `tests/check.h` directory.
- **Fix:** Added the directory to `tests/inventory/ci.txt` and the test include path to `tests/built/tests.cmake`.
- **Verification:** Both BIOS tests were selected by full CI and passed.
- **Committed in:** `df4f4bd`.

**2. [Rule 3 - Integration] Updated existing gates for the added core object targets and API name.**
- **Found during:** Final CI.
- **Issue:** The embed regression pins its internal target list, and `docs.api_names` requires each public type name to appear in the guide.
- **Fix:** Added the BIOS and shared SHA object targets to `tests/embed/CMakeLists.txt` and named `schway_bios_info` in `docs/embedding.md`.
- **Verification:** `embed.subdirectory` and `docs.api_names` passed; final CI passed.
- **Committed in:** `0383228`.

**3. [Rule 3 - Integration] Refreshed interim v4 state identity goldens.**
- **Found during:** Final CI.
- **Issue:** Adding the BIOS loaded flag and SHA-256 to serialized v4 state intentionally changed the two pinned `gpu_motion` state hashes.
- **Fix:** Updated the frame-150 and frame-180 expected state hashes to the values produced by the new state format.
- **Verification:** Both `fixtures.gpu_motion.state.*` checks and final CI passed.
- **Committed in:** `0383228`.

**Total deviations:** 3 auto-fixed integration updates. **Impact:** Each keeps an existing registered build, API, or state check aligned with this planned v4 behavior; scope remains within the BIOS slice.

## Issues Encountered

The first full CI run exited 8 because those four expected integrations were stale. After updating the CI target allowlist, API type documentation, and two v4 state hashes, the focused rechecks and final full CI passed. No unresolved issues remain.

## User Setup Required

None. No external service, personal BIOS, or hardware verification is required for this slice.

## Next Phase Readiness

The copied-image loader and identity contract are ready for dependent work. Retail DAT recognition remains separate in plan 04-38, and no firmware bytes were committed.

## Self-Check: PASSED

- SUMMARY exists at the planned phase path.
- Task commits `df4f4bd`, `1b800e2`, `c31a7a6`, and `0383228` are ancestors of `plan_head_after`.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-11*
