---
phase: 17-a-measured-and-trustworthy-playability-baseline
plan: "03"
subsystem: quality-baselines
tags:
  - quality
  - cachegrind
  - wasi
requires:
  - phase: 17-01
    provides: Existing producer and documentation inventory
provides:
  - Exact producer evidence for current Cachegrind and serialized-state bounds
  - Current WASI size interval and explicit exact-output gap
  - Low-power and browser performance evidence limits
affects:
  - Phase 17 quality scoring and guidance refresh
  - Phase 17 timing reconciliation
actuals:
  tokens: 7900
  tasks: 2
  commits: 2
tech-stack:
  added: []
  patterns:
    - Reuse exact producer outputs only when source, configuration and workload inputs match
key-decisions:
  - "Keep all quality values and relative performance controls unchanged; current exact Cachegrind and state outputs match their bounds."
  - "Retain the 1,014,617–1,014,665-byte WASI interval because the latest matching hosted check does not expose its exact integer."
  - "Leave ci.critical_path_s and its workload fingerprint for Plan 17-09."
patterns-established:
  - "A successful interval check is not an exact measurement when the producer does not log its measured value."
requirements-completed: []
coverage:
  - id: P17-03-A
    description: "All five current quality metrics have producer configurations, exact available results, and explicit pending evidence where output is unavailable."
    requirement: TUNE-10
    verification:
      kind: other
      ref: "17-BASELINES.md; exact run and job links; source-input comparisons exited 0"
      status: pass
    human_judgment: false
  - id: P17-03-B
    description: "Bounds changed only from exact matching measurements; existing comparison and quality controls preserve integer, boundary, malformed-input, frame-window and relative-limit behavior."
    requirement: TUNE-10
    verification:
      kind: unit
      ref: "quality.cmake exact metric comparisons (exit 0); tests/cmake/quality_controls.cmake (exit 0); diff and registry checks (exit 0)"
      status: pass
    human_judgment: false
duration: 27m
completed: 2026-11-27
status: complete
---

# Phase 17 Plan 03 Summary

**Recorded current quality producers and kept limits unchanged where exact matching evidence did not justify a ratchet.**

## Performance

- **Duration:** About 27 minutes.
- **Started:** 2026-11-28T02:21:36Z (approximate).
- **Completed:** 2026-11-28T02:48:28Z.
- **Runtime/model:** Codex/OpenGSD 1.23.0; primary model ID unavailable.
- **Tasks:** 2.
- **Task commits:** 2.

## Accomplishments

- Recorded the exact hosted Cachegrind outputs: 1,620,381 Ir/frame for `testcard` and 138,406,446 for `gpu_motion`; both same-run comparisons passed at 0 basis points. Their source, fixture, runner image, compiler and Cachegrind configuration are linked in `17-BASELINES.md`.
- Reused the exact Linux x64 stable-state output of 3,757,754 bytes, including the state header, and documented its 45-frame save/load/save producer.
- Preserved the 1,014,617–1,014,665-byte WASI interval. The latest matching hosted job passed within it but did not print the linked module size or upload the module, so its exact integer remains unavailable.
- Kept `ci.critical_path_s` pending for Plan 17-09 and preserved its current fingerprint. Low-power budgets remain explicitly based on assumed sustained rates; no device or browser performance was inferred.
- Made no `quality.json` changes because no bound change was supported by a new exact producer result.

## Task Commits

1. **Task 1: Remeasure the existing producers and identify unmatched environments** — `be197e0` (`docs(17-03): record measured quality baselines`).
2. **Task 2: Ratchet only bounds supported by exact measured output** — `d516cef` (`docs(17-03): preserve only measured quality limits`).

## Verification

- Each task's required `cmake -DQUALITY=quality.json -P tests/cmake/quality.cmake && git diff --check origin/main` exited 0.
- The exact `testcard`, `gpu_motion`, and state values passed the existing comparator. The two already-recorded WASI endpoint samples each passed against the retained interval; every comparator invocation exited 0.
- Existing `tests/cmake/quality_controls.cmake` boundary, malformed-input, integer and producer-wiring controls exited 0. Its first direct invocation used `ROOT=.` and exited 1 at the script's absolute-root argument check; the corrected invocation with the repository root completed successfully.
- A read-only comparison confirmed `quality.json` still matches `origin/main` (exit 0). No builds or product test suites ran.

## Remaining evidence

The latest hosted WASI producer checks the current interval but does not expose its exact byte count. The timing row remains for Plan 17-09. Neither gap is represented as a new measurement, and TUNE-10 remains pending for phase-level reconciliation.

---
*Phase: 17-a-measured-and-trustworthy-playability-baseline*
*Plan: 03*
*Completed: 2026-11-27*
