---
phase: 08-per-game-card-saves
plan: "07"
subsystem: libretro-ci
tags: [retroarch, card-saves, github-actions, e2e]
requires:
  - phase: 08-05
    provides:
      - Per-game libretro card persistence and fresh-instance card reload coverage
provides:
  - Bounded two-process RetroArch acceptance for both memory-card slots
  - Required CI artifact and workflow wiring for the card guest
affects: [phase-08-verification, phase-08-ship, retroarch-ci]
actuals:
  tokens: 1484
  tasks: 2
  commits: 2
  plan_head_before: cb99a4b0d900fa720d99fa0cb71acae91ddaede2
  plan_head_after: d8bcc7993d9ec89a36d0b674c989f5c05aec38ee
tech-stack:
  added: []
  patterns:
    - Reuse one isolated RetroArch setup for fresh sequential frontend processes
    - Preflight required guest artifacts before media builds and frontend launches
key-files:
  created: []
  modified:
    - .github/workflows/ci.yml
    - docs/testing.md
    - tests/cmake/workflows.cmake
    - tests/retroarch/run_retroarch.cmake
    - tests/retroarch/test.cfg.in
    - tests/retroarch/verify.py
key-decisions:
  - Keep both card files in the isolated save directory and disable content/core subfolders.
  - Keep the existing five-minute RetroArch job timeout until the new host run measures the added launches.
requirements-completed: [SAVE-04]
coverage:
  - id: SAVE-04-retroarch-wiring
    description: The required RetroArch job receives the round-trip guest and invokes the two-process card persistence scenario with bounded logs.
    requirement: SAVE-04
    verification:
      - kind: integration
        ref: "cmake -DCASE=spu_registration -DROOT=\"$PWD\" -P tests/cmake/workflows.cmake (exit 0)"
        status: pass
      - kind: other
        ref: "run_retroarch.cmake MODE=cards with a throwaway frontend, two launches and exact card-size/counter assertions (exit 0)"
        status: pass
    human_judgment: false
duration: "at least 318min"
completed: 2026-11-19
status: complete
---

# Phase 08 Plan 07: Two-process RetroArch card persistence Summary

**The required macOS RetroArch job now downloads the round-trip guest and checks that two fresh frontend processes advance both fixed-size card files.**

## Performance

- **Observed wall interval:** At least 5h17m49s from the earliest retained implementation edit (2026-11-19T12:18:34Z) through the second task commit (2026-11-19T17:36:23Z). The original GSD start and active time were not recorded, so this is a lower bound rather than a full runtime breakdown.
- **Tasks:** 2.
- **Files modified:** 6.
- **Runtime/model:** Codex and OpenGSD CLI 1.20.0; primary GPT-6 session, exact deployed model ID unavailable.

## Accomplishments

- Added a `cards` mode that creates one isolated RetroArch root, launches the same guest in two separate processes, and checks card sizes and slot counters after each process exits. Existing screenshot, pixel, dimensions, and replay checks remain in the same counted harness at 300/300 lines.
- Added the guest to the existing short-lived fixture artifact, checks all three downloaded guests before the disc build and frontend launches, runs the card case under pinned RetroArch 1.22.2, and collects bounded logs on failure.
- Updated the existing workflow regression selector and the maintainer testing guide.

## Verification Evidence

| Check | Result | Evidence |
| --- | --- | --- |
| Harness budget and Python syntax | Exit 0; 300/300 lines; AST parsed | `cmake -DROOT=. '-DPATTERNS=tests/retroarch/*' -DLIMIT=300 -P tests/cmake/line_budget.cmake`; `python3 -B -c 'import ast; ast.parse(open("tests/retroarch/verify.py", encoding="utf-8").read())'` |
| Workflow wiring | Exit 0; fixture, preflight, card mode, required RetroArch job, and existing registration checks connected | `cmake -DCASE=spu_registration -DROOT="$PWD" -P tests/cmake/workflows.cmake` |
| Action pins and YAML syntax | Exit 0; 60 action uses pinned; YAML parser accepted `ci.yml` | `cmake -DMODE=pins -DDIR=.github/workflows -P tests/cmake/workflows.cmake`; Python `yaml.safe_load` of `.github/workflows/ci.yml` |
| Save claim | Exit 0 against the fresh CI configure output | `cmake -DINFO_FILE=build/ci/libretro/schwaystation_libretro.info -DINVENTORY=tests/libretro/inventory.txt -P tests/cmake/info_claims.cmake` |
| Guest preflight | Exit 0; source SHA-256 matched the fixture manifest; assembled file is a valid 4,096-byte PS-X EXE with a matching 2,048-byte payload declaration | `cmake --build --preset ci --target schwaystation-fixtures`; one-off source digest and PS-X EXE header checks |
| Driver smoke | Exit 0; both fake frontend launches produced 131,072-byte cards with counters 01/02 then 02/03 | One-off `run_retroarch.cmake -DMODE=cards` run using a temporary fake frontend; this checks driver flow, not RetroArch compatibility |
| Repository gate budget and diff | Exit 0; 3,901/3,901 repository-check lines; `git diff --check HEAD` clean | `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake`; `git diff --check HEAD` |

No hosted RetroArch run was started in this plan; the real frontend proof is collected by Plan 08-10.

## CI Efficiency Review

Historical required RetroArch job [run 44253375502](https://github.com/szTheory/schwaystation/actions/runs/44253375502) took 101 seconds. Its largest visible step was the checked disc-fixture build at 34 seconds; the disc boot took 18 seconds. The two added frontend launches each have a 50-second process timeout, so the conservative bound is 101 + 100 = 201 seconds against the existing 300-second job timeout. The timeout remains unchanged until the card scenario is measured on the hosted runner.

The disc build is a possible critical-path improvement, but moving it into a producer job would change the dependency graph and runner usage. This plan keeps the graph unchanged; measure the changed-head run in 08-10 before deciding whether that move reduces total time or cost.

## Task Commits

1. **Task 1: Add a two-launch save/continue case to the bounded harness** — `f9988e1`.
2. **Task 2: Wire the owned card guest into the required hosted job** — `d8bcc79`.

## Deviations from Plan

Added the maintainer guide row and expanded the existing workflow consistency selector so the new artifact and preflight wiring remain discoverable and regression-checked. No runtime assertion or existing host case was removed.

## Issues Encountered

The delegated executor stopped at the 300-line harness limit and the worktree dispatch failed in this environment. The shared harness was refactored inline without removing coverage. One early `.info` check read a stale generated build file and failed; a fresh CI configure produced the current metadata and the claim check passed. The preceding orchestration did not retain active-time or start-time measurements.

## Next Phase Readiness

The card scenario is wired and its driver control flow is checked. Continue with Plan 08-08; run and inspect the real pinned RetroArch scenario with the final changed head in Plan 08-10.

---
*Phase: 08-per-game-card-saves*
*Completed: 2026-11-19*
