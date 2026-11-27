# Phase 11 plan review

## Scope update after execution sizing

The initial review below applies to the original twelve-plan set. After Plan 11-04 Task 1 took about 33 minutes, its unstarted companion-confinement task was moved intact to Plan 11-13. The 11-04/11-13 split and ownership were independently reviewed below. The latest re-review now also evaluates the evidence-based D-08 deferral and revised Plan 11-02.

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

## Superseded re-review after D-08 budget deferral (2026-11-22)

**Phase:** 11 — ECM and CloneCD images

**Plans checked:** 13

**Verdict:** ISSUES FOUND — 1 blocker, 1 warning

### Coverage and structure

- ECM-01 is still claimed by Plans 03, 05, 07–12; its tasks cover modeled bytes, bounded reads, consumers, hostile input, guest evidence and CI closure.
- CCD-01 is still claimed by Plans 04, 06–13; its tasks cover the single-track reader, confined companions, indexes/SUB, consumers, hostile input, guest evidence and CI closure.
- Plan 02 now has `requirements: []`, so the budget disposition cannot falsely complete either product requirement. D-12 supersedes only the optional D-08 Nightly shortcut; the current route, all gates and all lanes remain.
- `verify.plan-structure` passed 12 of the 13 plans. Plan 02 failed because the YAML frontmatter opening delimiter is never closed before `<objective>`, so the parser reported no frontmatter fields and all required frontmatter keys missing. The generic balanced-tag/shell probe passed 13 plans and 25 command blocks, but it does not validate GSD frontmatter and is not evidence that Plan 02 is executable.
- Apart from that parse failure, the intended dependency graph is acyclic: Plan 02/03 follow 01, Plan 04 follows 03, Plans 05/13 are disjoint in Wave 4, Plan 06 waits for 13, and Plan 12 depends on 02 and 11.
- The cap decision is justified by new measured evidence: 3,898/3,901 current lines plus 30 proposed controls equals 3,928 before classifier/workflow additions; the two read-only audits found no safe measured reduction. No cap change or test deletion is planned.

### Blocker

**1. [task_completeness] Every executable plan must have parseable GSD frontmatter.**

- Plan: `11-02`
- Required property: The GSD planner/executor can parse the plan's required frontmatter and dependencies.
- Evidence: `node "$HOME/.codex/gsd-core/bin/gsd-tools.cjs" query verify.plan-structure .planning/phases/11-ecm-and-clonecd-images/11-02-PLAN.md` returned `valid: false`, listed all eight required frontmatter fields as missing and returned an empty `frontmatter_fields` array. Inspection shows no closing `---` between `must_haves` and `<objective>`. Plan 12 depends on Plan 02, so the malformed file blocks downstream phase execution.
- Example fix (non-binding): Close the YAML frontmatter after `must_haves`, then rerun the plan structure check.

### Warning

**1. [verification_derivation] Roadmap and coverage labels must match the re-scoped plan.**

- Evidence: ROADMAP still describes Plan 11-02 as implementing the “Exact metadata-only PR Nightly route,” while D-12 and the new Plan 02 defer that route. `11-PLAN-COVERAGE.md` also retains the row “Conservative trusted-files Nightly route” as COVERED for Plans 02/12, even though the exact metadata shortcut is deferred.
- Example fix (non-binding): Rename the Roadmap plan entry to the evidence-based deferral and make the coverage row distinguish retained current routing from the deferred shortcut.

### Structured issues

```yaml
issues:
  - plan: "11-02"
    dimension: task_completeness
    severity: blocker
    required_property: "Every executable plan has parseable GSD frontmatter with required fields."
    description: "verify.plan-structure returned valid=false with no frontmatter fields because Plan 02 lacks the closing YAML delimiter before its objective; Plan 12 depends on this plan."
    fix_hint: "Close Plan 02's frontmatter and rerun the structure check."
  - plan: "11-02"
    dimension: verification_derivation
    severity: warning
    required_property: "Roadmap and coverage artifacts describe the revised plan scope accurately."
    description: "The Roadmap still says Plan 02 implements the metadata-only route and the coverage map still marks the trusted-files Nightly route covered, while D-12 defers that shortcut."
    fix_hint: "Rename the roadmap item and distinguish retained routing from the deferred shortcut in the coverage map."
```

### Recommendation

Do not execute Plan 02 or its dependent Plan 12 until the frontmatter blocker is repaired. The D-08 deferral itself is consistent with D-12 and the locked cap; after the structural and bookkeeping corrections, rerun the plan checker before execution. No product build, test or hosted workflow was run for this review.

Runtime/model: Codex GSD plan checker (`gpt-6-luna`, xhigh). Review was static and read-only apart from this report.

## Latest re-review after Plan 02 correction (2026-11-22)

**Phase:** 11 — ECM and CloneCD images

**Plans checked:** 13

**Verdict:** ISSUES FOUND — 0 blockers, 1 warning

### Coverage and structure

- `verify.plan-structure` returned `valid: true` with no errors or warnings for all 13 plans. Every task has files, action, verify and done fields; no plan exceeds three tasks.
- ECM-01 is claimed in Plans 01, 03, 05, 07–12; their task actions cover ECM bytes, bounded reads, host consumers, hostile input, guest evidence and CI closure. CCD-01 is claimed in Plans 01, 04, 06–13; tasks cover the reader, confined companions, indexes/SUB, consumers, hostile input, guest evidence and CI closure.
- Plan 02 has `requirements: []` and explicitly keeps ECM-01/CCD-01 open. `.planning/REQUIREMENTS.md` still marks both pending and the Phase 11 checkbox remains open in ROADMAP; no product requirement is falsely completed.
- The dependency DAG is acyclic and waves follow dependencies. Same-wave plans have disjoint declared `files_modified`: Plans 02/03 in Wave 2, Plans 05/13 in Wave 4, and Plans 08/09/10 in Wave 7. Plan 06 follows Plan 13 before shared CCD ownership; Plan 12 follows Plans 02 and 11.
- The 3,901-line ceiling is stated in REQUIREMENTS and preserved. D-12 records 3,898 current lines plus 30 proposed route controls (3,928), with two independent audits finding no safe measured consolidation. Plan 02 now defers only the metadata shortcut, retains the current full route/cases/lanes, and Plan 12 still closes fixture/corpus producers on that full route.
- The coverage ledger now describes the existing trusted-files boundary and separate Main/Release gates as retained behavior, and marks the D-08 metadata shortcut `DEFERRED WITH EVIDENCE`. The Wave 2 Roadmap entry accurately names Plan 02's cap-based deferral; neither artifact says the shortcut was implemented.
- All 13 estimate checks are below the 100,000-token smart-zone budget. Confidence is `low` with zero calibration samples, so these estimates are not project-calibrated; task/file limits remain the more useful scope signal.
- No product build, test, emulator, or hosted workflow was run for this static review.

### Warnings

**1. [scope_sanity] Plans should stay within the project's measured file and duration limits.**

- Plan: `11-01` (completed)
- Required property: Plan scope should remain within the per-plan file threshold and the project's 30-minute split rule.
- Evidence: The plan declares 11 modified files, above the 10-file warning threshold; STATE.md records its three tasks took 37 minutes. This is completed historical work and does not block the remaining plans, but it exceeded the project's intended plan duration.
- Example fix (non-binding): Use the measured overrun to split future work when its size approaches this scope; no retroactive change is required for completed Plan 01.

### Structured issues

```yaml
issues:
  - plan: "11-01"
    dimension: scope_sanity
    severity: warning
    required_property: "Plan scope should remain within the per-plan file threshold and the project's 30-minute split rule."
    description: "Completed Plan 01 declares 11 modified files and STATE.md records three tasks taking 37 minutes, exceeding the 10-file warning threshold and 30-minute project rule."
    fix_hint: "Use this measured overrun to split future work when it approaches this scope; no retroactive change is required for completed Plan 01."
```

### Recommendation

No blocker remains: D-12, the coverage ledger and Roadmap now agree on the shortcut deferral and retained CI routes. The only warning is retrospective: completed Plan 01 exceeded the scope target. The cap decision and all product requirements remain intact. No owner action is needed for this static review; no build, test or hosted run was performed.

Runtime/model: Codex GSD plan checker; inherited model identifier unavailable. Review was static and read-only apart from this report update.
