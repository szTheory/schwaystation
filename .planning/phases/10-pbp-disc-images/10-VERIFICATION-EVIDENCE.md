# Phase 10 Verification Evidence

This records Plan 10-13's final local verification, ship repairs, and exact-head hosted CI/Nightly result. The
initial PR/Nightly attempts exposed fixable integration failures; they were repaired and independently reviewed.
The final exact-head CI and full Nightly gates are green. Phase 10 remains at ship until its PR is merged and the
automatic release is published; do not advance to Phase 11 before that boundary is confirmed.

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

## Initial hosted attempt and WASI bound

The first exact-head attempt ran on commit `50efc31217b851de8cbe3b53677b525b9b65447d` and was cancelled after
the failures listed in `10-CI-EFFICIENCY.md`; it did not satisfy the ship gate. The WASI job produced the actual
linked module measurement of 875,971 bytes (run
[44924014258](https://github.com/szTheory/schwaystation/actions/runs/44924014258), job
[136906260832](https://github.com/szTheory/schwaystation/actions/runs/44924014258/job/136906260832)), now recorded
in `quality.json`. The final repaired-head checks and their inspection are recorded below.

The local Mac has Apple Clang and Wasmtime but no WASI sysroot or wasm compiler runtime; the BE32 cross compiler
and QEMU are also absent, and the existing `build/be32` cache belongs to an old `/repo` checkout. `act --dryrun`
establishes routing only. It cannot establish the native platform matrix, ARM runners, hosted artifact handoffs
and permissions, MSVC, BE32, WASI size, macOS RetroArch, or release API integration. Use GitHub workflow-run/job
APIs or `gh` for live monitoring and record run URLs.

## Second ship-repair attempt — 2026-11-21

The repaired head `0cbf4fa1c9e56893cc34ea448e88388b88fae549` ran as
[CI 44927726830](https://github.com/szTheory/schwaystation/actions/runs/44927726830) and
[Nightly 44927726826](https://github.com/szTheory/schwaystation/actions/runs/44927726826). WASI passed and accepted
the 875,971-byte measurement. Linux x64 failed while linking the SDL2 example because the source-built host
image layer omitted the PBP reader and inflater. Nightly fuzz failed because its explicit fuzz build preset did
not build the owned PBP image before `fuzz.regress.pbp` replay. That PBP replay was last in the campaign loop,
after earlier timed campaigns.

The example target now builds and links the PBP provider plus the private inflater library; the local SDL2
example build passed. The full Nightly route now builds `schwaystation-pbp-fixtures`, and seed generation and
count checks run before any campaign. Local `schwaystation-fuzz-pbp-replay --write` produced 14 seeds; the
updated workflow-closure check, actionlint, and `git diff --check` passed. The runner-time totals and cancellations
are recorded in `10-CI-EFFICIENCY.md`; these two failed/cancelled runs do not satisfy the ship gate.

## Seed-order assertion review follow-up — 2026-11-21

The independent reviewer found a false-positive path in the regression assertion: the route's literal `||` was
treated as regex alternation, and the loader-campaign fragment came before the seed loop's completion. Both were
corrected without increasing the gate budget. The focused commands below were run after review:

| Command | Exit | Result |
| --- | ---: | --- |
| `cmake -DCASE=disc_registration -DROOT=. -P tests/cmake/workflows.cmake` | 0 | Actual Nightly seed and campaign ordering accepted |
| `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` | 0 | 3,901/3,901 lines |
| Temporary negative fixture, same workflow check | 1 (expected) | Campaign moved inside seed loop rejected |
| `actionlint -ignore 'label "ubuntu-26[.]04(-arm)?"' .github/workflows/ci.yml .github/workflows/nightly.yml` | 0 | Workflow syntax and action schema checks pass |
| `git diff --check` | 0 | No whitespace errors |

The hosted exact-head CI/Nightly and release-pipeline runs remain the ship gates; the preceding failed runs are
linked above and do not count as passing evidence.

## Clean fixture-graph follow-up — 2026-11-21

The exact-head Nightly run [44931682921](https://github.com/szTheory/schwaystation/actions/runs/44931682921)
failed in the fuzz job because the generated media files used by the PBP source image were not declared as
outputs to Ninja. The required CI run
[44931682851](https://github.com/szTheory/schwaystation/actions/runs/44931682851) was cancelled after local
reproduction and repair; all six native platform builds, WASI, and the other listed completed gates passed, but
the aggregate required check did not.

| Command | Exit | Result |
| --- | ---: | --- |
| `env -u SCHWAYSTATION_DISC_MEDIA_DIR cmake -S . -B /private/tmp/schwaystation-pbp-graph -G Ninja -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DCMAKE_BUILD_TYPE=Release -DSCHWAYSTATION_CHD=ON -DSCHWAYSTATION_BUILD_CHD_TOOL=OFF -DSCHWAYSTATION_CHD_TOOL_EXECUTABLE=build/chd-tool/tools/schwaystation-chd-tool -DSCHWAYSTATION_BUILD_TESTS=ON -DSCHWAYSTATION_BUILD_RUNNER=ON -DSCHWAYSTATION_BUILD_LIBRETRO=ON -DSCHWAYSTATION_FIXTURES=ON` | 0 | Fresh no-external-media CMake graph configured |
| `ninja -C /private/tmp/schwaystation-pbp-graph -t query fixtures/media/cdda.wav` | 0 | `cdda.wav` has a `CUSTOM_COMMAND` producer |
| `ninja -C /private/tmp/schwaystation-pbp-graph -t query fixtures/media/xa.xa` | 0 | `xa.xa` has a `CUSTOM_COMMAND` producer |
| `ninja -C /private/tmp/schwaystation-pbp-graph -t query fixtures/media/movie.str` | 0 | `movie.str` has the same declared media producer |
| `cmake -DCASE=disc_registration -DROOT=. -P tests/cmake/workflows.cmake` | 0 | Nightly ordering and media-output closure pass |
| Repository line-budget command (above) | 0 | 3,901/3,901 lines |
| Temporary Python negative-control harness; moves media BYPRODUCTS to an unrelated command, then runs the workflow check | 0 outer / 1 expected inner | The guard rejects declarations not attached to the `media.sha256` producer |

The build graph check ran with the media directory unset and did not build the encoder or start fuzz campaigns.
The CMake workflow guard now requires the byproducts in the same command block as `media.sha256`; independent
review found this binding necessary and re-reviewed the correction clean. A temporary negative control that
moved the declarations to another command was rejected. The runs above remain failed/cancelled and are not ship
approval.

## Final exact-head CI and Nightly — 2026-11-21

Shipping head `df838815a5a4afd99f95d50e8caf66813aaa7cb4` passed PR [CI 44947489771](https://github.com/szTheory/schwaystation/actions/runs/44947489771)
and [Nightly 44947489816](https://github.com/szTheory/schwaystation/actions/runs/44947489816). CI passed its six
native builds, ASan, no-float, BE32, WASI, eight-way hash equality, RetroArch replay, release rehearsal, cleanup
and required aggregate. Nightly passed its complete vectors, benchmark and fuzz route. The WASI linked-module
comparison passed the recorded 875,971-byte ceiling. A successful comparison does not emit its measured number
and the module is not retained as a workflow artifact, so this records the final run as within the bound without
claiming a new exact size or changing the bound.

CI took 63m30s wall and 8,250 summed job-seconds across 22 jobs; BE32/QEMU took 62m18s. Hash equality began two
seconds after BE32 completed, and the required aggregate finished ten seconds later. Nightly took 9m43s wall and
908 summed job-seconds across eight jobs. Dollar billing was unavailable. The prior comparable CI critical path
was within six seconds of this result, so the measured evidence does not support a coverage-preserving workflow
reduction. See `10-CI-EFFICIENCY.md` for the run comparison and disposition.

| Hosted gate | Command/event | Exit | Evidence |
| --- | --- | ---: | --- |
| PR CI | `pull_request` on `df838815a5a4afd99f95d50e8caf66813aaa7cb4` | 0 | [Run 44947489771](https://github.com/szTheory/schwaystation/actions/runs/44947489771); all required jobs passed |
| Nightly | `pull_request` on `df838815a5a4afd99f95d50e8caf66813aaa7cb4` | 0 | [Run 44947489816](https://github.com/szTheory/schwaystation/actions/runs/44947489816); full route passed |
| WASI linked size | Hosted `cmake -DQUALITY=quality.json -DMETRIC=size.wasm -DARTIFACT=build/wasm32/schwaystation-run -P tests/cmake/quality.cmake` | 0 | [WASI job](https://github.com/szTheory/schwaystation/actions/runs/44947489771/job/136978138043); within 875,971-byte ceiling |

The existing `act pull_request --dryrun` is routing evidence only. Workflow files did not change after that dry
run; hosted-only runners, permissions and artifact handoffs are established by the successful hosted jobs. The
ship PR and automatic release pipeline remain the Phase 10 boundary.
