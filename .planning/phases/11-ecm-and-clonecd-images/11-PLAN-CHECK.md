# Phase 11 plan review

## Scope update after execution sizing

The independent review below applies to the original twelve-plan set. After Plan 11-04 Task 1 took about 33 minutes, its unstarted companion-confinement task was moved intact to Plan 11-13. Plan 11-06 now waits on Plan 11-13 to preserve exclusive CCD file ownership; Plan 11-05 remains independent in Wave 4. The new Plan 11-13 and updated dependencies/waves have not yet received a refreshed independent plan-checker verdict.

## VERIFICATION PASSED

Reviewed twelve executable plans against the Phase 11 goal, ECM-01/CCD-01, all eleven locked decisions, the research responsibility map and project instructions. No remaining blocker, warning or advisory finding.

The pre-review consumer-verification gap was corrected before this verdict: Plan12 now explicitly verifies the installed SDL2 example link and CHD-ON consumers in its third task. Its preceding package task no longer claims the later example result.

## Coverage

| Required outcome | Plans | Result |
|---|---|---|
| Complete ECM validation, exact reconstructed stream, fixed random-access work/memory | 03,05,07,10 | Covered |
| Primary format mapping plus independent type1–3 source/integrity proof | 05,07 | Covered |
| Fixture-proven CCD modes, indexes, pregaps, confined same-stem companions | 04,06,07,10 | Covered |
| Independent channel-major SUB transpose vector and distinct valid-Q consumer proof | 06,09 | Covered |
| Shared runner/libretro loading, content identity and replacement preservation | 08,09 | Covered |
| Source/request/output preflights before bounded guest work | 07,08,09,11 | Covered |
| Protected evidence rows for both providers | 11,12 | Covered |
| Exact trusted metadata-only PR Nightly exception and fail-closed negatives | 02,12 | Covered |
| Existing cap, notices, distinct Main/Release checks and portable/fuzz closure | 01,02,09,10,12 | Covered |

The broader SPDX scanner, undocumented CCD variants, unproven ECM mappings and broad CI redesign remain excluded as directed. Rejection is explicitly required where variant evidence is missing.

## Execution structure

Dependencies are acyclic and wave numbers follow their prerequisites. Shared dispatcher/image-test edits serialize Plans03/04; independent module expansion and host/fuzz ownership have no same-wave file overlap. Plan01 establishes the required CMake/inventory skeleton and real private rejection path before positive reader slices.

Plans contain two or three tasks; each task names no more than five modified files. Plan01 has eleven files overall, primarily small private boundaries and their required skeleton registrations; its three separate tasks and low context projection keep this within a coherent executable slice. The explicit thirty-minute split rule remains applicable during execution.

All estimates are below the configured 100000-token smart-zone budget. Estimates are uncalibrated: factor1, zero samples, confidence low. Reported projections are 6500–15000 tokens; these are planning estimates, not runtime promises.

## Static evidence

- All twelve GSD plan-structure checks returned valid.
- Planner-provided command-path probe reports not_applicable for the CMake-oriented commands, with no findings. This is not a claim that every future target already exists.
- Refreshed failure-direction probe covers twenty-six automated commands with no findings.
- The one-shot balanced-tag, shell syntax, task-file-count, terminal-output-tag and same-wave-ownership check passed: `python3 /tmp/schway11-plan-syntax.py`, exit0.
- Task12's current three-task structure was checked after the consumer-verification correction.
- Optional security-enforcement/Nyquist gates are disabled in project configuration; mandatory hostile-input/path validation remains fully planned. No PATTERNS.md exists, so pattern-specific checking is inapplicable.

No application, source implementation, broad suite or hosted workflow was run by this review. The plans require source review/fixes before final phase verification and keep hosted-only evidence assigned to the appropriate lifecycle step.

Runtime/model: Codex GSD plan checker; inherited model identifier unavailable. Planning may proceed to the orchestrator's commit and execution handoff. No owner action is needed.

## Re-review after the CCD plan split

**Phase:** 11 — ECM and CloneCD images

**Plans checked:** 13

**Status:** The 11-04 / 11-13 split and dependency updates are sound. One existing blocker remains in Plan 11-02.

### Split review

| Check | Result |
|---|---|
| ECM-01 coverage | Plans 03, 05, 07–12 cover decoding, bounds, consumers, evidence and CI closure. |
| CCD-01 coverage | Plans 04, 06–13 cover direct data reads, confinement, indexes, SUB, consumers, evidence and CI closure. |
| Plan structure | All 13 `verify.plan-structure` checks returned valid; the new Plan 13 has one complete task. |
| Dependencies and waves | The graph is acyclic. Plan 13 follows Plan 04; Plans 05 and 13 occupy Wave 4 with disjoint files; Plan 06 waits for Plan 13 before editing the same CCD files. |
| Context decisions | Plan 13 preserves D-05 same-stem canonical confinement, D-07 missing/empty SUB behavior and explicit rejection of nonempty SUB until Plan 06 proves its mapping. Plan 06 retains independent SUB transpose and consumer checks. |
| Gate ceiling | The split adds no repository-check controls and leaves the 3,901-line ceiling unchanged. |
| Syntax and same-wave ownership | `python3 /tmp/schway11-plan-syntax.py` passed: 13 plans, 26 shell commands, no syntax or ownership findings. |

### Blocker

**1. [scope_sanity] Planned work must pass the approved 3,901-line gate before execution.**

- Plan: `11-02`
- Required property: The exact Nightly-route task is executable while retaining the approved gate ceiling and all required cases.
- Evidence: `.planning/STATE.md` records Plan 11-02 at 3,928/3,901 counted lines. Its verification begins with the 3,901-line budget command, so it cannot reach the classifier checks on the measured state. D-08 remains part of the phase scope and D-09 fixes the cap.
- Example fix (non-binding): Find a semantics-preserving consolidation that keeps every gate and case, or leave Plan 11-02 deferred and do not claim Phase 11 complete until a cap-compatible route is planned.

### Structured issues

```yaml
issues:
  - plan: "11-02"
    dimension: scope_sanity
    severity: blocker
    required_property: "The exact metadata-only Nightly route is executable under the approved 3901-line ceiling while retaining all required cases."
    description: "STATE.md records 3928/3901 counted lines. Plan 11-02's first verification command enforces the 3901-line limit, so its current measured state fails before the classifier checks. D-08 remains in phase scope and D-09 locks the ceiling."
```

### Recommendation

The CCD split is ready for execution. Continue independent Phase 11 plans, but resolve or explicitly re-plan the Plan 11-02 budget blocker before phase-level verification and ship; do not raise the cap or remove required controls to force a pass.

Runtime/model: Codex GSD plan checker (`gpt-6-luna`, xhigh). No product build, test, hosted workflow or source inspection was performed for this re-review.
