---
phase: 07-faster-and-more-trustworthy-ci
plan: "03"
subsystem: infra
tags: [cmake, ctest, github-actions, ci, quality-metrics, dependency-pins]
requires:
  - phase: 07-faster-and-more-trustworthy-ci
    provides: [native policy tests, exact-head CI workload evidence]
provides:
  - Revised 3300-line repository-check ceiling with the separate 300-line RetroArch ceiling retained
  - Canonical measured quality baselines and a focused WASI size measurement prerequisite
  - Read-only evidence for repository settings, Actions, hosted runners, and active upstream pins
affects: [ci-quality-controls, ci-maintenance, dependency-pins]
actuals:
  tokens: 5199
  tasks: 3
  commits: 3
commits: 3
plan_head_before: c9c0dca48fda5e3f31756abfa072b4f2049d4c82
plan_head_after: 991dae7203a6dd22e7bbc62cc79690ec0360e552
tech-stack:
  added: []
  patterns: [producer-bound quality measurements, read-only settings and pin audits]
key-files:
  created:
    - .planning/phases/07-faster-and-more-trustworthy-ci/07-BASELINES.md
    - .planning/phases/07-faster-and-more-trustworthy-ci/07-UPGRADE-AUDIT.md
  modified:
    - tests/hygiene/tests.cmake
key-decisions:
  - "Keep the repository-check budget at 3300 and the independent RetroArch budget at 300 until the phase ship ratchet."
  - "Use only matching producer inputs for quality bounds and withhold the WASI size bound until its focused toolchain measurement succeeds."
  - "Keep current repository settings, action pins, and runner labels where evidence did not justify a change; record Wasmtime 48.0.9 as the next LTS patch candidate under the existing advisory policy."
requirements-completed: [TUNE-05, TUNE-06]
coverage:
  - id: D1
    description: The repository policy gate enforces the 3300-line ceiling, the RetroArch gate stays at 300, and both over-limit controls still reject as expected.
    requirement: TUNE-05
    verification:
      - kind: integration
        ref: ctest --preset hygiene --no-tests=error -R '^policy[.](gate_budget|e2e_budget)([.]self_test)?$' -V
        status: pass
    human_judgment: false
  - id: D2
    description: Current Cachegrind, serialized-state, and comparable CI path values have traceable producer identities; the missing WASI module size is explicitly withheld pending a focused measurement.
    requirement: TUNE-05
    verification:
      - kind: other
        ref: .planning/phases/07-faster-and-more-trustworthy-ci/07-BASELINES.md
        status: pass
    human_judgment: false
  - id: D3
    description: Current settings, Dependabot alerts/configuration, action pins, runner evidence, and active versus future package pins are documented with upstream references.
    requirement: TUNE-06
    verification:
      - kind: other
        ref: sh scripts/apply-repo-settings.sh --check; .planning/phases/07-faster-and-more-trustworthy-ci/07-UPGRADE-AUDIT.md
        status: pass
    human_judgment: false
duration: 44min
completed: 2026-11-18
status: complete
---

# Phase 07 Plan 03: Quality baselines and upgrade audit

**A 3,300-line repository gate, producer-matched quality baselines, and evidence-based settings, runner, and pin audit.**

## Performance

- **Duration:** 44 minutes wall elapsed from phase execution start through summary finalization; active time was not separately measured.
- **Started:** 2026-11-18T20:14:43Z.
- **Completed:** 2026-11-18T20:58:06Z.
- **Tasks:** 3.
- **Files modified by plan tasks:** 3, excluding GSD metadata.
- **Runtime/model:** Codex/OpenGSD CLI 1.20.0; executor `gpt-6-luna` at xhigh effort. The inherited session runtime was Codex.

## Accomplishments

- Raised only the repository-check budget registration from 3,200 to 3,300 lines. Its counted globs and independent 300-line RetroArch limit stayed unchanged. The measured counts were 3,133/3,300 and 300/300. The two budgets and their two negative controls passed (4/4 focused checks).
- Recorded the two current Cachegrind producers with three zero-spread samples each: test card 1,619,573 Ir/frame and `gpu_motion` 137,752,334 Ir/frame. The state API serialized 3,755,020 bytes including the 20-byte header. The matching CI path group measured 2,249 and 3,293 seconds across the exact `changes → openbios → be32 → hash-equality → ci-required` DAG.
- Left `size.wasm` without a bound because the local checkout lacks the clang-21/WASI toolchain and a generated `build/wasm32` tree. The baseline records the focused Ubuntu WASI target-build and `wc -c` commands needed before enforcement; no substitute artifact size was used.
- Confirmed the repository settings check has no drift, vulnerability alerts are enabled, and there are no open Dependabot alerts. There is no .github/dependabot.yml, so Actions version-update PRs are not configured. The audit links six active SHA-pinned Actions, current runner-image evidence, actual upstream pin consumers, and the Phase 9-only libchdr proposal.
- Recorded Wasmtime 48.0.9 as a candidate LTS pin update because the pinned 48.0.7 release falls within upstream advisory ranges and the repository pin policy calls for LTS security patches. The audit separately records that the AsyncWriteStream advisory says the Wasmtime CLI does not use that component and that the GC advisory conditions are not present in the current C-built module. The candidate update is left for the normal `ci:` pin-update step and must use the new upstream v48.0.9 digests.

## Checks and evidence

| Command | Exit | Result |
| --- | ---: | --- |
| `cmake --preset hygiene` | 0 | Configured the focused budget checks. |
| `ctest --preset hygiene --show-only=json-v1 \| jq -e '(.tests \| any(.name == "policy.gate_budget" and (.command \| index("-DLIMIT=3300") != null))) and (.tests \| any(.name == "policy.e2e_budget" and (.command \| index("-DLIMIT=300") != null)))'` | 0 | Confirmed configured repository and RetroArch limits. |
| `ctest --preset hygiene --no-tests=error -R '^policy[.](gate_budget\|e2e_budget)([.]self_test)?$' -V` | 0 | Four focused checks passed, including both expected-rejection controls. Measured 3,133/3,300 and 300/300. |
| Focused Cachegrind, state-size, and matching-run evidence | 0 | Producer identities, outputs, and limitations are recorded in [07-BASELINES.md](07-BASELINES.md). Cachegrind evidence is [run 44218206100](https://github.com/szTheory/schwaystation/actions/runs/44218206100); comparable CI timing evidence is [runs 44253375502](https://github.com/szTheory/schwaystation/actions/runs/44253375502) and [44307395327](https://github.com/szTheory/schwaystation/actions/runs/44307395327). |
| `sh scripts/apply-repo-settings.sh --check` | 0 | `no drift on szTheory/schwaystation`. |
| `gh api repos/szTheory/schwaystation/vulnerability-alerts --silent` | 0 | Security alerts are enabled. |
| `gh api 'repos/szTheory/schwaystation/dependabot/alerts?state=open&per_page=100' --jq length` | 0 | 0 open alerts. |
| GitHub REST release, commit, runner-image, and Actions job lookups | 0 | Current upstream refs and runner labels are linked in [07-UPGRADE-AUDIT.md](07-UPGRADE-AUDIT.md); successful full-CI runner evidence is [run 44307395327](https://github.com/szTheory/schwaystation/actions/runs/44307395327). |
| `gsd_run query roadmap.update-plan-progress 07` | 0 | ROADMAP now records 3/10 plans complete; phase remains in progress. |
| `gsd_run query state.advance-plan` and `gsd_run query state.record-session --stopped-at "Completed 07-03-PLAN.md" --resume-file "None"` | 0 | STATE advances to Plan 4 and records Plan 07-03 as the stopped-at point. |
| `gsd_run query state.update-progress` | 0 | The command reported no body `Progress:` line to update; the state frontmatter and ROADMAP plan counts record 3 completed plans. |
| `gsd_run query requirements.ready-ids .planning/phases/07-faster-and-more-trustworthy-ci/07-03-PLAN.md TUNE-05,TUNE-06` | 0 | `ready` was empty; the TUNE-05 and TUNE-06 phase-level checkboxes remain pending for their later end-to-end plans. |
| `git diff --check HEAD` for the baseline and git diff --no-index --check /dev/null .planning/phases/07-faster-and-more-trustworthy-ci/07-UPGRADE-AUDIT.md for the new audit | 0 | Both planning artifacts passed whitespace checks. |

No full suite or new hosted workflow was run for this plan. The reused hosted runs are linked evidence for existing measurements, not new executions.

## Task commits

1. **Task 1: apply the revised repository-check ceiling** — `b5eb401` (`feat(07-03): enforce 3300-line repository budget`)
2. **Task 2: measure canonical deterministic quality inputs** — `f040d75` (`docs(07-03): record measured quality baselines`)
3. **Task 3: audit workflow permissions, runner images, and tool pins** — `991dae7` (`docs(07-03): audit workflow settings runners and pins`)

## Decisions and issues

- Keep the plan's three requirements traced to TUNE-05 and TUNE-06, but leave the phase-level requirement checkboxes pending until their later enforcement and documentation work is complete.
- Keep the current action pins and hosted runner labels where the inspected releases and matching run gave no evidence for a change. The missing Dependabot version-update file is documented separately from enabled security alerts.
- The first scoped GSD staging attempt for the audit hit a Git index-lock sandbox restriction. The orchestrator repeated that exact single-file commit with repository-write access as `991dae7`; no unrelated file was staged.
- The WASI size gap is an explicit measurement prerequisite, not a fabricated baseline or a test failure.

## Deviations from Plan

None. Plan 07-03's checked-in changes are limited to the revised budget registration and the two requested evidence artifacts.

## Next phase readiness

The measured budget and evidence artifacts are ready for the remaining Phase 7 plans. The WASI size bound remains disabled until its focused producer check runs. The Wasmtime v48.0.9 candidate can be handled in the planned pin-update step. No owner action is needed.

---
*Phase: 07-faster-and-more-trustworthy-ci*
*Completed: 2026-11-18*

## Self-Check: PASSED

The summary, baseline, audit, and budget registration exist; all three task commits are reachable from the phase branch. The focused budget checks, producer evidence, read-only repository-settings check, and whitespace checks are recorded above.
