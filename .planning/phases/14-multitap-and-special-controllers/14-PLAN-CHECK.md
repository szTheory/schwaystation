# Phase 14 Plan Check

**Phase:** Multitap and special controllers  
**Plans checked:** 12  
**Status:** ISSUES FOUND — 0 blockers, 2 warnings

## Requirement coverage

| Requirement | Plans | Status |
|---|---|---|
| SIO-01 | 01–04, 07–12 | Covered: tap controller polls/history, both ports, cards, state continuation, stable filenames, runner and libretro persistence, and narrow compatibility evidence. |
| SIO-02 | 01–02, 05, 07, 10, 12 | Covered: bounded API input, frame-latched mouse movement/buttons/reset, serial and libretro mapping, and narrow evidence. |
| SIO-03 | 01–02, 05–07, 10, 12 | Covered: neGcon/Jogcon native payloads, frame latching, explicit unsupported modes, state validation and host mappings. |
| SIO-04 | 01–02, 06–07, 10, 12 | Covered: API v9 flight controls, serial payload, libretro mapping and legacy caller compatibility. |

The task actions implement all four requirements. The mapped Phase 15 gun/lightgun requirements and CONTEXT deferred ideas are not pulled into Phase 14. CONTEXT decisions are represented across the plans, including the exact tap poll-history rule, 28-byte input prefix, card 1/2 aliases, independent persistent media, protocol-only state restore, default-off tap settings, and existing workflow/test coverage.

## Plan integrity and dependency waves

All 12 plans pass `verify.plan-structure`; each has 2–3 tasks with files, specific actions, verification and done criteria. Task file lists match their plan-level `files_modified` lists. Every runnable command has a `<fails_when>` statement. The dependency graph is acyclic, all references resolve, and wave numbers match dependencies. Same-wave plans have no shared modified files:

| Plan | Tasks | Files | Wave | Dependencies | Status |
|---|---:|---:|---:|---|---|
| 14-01 | 3 | 11 | 1 | — | Valid |
| 14-02 | 3 | 10 | 2 | 14-01 | Valid |
| 14-03 | 2 | 4 | 3 | 14-02 | Valid |
| 14-04 | 2 | 8 | 4 | 14-03 | Valid |
| 14-05 | 2 | 5 | 5 | 14-04 | Valid |
| 14-06 | 2 | 4 | 6 | 14-05 | Valid |
| 14-07 | 2 | 8 | 7 | 14-04, 14-06 | Valid |
| 14-08 | 2 | 4 | 5 | 14-04 | Valid |
| 14-09 | 2 | 3 | 6 | 14-08 | Valid |
| 14-10 | 2 | 3 | 7 | 14-06, 14-08 | Valid |
| 14-11 | 2 | 3 | 8 | 14-10 | Valid |
| 14-12 | 2 | 7 | 9 | 14-07, 14-09, 14-11 | Valid |

## Verification and efficiency review

The GSD failure-direction probe found 26 runnable commands and no missing, empty, placeholder or orphan failure statements. The path probe reported `not_applicable` for all commands because none uses a grounded `cd` or `npm --prefix` target. Static validation passed `bash -n` for all 26 command bodies and Python AST parsing for all 23 embedded `python3 -c` payloads. No build, test suite, application, hosted CI, Act job or Docker resource was run during this plan review.

Plans sort changed inventories before configure, assert expected selectors before target builds where required, and run focused selectors before the single final workflow handoff. They retain existing full local and hosted coverage. No workflow changes are planned, so Act is correctly omitted. Plan 12 calls for metric producer measurements before any bound change, source review before aggregate verification, and CI critical-path/cost inspection at ship. The summary contract in each plan preserves block-style `coverage` entries with verification kind/reference/status and `human_judgment`.

The plan-structure probe also warns about bare `git diff --check`; these task checks inspect the in-progress, uncommitted task tree before its atomic commit, so the warning does not indicate a vacuous committed-tree check.

## Warnings

### 1. Plan 14-01 exceeds the preferred file-count range

- **Severity:** WARNING
- **Dimension:** scope_sanity
- **Required property:** A plan stays within the normal per-plan change-scope range.
- **Evidence:** Plan 14-01 lists 11 modified files, above the checker’s 10-file warning threshold. Its tasks are separately scoped to 3, 4 and 4 files, and the breadth comes from the project requirement to establish test registrations/inventories in the first plan.
- **Example fix:** Keep each task bounded to its listed files and split any implementation task that would exceed the 30-minute project limit.

### 2. Plan 14-02 reaches the preferred file-count warning threshold

- **Severity:** WARNING
- **Dimension:** scope_sanity
- **Required property:** A plan stays within the normal per-plan change-scope range.
- **Evidence:** Plan 14-02 lists 10 modified files, at the checker’s warning threshold. Its three tasks own 5, 4 and 3 files, respectively.
- **Example fix:** Keep each task bounded to its listed files and split any implementation task that would exceed the 30-minute project limit.

## Structured issues

```yaml
issues:
  - plan: 14-01
    dimension: scope_sanity
    severity: warning
    required_property: A plan stays within the normal per-plan change-scope range.
    description: The plan lists 11 modified files, above the 10-file warning threshold; its three tasks separately list 3, 4 and 4 files, and the breadth is required by the project's first-plan inventory rule.
    fix_hint: Keep each task bounded and split any task that would exceed the 30-minute project limit.
  - plan: 14-02
    dimension: scope_sanity
    severity: warning
    required_property: A plan stays within the normal per-plan change-scope range.
    description: The plan lists 10 modified files at the warning threshold; its three tasks separately list 5, 4 and 3 files.
    fix_hint: Keep each task bounded and split any task that would exceed the 30-minute project limit.
```

Calibrated estimates are 9,000–15,000 tokens per plan, 9–15% of the 100,000-token budget; all are below budget. Their confidence is low because the project has no calibration actuals yet. The research has no Architectural Responsibility Map or Validation Architecture section; those optional dimensions do not apply.

**Recommendation:** The plans cover the Phase 14 goal with no blockers. All 26 commands now have one validate-only annotation, correctly bound inside their task verification blocks. The two remaining file-count warnings are contained to small, separately scoped tasks and preserve the required first-plan inventory work.
