---
title: "Triage the one-off install.consumer.dll_crt failure on windows-x64"
status: pending
priority: P2
source: "phase 01, plan 01-15"
created: 2026-10-20
theme: install
---

## Goal

Find why `install.consumer.dll_crt` once printed nothing on the hosted windows-x64 leg, and make the test report
the consumer's exit status so the next failure says why.

## Context

- Run https://github.com/szTheory/schwaystation/actions/runs/39458992541 (head d8a0ab4, a workflow-comment-only
  change): `consumer.exe` built against the DLL C runtime ran and printed no output, so its `PASS_REGULAR_EXPRESSION`
  missed; every other test passed. The same test passed on the same runner image in runs 39455724329 and
  39459524424, so it failed once in three hosted samples.
- An empty output with no message fits a process that never reached `main` (a missing DLL exits 0xC0000135
  silently) or was blocked while the fresh executable was scanned.
- A failing harness assertion is a test bug to triage, not a release gate (AGENTS rule 8).
- Bound to phase 2: the first plan that edits `tests/install/tests.cmake`, or the next hosted occurrence, whichever
  comes first.

## Acceptance Criteria

- The consumer test prints the program's exit status when its expected line is missing.
- The cause is recorded, or the test passes in the next 10 hosted windows-x64 runs with no change but the report.
