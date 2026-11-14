---
phase: 05-the-sound-matches-the-console
plan: "24"
subsystem: repository check helpers
tags:
  - CMake
  - workflow-policy
  - check-budget
requires:
  - phase: 05-the-sound-matches-the-console
    provides:
      - Plan 05-15 audio hash and workflow registration checks
provides:
  - Shared CMake helpers for repeated compatibility, inventory, workflow and hash checks
  - Phase 5 check coverage within the fixed 2688-line repository-check ceiling
affects:
  - Phase 5 final CI and hygiene verification
actuals:
  tasks: 1
  commits: 1
tech-stack:
  added: []
  patterns:
    - Keep caller scope for regex captures used by helper macros.
    - Reuse ordered job and inventory matchers while retaining job-scoped assertions and diagnostics.
key-files:
  created:
    - .planning/phases/05-the-sound-matches-the-console/05-24-SUMMARY.md
  modified:
    - tests/cmake/compat-controls.cmake
    - tests/cmake/hashes.cmake
    - tests/cmake/info_claims_self_test.cmake
    - tests/cmake/workflows.cmake
key-decisions:
  - Preserve all existing check assertions and the exact line ceiling while removing repeated helper logic.
  - Keep workflow matching sensitive to sorted inventory and YAML job argument order.
requirements-completed: []
coverage:
  - id: P24-BUDGET
    description: The existing counted CMake and shell checks remain at the immutable repository-check limit.
    requirement: SOUND-01
    verification:
      kind: other
      ref: "cmake -DROOT=. -DLIMIT=2688 '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -P tests/cmake/line_budget.cmake (exit 0; 2688 lines)"
      status: pass
    human_judgment: false
  - id: P24-CONTROLS
    description: Compatibility, workflow registration, lane hash, timeout and pinned action controls retain their behavior after helper reuse.
    requirement: SOUND-02
    verification:
      kind: unit
      ref: "Focused compatibility/info self-tests (3/3), runner.hashes_file (2/2 including fixture setup), hygiene policy self-tests (4/4), timeout and pin scans (exit 0)"
      status: pass
    human_judgment: false
duration: "about 30 minutes wall; exact active time unavailable"
completed: 2026-11-14
status: complete
---

# Phase 05 Plan 24: Counted CMake helper reuse Summary

**Repeated check logic is shared without reducing existing assertions, and the counted files meet the 2688-line ceiling.**

## Performance

- **Duration:** About 30 minutes wall; exact active time unavailable.
- **Tasks:** 1.
- **Files modified:** 4 CMake helpers.
- **Runtime/model:** Codex, OpenGSD 1.21.0; GPT-6 session (exact model ID not exposed); configured executor profile gpt-6-luna xhigh.

## Accomplishments

- Shared compatibility row-writing, inventory-row removal, ordered workflow job matching and hash runner capture logic.
- Kept the checks sensitive to workflow argument order, sorted inventory order and basename diagnostics.
- Reduced the counted checks to exactly 2688 lines with all listed controls passing.

## Checks Run

- `cmake -DROOT=. -DLIMIT=2688 '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -P tests/cmake/line_budget.cmake` — exit 0; 2688 lines.
- `cmake -DROOT=. -DCASE=spu_registration -P tests/cmake/workflows.cmake` — exit 0.
- `cmake -DMODE=timeouts -DDIR=.github/workflows -P tests/cmake/workflows.cmake` — exit 0; 24 timeout entries.
- `cmake -DMODE=pins -DDIR=.github/workflows -P tests/cmake/workflows.cmake` — exit 0; 50 pinned action entries.
- Focused compatibility/info, runner hash and hygiene policy self-tests passed (3/3, 2/2, 4/4 respectively).

## Deviations

- Workflow check refactoring surfaced two ordering assumptions: sorted inventory order and YAML argument order. Both were corrected, and diagnostics were restored to the expected workflow basename before final review.

## Next Phase Readiness

- Plan 05-24 is complete and precedes Plan 05-15 in wave order. Phase verification passed; actual hosted PR execution is a ship-time check.

---
*Phase: 05-the-sound-matches-the-console*
*Completed: 2026-11-14*
