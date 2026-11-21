---
title: "Route Nightly pull-request jobs by changed work"
status: pending
priority: P2
resolves_phase: 10
source: "Phase 9 CI efficiency audit, run 44825237757"
created: 2026-11-20
theme: ci
---

## Goal

Avoid spending runner time on Nightly workloads that cannot exercise a pull request's changes, while keeping the full scheduled and manually dispatched Nightly suite.

## Context

The Phase 9 Nightly pull-request run took about 70 seconds wall time and 266 raw runner-seconds (4.43 runner-minutes). Four benchmark jobs plus `vectors-full` accounted for 234 seconds (3.90 raw runner-minutes); none exercised CHD. The fuzz job was relevant to CHD, though its generator build failed before the campaigns. This Phase 9 pull request also changed shared CMake and workflow routing, so retaining the full job set was the conservative choice. The measurement is raw runner time, not billing-weighted cost.

Bind this to the Phase 10 plan that first changes Nightly job routing. Keep PR-only job selection conservative: build, workflow, or unclassified changes run every Nightly job. Scheduled and `workflow_dispatch` runs always keep complete coverage.

## Acceptance Criteria

- The PR classifier can skip only the benchmark or full-vector jobs whose inputs are unaffected; source, parser, fuzz, and test-harness changes still run the relevant fuzz campaigns.
- Any unknown path and any workflow/build-routing change selects every Nightly job.
- Scheduled and manually dispatched runs execute the complete existing workload.
- A fast self-test covers representative PBP-only, fuzz-relevant, build/workflow, and unknown-path changes; the existing campaign bounds and test cases remain unchanged.
- Act dry-run and one hosted changed-head run confirm routing; record the measured critical path and raw job time before and after when the hosted result permits.

## Phase 10 binding

Plan 10-11 routes only a complete exact PBP path set to fuzz, and skips Nightly only for complete docs/planning-only
lists. Mixed, renamed (including both rename names in the allowlist), unknown, incomplete, API-failure and failed
classifier-job cases run the full workload. The API requests up to 100 files per page; route controls cover a
non-PBP fuzz input and each former Nightly path. Keep this todo pending until the exact-head hosted run confirms
the new read-only GitHub API route and its measured cost.
