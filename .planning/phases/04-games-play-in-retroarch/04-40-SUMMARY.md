---
phase: 04-games-play-in-retroarch
plan: "40"
subsystem: testing
tags: [openbios, runner, cmake, portable-ci, ctest]

requires:
  - phase: "04-23"
    provides: Runner BIOS loading and identity reporting through the real CLI.
  - phase: "04-39"
    provides: Same-run digest-checked OpenBIOS handoff to portable consumers.
  - phase: "04-50 through 04-53"
    provides: Source-owned POST, ATTR, DMA4 and GPUSTAT fixes needed by the inspected shell.
provides:
  - Independently inspected NTSC-U frame 240 shell image and pinned native frame hash.
  - Same-run shell verification and hash tuple consumption across all eight portable lanes.
  - Selector alternatives, missing-lane, missing-tuple and wrong-hash refusal coverage.
affects: [GAME-05, runner, portable-ci, phase-04-verification]

actuals:
  tokens: 48032
  tasks: 2
  commits: 23
commits: 23
plan_head_before: 64d8abcb32694dc146b99224eb561dbe5513cd7f
plan_head_after: b7fa360b3967d9a7d91d136c14a608580a71f1e4

tech-stack:
  added: []
  patterns:
    - Use a CTest fixture setup to bind the hash writer to a fresh same-run OpenBIOS shell tuple.
    - Keep artifact requirements at actual firmware-consumer execution, not build-only configuration.

key-files:
  created: []
  modified:
    - tests/built/tests.cmake
    - tests/built/inventory.txt
    - tests/cmake/hashes.cmake
    - docs/runner.md
    - docs/testing.md
    - docs/embedding.md
    - .planning/phases/04-games-play-in-retroarch/04-40-BOOT-DIAGNOSIS.md

key-decisions:
  - Pin NTSC-U frame 240 because it is the earliest inspected frame with a clearly recognizable cube shell.
  - Require the shell fixture setup before runner.hashes_file so a stale tuple cannot satisfy a later run.
  - Keep Linux-only firmware compilation separate from portable execution of the same run-local artifact.
  - Reuse the existing eight-file hash comparison helper for shell tuple refusal controls.

patterns-established:
  - A checked artifact is validated by the real consumer immediately before guest execution.
  - CTest fixture setup supplies current-run provenance to tests that consume generated result tuples.

requirements-completed:
  - GAME-05

coverage:
  - id: D1
    description: The checked OpenBIOS reaches the inspected cube shell at NTSC-U frame 240 with its pinned native hash.
    requirement: GAME-05
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^built\\.openbios\\.shell$' --no-tests=error --output-on-failure"
        status: pass
      - kind: other
        ref: ".planning/phases/04-games-play-in-retroarch/04-40-BOOT-DIAGNOSIS.md#inspected-shell-frame-after-04-53"
        status: pass
    human_judgment: false
  - id: D2
    description: All applicable portable lanes boot the same run-local image and compare the independently pinned shell tuple.
    requirement: GAME-05
    verification:
      - kind: integration
        ref: "runner.hashes_file fixture setup and hash-equality; run 43133593393"
        status: pass
      - kind: integration
        ref: "https://github.com/szTheory/schwaystation/actions/runs/43133593393"
        status: pass
    human_judgment: false
  - id: D3
    description: NTSC-J and PAL selectors run, and the comparison helper rejects a missing lane, missing tuple, or wrong native hash.
    requirement: GAME-05
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -VV -R '^built\\.openbios\\.shell_controls$' --no-tests=error"
        status: pass
      - kind: integration
        ref: "run 43133593393; WASI, be32, hash-equality and CI required passed"
        status: pass
    human_judgment: false

duration: 4h 28m
completed: 2026-11-12
status: complete
---

# Phase 04 Plan 40: OpenBIOS Shell Summary

**The verified OpenBIOS shell now has an inspected NTSC-U frame pin, current-run portable hash coverage and selector/refusal controls.**

## Performance

- Duration: 4h 28m; started 2026-11-11T20:54:50Z, completed 2026-11-12T01:22:53Z.
- Tasks: 2; files modified: 7.
- Runtime/model: Codex, OpenGSD 1.20.0, sequential shared checkout; gpt-6-luna xhigh.
- The measured plan ledger contains 23 commits since its recorded start, including the source-owner prerequisite fixes completed while this plan was paused. The three plan-specific implementation commits are listed below.
- Final be32 job: 4m01s; `cmake --workflow --preset be32`: 3m41s with the selector controls and existing timeouts unchanged.

## Accomplishments

- Inspected the 640x239 PNG from NTSC-U frame 240 and pinned frame hash `4d1f0ff260e2cf065ae0b7add4ba60353b2b01241dda8325309b5398ba00acec`. This is the earliest inspected frame where the cube shell is clearly recognizable; frame 120 is edge-on and frame 180 only partly shows it.
- Added `built.openbios.shell` and made it the CTest fixture setup for `runner.hashes_file`. The test removes stale output, checks the artifact size and digest, boots the real shell and writes the tuple only after its output matches the native pin.
- Added `built.openbios.shell_controls` for NTSC-J/PAL selection and for rejecting a missing lane, missing tuple and incorrect native hash. Existing missing/corrupt firmware controls remain registered.
- Kept build-only configuration independent of firmware availability while requiring the checked artifact whenever the shell consumer runs. Reused the existing comparison helper and kept the repository gate at 1842/1850 lines.
- Updated runner, testing and embedding documentation; recorded the source diagnosis and final hosted run evidence.

## Task Commits

1. **Task 1: Pin inspected OpenBIOS shell frame** — `92542ed` (test).
2. **Task 1 portability correction** — `852c162` (fix).
3. **Task 2: Cover portable shell controls** — `b7fa360` (test).

## Files Created/Modified

- `tests/built/tests.cmake`, `tests/built/inventory.txt` — shell fixture, portability controls and exact registrations.
- `tests/cmake/hashes.cmake` — consumes the current-run tuple supplied by the shell fixture.
- `docs/runner.md`, `docs/testing.md`, `docs/embedding.md` — documented BIOS selection, packaging boundaries and test behavior.
- `.planning/phases/04-games-play-in-retroarch/04-40-BOOT-DIAGNOSIS.md` — inspected frame, source findings and hosted evidence.

## Decisions Made

- Frame 240 is the acceptance point because it is the first inspected frame with a clearly recognizable shell; no earlier diagnostic frame or hash was substituted.
- The shell test is a CTest fixture setup, so selecting the hash writer alone cannot consume a stale tuple.
- Missing firmware fails at consumer test time. Build-only workflows such as perf can still configure without that consumer artifact.
- The selector controls use one frame each; tuple controls use the existing eight-lane comparison path.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking issue] Corrected portability and gate failures exposed by the first hosted tracer run.**
- **Found during:** Task 1 (hosted tracer verification).
- **Issue:** The first required workflow reported 1874 counted lines against the 1850-line ceiling; the build-only perf lane rejected a missing artifact during configure; WASI could not open the shell image; and raw BIOS runner scripts invoked the WASM executable without its emulator.
- **Fix:** Let the verified shell fixture own tuple validation and current-run provenance, removed the global configure-time artifact fatal, registered the shell command directly to preserve emulator arguments, and explicitly prefixed the emulator for raw BIOS runner script calls.
- **Files modified:** `tests/built/tests.cmake`, `tests/cmake/hashes.cmake`.
- **Verification:** The gate is 1842/1850; run 43131466212 passed the full workflow on the corrected tracer head, and final run 43133593393 passed on the completed head.
- **Committed in:** `852c162`.

**Total deviations:** 1 auto-fixed blocking issue.
**Impact on plan:** These changes preserved every acceptance check and existing timeout while making the harness work on all declared lanes.

## Issues Encountered

The first hosted tracer run was red for the portability and gate issues above. The corrected tracer run and final current-head run both passed; no unresolved issues remain.

## Evidence

- `build/ci/schwaystation-run --region ntsc-u --bios /private/tmp/schwaystation-openbios-43083684528/openbios.bin --frames 240 --tty --hash --png 240:/private/tmp/schway-openbios-actual-240.png` exited 0. The image SHA-256 was `2dd216c41430d7f86b8023c31a0b0ac73daa65fd9bfac37cfe9ad284fbbe405c`; the image and temporary trace stayed outside the repository.
- `ctest --test-dir build/ci -VV -R '^built\.openbios\.shell_controls$' --no-tests=error` exited 0 in 0.14s against the 524288-byte image with SHA-256 `92c2f6d5e6ff056bf2ba177f37246e5468b6164337935297cf7b22266af20cc8`.
- `cmake --workflow --preset ci` exited 0, including the complete test and release-package checks. `cmake --workflow --preset hygiene` exited 0; all 23 hygiene checks passed.
- `CI=true cmake --preset ci -B /private/tmp/schwaystation-04-40-no-bios` exited 0. `CI=true ctest --test-dir /private/tmp/schwaystation-04-40-no-bios -R '^runner\\.hashes_file$' --no-tests=error --output-on-failure` exited 8 because the required shell fixture failed, and the hash writer did not run. Selecting `built.openbios.shell_controls` in the same no-handoff build also exited 8 with the required handoff error.
- `gh run view 43133593393 --json status,conclusion,headSha,event,headBranch,url` exited 0. [Final current-head run 43133593393](https://github.com/szTheory/schwaystation/actions/runs/43133593393) was `workflow_dispatch` on `phase/04-games-play-in-retroarch`, exact head `b7fa360b3967d9a7d91d136c14a608580a71f1e4`, and concluded success. The OpenBIOS producer, eight applicable consumers, WASI, be32, six native builds, ASAN, nofp, perf, RetroArch, hygiene, hash equality and required aggregate passed.

## User Setup Required

None — no external configuration or owner UAT is required.

## Next Phase Readiness

GAME-05's OpenBIOS shell and portable hash acceptance is complete. Plan 04-40 makes no API, hardware-state or behavior revision. The roadmap's next execution wave can proceed without another firmware or shell-readiness gate.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-12*

## Self-Check: PASSED

- Summary exists and `gsd_run verify-summary` found no errors.
- Task commits `92542ed`, `852c162`, and `b7fa360` exist in HEAD history.
