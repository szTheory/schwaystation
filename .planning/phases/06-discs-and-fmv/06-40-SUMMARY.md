---
phase: 06-discs-and-fmv
plan: "40"
subsystem: testing
tags: [mdec, acceptance, evidence, documentation]
requires: []
provides:
  - "Owner-approved v1.0 MDEC acceptance scope recorded across requirements, decisions, roadmap, context and hardware/testing docs."
  - "Known hardware mismatches, incomplete input counts and status-budget limit recorded without claiming decoder passes."
affects: ["Phase 6 successor plans 06-41 through 06-45", MDEC, docs/testing]
actuals:
  tokens: 5858
  tasks: 2
  commits: 2
commits: 2
plan_head_before: df6b11d1f3be8652e84caaa2c49ea85d5cd8c2a3
plan_head_after: b6fd4aa32e2b65ed85afeab74e96bf177c2acc88
tech-stack:
  added: []
  patterns:
    - "Use scoped documentation checks before expensive suites."
    - "Keep hardware observations separate from implementation regressions."
key-files:
  created:
    - .planning/phases/06-discs-and-fmv/06-40-SUMMARY.md
  modified:
    - .planning/REQUIREMENTS.md
    - .planning/preparation/DECISIONS.md
    - .planning/ROADMAP.md
    - .planning/phases/06-discs-and-fmv/06-CONTEXT.md
    - .planning/preparation/PSX-SPU-CDROM-AND-MDEC.md
    - docs/hardware/mdec.md
    - docs/testing.md
key-decisions:
  - "Preserve DEC.33 arithmetic, tables, cycle baseline, raw-before-DMA1 hash and hardware bytes while narrowing only unsupported v1.0 acceptance claims."
  - "Keep DISC-03 and 06-23/24 acceptance pending until successor local and hosted gates are complete."
patterns-established:
  - "MDEC hardware bytes remain evidence; implementation-only baselines are named separately."
  - "Incomplete or over-budget assets are classified before emulation and never counted as decoder passes."
requirements-completed: []
coverage:
  - id: D1
    description: "The approved MDEC acceptance claim and evidence limits are consistent across requirements, decisions, context, roadmap, hardware and testing documentation; DISC-03 remains explicitly pending."
    requirement: DISC-03
    verification:
      - kind: other
        ref: "git diff --check origin/main -- .planning/REQUIREMENTS.md .planning/preparation/DECISIONS.md .planning/ROADMAP.md .planning/phases/06-discs-and-fmv/06-CONTEXT.md; git diff --check origin/main -- .planning/preparation/PSX-SPU-CDROM-AND-MDEC.md docs/hardware/mdec.md docs/testing.md"
        status: pass
      - kind: other
        ref: "Agent cross-check of approved scope, original hardware bytes, 16-pixel discrepancy, status cap, incomplete source counts, and successor selector names"
        status: pass
    human_judgment: false
  - id: D2
    description: "Pending 06-23/24 acceptance is assigned to 06-44/45; future selector registration is assigned to 06-43 and is not described as already passing."
    requirement: DISC-03
    verification:
      - kind: other
        ref: "Scoped documentation diff review against 06-40-PLAN.md"
        status: pass
    human_judgment: false
duration: 19min
completed: 2026-11-17
status: complete
---

# Phase 6 Plan 06-40: Record Approved MDEC Scope Summary

**The MDEC documentation now distinguishes exact 4-bit hardware evidence, an 8-bit implementation regression, direct model controls, and incomplete or over-budget inputs.**

## Performance

- **Duration:** 19m27s (start 2026-11-17T18:19:13Z; bookkeeping complete 2026-11-17T18:38:40Z)
- **Runtime/model:** Codex/OpenGSD 1.20.0; executor model gpt-6-luna
- **Tasks:** 2
- **Files modified:** 7 plan-owned docs, plus this summary

## Accomplishments

- Updated DISC-03, DEC.33's acceptance note, Phase 6 roadmap/context and hardware/testing references to reflect the owner-approved v1.0 scope.
- Preserved the pinned 4-bit hardware check and original 8-bit expected bytes; documented the 16-pixel 8-bit mismatch, 7,680,000-cycle status request against the 4,915,200 cap, and incomplete 296/300 and 162/240-plus-five-block sources.
- Kept DISC-03 and the 06-23/24 acceptance pending, with selector registration assigned to 06-43 and final local/hosted acceptance to 06-44/45.

## Verification Evidence

| Command | Exit | Runtime | Result |
|---|---:|---:|---|
| `git diff --check origin/main -- .planning/REQUIREMENTS.md .planning/preparation/DECISIONS.md .planning/ROADMAP.md .planning/phases/06-discs-and-fmv/06-CONTEXT.md` | 0 | under 0.1s | Scoped Task 1 documentation check passed. |
| `git diff --check origin/main -- .planning/preparation/PSX-SPU-CDROM-AND-MDEC.md docs/hardware/mdec.md docs/testing.md && cmake --list-presets` | 0 | under 0.1s | Scoped Task 2 check passed; available configure presets were listed. |

These were documentation consistency and preset-discovery checks, not decoder or phase acceptance tests. No hosted run URL applies. No source test suite was run by this documentation plan.

## Task Commits

1. **Task 1: Trace the approved claim through requirements, DEC.33 and phase acceptance** — `0131b31` (`docs(06-40): record approved MDEC scope`).
2. **Task 2: Align testing and hardware claims with the exact surviving evidence** — `b6fd4aa` (`docs(06-40): document MDEC evidence limits`).

The two task commits were measured from plan base `df6b11d1f3be8652e84caaa2c49ea85d5cd8c2a3` to `b6fd4aa32e2b65ed85afeab74e96bf177c2acc88` (`2` commits). Their changed-content diff measured 23,429 characters, or 5,858 tokens at chars/4.

## Decisions Made

- Preserve the selected integer transform, tables and coefficients, 1,000-cycle block baseline, 4,915,200-cycle limit, raw-before-DMA1 hash, original hardware bytes, and the complete owned `disc_boot` STR path.
- Do not claim exact 8-bit rounding or full external frame/movie hardware conformance from the current captures and incomplete assets.
- Keep DISC-03 unchecked until the successor gates and final acceptance complete; 06-23/24 remain pending rather than being relabelled as passes.

## Deviations from Plan

None. The existing Task 2 documentation diffs in its three owned files were reviewed by the parent as authorized Phase 6 MDEC/status/CI/RetroArch documentation changes and committed with those files.

## Issues Encountered

The first standard GSD task commit attempt could not create `.git/index.lock` (`Operation not permitted`). The parent completed both approved path-scoped GSD commits through the authorized sandbox escalation path; no raw Git staging or commit workaround was used.

The pre-edit repository-wide `git diff --check origin/main` failed on pre-existing unrelated whitespace. Both task-scoped checks passed. The exact failures are deferred to integration cleanup in 06-44:

- EOF blank lines: `06-01-PLAN.md:112`, `06-02-PLAN.md:108`, `06-03-PLAN.md:110`, `06-04-PLAN.md:108`, `06-05-PLAN.md:99`, `06-06-PLAN.md:111`, `06-07-PLAN.md:122`, `06-08-PLAN.md:108`, `06-09-PLAN.md:113`, `06-10-PLAN.md:107`, `06-11-PLAN.md:107`, `06-12-PLAN.md:107`, `06-13-PLAN.md:118`, `06-14-PLAN.md:126`, `06-15-PLAN.md:109`, `06-16-PLAN.md:109`, `06-17-PLAN.md:110`, `06-18-PLAN.md:107`, `06-19-PLAN.md:127`, `06-20-PLAN.md:122`, `06-21-PLAN.md:123`, `06-22-PLAN.md:105`, `06-23-PLAN.md:109`, `06-24-PLAN.md:142`, `06-25-PLAN.md:104`, `06-26-PLAN.md:108`, `06-26-SUMMARY.md:160`, and `06-36-SUMMARY.md:149` under `.planning/phases/06-discs-and-fmv/`.
- Trailing whitespace: `tests/probe/rc_hash_disc.c:565` and `tests/probe/rc_hash_disc.c:568`.

No whitespace cleanup was made outside this plan's owned files.

## Auth Gates

None.

## Next Phase Readiness

Plan 06-41 can add the separate 8-bit implementation regression while keeping the hardware bytes unchanged. Plan 06-42 adds bounded source and status-budget checks; 06-43 registers the selectors. Plans 06-44/45 still own local and hosted acceptance, including the pending 06-23/24 criteria. No owner capture is available or required, and no user action is needed at this step.

## Self-Check: PASSED

- Summary file exists at `.planning/phases/06-discs-and-fmv/06-40-SUMMARY.md`.
- Task commits `0131b31` and `b6fd4aa` are present in the current branch history.
- DISC-03 remains unchecked; 06-23/24 acceptance and 06-45 ship-only checks remain pending.

---
*Phase: 06-discs-and-fmv*
*Completed: 2026-11-17*
