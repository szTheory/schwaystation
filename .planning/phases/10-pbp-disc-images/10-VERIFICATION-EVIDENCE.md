# Phase 10 Verification Evidence

This records Plan 10-13's final local verification. The required exact-head hosted PR run will happen during
the authorized Phase 10 ship step after GSD phase verification; no standalone manual dispatch was started, to
avoid duplicating the full CI and Nightly workloads. Therefore hosted CI/Nightly and the final linked WASI size
remain pending here until that run is inspected.

## Frozen review and cheap checks

The phase-wide review at `cc46618` was clean after resolving WR-01 in `76bdc68`; the report is
`10-REVIEW.md`. The later test-only correction in `e9ce168` was independently reviewed before the final local
workflow. Its focused selector passed.

| Command | Result |
| --- | --- |
| `LC_ALL=C sort -cu tests/disc/inventory.txt` (and fuzz, runner, libretro inventories) | exit 0 |
| `cmake -DMODE=pins -DDIR=.github/workflows -P tests/cmake/workflows.cmake` | exit 0; all 61 workflow action uses are pinned |
| `cmake -DMODE=timeouts -DDIR=.github/workflows -P tests/cmake/workflows.cmake` | exit 0; all 26 timeout lines have run evidence/provisional comments |
| `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` | exit 0; 3,900/3,901 lines |
| `cmake -DCASE=disc_registration -DROOT=. -P tests/cmake/workflows.cmake` | exit 0 |
| `cmake --preset ci` | exit 0 |
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
| `cmake --workflow --preset ci` | 0 | 655/655 standard tests and 7/7 release tests; 78s total, with incremental build |

PBP provider, runner, libretro, fixture and compatibility selectors ran in the final CTest inventory. The full
local suite took 75.65s; the seven release checks took 1.35s. It creates the local package archives as part of
the five-step CMake workflow. The command's console output was retained only at
`/private/tmp/schwaystation-phase10-ci-final.log`; it is not committed.

## Hosted gates still required at ship

No PR or standalone workflow run exists on this local branch yet. During `$gsd-ship 10`, inspect one exact-head
PR-triggered CI run and Nightly run, including all six native/package lanes, ASan, nofp, BE32, WASI, eight-way
hash equality, RetroArch, release rehearsal and the full Nightly route. The mixed source and workflow change
must select full Nightly work. Capture the linked WASI module's measured size from its actual job log before
changing `quality.json`; the current 859,379-byte CHD-enabled bound is the last valid producer measurement and
remains unchanged. Do not substitute the native macOS binary size.

The local Mac has Apple Clang and Wasmtime but no WASI sysroot or wasm compiler runtime, and
`build/wasm32/schwaystation-run` is absent. `act --dryrun` establishes route parsing only. It cannot establish
the native platform matrix, ARM runners, hosted artifact handoffs and permissions, macOS RetroArch, or release
API integration. The skill's `scripts/ci_monitor.cjs` helper is absent from this checkout; use the GitHub
workflow-run/job APIs or `gh` for live monitoring and record run URLs.
