---
phase: 04-games-play-in-retroarch
plan: "36"
subsystem: runner-inputs
tags: [C, CMake, PS-EXE, SIO0, GPU, input-movies]

requires:
  - phase: 04-11
    provides: Guest-controlled GPU DMA2 execution.
  - phase: 04-14
    provides: Cycle-scheduled SIO0 digital pad transfers.
  - phase: 04-18
    provides: Validated runner movies applied before each frame latch.
provides:
  - Guest-side Cross edge counting and GPU VRAM readback for frame 300.
  - Runner lag totals derived from each successful frame result.
  - Registered movie coverage for empty, single, held, repeated-edge, unsorted, conflicting and malformed input.
affects: [GAME-03, runner-inputs, guest-fixtures]

actuals:
  tokens: 5679
  tasks: 2
  commits: 3
commits: 3
plan_head_before: e7b538331bfa2d7340dced13d4a0b42b023b5ac0
plan_head_after: 31a4e4fc77de6baf2a466b85f75f822604360bf8

tech-stack:
  added: []
  patterns:
    - Guest SIO transfers and GPUREAD records provide independent input and pixel assertions.
    - Per-frame core lag results are summed by the runner without adding core counters.

key-files:
  created:
    - tests/fixtures/src/pad_counter.asm
  modified:
    - runner/main.c
    - docs/runner.md
    - tests/fixtures/tests.cmake
    - tests/fixtures/manifest.txt
    - tests/fixtures/inventory.txt
    - tests/diag/expected.csv

key-decisions:
  - "Judge actual guest input through SIO0 and GPU VRAM readback, not only host-side snapshots or converted images."
  - "Poll on alternating VBlanks so 150 guest no-poll frames remain observable while mailbox peeks happen each frame."
requirements-completed: []
coverage:
  - id: D1
    description: "A Cross input at frame 60 changes the guest press count, red counter pixel, and frame-300 image compared with no input."
    requirement: GAME-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^fixtures\\.pad_counter\\.press60$' --no-tests=error --output-on-failure"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci (exit 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: "Three released Cross edges count three, repeated held snapshots count once, and empty/single movies have exact guest counter-pixel records."
    requirement: GAME-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^fixtures\\.pad_counter\\.movies$' --no-tests=error --output-on-failure"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci (exit 0)"
        status: pass
    human_judgment: false
  - id: D3
    description: "The runner reports 150 lagged frames over 300 alternating-poll frames while mailbox checks peek each frame, and rejects conflicting or malformed movies before guest output."
    requirement: GAME-03
    verification:
      - kind: integration
        ref: "fixtures.pad_counter.movies via tests/fixtures/tests.cmake#pad_counter_movies"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset hygiene (exit 0)"
        status: pass
    human_judgment: false

duration: 41min
completed: 2026-11-11
status: complete
---

# Phase 04 Plan 36: Guest Input Edges, Pixels and Lag Summary

**The `pad_counter` guest proves scheduled SIO input changes real press counts and GPU pixels, while the runner reports guest polling lag.**

## Performance

- **Duration:** 41 min
- **Started:** 2026-11-11T11:24:03Z
- **Completed:** 2026-11-11T12:04:39Z
- **Tasks:** 2
- **Files modified:** 7

## Accomplishments

- Added a first-party PS-EXE guest that polls scheduled digital-pad bytes, counts Cross rising edges, draws a red pixel per edge through GPU DMA2, and records the count plus first/last VRAM pixels through GPUREAD.
- Added runner `lag frames=N` output by summing the core's per-frame lag result; `schway_peek` remains read-only and does not count as a guest poll.
- Verified no-input, one-event, held, and three-edge movies through the real runner and sealed mailbox. The three-edge movie deliberately lists frame events out of order; the guest records exactly three presses and RGB555 `0x001f` at both ends of the counter.
- Registered movie ordering/conflict and axis-encoding controls, pinned the first-party fixture source hash, and documented lag behavior.

## Task Commits

1. **Task 1 RED:** `b7440e9` — require lag output for input runs.
2. **Task 1 GREEN:** `913ec9a` — prove input edges with guest pixels.
3. **Task 2:** `31a4e4f` — cover held and repeated input movies.

## Files Created/Modified

- `tests/fixtures/src/pad_counter.asm` — scheduled SIO polling, edge counting, DMA2 drawing, and exact mailbox pixel readback.
- `runner/main.c`, `docs/runner.md` — lag reporting and the read-only peek contract.
- `tests/fixtures/tests.cmake`, `tests/fixtures/inventory.txt`, `tests/fixtures/manifest.txt`, `tests/diag/expected.csv` — registered CLI movies, independent expected records, and fixture source pin.

## Decisions Made

- Kept lag accounting in the runner and reused the core's existing `schway_run_result.lag` value.
- Made the guest poll on alternating VBlanks. This produces exactly 150 lagged frames in a 300-frame run, so per-frame mailbox peeks run alongside both polled and unpolled guest frames.
- Added a last-counter-pixel readback in addition to pixel zero, so a three-edge run checks the final drawn pixel as well as the exact count.

## Deviations from Plan

None — the fixture, runner output, movie cases, documentation, and verification all stayed within the planned input/pixel/lag slice.

## Issues Encountered

- The first guest draft waited on a VBlank IRQ handler that was not entered. Polling I_STAT made guest progress visible; the fixture now acknowledges VBlank with the core's write-zero behavior.
- Initial CMake selector expectations used the wrong hash relation and runner error wording. The assertions were corrected to the empty-versus-press behavior and the CLI's bounded line-numbered refusal; the focused selector then passed.

## TDD Gate Compliance

- **RED:** The registered `fixtures.pad_counter.press60` test failed before implementation because the runner emitted no lag summary. `gsd_run check tdd-red-evidence build/ci/pad-counter-red.json --raw` returned `RED_EVIDENCE_OK`; the captured target CTest exited 8 with the expected missing-summary assertion.
- **GREEN:** Commit `913ec9a` added lag output and the actual guest SIO/GPU fixture. The focused guest test passed before task 2 and again with the final implementation.
- **Task 2:** Expanded the proved fixture with exact edge, hold, empty, pixel-endpoint, ordering, and refusal assertions; no second RED/GREEN production cycle was needed for this regression expansion.

## Evidence

- `cmake --preset ci && cmake --build --preset ci --target schwaystation-run schwaystation-fixtures schwaystation-test-boot && ctest --test-dir build/ci -R '^fixtures\.pad_counter\.(press60|movies)$' --no-tests=error --output-on-failure` — exit 0; both registered guest cases passed.
- `cmake --workflow --preset ci` — exit 0; all 454 CI tests passed and the package/archive checks passed.
- `cmake --workflow --preset hygiene` — exit 0; all 23 hygiene and policy checks passed.
- `gsd_run check tdd-red-evidence build/ci/pad-counter-red.json --raw` — exit 0; the pre-implementation target failure was classified as valid RED evidence.
- `LC_ALL=C sort -c tests/fixtures/inventory.txt` — exit 0; the fixture inventory is byte-sorted.
- `sha256sum tests/fixtures/src/pad_counter.asm` — `8520b66e145ae33ae44b7a30c0c4886adc97f32a1afe7cce90032f107ffb3057`, matching its manifest row.

## Issues for Later Plans

GAME-03 remains pending because it covers the full SIO/libretro/fuzz integration; this plan completes the guest input/pixel/lag portion only. Runner card persistence and its fresh-process guest proof remain in plans 04-19 and 04-37.

## User Setup Required

None — all checks ran locally and headlessly.

## Next Phase Readiness

Plan 04-36 is complete. The next ready plans retain the separate runner card persistence and BIOS work; no user action or owner UAT is needed for this slice.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-11*

## Self-Check: PASSED

- Summary file exists.
- Task commits `b7440e9`, `913ec9a`, and `31a4e4f` are ancestors of HEAD.
- `gsd_run verify-summary` passes with no missing file or commit claims.
