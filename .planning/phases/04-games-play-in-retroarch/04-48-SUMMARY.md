---
phase: 04-games-play-in-retroarch
plan: "48"
subsystem: testing
tags: [gpu, ctest, cmake, github-actions, provenance]
requires:
  - phase: "04-47"
    provides: exact GPU capture references and bounded oracle adapter
provides:
  - Required hosted GPU reference lane for ten captures and three hardware logs
  - Portable and bounded GPU log predicate controls
affects: [04-31, 04-29, 04-32, phase-04-verification]
actuals:
  tokens: 21368
  tasks: 2
  commits: 4
plan_head_before: 6ee11f2265b2415c5b0c374d216914febe669819
plan_head_after: 2f839fd29819d3d32a7ade8f0348db8fc6a1b1c5
tech-stack:
  added: []
  patterns:
    - Required exact-head GPU-reference workflow job
    - Bounded portable parsing for real GPU log predicates
key-files:
  created:
    - .planning/phases/04-games-play-in-retroarch/04-REVIEW.md
    - .planning/phases/04-games-play-in-retroarch/04-REVIEW-FINGERPRINT.json
    - .planning/phases/04-games-play-in-retroarch/04-REVIEW-REMAINING.md
  modified:
    - .github/workflows/ci.yml
    - cmake/GPUReferences.cmake
    - tests/gpu/reference.c
    - docs/testing.md
    - PROVENANCE.md
    - .planning/LESSONS.md
    - .planning/AUTONOMOUS.md
key-decisions:
  - "Keep all ten real captures and three real GPU logs in a required exact-head hosted lane."
  - "Use only project-owned GPU contracts and vetted hardware references for DMA predicates; exclude the forbidden GPL helper."
patterns-established:
  - "Run real captured GPU oracles in the focused required lane, with synthetic controls remaining gating in ordinary CI."
requirements-completed: []
coverage:
  - id: D1
    description: Ten GPU captures and three ordered hardware-log predicates pass in the required hosted lane.
    requirement: GAME-01
    verification:
      - kind: integration
        ref: https://github.com/szTheory/schwaystation/actions/runs/43404686338
        status: pass
    human_judgment: false
  - id: D2
    description: GPU log judge controls are bounded and portable across the native and WASM builds.
    requirement: GAME-01
    verification:
      - kind: unit
        ref: "Focused GPU reference CTests: 13/13 passed locally; current exact-head hosted run 43404686338 also passed GPU and WASM lanes."
        status: pass
    human_judgment: false
duration: "not measured; resumed from checkpoint"
completed: 2026-11-13
status: complete
---

# Phase 4 Plan 48: Required GPU Reference Lane Summary

**All ten GPU captures and three ordered hardware logs now run in a required hosted lane with portable bounded judge controls.**

## Performance

- **Duration:** Not measured; work resumed from a checkpoint and retained timestamps do not isolate active execution time.
- **Started:** Not recorded for the resumed execution.
- **Completed:** 2026-11-13
- **Tasks:** 2
- **Files modified:** 16
- **Runtime/model:** Codex/OpenGSD 1.20.0; gpt-6-luna xhigh.

## Accomplishments

- Added a required CI GPU-reference job that runs the ten captured images and three real hardware logs on the exact head.
- Preserved ordered register/predicate assertions for GP0 E1, mask-bit and GPustat, plus gating malformed-input and refusal controls.
- Recorded the observed forbidden GPL helper exclusion in provenance and lessons; no bytes from that helper were retained or applied.
- Reconciled the phase review consolidation and retained companion report, and recorded the current repository-check ceiling and closeout efficiency practice.

## Task Commits

1. **Task 1: Judge the three real GPU hardware logs and run the required lane** — `50c927b` (`fix`)
2. **Task 2: Expand and document the proven path** — `f058387` (`chore`)
3. **Separate partial API/docs caller candidate for plan 32** — `0b13b66` (`test`; cross-plan commit inside this plan's measured interval)
4. **Windows/WASM portability and bounded-stack repair** — `2f839fd` (`fix`)

The measured plan ledger interval contains four commits. Its realized 48-file diff is 16 files; the interval also includes the separate plan32 candidate commit.

## Files Created/Modified

- `.github/workflows/ci.yml` — makes the GPU-reference lane required for relevant changes.
- `cmake/GPUReferences.cmake`, `tests/gpu/reference.c` — register and judge real reference predicates and bounded controls.
- `tests/gpu/tests.cmake`, `tests/gpu/inventory.txt`, `tests/nightly/tests.cmake`, `tests/nightly/inventory.txt` — preserve exact registration and inventory coverage.
- `.github/workflows/nightly.yml`, `THIRD-PARTY-NOTICES.md`, `docs/testing.md` — retain nightly oracle behavior, notices and maintainer instructions.
- `PROVENANCE.md`, `.planning/LESSONS.md` — record the forbidden helper exclusion and independently derived values.
- `.planning/AUTONOMOUS.md` — record the measured CI batching and wrapper-result lesson.
- `.planning/phases/04-games-play-in-retroarch/04-REVIEW.md`, `04-REVIEW-FINGERPRINT.json`, `04-REVIEW-REMAINING.md` — retain the consolidated source review and companion findings.

## Decisions Made

- Kept the GPU reference lane required and exact-head, retaining every capture and real log case.
- Used project-owned contracts and vetted hardware references for DMA predicates; did not retain or apply the forbidden helper.
- Kept the 2700-line repository-check ceiling and 300-line RetroArch harness ceiling unchanged.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed native portability and WASM stack exhaustion in GPU log controls**

- **Found during:** Task 2 exact-head hosted CI.
- **Issue:** Windows warning-as-error diagnostics rejected signed/unsigned comparisons and unbounded `sscanf`; four WASM reference-control cases exhausted the default stack when a 64 KiB local buffer was inlined.
- **Fix:** Cast pixel operands to `uint32_t`, use bounded `sscanf_s` on MSVC, and allocate/check/free the bounded 64 KiB control buffer on the heap without changing test cases or limits.
- **Files modified:** `tests/gpu/reference.c`
- **Verification:** Focused native tests and all 13 GPU reference cases passed locally; current hosted GPU, WASM and full required CI run passed.
- **Committed in:** `2f839fd`.

**Total deviations:** 1 auto-fixed issue (Rule 1). The repair was required by concrete Windows and WASM failures on the first exact-head run and stayed within the existing plan owner.

## Issues Encountered

- A temporary helper was identified as copied from GPL-2 Avocado and deleted before integration. No bytes were retained or applied; the source contract and vetted project references were used instead.
- A local full-workflow wrapper exited 1 after the shell attempted to assign zsh's read-only `status` variable. The actual CMake/CTest stages and their fresh reports were green; the wrapper failure is recorded accurately and was not used to justify repeating the broad suite.

## Evidence

- Focused local GPU reference suite: 13/13 cases passed, including ten captures and three logs.
- Affected native GPU reference controls: 7/7 passed. Local hygiene workflow: 23/23 passed, outer exit 0.
- Exact-head hosted run [43404686338](https://github.com/szTheory/schwaystation/actions/runs/43404686338) completed successfully. GPU job 131816075623 passed 13/13 and checked each case ran once; WASM passed 460/460; BE32 passed 492/492; RetroArch proved distinct exact frames 59/60 against runner references with an empty system directory; all six native builds and the remaining required lanes passed. Required aggregate CI job 131825214695 succeeded.

## User Setup Required

None.

## Next Phase Readiness

This GPU reference slice is complete. Plan 31, plan 29, plan 32 and the phase verification are being closed from the same successful exact-head run. Stop after phase verification before phase 4 ship or phase 5.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-13*

## Self-Check: PASSED

The summary file exists, and all four listed implementation commits are ancestors of the recorded plan head.
