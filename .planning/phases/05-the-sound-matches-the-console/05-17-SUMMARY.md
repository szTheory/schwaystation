---
phase: 05-the-sound-matches-the-console
plan: "17"
subsystem: audio
tags: [runner, pcm, sha-256, ctest]
requires:
  - phase: 05-11
    provides: SPU audio drain and shared SHA-256 implementation
provides:
  - Accumulated native PCM SHA-256 output from the headless runner
  - Tests and documentation for stream boundaries, options, and drain failures
affects: [phase-05-verification]
actuals:
  tokens: 3917
  tasks: 2
  commits: 4
plan_head_before: eec24623ac2e18e80315c49c837b98d6e4891533
plan_head_after: 57df2459addd3063388c223896d072b277b7414c
tech-stack:
  added: []
  patterns:
    - Hash explicitly serialized little-endian signed stereo PCM
    - Drain once per emulated frame into a bounded pair buffer
key-files:
  created: []
  modified:
    - runner/main.c
    - tests/runner/test_runner.c
    - tests/runner/tests.cmake
    - tests/runner/inventory.txt
    - docs/runner.md
key-decisions:
  - "Keep the hash at the native 44.1 kHz stereo PCM boundary before host processing."
  - "Preserve SOUND-01 as pending until final Phase 5 verification."
requirements-completed: []
coverage:
  - id: D1
    description: Runner emits a deterministic SHA-256 over every native PCM sample actually drained.
    requirement: SOUND-01
    verification:
      - kind: integration
        ref: ctest runner.audio.hash in build/ci
        status: pass
    human_judgment: false
  - id: D2
    description: Audio drain errors and duplicate or conflicting CLI options preserve their exit behavior.
    requirement: SOUND-01
    verification:
      - kind: integration
        ref: ctest runner.audio.failures in build/ci
        status: pass
    human_judgment: false
duration: 32min
completed: 2026-11-14
status: complete
---

# Phase 5 Plan 17: The Sound Matches the Console Summary

**The headless runner now hashes its complete native stereo PCM stream and reports drain failures with stable CLI statuses.**

## Performance

- **Duration:** 32 min
- **Started:** 2026-11-14T07:09:13Z
- **Completed:** 2026-11-14T07:41:31Z
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments

- Added `--hash-audio`, hashing each frame's actual left/right signed 16-bit samples after explicit little-endian serialization.
- Added golden tone and silence checks, partition invariance, partial-run stream checks, and failure/option interaction coverage.
- Documented the native 44.1 kHz stereo PCM boundary, output ordering, and failure behavior.

## Task Commits

Task 1, Trace real tone PCM into the accumulated runner SHA-256:

- `90e1ee5` test(05-17): add audio hash stream checks
- `404f1b3` feat(05-17): hash the runner PCM stream

Task 2, Expand CLI failure directions and document stream semantics:

- `2350833` test(05-17): cover audio drain failure directions
- `57df245` test(05-17): cover audio stream failure directions

## Files Created/Modified

- `runner/main.c` — accumulates and prints the native PCM digest, with bounded drain error handling.
- `tests/runner/test_runner.c` — validates SHA byte order, empty input, and chunk equivalence.
- `tests/runner/tests.cmake` — exercises CLI hashes, partitioning, partial runs, failure behavior, and combined output.
- `tests/runner/inventory.txt` — registers both audio runner cases in sorted order.
- `docs/runner.md` — describes the audio hash option and stream semantics.

## Decisions Made

- The digest covers native PCM after main volume and final saturation, before host processing. No host audio device or resampling was added.
- SOUND-01 remains pending for final Phase 5 verification.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking issue] Used the existing runner test target**
- **Found during:** Task 1
- **Issue:** The planned build target `schwaystation-test-runner` does not exist in this checkout; the runner tests are built through the existing `schwaystation-test-bench` target, with `schwaystation-test-run-stop` required by its integration fixture.
- **Fix:** Built those existing targets alongside `schwaystation-run`.
- **Files modified:** None beyond the planned files.
- **Verification:** The focused audio runner CTest cases passed.
- **Committed in:** `404f1b3` and `57df245` (implementation/test commits)

**2. [Rule 1 - Bug] Accounted for host-frame boundary in partial-run test**
- **Found during:** Task 1
- **Issue:** The mailbox fixture's recorded guest frame 600 corresponds to host frame 601 in the runner output.
- **Fix:** The test obtains the host frame count from the runner and compares the partial stream to a separate run of exactly that many host frames.
- **Files modified:** `tests/runner/tests.cmake`
- **Verification:** The `runner.audio.hash` integration case passed.
- **Committed in:** `404f1b3`

**3. [Rule 1 - Bug] Corrected test assertions for stderr and output ordering**
- **Found during:** Task 2
- **Issue:** The initial drain-error assertion inspected stdout, and a multiline CMake regex did not reliably check digest order.
- **Fix:** Check the failure message on stderr and compare the locations of each digest in output.
- **Files modified:** `tests/runner/tests.cmake`
- **Verification:** The `runner.audio.failures` integration case passed.
- **Committed in:** `57df245`

**Total deviations:** 3 auto-fixed (1 Rule 3, 2 Rule 1). All were necessary to use the repository's existing test seam and verify the specified behavior.

## Issues Encountered

The focused test command used the existing target names in place of the missing planned target. No test or build failures remain.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

This plan's runner audio hash slice is implemented and covered by focused tests. SOUND-01 and SOUND-02 remain pending until final Phase 5 verification. No hosted run was needed for this local deterministic test slice.

## Commands and Evidence

Focused configure, build, and CTest command (exit status 0; two audio runner tests passed):

```text
cmake --preset ci && cmake --build --preset ci --target schwaystation-run schwaystation-test-bench schwaystation-test-run-stop && ctest --test-dir build/ci -R '^runner\.audio\.' --no-tests=error --output-on-failure
```

Additional checks `git diff --check` and `LC_ALL=C sort -c tests/runner/inventory.txt` passed. No hosted run applies to this plan.

---
*Phase: 05-the-sound-matches-the-console*
*Completed: 2026-11-14*

## Self-Check: PASSED

Summary and all five planned source files exist; all four task commits are ancestors of HEAD.
