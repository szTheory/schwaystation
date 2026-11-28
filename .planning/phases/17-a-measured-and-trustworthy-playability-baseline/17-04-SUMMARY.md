---
phase: 17-a-measured-and-trustworthy-playability-baseline
plan: "04"
subsystem: maintenance-audit
tags:
  - repository-settings
  - dependabot
  - runner-images
  - pins
requires:
  - phase: 17-01
    provides: Documentation and maintainer-surface inventory
provides:
  - Read-only repository policy, active runner, action, and consumed pin audit
  - Current Dependabot policy reconciliation and exact documentation drift dispositions
affects:
  - Phase 17 scoring and single-improvement selection
  - Phase 17 documentation refresh
actuals:
  tokens: 6600
  tasks: 2
  commits: 2
tech-stack:
  added: []
  patterns:
    - Map every active pin to a concrete consumer before assessing its update need
key-files:
  created:
    - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-UPGRADE-AUDIT.md
    - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-04-SUMMARY.md
  modified:
    - .planning/preparation/DECISIONS.md
key-decisions:
  - "Keep Dependabot auto-merge off; TODO-010 remains conditional on Plan 06's score, and TODO-011/TODO-012 remain deferred."
  - "Keep hosted runner labels and unrelated source pins unchanged; record missing immutable macOS Intel image tag as unavailable."
  - "Record Wasmtime 48.0.10 as the next LTS pin candidate under the existing security-patch policy; do not change pins in this audit plan."
patterns-established:
  - "Separate repository auto-merge capability from policy that would automatically merge Dependabot pull requests."
requirements-completed: []
coverage:
  - id: P17-04-A
    description: "The read-only settings status, active runner/action map, actual pin consumers, exact pin identities, and evidence-scoped update dispositions are recorded."
    requirement: TUNE-11
    verification:
      kind: other
      ref: "17-UPGRADE-AUDIT.md; settings check exit 0; upstream release/advisory links; task shell check and diff check exit 0"
      status: pass
    human_judgment: false
  - id: P17-04-B
    description: "DEC.94 is reconciled to the later no-auto-merge policy, Dependabot token/secret restrictions are current, and affected versus unchanged documentation paths are explicit."
    requirement: TUNE-11
    verification:
      kind: other
      ref: "17-UPGRADE-AUDIT.md and DECISIONS.md; task diff check and artifact nonempty check exit 0"
      status: pass
    human_judgment: false
duration: 21m
completed: 2026-11-27
status: complete
---

# Phase 17 Plan 04 Summary

**Recorded live maintenance state and kept update policy reviewable without speculative churn.**

## Performance

- **Duration:** About 21 minutes.
- **Started:** 2026-11-28T02:51:00Z (approximate).
- **Completed:** 2026-11-28T03:11:50Z.
- **Runtime/model:** Codex/OpenGSD 1.23.0; primary model ID unavailable.
- **Tasks:** 2.
- **Files modified:** 3 (including this summary).

## Accomplishments

- Ran the repository-settings check read-only: exit 0, no drift. Security alerts are enabled with zero open alerts; version-update configuration is absent. No secret values or repository settings were read or changed.
- Mapped every active runner label and workflow action SHA to its workflow consumer and current official source. The Intel macOS image label is active, but no current image-specific release tag was available.
- Mapped active downloaded/source pins to actual uses, including the optional CHD reader/tool, OpenBIOS, fixture builders, reference data and CPU vectors. The old Phase 7 statement that libchdr had no active consumer is corrected here: CHD-enabled CI consumes it.
- Recorded Wasmtime 48.0.10's current LTS asset digests as a candidate under the existing patch policy, without changing `pins.txt` in this audit plan.
- Reconciled DEC.94 to Phase 7's later no-auto-merge decision; kept TODO-010 conditional and TODO-011/TODO-012 deferred. Listed exact documentation paths with reproduced drift or a no-drift disposition.

## Task commits

1. **Task 1: Inspect live repository policy, active runners and consumed pins** — `bd5f06a` (`docs(17-04): audit live upgrade policy`).
2. **Task 2: Reconcile update policy and identify affected install and hardware guidance** — `746373e` (`docs(17-04): reconcile update policy`).

## Verification

- Task 1's required shell syntax and whitespace checks both exited 0.
- Task 2's required whitespace and nonempty-audit checks both exited 0.
- `sh scripts/apply-repo-settings.sh --check` — exit 0, `no drift on szTheory/schwaystation`.
- `gh api repos/szTheory/schwaystation/vulnerability-alerts --silent` — exit 0; open-alert count query — exit 0 and returned 0.
- Public upstream release/image and Wasmtime asset-digest API queries exited 0. The GitHub REST advisories route returned 404 (exit 1); official public advisory pages were used for the Wasmtime scope review and are linked in the audit.
- No build or product test suite ran. No settings, action pins, downloaded pins, caches, or dependency trees were changed.

## Remaining work

TUNE-11 remains phase-pending because later plans own the score-selected improvement, user documentation refresh, rolling roadmap and prompt coverage updates, and lesson export. Plan 17-06 must select the single improvement from measured scores; Plan 17-08 owns only demonstrated documentation corrections. Continue to Plan 17-05. No user action is required.

---
*Phase: 17-a-measured-and-trustworthy-playability-baseline*
*Plan: 04*
*Completed: 2026-11-27*
