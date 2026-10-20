---
phase: 01-a-test-frame-in-retroarch
fixed_at: 2026-10-20T17:51:48Z
review_path: .planning/phases/01-a-test-frame-in-retroarch/01-REVIEW.md
iteration: 1
findings_in_scope: 2
fixed: 1
skipped: 1
status: partial
---

# Phase 1: Code Review Fix Report

Applied inline in Codex using the existing review; no new review round.
One core finding is fixed. The tooling warning retains its explicit seed
disposition under AGENTS rule 7. The original review remains historical evidence.

## Fixed Issues

### CR-01: A loaded boundary state advances into an unloadable save

**Files modified:** `src/frame.c`, `src/internal.h`, `src/state.c`,
`tests/core/test_state.c`, `tests/core/tests.cmake`, `tests/core/inventory.txt`,
`docs/embedding.md`.
**Commit:** b7c658a51af8663c5ec4849b6f6b448a8fc13ba0.
**Applied fix:** The loader and frame runner share the same inclusive counter
limits. The runner computes the frame's cycle increment without mutation and
returns `SCHWAY_STATUS_ERR_STATE_FIELD` before touching state or output if either
counter would exceed its limit. Existing accepted states and the state format
are preserved. The embedding documentation explains the refusal and power reset.

The new `core.state.run_bounds` regression loads CRC-valid states at the frame
limit, one frame below it, at and one tick below the tick limit, and at one
frame's cycle increment below that limit and its adjacent ticks. It covers all
three regions and accumulator remainders zero and denominator minus one.
Successful advances save and reload byte for byte; refusals preserve the entire
instance and the result, with both a result pointer and NULL.

The fix diff was reviewed before the full verification workflows. The guard
precedes audio, profile locking, counters, drawing and result writes; subtraction
avoids overflowing the tick counter. No ordinary frame hash changes.

## Skipped Issues

### WR-01: PASS_REGEX can turn a command failure into a passing test

**File:** `cmake/SchwaystationTests.cmake:27`.
**Reason:** Deferred tooling seed under AGENTS rule 7, as the source review
requires. This is not an unresolved core blocker or a new phase requirement.
Bind this one seed to the next plan that changes the test helper or adds its
next caller; that plan should require command exit zero alongside the tolerant
output regex using the existing wrapper. No test-helper code changed here.
**Original issue:** CTest's PASS_REGULAR_EXPRESSION can override an ordinary
nonzero command exit; the JUnit inventory cannot recover the child's exit code.

## Verification

Commands ran from the repository root in the main checkout
(`workflow.use_worktrees=false`), on the existing phase branch. Local commands
have no hosted run URL (not applicable). No logs or state bytes are committed.

- `cmake --preset ci`: exit 0.
- `cmake --build --preset ci --target schwaystation-test-state`: exit 0.
- `ctest --test-dir build/ci -R '^core.state.run_bounds$' --output-on-failure`:
  exit 8 before the source fix, reproducing successful runs beyond the bounds;
  exit 0 after the source fix.
- `cmake --workflow --preset ci`: exit 0; 78 regular tests and two archive
  tests passed, including the new regression; all three packages generated.
- `cmake --workflow --preset hygiene`: exit 0; 21 tests passed.
- `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake`:
  exit 0; 78 selected tests ran once each.
- `cmake -DJUNIT=build/ci/junit-release.xml -DINVENTORY=tests/inventory/ci.txt -DLABELS=release -P tests/cmake/check_junit.cmake`:
  exit 0; both archive tests ran once each.
- `cmake -DJUNIT=build/hygiene/junit.xml -DINVENTORY=tests/inventory/hygiene.txt -P tests/cmake/check_junit.cmake`:
  exit 0; 21 selected tests ran once each.
- `git diff --check`: exit 0.

**Next command:** `$gsd-verify-work 1`.
