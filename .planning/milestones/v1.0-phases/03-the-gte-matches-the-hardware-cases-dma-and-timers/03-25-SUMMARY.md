---
phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
plan: 25
subsystem: planning
tags: [todos, review-disposition, ledger]
requires:
  - phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
    provides: plan 03-21 fixed review finding WR-06 and wrote completed todo 030; plan 03-06 fixed IN-01 with the interrupt-entry peek
provides:
  - one todo for each of the 13 phase-2 review findings (030 from plan 03-21, 031 completed here, 025-029 and 032-037 pending) and a disposition row for each
  - todos 038 (GTE write-to-use latency) and 039 (CPU-bound Ir fixture)
  - the older todos rebound off phase 2 to their first caller or SEED-007
affects: [phase 4 planning, SEED-007 tune-up]
actuals:
  tokens: 9500
  tasks: 2
  commits: 2
plan_head_before: b1afe29365df13741cd6292ec3509999cc23b2da
plan_head_after: 04572dd279f4785b2aae94c6afb5fc63c351949b
tech-stack:
  added: []
  patterns:
    - "a phase-4 binding names the first plan that edits the file the finding names, while phase 4 is unplanned"
key-files:
  created:
    - .planning/todos/pending/025-user-mode-fetch-address-error.md
    - .planning/todos/pending/026-state-load-cop0-and-r0-validation.md
    - .planning/todos/pending/027-icache-refill-bus-error-word.md
    - .planning/todos/pending/028-exception-loop-progress-heuristic.md
    - .planning/todos/pending/029-runner-tty-byte-filter.md
    - .planning/todos/completed/031-irq-entry-peek-without-side-effects.md
    - .planning/todos/pending/032-print-hash-height-bound.md
    - .planning/todos/pending/033-vector-harness-uninitialised-record.md
    - .planning/todos/pending/034-nofp-job-timeout-floor.md
    - .planning/todos/pending/035-sst-slice-name-field.md
    - .planning/todos/pending/036-load-exe-after-run.md
    - .planning/todos/pending/037-swl-swr-single-bus-write.md
    - .planning/todos/pending/038-gte-write-to-use-latency.md
    - .planning/todos/pending/039-cpu-bound-ir-fixture.md
  modified:
    - .planning/phases/02-the-cpu-matches-the-public-vectors/02-REVIEW-DISPOSITION.md
    - .planning/todos/pending/001-ci-required-gate.md
    - .planning/todos/pending/002-release-floor-gate.md
    - .planning/todos/pending/003-pins-gate.md
    - .planning/todos/pending/004-single-version-source-gate.md
    - .planning/todos/pending/014-no-test-entries-gate.md
    - .planning/todos/pending/016-hygiene-format-gate.md
    - .planning/todos/pending/017-hygiene-analyzer-gate.md
    - .planning/todos/pending/022-windows-dll-crt-consumer-flake.md
key-decisions:
  - "Todos 016 and 017 bind to SEED-007: the CPU core landed without the format and analyzer checks, so no plan is a first caller"
  - "Todo 014 binds to the phase 4 plan that vendors the System.dat digest table, the first bundled table with a test-only key list"
requirements-completed: []
coverage:
  - id: D1
    description: "Each phase-2 review finding has one todo and a disposition row; none is open"
    verification:
      - kind: other
        ref: "test count of 02[5-9], 03[2-7] and completed 031 todo files equals 12 and no 'disposition: open' in 02-REVIEW-DISPOSITION.md"
        status: pass
    human_judgment: false
  - id: D2
    description: "Every pending review-finding todo names its binding (bound to or a SEED); the WR-06 row names todo 030, which exists"
    verification:
      - kind: other
        ref: "grep -qiE 'bound to|SEED-0' over todos 025-029 and 032-037, and grep 030 in the disposition file plus test -f of completed 030"
        status: pass
    human_judgment: false
  - id: D3
    description: "Todos 038 and 039 exist; 038 binds to SEED-003 as the nearest seed and 039 to the gpu_motion plan"
    verification:
      - kind: other
        ref: "grep SEED-003 and 'nearest seed' in todo 038 and gpu_motion in todo 039"
        status: pass
    human_judgment: false
  - id: D4
    description: "No pending todo is still bound to phase 2 or phase 3"
    verification:
      - kind: other
        ref: "grep -l 'Bound to phase 2\\|Bound to phase 3' .planning/todos/pending/*.md prints nothing (exit 1)"
        status: pass
    human_judgment: false
duration: 7 min
completed: 2026-11-10
status: complete
---

# Phase 3 Plan 25: Todo ledger and phase-2 review dispositions Summary

**Every phase-2 review finding now has one todo and a fixed-or-deferred disposition row, the GTE write-to-use latency and a CPU-bound Ir fixture are recorded as deferrals, and the older todos no longer point at finished phases.**

## Performance

- **Duration:** 7 min
- **Started:** 2026-11-10T08:16:00Z
- **Completed:** 2026-11-10T08:23:05Z
- **Tasks:** 2
- **Files modified:** 32 (14 created, 18 modified, counting the disposition file once)

## Accomplishments

- Twelve review-finding todos written (WR-01 to WR-05 as 025-029, IN-01 as completed 031, IN-02 to IN-07 as 032-037); WR-06's todo 030 is plan 03-21's. `02-REVIEW-DISPOSITION.md` now records 2 fixed (WR-06 by plan 03-21, IN-01 by plan 03-06), 11 deferred, 0 open, each row naming its todo.
- Phase-4 bindings name the first plan that edits the file each finding names (`src/cpu_exec.inc`, `src/cpu.c`, `src/cop0.c`, `runner/main.c`, `src/exe.c`, `.github/workflows/ci.yml`, the display-range plan, the first width-sensitive GPU register). IN-03 and IN-05 (tooling) bind to SEED-007.
- Todo 038 (GTE write-to-use latency, bound to SEED-003 as the nearest seed, evidence nugget `tests/gte-write-to-use/RESULTS.md` at 9db6d88d23c4) and 039 (CPU-bound Ir fixture bound to the phase 4 `gpu_motion` plan).
- Todos 001, 002, 003, 004, 014, 016, 017 and 022 rebound off phase 2; 006, 009, 013 and 010, 011, 012, 015, 021 keep their phase 4, phase 6 or SEED-007 bindings, with a "reviewed at phase 3" line.

## Task Commits

1. **Task 1: one todo per review finding and its disposition row** - `b445d60` (docs)
2. **Task 2: the two new deferrals and the rebinding of the existing todos** - `04572dd` (docs)

**Plan metadata:** recorded by the docs commit that carries this SUMMARY.

## Files Created/Modified

- `.planning/phases/02-the-cpu-matches-the-public-vectors/02-REVIEW-DISPOSITION.md` - disposition, todo and binding for all 13 findings; counts open 0, fixed 2, deferred 11
- `.planning/todos/pending/025` to `029`, `032` to `037` - the deferred review-finding todos
- `.planning/todos/completed/031-irq-entry-peek-without-side-effects.md` - IN-01, resolved by plan 03-06's peek
- `.planning/todos/pending/038-gte-write-to-use-latency.md`, `039-cpu-bound-ir-fixture.md` - the two new deferrals
- `.planning/todos/pending/001`-`004`, `014`, `016`, `017`, `022` - binding line rewritten; `006`, `009`, `010`-`013`, `015`, `021` - a "reviewed at phase 3" line

## Decisions Made

- 016 and 017 bind to SEED-007 rather than a phase-4 plan: the CPU core they were bound to landed without them, so no later plan is a first caller.
- 014 binds to the phase 4 plan that vendors `System.dat` (todo 013's plan), the first bundled lookup table that can carry a test-only key list.
- 034 (the `nofp` timeout) binds to the first phase-4 plan that edits `ci.yml`, with SEED-007 as the fallback, since the tune-up re-derives every timeout anyway.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Todo 038 first claimed the deviation is documented in `docs/hardware/gte.md`**
- **Found during:** Task 2
- **Issue:** The plan and D-05 call the write-to-use latency a documented deviation, but `docs/hardware/gte.md` says only that a move into the engine does not wait; it never names the two-instruction latency.
- **Fix:** Todo 038 states that the page does not yet name the latency, and its acceptance criterion is a test or a sentence naming the deviation with the evidence. No source or doc change (the plan edits planning files only).
- **Files modified:** `.planning/todos/pending/038-gte-write-to-use-latency.md`
- **Committed in:** `04572dd`

---

**Total deviations:** 1 auto-fixed (1 bug in the todo's own claim)
**Impact on plan:** None on scope. The undocumented deviation is carried by todo 038 for the seed that closes it.

## Triage

| Finding | Disposition | Where it goes |
|---------|-------------|---------------|
| WR-01 | deferred | todo 025, phase 4 first plan editing `src/cpu_exec.inc` |
| WR-02 | deferred | todo 026, phase 4 first plan editing `src/cpu.c` or the CPU state walk |
| WR-03 | deferred | todo 027, phase 4 first plan editing `src/cpu.c` |
| WR-04 | deferred | todo 028, phase 4 first plan editing `src/cop0.c` |
| WR-05 | deferred | todo 029, phase 4 first plan editing `runner/main.c` |
| WR-06 | fixed | plan 03-21, completed todo 030 |
| IN-01 | fixed | plan 03-06, completed todo 031 |
| IN-02 | deferred | todo 032, phase 4 display-range plan |
| IN-03 | deferred | todo 033, SEED-007 (tooling seed) |
| IN-04 | deferred | todo 034, phase 4 first plan editing `ci.yml`, else SEED-007 |
| IN-05 | deferred | todo 035, SEED-007 (tooling seed) |
| IN-06 | deferred | todo 036, phase 4 first plan editing `src/exe.c` |
| IN-07 | deferred | todo 037, phase 4 first width-sensitive GPU register |

## Issues Encountered

None. A first `git add` with a space-joined shell variable failed under zsh before anything was staged; it was re-run with an array.

## Known Stubs

None. The plan edits planning files only; no source or test code changed, so no local test or CI run applies (asan and be32 need clang-21, which this host lacks, and hosted CI runs them).

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Plan 03-26 is the last plan of the phase (already complete per STATE.md ordering). Phase 4 planning should re-bind todos 025-029, 032, 034, 036, 037 and 039 to plan numbers once its plans exist.

## Self-Check: PASSED

- All 14 created todo files and the disposition file exist; commits `b445d60` and `04572dd` are ancestors of HEAD.
- Verify commands: task 1 verify exit 0, task 1 acceptance (binding grep, disposition names 030 with the completed file present) exit 0, task 2 verify exit 0, task 2 acceptance exit 0.

---
*Phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers*
*Completed: 2026-11-10*
