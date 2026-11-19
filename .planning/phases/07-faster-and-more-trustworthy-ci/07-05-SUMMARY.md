---
phase: 07-faster-and-more-trustworthy-ci
plan: "05"
subsystem: ci
tags: [github-actions, gh-api, jq, posix-shell]

requires:
  - phase: "07-01"
    provides:
      - registered ci.triage test
      - workflow pin and timeout checks
  - phase: "07-03"
    provides:
      - 3300-line repository-check ceiling
provides:
  - bounded read-only GitHub metadata collector for sanitized triage JSON
  - weekly and manual triage workflow with seven-day artifact retention
affects:
  - Phase 7 ship verification
  - CI maintenance

actuals:
  tokens: 4960
  tasks: 2
  commits: 3
commits: 3
plan_head_before: 07270f987065ae6b70832d9ac3f4faae23425763
plan_head_after: 6079c6c1ce3b108167bdddec2d9f5efd79a3875d

tech-stack:
  added: []
  patterns:
    - POSIX shell with gh api and jq for bounded, read-only collection
    - Private scratch files and atomic publication after validation
key-files:
  created:
    - .github/workflows/triage.yml
  modified:
    - scripts/triage-report.sh
    - tests/cmake/triage_controls.cmake
key-decisions:
  - Use one issues API page of up to 100 records and flag truncation at the cap.
  - Give the job only actions, contents, and issues read permissions.
patterns-established:
  - Collect only fixed workflow run endpoints with per_page=1.
  - Sanitize and bound untrusted issue titles and labels before publication.
requirements-completed: []
coverage:
  - id: D1
    description: Bounded collector emits sanitized open-issue metadata and the latest fixed workflow statuses.
    requirement: TUNE-01
    verification:
      - kind: integration
        ref: ctest --preset ci --no-tests=error -R '^ci[.]triage$' --output-on-failure
        status: pass
    human_judgment: false
  - id: D2
    description: Weekly/manual workflow configuration uses read-only scopes and specifies one seven-day triage.json upload.
    requirement: TUNE-01
    verification:
      - kind: integration
        ref: "cmake --preset hygiene && ctest --preset hygiene --no-tests=error -R '^(policy[.](action_pins|gate_budget)|hygiene[.]timeout_comments)$' --output-on-failure"
        status: pass
      - kind: other
        ref: >-
          yq -o=json '.' .github/workflows/triage.yml | jq -e '(.on.schedule | length) == 1 and .on.schedule[0].cron == "17 5 * * 6" and (.on | has("workflow_dispatch")) and .permissions == {} and (.jobs | keys == ["triage"]) and .jobs.triage.permissions == {actions:"read",contents:"read",issues:"read"} and ([.jobs.triage.steps[] | select((.uses // "") | startswith("actions/upload-artifact@"))] | length) == 1 and ([.jobs.triage.steps[] | select((.uses // "") | startswith("actions/upload-artifact@")) | .with.path] == ["triage.json"]) and ([.jobs.triage.steps[] | select((.uses // "") | startswith("actions/upload-artifact@")) | .with["retention-days"]] == [7]) and ([.jobs.triage.steps[] | select((.uses // "") | startswith("actions/upload-artifact@")) | .with["if-no-files-found"]] == ["error"]) and ([.jobs.triage.steps[] | select(.env.GH_TOKEN? != null) | .env.GH_TOKEN] == ["${{ github.token }}"])' >/dev/null
        status: pass
      - kind: other
        ref: "XDG_CACHE_HOME=/private/tmp/schway-act-cache act workflow_dispatch --dryrun -W .github/workflows/triage.yml -P ubuntu-26.04=catthehacker/ubuntu:act-latest --container-architecture linux/amd64"
        status: pass
    human_judgment: false

duration: 35min
completed: 2026-11-18
status: complete
---

# Phase 7 Plan 5: Weekly triage artifact summary

**A read-only `gh api` collector and weekly/manual workflow publish bounded, sanitized `triage.json` with seven-day retention.**

## Performance

- **Duration:** 35 min
- **Started:** 2026-11-18T21:26:51Z
- **Completed:** 2026-11-18T22:02:14Z
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments

- Added a POSIX collector that reads one bounded open-issues page and the latest row for each fixed CI, Release, and Nightly workflow.
- Sanitizes and bounds issue titles and labels, filters pull requests, emits null fields for workflows with no runs, and atomically publishes only after all requests and JSON validation succeed.
- Added a weekly Saturday schedule and manual dispatch job with default-deny permissions, only the planned read scopes, pinned actions, and one seven-day `triage.json` artifact.
- Kept repository-check code at 3,266 lines, below the 3,300-line ceiling.

## TDD RED/GREEN Evidence

- **RED:** `ci.triage` exited 8 before the collector existed. The sole target failed at the planned valid-API-artifact assertion. The fresh JUnit report was classified `RED_EVIDENCE_OK`; the evidence record is in the ignored `build/ci` directory.
- **GREEN:** The collector implementation passed the same focused `ci.triage` test. No refactor commit was needed.

## Checks

- `cmake --preset ci && ctest --preset ci --no-tests=error -R '^ci[.]triage$' --output-on-failure` — exit 0; 1/1 test passed.
- `cmake --preset hygiene && ctest --preset hygiene --no-tests=error -R '^(policy[.](action_pins|gate_budget)|hygiene[.]timeout_comments)$' --output-on-failure` — exit 0; 3/3 focused checks passed.
- git diff --check, sh -n scripts/triage-report.sh, yq eval '.' .github/workflows/triage.yml, and the 3,300-line budget check — exit 0.
- XDG_CACHE_HOME=/private/tmp/schway-act-cache act workflow_dispatch --dryrun -W .github/workflows/triage.yml -P ubuntu-26.04=catthehacker/ubuntu:act-latest --container-architecture linux/amd64 — exit 0; checkout, collector, and upload wiring parsed. This dry-run did not execute actions or prove an artifact upload.
- No hosted workflow was run. The plan reserves the first manual dispatch and artifact verification for ship after merge; record that run URL there.
- Plan 07-05 implementation is complete, but phase requirement TUNE-01 remains pending until ship confirms the hosted job uploads and exposes the artifact.

## Task Commits

1. **Task 1 RED controls** — `8eb616c` (`test(07-05): add failing triage collector controls`)
2. **Task 1 GREEN collector** — `889f9e5` (`feat(07-05): collect sanitized triage metadata`)
3. **Task 2 workflow** — `6079c6c` (`ci(07-05): add weekly triage artifact workflow`)

## Files Created/Modified

- `scripts/triage-report.sh` — bounded GH API collection, validation, sanitization, and atomic output.
- `tests/cmake/triage_controls.cmake` — stub GH endpoints, privacy checks, cap/empty cases, and failure controls.
- `.github/workflows/triage.yml` — weekly/manual read-only job and short-retention artifact upload.

## Decisions Made

- A single `per_page=100&page=1` issues request bounds API traffic; a full page sets `issues_truncated` because more open records may remain.
- The schedule runs Saturday at 05:17 UTC, away from the existing nightly schedule.
- The collector makes no API writes and does not include issue bodies, authors, logs, branches, or commit SHAs in the artifact.

## Deviations from Plan

### Auto-fixed Issues

1. **[Rule 1 - Bug] Allowed the repository separator in input validation.** The first implementation rejected a valid `owner/repository` value. Validation now permits exactly one slash while restricting both components. Fixed in `889f9e5`.
2. **[Rule 1 - Bug] Scoped jq workflow rows explicitly.** Parentheses now bind each fixed workflow response to its own sanitizer call. Fixed in `889f9e5`.
3. **[Rule 1 - Test bug] Corrected the empty-result controls.** The empty-issues fixture still had a latest CI run, so the assertion now expects that row and separately covers all workflows with no runs. Fixed in `8eb616c`.
4. **[Rule 1 - Test bug] Corrected jq label-bound assertions.** Explicit parentheses now check label count and label length independently. Fixed in `8eb616c`.

**Total deviations:** 4 automatic fixes (2 implementation bugs, 2 test assertion bugs).

## Issues Encountered

- The installed `act` initially requested an interactive default image and its default action cache path was outside writable roots. A scoped dry-run using `/private/tmp` for its cache and an explicit runner mapping completed successfully.
- Hosted upload evidence remains pending until the workflow is available after merge. The agent should dispatch it at ship and record the successful job, single artifact, and run URL.

## Known Stubs

None.

## User Setup Required

None. The workflow passes its repository token only to the collector through `GH_TOKEN`.

## Next Phase Readiness

The collector and workflow are ready for Phase 7 ship. At ship, dispatch the workflow, confirm the job succeeds and exposes only `triage.json` with seven-day retention, then record the run URL. No owner verification is needed; the workflow step is available after merge.

---
*Phase: 07-faster-and-more-trustworthy-ci*
*Completed: 2026-11-18*

## Self-Check: PASSED
