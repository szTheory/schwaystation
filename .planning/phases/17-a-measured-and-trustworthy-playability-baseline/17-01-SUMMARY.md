---
phase: 17-a-measured-and-trustworthy-playability-baseline
plan: "01"
subsystem: testing-and-documentation
tags:
  - cmake
  - inventory
  - installer
requires: []
provides:
  - Recorded current hygiene/policy test inventories and install evidence
  - Audited user-facing and maintainer claims against current sources
affects:
  - Phase 17 scoring and documentation refresh
actuals:
  tokens: 4185
  tasks: 2
  commits: 2
tech-stack:
  added: []
  patterns:
    - Existing deferred CMake registration check remains the authority for test names and labels
key-files:
  created:
    - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-SKELETON.md
    - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-DOC-AUDIT.md
  modified:
    - tests/hygiene/tests.cmake
key-decisions:
  - "Keep the existing 4,694 repository-check and 300-line RetroArch ceilings unchanged."
  - "Record the stale published quality figures and moved Phase 7 links for the later evidence-led documentation pass."
patterns-established:
  - "The phase reuses current CMake inventories, registration checks and README installer controls."
requirements-completed: []
coverage:
  - id: P17-01-A
    description: "Current CMake inventories and the public README install path are evidenced."
    requirement: TUNE-09
    verification:
      kind: integration
      ref: "17-SKELETON.md; install_line.cmake remote and flipped-byte invocations (exit 0); inventory preflight (exit 0)"
      status: pass
    human_judgment: false
  - id: P17-01-B
    description: "User and maintainer claims have source-backed dispositions and known drift is recorded."
    requirement: TUNE-11
    verification:
      kind: other
      ref: "17-DOC-AUDIT.md; one-off local-link scan over 25 Markdown pages (exit 0, four archived Phase 7 links identified)"
      status: pass
    human_judgment: false
duration: 13m
completed: 2026-11-27
status: complete
---

# Phase 17 Plan 01 Summary

**Recorded the existing test-registration skeleton, verified the released README installer, and mapped current documentation drift.**

## Performance

- **Duration:** About 13 minutes, including the installer and evidence audit.
- **Started:** 2026-11-28T01:10:00Z (approximate; the earliest timestamped task command was 01:13:09Z).
- **Completed:** 2026-11-28T01:23:07Z.
- **Tasks:** 2.
- **Files modified:** 4, including this summary.

## Accomplishments

- Recorded the exact 24-row hygiene and 12-row policy inventories and existing deferred CMake check. Names matched their registrations and both inventories are C-locale sorted.
- The public README installer installed the core and info file from `v0.16.0`; the flipped-byte control rejected the altered archive without installing it. The release workflow run is [45923155197](https://github.com/szTheory/schwaystation/actions/runs/45923155197).
- Identified stale performance figures and four broken archived Phase 7 links in `docs/testing.md`. Their smallest corrections are deferred to the evidence and documentation plans.
- Confirmed the current 4,694-line repository-check and 300-line RetroArch ceilings without changing either.

## Task Commits

1. **Task 1: Trace inventories and README installer** - `21a78be` (`docs(17-01): record CMake and installer baseline`).
2. **Task 2: Audit user guidance and public claims** - `c66fe01` (`docs(17-01): audit user and maintainer claims`).

## Verification

- README installer against `releases/latest/download/`: exit 0; both expected files were installed under isolated HOME.
- Flipped-byte installer control: CMake exit 0; the README shell command rejected the tampered archive with exit 1 and installed no core.
- Inventory name/sort preflight: exit 0. Existing CMake deferred registration check remains responsible for name and label equality at configure time.
- Repository-check budget: 4,694 lines, limit 4,694, exit 0. RetroArch budget: 300 lines, limit 300, exit 0.
- Markdown local-target scan: exit 0; four stale references were recorded for follow-up. `git diff --check origin/main` passed.

The first RetroArch budget invocation let zsh expand the wildcard before CMake; quoting the CMake argument fixed the command, and the intended budget check passed. No project change was needed.

## Requirements and remaining evidence

No TUNE requirement is complete from this audit alone. TUNE-09 still needs the scored seven-dimension review and verified improvement. TUNE-11 still needs current settings/pin reconciliation and any evidence-selected documentation refresh.

## Cleanup

The task-owned temporary directory, downloaded archive, installer outputs and CMake scratch were removed after the bounded checks completed; no archive bytes or logs were committed.

---
*Phase: 17-a-measured-and-trustworthy-playability-baseline*
*Plan: 01*
*Completed: 2026-11-27*
