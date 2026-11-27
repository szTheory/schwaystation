---
phase: 11-ecm-and-clonecd-images
plan: "02"
subsystem: ci-routing
tags: [nightly, budget, planning]

requires:
  - phase: 11-01
    provides: Existing classifier controls and the measured 3,901-line cap.
provides:
  - Evidence-based deferral of the proposed generated-metadata Nightly shortcut.
  - Preserved current routing, required coverage and Main/Release separation.
affects: [11-12, TODO-051]

actuals:
  tasks: 1
  commits: 1
plan_head_before: dc38fc38a2bf5ac409410943c7e63832f3d5775e
plan_head_after: 106a1edf3cc2315196818e6fe5700993051d1639

tech-stack:
  added: []
  patterns:
    - Keep the existing full-Nightly fallback when a safe classifier exception cannot fit the fixed gate budget.

key-files:
  created: []
  modified:
    - .planning/phases/11-ecm-and-clonecd-images/11-CONTEXT.md
    - .planning/phases/11-ecm-and-clonecd-images/11-DISCUSSION-LOG.md
    - .planning/todos/pending/051-nightly-pr-job-routing.md
    - .planning/phases/11-ecm-and-clonecd-images/11-12-PLAN.md

key-decisions:
  - D-12 supersedes only the D-08 shortcut proposal: retain the 3,901-line cap and defer the metadata-only Nightly route until all controls fit without loss.
  - Do not remove assertions, broaden filters, or change Main, Release, scheduled or manual coverage to save lines.

requirements-completed: []
coverage:
  - id: D08-DEFERRED
    description: The proposed metadata-only shortcut is deferred; current classifier behavior is unchanged and the focused route control remains green.
    requirement: "D-08 decision (not a product requirement)"
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^ci[.]classify_policy$' --output-on-failure"
        status: pass
    human_judgment: false

duration: about27min
completed: 2026-11-22
status: complete
---

# Phase 11 Plan 02: Nightly shortcut budget decision

**The proposed metadata-only Nightly shortcut is deferred because its complete controls do not fit the approved gate cap; the current fail-closed routing remains unchanged.**

## Performance

- **Duration:** About 27 minutes from budget-audit kickoff through closeout; exact start time was not recorded.
- **Runtime/model:** Codex primary model identifier unavailable; OpenGSD 1.20.0. The two read-only budget audits and plan checker used `gpt-6-luna` xhigh.
- **Tasks:** 1 documentation and verification task.
- **Bottleneck:** Independent budget review and planning updates. The cap check completed immediately; the focused CTest selector passed in 0.15 seconds.

## Evidence and decision

- The current counted repository checks are 3,898/3,901 lines. The proposed route-control tests alone add 30 lines, reaching 3,928 before classifier or workflow-boundary changes.
- Two independent read-only audits found no safe, measured consolidation that would save the required 27 lines while preserving all existing checks.
- D-12 defers only the generated-release-metadata Nightly shortcut to existing pending todo 051. It does not raise the cap, remove cases, change any CI lane, or complete ECM-01/CCD-01.
- Main CI, Release CI, scheduled/manual Nightly and the existing PR fallback remain unchanged. No hosted CI or `act` run was needed because no workflow code changed.

## Checks

| Command | Exit | Result |
| --- | ---: | --- |
| GSD plan-frontmatter validation | 0 | Plan structure valid with one executable task |
| `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` | 0 | 3898/3901 counted lines |
| `ctest --preset ci --no-tests=error -R '^ci[.]classify_policy$' --output-on-failure` | 0 | Current classifier policy passed 1/1 |
| `git diff --check` | 0 | No whitespace errors |
| node "$HOME/.codex/gsd-core/bin/gsd-tools.cjs" verify-summary .planning/phases/11-ecm-and-clonecd-images/11-02-SUMMARY.md | 0 | Summary file, commit and self-check validated |

No classifier, workflow, test or product source changed. The test pass verifies the retained route controls, not the deferred shortcut. No hosted CI was started.

## Next Phase Readiness

Plan 02 is closed as an evidence-based scope decision. Plan 05 is ready for modeled ECM records; Plan 06 follows the completed Plan 13 companion-confinement work. The exact Nightly shortcut remains pending in todo 051, which requires a measured no-quality-loss route under the existing cap before it is reconsidered.

## Self-Check: PASSED

- Plan 11-02 is valid and its evidence-based deferral is recorded in D-12 and todo 051.
- The current line-budget check and classifier selector passed; no workflow or product code changed.
- `requirements-completed` remains empty, and the status of ECM-01 and CCD-01 is unchanged.

---
*Phase: 11-ecm-and-clonecd-images*
*Completed: 2026-11-22*
