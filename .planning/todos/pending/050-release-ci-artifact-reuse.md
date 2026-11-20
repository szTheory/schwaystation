---
title: "Measure safe artifact reuse between main CI and Release CI"
status: pending
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

## Acceptance Criteria

- Before any long hosted run, a cheap preflight verifies the exact source SHA, selected matrix/inventory, artifact
  names and digests, and required-check wiring.
- Measure runner-minute duplication and critical-path effects from attributable run/job data; do not infer a
  wall-clock improvement from concurrent runs.
- Either implement a small reuse path with gating tests that retains every required main/release check and
  validates the produced release archives, or record why reuse is unsafe and leave both workflows intact.
- Do not add a cache, dependency, matrix skip, or concurrency change without evidence that required coverage and
  release guarantees remain equivalent.
