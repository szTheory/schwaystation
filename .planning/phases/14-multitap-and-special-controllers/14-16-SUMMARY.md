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

## Hosted Metrics Deferred

- WASI module size remains 923,753 bytes until the exact `.github/workflows/ci.yml` `wasm32` job, `Measure linked WASI module` step, supplies a new measurement. This host has no `clang-21`, `wasm32-wasi-clang`, or `wasm-ld`; no WASI bound was guessed or widened. The pinned `wasmtime` runtime is present but does not build the module.
- Ir/frame remains at the measured bounds in `quality.json` until the `.github/workflows/ci.yml` `perf` job, `Instructions per frame` step, runs with its `Cachegrind` setup. This host has no `valgrind`/Cachegrind.
- The full `cmake --workflow --preset ci` run is intentionally left for the parent orchestrator after source review, so the phase aggregate runs once on the reviewed head.

## Decisions Made

- The state-size change is based on the exact local stable-state producer output; no other unmeasured bound changed.
- Requirements-completed remains empty because the shared SIO requirements are reconciled at phase verification.

## Deviations from Plan

The implementation and focused checks finished around 12 minutes, but GSD closeout extended total wall time to about 32 minutes, exceeding the 30-minute slice cap by about 2 minutes. The plan's conditional hosted measurements remain pending because their pinned toolchains are unavailable locally.

## Issues Encountered

The workspace sandbox denied the required GSD plan-ledger write inside `.git`. The authorized bookkeeping write and task commit then succeeded through the sandbox escalation path; no project files were affected by the initial denial.

## Next Phase Readiness

The implementation handoff is ready for the parent orchestrator's source review. After review, run the one required aggregate local workflow and the hosted gates on the final reviewed head.

## Self-Check: PASSED

- Summary and all four task files exist.
- Task commit `b51876c4ed8f60460c3229557073d8cdd72b99b6` is an ancestor of HEAD.
- Summary whitespace check passed; the measured plan commit count is 1 from `107d649a7595ab71c7b87cee7b3848413f18eb82` through `b51876c4ed8f60460c3229557073d8cdd72b99b6`.
