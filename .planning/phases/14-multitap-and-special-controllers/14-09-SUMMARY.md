---
phase: 14-multitap-and-special-controllers
plan: "09"
subsystem: runner
tags: [runner, multitap, memory-card, persistence]
requires:
  - phase: 14-08
    provides: Stable per-socket card path helpers and atomic save behavior
provides:
  - Eight-socket runner card import, generation tracking, and dirty-only atomic flush
  - Independent frame-latched multitap options for physical ports 1 and 2
affects: [runner, memory-card persistence, controller input movies]
actuals:
  tokens: 9120
  tasks: 2
  commits: 3
  plan_head_before: dee0173152395192239be7ea2f24be2015e452a8
  plan_head_after: cc3d2d808d22b85fdccbcfd073c55b8df40a117a
tech-stack:
  added: []
  patterns:
    - Coordinate-based host persistence across fixed physical port/socket identities
    - Reapply runner-owned tap flags after each input movie snapshot and before frame latching
key-files:
  created: []
  modified:
    - runner/main.c
    - tests/runner/tests.cmake
    - docs/runner.md
    - .planning/phases/14-multitap-and-special-controllers/14-09-PLAN.md
key-decisions:
  - Preserve port 1/socket A and port 2/socket A filenames while adding stable B–D names.
  - Keep both multitaps disabled by default and let each CLI option control one physical port.
requirements-completed:
  - SIO-01
coverage:
  - id: D1
    description: Runner persists and reloads eight independent card sockets without changing unselected files.
    requirement: SIO-01
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^runner[.]cards[.](multitap|persistence|controls)$' --output-on-failure (3/3)"
        status: pass
    human_judgment: false
  - id: D2
    description: Runner tap options independently enable either port, reject invalid syntax, and survive movie updates.
    requirement: SIO-01
    verification:
      - kind: integration
        ref: "runner.cards.multitap: default/disabled/port1/port2/both state hashes, input movie, malformed options, and --help assertions"
        status: pass
    human_judgment: false
duration: 61m
completed: 2026-11-24
status: complete
---

# Phase 14 Plan 14-09: Runner multitap persistence Summary

**Runner card saves now cover all eight stable port/socket coordinates, and two independent options keep tap selection active across input movie updates.**

## Performance

- **Duration:** 60m42s wall time (the plan's 30-minute split bound was exceeded)
- **Started:** 2026-11-24T07:13:54Z
- **Completed:** 2026-11-24T08:14:36Z
- **Runtime/model:** Codex GSD executor, configured `gpt-6-luna` xhigh; primary runtime model ID unavailable
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments

- Runner derives each of eight filenames from the resolved identity and stable socket coordinates, imports each image, and flushes only sockets whose write generation changed.
- The test-only SIO writer exercises all eight sockets in fresh processes; the matrix checks exact file sets and sizes, distinct sectors, reload isolation, abort/checksum behavior, clean exits, and atomic-save failure preservation.
- Added `--multitap-port1` and `--multitap-port2`, both defaulting to disabled. The runner reapplies these flags after each movie snapshot and immediately before the frame latch. Runner docs now describe the options and eight-socket lifecycle.

## Task Commits

1. **Task 1 RED:** `ae0311f` — assert the runner persists all eight card sockets.
2. **Task 1 GREEN:** `1f14811` — persist runner cards across eight sockets.
3. **Task 2:** `cc3d2d8` — add independent runner multitap controls, docs, and corrected focused build targets.

**Plan metadata:** `9c16aae` — summary, State, and Roadmap closeout.

The plan's focused verify command builds `schwaystation-test-run-stop`, the hook binary selected by the card tests.

## Files Created/Modified

- `runner/main.c` — socket-coordinate card loading/flushing, per-port options, and bounded test-only SIO selectors.
- `tests/runner/tests.cmake` — eight-socket fresh-process matrix, option/state assertions, and help coverage.
- `docs/runner.md` — user-facing tap and persistence behavior.
- `.planning/phases/14-multitap-and-special-controllers/14-09-PLAN.md` — focused verification now builds the actual test-hook target.

## TDD Gate Compliance

- **RED:** `runner.cards.multitap` exited 8 on the planned missing `schwaystation.p1b.mcr` persistence assertion. The unchanged JUnit report was classified `RED_EVIDENCE_OK`; semantic review confirmed the selected test ran and failed on the intended behavior. Evidence is ignored under `.gsd/tdd-evidence/14-09/`.
- **GREEN:** The eight-socket lifecycle and reload matrix passed, then the complete focused trio passed 3/3. The final help assertion change was checked by a focused multitap rerun, 1/1.
- **REFACTOR:** No separate refactor was needed; path generation and flush logic use the existing host helpers and APIs.

## Verification and Iteration Time

- `LC_ALL=C sort -cu tests/runner/inventory.txt` — exit 0; no inventory changed.
- `git diff --check` — exit 0.
- CTest JSON preflight for `runner.cards.multitap`, `runner.cards.persistence`, and `runner.cards.controls` — all present.
- `cmake --preset ci` — exit 0.
- `cmake --build --preset ci --target schwaystation-run schwaystation-run-test schwaystation-test-run-stop` — exit 0.
- `ctest --preset ci --no-tests=error -R '^runner[.]cards[.](multitap|persistence|controls)$' --output-on-failure` — exit 0, 3/3; multitap 1.37s, controls 0.42s, persistence 14.33s, 14.39s total wall time.
- After the final help-text edit, the exact hook target rebuilt successfully and `ctest --preset ci --no-tests=error -R '^runner[.]cards[.]multitap$' --output-on-failure` passed 1/1 in 0.55s. The same lifecycle-only selector passed 1/1 in 0.61s while validating the task-scoped commit boundary.
- No workflow changed, so no hosted CI or Act run was needed. No Docker resources were created. The existing `runner.cards.persistence` selector was the longest part of the focused trio; it remained unchanged because its multi-disc identity coverage is unrelated to this runner card slice.

## Decisions Made

- A remains socket A on each physical port, preserving `.srm` and `.1.mcr`; B–D use the existing Plan 08 suffix helpers.
- Runner tap flags override tap bits in movie snapshots by being applied after each snapshot update; movie button/device fields retain their existing behavior.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Build wiring] Added the runner card hook binary to the focused build command.**
- **Found during:** Task 1
- **Issue:** The plan built `schwaystation-run-test`, but the selected card tests execute `schwaystation-test-run-stop`; the first implementation run therefore used an old hook binary and rejected the new test selector.
- **Fix:** Updated both focused verification commands to build `schwaystation-test-run-stop`, then rebuilt the exact target and reran the affected selector and trio.
- **Files modified:** `.planning/phases/14-multitap-and-special-controllers/14-09-PLAN.md`
- **Verification:** Focused trio passed 3/3; final help-inclusive multitap selector passed 1/1.
- **Committed in:** `cc3d2d8`

**2. [Rule 3 - Compile blocker] Read the card span through a typed byte pointer in the test hook.**
- **Found during:** Task 1
- **Issue:** Strict C warnings rejected indexing `schway_span.data` while it was still `const void *`.
- **Fix:** Added a local `const uint8_t *` view before checking the expected sector bytes.
- **Files modified:** `runner/main.c`
- **Verification:** The strict focused build passed and all card selectors passed.
- **Committed in:** `1f14811`

**3. Execution exceeded the 30-minute split bound.**
- The measured wall interval was 60m42s. The plan was not split before the bound; execution remained limited to Plan 14-09 and closed out here. Future execution should split before reaching that limit.

**Total deviations:** 3 (two blocking fixes and one execution-bound overrun)
**Impact on plan:** Required runner behavior and coverage completed; no broader suite, CI routing, dependency, or unrelated source changes were added.

## Issues Encountered

The first failure-injection vector reused the reload check after that socket had already been updated, so it failed before reaching the injected save failure. The vector was corrected to issue a normal selected-socket write under the failure injector; the subsequent test confirmed the old file was preserved.

## Deferred Issues

None.

## Self-Check: PASSED

- Summary file exists at the phase path.
- Task commits `ae0311f`, `1f14811`, and `cc3d2d8` are ancestors of HEAD.
- No new stubs or placeholders were found in this plan's committed diff.
