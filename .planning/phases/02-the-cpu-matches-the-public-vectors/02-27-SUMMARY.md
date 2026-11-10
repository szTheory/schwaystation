---
phase: 02-the-cpu-matches-the-public-vectors
plan: 27
subsystem: ci
tags:
  - ci
  - nightly
  - timeouts
  - hosted-runners
  - todos

requires:
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: the finished phase on the branch (02-01 to 02-26), the draft probe pull request #7 (02-09), the nightly vectors-full and fuzz jobs (02-15, 02-22)
provides:
  - a green CI run on the branch head across all eight lanes, with CI required and hash-equality success
  - a green nightly dispatch on the branch with vectors-full and fuzz success
  - measured timeouts for the jobs the phase grew, each naming a run ID of this phase
  - the fresh local ci workflow time for ship
  - the probe pull request closed unmerged and five todos moved to completed
affects:
  - ship

actuals:
  tokens: 2500
  tasks: 3
  commits: 2

plan_head_before: a91d542dc6166f8572dcbaa89badb1912a62ec27
plan_head_after: a89637d9fc64af235d804f98747a174df0973a00
commits: 2

tech-stack:
  added: []
  patterns:
    - "a job timeout is twice the slowest successful duration among this phase's runs, rounded up to whole minutes and never below the job's floor, with the run ID of that slowest run in the comment"
    - "a hosted run is polled by one process at a time, 20 to 30 seconds apart, because the API rate limit is per user and shared with every other project on the machine"

key-files:
  created: []
  modified:
    - .github/workflows/ci.yml
    - .github/workflows/nightly.yml
    - .planning/todos/completed/007-exit-table-gate.md
    - .planning/todos/completed/008-api-names-gate.md
    - .planning/todos/completed/019-dev-preset-fortify-helpers.md
    - .planning/todos/completed/020-testing-doc-rehearsal-rows.md
    - .planning/todos/completed/023-test-helper-exit-status.md

key-decisions:
  - "The build timeout rises from 7 to 11 minutes because the windows-x64 leg took 311 s in run 42725764966; the same leg took 107 s in the next run, so the number is the slowest measured, not typical"
  - "The hygiene timeout rises from 1 to 2 minutes: the job now runs 21 tests and took 31 s, so the old minute was less than twice its time"
  - "The rehearsal timeout rises from 11 to 13 minutes: its slowest phase run was 386 s against the 328 s the old value was derived from"
  - "asan, be32, wasm32, vectors-full and fuzz keep their 5-minute floors, and nofp keeps its 1 minute; their comments now name this phase's runs"
  - "CMakePresets.json is unchanged: no measured test needed a different SCHWAYSTATION_TIMEOUT_SCALE"
  - "workflow.test_gate_timeout is left for ship to re-derive from the 8 s local time recorded here; .planning/config.json was not touched"

requirements-completed:
  - CPU-01
  - CPU-02
  - CPU-03
  - CPU-04
  - CPU-05

coverage:
  - id: D1
    description: "The branch head has a green CI run: CI required and hash-equality succeed, and on all six build legs, be32 and wasm32 the vectors, cpu, bus, exe, fuzz and fixtures tests pass with the JUnit check seeing every inventory test once"
    requirement: "CPU-01"
    verification:
      - kind: e2e
        ref: "https://github.com/szTheory/schwaystation/actions/runs/42729199629 (head a89637d, conclusion success, 20 of 20 jobs success); gh run list --commit on the head with --json conclusion gives success (exit 0); the CI required and hash-equality count is 2 (exit 0)"
        status: pass
      - kind: e2e
        ref: "https://github.com/szTheory/schwaystation/actions/runs/42725764966 (head a91d542, conclusion success, 20 of 20 jobs success)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The nightly dispatched on the branch concludes success with vectors-full and fuzz green"
    requirement: "CPU-02"
    verification:
      - kind: e2e
        ref: "gh workflow run nightly.yml --ref phase/02-the-cpu-matches-the-public-vectors (exit 0), run https://github.com/szTheory/schwaystation/actions/runs/42728718880 concluded success (vectors-full 39 s, fuzz 72 s)"
        status: pass
    human_judgment: false
  - id: D3
    description: "Each timeout the phase grew is twice its slowest measured run with a run ID of this phase, and no added line reuses a run ID already on origin/main"
    requirement: "CPU-03"
    verification:
      - kind: integration
        ref: "cmake --workflow --preset hygiene (exit 0) and the plan's added-line check over git diff origin/main on the two workflow files (exit 0, five distinct run IDs, none on origin/main)"
        status: pass
    human_judgment: false
  - id: D4
    description: "A fresh local ci workflow after rm -rf build/ci is timed once for ship"
    requirement: "CPU-04"
    verification:
      - kind: integration
        ref: "rm -rf build/ci then cmake --workflow --preset ci: exit 0 in 8 s, 240 of 240 tests, check_junit.cmake against tests/inventory/ci.txt exit 0"
        status: pass
    human_judgment: false
  - id: D5
    description: "The probe pull request is closed unmerged and the five closed todos are in completed, with 022 still pending"
    requirement: "CPU-05"
    verification:
      - kind: other
        ref: "gh pr close 7 --comment (exit 0); test of gh pr view 7 state equal to CLOSED (exit 0, mergedAt null); the plan's todo ls and grep checks (exit 0)"
        status: pass
    human_judgment: false
---

# Phase 2 Plan 27: Hosted runs, measured timeouts and the todo moves Summary

**The finished phase is green on every hosted lane (CI run 42729199629 with CI required and hash-equality success, nightly dispatch 42728718880 with vectors-full and fuzz success), the jobs it grew carry timeouts measured from this phase's runs, the probe pull request is closed unmerged, and the local ci workflow takes 8 s for ship to record.**

## Performance

- **Duration:** about 37 min, nearly all waiting on hosted CI and one hour-window API rate limit (see Issues Encountered); authoring was a few minutes
- **Started:** 2026-11-10T00:05Z
- **Completed:** 2026-11-10T00:43Z
- **Tasks:** 3
- **Files modified:** 7 (the plan listed 13; CMakePresets.json and the five `pending` paths are only the move's sources)

## Run URLs

| What | Head commit | Run | Result |
|------|-------------|-----|--------|
| CI, after the first push | a91d542 | https://github.com/szTheory/schwaystation/actions/runs/42725764966 | success, 20 of 20 jobs |
| Nightly, dispatched on the branch | a91d542 | https://github.com/szTheory/schwaystation/actions/runs/42728718880 | success, 7 of 7 jobs |
| CI, on the final head | a89637d | https://github.com/szTheory/schwaystation/actions/runs/42729199629 | success, 20 of 20 jobs |

No failure needed a fix: the first run was green, so no code changed for a hosted-only reason.

### CI job durations (seconds)

The first column is run 42725764966, the second the final head's run 42729199629.

| Job | First | Final | Job | First | Final |
|-----|-------|-------|-----|-------|-------|
| changes | 4 | 6 | wasm32 | 46 | 57 |
| hygiene | 31 | 27 | hash-equality | 5 | 5 |
| title | 4 | 6 | retroarch | 22 | 24 |
| build linux-x64 | 61 | 35 | perf | 23 | 31 |
| build linux-arm64 | 33 | 35 | rehearsal | 365 | 311 |
| build macos-arm64 | 54 | 47 | cleanup | 9 | 8 |
| build macos-x64 | 78 | 79 | nofp ubuntu-26.04 | 7 | 10 |
| build windows-x64 | 311 | 107 | nofp ubuntu-26.04-arm | 23 | 7 |
| build windows-arm64 | 102 | 104 | asan | 25 | 26 |
| be32 | 91 | 66 | CI required | 4 | 3 |

On both runs `CI required` and `hash-equality` succeeded and every build leg, be32 and wasm32 ran the JUnit check after its tests.

### Nightly job durations (seconds), run 42728718880

| Job | Seconds | Result |
|-----|---------|--------|
| vectors-full | 39 | success |
| fuzz | 72 | success |
| bench (ubuntu-26.04) | 37 | success |
| bench (ubuntu-26.04-arm) | 35 | success |
| bench (macos-15) | 63 | success |
| bench-wasm32 | 126 | success |
| report | 4 | success |

## Timeouts

Slowest successful durations come from five green CI runs and six green nightly runs of this phase (42691517611, 42689688852, 42722584020, 42724296965, 42725764966 and the nightly runs 42703494243, 42720996208, 42722583700, 42724297002, 42725764976, 42728718880). At the start of this plan `ci.yml` had 14 timeout lines naming phase-1 run IDs (the baseline); it still has 14 lines with a run ID, now all of this phase for the ones below, and `nightly.yml` has 5.

| Job | Old minutes | New minutes | Slowest run (s) | Run ID |
|-----|-------------|-------------|-----------------|--------|
| build (six legs, slowest windows-x64) | 7 | 11 | 311 | 42725764966 |
| hygiene | 1 | 2 | 31 | 42725764966 |
| rehearsal | 11 | 13 | 386 | 42691517611 |
| asan | 5 | 5 (floor) | 28 | 42722584020 |
| be32 | 5 | 5 (floor) | 102 | 42724296965 |
| wasm32 | 5 | 5 (floor) | 60 | 42691517611 |
| nofp | 1 | 1 | 28 | 42691517611 |
| vectors-full (nightly) | 5 | 5 (floor) | 49 | 42720996208 |
| fuzz (nightly) | 5 | 5 (floor) | 81 | 42720996208 |

The armips cost table of plan 02-09 predicted the macos-x64 leg as the slowest; hosted variance put windows-x64 above it. `CMakePresets.json` is unchanged because no measured test needed another `SCHWAYSTATION_TIMEOUT_SCALE`.

## Local ci time for ship

`rm -rf build/ci` then `cmake --workflow --preset ci`: **8 s, exit 0**, 240 of 240 tests, and `check_junit.cmake` against `tests/inventory/ci.txt` said "240 tests ran once each" (exit 0). Ship re-derives `workflow.test_gate_timeout` as twice that time with GSD's config writer under an empty `GSD_HOME`; this plan did not edit `.planning/config.json`.

## Accomplishments

- The phase passes on every hosted lane: both CI runs (first push head and final head) and the nightly dispatch are green, which shows ROADMAP success criteria 1 to 5 on all six build legs, be32, wasm32, asan and both nofp legs, and hash-equality over eight files.
- Nine job timeouts now cite measured durations of this phase; three of them changed value (build, hygiene, rehearsal), the other six kept their floor.
- Probe pull request #7 is closed unmerged: `gh pr close 7 --comment "Probe for the phase 2 fixture toolchain; closed unmerged before ship."` exit 0, state CLOSED, mergedAt null, and the branch `phase/02-the-cpu-matches-the-public-vectors` is untouched (remote head a89637d).
- Todos 007, 008, 019, 020 and 023 moved with `git mv` to `.planning/todos/completed/`, each with `status: completed`, a date, `closed_by` and a Completion section; 007 and 008 move because plan 02-26 deferred neither. 022 stays pending.

## Task Commits

1. **Task 1: Tracer, push the finished branch and watch its CI run** - no commit of its own; the work is the push of a91d542 and run 42725764966 (success)
2. **Task 2: Dispatch the nightly on the branch** - no commit; the work is dispatch 42728718880 (success)
3. **Task 3: Measured timeouts, local ci time, probe close, todo moves** - `c2d9a7b` (docs, the todo moves) and `a89637d` (ci, the timeouts)

**Plan metadata:** recorded in the docs commit that carries this file.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Two timeouts outside the plan's list were below twice their measured run**
- **Found during:** Task 3
- **Issue:** hygiene (1 minute against 31 s measured, so under 2x) and rehearsal (11 minutes against 386 s, whose old value came from 328 s) were not on the plan's list of grown jobs, but the same rule (twice the slowest measured run) is not met by them.
- **Fix:** hygiene 1 to 2 and rehearsal 11 to 13 minutes, each with a run ID of this phase.
- **Files modified:** `.github/workflows/ci.yml`
- **Verification:** `cmake --workflow --preset hygiene` exit 0; the final head's CI run is green
- **Committed in:** a89637d

---

**Total deviations:** 1 auto-fixed (1 bug)
**Impact on plan:** two extra timeout values in a file the plan already owns; no gate was loosened and nothing moved out of `CI required`.

## Issues Encountered

- **API rate limit.** Three overlapping poll loops of mine, with other projects' `gh` calls on the same login, exhausted the hourly limit (HTTP 403) while the first run was in flight. I stopped my loops (the other projects' were left alone), waited for the window to reset using `gh api rate_limit`, which does not count, and read the finished run once. No result was lost. Later polls ran one at a time, 20 to 30 s apart.
- **Hosted variance.** windows-x64 took 311 s on one run and 107 s on the next, which is why the build timeout is 11 minutes and not 7.

## Known Stubs

None. The plan changed workflow comments, timeouts and todo files.

## Threat Flags

None. No network endpoint, auth path, file access pattern or schema changed; the only outward actions were a push to the phase branch, a workflow dispatch on it and closing the probe pull request, all named by the plan.

## User Setup Required

None - no external service configuration required.

## Notes for ship

1. Probe pull request #7 is already closed; ship opens the real pull request from `phase/02-the-cpu-matches-the-public-vectors`.
2. Re-derive `workflow.test_gate_timeout` from the 8 s local time (twice that, with GSD's config writer under an empty `GSD_HOME`), and ratchet `policy.gate_budget` to 1495 as plan 02-26 recorded.
3. The `vectors-full` and `fuzz` jobs ran green once more on the branch and are outside `CI required`.

## Next Phase Readiness

- Phase 2 has all 27 plans executed; the next step is `/gsd-verify-work` after code review, then `/gsd-ship`.
- `.planning/config.json` still carries the owner's uncommitted edit; it was not touched.

## Self-Check: PASSED

- Files: `.github/workflows/ci.yml`, `.github/workflows/nightly.yml` and the five `.planning/todos/completed/` files present; the five `pending` paths absent; 022 present in `pending`.
- Commits `c2d9a7b` and `a89637d` are ancestors of HEAD; `git rev-list --count a91d542dc6166f8572dcbaa89badb1912a62ec27..HEAD` was 2 before this file.
- Acceptance commands: Task 1 CI required and hash-equality count 2 (exit 0); Task 1 and Task 3 head-commit CI conclusion success (exit 0); todo, 022 and probe-state checks (exit 0); the Task 3 verify command (exit 0).

---
*Phase: 02-the-cpu-matches-the-public-vectors*
*Completed: 2026-11-10*
