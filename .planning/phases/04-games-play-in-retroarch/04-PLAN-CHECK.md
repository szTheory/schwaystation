## ISSUES FOUND

**Phase:** Games play in RetroArch  
**Plans checked:** 32  
**Issues:** 3 blockers, 2 warnings, 0 info

### Blockers

**1. [context_compliance] The first ledger comparison must work from the base state that actually exists.**
- Plan: `04-24`, Task 1
- Evidence: The task says the first-introduction baseline will use “a pinned immutable bootstrap judge.” The base tree has no prior ledger or judge, and phase context explicitly says not to require a nonpublic/fictitious bootstrap pin or publication ceremony. This makes the planned first ledger admission depend on an unavailable artifact.
- Example fix (non-binding): Treat a missing base ledger/judge as an explicit empty/all-unknown baseline for first introduction; use the protected base-copy path once those artifacts exist.

**2. [task_completeness] A tracer task's own verification must execute its new behavior through a registered test.**
- Plans/tasks: `04-05`/1, `04-06`/1, `04-08`/1, `04-09`/1, `04-14`/1, `04-16`/1, `04-17`/1, `04-18`/1, `04-20`/1, `04-21`/1, `04-22`/1, `04-24`/1, `04-25`/1, `04-26`/1, `04-27`/1, `04-30`/1
- Evidence: These tracer actions defer case registration or inventory completion to a later task, while their own `<verify>` commands run the corresponding label or full CI. For example, `04-14` Task 1 says wiring is completed in Task 2 and Task 3 registers the SIO tests, but Task 1 already runs `ctest -L '^sio$' --no-tests=error`; Plan `04-01` creates the SIO inventory empty, so that first task has no SIO test to execute. `04-24` Task 1 similarly runs the `compat` label before its first compat tests are registered. In `04-05`/`06`/`08`/`09`, `04-16`/`17`/`18`, `04-20`/`21`/`22`, `04-25`/`26`/`27`, and `04-30`, existing tests under the same label can pass while the new tracer remains unregistered. The exact-inventory configure check also fails when a test is registered before its inventory is updated. For `04-30`, the tracer command selects the CI GPU label, not the separate nightly inventory that runs reference comparisons. These task-level checks therefore cannot pass the stated behavior acceptance as written.
- Example fix (non-binding): Make each tracer's test registration and matching inventory part of that task before its verification, or move the verification boundary after the task that completes registration.

**3. [scope_sanity] Each plan must stay below the 15-file blocker threshold.**
- Plan: `04-01`
- Evidence: `files_modified` declares 16 distinct files across its three tasks, exceeding the plan-check scope threshold.
- Example fix (non-binding): Reduce per-plan file scope while retaining the required first-plan CMake skeleton/inventories and splitting independent registration or documentation work into a dependent plan.

### Warning

**1. [scope_sanity] Plans should stay within the 10-file warning threshold.**
- Plans: `04-14`, `04-16`, `04-20`
- Evidence: Each plan declares 14 distinct modified files across three tasks.
- Example fix (non-binding): Move independent registration or documentation work into a dependent plan if the per-plan work estimate exceeds the 30-minute limit.

**2. [requirement_coverage] The library test must produce a meaningful ledger-linked fixture result.**
- Plans: `04-24`–`04-26`
- Evidence: `04-25` creates a temporary cue/bin identity and synthetic DAT test row; `04-24` describes committed ledger rows for `gpu_motion`, `pad_counter`, `memcard_roundtrip`, and OpenBIOS; `04-26` says the test uses a “temporary listed fixture” and truthfully reports unsupported execution, but does not specify the matching ledger row or how the test supplies it. Without a row match, the private-library path can redact/count the fixture as unlisted instead of proving a measured supported predicate.
- Example fix (non-binding): Specify the existing temporary-fixture route that makes the cue/bin identity resolve to a ledger predicate with a measured metadata result, while leaving boot/play unknown until Phase 6.

### Structured Issues

```yaml
issues:
  - plan: "04-24"
    task: 1
    dimension: context_compliance
    severity: blocker
    required_property: "The first ledger comparison works from the base state that actually exists, without a nonexistent bootstrap judge."
    description: "The base tree has no ledger or judge, but Task 1 requires a pinned immutable bootstrap judge for first introduction; phase context explicitly says not to demand a fictional pin or publication ceremony."
    fix_hint: "Treat an absent base ledger/judge as an explicit empty/all-unknown baseline for first introduction, then use the protected base-copy path after those artifacts exist."
  - plan: "04-14"
    plans: ["04-05", "04-06", "04-08", "04-09", "04-14", "04-16", "04-17", "04-18", "04-20", "04-21", "04-22", "04-24", "04-25", "04-26", "04-27", "04-30"]
    task: 1
    dimension: task_completeness
    severity: blocker
    required_property: "A tracer task's verification executes its new behavior through a test registered in the selected build."
    description: "Each listed tracer defers test registration or inventory completion to a later task but verifies the corresponding behavior before that integration exists. In 04-14 Task 1, the SIO inventory created by 04-01 is still empty and the only SIO registrations are in Task 3, so its ctest -L '^sio$' selection has no SIO test; 04-24 Task 1 likewise selects compat before that plan registers any compat case. Other listed plans can select preexisting label tests and pass without exercising the new tracer, and 04-30's ctest command selects CI GPU tests rather than the nightly reference inventory."
    fix_hint: "Complete the tracer test's registration and matching inventory in the same task, or place its verification after the registration task."
  - plan: "04-01"
    dimension: scope_sanity
    severity: blocker
    required_property: "No plan has 15 or more distinct modified files."
    description: "04-01 declares 16 distinct modified files."
    fix_hint: "Reduce per-plan file scope while preserving the phase's required first-plan CMake skeleton and inventories."
  - plan: "04-14"
    plans: ["04-14", "04-16", "04-20"]
    dimension: scope_sanity
    severity: warning
    required_property: "Per-plan file scope stays within 10 files unless a larger scope is shown to be necessary."
    description: "04-14, 04-16, and 04-20 each declare 14 distinct modified files across three tasks."
    fix_hint: "Move independent registration or documentation work into a dependent plan if the per-plan work estimate exceeds the 30-minute limit."
  - plan: "04-26"
    plans: ["04-24", "04-25", "04-26"]
    dimension: requirement_coverage
    severity: warning
    required_property: "The local library test joins its temporary cue/bin identity to a ledger predicate and measures a supported metadata result without claiming boot or play."
    description: "The plans create a temporary cue/bin identity and synthetic DAT row, but do not specify the matching ledger row or how the test supplies it; without that match, the path can only count/redact the fixture as unlisted."
    fix_hint: "Specify the temporary-fixture route that resolves the cue/bin identity to a ledger predicate with a measured metadata result, leaving boot/play unknown until Phase 6."
```

### Additional review notes

- All seven GAME requirements are claimed in plan frontmatter. The supplied plan-structure and decision-coverage probes pass; the supplied path and failing-direction probes have no findings.
- Declared dependencies are acyclic and wave-consistent; no same-wave plans have overlapping declared files.
- Nyquist/security checks are disabled by project configuration. No UI or AI spec applies; no unresolved research questions or architectural responsibility map were found.
- Every estimate is under the 100,000-token smart-zone budget. Confidence is `low` on all plans because the project has zero actuals (`sample_count=0`); treat the estimates as uncalibrated and weigh the task/file thresholds more heavily. No plan is over budget, so no re-slicing recommendation applies.

| Plan | Estimated tokens | Budget | Over budget | Confidence |
|---|---:|---:|---|---|
| 04-01 | 17,000 | 100,000 | no | low |
| 04-02 | 24,000 | 100,000 | no | low |
| 04-03 | 25,000 | 100,000 | no | low |
| 04-04 | 22,000 | 100,000 | no | low |
| 04-05 | 23,000 | 100,000 | no | low |
| 04-06 | 21,000 | 100,000 | no | low |
| 04-07 | 24,000 | 100,000 | no | low |
| 04-08 | 23,000 | 100,000 | no | low |
| 04-09 | 25,000 | 100,000 | no | low |
| 04-10 | 24,000 | 100,000 | no | low |
| 04-11 | 25,000 | 100,000 | no | low |
| 04-12 | 22,000 | 100,000 | no | low |
| 04-13 | 16,000 | 100,000 | no | low |
| 04-14 | 25,000 | 100,000 | no | low |
| 04-15 | 24,000 | 100,000 | no | low |
| 04-16 | 25,000 | 100,000 | no | low |
| 04-17 | 21,000 | 100,000 | no | low |
| 04-18 | 23,000 | 100,000 | no | low |
| 04-19 | 22,000 | 100,000 | no | low |
| 04-20 | 24,000 | 100,000 | no | low |
| 04-21 | 19,000 | 100,000 | no | low |
| 04-22 | 22,000 | 100,000 | no | low |
| 04-23 | 22,000 | 100,000 | no | low |
| 04-24 | 21,000 | 100,000 | no | low |
| 04-25 | 24,000 | 100,000 | no | low |
| 04-26 | 20,000 | 100,000 | no | low |
| 04-27 | 22,000 | 100,000 | no | low |
| 04-28 | 25,000 | 100,000 | no | low |
| 04-29 | 17,000 | 100,000 | no | low |
| 04-30 | 25,000 | 100,000 | no | low |
| 04-31 | 19,000 | 100,000 | no | low |
| 04-32 | 18,000 | 100,000 | no | low |

### Recommendation

3 blocker(s) and 2 warning(s) require revision. Returning to planner with feedback.
