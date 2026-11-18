---
phase: 01-a-test-frame-in-retroarch
plan: 15
subsystem: ci, planning
tags: [github-actions, timeouts, rulesets, todos, gate-budget]
status: complete

requires:
  - phase: 01-14
    provides: "the measured duration of every CI and nightly job, run 39455724329, and the open probe pull request #2"
provides:
  - "Every ci.yml and nightly.yml timeout set to twice the slowest successful measured job, naming the run, the sample count and provisional"
  - "workflow.test_gate_timeout 4 (twice the 1.62 s fresh local ci workflow)"
  - "The be32 and wasm32 per-PR cost question answered in DECISIONS (36 s and 22 s, run 39455724329)"
  - "Todos 001-018: one per deferred gate bound to the phase that adds its first caller, and the gate-budget re-pin (1392 lines) for ship"
  - "Todos 019-022: carry-overs and the one-off windows DLL consumer failure"
  - "Probe pull request #2 closed unmerged"
  - "The owner-applied ruleset main (id 25491748), active, requiring CI required, confirmed read-only"
affects: [01-ship, 02]

actuals:
  tokens: 11500
  tasks: 3
  commits: 3
plan_head_before: 856831b39c8c4a41c4cc398876f2b3445548bffd
plan_head_after: c078b3127f0128460b56070ce1f5f24559d18f85

tech-stack:
  added: []
  patterns:
    - "Timeout comments name the run ID, the sample count and provisional until 10 samples exist"
    - "A deferred gate is a pending todo bound to the phase that adds its first caller"

key-files:
  created:
    - .planning/todos/pending/001-ci-required-gate.md
    - .planning/todos/pending/018-repin-gate-budget-at-ship.md
    - .planning/todos/pending/022-windows-dll-crt-consumer-flake.md
  modified:
    - .github/workflows/ci.yml
    - .github/workflows/nightly.yml
    - .planning/config.json
    - .planning/preparation/DECISIONS.md
    - docs/testing.md

key-decisions:
  - "01-15: be32 (36 s) and wasm32 (22 s) stay per-PR lanes; the libgcc and wasmtime-count questions wait for the GTE in phase 3"
  - "01-15: workflow.test_gate_timeout is 4 s, twice the measured 1.62 s fresh local ci workflow"
  - "01-15: policy.gate_budget is re-pinned from 1500 to the measured gate-line total (1392 at d8a0ab4) at phase 1 ship, after code review"
  - "01-15: the single windows-x64 install.consumer.dll_crt failure in run 39458992541 is a test bug to triage (todo 022), not a gate"

requirements-completed: [FRAME-01, FRAME-05]

coverage:
  - id: D1
    description: "Every measured timeout in ci.yml and nightly.yml is twice the slowest successful job with a comment naming the run ID, the sample count and provisional; hygiene.timeout_comments passes and CI is green on the head commit"
    requirement: FRAME-01
    verification:
      - kind: e2e
        ref: "https://github.com/szTheory/schwaystation/actions/runs/39460774025 (head c078b31, conclusion success); cmake --workflow --preset hygiene exit 0"
        status: pass
    human_judgment: false
  - id: D2
    description: "workflow.test_gate_timeout is twice the measured local ci workflow time, written with GSD's config writer and read back"
    requirement: FRAME-01
    verification:
      - kind: other
        ref: "gsd-tools query config-get workflow.test_gate_timeout printed 4, not 900 (exit 0)"
        status: pass
    human_judgment: false
  - id: D3
    description: "The DECISIONS open-question row on be32 and wasm32 per-PR cost records 36 s and 22 s from run 39455724329"
    requirement: FRAME-01
    verification:
      - kind: other
        ref: "grep -n 'run 39455724329' .planning/preparation/DECISIONS.md exit 0"
        status: pass
    human_judgment: false
  - id: D4
    description: "One pending todo per deferred gate, each bound to the phase that adds its first caller, plus the ship-time gate-budget re-pin from the measured 1392-line total"
    requirement: FRAME-05
    verification:
      - kind: other
        ref: "the plan's Task 2 todo loop (22 files with status pending, at least 18) exit 0; grep -c LIMIT .planning/todos/pending/018-repin-gate-budget-at-ship.md at least 1"
        status: pass
    human_judgment: false
  - id: D5
    description: "Probe pull request #2 is closed unmerged before ship and origin/main is unchanged"
    requirement: FRAME-01
    verification:
      - kind: other
        ref: "gh pr view phase/01-a-test-frame-in-retroarch --json state -q .state printed CLOSED (exit 0); git log origin/main --oneline -1 printed 82d61d2"
        status: pass
    human_judgment: false
  - id: D6
    description: "The tree holds no cruft and no planning ID in code, tests, workflows or config"
    requirement: FRAME-05
    verification:
      - kind: other
        ref: "the plan's cruft git grep pair a=1 b=1 exit 0; the planning-ID git grep then test $? -eq 1 exit 0"
        status: pass
    human_judgment: false
  - id: D7
    description: "The owner applied the ruleset main requiring CI required; the agent confirmed it read-only"
    requirement: FRAME-01
    verification:
      - kind: other
        ref: "gh api repos/szTheory/schwaystation/rulesets --jq '[.[] | select(.name == \"main\")] | length' printed 1, test -ge 1 exit 0; scripts/apply-repo-settings.sh --check printed no drift, exit 0; ruleset 25491748 enforcement active with required context CI required"
        status: pass
    human_judgment: true
    rationale: "A branch ruleset is an admin change on the owner's repository; AGENTS rule 6 leaves that identity-bound consent to the owner. The owner ran scripts/apply-repo-settings.sh; the agent only confirmed it."

duration: 50min
completed: 2026-10-20
---

# Phase 01 Plan 15: Measured timeouts, bound todos and the required-check ruleset Summary

**Timeouts and the GSD test gate set from measured runs, every deferred gate recorded as a bound todo with the 1392-line gate-budget re-pin handed to ship, the probe PR closed unmerged, and the owner's `main` ruleset requiring `CI required` confirmed live.**

## Performance

- **Duration:** about 50 min wall time, including the wait for the owner's ruleset step
- **Started:** 2026-10-20T16:32:32Z
- **Completed:** 2026-10-20T17:22:00Z
- **Tasks:** 3
- **Files modified:** 27

## Accomplishments

- Every measured job in `ci.yml` and `nightly.yml` has a timeout of twice its slowest successful run, naming the run ID, the sample count and `provisional`. `release.yml` is unchanged: its jobs have no sample yet and already carry provisional bounds.
- `workflow.test_gate_timeout` went from 900 to 4: the fresh `cmake --workflow --preset ci` took 1.62 s, and an incremental re-run took 1.14 s (exit 0).
- The DECISIONS open-question row now has the measured per-PR cost: `be32` takes 36 s and `wasm32` 22 s (run 39455724329). Both stay per-PR lanes.
- Todos 001 to 018 are the seventeen deferred seed gates, each bound to the phase that adds its first caller, plus the gate-budget re-pin. The re-pin lowers `LIMIT=1500` to the measured 1392 at phase 1 ship, re-measured after code review.
- Probe pull request #2 is closed unmerged, and `origin/main` is still 82d61d2.
- The owner applied ruleset `main` (id 25491748). It is active and requires `CI required`, and the agent confirmed it read-only.

## Task Commits

1. **Task 1: measured timeouts, GSD test-gate timeout, per-PR cost answer** - `d8a0ab4` (ci)
2. **Task 2: one bound todo per deferred gate, ship re-pin, probe PR closed** - `5b0761b` (docs), `c078b31` (docs, todo 022)
3. **Task 3: owner applies the ruleset; agent confirms read-only** - evidence only, no code commit

**Plan metadata:** recorded in the docs(01-15) completion commit

## Task 3 evidence (from the repository root)

- `n=$(gh api repos/szTheory/schwaystation/rulesets --jq '[.[] | select(.name == "main")] | length') && test "$n" -ge 1` printed n=1, exit 0
- `scripts/apply-repo-settings.sh --check` printed "no drift on szTheory/schwaystation", exit 0
- `gh api repos/szTheory/schwaystation/rulesets/25491748 --jq '[.enforcement, (.rules[] | select(.type=="required_status_checks") | .parameters.required_status_checks[].context)]'` printed `["active","CI required"]`, exit 0
- `git log origin/main --oneline -1` printed 82d61d2, so nothing was merged
- `gh pr view phase/01-a-test-frame-in-retroarch --json state -q .state` printed CLOSED, exit 0
- The planning-ID `git grep` from Task 2 exited 1 (clean)

## Files Created/Modified

- `.github/workflows/ci.yml`, `.github/workflows/nightly.yml`: timeouts from measured runs
- `.planning/config.json`: `workflow.test_gate_timeout` 4
- `.planning/preparation/DECISIONS.md`: the per-PR cost row answered
- `.planning/todos/pending/001`..`022`: deferred gates, the re-pin and the carry-overs
- `docs/testing.md`: the timeout sentence reworded to match the measured provisional timeouts

## Decisions Made

See key-decisions in the frontmatter.

## Deviations from Plan

**1. [Rule 1 - Doc accuracy] Reworded the timeout sentence in docs/testing.md**
- **Found during:** Task 1
- **Issue:** The sentence described the old guessed bounds, not the measured provisional timeouts.
- **Fix:** Reworded it to match the measured provisional timeouts.
- **Committed in:** d8a0ab4

**2. [Scope] Four todos beyond the planned eighteen (019 to 022)**
- 019: the macOS Debug `dev` preset `abi.undefined` sees `__memcpy_chk` and `__memset_chk`.
- 020: the CI table in `docs/testing.md` lacks the rehearsal and cleanup rows.
- 021: actionlint needs a local config for the `ubuntu-26.04` labels and `concurrency.queue`.
- 022: a one-off `install.consumer.dll_crt` failure on windows-x64 in run 39458992541, on d8a0ab4. The test passed in 39455724329, 39459524424 and 39460774025, so it was triaged as a test bug under AGENTS rule 8.
- **Committed in:** 5b0761b, c078b31

**Total deviations:** 2 (one doc fix, one set of recorded carry-overs). **Impact:** no scope creep in code.

## Issues Encountered

The windows-x64 failure in run 39458992541 (deviation 2, todo 022) happened once. CI is green on head c078b31: https://github.com/szTheory/schwaystation/actions/runs/39460774025

## User Setup Required

None. The owner's one action, applying the ruleset, is done.

## Next Phase Readiness

This was the last plan of phase 1. The phase is ready for code review, then verification, then `/gsd-ship`. Ship re-pins `policy.gate_budget` (todo 018) and arms auto-merge, which the live ruleset allows.

## Self-Check: PASSED

- FOUND: .planning/todos/pending/001-ci-required-gate.md, 018-repin-gate-budget-at-ship.md, 022-windows-dll-crt-consumer-flake.md
- FOUND: d8a0ab4, 5b0761b, c078b31 (ancestors of HEAD)
