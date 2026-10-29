---
quick_id: 261029-ldd
description: Complete the phase-1 CI tail on phase/01-ci-tail
mode: quick
requirements:
  - FRAME-02
  - FRAME-09
---

# Phase-1 CI tail

Complete the separate numbered tail from GSD-HANDOFF section 3, step 9, on
`phase/01-ci-tail`, branched from the released `origin/main`. Work runs inline
under the quick skill's fallback; no discussion or research step is needed.
The released v0.1.0 and green release PR #5 are already observed live.

## Task 1: Arm performance and require all eight lanes

Files: `.github/workflows/ci.yml`, `tests/runner/tests.cmake`,
`tests/runner/inventory.txt`, `docs/testing.md`, `docs/maintaining.md`.

Action: Obtain two manual controls on the same released commit; each must
measure it three times with zero spread before changing performance mode to
`gate`. Promote wasm32 into both required dependencies and the eight-file
comparison, removing its duplicate report-only comparison. Include the observed
Windows ARM64 overrun and successful timing samples in the shared build timeout.
Extend existing hash comparison tests to cover eight equal files, a missing
eighth file, and a mismatching eighth file. Correct the documentation to describe
the resulting behavior and the single tail PR already settled in DEC.99.

Review before verification. Verify with `actionlint` (existing runner-label and
concurrency-key exclusions), `cmake --workflow --preset ci`,
`cmake --workflow --preset hygiene`, and both inventory checks. Exercise the
live roll-up expression against synthetic wasm32 failure and skip outcomes as
a one-off check; leave the deferred phase-2 policy gate deferred. Commit the
reviewed changes atomically.

Done: Controls have zero spread; perf enforces +1.00%; wasm32 is required;
hash comparison requires eight files and catches a missing or different eighth;
the measured timeout and all fresh local checks pass.

## Task 2: Record and merge the budgeted tail PR

Files: `.planning/STATE.md`, this task's SUMMARY, and the bound timeout todo.

Action: Record runtime/model, command-generated evidence, task coverage and the
completed timeout seed. Append the quick task using the schema-backed helper.
Open one Conventional Commit `ci:` PR, enable squash auto-merge in a shell,
confirm CI exists on its head, and follow it through green checks and merge.
Confirm main is green and no release PR is left pending. The tail creates no
release because `ci:` changes do not bump the version. Do not change ROADMAP.md
or start another GSD step.

Done: One tail PR is merged and green; state and summary identify the next
command as `$gsd-discuss-phase 2`.
