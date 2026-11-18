---
phase: 04-games-play-in-retroarch
plan: "37"
subsystem: runner-cards
tags: [C, CMake, armips, PS-EXE, SIO0, GPU]

requires:
  - phase: 04-19
    provides: Runner raw-card import and durable save-dir writes across fresh processes.
provides:
  - Guest SIO0 card counters, GPU rectangle rendering, GPUREAD mailbox values, and frame-120 pins.
  - Fresh-process persistence, missing-card, and wrong-slot integration coverage.
affects: [GAME-04, runner-cards, guest-fixtures]

actuals:
  tokens: 5086
  tasks: 2
  commits: 3
commits: 3
plan_head_before: b603e4cd88d7f6744168d423fde2f477d679d4cf
plan_head_after: 688255579c5fb7f6db49668e1561d1f66582e516

tech-stack:
  added: []
  patterns:
    - Guest SIO0 card transactions persist raw sector data through separate runner processes.
    - Mailbox counters, GPUREAD pixels, raw frame checksums, and frame hashes independently prove card behavior.

key-files:
  created:
    - tests/fixtures/src/memcard_roundtrip.asm
  modified:
    - docs/runner.md
    - tests/diag/expected.csv
    - tests/fixtures/inventory.txt
    - tests/fixtures/manifest.txt
    - tests/fixtures/tests.cmake

key-decisions:
  - "Map the formatted sector's A0h seed to guest-visible card counters 1 and 2, then increment each slot independently."
  - "Wait for 117 guest VBlanks so the mailbox and exact frame-120 sample complete within the runner's 120-frame cap."
  - "Validate the persisted frame checksum as well as counter bytes, GPU pixels, and the raw frame-120 hash."

patterns-established:
  - "A nested mailbox judge can run the same guest with an explicit save directory and capture a selected frame."
  - "The fixture keeps generated card images and PNG captures in the ignored build tree."

requirements-completed:
  - GAME-04
coverage:
  - id: D1
    description: "Two fresh runner processes persist card values 1/2 then 2/3 and match the exact frame-120 pixel/hash expectations."
    requirement: GAME-04
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^fixtures\\.memcard_roundtrip\\.persistence$' --no-tests=error --output-on-failure"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci"
        status: pass
    human_judgment: false
  - id: D2
    description: "A missing-card run and a wrong-slot image produce distinct mailbox counters, valid raw card frames, and exact frame hashes."
    requirement: GAME-04
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^fixtures\\.memcard_roundtrip\\.controls$' --no-tests=error --output-on-failure"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset hygiene"
        status: pass
    human_judgment: false

duration: 49min
completed: 2026-11-11
status: complete
---

# Phase 04 Plan 37: Fresh-Process Memory Card Persistence Summary

**The guest proves card 1/2 values survive two fresh runner processes, with card bytes, GPU pixels, and frame-120 hashes checked independently.**

## Performance

- **Duration:** 49 min
- **Started:** 2026-11-11T13:29:05Z
- **Completed:** 2026-11-11T14:18:21Z
- **Tasks:** 2
- **Files modified:** 6

## Accomplishments

- Added a real guest that reads and writes both card slots through SIO0, renders their values, and publishes five mailbox records after the exact frame-120 sample.
- Proved separate runner processes persist values 1/2 then 2/3. Card images are exactly 131,072 bytes and their sector checksums remain valid on import.
- Added missing-card and wrong-slot controls. They report 1/2 and 3/2 respectively, with distinct frame hashes.

## Task Commits

1. **Task 1: Prove card values survive two fresh guest processes** — RED `6c0f339`; GREEN `0c468a6`.
2. **Task 2: Expand and document the proven path** — `6882555`.

**Plan metadata:** The closeout commit contains this summary and the GSD state updates.

## Files Created/Modified

- `tests/fixtures/src/memcard_roundtrip.asm` — SIO0 reads and writes, two visible counters, pixel readbacks, and a sealed guest mailbox.
- `tests/fixtures/tests.cmake` and `tests/fixtures/inventory.txt` — Fresh-process persistence and missing/wrong-card cases registered in CI.
- `tests/diag/expected.csv` — Exact counter and RGB555 mailbox values for each guest run.
- `tests/fixtures/manifest.txt` — First-party fixture source digest.
- `docs/runner.md` — Save-dir persistence proof and the wrong-slot control.

## Decisions Made

- The fixture normalizes the formatted blank card's A0h sector byte to visible starting values 1 and 2, then advances each slot from its own saved value.
- The guest polls 117 VBlanks so its mailbox seals at frame 120; the runner records the selected frame hash and PNG at that same frame.
- The test checks both the counter byte and the sector checksum byte, and rejects imported images that report a bad frame checksum.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Removed an extra byte before the card write payload**
- **Found during:** Task 1
- **Issue:** The guest sent one extra zero before the 128-byte sector payload, shifting the checksum and final ACK.
- **Fix:** Removed the extra transfer so the checksum and final `47h` follow the documented write sequence.
- **Files modified:** `tests/fixtures/src/memcard_roundtrip.asm`
- **Verification:** The persistence CTest completed both card writes and observed the expected mailbox records.
- **Committed in:** `0c468a6`.

**2. [Rule 1 - Bug] Stored a valid memory-card frame checksum**
- **Found during:** Task 1
- **Issue:** The counter sector was writable, but its frame checksum stayed at the formatted-card value and the next process reported a bad checksum.
- **Fix:** Store the counter in the frame's XOR byte and send the corresponding SIO checksum; assert the raw checksum and clean import.
- **Files modified:** `tests/fixtures/src/memcard_roundtrip.asm`, `tests/fixtures/tests.cmake`
- **Verification:** The second process imported both cards without checksum warnings and advanced values to 2/3.
- **Committed in:** `0c468a6`.

**3. [Rule 3 - Blocking issue] Passed the save directory to the nested runner**
- **Found during:** Task 1
- **Issue:** The guest wrote cards, but the nested test invocation omitted `--save-dir`, so no host files were persisted.
- **Fix:** Pass the case's temporary directory to the runner and verify both generated files.
- **Files modified:** `tests/fixtures/tests.cmake`
- **Verification:** Both fresh-process persistence and control tests found the expected raw card files.
- **Committed in:** `0c468a6`.

**Total deviations:** 3 auto-fixed issues (2 bugs, 1 blocking harness issue).
**Impact on plan:** Each fix was required for the planned persistence proof; scope and phase boundaries were unchanged.

## Issues Encountered

The first 120 guest VBlanks completed at runner frame 123 because card transactions and rendering used three frames before the wait began. The guest now waits 117 VBlanks so the required mailbox and frame-120 capture both complete under the 120-frame cap.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Plan 04-38 can proceed with BIOS DAT recognition. The guest card persistence and controls from this plan are committed and green.

## Evidence

- **Task 1 RED:** `ctest --preset ci -R '^fixtures\.memcard_roundtrip\.persistence$' --output-on-failure --output-junit build/ci/memcard-roundtrip-red.xml` — exit 8. The fresh-process persistence assertion failed before the guest implementation; the report passed the RED evidence validator and semantic inspection.
- **Task 1 GREEN:** `ctest --test-dir build/ci -R '^fixtures\.memcard_roundtrip\.persistence$' --no-tests=error --output-on-failure` — exit 0. Frame-120 hashes: first `c532f85314c83a974466042df32a03ef7ce02ce8aea7c386843e44242d027a99`; second `cd1c037ef800397a9c95eb23ebc5524710f55db2eca8e03c975f841f0e9232db`.
- **Task 2:** `ctest --test-dir build/ci -R '^fixtures\.memcard_roundtrip\.controls$' --no-tests=error --output-on-failure` — exit 0. Missing-card hash `c532f85314c83a974466042df32a03ef7ce02ce8aea7c386843e44242d027a99`; wrong-slot hash `543fbfefe1dd81b5e59def1ab73bd902bd13a93285f332f54664e1479e40e545`.
- **Full CI:** `cmake --workflow --preset ci` — exit 0; all 461 tests and release-package checks passed.
- **Hygiene:** `cmake --workflow --preset hygiene` — exit 0; all 23 checks passed.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-11*

## Self-Check: PASSED

- Summary file exists.
- RED, GREEN, and Task 2 commits are ancestors of HEAD.
