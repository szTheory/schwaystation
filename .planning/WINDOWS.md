---
schema_version: 1
open_count: 18
waived_count: 5
fixed_count: 10
total_count: 33
last_updated: 2026-11-26T00:26:23.515Z
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
| 14 | 06 | deviation | .planning/phases/06-discs-and-fmv/06-12-PLAN.md |  | Plan 06-12 was split at the 30-minute boundary after its real MDEC guests exposed a production DMA0 decode mismatch; Plan 06-35 preserves and owns the repair and full gating oracle. | open |  | 2026-11-16T09:38:06.684Z |  |
| 15 | 10 | deviation | host/disc_image.c |  | Kept PBP excluded from M3U playlists while enabling standalone PBP through the shared provider. | fixed |  | 2026-11-21T13:37:30.614Z | 2026-11-21T13:37:36.507Z |
| 16 | 11 | stub | host/disc_ecm.c | 132 | Structurally valid ECM record streams remain unsupported until Plan 11-03 validates and decodes them. | fixed |  | 2026-11-22T02:07:27.684Z | 2026-11-22T03:07:48.499Z |
| 17 | 11 | stub | host/disc_ecm.c | 149 | ECM TOC generation remains unsupported until Plan 11-03 validates the complete record stream. | fixed |  | 2026-11-22T02:07:27.824Z | 2026-11-22T03:07:48.628Z |
| 18 | 11 | stub | host/disc_ecm.c | 157 | ECM decoded-size reporting remains unsupported until Plan 11-03 builds bounded checkpoints. | fixed |  | 2026-11-22T02:07:27.969Z | 2026-11-22T03:07:48.764Z |
| 19 | 11 | stub | host/disc_ecm.c | 165 | ECM random byte reads remain unsupported until Plan 11-03 builds bounded checkpoints. | fixed |  | 2026-11-22T02:07:28.118Z | 2026-11-22T03:07:48.889Z |
| 20 | 11 | stub | host/disc_ecm.c | 175 | ECM sector reads remain unsupported until Plan 11-03 validates its sector mapping. | fixed |  | 2026-11-22T02:07:28.247Z | 2026-11-22T03:07:49.015Z |
| 21 | 11 | stub | host/disc_ccd.c | 171 | CloneCD v3 track metadata remains unsupported until Plan 11-04 validates tracks and companions. | fixed |  | 2026-11-22T02:07:28.377Z | 2026-11-22T13:35:34.529Z |
| 22 | 11 | stub | host/disc_ccd.c | 188 | CloneCD TOC generation remains unsupported until Plan 11-04 validates the descriptor tracks. | fixed |  | 2026-11-22T02:07:28.513Z | 2026-11-22T13:35:43.098Z |
| 23 | 11 | stub | host/disc_ccd.c | 198 | CloneCD sector reads remain unsupported until Plan 11-04 validates its sector mapping. | fixed |  | 2026-11-22T02:07:28.648Z | 2026-11-22T13:35:43.206Z |
| 24 | 11 | stub | host/disc_ecm.c | 222 | ECM type-1/2/3 record decoding remains unsupported pending independent output vectors in Plan 11-05. | open |  | 2026-11-22T03:12:50.763Z |  |
| 25 | 13 | deviation | tests/core/test_state.c |  | Task 2 validation assertions were green immediately because shared board-state validation landed in Task 1; no separate Task 2 RED run was fabricated. | waived | Task 2 adds planned regression coverage for validation implemented in Task 1; all negative cases passed the exact focused selector, and the sequencing exception is recorded in 13-04-SUMMARY and STATE. No test or behavior gap remains. | 2026-11-23T17:38:49.703Z | 2026-11-23T17:39:13.635Z |
| 26 | 15 | deviation | src/pad.c |  | Raw fixed-width signed gun coordinates are required so the state visitor reproduces saved bytes. | open |  | 2026-11-25T15:06:00.729Z |  |
| 27 | 15 | deviation | tests/core/test_state.c |  | Expanded the test-only state field-record capacity to cover the new serialized gun fields. | open |  | 2026-11-25T15:07:38.675Z |  |
| 28 | 16 | stub | tests/disc/patch.c | 26 | Patch selector entry points fail until Plans 07-09 implement the source mapping and patch parsers. | open |  | 2026-11-26T00:26:23.287Z |  |
| 29 | 16 | stub | tests/runner/cheats.c | 54 | Cheat parser, bounds and order selectors fail until Plan 03 implements the shared bounded write table. | open |  | 2026-11-26T00:26:23.333Z |  |
| 30 | 16 | stub | tests/runner/tests.cmake | 37 | Runner cheat and patch CLI/hash modes fail until Plans 06 and 10 implement the options and hash behavior. | open |  | 2026-11-26T00:26:23.378Z |  |
| 31 | 16 | stub | tests/libretro/tests.cmake | 444 | Libretro cheat callback and state selectors await the host behavior in Plans 04-05. | open |  | 2026-11-26T00:26:23.425Z |  |
| 32 | 16 | stub | tests/compat/tests.cmake | 1292 | DAT inventory and capability evidence script modes fail until Plans 12-15 implement accounting and evaluation. | open |  | 2026-11-26T00:26:23.471Z |  |
| 33 | 16 | stub | tests/fuzz/tests.cmake | 337 | Patch and cheat replay executables are absent until Plan 11 adds the replay targets. | open |  | 2026-11-26T00:26:23.515Z |  |

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
  },
  {
    "id": 14,
    "kind": "deviation",
    "phase": "06",
    "file": ".planning/phases/06-discs-and-fmv/06-12-PLAN.md",
    "line": null,
    "description": "Plan 06-12 was split at the 30-minute boundary after its real MDEC guests exposed a production DMA0 decode mismatch; Plan 06-35 preserves and owns the repair and full gating oracle.",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-11-16T09:38:06.684Z",
    "resolved_at": null,
    "milestone": "v1.0"
  },
  {
    "id": 15,
    "kind": "deviation",
    "phase": "10",
    "file": "host/disc_image.c",
    "line": null,
    "description": "Kept PBP excluded from M3U playlists while enabling standalone PBP through the shared provider.",
    "status": "fixed",
    "reason": "",
    "recorded_at": "2026-11-21T13:37:30.614Z",
    "resolved_at": "2026-11-21T13:37:36.507Z",
    "milestone": "v2.0"
  },
  {
    "id": 16,
    "kind": "stub",
    "phase": "11",
    "file": "host/disc_ecm.c",
    "line": 132,
    "description": "Structurally valid ECM record streams remain unsupported until Plan 11-03 validates and decodes them.",
    "status": "fixed",
    "reason": "",
    "recorded_at": "2026-11-22T02:07:27.684Z",
    "resolved_at": "2026-11-22T03:07:48.499Z",
    "milestone": "v2.0"
  },
  {
    "id": 17,
    "kind": "stub",
    "phase": "11",
    "file": "host/disc_ecm.c",
    "line": 149,
    "description": "ECM TOC generation remains unsupported until Plan 11-03 validates the complete record stream.",
    "status": "fixed",
    "reason": "",
    "recorded_at": "2026-11-22T02:07:27.824Z",
    "resolved_at": "2026-11-22T03:07:48.628Z",
    "milestone": "v2.0"
  },
  {
    "id": 18,
    "kind": "stub",
    "phase": "11",
    "file": "host/disc_ecm.c",
    "line": 157,
    "description": "ECM decoded-size reporting remains unsupported until Plan 11-03 builds bounded checkpoints.",
    "status": "fixed",
    "reason": "",
    "recorded_at": "2026-11-22T02:07:27.969Z",
    "resolved_at": "2026-11-22T03:07:48.764Z",
    "milestone": "v2.0"
  },
  {
    "id": 19,
    "kind": "stub",
    "phase": "11",
    "file": "host/disc_ecm.c",
    "line": 165,
    "description": "ECM random byte reads remain unsupported until Plan 11-03 builds bounded checkpoints.",
    "status": "fixed",
    "reason": "",
    "recorded_at": "2026-11-22T02:07:28.118Z",
    "resolved_at": "2026-11-22T03:07:48.889Z",
    "milestone": "v2.0"
  },
  {
    "id": 20,
    "kind": "stub",
    "phase": "11",
    "file": "host/disc_ecm.c",
    "line": 175,
    "description": "ECM sector reads remain unsupported until Plan 11-03 validates its sector mapping.",
    "status": "fixed",
    "reason": "",
    "recorded_at": "2026-11-22T02:07:28.247Z",
    "resolved_at": "2026-11-22T03:07:49.015Z",
    "milestone": "v2.0"
  },
  {
    "id": 21,
    "kind": "stub",
    "phase": "11",
    "file": "host/disc_ccd.c",
    "line": 171,
    "description": "CloneCD v3 track metadata remains unsupported until Plan 11-04 validates tracks and companions.",
    "status": "fixed",
    "reason": "",
    "recorded_at": "2026-11-22T02:07:28.377Z",
    "resolved_at": "2026-11-22T13:35:34.529Z",
    "milestone": "v2.0"
  },
  {
    "id": 22,
    "kind": "stub",
    "phase": "11",
    "file": "host/disc_ccd.c",
    "line": 188,
    "description": "CloneCD TOC generation remains unsupported until Plan 11-04 validates the descriptor tracks.",
    "status": "fixed",
    "reason": "",
    "recorded_at": "2026-11-22T02:07:28.513Z",
    "resolved_at": "2026-11-22T13:35:43.098Z",
    "milestone": "v2.0"
  },
  {
    "id": 23,
    "kind": "stub",
    "phase": "11",
    "file": "host/disc_ccd.c",
    "line": 198,
    "description": "CloneCD sector reads remain unsupported until Plan 11-04 validates its sector mapping.",
    "status": "fixed",
    "reason": "",
    "recorded_at": "2026-11-22T02:07:28.648Z",
    "resolved_at": "2026-11-22T13:35:43.206Z",
    "milestone": "v2.0"
  },
  {
    "id": 24,
    "kind": "stub",
    "phase": "11",
    "file": "host/disc_ecm.c",
    "line": 222,
    "description": "ECM type-1/2/3 record decoding remains unsupported pending independent output vectors in Plan 11-05.",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-11-22T03:12:50.763Z",
    "resolved_at": null,
    "milestone": "v2.0"
  },
  {
    "id": 25,
    "kind": "deviation",
    "phase": "13",
    "file": "tests/core/test_state.c",
    "line": null,
    "description": "Task 2 validation assertions were green immediately because shared board-state validation landed in Task 1; no separate Task 2 RED run was fabricated.",
    "status": "waived",
    "reason": "Task 2 adds planned regression coverage for validation implemented in Task 1; all negative cases passed the exact focused selector, and the sequencing exception is recorded in 13-04-SUMMARY and STATE. No test or behavior gap remains.",
    "recorded_at": "2026-11-23T17:38:49.703Z",
    "resolved_at": "2026-11-23T17:39:13.635Z",
    "milestone": "v2.0"
  },
  {
    "id": 26,
    "kind": "deviation",
    "phase": "15",
    "file": "src/pad.c",
    "line": null,
    "description": "Raw fixed-width signed gun coordinates are required so the state visitor reproduces saved bytes.",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-11-25T15:06:00.729Z",
    "resolved_at": null,
    "milestone": "v2.0"
  },
  {
    "id": 27,
    "kind": "deviation",
    "phase": "15",
    "file": "tests/core/test_state.c",
    "line": null,
    "description": "Expanded the test-only state field-record capacity to cover the new serialized gun fields.",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-11-25T15:07:38.675Z",
    "resolved_at": null,
    "milestone": "v2.0"
  },
  {
    "id": 28,
    "kind": "stub",
    "phase": "16",
    "file": "tests/disc/patch.c",
    "line": 26,
    "description": "Patch selector entry points fail until Plans 07-09 implement the source mapping and patch parsers.",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-11-26T00:26:23.287Z",
    "resolved_at": null,
    "milestone": "v2.0"
  },
  {
    "id": 29,
    "kind": "stub",
    "phase": "16",
    "file": "tests/runner/cheats.c",
    "line": 54,
    "description": "Cheat parser, bounds and order selectors fail until Plan 03 implements the shared bounded write table.",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-11-26T00:26:23.333Z",
    "resolved_at": null,
    "milestone": "v2.0"
  },
  {
    "id": 30,
    "kind": "stub",
    "phase": "16",
    "file": "tests/runner/tests.cmake",
    "line": 37,
    "description": "Runner cheat and patch CLI/hash modes fail until Plans 06 and 10 implement the options and hash behavior.",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-11-26T00:26:23.378Z",
    "resolved_at": null,
    "milestone": "v2.0"
  },
  {
    "id": 31,
    "kind": "stub",
    "phase": "16",
    "file": "tests/libretro/tests.cmake",
    "line": 444,
    "description": "Libretro cheat callback and state selectors await the host behavior in Plans 04-05.",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-11-26T00:26:23.425Z",
    "resolved_at": null,
    "milestone": "v2.0"
  },
  {
    "id": 32,
    "kind": "stub",
    "phase": "16",
    "file": "tests/compat/tests.cmake",
    "line": 1292,
    "description": "DAT inventory and capability evidence script modes fail until Plans 12-15 implement accounting and evaluation.",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-11-26T00:26:23.471Z",
    "resolved_at": null,
    "milestone": "v2.0"
  },
  {
    "id": 33,
    "kind": "stub",
    "phase": "16",
    "file": "tests/fuzz/tests.cmake",
    "line": 337,
    "description": "Patch and cheat replay executables are absent until Plan 11 adds the replay targets.",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-11-26T00:26:23.515Z",
    "resolved_at": null,
    "milestone": "v2.0"
  }
]
````
