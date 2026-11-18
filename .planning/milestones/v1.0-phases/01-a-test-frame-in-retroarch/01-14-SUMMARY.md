---
phase: 01-a-test-frame-in-retroarch
plan: 14
subsystem: ci, release, e2e
tags: [github-actions, hosted-runners, retroarch, rehearsal, nightly, durations]
status: complete

requires:
  - phase: 01-12
    provides: "ci.yml with every lane, the RetroArch compare and the perf report"
  - phase: 01-13
    provides: "the rehearsal and cleanup jobs, nightly.yml with force_failure"
provides:
  - "Draft probe pull request #2 (ci: probe the phase 1 pipeline), left open for plan 01-15 to close unmerged"
  - "A green CI run on head e669f21 with every job success: run 39455724329"
  - "A forced-failure nightly that opened issue #3 and a good nightly that closed it"
  - "The measured duration of every CI job and nightly job, for plan 01-15's timeouts"
affects: [01-15]

actuals:
  tokens: 4000
  tasks: 3
  commits: 0
plan_head_before: e669f21032f7b46db6e41295e8ffca7a3caa2054
plan_head_after: e669f21032f7b46db6e41295e8ffca7a3caa2054

tech-stack:
  added: []
  patterns:
    - "The phase branch itself is the probe pull request's head; nothing is pushed to main"

key-files:
  created: []
  modified: []

key-decisions:
  - "01-14: the first hosted CI run was green on every job, so no hosted fix was made and ci.yml, abi.cmake and CMakePresets.json are unchanged"
  - "01-14: RetroArch's screenshot matched the runner's frame 5 with no offset, so no runner frame number is pinned in ci.yml"
  - "01-14: perf reported verdict NO_BASE (Ir/frame 1620415 at head) because main's merge base has no perf preset; this is the defined first-PR result"

requirements-completed: [FRAME-01, FRAME-02, FRAME-03, FRAME-04, FRAME-05, FRAME-06, FRAME-07, FRAME-08, FRAME-09]

coverage:
  - id: D1
    description: "Draft probe pull request from the phase branch with a Conventional Commit title, and a CI run on its head commit"
    requirement: FRAME-01
    verification:
      - kind: e2e
        ref: "https://github.com/szTheory/schwaystation/pull/2; the plan's Task 1 automated verify exit 0"
        status: pass
    human_judgment: false
  - id: D2
    description: "CI run on head e669f21 concludes success with CI required success; all six build legs, asan, both nofp legs and be32 pass, each with its check_junit steps"
    requirement: FRAME-01
    verification:
      - kind: e2e
        ref: "https://github.com/szTheory/schwaystation/actions/runs/39455724329"
        status: pass
    human_judgment: false
  - id: D3
    description: "hash-equality reports 7 identical files and wasm32 reports its hashes equal Linux x64's (report only)"
    requirement: FRAME-02
    verification:
      - kind: e2e
        ref: "https://github.com/szTheory/schwaystation/actions/runs/39455724329 jobs hash-equality and wasm32"
        status: pass
    human_judgment: false
  - id: D4
    description: "Determinism and the bounded core hold on hosted runners: the build legs' tests and the nofp and be32 lanes pass"
    requirement: FRAME-04
    verification:
      - kind: e2e
        ref: "https://github.com/szTheory/schwaystation/actions/runs/39455724329 jobs nofp and be32"
        status: pass
    human_judgment: false
  - id: D5
    description: "RetroArch 1.22.2 runs the core for 5 frames and its screenshot equals the runner's frame 5 at 320x240; the flipped-byte control fails at 7,3"
    requirement: FRAME-07
    verification:
      - kind: e2e
        ref: "https://github.com/szTheory/schwaystation/actions/runs/39455724329 job retroarch"
        status: pass
    human_judgment: false
  - id: D6
    description: "The rehearsal prepares 18 archives and SHA256SUMS, attests them, publishes rehearsal-39455724329-1 with 19 assets that verify, and cleanup deletes the prerelease and tag; release.archive_contents passes on every build leg"
    requirement: FRAME-06
    verification:
      - kind: e2e
        ref: "https://github.com/szTheory/schwaystation/actions/runs/39455724329 jobs rehearsal and cleanup; gh release list printed []; git ls-remote --tags origin matched no rehearsal tag"
        status: pass
    human_judgment: false
  - id: D7
    description: "perf prints the test card's Ir per frame in report mode"
    requirement: FRAME-09
    verification:
      - kind: e2e
        ref: "https://github.com/szTheory/schwaystation/actions/runs/39455724329 job perf"
        status: pass
    human_judgment: false
  - id: D8
    description: "A forced-failure nightly dispatch opens the rolling nightly issue and the next good dispatch closes it"
    requirement: FRAME-09
    verification:
      - kind: e2e
        ref: "https://github.com/szTheory/schwaystation/actions/runs/39457445780 and https://github.com/szTheory/schwaystation/actions/runs/39457820699; https://github.com/szTheory/schwaystation/issues/3 closed"
        status: pass
    human_judgment: false
  - id: D9
    description: "The tree holds no cruft and no planning ID in code, tests, workflows or config"
    requirement: FRAME-05
    verification:
      - kind: other
        ref: "the plan's cruft git grep pair a=1 b=1 exit 0; the planning-ID git grep then test $? -eq 1 exit 0"
        status: pass
    human_judgment: false
  - id: D10
    description: "The libretro core and runner built and packaged on all six platforms pass their tests"
    requirement: FRAME-03
    verification:
      - kind: e2e
        ref: "https://github.com/szTheory/schwaystation/actions/runs/39455724329 build jobs"
        status: pass
    human_judgment: false
  - id: D11
    description: "The README install line installs the macOS arm64 leg's own zip and refuses a flipped copy"
    requirement: FRAME-08
    verification:
      - kind: e2e
        ref: "https://github.com/szTheory/schwaystation/actions/runs/39455724329 job build (macos-arm64) step README install line"
        status: pass
    human_judgment: false

duration: 17min
completed: 2026-10-20
---

# Phase 1 Plan 14: Hosted Pipeline Probe Summary

The first hosted run on draft probe pull request #2 passed every job: six build legs, asan, nofp on two runners, be32, seven-file hash equality, report-only wasm32, the RetroArch 1.22.2 pixel compare, perf, the full release rehearsal and its cleanup, and `CI required`. No hosted-only fix was needed. A forced-failure nightly opened the rolling issue and the next good nightly closed it.

## Performance

- **Duration:** 17 min wall time (2026-10-20T16:14:11Z to 16:31:13Z)
- **Tasks:** 3
- **Files modified:** 0 source files (this SUMMARY only)

## Accomplishments

- Pushed `phase/01-a-test-frame-in-retroarch` and opened draft PR https://github.com/szTheory/schwaystation/pull/2, titled `ci: probe the phase 1 pipeline`. `scripts/check-title.sh` exited 0 on the title and body before the PR was opened. The PR stays open, and plan 01-15 closes it unmerged.
- CI run https://github.com/szTheory/schwaystation/actions/runs/39455724329 (head `e669f21`): conclusion success, and `CI required` success. It was created at 16:14:48Z and finished at 16:23:29Z, about 8m41s.
- Nightly forced failure: https://github.com/szTheory/schwaystation/actions/runs/39457445780 concluded failure. Only bench-wasm32's `Forced failure` step failed, which came after its upload. The report job opened https://github.com/szTheory/schwaystation/issues/3.
- Good nightly: https://github.com/szTheory/schwaystation/actions/runs/39457820699 concluded success, and the report job closed issue #3.
- Opening the PR also started a pull_request nightly run, https://github.com/szTheory/schwaystation/actions/runs/39455724030. Its four bench jobs passed and the report job was skipped, as designed for pull requests.

## CI job durations (run 39455724329, seconds, completedAt minus startedAt)

| Job | Seconds | Run ID |
|-----|---------|--------|
| title | 7 | 39455724329 |
| changes | 17 | 39455724329 |
| hygiene | 17 | 39455724329 |
| build (linux-x64) | 37 | 39455724329 |
| build (linux-arm64) | 19 | 39455724329 |
| build (macos-arm64) | 28 | 39455724329 |
| build (macos-x64) | 37 | 39455724329 |
| build (windows-x64) | 49 | 39455724329 |
| build (windows-arm64) | 63 | 39455724329 |
| asan | 21 | 39455724329 |
| nofp (ubuntu-26.04) | 10 | 39455724329 |
| nofp (ubuntu-26.04-arm) | 16 | 39455724329 |
| be32 | 36 | 39455724329 |
| hash-equality | 5 | 39455724329 |
| wasm32 | 22 | 39455724329 |
| retroarch | 23 | 39455724329 |
| perf | 22 | 39455724329 |
| rehearsal | 328 | 39455724329 |
| cleanup | 9 | 39455724329 |
| CI required | 4 | 39455724329 |

- be32 took 36 s and wasm32 took 22 s. Both lanes are cheap per PR, which answers the DECISIONS open question on per-PR cost.
- The rehearsal's time goes almost all to `publish.sh attested` (119 s) and `publish.sh verify` (184 s), which check attestations archive by archive. Upload took 13 s.
- The macOS build legs queued for about 80 to 110 s before they started, so the run's wall time is longer than the sum of its critical-path jobs.
- Nightly durations: in run 39457820699, bench (ubuntu-26.04) took 29 s, bench (ubuntu-26.04-arm) 34 s, bench (macos-15) 57 s, bench-wasm32 66 s and report 8 s. In run 39457445780 they were 39 s, 34 s, 66 s, 106 s and 6 s.
- This is one sample per job. Plan 01-15 sets the timeouts from these numbers.

## Evidence (commands from the repository root, exit status)

- `sh scripts/hygiene.sh --tree`: exit 0. `cmake --workflow --preset ci` before the push: exit 0.
- `git push -u origin phase/01-a-test-frame-in-retroarch`: exit 0. `gh pr create --draft ...`: exit 0.
- `timeout 580 gh run watch 39455724329 --exit-status --interval 30`: exit 0.
- Task 1 verify (draft PR from the phase branch, CI run on HEAD): exit 0. Task 2 verify (latest CI run for HEAD is success): exit 0. `gh run view 39455724329 --json jobs -q '.jobs[] | select(.name == "CI required") | .conclusion'` printed success.
- I read these from `gh run view 39455724329 --log` (exit 0), grepping per job:
  - Every check_junit step passed: both steps on each of the six build legs, and the single step in asan, both nofp legs, be32, wasm32 and hygiene.
  - hash-equality printed `hashes: 7 identical files`, and wasm32 printed `wasm32 hashes.txt equals Linux x64's`.
  - retroarch printed `RetroArch 1.22.2`, then `320x240, every pixel equal`. The flip control printed `compare exited 1: first difference at 7,3`.
  - rehearsal printed `publish: 18 archives from 2f369db…; SHA256SUMS written`, then `rehearsal-39455724329-1 published with 19 assets; every download matches SHA256SUMS and verifies`. cleanup printed `rehearsal-39455724329-1 and its release are gone`.
  - `release.archive_contents` and its self test passed on all six legs.
  - perf printed `perf testcard head=1620415 base=none delta_bp=none verdict=NO_BASE`.
- `gh release list --limit 20 --json tagName,isPrerelease` printed `[]`. `git ls-remote --tags origin | grep -c rehearsal` printed 0.
- Task 3 verify (the latest nightly on the branch is success): exit 0. `gh issue list --label nightly --state open --json number -q length` printed 0. `gh issue view 3` reported state CLOSED.
- Cruft grep (the plan's two `git grep --untracked` commands under bash): a=1, b=1, exit 0.
- Planning-ID scan over the plan's path list, followed by `test $? -eq 1`: exit 0.
- At the end, `cmake --workflow --preset ci` exited 0 and `cmake --workflow --preset hygiene` exited 0.
- `git log origin/main..HEAD --oneline | wc -l` printed 56, and nothing was pushed to main.

## Hosted fixes

None. Every lane passed on its first hosted run, including the three risks the research named: MSVC ARM64 COMDATs and ppc32 libgcc helpers in `abi.undefined`, RetroArch frame alignment, and the be32/wasm32 timeout scales.

## Decisions Made

See `key-decisions` in the frontmatter.

## Deviations from Plan

None. The plan ran as written, and Task 2's fix loop had nothing to fix. Tasks 1 and 2 made no source change, so they have no task commit. The only commit is this SUMMARY's metadata commit.

## Issues Encountered

None. Two items carry over unchanged:
- The macOS Debug `dev`-preset `abi.undefined` failure in deferred-items.md. It is not a hosted failure, and `tests/cmake/abi.cmake` was not edited.
- The `docs/testing.md` CI table still lacks the rehearsal and cleanup rows. That file is outside this plan's files.

## Known Stubs

None.

## Next Phase Readiness

Plan 01-15 sets every timeout from the durations above (one sample each) and closes probe PR #2 unmerged. FRAME-01 and FRAME-05 stay open in REQUIREMENTS.md while 01-15 declares them.

## Self-Check: PASSED

- No files to create. The run, issue and PR URLs above were read back with `gh` (exit 0).
- commits: 0, with no code change, so this was evidence-only work.
