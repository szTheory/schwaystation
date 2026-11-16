---
schema_version: 1
open_count: 8
waived_count: 4
fixed_count: 1
total_count: 13
last_updated: 2026-11-16T05:30:45.909Z
---

# Broken Windows Ledger

> Cross-phase defect register. With `workflow.windows_enforce` enabled, `/gsd-ship` blocks while `open_count > 0`.
> Waive with `gsd-tools windows waive <id> "<reason>"` (reason required).
> Mark fixed with `gsd-tools windows fixed <id>`.

| id | phase | kind | file | line | description | status | reason | recorded_at | resolved_at |
|----|-------|------|------|------|-------------|--------|--------|-------------|-------------|
| 1 | 04 | deviation | .planning/phases/04-games-play-in-retroarch/04-14-PLAN.md |  | Task 2 added regression coverage after task 1 had already delivered the behavior, so no separate RED/GREEN production change was possible. | waived | Task 2 adds regression coverage for transport behavior required by task 1; the existing tracer had a genuine RED/GREEN cycle. Documented test-only sequencing exception, no unresolved defect or owner gate. | 2026-11-11T00:58:34.703Z | 2026-11-11T01:04:45.408Z |
| 2 | 04 | deviation | .planning/phases/04-games-play-in-retroarch/04-24-PLAN.md |  | Resumed Task 1 judge/docs implementation and Task 2 expansion controls landed together after prior RED and partial schema/data GREEN commits; no second RED was claimed. | waived | Documented in 04-24 SUMMARY and STATE; verified implementation and checks are complete, and no source defect or owner gate remains. | 2026-11-12T03:50:26.230Z | 2026-11-12T03:50:30.829Z |
| 3 | 04 | deviation | .planning/phases/04-games-play-in-retroarch/04-26-PLAN.md |  | Recovered tasks 1 and 2 were committed together after the valid RED evidence remained uncommitted in ignored scratch output; no RED commit was reconstructed. | waived | The summary documents the authentic RED evidence and combined recovered commit. All planned tests and final workflows passed; no product defect or owner action remains. | 2026-11-12T13:40:13.705Z | 2026-11-12T13:40:18.081Z |
| 4 | 05 | stub | src/spu.c | 76 | Reset SPU event emits a zero stereo pair until planned voice and reverb synthesis lands. | open |  | 2026-11-13T22:11:13.643Z |  |
| 5 | 05 | deviation | .planning/phases/05-the-sound-matches-the-console/05-05-SUMMARY.md |  | Split remaining DMA4 timing, DEV4, seven-case comparator, and nightly work into plan 05-21 at the 30-minute target. | open |  | 2026-11-14T01:34:02.691Z |  |
| 6 | 05 | stub | src/spu_reverb.c | 79 | The 22.05 kHz reverb feedback stage remains a unity pass-through seam; plan 05-10 replaces it with register-driven feedback. | open |  | 2026-11-14T06:29:30.694Z |  |
| 7 | 06 | deviation | .planning/phases/06-discs-and-fmv/06-02-PLAN.md |  | Registered the test target before Task 1 RED because its required verification depended on Task 2 CMake wiring. | waived | Documented sequencing adjustment; the test wiring and required compatibility check passed, with no product defect or owner action. | 2026-11-15T16:16:55.624Z | 2026-11-15T16:16:59.252Z |
| 8 | 06 | stub | src/cdrom.c | 180 | CD-ROM MMIO reads return zero and writes are ignored until Plans 06-04 and 06-05 add command and sector behavior. | open |  | 2026-11-15T16:42:46.664Z |  |
| 9 | 06 | stub | src/mdec.c | 124 | MDEC MMIO reads return zero and writes are ignored until Plan 06-11 adds command and block behavior. | open |  | 2026-11-15T16:42:46.800Z |  |
| 10 | 06 | stub | src/cdrom.c | 38 | The CD-ROM field visitor is not called by state.c until Plan 06-26 wires state integration. | open |  | 2026-11-15T16:42:46.984Z |  |
| 11 | 06 | stub | src/mdec.c | 54 | The MDEC field visitor is not called by state.c until Plan 06-26 wires state integration. | open |  | 2026-11-15T16:42:47.169Z |  |
| 12 | 6 | stub | src/cdrom.c | 1083 | CD sector events clear the deadline; sector acquisition and INT1 delivery are deferred to Plan 06-05. | open |  | 2026-11-15T21:08:31.964Z |  |
| 13 | 6 | deviation | .planning/phases/06-discs-and-fmv/06-31-SUMMARY.md | 70 | MDEC pixel-order, clipping, and FE00-padding vectors moved to dependent Plan 06-32 after Plan 06-31 crossed 30 minutes. | fixed |  | 2026-11-16T05:22:27.956Z | 2026-11-16T05:30:45.909Z |

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
  },
  {
    "id": 2,
    "kind": "deviation",
    "phase": "04",
    "file": ".planning/phases/04-games-play-in-retroarch/04-24-PLAN.md",
    "line": null,
    "description": "Resumed Task 1 judge/docs implementation and Task 2 expansion controls landed together after prior RED and partial schema/data GREEN commits; no second RED was claimed.",
    "status": "waived",
    "reason": "Documented in 04-24 SUMMARY and STATE; verified implementation and checks are complete, and no source defect or owner gate remains.",
    "recorded_at": "2026-11-12T03:50:26.230Z",
    "resolved_at": "2026-11-12T03:50:30.829Z",
    "milestone": "v1.0"
  },
  {
    "id": 3,
    "kind": "deviation",
    "phase": "04",
    "file": ".planning/phases/04-games-play-in-retroarch/04-26-PLAN.md",
    "line": null,
    "description": "Recovered tasks 1 and 2 were committed together after the valid RED evidence remained uncommitted in ignored scratch output; no RED commit was reconstructed.",
    "status": "waived",
    "reason": "The summary documents the authentic RED evidence and combined recovered commit. All planned tests and final workflows passed; no product defect or owner action remains.",
    "recorded_at": "2026-11-12T13:40:13.705Z",
    "resolved_at": "2026-11-12T13:40:18.081Z",
    "milestone": "v1.0"
  },
  {
    "id": 4,
    "kind": "stub",
    "phase": "05",
    "file": "src/spu.c",
    "line": 76,
    "description": "Reset SPU event emits a zero stereo pair until planned voice and reverb synthesis lands.",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-11-13T22:11:13.643Z",
    "resolved_at": null,
    "milestone": "v1.0"
  },
  {
    "id": 5,
    "kind": "deviation",
    "phase": "05",
    "file": ".planning/phases/05-the-sound-matches-the-console/05-05-SUMMARY.md",
    "line": null,
    "description": "Split remaining DMA4 timing, DEV4, seven-case comparator, and nightly work into plan 05-21 at the 30-minute target.",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-11-14T01:34:02.691Z",
    "resolved_at": null,
    "milestone": "v1.0"
  },
  {
    "id": 6,
    "kind": "stub",
    "phase": "05",
    "file": "src/spu_reverb.c",
    "line": 79,
    "description": "The 22.05 kHz reverb feedback stage remains a unity pass-through seam; plan 05-10 replaces it with register-driven feedback.",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-11-14T06:29:30.694Z",
    "resolved_at": null,
    "milestone": "v1.0"
  },
  {
    "id": 7,
    "kind": "deviation",
    "phase": "06",
    "file": ".planning/phases/06-discs-and-fmv/06-02-PLAN.md",
    "line": null,
    "description": "Registered the test target before Task 1 RED because its required verification depended on Task 2 CMake wiring.",
    "status": "waived",
    "reason": "Documented sequencing adjustment; the test wiring and required compatibility check passed, with no product defect or owner action.",
    "recorded_at": "2026-11-15T16:16:55.624Z",
    "resolved_at": "2026-11-15T16:16:59.252Z",
    "milestone": "v1.0"
  },
  {
    "id": 8,
    "kind": "stub",
    "phase": "06",
    "file": "src/cdrom.c",
    "line": 180,
    "description": "CD-ROM MMIO reads return zero and writes are ignored until Plans 06-04 and 06-05 add command and sector behavior.",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-11-15T16:42:46.664Z",
    "resolved_at": null,
    "milestone": "v1.0"
  },
  {
    "id": 9,
    "kind": "stub",
    "phase": "06",
    "file": "src/mdec.c",
    "line": 124,
    "description": "MDEC MMIO reads return zero and writes are ignored until Plan 06-11 adds command and block behavior.",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-11-15T16:42:46.800Z",
    "resolved_at": null,
    "milestone": "v1.0"
  },
  {
    "id": 10,
    "kind": "stub",
    "phase": "06",
    "file": "src/cdrom.c",
    "line": 38,
    "description": "The CD-ROM field visitor is not called by state.c until Plan 06-26 wires state integration.",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-11-15T16:42:46.984Z",
    "resolved_at": null,
    "milestone": "v1.0"
  },
  {
    "id": 11,
    "kind": "stub",
    "phase": "06",
    "file": "src/mdec.c",
    "line": 54,
    "description": "The MDEC field visitor is not called by state.c until Plan 06-26 wires state integration.",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-11-15T16:42:47.169Z",
    "resolved_at": null,
    "milestone": "v1.0"
  },
  {
    "id": 12,
    "kind": "stub",
    "phase": "6",
    "file": "src/cdrom.c",
    "line": 1083,
    "description": "CD sector events clear the deadline; sector acquisition and INT1 delivery are deferred to Plan 06-05.",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-11-15T21:08:31.964Z",
    "resolved_at": null,
    "milestone": "v1.0"
  },
  {
    "id": 13,
    "kind": "deviation",
    "phase": "6",
    "file": ".planning/phases/06-discs-and-fmv/06-31-SUMMARY.md",
    "line": 70,
    "description": "MDEC pixel-order, clipping, and FE00-padding vectors moved to dependent Plan 06-32 after Plan 06-31 crossed 30 minutes.",
    "status": "fixed",
    "reason": "",
    "recorded_at": "2026-11-16T05:22:27.956Z",
    "resolved_at": "2026-11-16T05:30:45.909Z",
    "milestone": "v1.0"
  }
]
````
