---
phase: 07-faster-and-more-trustworthy-ci
plan: "09"
subsystem: documentation
tags: [quality-baselines, CI, roadmap, maintainer-guidance, architecture-evidence]
requires:
  - phase: "07-04"
    provides: Evidence-backed quality review and verified README correction
  - phase: "07-05"
    provides: Read-only weekly triage artifact workflow
  - phase: "07-06"
    provides: Release App route, runner and pin audit evidence
  - phase: "07-07"
    provides: Measured quality producers and shared comparator
  - phase: "07-08"
    provides: Exact-attempt CI critical-path measurement
provides:
  - Current maintainer and test documentation for CI, triage, quality, timeouts, and budget checks
  - Evidence-linked Phase 7 roadmap and prompt-coverage updates
  - Corrected owned STR integration statement with explicit external conformance limits
affects: [Phase 7 ship, v2.0 milestone roadmap]
actuals:
  tokens: 17160
  tasks: 2
  commits: 2
commits: 2
plan_head_before: 98d0b9c829a9eb189706587285d9be149ca535d9
plan_head_after: 11a1251261e0c71fc0ce03702632aa27ffe08a21
range_commits: 2
tech-stack:
  added: []
  patterns:
    - Reconcile docs to current workflow source and attributable evidence without claiming new hosted runs
    - Keep device-speed assumptions and browser performance distinct from deterministic instruction and hash checks
key-files:
  created:
    - .planning/phases/07-faster-and-more-trustworthy-ci/07-09-SUMMARY.md
  modified:
    - .planning/phases/07-faster-and-more-trustworthy-ci/07-09-PLAN.md
    - .planning/phases/07-faster-and-more-trustworthy-ci/07-UPGRADE-AUDIT.md
    - .planning/preparation/ARCHITECTURE.md
    - .planning/preparation/ENGINEERING.md
    - .planning/preparation/PROMPT-COVERAGE.md
    - .planning/PRODUCT-ROADMAP.md
    - .planning/ROADMAP.md
    - .planning/STATE.md
    - docs/maintaining.md
    - docs/testing.md
key-decisions:
  - Keep the existing README install command; it still matches the automated install-line control and attributable release run.
  - Leave the linked WASI module size unbounded until the actual Release module is measured.
  - Keep device MIPS assumptions explicitly unverified and retain the low-power implementation gap in SEED-005.
requirements-completed:
  - TUNE-04
  - TUNE-06
coverage:
  - id: C1
    description: Maintainer and testing guidance matches current CI, triage, measurement, timeout, install and budget paths.
    requirement: TUNE-06
    verification:
      - kind: other
        ref: "git diff --check HEAD (exit 0); cmake -DROOT=. -P tests/cmake/budgets.cmake (exit 0; docs/testing.md 59,879/61,440 bytes)"
        status: pass
      - kind: integration
        ref: "cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3900 -P tests/cmake/line_budget.cmake (exit 0; 3,890/3,900)"
        status: pass
      - kind: integration
        ref: "cmake -DROOT=. '-DPATTERNS=tests/retroarch/*' -DLIMIT=300 -P tests/cmake/line_budget.cmake (exit 0; 300/300)"
        status: pass
    human_judgment: false
  - id: C2
    description: The README install invocation remains unchanged and matches its real release consumer.
    requirement: TUNE-06
    verification:
      - kind: integration
        ref: "tests/cmake/install_line.cmake source comparison and existing install-line-latest job in https://github.com/szTheory/schwaystation/actions/runs/44262491280; no new hosted run claimed"
        status: pass
    human_judgment: false
  - id: C3
    description: The Phase 6 owned disc_boot STR route is documented without claiming full external movie/frame conformance or hardware fidelity.
    requirement: TUNE-06
    verification:
      - kind: other
        ref: "Archived Phase 6 Plan 06-16 summary and DISC-03 at .planning/milestones/v1.0-phases/06-discs-and-fmv/06-16-SUMMARY.md and .planning/milestones/v1.0-REQUIREMENTS.md#disc-03"
        status: pass
    human_judgment: false
  - id: C4
    description: Hosted triage artifact, matching release-PR critical path, and actual linked WASI module size remain explicit ship-time evidence gaps.
    requirement: TUNE-05
    verification:
      - kind: other
        ref: "No post-merge triage artifact run, matching Phase 7 release-PR timing run, or actual Ubuntu 26 WASI module byte count is claimed; pending ship"
        status: unknown
    human_judgment: false
metrics:
  duration: "about 27 minutes wall elapsed; exact start and active time not captured across handoff"
  completed: 2026-11-19
status: complete
---

# Phase 7 Plan 9: Refresh maintainer and measurement records Summary

**Maintainer docs, quality records and roadmap now reflect the current CI paths and measured values while preserving the hardware and external guest evidence limits.**

## Performance

- **Duration:** about 27 minutes wall elapsed; exact start and active time were not captured across the executor handoff.
- **Completed:** 2026-11-19
- **Tasks:** 2
- **Files modified:** 11, including this summary, the bounded plan revision, and required planning metadata; the pre-existing `.planning/config.json` user edit is excluded.
- **Runtime/model:** Codex/OpenGSD CLI 1.20.0; executor gpt-6-luna at xhigh effort.

## Accomplishments

- Updated maintainer and testing docs for the Saturday/manual triage artifact, exact-attempt CI timing, current quality producers, timeout evidence, and gate/RetroArch budget commands. The README install invocation was verified unchanged against its existing control and release evidence.
- Updated engineering, product roadmap, and prompt coverage with the measured 1,619,573 testcard and 137,752,334 `gpu_motion` Ir/frame, 3,755,020 serialized-state bytes, and 2,249–3,293-second comparable historical CI push path. Sustained device MIPS remain assumptions; `gpu_motion` exceeds the provisional budgets for the listed low-power targets.
- Corrected ARCHITECTURE §10: the owned `disc_boot` fixture exercises DMA3 → RAM → DMA0 → MDEC → DMA1 → RAM → DMA2 → native 24-bit display. The Phase 6 summary and DISC-03 support that integration claim only; full decoded output and hardware conformance for pinned external frame/movie guests remain unproven.
- Recorded in the upgrade audit that the install line, settings, pins, runner labels, and hardware pages were unchanged for this documentation/CI-only scope. Existing hardware references remain applicable; no emulator behavior changed.

## Verification

- `git diff --check HEAD` — exit 0.
- `cmake -DROOT=. -P tests/cmake/budgets.cmake` — exit 0; `docs/testing.md` is 59,879/61,440 bytes and `docs/maintaining.md` is 19,154/61,440 bytes.
- Repository-check line budget — exit 0; 3,890/3,900 lines.
- RetroArch end-to-end line budget — exit 0; 300/300 lines.
- The first combined shell invocation stopped at zsh glob expansion for the RetroArch pattern; rerunning it with the pattern quoted passed. No code or test changes resulted.
- No hosted workflow or full test suite was started. Existing linked install-line evidence is release run 44262491280.

## Deferred for ship

The scheduled or manual post-merge triage artifact still needs an attributable successful run. The matching Phase 7 release-App PR must provide a comparable critical-path measurement, and the Ubuntu 26 Release WASI producer must report the linked runner module's byte count before `size.wasm` receives a bound. No browser-speed result is inferred from Wasmtime hash equality.

## Task commits

1. `b9a4ecd` — bounded Plan 07-09 revision adding ARCHITECTURE.md to the named scope.
2. `11a1251` — Plan 07-09 documentation and evidence updates.

Plan head before: `98d0b9c`. Plan head after: `11a1251`. The measured Plan 07-09 range contains two commits.

## Deviations from Plan

The README install command and hardware pages required no edits: source comparison and attributable existing evidence confirmed they remain applicable. No stale hardware, security, or compatibility claim outside the plan's named files was found. `.planning/config.json` remains an uncommitted pre-existing user edit.

## Self-Check: PASSED

The summary, plan and audit files exist; commits `b9a4ecd` and `11a1251` are ancestors of `HEAD`; all prescribed local doc and line-budget checks passed.
