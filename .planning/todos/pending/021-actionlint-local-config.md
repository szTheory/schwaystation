---
title: "Teach a local actionlint run the runner labels and queue key the workflows use"
status: pending
priority: P2
source: "phase 01, plan 01-15"
created: 2026-10-20
theme: ci
---

## Goal

A local `actionlint` run over `.github/workflows` exits 0, with the `ubuntu-26.04` and `ubuntu-26.04-arm` runner labels and the `concurrency.queue` key accepted.

## Context

- Noted by plan 01-14's executor: the local actionlint reports those labels and that key as unknown; hosted runs accept them.
- No gate calls actionlint today, so this is a maintainer convenience: an `.github/actionlint.yaml` with `self-hosted-runner.labels` and an ignore pattern for the queue key.
- Bound to the v1.0 milestone tune-up (SEED-007), unless a phase adds an actionlint gate first.

## Acceptance Criteria

- `actionlint` exits 0 locally on the workflows.
