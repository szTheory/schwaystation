---
title: "Measure safe artifact reuse between main CI and Release CI"
status: complete
priority: P2
resolves_phase: 9
source: "Phase 7 same-commit CI and Release workflows"
created: 2026-11-19
theme: ci
---

## Goal

Determine whether the main push and Release workflows can reuse exact-SHA, fingerprinted CI outputs without
weakening the required main check, the Release check, matrix coverage, or exact-release-SHA archive validation.

## Context

- On release commit `af64484f388eddf67c615673b98edd6ccb5dae4c`, main push CI and the automatic Release workflow
  each ran a full matrix. Both passed; their overlap means duplicate runner minutes are observed, but no
  wall-clock saving is established.
- Release-side exact-SHA package validation and main's required check are independent delivery guarantees.
  Preserve both unless a tested producer/consumer design proves equivalent source, matrix and artifact identity.
- Bind to the Phase 9 plan that first edits `.github/workflows/ci.yml` or `.github/workflows/release.yml`.

## Phase 9 Plan 10 measurement

Main CI run [44645037841](https://github.com/szTheory/schwaystation/actions/runs/44645037841) and Release run
[44645037923](https://github.com/szTheory/schwaystation/actions/runs/44645037923) both passed on source SHA
`830bf4cfe74aca92930e34fdf9667c5c1b24c370`. Main used 139.13 runner-minutes; Release's repeated CI jobs used
117.53 runner-minutes and its full run completed in 64m06s versus Main's 82m22s. The six artifacts from both
runs carried the exact source SHA, but all 18 Main product ZIP digests differ from the corresponding Release
and published v0.8.0 asset digests. See `09-CI-REUSE.md` for the complete per-job timings and digests.

**Disposition:** retain both workflows and every matrix lane. Reuse would require a new, attempt-bound external
artifact handoff with negative controls for wrong SHA, wrong attempt, incomplete/expired artifacts, missing or
duplicate lanes, and startup races. Waiting for Main's required check before publication projects Release at
about 88m31s, 24m25s slower than this observed Release run. Plan 11 must verify the separate gates and current
archive validation remain intact after its workflow edits, then close this todo; no cross-run reuse is authorized.

## Phase 9 Plan 11 verification

Runtime/model: Codex GSD executor, gpt-6-luna xhigh, 2026-11-20.

Plan 11 kept Main CI and Release CI independent, left `release.yml`'s exact-SHA `release-ci` call and `publish`
gates intact, and retained all 18 product ZIP names and the `SHA256SUMS` asset. The release pipeline has no
cross-run artifact transfer or cache. The measured Phase 10 disposition above remains the evidence for keeping
these gates separate.

The new preflight runs once in the existing CI `changes` job after checkout and before jobs that depend on it;
nightly fuzz runs the same checks before tool setup. Local preflight results:

| Check | Command | Exit | Elapsed |
|---|---|---:|---:|
| Workflow action pins | `cmake -DMODE=pins -DDIR=.github/workflows -P tests/cmake/workflows.cmake` | 0 | 0.05 s |
| Workflow timeout evidence | `cmake -DMODE=timeouts -DDIR=.github/workflows -P tests/cmake/workflows.cmake` | 0 | 0.05 s |
| Selector and fixture routes | `cmake -DCASE=disc_registration -DROOT=. -P tests/cmake/workflows.cmake` | 0 | 0.08 s |
| Sorted source/test inventories | `LC_ALL=C sort -cu` for each `tests/inventory/*.txt`, `tests/disc/inventory.txt`, `tests/fuzz/inventory.txt`, and `tests/release/inventory.txt` | 0 | 0.07 s |
| Repository-check budget | `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` | 0 | 0.07 s |

`cmake --preset ci` exited 0 in 1.02 s and set `SCHWAYSTATION_CHD=ON` and `SCHWAYSTATION_BUILD_CHD_TOOL=ON`; the generator and fixture targets are present. `ctest --preset ci --no-tests=error -R '^ci[.]classify_policy$' --output-on-failure` exited 0 (1/1, 0.06 s total). `CMakePresets.json` parsed and all configure, build, test and workflow presets enumerated successfully.

Local routing evidence:

- `act pull_request --dryrun` for `.github/workflows/ci.yml`, with all hosted labels mapped to the local `ubuntu:latest` image, exited 0. Act simulated `changes`, `hygiene` and `CI required`; it prints but does not execute the classifier, so it cannot resolve `needs.changes.outputs.heavy` or route the dependent matrix jobs. This is a routing/syntax check only.
- `act pull_request --dryrun` for `.github/workflows/nightly.yml`, with all six runner labels mapped to `ubuntu:latest`, exited 0 and displayed the new preflight, separate uninstrumented generator, fixture generation and bounded CHD fuzz campaign. Neither dryrun executed workflow steps.
- No actual local job ran. The available `ghcr.io/catthehacker/ubuntu:act-24.04` image is x86-64 on the local ARM64 Docker engine and has no CMake installed. Local `clang-21`, `clang++-21` and `wasm-ld-21` are also absent, and there is no local WASI module. There is therefore no local CHD-enabled WASI size measurement.

The WASI bound remains exactly 608,540 bytes in `quality.json`; it was not loosened. Full hosted CI was not dispatched because Plan 12, Plan 14 and source review must finish first. The final exact-head WASI lane must measure the CHD-enabled module against that bound; if it exceeds the bound, optimize or make a concrete resource decision before release. Hosted Main/Release critical path and runner-minute review also remains part of that final exact-head run.

**Disposition:** complete. No cross-run artifact reuse; preserve the two required workflows, all eight parity lanes, all release assets, exact-release-SHA checks and archive validation. Plan 11 records local evidence and leaves the mandated hosted measurements for final verification.

## Acceptance Criteria

- Before any long hosted run, a cheap preflight verifies the exact source SHA, selected matrix/inventory, artifact
  names and digests, and required-check wiring.
- Measure runner-minute duplication and critical-path effects from attributable run/job data; do not infer a
  wall-clock improvement from concurrent runs.
- Either implement a small reuse path with gating tests that retains every required main/release check and
  validates the produced release archives, or record why reuse is unsafe and leave both workflows intact.
- Do not add a cache, dependency, matrix skip, or concurrency change without evidence that required coverage and
  release guarantees remain equivalent.
