---
phase: 04-games-play-in-retroarch
plan: "39"
subsystem: testing
tags: [github-actions, openbios, cmake, ci, packaging]

requires:
  - phase: 04-22
    provides: Pinned OpenBIOS source/build identity and digest verification.
provides:
  - A short-retention, same-run OpenBIOS handoff to every applicable CI consumer.
  - Gating corruption and missing-transfer refusals with packaging exclusion evidence.
affects: [04-23, 04-40, CI, packaging, GAME-05]

actuals:
  tokens: 4890
  tasks: 2
  commits: 6
commits: 6
plan_head_before: b6b00074733ef4d5435a3948acfa3a9c13609257
plan_head_after: 11a79aeebacc9734c2bc5a5295e1815cfeb15b38

tech-stack:
  added: []
  patterns:
    - Build once per CI run and transfer through a short-retention artifact; verify digest before consumer use.

key-files:
  created: []
  modified:
    - .github/workflows/ci.yml
    - tests/built/tests.cmake
    - tests/built/inventory.txt
    - docs/testing.md
    - cmake/packaging.cmake
    - tests/fixtures/tests.cmake
    - tests/diag/expected.csv

key-decisions:
  - Keep the pinned upstream-supported OPENBIOS_SHELL=shell source/toolchain identity and verify the exact image digest before each consumer uses it.
  - Limit OpenBIOS transfer to a one-day artifact in the current workflow run; keep it outside packages and publication paths.
  - Reuse the existing one-edge frame hash for the held-snapshot guest check while retaining empty-input, repeated-held, and three-edge frame-300 guest runs.

patterns-established:
  - Consumers download a run-local firmware artifact and verify its expected size and SHA-256 before configuration or tests.
  - Corrupt and missing transfer cases use the shared expected-failure harness and stay registered in the directory inventory.

requirements-completed: []
coverage:
  - id: D1
    description: Every applicable CI consumer receives the pinned OpenBIOS image built in that same workflow run and verifies its digest before use.
    requirement: GAME-05
    verification:
      - kind: integration
        ref: OPENBIOS_HANDOFF_RUN_ID=43083684528, workflow_dispatch on head 11a79aeebacc9734c2bc5a5295e1815cfeb15b38; https://github.com/szTheory/schwaystation/actions/runs/43083684528
        status: pass
      - kind: integration
        ref: Producer and all nine consumer jobs logged 524288 bytes with SHA-256 92c2f6d5e6ff056bf2ba177f37246e5468b6164337935297cf7b22266af20cc8 in the same run.
        status: pass
    human_judgment: false
  - id: D2
    description: Handoff checks accept the expected image, refuse corrupted or missing transfers, and keep firmware out of packaged artifacts.
    requirement: GAME-05
    verification:
      - kind: integration
        ref: built.openbios.handoff, built.openbios.handoff_corruption, and built.openbios.handoff_missing passed in the nine CI consumer lanes; https://github.com/szTheory/schwaystation/actions/runs/43083684528
        status: pass
      - kind: integration
        ref: cmake --workflow --preset ci; exit 0, 469 tests and five release checks passed.
        status: pass
      - kind: integration
        ref: cmake --workflow --preset hygiene; exit 0, 23 checks passed, including the 1832-of-1850 line budget and release firmware scan.
        status: pass
    human_judgment: false

duration: 1h 15m
completed: 2026-11-11
status: complete
---

# Phase 04 Plan 39: Run-local OpenBIOS Handoff Summary

**CI now builds the pinned OpenBIOS image once per run, checks it before each consumer, and keeps it out of packages.**

## Performance

- **Duration:** about 1h 15m
- **Started:** 2026-11-11T18:11:17Z (first plan task commit)
- **Completed:** 2026-11-11T19:26:17Z (planned metadata closeout)
- **Tasks:** 2
- **Files modified:** 7
- **Runtime/model:** Codex / OpenGSD 1.20.0, gpt-6-luna xhigh, sequential shared phase checkout.

## Accomplishments

- Added an `openbios` producer that builds the MIT OpenBIOS revision `9db6d88d23c4da7a39b1a4b9729f1e5fff80dec9` with the pinned GCC toolchain archive SHA-256 `228f031a25cf2687d8845fd1421f625bafc211fa27da428e458e80d030a726f8`, using `OPENBIOS_SHELL=shell`. It verifies the expected 524288-byte image and uploads it as a one-day `openbios-handoff` artifact.
- Wired the six platform build jobs, asan, be32, and wasm32 to download that artifact from the same workflow run and recheck its digest before use. No cross-run cache or repository firmware bytes were added.
- Added registered positive, corrupt-transfer, and missing-transfer checks. The package component allowlist and release archive scan keep firmware out of build packages and publication artifacts.
- Reduced one redundant 300-frame guest replay after the be32 lane hit its seven-minute timeout. The existing `press60` test already pins the one-edge frame hash; the held snapshots still run through the guest, and the empty-input and three-edge guest cases remain separate.

## Evidence

- Focused transfer test: `cmake --preset ci && cmake --build --preset ci --target schwaystation-run && ctest --test-dir build/ci -R '^built\.openbios\.handoff$' --no-tests=error --output-on-failure` — exit 0.
- Focused refusal tests: the same configure/build sequence followed by `ctest --test-dir build/ci -R '^built\.openbios\.handoff_corruption$' --no-tests=error --output-on-failure`, then separately `ctest --test-dir build/ci -R '^built\.openbios\.handoff_missing$' --no-tests=error --output-on-failure` — both exit 0.
- Final local `cmake --workflow --preset ci` — exit 0; all 469 tests and five release archive checks passed. Final `cmake --workflow --preset hygiene` — exit 0; all 23 checks passed. The explicit budget check reported 1832 lines against the 1850-line cap.
- Captured `OPENBIOS_HANDOFF_RUN_ID=43083684528`. `workflow_dispatch` completed with `CI required` successful on exact head `11a79aeebacc9734c2bc5a5295e1815cfeb15b38`: https://github.com/szTheory/schwaystation/actions/runs/43083684528.
- The producer and all nine consumers reported 524288 bytes and SHA-256 `92c2f6d5e6ff056bf2ba177f37246e5468b6164337935297cf7b22266af20cc8`. The handoff, corruption, and missing-transfer tests passed in each consumer lane. The uploaded `openbios-handoff` artifact expires at `2026-11-12T19:04:25Z`; the run's `archives-*` artifacts are separate.
- The final be32 job passed all 436 of 436 tests in 6m57s under its unchanged seven-minute job timeout. `fixtures.pad_counter.movies` completed in 273.33s with the three remaining distinct guest scenarios.

## Task Commits

1. **Task 1:** `85308ba` — hand off checked OpenBIOS within each CI run.
2. **Task 1 fix:** `a427ddc` — order OpenBIOS artifact consumers.
3. **Task 1 fix:** `a3450ad` — scope handoff checks to actual consumers.
4. **Task 2:** `cf90c37` — add corrupt-handoff refusal coverage.
5. **Task 2 fix:** `10a289b` — reuse the shared expected-failure harness for corrupt and missing files.
6. **Measured efficiency fix:** `11a79ae` — reuse the pinned one-edge guest result.

## Files Created/Modified

- Created: `.planning/phases/04-games-play-in-retroarch/04-39-SUMMARY.md`.
- Modified: `.github/workflows/ci.yml`, `tests/built/tests.cmake`, `tests/built/inventory.txt`, `docs/testing.md`, `cmake/packaging.cmake`, `tests/fixtures/tests.cmake`, and `tests/diag/expected.csv`.
- An intermediate 38-line corruption helper at tests/cmake/openbios_handoff_corruption.cmake was intentionally removed and replaced with the existing `tests/cmake/expect_fail.cmake` harness to keep the repository-check budget below its cap.

## Decisions Made

- Preserve the pinned source/toolchain and supported C-shell build identity from plan 04-22; no retail BIOS or firmware bytes are committed.
- Use a one-day GitHub Actions artifact only for same-run transfer, with every consumer checking the digest before use.
- Keep corruption and missing-artifact cases as separate gating tests while sharing the repository's refusal harness.
- Keep GAME-05 incomplete because this plan delivers only the internal OpenBIOS handoff slice.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking issue] Wait for the OpenBIOS producer in every consumer job.**
- **Found during:** Task 1 hosted tracer.
- **Issue:** The first run dispatched asan, be32, and wasm32 before the artifact producer completed.
- **Fix:** Added the producer to each consumer job's `needs` list.
- **Files modified:** `.github/workflows/ci.yml`.
- **Verification:** The final exact-head run downloaded and verified the artifact in all nine consumers.
- **Committed in:** `a427ddc`.

**2. [Rule 3 - Blocking issue] Restrict missing-artifact checks to real firmware consumers.**
- **Found during:** Task 1 hosted tracer.
- **Issue:** A broad Actions-environment missing-artifact guard also blocked the configure-only perf job.
- **Fix:** Kept the missing-image refusal in each consumer pre-use step; build-only local/perf configurations retain the existing fixture path.
- **Files modified:** `.github/workflows/ci.yml`, `tests/built/tests.cmake`.
- **Verification:** The final hosted perf job and all consumer tests passed.
- **Committed in:** `a3450ad`.

**3. [Rule 3 - Blocking issue] Keep refusal controls inside the existing repository-check budget.**
- **Found during:** Task 2 local hygiene.
- **Issue:** A new 38-line helper raised the phase budget to 1870 lines, above the 1850-line limit.
- **Fix:** Removed the helper and reused `tests/cmake/expect_fail.cmake` for separate corrupt and missing transfer cases. The budget returned to 1832 lines. The helper's removal was intentional.
- **Files modified:** `tests/built/tests.cmake`, `tests/built/inventory.txt`; the intermediate helper was deleted.
- **Verification:** Both refusal tests and `policy.gate_budget` passed; final hygiene passed all 23 checks.
- **Committed in:** `10a289b`.

**4. [Rule 3 - Blocking issue] Preserve the be32 run within its measured job bound.**
- **Found during:** Final hosted validation of the first completed test set.
- **Issue:** Run `43079724359` reached the seven-minute job limit with 435/436 tests complete; only `fixtures.pad_counter.movies` had not completed. Its four guest replays were the largest controllable test work, while `runner.hashes_file` took 106.77 seconds and GPU motion took 43.65 seconds in that run.
- **Fix:** Reused the existing pinned one-edge hash from `fixtures.pad_counter.press60` for the held-snapshot comparison. Kept the empty-input, repeated-held, and three-edge guest proofs at frame 300; retained the exact one-edge/pixel/hash assertions through the existing press60 run. The timeout remains seven minutes.
- **Files modified:** `tests/fixtures/tests.cmake`, `tests/diag/expected.csv`.
- **Verification:** Final run `43083684528` passed all 436 be32 tests in 6m57s and passed the required check.
- **Committed in:** `11a79ae`.

**Total deviations:** 4 direct fixes. **Impact:** Corrected handoff scheduling, preserved the 1850-line budget, and retained all distinct guest assertions while allowing be32 to complete without raising its timeout.

## Issues Encountered

- The first local full-CI attempt overlapped with hygiene and timed out `core.state.record_sweep`; an isolated retry passed in 6.26 seconds. The final full CI and hygiene workflows ran sequentially and passed.
- Two intermediate hosted runs were canceled while fixing artifact ordering and the consumer-only missing-artifact guard. The completed first tracer run and final exact-head run both succeeded. The only final-run evidence is from run `43083684528`.

## User Setup Required

None.

## Next Phase Readiness

Plan 04-39 is complete. Plan 04-23 remains the next runner BIOS CLI slice, and plan 04-40 retains the separate actual OpenBIOS C-shell/boot inspection. GAME-05 remains open until its remaining planned work is complete.

## Self-Check: PASSED

- The summary file exists at the planned phase path.
- All six task commits listed above are ancestors of HEAD.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-11*
