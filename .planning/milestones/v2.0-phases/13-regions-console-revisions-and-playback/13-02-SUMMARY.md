---
phase: 13-regions-console-revisions-and-playback
plan: 02
subsystem: testing
tags: [mdec, ctest, nightly, cmake, c]
requires:
  - phase: 13-regions-console-revisions-and-playback
    provides: "Pinned MDEC reference fixtures and bounded color guest runner"
provides:
  - "Fixture-gated movie15 first-frame diagnostic with fail-closed expected-incomplete classification"
affects: [Phase 13 MDEC verification]
actuals:
  tokens: 1958
  tasks: 1
  commits: 2
  plan_head_before: 2e159bab7c4baa3d0484c30621fcff7cbf91f94e
  plan_head_after: 4ab751cd79a18b01f0d94a1be9af8f58abffc6dc
tech-stack:
  added: []
  patterns:
    - "CTest fixture gates source and color reference checks before a bounded guest diagnostic"
key-files:
  created: []
  modified:
    - cmake/MDECReferences.cmake
    - tests/mdec/color_reference.c
    - tests/nightly/inventory.txt
key-decisions:
  - "Reuse the existing movie15 guest path and accept only its measured exact incomplete signature as a diagnostic result."
  - "A completed guest still has to pass both raw VRAM and displayed-scene comparisons."
patterns-established:
  - "Expected-incomplete guest outcomes name source, decoder, guest, and display status separately and never claim a frame pass."
requirements-completed: []
coverage:
  - id: D1
    description: "MDEC source and asset preflights gate a bounded movie15 diagnostic that reports the exact expected-incomplete outcome without a playback or frame-conformance claim."
    requirement: MDEC-01
    verification:
      - kind: integration
        ref: "ctest --preset nightly --no-tests=error -R '^mdec[.]reference[.](input_preflight|edge)$' --output-on-failure"
        status: pass
    human_judgment: false
metrics:
  duration: "60m (wall interval from saved state checkpoint; executor start not separately captured)"
  completed: 2026-11-23
  status: complete
---

# Phase 13 Plan 02: Register the Bounded Movie15 Diagnostic Summary

**A fixture-gated movie15 first-frame check records the exact bounded MDEC input-exhaustion state without claiming playback or frame conformance.**

## Performance

- **Runtime/model:** Codex GSD executor; configured `gpt-6-luna` xhigh; primary runtime model ID unavailable.
- **Duration:** 60 minutes of wall time from the saved STATE checkpoint at 2026-11-23T15:11:48Z through the 2026-11-23T16:11:52Z closeout checkpoint; executor start and active time were not separately captured.
- **Completed:** 2026-11-23T16:11:52Z.
- **Tasks:** 1
- **Files modified:** 3 implementation files

## Accomplishments

- Registered `mdec.reference.edge` through the existing bounded movie15 guest path and required both source preflight and color asset fixture setup.
- Added a fail-closed classifier requiring complete pinned source coverage, eight successful empty-FIFO samples, no fault or API/TTY error, a below-cap run, and the exact observed MDEC/DMA terminal signature.
- Preserved the `4,915,200` cycle cap and expected images. A completed guest still goes through both raw VRAM and display comparisons.
- The corrected selector reported source complete (1,440 blocks, 240 macroblocks, zero trailing blocks), decoder FIFO empty, guest expected-incomplete, display not compared, and no fault at 1,902,230 ticks. This is diagnostic evidence only; no decoded-frame, playback, or hardware-conformance claim is made.

## Task Commits

1. **Task 1 RED: register the bounded edge selector** — `77ae668` (`test(13-02): register bounded movie15 edge diagnostic`)
2. **Task 1 GREEN: classify the exact incomplete result** — `4ab751c` (`fix(13-02): classify expected incomplete movie15 outcome`)

## Files Modified

- `cmake/MDECReferences.cmake` — registers the edge selector with source and asset fixtures.
- `tests/mdec/color_reference.c` — classifies only the recorded movie15 incomplete signature and reports independent outcomes.
- `tests/nightly/inventory.txt` — adds the byte-sorted Nightly selector row.

## Verification Evidence

- Initial unsupported CI selector probe: `cmake --preset ci` configured successfully in 0.9 seconds, but exposed 0/4 MDEC selectors. `cmake --preset nightly -DSCHWAYSTATION_MEDIA_REFERENCE_ONLY=mdec` found the existing selector graph in 1.8 seconds; after registration, the same configure command took 1.3 seconds and the CTest JSON show-only assertion found only `mdec.reference.input_preflight`, `mdec.reference.color_assets`, and `mdec.reference.edge`. The focused target build passed in 0.4 seconds.
- RED: `ctest --preset nightly --no-tests=error -R '^mdec[.]reference[.](input_preflight|edge)$' --output-on-failure` exited 8 in 0.07 seconds. The source and asset fixtures passed. The guest emitted `MDEC_COLOR_INPUT_EXHAUSTED: movie15 guest waits after a 1696-word DMA0 command exhausted; the MDEC output FIFO is empty`, then `MDEC_COLOR_GUEST_ERROR: movie15 did not finish (input exhausted while waiting for decoded pixels; ticks=1902230)`. State was `pc=80010918 a0=1f800000 v0=00000001 ra=800108ec mdec=380006a0 active=0 pending=0 in=0 rem=0 out=0 dma0=00000201/00013a80/00350020 dma1=01000200/001da914/003c0020 dma2=00000201/001dad94/00780010`; progress was `dreq=04 dma1_words=0 dma1_remain=9 dma1_phase=2 dma2_words=0`. No fault occurred; the run stopped below the cap.
- Corrected preflights: `env LC_ALL=C sort -cu tests/nightly/inventory.txt` exited 0 in 0.00 seconds; `cmake -DROOT=. -DLIMIT=3901 '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -P tests/cmake/line_budget.cmake` exited 0 at 3,901 lines in 0.00 seconds; `cmake --preset nightly -DSCHWAYSTATION_MEDIA_REFERENCE_ONLY=mdec` exited 0 in 1.17 seconds; `ctest --preset nightly --show-only=json-v1 -R '^mdec[.]reference[.](input_preflight|edge)$'` plus the exact-set JSON assertion exited 0 in 0.00 seconds with 3/3 expected selectors.
- Focused build: `cmake --build --preset nightly --target schwaystation-test-mdec-reference` exited 0 in 0.12 seconds.
- Corrected acceptance: the exact CTest command above exited 0 in 0.06 seconds, with source preflight 0.02 seconds, asset setup 0.03 seconds, and edge 0.03 seconds. `LastTest.log` records `source=complete blocks=1440 macroblocks=240 decoder=output-fifo-empty guest=expected-incomplete display=not-compared ticks=1902230 cycle_cap=4915200 dma0=idle dma1=waiting dma1_remain=9 dma1_phase=2 fault=none`.
- An initial timed inventory command was malformed (`time` treated `LC_ALL=C` as its executable), exiting 127 in 0.00 seconds; rerunning through `env` passed. No further test run was needed.
- The pinned source preflight and asset fixture passed before guest execution. No full unaffected 4-bit/8-bit suite, hosted CI, Act job, or Docker resource was run; workflow wiring did not change.

## Decisions Made

- Kept the accepted MDEC inputs, output references, and cycle cap unchanged; the observed incomplete movie state is a named diagnostic result rather than success for playback or frame conformance.

## Deviations from Plan

The initial plan route was corrected from the CI preset to the supported MDEC-only Nightly preset after the selector probe found no CI entries. The edge test reused the existing runner as planned; no implementation scope was added beyond the checked plan amendment.

## Issues Encountered

- The first registered test exposed the known movie15 empty-FIFO state. The implementation now accepts only its exact observed signature after eight consecutive successful run/TTY samples and fails closed for other incomplete or faulted states.
- A shell timing wrapper error was corrected without changing the check or product code.

## User Setup Required

None.

## Next Phase Readiness

Plan 13-02 is complete. Plan 13-03 is next. The movie15 first-frame path remains explicitly expected-incomplete at the observed state; it makes no complete movie, decoded-frame, display, or hardware-conformance claim.

---
*Phase: 13-regions-console-revisions-and-playback*
*Completed: 2026-11-23*

## Self-Check: PASSED

The summary exists, and task commits `77ae668` and `4ab751c` are ancestors of the current branch head.
