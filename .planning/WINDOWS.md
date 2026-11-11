---
schema_version: 1
open_count: 0
waived_count: 1
fixed_count: 0
total_count: 1
last_updated: 2026-11-11T01:04:45.408Z
---

# Broken Windows Ledger

> Cross-phase defect register. With `workflow.windows_enforce` enabled, `/gsd-ship` blocks while `open_count > 0`.
> Waive with `gsd-tools windows waive <id> "<reason>"` (reason required).
> Mark fixed with `gsd-tools windows fixed <id>`.

| id | phase | kind | file | line | description | status | reason | recorded_at | resolved_at |
|----|-------|------|------|------|-------------|--------|--------|-------------|-------------|
| 1 | 04 | deviation | .planning/phases/04-games-play-in-retroarch/04-14-PLAN.md |  | Task 2 added regression coverage after task 1 had already delivered the behavior, so no separate RED/GREEN production change was possible. | waived | Task 2 adds regression coverage for transport behavior required by task 1; the existing tracer had a genuine RED/GREEN cycle. Documented test-only sequencing exception, no unresolved defect or owner gate. | 2026-11-11T00:58:34.703Z | 2026-11-11T01:04:45.408Z |

````json
[
  {
    "id": 1,
    "kind": "deviation",
    "phase": "04",
    "file": ".planning/phases/04-games-play-in-retroarch/04-14-PLAN.md",
    "line": null,
    "description": "Task 2 added regression coverage after task 1 had already delivered the behavior, so no separate RED/GREEN production change was possible.",
    "status": "waived",
    "reason": "Task 2 adds regression coverage for transport behavior required by task 1; the existing tracer had a genuine RED/GREEN cycle. Documented test-only sequencing exception, no unresolved defect or owner gate.",
    "recorded_at": "2026-11-11T00:58:34.703Z",
    "resolved_at": "2026-11-11T01:04:45.408Z",
    "milestone": "v1.0"
  }
]
````
