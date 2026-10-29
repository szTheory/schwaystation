---
phase: 01-a-test-frame-in-retroarch
plan: 13
subsystem: ci, release, docs
tags: [github-actions, release-please, attestations, rehearsal, nightly, github-app]
status: complete

requires:
  - phase: 01-02
    provides: "scripts/publish.sh with the SIGNER_WORKFLOW rehearsal signer, the removed one-off App script"
  - phase: 01-11
    provides: "the 18 version-free archives, install_line.cmake and the README install line"
  - phase: 01-12
    provides: "ci.yml with changes (release_inputs), the six build legs' archives-* artifacts and CI required"
provides:
  - "ci.yml jobs rehearsal and cleanup, inside CI required, with the workflow env REHEARSAL_TAG"
  - "release-please-config.json (simple, initial 0.1.0) and .release-please-manifest.json at 0.0.0"
  - ".github/workflows/release.yml: release-please, release-ci, publish, install-line-latest"
  - ".github/workflows/nightly.yml: bench (3 runners), bench-wasm32, report with the force_failure input"
  - "docs/maintaining.md"
affects: [01-14, 01-15]

actuals:
  tokens: 8600
  tasks: 3
  commits: 3
plan_head_before: 3b749886659d5a751161166069bfe37db1c8cc0e
plan_head_after: 98be8f89b5eb62defb847b85d28ea83e0ee8fc88

tech-stack:
  added: [googleapis/release-please-action v5.0.0, actions/create-github-app-token v3.3.0, actions/attest v4.2.2]
  patterns:
    - "One tag per rehearsal attempt (rehearsal-<run_id>-<run_attempt>), deleted by an always-run cleanup job"
    - "Drafts addressed by release ID, never by tag lookup"

key-files:
  created:
    - .github/workflows/release.yml
    - .github/workflows/nightly.yml
    - release-please-config.json
    - .release-please-manifest.json
    - docs/maintaining.md
  modified:
    - .github/workflows/ci.yml

key-decisions:
  - "01-13: every new timeout keeps the seed bound marked provisional with no run ID; the nightly bench comments lost the seed's run IDs, as in ci.yml"
  - "01-13: the one-off actionlint command also ignores 'unexpected key \"queue\" for \"concurrency\" section': actionlint 1.7.12 predates concurrency.queue, which the seed's release.yml ran with successfully (run 39346780552)"
  - "01-13: the rehearsal's not-configured message and its job comment point to docs/maintaining.md; the removed App script is cited there as git show 61a5bb7^:scripts/setup-release-app.sh"
  - "01-13: nightly keeps the seed's 300,000 frames and its wasm32 bench under wasmtime, built with the perf preset (build/perf/schwaystation-run)"

requirements-completed: [FRAME-06, FRAME-07, FRAME-09]

coverage:
  - id: D1
    description: "ci.yml has rehearsal and cleanup jobs with the per-attempt tag, cleanup runs with always(), and CI required needs both, skipped only when no release input changed, outside pull requests or in a nested call"
    requirement: FRAME-06
    verification:
      - kind: other
        ref: "actionlint -shellcheck= -pyflakes= -ignore 'label \"ubuntu-26\\.04(-arm)?\" is unknown' .github/workflows/ci.yml exit 0; yq -e '.jobs.rehearsal and .jobs.cleanup' exit 0; /usr/bin/grep -q 'rehearsal-${{ github.run_id }}-${{ github.run_attempt }}' exit 0; yq '.jobs.cleanup.if' contains always(); yq '.jobs[\"ci-required\"].needs' lists rehearsal"
        status: pass
    human_judgment: false
  - id: D2
    description: "The rehearsal runs every publish step on a probe pull request and its cleanup leaves no rehearsal release or tag"
    requirement: FRAME-06
    verification:
      - kind: e2e
        ref: "hosted CI jobs rehearsal and cleanup on a probe pull request, run URL supplied by plan 01-14"
        status: unknown
    human_judgment: false
  - id: D3
    description: "release-please is simple on version.txt with initial-version 0.1.0 and the manifest at 0.0.0, so the first release is 0.1.0"
    requirement: FRAME-06
    verification:
      - kind: other
        ref: "test \"$(jq -r '.[\".\"]' .release-please-manifest.json)\" = 0.0.0 exit 0; test \"$(jq -r '.packages[\".\"][\"release-type\"]' release-please-config.json)\" = simple exit 0"
        status: pass
    human_judgment: false
  - id: D4
    description: "release.yml runs release-please with the App token, auto-merges the release pull request, calls ci.yml on release-please's sha, publishes by draft ID, attests, verifies, marks latest and runs the install line against releases/latest/download/"
    requirement: FRAME-07
    verification:
      - kind: other
        ref: "actionlint -shellcheck= -pyflakes= -ignore 'label \"ubuntu-26\\.04(-arm)?\" is unknown' -ignore 'unexpected key \"queue\" for \"concurrency\" section' .github/workflows/release.yml exit 0; grep -c 'releases/latest/download/' prints 1; grep -c setup-release-app prints 0"
        status: pass
    human_judgment: false
  - id: D5
    description: "nightly.yml benches 300,000 frames on the perf preset on ubuntu x64, ubuntu arm64 and macos-15, plus wasm32 under wasmtime, takes force_failure by workflow_dispatch and keeps one rolling nightly issue"
    requirement: FRAME-09
    verification:
      - kind: other
        ref: "actionlint (same filters) .github/workflows/nightly.yml exit 0; grep -c 'preset bench' prints 0 for nightly.yml and ci.yml"
        status: pass
    human_judgment: false
  - id: D6
    description: "Action pins and timeout comments hold across the three workflows"
    requirement: FRAME-06
    verification:
      - kind: integration
        ref: "tests/hygiene/tests.cmake#policy.action_pins"
        status: pass
      - kind: integration
        ref: "tests/hygiene/tests.cmake#hygiene.timeout_comments"
        status: pass
    human_judgment: false
  - id: D7
    description: "docs/maintaining.md covers the release App and its history pointer, settings and the ruleset, the release and rehearsal flow, withdrawal, the nightly run and the post-release ci: pull requests, under its byte budget and with no planning ID"
    requirement: FRAME-06
    verification:
      - kind: integration
        ref: "tests/policy/tests.cmake#docs.budgets"
        status: pass
      - kind: other
        ref: "grep -c -i rehearsal docs/maintaining.md prints 14; grep -c -i withdraw prints 4; grep -E -c 'D-[0-9]|FRAME-|XFER-|DEC\\.' prints 0"
        status: pass
    human_judgment: false

duration: 5min
completed: 2026-10-20
---

# Phase 1 Plan 13: Release Path, Rehearsal and Nightly Summary

Every release step is now in place. A same-repository pull request that touches a release input rehearses all the publish steps inside `CI required` against its own `rehearsal-<run_id>-<run_attempt>` prerelease, and a cleanup job deletes that prerelease whatever happened. On a merge to `main`, release-please makes 0.1.0 from a manifest at `0.0.0`, `release.yml` publishes the draft by ID with attestations and the 19 assets, and the README install line runs against `releases/latest/download/`. `nightly.yml` benches 300,000 frames on four targets and keeps one rolling issue. `docs/maintaining.md` explains how to run all of it.

## Performance

- **Duration:** about 5 min wall time
- **Started:** 2026-10-20T16:06:19Z
- **Completed:** 2026-10-20T16:11:41Z (before the SUMMARY)
- **Tasks:** 3
- **Files modified:** 6 (5 created, 1 modified)

## Accomplishments

- `ci.yml` gains `rehearsal`, which runs `prepare`, `rehearsal-create`, `upload`, `actions/attest` with `subject-checksums`, `attested`, a never-latest prerelease publish and `verify`. It signs as `ci.yml` from `refs/pull/N/merge`, and only it asks for `id-token` and `attestations: write`. `ci.yml` also gains `cleanup`, which runs `if: always()` once the rehearsal has started. `CI required` now needs both. It allows them to be skipped only when no release input changed, outside a pull request, or in `release.yml`'s nested call.
- `release-please-config.json` is the seed's file unchanged. `.release-please-manifest.json` is `0.0.0`, not the seed's `0.1.0`.
- `release.yml` is ported with the App names renamed. Its concurrency group queues runs and never cancels them. `release-ci` is called with release-please's `sha` and the union of `ci.yml`'s permissions. `publish` refuses archives from another commit (`prepare` with `EXPECTED_SHA`), and its install line points at `szTheory/schwaystation`.
- `nightly.yml` is ported onto the `perf` preset. It has the three-runner bench and the wasm32 bench under wasmtime. `force_failure` comes after the upload, and the `report` job decides the rolling issue from `needs.*.result`.
- `docs/maintaining.md` is 12,311 bytes. It covers the release, the rehearsal, re-runs, phase close, withdrawal, the App, rotation, the removed script (`git show 61a5bb7^:scripts/setup-release-app.sh`), settings and the ruleset, the nightly run, the two `ci:` pull requests after the first release (arm the Ir gate, promote wasm32), and pins.

## Task Commits

1. **Task 1: rehearsal and cleanup jobs wired into CI required** - `6226baa` (ci)
2. **Task 2: release-please files, release.yml and nightly.yml** - `faeae06` (ci)
3. **Task 3: docs/maintaining.md** - `98be8f8` (docs)

## Evidence (commands from the repository root, exit status)

- Pins (rule 12): `gh api repos/<o>/<r>/commits/<sha>` gave exit 0 for `actions/create-github-app-token@64ab729a…` (v3.3.0), `actions/attest@1e69f48a…` (v4.2.2) and `googleapis/release-please-action@45996ed1…` (v5.0.0). Each tag resolves to the same SHA.
- `actionlint -shellcheck= -pyflakes= -ignore 'label "ubuntu-26\.04(-arm)?" is unknown' -ignore 'unexpected key "queue" for "concurrency" section' .github/workflows/ci.yml .github/workflows/release.yml .github/workflows/nightly.yml`: exit 0. See Deviations for why the plain command exits 1.
- `yq -e '.jobs.rehearsal and .jobs.cleanup' .github/workflows/ci.yml`: exit 0. The per-attempt tag grep: exit 0 with `/usr/bin/grep` and with bash's `grep`. In this zsh, `grep` is ugrep, which reads `{` as an interval and exits 1. `grep -qF` exits 0 there too.
- `yq '.jobs.cleanup.if'` printed `always() && needs.rehearsal.result != 'skipped'`. `ci-required.needs` lists `rehearsal` and `cleanup`.
- The manifest and release-type `test`s: exit 0 each. `grep -c 'preset bench'` on nightly.yml and ci.yml printed 0 and 0. `grep -c setup-release-app release.yml` printed 0. `grep -c 'releases/latest/download/' release.yml` printed 1.
- `cmake --workflow --preset hygiene`: exit 0 after each task and at the end (21 tests).
- `test "$(wc -c < docs/maintaining.md)" -lt 61440`: exit 0 (12,311 bytes).
- `cmake --workflow --preset ci`: exit 0 (77 tests, then 2 release tests). Both JUnit checks (`-DEXCLUDE=release`, `-DLABELS=release`): exit 0.
- Cruft grep (the plan's two `git grep --untracked` commands, run under bash): a=1, b=1, exit 0.
- The planning-ID scan `git grep --untracked -nE -e 'D-[0-9]{2}' -e 'FRAME-[0-9]' -e 'XFER-[0-9]' -e 'DEC\.[0-9]' -- .github release-please-config.json .release-please-manifest.json docs/maintaining.md`: exit 1, so no matches. The baseline at HEAD before this plan was also exit 1.
- Not shown here: the hosted rehearsal run, which plan 01-14 runs on a probe pull request (D2). The real publish on merge is confirmed at ship (AGENTS rules 6 and 9) and is not an acceptance step of this phase.

## Decisions Made

See `key-decisions` in the frontmatter.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] actionlint 1.7.12 rejects `concurrency.queue`**
- **Found during:** Task 2
- **Issue:** actionlint reports `unexpected key "queue" for "concurrency" section` on `release.yml`. The plan requires `queue: max`, and the seed's `release.yml` at the pinned commit ran with it successfully (`gh api` lists run 39346780552, success). So this is a gap in the linter, not a fault in the workflow.
- **Fix:** I ran the one-off lint with `-ignore 'unexpected key "queue" for "concurrency" section'` as well as the ubuntu-26.04 label filter that 01-12 used. No config file was added.
- **Files modified:** none
- **Commit:** n/a

**2. [Rule 1 - Consistency] Seed run IDs removed from the nightly timeouts**
- **Found during:** Task 2
- **Issue:** The seed's two bench timeouts cite cogmaster run IDs, and this repository's `ci.yml` keeps no foreign run IDs (01-12).
- **Fix:** I kept the bounds and changed the comments to `# provisional` until plan 01-15 measures them.
- **Files modified:** .github/workflows/nightly.yml
- **Commit:** faeae06

### Plan interpretation

- The seed's rehearsal comment said `docs/testing.md describes the rehearsal`. Here it points to `docs/maintaining.md`, because `docs/testing.md` is outside this plan's files. The CI jobs table and the `CI required` row in `docs/testing.md` do not list `rehearsal`/`cleanup` yet. That one-row update should go to the next plan that edits `docs/testing.md` (01-14 or 01-15).

**Total deviations:** 2 auto-fixed (1 blocking tooling, 1 consistency). **Impact:** none on behaviour.

## Issues Encountered

None. The known `dev`-preset `abi.undefined` failure on macOS Debug is still in deferred-items.md. This plan did not touch it.

## Known Stubs

None.

## User Setup Required

None. The App variable and secret are already on the repository.

## Next Phase Readiness

Plan 01-14 opens a probe pull request that changes a release input. It records the run URL for the rehearsal, the cleanup and the jobs pending from 01-12, and checks that no `rehearsal-*` release or tag remains. FRAME-06, 07 and 09 stay open in REQUIREMENTS.md while plans 01-14 and 01-15 still declare them.

## Self-Check: PASSED

- FOUND: .github/workflows/release.yml, .github/workflows/nightly.yml, release-please-config.json, .release-please-manifest.json, docs/maintaining.md
- FOUND: 6226baa, faeae06, 98be8f8 (ancestors of HEAD)
