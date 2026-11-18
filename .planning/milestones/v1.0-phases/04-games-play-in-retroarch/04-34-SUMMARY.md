---
phase: 04-games-play-in-retroarch
plan: "34"
subsystem: card
tags: [memory-card, state, api-v4, sio]
requires:
  - phase: 04-games-play-in-retroarch
    plan: "16"
    provides: Stable card spans, raw import API, and saved read transport
  - phase: 04-games-play-in-retroarch
    plan: "33"
    provides: Staged writes that commit at successful protocol completion
provides:
  - Short raw card imports padded with FFh and refused imports that preserve bytes and generation
  - Validation of pending card transport before state application
  - Regression coverage for both v4 card slots and state resume across staged writes
affects: [card, state, host persistence]
actuals:
  tokens: 7127
  tasks: 2
  commits: 3
commits: 3
plan_head_before: a9c9c6f78b121fd6014605b6ce104c10b920b3ad
plan_head_after: ef9ef7b454634d9a0ebd9c25346ec717ede44c12
tech-stack:
  added: []
  patterns:
    - Validate saved card protocol fields and staged bytes before APPLY
    - Keep persistent card bytes and generations outside ordinary states
key-files:
  created: []
  modified:
    - src/card.c
    - src/instance.c
    - tests/api/v4.c
    - tests/card/card_tests.c
    - tests/card/tests.cmake
    - tests/card/inventory.txt
    - docs/embedding.md
    - docs/hardware/sio.md
key-decisions:
  - Keep API, state format, and behavior revision 4; preserve frozen v1-v3 callers.
  - Accept raw imports up to the fixed image size, pad the suffix with FFh, and reject all invalid or late imports before mutation.
  - Keep persistent card bytes and generations out of ordinary states while validating pending transport before state application.
patterns-established:
  - Card-state validation checks phase-specific address progress, staged checksums, and unused staging bytes.
  - A successful write completion advances only the receiving instance's card generation.
requirements-completed: []
coverage:
  - id: D1
    description: Raw imports accept short images with FFh padding and preserve card bytes, spans, and generation on refusal.
    requirement: GAME-04
    verification:
      - kind: unit
        ref: "abi.api_v4 (CTest)"
        status: pass
      - kind: unit
        ref: "card.state.import_atomic (CTest)"
        status: pass
    human_judgment: false
  - id: D2
    description: Pending Read, GetID, and Write transport is validated before APPLY; refusals preserve the complete target instance, and restored writes do not restore persistent card data.
    requirement: GAME-04
    verification:
      - kind: unit
        ref: "card.state.import_atomic (CTest)"
        status: pass
      - kind: unit
        ref: "card.state.pending_edges (CTest)"
        status: pass
    human_judgment: false
duration: 28min
completed: 2026-11-11
status: complete
---

# Phase 4 Plan 34: Card Import and State Validation Summary

**Raw card imports now FF-pad short images, and pending card transports are validated atomically before save-state application.**

## Performance

- **Duration:** 28min
- **Started:** 2026-11-11T09:15:36Z
- **Completed:** 2026-11-11T09:43:23Z
- **Tasks:** 2
- **Files modified:** 8

## Accomplishments

- Raw imports accept up to 131,072 bytes, pad shorter images with `FFh`, and reject oversized, invalid-slot, null, or post-execution imports without changing card bytes or generation.
- Card state validation now checks phase and address progression, known checksum relationships, write staging progress, and unused staging bytes before applying any saved state.
- Tests prove failed state loads preserve a full instance snapshot and that a restored staged write commits to the receiving instance without rewinding its card bytes or generation.

## Task Commits

1. **Task 1 RED:** `d77d0f2` — add failing card import and state tests.
2. **Task 1 GREEN:** `22bffc9` — pad raw imports and validate state transport.
3. **Task 2:** `ef9ef7b` — cover pending card state edge cases and both v4 card slots.

## Files Created/Modified

- `src/card.c` — validate card protocol fields and staged write data before state application.
- `src/instance.c` — accept short raw images and FF-pad their unused suffix.
- `tests/api/v4.c` — cover short, empty, oversized, null, and late imports across both card slots.
- `tests/card/card_tests.c`, `tests/card/tests.cmake`, `tests/card/inventory.txt` — register import atomicity, malformed-state, and pending-write resume cases.
- `docs/embedding.md` — document raw import bounds, host-owned file I/O, state separation, and generation semantics.
- `docs/hardware/sio.md` — align the card import description with the public API contract.

## Decisions Made

- Kept the unreleased API, state format, and behavior revision at 4; no state fields or v4 hash expectations changed.
- Kept card bytes and generation counters outside ordinary states. Saved transport resumes against the receiving instance's persistent card.
- Limited the core import API to raw bytes; hosts remain responsible for file access and wrapper decoding.

## TDD Gate Compliance

- Task 1 RED: `ctest --test-dir build/ci -R '^card\.state\.import_atomic$' --no-tests=error --output-on-failure --output-junit build/ci/card-state-import-red.xml` exited 8 on the expected short-import and malformed-checksum assertions. The machine classifier returned `RED_EVIDENCE_OK`; the report and record are under `build/ci/`.
- Task 1 GREEN: the focused import/state test and `abi.api_v4` passed after implementation.
- Task 2 expanded regression coverage for adjacent pending phases and state resume; it was not relabeled as a second RED cycle.
- The RED commit precedes the GREEN commit.

## Deviations from Plan

- Updated `docs/hardware/sio.md` because it repeated the stale exact-size-only import statement. The change keeps the hardware guide consistent with the planned API behavior.

## Issues Encountered

The first RED run exposed overly noisy per-byte failure assertions in the new padding test. They were combined into a single assertion, and the clean rerun produced valid machine-checked RED evidence. No unresolved issues remain.

## User Setup Required

None.

## Next Phase Readiness

The card import and pending-state slice is complete. Host file persistence and legacy wrapper decoding remain assigned to the dependent host-persistence plans.

## Evidence

- RED: focused `card.state.import_atomic` CTest command above — exit 8; `RED_EVIDENCE_OK` for the expected assertion failure.
- Focused GREEN: `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-card schwaystation-test-api-v4 && ctest --test-dir build/ci -R '^(card\.state\.(import_atomic|pending_edges)|abi\.api_v4)$' --no-tests=error --output-on-failure` — exit 0.
- Full CI: `cmake --workflow --preset ci` — exit 0; all 446 CI tests and both release archive checks passed.
- Hygiene: `cmake --workflow --preset hygiene` — exit 0; all 23 hygiene and policy checks passed.

## Self-Check: PASSED

- All eight modified source, test, and documentation files exist.
- Task commits `d77d0f2`, `22bffc9`, and `ef9ef7b` are ancestors of the current HEAD.
- `verify-summary` passed with no missing file or commit claims.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-11*
