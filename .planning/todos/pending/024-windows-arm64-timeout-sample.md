---
title: "Include the Windows ARM64 timeout sample in the phase-1 CI tail"
status: pending
priority: P2
source: "phase 01 verification, run 40929433476"
created: 2026-10-29
theme: ci
bound_step: "phase/01-ci-tail, plan 1"
---

## Goal

When the already-planned phase-1 CI tail next edits the workflow timeouts,
include the Windows ARM64 measurement from verification. Keep this as a
tooling seed, with no new phase-1 acceptance requirement.

## Evidence

In run https://github.com/szTheory/schwaystation/actions/runs/40929433476,
attempt 1 job 123643713395 completed build, all tests, both inventory checks,
packaging, uploads and cleanup successfully. The check annotation nevertheless
reports that the job exceeded its three-minute maximum. The successful work
spanned roughly 3 minutes 16 seconds including runner setup and cleanup.

The agent reran that job and its downstream checks after inspecting every
step and the timeout annotation. Use the next successful job duration, along
with this observed overrun, when applying the existing measured-timeout rule;
do not loosen assertions or skip tests.

Read-only reproduction: `gh api repos/szTheory/schwaystation/check-runs/123643713395/annotations`
(exit 0; run URL above).
