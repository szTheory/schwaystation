---
phase: 13-regions-console-revisions-and-playback
plan: "03"
subsystem: machine-profile
tags: [profiles, ram-control, gpu, cdrom, ctest]
requires:
  - phase: 13-regions-console-revisions-and-playback
    provides: "API v8 board selection and the per-instance defaults table"
provides:
  - "Board-selected RAM_SIZE is applied before the first frame and on power reset"
  - "CD Test 22h returns the vC0 unsupported response and keeps region strings on vC1+"
affects: [phase-13-profile-verification]
actuals:
  tokens: 7221
  tasks: 2
  commits: 7
  plan_head_before: fc67c35a1ae37f94bf433c0a895cd77f5dd3ae5f
  plan_head_after: cc256122959da2c105853794c8c4e0fa59009dd5
tech-stack:
  added: []
  patterns:
    - "Apply board-derived register defaults at profile selection and reset"
    - "Test per-board capability responses while holding region constant"
key-files:
  created: []
  modified:
    - src/instance.c
    - src/bus.c
    - src/cdrom.c
    - tests/core/test_core.c
    - tests/cdrom/model.c
    - tests/cdrom/tests.cmake
    - tests/cdrom/inventory.txt
    - .planning/phases/13-regions-console-revisions-and-playback/13-03-PLAN.md
    - .planning/phases/13-regions-console-revisions-and-playback/13-PLAN-CHECK.md
key-decisions:
  - "Apply RAM_SIZE immediately when a supported board is selected, then reuse the same board value on power reset."
  - "Gate Test 22h only for vC0 using the documented INT5 (11h,10h); leave Test 20h and the unpinned Test 51h response unchanged."
requirements-completed: []
coverage:
  - id: D1
    description: "All nine board profiles apply RAM_SIZE before first frame and after reset; legacy profiles inherit PU-7's 0x0B80 value, and GPU v0/v2 behavior remains covered."
    requirement: PROFILE-02
    verification:
      - kind: unit
        ref: "ctest --preset ci --no-tests=error -R '^(core[.]profile[.]machine|gpu[.](reference|span)[.].*)$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: "All nine board/GPU profiles select their expected CD firmware generation; vC0 Test 22h returns INT5 (11h,10h), vC1+ returns the fixed-region string, and cdrom.region preserves separate region coverage."
    requirement: PROFILE-02
    verification:
      - kind: unit
        ref: "ctest --preset ci --no-tests=error -R '^(cdrom[.]profile[.]firmware|cdrom[.]region|core[.]profile[.]machine)$' --output-on-failure"
        status: pass
    human_judgment: false
metrics:
  duration: "59m22s wall interval from the saved STATE checkpoint at 2026-11-23T16:14:55Z to the final implementation evidence at 2026-11-23T17:14:17Z; executor start and active time were not separately captured"
  completed: 2026-11-23
  status: complete
---

# Phase 13 Plan 03: Board-Driven RAM, GPU and CD Defaults Summary

Board selection now applies RAM_SIZE before the first frame, retains GPU behavior, and gates the vC0 CD region query without coupling the region string to board identity.

## Performance

- **Runtime/model:** Codex GSD executor; configured `gpt-6-luna` xhigh; primary runtime model ID unavailable.
- **Duration:** 59m22s from the saved STATE checkpoint at 2026-11-23T16:14:55Z through the final implementation evidence at 2026-11-23T17:14:17Z; executor start and active time were not separately captured.
- **Tasks:** 2
- **Implementation files:** 7
- **Actuals:** 28,887 realized diff characters / 4 = 7,221 estimate-scale tokens; 7 plan commits measured from the persisted ledger.

## Accomplishments

- Set the initial RAM_SIZE register when an explicit board profile is applied and use that same board default during power reset. Tests check all nine boards before and after reset.
- Verified old-sized and board-zero profiles inherit PU-7's non-default 0x0B80 RAM_SIZE before and after reset. Existing GPU-visible v0/v2 checks and board/GPU rejection coverage remain in the core selector.
- Connected the existing CD firmware generation to Test 22h capability: vC0 returns INT5 `(0x11, 0x10)`, while vC1+ returns the region-derived response. The new `cdrom.profile.firmware` case covers all nine board/GPU tuples at fixed NTSC-U; `cdrom.region` stays selected independently.
- Kept Test 20h's existing complete date/version response and Test 51h's unmodeled signal-processor pass-through unchanged. The latter has no pinned expected response.
- PROFILE-02 remains pending until Phase 13's state serialization, validation, and deterministic-hash work and full phase verification are complete.

## Plan and Task Commits

- `c721b7f` — corrected the profile test selector graph.
- `57e54ce` — narrowed the board-profile implementation target.
- `c30cbb6` — added the required v2 GPU behavior assertion to the checked task.
- `2b6596a` — amended the task for the initial RAM_SIZE register seam.
- `7658e6a` — applied and tested initial/reset board RAM defaults and GPU behavior.
- `1ede7d2` — scoped CD coverage to the source-backed vC0 Test 22 response.
- `cc25612` — gated Test 22h by firmware generation and registered its profile test.

## Test Evidence

- Task 1 preflights: the profile/default table had 9 rows, both PU-7 and early PU-8 used 0x0B80, the core/GPU inventories were sorted, the line-budget gate passed at 3,901/3,901, and the exact selector graph contained 14 names including both disc fixtures. All exited 0.
- Task 1 final build: `cmake --build --preset ci --target schwaystation-test-core schwaystation-test-gpu` exited 0 in 0.45s. The exact CTest selection `ctest --preset ci --no-tests=error -R '^(core[.]profile[.]machine|gpu[.](reference|span)[.].*)$' --output-on-failure` passed 14/14 in 0.13s.
- Before the first-boot seam was added, an earlier version of Task 1 passed the same targets in 0.50s and 14/14 selectors in 0.15s. Those results are pre-change evidence only; the final 0.45s build and 14/14 run above are the acceptance evidence.
- Task 2 preflights: firmware generation counts were `[2,1,2,4]` across 9 board rows; core/CD-ROM inventories were sorted; `git diff --check` passed; and the line-budget gate passed at 3,901/3,901. All exited 0.
- Task 2 reconfigure: `cmake --preset ci` exited 0 in 0.81s. The exact `ctest --preset ci --show-only=json-v1` assertion found the five expected names (`cdrom.profile.firmware`, `cdrom.region`, `core.profile.machine`, and the two disc fixtures) in 0.05s.
- Task 2 build: `cmake --build --preset ci --target schwaystation-test-cdrom schwaystation-test-core` exited 0 in 0.25s. The focused CTest command `ctest --preset ci --no-tests=error -R '^(cdrom[.]profile[.]firmware|cdrom[.]region|core[.]profile[.]machine)$' --output-on-failure` passed 5/5 in 0.12s.
- The first exact-selector shell wrapper had an unmatched quote and exited 1 before invoking CTest; the corrected Python wrapper then passed the exact five-name assertion. No test was lost or rerun.
- No workflow changed, so hosted CI, Act, and Docker were not used. Local verification has no hosted run URL.

## Decisions Made

- Applied the selected board's RAM control during profile selection as well as reset because the host may choose a board after instance creation but before the first frame.
- Used only the documented vC0 Test 22h unsupported response. Test 20h's date/version tuple is coherent and was left intact; Test 51h was left alone because its expected signal-processor response is not pinned.

## Deviations from Plan

The plan was corrected during execution when the checked source and CTest graph exposed selector/target mismatches, the missing first-boot RAM register update, and the need to bound CD behavior to the pinned vC0 response. Those corrections were committed before the final implementations and acceptance runs. No hosted workflow changes or extra hardware behavior were added.

## User Setup Required

None.

## Next Plan Readiness

Plan 13-03 is complete. Plan 13-04 is next. PROFILE-02 remains pending; this plan supplies board-default behavior and test evidence but does not establish state/hash coverage or full phase completion.

## Self-Check: PASSED

The summary file was created, both task commits `7658e6a` and `cc25612` are ancestors of the current branch head, and the seven plan commits match the persisted ledger count.

---
*Phase: 13-regions-console-revisions-and-playback*
*Completed: 2026-11-23*
