---
phase: 14-multitap-and-special-controllers
plan: "16"
subsystem: quality-and-documentation
tags: [controllers, saves, quality-bounds, state-hashes]
requires:
  - phase: 14-15
    provides: Tested compatibility ledger and assembled controller implementation
provides:
  - Stable-state size bound tied to the current producer measurement
  - Updated frame-150 and frame-180 protocol-state hashes
  - Controller, save, and runner documentation reachable from existing guides
  - Exact hosted producers recorded for metrics unavailable locally
affects: [phase-14-verification, quality-metrics, peripheral-documentation]
actuals:
  tokens: 1483
  tasks: 1
  commits: 1
commits: 1
plan_head_before: 107d649a7595ab71c7b87cee7b3848413f18eb82
plan_head_after: b51876c4ed8f60460c3229557073d8cdd72b99b6
tech-stack:
  added: []
  patterns:
    - Change quality bounds and state pins only from named producer output.
key-files:
  created: []
  modified:
    - quality.json
    - tests/fixtures/tests.cmake
    - docs/embedding.md
    - docs/testing.md
key-decisions:
  - Set state.bytes to the exact 3,757,638-byte local stable-state producer result.
  - Leave WASI size and Ir/frame bounds unchanged until their hosted producers report exact values.
  - Keep SIO-01 through SIO-04 open for phase-level verification.
patterns-established:
  - Reproduce each affected protocol-state hash directly, then verify the updated focused selector.
requirements-completed: []
coverage:
  - id: C1
    description: The tap protocol-state measurements and the two GPU-motion state pins are current for this plan; phase-level SIO-01 acceptance remains for final verification.
    requirement: SIO-01
    verification:
      - kind: integration
        ref: "build/ci/tests/schwaystation-test-state state.stable_bytes and the two direct gpu_motion state-hash commands exited 0; core.state.stable_bytes passed 1/1 and fixtures.gpu_motion.state.150/.180 passed 2/2."
        status: pass
    human_judgment: false
  - id: C2
    description: The existing embedding guide links to the controller protocols, including the documented mouse contract; this is documentation reachability, not a standalone SIO-02 completion claim.
    requirement: SIO-02
    verification:
      - kind: other
        ref: "docs/embedding.md links to the existing docs/controllers.md page; target existence and the modified-page byte budget checks exited 0."
        status: pass
    human_judgment: false
  - id: C3
    description: The existing embedding guide links to the special-controller protocols; this is documentation reachability, not a standalone SIO-03 completion claim.
    requirement: SIO-03
    verification:
      - kind: other
        ref: "docs/embedding.md links to the existing docs/controllers.md page; target existence and the modified-page byte budget checks exited 0."
        status: pass
    human_judgment: false
  - id: C4
    description: The embedding and testing guides link to the save and runner contracts; this is documentation reachability, not a standalone SIO-04 completion claim.
    requirement: SIO-04
    verification:
      - kind: other
        ref: "docs/embedding.md links to docs/saves.md, docs/testing.md links to docs/runner.md, and all targets exist; byte-budget checks exited 0."
        status: pass
    human_judgment: false
metrics:
  duration: 32m
  completed: 2026-11-24
status: complete
---

# Phase 14 Plan 16: Measured state and discoverable peripheral docs Summary

**The state-size bound and two protocol-state pins now match measured outputs, and existing guides expose the controller, save, and runner documentation.**

## Performance

- **Runtime/model:** Codex GSD executor; configured `gpt-6-luna` xhigh; primary runtime model ID unavailable.
- **Duration:** 32 minutes (started 2026-11-24T12:25:11Z; includes GSD closeout).
- **Completed:** 2026-11-24.
- **Tasks:** 1.
- **Files modified:** 4 task files; this summary is the plan artifact.

## Accomplishments

- The local stable-state producer measured 3,757,638 bytes, 2,306 bytes above the prior bound; `quality.json` and the current-value note in `docs/testing.md` now use that exact measurement.
- Independently reproduced the frame-150 and frame-180 `gpu_motion` state hashes and updated only those two `STATE_HASH` values. Frame, image, and audio pins, cycle caps, and expected source counts were not changed.
- Linked `docs/controllers.md` and `docs/saves.md` from `docs/embedding.md`, and `docs/runner.md` from `docs/testing.md`.
- Kept the active WASI size and Ir/frame bounds unchanged because their exact producers are not available locally.

## Task Commits

1. **Task 1: Measure affected metrics and make documentation discoverable** — `b51876c` (docs).

**Plan metadata:** this summary, STATE, and ROADMAP are included in the GSD bookkeeping commit.

## Files Created/Modified

- `quality.json` — state-size floor and ceiling now match the measured 3,757,638-byte result, with producer attribution.
- `tests/fixtures/tests.cmake` — only the frame-150 and frame-180 state hashes were updated.
- `docs/embedding.md` — links to controller and save contracts.
- `docs/testing.md` — links to runner usage and reports the current state-size measurement.
- `.planning/phases/14-multitap-and-special-controllers/14-16-SUMMARY.md` — records results and the hosted-metric handoff.

## Checks

- `git diff --check` — exit 0.
- `LC_ALL=C sort -cu` on `tests/abi/inventory.txt`, `tests/sio/inventory.txt`, `tests/card/inventory.txt`, `tests/libretro/inventory.txt`, and `tests/runner/inventory.txt` — exit 0 for all five inventories.
- Repository-check line budget at 3,901 lines and RetroArch test budget at 300 lines — exit 0 for both existing limits.
- `cmake --preset ci` — exit 0; reused `build/ci`.
- `cmake --build --preset ci --target schwaystation-test-state` — exit 0; target was already current.
- `build/ci/tests/schwaystation-test-state state.stable_bytes` — exit 0; printed exactly `state_bytes=3757638`.
- `cmake -DQUALITY=quality.json -DMETRIC=state.bytes -DMEASURED=3757638 -P tests/cmake/quality.cmake` — exit 0.
- `ctest --preset ci --no-tests=error -R '^core[.]state[.]stable_bytes$' --output-on-failure` — exit 0; 1/1 passed in 0.15 seconds.
- `build/ci/schwaystation-run --exe build/ci/fixtures/gpu_motion.exe --frames 150 --hash-state` — exit 0; state SHA-256 `a9218281c1d5b51fff4f48fc9fa388cf0459c5c0bf136b8ecb462cbb0d466d9b`.
- `build/ci/schwaystation-run --exe build/ci/fixtures/gpu_motion.exe --frames 180 --hash-state` — exit 0; state SHA-256 `b8285ab088a9a7d19afc2986913aa3f16780bb8fd52bea2c8b75e2d538076124`.
- `ctest --preset ci --no-tests=error -R '^fixtures[.]gpu_motion[.]state[.](150|180)$' --output-on-failure` — exit 0; 2/2 passed in 1.40 seconds.
- Link-target existence checks — exit 0. Modified documentation page sizes are 47,006 bytes (`docs/embedding.md`) and 61,194 bytes (`docs/testing.md`), both below the 61,440-byte limit.
- `jq -e . quality.json` and the state-bound assertion — exit 0.
- Earlier exact producer-value preflights for `size.wasm=947976`, `ir_per_frame.testcard=1620101`, and `ir_per_frame.gpu_motion=138319540` via `tests/cmake/quality.cmake` — exit 0 for all three before the final watchdog-table representation change.
- After the SIO and C17 libretro portability fixes and measured-pin updates, `cmake --workflow --preset ci` — exit 0; 705/705 CTest tests passed in 69.07 seconds, all three macOS arm64 packages were produced, and the CI release selectors passed 8/8 in 1.47 seconds.
- In named Ubuntu 26.04 arm64 container `schwaystation-phase14-gcc-libretro`, GCC 15.2 warnings-as-errors configure with `SCHWAYSTATION_NOFP=ON`, then `cmake --build /tmp/schwaystation-phase14-gcc-libretro --target schwaystation_libretro` — exit 0. The `--rm` container was removed automatically.
- `cmake --build build/ci --target schwaystation-test-libretro-host` — exit 0; the focused card/input selector group `libretro.cards.persistence`, `libretro.cards.controls`, `libretro.cards.multitap`, and `libretro.input.multitap` passed 4/4 in 5.16 seconds.

## Hosted Metrics

- The exact Ubuntu 26.04 WASI producer measured the linked Release runner at 947,976 bytes on PR run 45420382438,
  job 138538583783. Its quality comparison failed only because the stored exact pin was stale; the new floor and ceiling
  tracked that result for source before the watchdog-table representation change: https://github.com/szTheory/schwaystation/actions/runs/45420382438/job/138538583783.
- The initial final-source local reproduction measured 947,899 bytes in Ubuntu 26.04 arm64 with
  `SCHWAYSTATION_BUILD_TESTS=OFF`; this did not match the hosted x86_64 `wasm32-chd` preset and the bound update was
  superseded. Main CI [45436318507](https://github.com/szTheory/schwaystation/actions/runs/45436318507) measured
  947,969 bytes on x86_64 and failed only because the temporary ceiling was 947,899. A task-named x86_64 Ubuntu 26.04
  container with Clang/LLD 21.1.8, the same WASI packages, CHD enabled and test registration enabled built
  `schwaystation-run` at exactly 947,969 bytes. `cmake --build /tmp/schwaystation-phase14-wasi-x64-exact-build
  --target schwaystation-run` and `stat -c 'WASI_MODULE_BYTES=%s' ...` exited 0. This producer-only reproduction did not
  run WASI CTest; CMake's emulator and fixture-tool paths were placeholders used only to configure test registration.
  After setting floor/ceiling to 947969, `cmake -DQUALITY=quality.json -DMETRIC=size.wasm -DMEASURED=947969 -P
  tests/cmake/quality.cmake` exited 0; `ctest --test-dir build/ci --output-on-failure -R '^policy[.]quality_metrics$'
  --no-tests=error` passed 1/1 in 47.15 seconds. `jq -e . quality.json`, the SIO inventory sort and `git diff --check`
  also exited 0.
- Main CI [45428490941](https://github.com/szTheory/schwaystation/actions/runs/45428490941) failed before the WASI build
  because that runner's apt update saw Microsoft's package feed but no Ubuntu package index. The same-head Nightly
  [45428491372](https://github.com/szTheory/schwaystation/actions/runs/45428491372) installed the same packages and
  passed its WASI benchmark, full vectors, native benchmarks and fuzz. Main CI was canceled after 1,159 seconds wall
  time once the local exact producer revealed the stale pin; GitHub's timing API returned zero billable milliseconds.
- On corrected-source head `d46ec1e`, Main CI run [45436318507](https://github.com/szTheory/schwaystation/actions/runs/45436318507)
  reached the size gate and reported 947,969 against the mistaken 947,899 ceiling. WASI setup, fixture generation,
  configure and runner build passed; only the expected-value comparison failed. After confirming the exact result with
  the x86_64 tests-enabled local producer, the rest of Main CI and Nightly run
  [45436318741](https://github.com/szTheory/schwaystation/actions/runs/45436318741) were canceled. Main ran 323 seconds
  total; Nightly ran 292 seconds; its fuzz job lasted 283 seconds wall, with the bounded-campaign step canceled after
  180 seconds. Both timing endpoints reported zero billable
  milliseconds, so no runner-minute savings are claimed. The focused corrected bound check and a final hosted retry
  remain pending.
- The exact Linux x64 Cachegrind producer measured testcard at 1,620,101 and `gpu_motion` at 138,319,540 Ir/frame
  on the same run, job 138538117721. Both relative comparisons passed; the absolute comparisons exceeded their exact
  pins by 528 Ir/frame, so those pins now track the producer outputs without changing the 100-basis-point regression
  limit: https://github.com/szTheory/schwaystation/actions/runs/45420382438/job/138538117721.
- The WASI producer is now locally reproducible in a task-named Ubuntu x86_64 container when test registration is
  enabled to match `wasm32-chd`; an earlier arm64/tests-off reproduction under-measured it. The local macOS host still
  lacks Cachegrind, so instructions-per-frame remains hosted-only. The initial hosted run exposed GCC SIO bounds and C17
  libretro qualifier issues; both are fixed and covered by local GCC builds/tests. A later no-FP run exposed the
  pointer-table ABI classification, fixed with a read-only table. The final exact size is measured locally; updated
  Main CI and Nightly results remain required before ship.

## Decisions Made

- The state-size change is based on the exact local stable-state producer output; no other unmeasured bound changed.
- Requirements-completed remains empty because the shared SIO requirements are reconciled at phase verification.

## Deviations from Plan

The implementation and focused checks finished around 12 minutes, but GSD closeout extended total wall time to about 32 minutes, exceeding the 30-minute slice cap by about 2 minutes. Cachegrind remains hosted-only on this host; the WASI module was reproduced later with the pinned Ubuntu 26.04 toolchain in a temporary container.

## Issues Encountered

The workspace sandbox denied the required GSD plan-ledger write inside `.git`. The authorized bookkeeping write and task commit then succeeded through the sandbox escalation path; no project files were affected by the initial denial.

## Next Phase Readiness

The implementation handoff is ready for the parent orchestrator's source review. After review, run the one required aggregate local workflow and the hosted gates on the final reviewed head.

## Self-Check: PASSED

- Summary and all four task files exist.
- Task commit `b51876c4ed8f60460c3229557073d8cdd72b99b6` is an ancestor of HEAD.
- Summary whitespace check passed; the measured plan commit count is 1 from `107d649a7595ab71c7b87cee7b3848413f18eb82` through `b51876c4ed8f60460c3229557073d8cdd72b99b6`.
