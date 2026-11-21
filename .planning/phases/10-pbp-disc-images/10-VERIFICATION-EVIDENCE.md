# Phase 10 Verification Evidence

This records Plan 10-13's final local verification and the subsequent Phase 10 ship repair. The first PR and
Nightly attempts ran on pre-repair commit `50efc31`, exposed actionable failures, and were cancelled. The
repaired local head passes; exact repaired-head CI/Nightly and release-pipeline gates remain pending. Do not
advance to Phase 11 until those gates are green and the release is published.

## Frozen review and cheap checks

The phase-wide review at `cc46618` was clean after resolving WR-01 in `76bdc68`; the report is
`10-REVIEW.md`. The ship correction diff was independently reviewed clean before the final local workflow and
the added sparse-checkout dependency was re-reviewed clean after the failure-direction fix.

| Command | Result |
| --- | --- |
| `LC_ALL=C sort -cu tests/disc/inventory.txt` (and fuzz, runner, libretro inventories) | exit 0 |
| `cmake -DMODE=pins -DDIR=.github/workflows -P tests/cmake/workflows.cmake` | exit 0; all 61 workflow action uses are pinned |
| `cmake -DMODE=timeouts -DDIR=.github/workflows -P tests/cmake/workflows.cmake` | exit 0; all 26 timeout lines have run evidence/provisional comments |
| `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` | exit 0; 3,900/3,901 lines |
| `cmake -DCASE=disc_registration -DROOT=. -P tests/cmake/workflows.cmake` | exit 0 |
| `cmake --preset ci` | exit 0 |
| `cmake -DQUALITY=quality.json -DMETRIC=size.wasm -DMEASURED=875971 -P tests/cmake/quality.cmake` | exit 0; matches the measured hosted artifact |
| `cmake -DCASE=disc_registration -DROOT=. -P tests/cmake/workflows.cmake` | exit 0; protected-base sparse-checkout closure is checked |
| `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` | exit 0; 3,901/3,901 lines |
| `act workflow_dispatch --dryrun -W .github/workflows/nightly.yml -j fuzz -P ubuntu-26.04=ubuntu:latest --action-cache-path /private/tmp/schwaystation-act-cache` | exit 0; Nightly changes and fuzz routing/dependency steps parsed |
| `actionlint -ignore 'label "ubuntu-26[.]04(-arm)?"' .github/workflows/nightly.yml` | exit 0; installed actionlint has a stale runner-label catalog for the existing Ubuntu 26 labels |
| `git diff --check HEAD` | exit 0 |

## Local CI workflow

The pre-correction run exposed one stale assertion in `compat.probe.malformed`: it expected PBP to be unsupported.
The malformed fixture is now expected to fail with `PBP metadata is invalid` and still redact its private path.
The focused rerun passed 2/2 including the compatibility preflight.

| Command | Exit | Evidence |
| --- | ---: | --- |
| `ctest --test-dir build/ci --no-tests=error -R '^compat[.]probe[.]malformed$' --output-on-failure` | 0 | 2/2, 0.07s |
| `cmake --workflow --preset ci` | 0 | 655/655 standard tests, 7/7 release checks; CTest 78.09s, release 1.38s |
| `ctest --preset hygiene --no-tests=error -R '^ci[.]classify$' --output-on-failure` | 0 | 1/1, 5.58s |
| `ctest --preset ci --no-tests=error -R '^ci[.]classify_policy$|^disc[.]image[.]pbp_' --output-on-failure` | 0 | 11/11, 0.18s |
| `cmake --build --preset ci --target schwaystation_disc_image` | 0 | PBP host module compiled and linked |
| `ctest --preset ci --no-tests=error -R '^compat[.]ratchet[.](protected_controls|rows)$' --output-on-failure` with full base checkout in `COMPAT_BASE_DIR` | 0 | 7/7 including required fixture setup; sparse hosted checkout now guards this file closure |
| `act pull_request --dryrun -W .github/workflows/ci.yml` with runner mappings and `/private/tmp/schwaystation-act-cache` | 0 | event routing and step wiring parsed; no hosted action ran |

PBP provider, runner, libretro, fixture and compatibility selectors ran in the final CTest inventory. The full
local suite took 75.65s; the seven release checks took 1.35s. It creates the local package archives as part of
the five-step CMake workflow. The command's console output was retained only at
`/private/tmp/schwaystation-phase10-ci-final.log`; it is not committed.

## Hosted gates still required at ship

The first exact-head attempt ran on commit `50efc31217b851de8cbe3b53677b525b9b65447d` and was cancelled after
the failures listed in `10-CI-EFFICIENCY.md`; it did not satisfy the ship gate. The WASI job produced the actual
linked module measurement of 875,971 bytes (run
[44924014258](https://github.com/szTheory/schwaystation/actions/runs/44924014258), job
[136906260832](https://github.com/szTheory/schwaystation/actions/runs/44924014258/job/136906260832)), now recorded
in `quality.json`. On the repaired head, inspect one exact-head PR CI run and Nightly run, including all native
and package lanes, ASan, nofp, BE32, WASI, eight-way hash equality, RetroArch, release rehearsal and the full
Nightly route. Re-measure the linked module from that run and adjust the bound only if the new measurement differs.

The local Mac has Apple Clang and Wasmtime but no WASI sysroot or wasm compiler runtime; the BE32 cross compiler
and QEMU are also absent, and the existing `build/be32` cache belongs to an old `/repo` checkout. `act --dryrun`
establishes routing only. It cannot establish the native platform matrix, ARM runners, hosted artifact handoffs
and permissions, MSVC, BE32, WASI size, macOS RetroArch, or release API integration. Use GitHub workflow-run/job
APIs or `gh` for live monitoring and record run URLs.
