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

## Acceptance Criteria

- Before any long hosted run, a cheap preflight verifies the exact source SHA, selected matrix/inventory, artifact
  names and digests, and required-check wiring.
- Measure runner-minute duplication and critical-path effects from attributable run/job data; do not infer a
  wall-clock improvement from concurrent runs.
- Either implement a small reuse path with gating tests that retains every required main/release check and
  validates the produced release archives, or record why reuse is unsafe and leave both workflows intact.
- Do not add a cache, dependency, matrix skip, or concurrency change without evidence that required coverage and
  release guarantees remain equivalent.
