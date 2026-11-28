---
phase: 17-a-measured-and-trustworthy-playability-baseline
plan: "07"
subsystem: quality-evidence
tags: [compatibility-ledger, fixture-hash, cmake, measured-evidence]
requires:
  - phase: 17
    provides:
      - Plan 06's selected gpu_motion frame-120 predicate and pinned expectation
provides:
  - Fresh live compatibility evidence for gpu_motion frames 60 and 120
  - Generated compatibility documentation with a distinct frame-120 result row
affects: [17-08, 17-09]
actuals:
  tokens: 3485
  tasks: 2
  commits: 3
  plan_head_before: e844b7c948e1229aeddb36ec7428f4a441c70cb7
  plan_head_after: 4670303b3cd2e66faed0427dc9831a14a0823b27
tech-stack:
  added: []
  patterns: [predicate-selected fixture measurements, protected evidence controls]
key-files:
  created:
    - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-IMPROVEMENT-EVIDENCE.md
    - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-07-SUMMARY.md
  modified:
    - tests/compat/tests.cmake
    - tests/compat/ledger.csv
    - docs/compatibility.md
key-decisions:
  - Only the selected compatibility-breadth candidate was implemented.
  - The existing frame-60 serial and all other ledger rows remain unchanged; frame 120 uses its own serial.
requirements-completed: [TUNE-09]
coverage:
  - id: D1
    description: The compatibility report carries the existing frame-60 row and a freshly measured pinned frame-120 row for gpu_motion.
    requirement: TUNE-09
    verification:
      - kind: integration
        ref: "ctest frame selector (3/3), compatibility selector (9/9), scripts/compat-doc.sh, and policy.compat_doc_fresh"
        status: pass
    human_judgment: false
  - id: D2
    description: Protected controls accept the exact frame-120 result and reject a wrong or missing later-frame result; unsupported frame predicates are rejected.
    requirement: TUNE-09
    verification:
      - kind: integration
        ref: "compat.ratchet.protected_controls and task-local compat_live frame@90 negative control"
        status: pass
    human_judgment: false
metrics:
  duration: 18min
  completed: 2026-11-27
  status: complete
  commits: 3
  plan_head_before: e844b7c948e1229aeddb36ec7428f4a441c70cb7
  plan_head_after: 4670303b3cd2e66faed0427dc9831a14a0823b27
---

# Phase 17 Plan 07: Fresh Frame-120 Compatibility Evidence Summary

**The live compatibility ledger now records the pinned gpu_motion frame-120 hash alongside its unchanged frame-60 result.**

## Performance

- **Runtime/model:** Codex with gpt-6-luna; OpenGSD 1.23.0.
- **Duration:** 18 minutes active.
- **Started:** 2026-11-28T04:02:23Z.
- **Completed:** 2026-11-28T04:20:15Z.
- **Tasks:** 2/2.
- **Files modified:** 5.

## Accomplishments

- Reproduced the pre-change evidence gap: the frame-120 fixture predicate passed independently while the live compatibility output still contained only frame 60.
- Updated `compat_live` to measure and cache both supported gpu_motion frames, select the requested frame from each row, and reject unsupported or mismatched predicates.
- Added exactly one `fixture/gpu_motion-frame120` ledger row with the existing pinned hash. The original frame-60 result and all other rows are unchanged.
- Regenerated `docs/compatibility.md`; it now reports four NTSC-U executable evidence rows.
- Extended protected judge controls to accept the exact frame-120 result and reject wrong-hash and missing-result cases.

## Task Commits

1. **Task 1: Implement only the evidence-selected bounded candidate** — `6082109` (`feat`).
2. **Task 2: Record the selected outcome and focused proof** — `4482560` (`docs`).
3. **Task 2 evidence detail follow-up** — `4670303` (`docs`), records the unsupported-frame negative-control command and the expected pre-fix CTest exit status.

The measured plan base is `e844b7c948e1229aeddb36ec7428f4a441c70cb7`. The three plan commits ended at `4670303b3cd2e66faed0427dc9831a14a0823b27`; `actuals.commits` is measured from that base.

## Files Created/Modified

- [tests.cmake](../../../tests/compat/tests.cmake) — measures frame 60 and 120, chooses the matching live result, and protects wrong/missing frame-120 behavior.
- [ledger.csv](../../../tests/compat/ledger.csv) — adds the measured frame-120 row without changing existing results.
- [compatibility.md](../../../docs/compatibility.md) — generated view of the expanded evidence.
- [17-IMPROVEMENT-EVIDENCE.md](17-IMPROVEMENT-EVIDENCE.md) — exact fixture identity, source revision, commands, statuses, measured hash and claim limit.

## Verification

- Before editing, `schwaystation-run`, `schwaystation-fixtures` and `schwaystation-test-compat` built successfully. The registered frame-120 fixture passed, and the live compatibility test still emitted only the frame-60 row.
- With the new row and before producer support, the live compatibility selector returned nonzero and named the missing frame-120 producer, as expected.
- The post-change fixture selector passed 3/3: frame 60, frame 120 and the wrong-hash control.
- The post-change compatibility selector passed 9/9: fixture setup, live ledger rows, ratchet transitions, protected wrong/missing controls and documentation freshness.
- Direct runner measurement returned `f92191242105428b4345519f237979aaf0b50b33431ce945f209f03b1d47257d` for frame 120.
- `scripts/compat-doc.sh`, quality checks, documentation byte budgets, `git diff --check origin/main`, and the 4,694-line repository-check budget all passed. The counted repository-check total remains exactly 4,694.
- No hosted run was needed for this selected test/ledger/report-only improvement. Full commands and statuses are in the evidence report.

## Decisions Made

- Compatibility breadth was the fixed Plan 06 selection, so no other candidate branch was changed.
- The new row uses the existing frame-120 fixture predicate and a distinct serial to preserve ledger key uniqueness. The row supports only the named fixture, region, and frame.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Restored protected-control fixture rows after the new frame-120 cases**
- **Found during:** Task 1.
- **Issue:** The new test cases reused the shared candidate/fresh files, so the existing deleted-helper control no longer exercised its original frame-60 regression input.
- **Fix:** Restored the original candidate and fresh control rows before the existing helper-deletion case.
- **Files modified:** `tests/compat/tests.cmake`.
- **Verification:** The protected-controls selector passed after the correction; the final compatibility selector passed 9/9.
- **Committed in:** `6082109`.

**Total deviations:** 1 auto-fixed (Rule 1). The correction stayed within the selected test-control file.

## Issues Encountered

The compatibility documentation generator requires explicit ledger and output arguments. The first invocation omitted them and exited 2; the documented form then succeeded: `sh scripts/compat-doc.sh tests/compat/ledger.csv docs/compatibility.md` (exit 0).

## User Setup Required

None.

## Known Stubs

None.

## Next Phase Readiness

Plan 17-07's selected compatibility evidence is complete. The frame-120 result is not a general playability claim. Plans 17-08 and 17-09 and the orchestrator's final Phase 17 review and verification remain outside this plan. No user action is required for this plan.

---
*Phase: 17-a-measured-and-trustworthy-playability-baseline*
*Completed: 2026-11-27*

## Self-Check: PASSED

- Evidence report and summary exist and are nonempty.
- Task commits `6082109`, `4482560`, and `4670303` are ancestors of the current branch head.
- The persisted Plan 17-07 ledger measures three task commits from `e844b7c948e1229aeddb36ec7428f4a441c70cb7` through `4670303b3cd2e66faed0427dc9831a14a0823b27`.
