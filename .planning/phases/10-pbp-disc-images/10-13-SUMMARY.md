---
phase: 10-pbp-disc-images
plan: "13"
subsystem: media
tags: [PBP, PSMF, CI, WASI, verification]
requires:
  - phase: 10-pbp-disc-images
    provides: bounded PBP provider, runner and libretro integrations
provides:
  - Clean Phase 10 source review with the Nightly fixture dependency resolved
  - Passing final macOS arm64 local workflow, including release package checks
affects: [11-ecm-clonecd]
actuals:
  tokens: 4413
  tasks: 2
  commits: 1
commits: 1
plan_head_before: cc46618447594d350c6b4c86d076b71b9650c40f
plan_head_after: e9ce168
tech-stack:
  added: []
  patterns:
    - Cheap inventory, wiring and budget checks before the full workflow
key-files:
  created:
    - .planning/phases/10-pbp-disc-images/10-VERIFICATION-EVIDENCE.md
    - .planning/phases/10-pbp-disc-images/10-CI-EFFICIENCY.md
  modified:
    - tests/compat/tests.cmake
    - .planning/phases/10-pbp-disc-images/10-REVIEW.md
key-decisions:
  - Keep the 3,901-line cap and defer the generic SPDX scanner.
  - Keep quality.json at 859,379 bytes until the linked Phase 10 WASI artifact is measured.
  - Run exact-head hosted CI and Nightly once at the ship PR to avoid a duplicate full workload.
requirements-completed: []
coverage:
  - id: D1
    description: Malformed PBP probe reports invalid metadata without leaking private input, and the local full workflow passes.
    requirement: PBP-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci --no-tests=error -R '^compat[.]probe[.]malformed$' --output-on-failure"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci (655 tests and 7 release checks)"
        status: pass
    human_judgment: false
  - id: D2
    description: Exact-head hosted CI, Nightly, hash/fuzz evidence and linked WASI size.
    requirement: PBP-01
    verification:
      - kind: other
        ref: "Pending the single exact-head PR CI and Nightly run at $gsd-ship 10."
        status: unknown
    human_judgment: false
duration: 38m
completed: 2026-11-21
status: complete
---

# Phase 10 Plan 13: Review and final verification summary

**The Phase 10 review is clean, malformed-PBP refusal preserves input redaction, and the complete local macOS workflow passes.**

## Performance

- **Duration:** About 38 minutes from the prior saved state checkpoint; active time was not measured.
- **Started:** 2026-11-21T15:15:19Z (prior saved state checkpoint)
- **Completed:** 2026-11-21T15:53:05Z
- **Tasks:** 2 plan tasks; the exact-head hosted run is scheduled with phase ship.
- **Files modified:** 7, including the phase state and roadmap.

## Accomplishments

- Resolved the reviewed Nightly PBP fixture dependency warning in commit `76bdc68`; recorded its clean disposition in `cc46618`.
- Fixed the compatibility probe to expect malformed PBP metadata rejection while retaining the no-stdout and path-redaction checks. Focused review and the focused selector passed.
- Ran the full local CMake CI workflow on the corrected content: all 655 regular tests and all 7 release checks passed in 78 seconds. The build step was incremental.
- Kept the repository gate at 3,900/3,901 lines. The WASI size bound remains unchanged because this Mac has no WASI sysroot or linked module.

## Task Commits

1. **Task 1: Review the phase before final local verification** — `76bdc68` (workflow fix), `cc46618` (review disposition).
2. **Task 2: Correct the malformed-PBP redaction probe** — `e9ce168` (test-only fix).

The plan ledger recorded one commit after `cc46618`; the two earlier Phase 10 review commits were already present at this executor's start.

**Plan metadata:** separate documentation/state commit follows this summary.

## Files Created/Modified

- `tests/compat/tests.cmake` — expects supported PBP parsing to reject the malformed fixture as invalid metadata, without disclosing its path.
- `10-REVIEW.md` — records the follow-up test review and verification.
- `10-VERIFICATION-EVIDENCE.md` — local evidence and the exact-hosted run boundary.
- `10-CI-EFFICIENCY.md` — measured local critical path, historical hosted baseline, and efficiency disposition.

## Decisions Made

- Keep the 3,901-line ceiling and defer the generic SPDX scanner, as the owner selected.
- Leave the WASI bound at the prior valid 859,379-byte CHD-enabled producer measurement until the Phase 10 hosted job measures the actual linked PBP-enabled module.
- Avoid a standalone `workflow_dispatch` run. The required exact-head CI and Nightly workloads will run once through the ship PR after phase verification, avoiding duplicate Actions cost.

## Deviations from Plan

### Auto-fixed issues

**1. [Rule 1 - Bug] Updated a stale malformed-PBP test expectation.**
- **Found during:** Task 2.
- **Issue:** The compatibility probe still expected all `.pbp` files to be unsupported after this phase added PBP parsing.
- **Fix:** Require the existing invalid-metadata diagnostic and retain stdout/path redaction assertions.
- **Files modified:** `tests/compat/tests.cmake`.
- **Verification:** Focused selector passed 2/2; the corrected full workflow passed 655/655 plus 7/7 release checks.
- **Committed in:** `e9ce168`.

**2. [Plan sequencing] Defer exact-head hosted measurements to the single ship PR run.**
- **Issue:** Starting a standalone hosted CI and Nightly run now would duplicate the required ship-PR workloads. Phase rules put ship after formal verification.
- **Disposition:** Record all local evidence now; Phase 10's exact-head CI/Nightly run, hosted lane inventory, Actions timing and linked WASI size remain explicit pending fields in the evidence and coverage metadata. Parent orchestration will inspect these runs at ship and update the evidence before release completion.

**Total deviations:** 2 (one test expectation fix; one hosted-run sequencing adjustment).
**Impact on plan:** No acceptance cases were removed. PBP-01 remains pending until the hosted exact-head requirements are established.

## Issues Encountered

The first broad local workflow run failed only because the malformed PBP compatibility test had a pre-feature expectation. After the focused correction and review, the one final full local workflow passed. The host cannot build the WASI target; `quality.json` was not changed. Exact-head hosted evidence remains pending the ship PR.

## User Setup Required

None. No owner-only verification is required.

## Self-Check: PASSED

- Summary, review addendum, verification evidence and CI-efficiency evidence exist.
- Task commit `e9ce168` is present on the phase branch.
- Exact-head hosted CI/Nightly and the linked WASI size remain explicitly pending for ship.

## Next Phase Readiness

Ready for formal Phase 10 verification. At ship, run and inspect one exact-head CI and Nightly attempt, extract the actual linked WASI module size from the hosted job log, and update the evidence before confirming release. No user action is needed.

---
*Phase: 10-pbp-disc-images*
*Completed: 2026-11-21*
