---
phase: 07-faster-and-more-trustworthy-ci
plan: "01"
subsystem: ci
tags: [cmake, ctest, jq, privacy]
requires: []
provides:
  - CI policy tests for the triage transform, lesson ledger, quality comparator and changed-path classifier
  - Validated quality registry format and measurement interface for later CI jobs
affects: [weekly-triage, ci-routing, measured-quality]
tech-stack:
  added: []
  patterns: [fixture-to-production-script controls, CMake host policy tests]
key-files:
  created:
    - scripts/triage-report.sh
    - tests/cmake/ci_route_controls.cmake
    - tests/cmake/lessons_unique.cmake
    - tests/cmake/quality.cmake
    - tests/cmake/quality_controls.cmake
    - tests/cmake/triage_controls.cmake
  modified:
    - tests/hygiene/tests.cmake
    - tests/policy/inventory.txt
    - tests/policy/tests.cmake
key-decisions:
  - Redact path text through its next quote or boundary to protect path components containing spaces.
  - Compare decimal digit strings exactly so rounding cannot admit a value just above a large integer bound.
requirements-completed: []
coverage:
  - id: D1
    description: The production triage transform emits only bounded, sanitized metadata or fails without output.
    requirement: TUNE-01
    verification:
      - kind: integration
        ref: ci.triage in the native CI preset
        status: pass
    human_judgment: false
  - id: D2
    description: The production classifier retains its planning-only fast path and gates source, release inputs and unknown change lists.
    requirement: TUNE-02
    verification:
      - kind: integration
        ref: ci.classify_policy in the native CI preset and ci.classify in the hygiene preset
        status: pass
    human_judgment: false
  - id: D3
    description: The lesson ledger passes today and rejects repeated primary IDs or equivalent normalized prose.
    requirement: TUNE-05
    verification:
      - kind: integration
        ref: policy.lessons_unique and policy.lessons_unique.self_test in the native CI preset
        status: pass
    human_judgment: false
  - id: D4
    description: The quality comparator validates all five metric leaves and enforces their inclusive bounds against numbers and Wasm file size.
    requirement: TUNE-05
    verification:
      - kind: integration
        ref: policy.quality_metrics in the native CI preset
        status: pass
    human_judgment: false
  - id: D5
    description: Hygiene checks enforce the approved repository-check and RetroArch test line caps.
    requirement: TUNE-05
    verification:
      - kind: integration
        ref: policy.gate_budget and policy.e2e_budget with their negative controls in the hygiene preset
        status: pass
    human_judgment: false
actuals:
  tokens: 6432
  tasks: 3
  commits: 6
commits: 6
plan_head_before: d61a92bd38589fc12ba2f19d7797f616f8916686
plan_head_after: c110aba6e280ae6e99c99e7fd4130cec2c3ae8fc
duration: 11min
completed: 2026-11-18
status: complete
---

# Phase 07 Plan 01 — CI policy controls

The native CI preset runs production-path tests for sanitized triage, lesson uniqueness, exact inclusive quality bounds and changed-path routing. At completion the outer line checks measured **3,133 of 3,200** repository-check lines and **300 of 300** RetroArch test lines. The cap then left **67 lines**. The later 3,300-line decision gives Plan 03 ownership of the cap update before the remaining implementation. [Dated budget decision](07-CONTEXT.md#repository-check-cap-adjustment-on-2026-11-18)

## Verification

| Command | Exit | Result |
| --- | ---: | --- |
| `cmake --preset ci` | 0 | The byte-sorted directory inventory matched configured registrations. |
| `ctest --preset ci --no-tests=error -N -R '^(ci[.](triage\|classify_policy)\|policy[.](lessons_unique([.]self_test)?\|quality_metrics))$'` | 0 | All five new names were present. CTest JSON confirmed zero skip properties on these cases. |
| `ctest --preset ci --no-tests=error -R '^(ci[.](triage\|classify_policy)\|policy[.](lessons_unique([.]self_test)?\|quality_metrics\|no_skip([.]self_test)?))$' --output-on-failure` | 0 | All new policy cases and both existing no-skip controls passed. |
| `cmake --preset hygiene` | 0 | The hygiene inventory configured. |
| `ctest --preset hygiene --no-tests=error -R '^policy[.](gate_budget\|e2e_budget)$' -V` | 0 | Both measured totals fit their cap. |
| `ctest --preset hygiene --no-tests=error -R '^(ci[.]classify\|policy[.](gate_budget\|e2e_budget)[.]self_test)$' --output-on-failure` | 0 | The existing workflow route test and both negative budget controls passed. |
| `sh -n scripts/triage-report.sh` and `git diff --check` | 0 | POSIX shell syntax and patch whitespace passed. |

All checks above ran locally. Plan 05 supplies the live API collector and hosted triage run. Plans 02 and 06 supply CI measurements and updated workflow routes. Plans 03, 07 and 08 supply measured registry contents and live producers. The overall TUNE-01, TUNE-02 and TUNE-05 requirements remain pending that evidence.

## Task commits

| Task | Test first | Implementation | Observed initial failure |
| --- | --- | --- | --- |
| Triage artifact | `8f56295` | `7bf7cdc` | A valid empty JSON object failed the fixed-schema assertion. |
| Lesson uniqueness and budget | `0898c0f` | `2130f46` | The repeated primary ID fixture returned exit zero. |
| Quality bounds and routing | `159b04e` | `c110aba` | A below-floor measurement returned exit zero. |

Each initial CTest assertion exited 8 and produced JUnit evidence accepted by OpenGSD's RED classifier. The three implementation checks exited 0. The classifier test exercised the existing production script successfully when first registered.

## Implementation notes

The triage transform expects an issue array and an object keyed by `ci`, `release` and `nightly` containing their respective workflow API responses. The comparator validates the registry when passed only `-DQUALITY`. It accepts `-DARTIFACT` only for `size.wasm` and uses the file's byte count. The lesson check derives identities from primary entries in the imported and local sections and excludes source and status fields from the prose comparison.

Runtime was Codex with OpenGSD CLI 1.20.0 and the inherited GPT-6 session model. The deployed model ID was unavailable.

## Self-Check: PASSED

All nine code and inventory files exist, all six task commits are reachable, and OpenGSD accepted all five coverage entries.
