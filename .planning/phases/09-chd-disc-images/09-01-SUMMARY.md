---
phase: 09-chd-disc-images
plan: "01"
subsystem: testing
tags: [disc, C, CMake, CTest, preflight]
requires: []
provides:
  - Production callback preflight for the owned two-track CUE fixture
  - Documented cheap-first validation order for future media providers
affects: [Phase 09 media providers, Phase 10 PBP images, Phase 11 ECM and CCD images]
actuals:
  tokens: 1721
  tasks: 2
  commits: 2
tech-stack:
  added: []
  patterns:
    - CTest setup fixture gates guest media tests on a host callback preflight
    - Guard bytes detect writes beyond fixed sector and subcode callback buffers
key-files:
  created:
    - tests/disc/media.c
  modified:
    - tests/disc/tests.cmake
    - tests/disc/inventory.txt
    - docs/testing.md
key-decisions:
  - Follow the existing CUE provider contract and keep this plan free of guest execution.
patterns-established:
  - Validate media geometry and boundary reads before running expensive guest checks.
requirements-completed: []
coverage:
  - id: D1
    description: The owned CUE is checked through the production disc callbacks for track geometry, touching boundaries, pregap, exclusive leadout, empty inputs, sector bounds, and zero subcode before guest tests.
    requirement: MEDIA-02
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^disc[.]media[.]preflight$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: The shared media preflight order and test-budget constraints are documented, with directory inventories kept bytewise sorted.
    requirement: MEDIA-02
    verification:
      - kind: other
        ref: "LC_ALL=C sort -cu on disc, fuzz, and compatibility inventories; git diff --check"
        status: pass
    human_judgment: false
duration: 9min
completed: 2026-11-19
status: complete
---

# Phase 9 Plan 09-01 Summary

The owned CUE now passes a fast host-provider geometry and callback-boundary check before any disc guest runs.

## Performance

- **Duration:** about 9 minutes from resumed execution; exact start was not captured before the first status reconciliation.
- **Started:** 2026-11-20T03:46:17Z (first observed execution setup)
- **Completed:** 2026-11-20T03:54:52Z
- **Tasks:** 2
- **Files modified:** 4
- **Runtime/model:** Codex with OpenGSD 1.20.0; primary GPT-6 model ID is not exposed. Sequential inline execution was selected because `workflow.use_worktrees=false`; no executor agent ran.

## Accomplishments

- Added `disc.media.preflight`, which opens the generated CUE through `schwaystation_disc_image`, checks the independently authored two-track geometry, reads each track edge and index boundary, checks guard bytes around the fixed-size sector and subcode buffers, and rejects the leadout and empty inputs.
- Made all four `disc.fixture.*` guest tests require that setup fixture.
- Documented the shared preflight contract and cheap-first command order. The disc, fuzz and compatibility inventories remain sorted and tab-separated.

## Task Commits

1. **Task 1: Check the owned CUE through the shipping callback before boot** - `b6ae475` (`test(09-01): add gated disc media preflight`)
2. **Task 2: Document the shared test contract and confirm inventories** - `80be717` (`docs(09-01): document media preflight order`)

The summary and state/roadmap closeout commits follow the task commits.

## Files Created/Modified

- `tests/disc/media.c` - bounded callback-level geometry, boundary and buffer checks.
- `tests/disc/tests.cmake` - preflight target, test registration and guest-test fixture dependency.
- `tests/disc/inventory.txt` - exact inventory row for the preflight test.
- `docs/testing.md` - media check order and retained quality bounds.

## Verification Evidence

| Command | Exit | Result |
|---|---:|---|
| `LC_ALL=C sort -cu` for `tests/disc/inventory.txt`, `tests/fuzz/inventory.txt`, and `tests/compat/inventory.txt` | 0 | All inventories remained bytewise sorted. |
| `git diff --check origin/main...HEAD && git diff --check HEAD` | 0 | No whitespace errors. |
| `cmake --preset ci -DSCHWAYSTATION_CHD=OFF` | 0 | Configure passed; the first configure reported the not-yet-introduced CHD option as unused. |
| `cmake --build --preset ci --target schwaystation-test-disc-media schwaystation-disc-fixtures` | 0 | The test and provider built; fixture artifacts were already current. |
| `ctest --preset ci --no-tests=error -R '^disc[.]media[.]preflight$' --output-on-failure` | 0 | Preflight passed 1/1. It was rerun after the task commits; the repeated build had no work. |

The first configure/build/test pass took under one second in total; the post-commit bounded verification also took under one second. No full suite, guest boot, hosted CI, or `act` run was needed for this skeleton, and no hosted URL applies.

## Decisions Made

No new product decisions; the test follows the existing host callback and the phase context's locked behavior.

## Deviations from Plan

None. The fuzz and compatibility inventories were inspected and validated but did not change because this plan adds no tests to those directories.

## Issues Encountered

The first CMake configure emitted an unused-command-line-variable warning for `SCHWAYSTATION_CHD=OFF`, because that option is introduced by a later Phase 9 plan. Configuration and all requested checks passed; no option or dependency was added early.

## User Setup Required

None.

## Next Phase Readiness

Plan 09-01 is complete. Continue with the ready Phase 9 plans; the license-closure prerequisite remains in the later generator plan and must be cleared before any CHD fixture generator is built or used.

---
*Phase: 09-chd-disc-images*
*Completed: 2026-11-19*
