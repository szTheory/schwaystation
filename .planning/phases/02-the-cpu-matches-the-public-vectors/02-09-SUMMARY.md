---
phase: 02-the-cpu-matches-the-public-vectors
plan: 09
subsystem: testing
tags: [ci, armips, msvc, hash-equality, hosted-runners, draft-pull-request]

requires:
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: cmake/Armips.cmake, the hello fixture, fixtures.hello.header and the hashes.txt fixture line (plan 02-05)
provides:
  - a draft probe pull request (#7) from the phase branch, to be closed unmerged before ship
  - a green CI run on the head commit with armips built on all six build legs, be32 and wasm32, and hash-equality over eight files
  - three hosted-only fixes (an audio-pair test timeout tier, a drive-letter file URL, an MSVC unreachable-code warning)
  - the armips submodule text corrected in PROVENANCE, CONFORMANCE section 2 and PX.50 and PX.58
  - a per-leg armips cost table for the CI timeout work in plan 02-27
affects: [02-10, 02-27]

actuals:
  tokens: 3900
  tasks: 3
  commits: 5

plan_head_before: 85d6b476be87e52a2701769856efb037981adda9
plan_head_after: 451c5264ac5344ac2747227fdf2acb5663211577
commits: 5

tech-stack:
  added: []
  patterns:
    - "a hosted-only failure is read from the failed job's log, fixed in the file that owns it, and each further push names the job, log line and run URL in its commit body"
    - "a drive-letter path becomes a file URL by prepending one slash when it has none, never by string(REGEX REPLACE) on a caret, because CMake restarts the caret after each match"

key-files:
  created: []
  modified:
    - tests/core/tests.cmake
    - tests/vectors/tests.cmake
    - tests/vectors/vectors.c
    - PROVENANCE.md
    - .planning/preparation/CONFORMANCE.md
    - .planning/preparation/PSXDEV-EXTRACTS.md

key-decisions:
  - "armips needed no change: cmake/Armips.cmake and .github/workflows/ci.yml built it unchanged on every hosted leg, MSVC x64 and arm64 included; every failure in the first runs was in a phase-2 test file"
  - "core.audio.pairs takes the 30 second tier the repository already uses for 34 tests; its assertions and its 4,000 frames per region are unchanged"
  - "the diagnostic that printed three URL forms was committed on its own and replaced by the fix, so the cause (the anchor restart) is recorded in a run log, not guessed"

requirements-completed: [CPU-04]

coverage:
  - id: D1
    description: "The phase branch is pushed and a draft probe pull request titled with a Conventional Commit exists; its CI run on the head commit is recorded with its URL"
    requirement: "CPU-04"
    verification:
      - kind: other
        ref: "gh pr view --json isDraft -q .isDraft prints true for https://github.com/szTheory/schwaystation/pull/7, and gh run list --commit 451c5264ac5344ac2747227fdf2acb5663211577 --workflow ci.yml names https://github.com/szTheory/schwaystation/actions/runs/42691517611 (exit 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: "armips builds from the pinned tarball and fixtures.hello.header passes on all six build legs, be32 and wasm32; asan and both nofp legs pass without building it"
    requirement: "CPU-04"
    verification:
      - kind: e2e
        ref: "cmake --workflow --preset ci, ci-msvc, be32 and wasm32 on the hosted legs of https://github.com/szTheory/schwaystation/actions/runs/42689688852 (all jobs success)"
        status: pass
      - kind: e2e
        ref: "gh run view 42691517611 --json jobs on https://github.com/szTheory/schwaystation/actions/runs/42691517611 (asan, nofp x2 and the eight armips lanes success)"
        status: pass
    human_judgment: false
  - id: D3
    description: "hash-equality passes over eight hashes.txt files carrying the same fixture hello.exe sha256 line, so armips assembles the fixture to one digest on every lane"
    requirement: "CPU-04"
    verification:
      - kind: integration
        ref: "test $(gh run view 42691517611 --json jobs -q '[.jobs[] | select(.name == \"CI required\" or .name == \"hash-equality\") | select(.conclusion == \"success\")] | length') -eq 2 (exit 0)"
        status: pass
    human_judgment: false
  - id: D4
    description: "PROVENANCE, CONFORMANCE section 2, PX.50 and PX.58 say that only ext/filesystem is a submodule at the pin, ext/tinyformat is vendored in the tarball, and with std::filesystem neither needs a second fetch"
    requirement: "CPU-04"
    verification:
      - kind: other
        ref: "grep -q tinyformat .planning/preparation/CONFORMANCE.md && grep -qi vendored .planning/preparation/CONFORMANCE.md && sh scripts/hygiene.sh --tree (exit 0)"
        status: pass
    human_judgment: false
  - id: D5
    description: "Each hosted-only failure is fixed in the file that owns it with the ci and hygiene workflows passing locally, and nothing is skipped or moved out of CI required"
    requirement: "CPU-04"
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci && cmake --workflow --preset hygiene after the last fix (exit 0, 114 and 2 tests passed)"
        status: pass
      - kind: e2e
        ref: "https://github.com/szTheory/schwaystation/actions/runs/42691517611 concludes success with CI required success"
        status: pass
    human_judgment: false
  - id: D6
    description: "Per-leg armips cost is recorded for the timeout work in plan 02-27"
    requirement: "CPU-04"
    verification:
      - kind: other
        ref: "the table in this file, measured from the job and step timestamps of run 42689688852"
        status: pass
    human_judgment: false

duration: 36min
completed: 2026-11-09
status: complete
---

# Phase 2 Plan 09: Prove armips on every hosted lane Summary

**armips builds from the pinned tarball and assembles hello.exe to one digest on all eight lanes, with a green CI run on the phase branch's draft probe pull request after three fixes to phase-2 test files that only the hosted legs showed.**

## Performance

- **Duration:** 36 min, most of it waiting on hosted CI (four CI runs of 6 to 9 minutes each)
- **Started:** 2026-11-09T19:50:42Z
- **Completed:** 2026-11-09T20:26:50Z
- **Tasks:** 3
- **Files modified:** 6

## Evidence

| What | Command | Exit | URL |
|------|---------|------|-----|
| Draft probe pull request | `gh pr view --json isDraft,title,url` (draft true, title `ci: probe the phase 2 fixture toolchain`) | 0 | https://github.com/szTheory/schwaystation/pull/7 |
| First CI run, head `85d6b47` | `gh run watch 42687433421 --exit-status` | 1 | https://github.com/szTheory/schwaystation/actions/runs/42687433421 |
| Second run, head `070f42c` | `gh run view 42688644088` | superseded by the next push | https://github.com/szTheory/schwaystation/actions/runs/42688644088 |
| Third run, head `6f5686c` | `gh run watch 42689148009 --exit-status` | 1 | https://github.com/szTheory/schwaystation/actions/runs/42689148009 |
| Green run, head `c84786f` | `gh run view 42689688852 --json conclusion` (success) | 0 | https://github.com/szTheory/schwaystation/actions/runs/42689688852 |
| Green run on the plan's head commit `451c526` | `gh run view 42691517611 --json conclusion` (success; `CI required` and `hash-equality` success) | 0 | https://github.com/szTheory/schwaystation/actions/runs/42691517611 |

The local commands before the first push, from the repository root: `sh scripts/hygiene.sh --tree` exit 0 and `cmake --workflow --preset ci` exit 0.

## Per-leg armips cost

Run 42689688852, the green run on the commit that fixed the last hosted-only failure. "Job" is the job's started-to-completed time. "armips span" runs from the first `schwaystation-armips` ExternalProject step line in the log to its `Completed` line, so it includes the download and the host-native configure and build and overlaps the library build.

| Job | Job duration (s) | armips span (s) | Run ID |
|-----|------------------|-----------------|--------|
| build (linux-x64) | 50 | 31 | 42689688852 |
| build (linux-arm64) | 35 | 20 | 42689688852 |
| build (macos-arm64) | 36 | 17 | 42689688852 |
| build (macos-x64) | 149 | 92 | 42689688852 |
| build (windows-x64) | 98 | 57 | 42689688852 |
| build (windows-arm64) | 98 | 43 | 42689688852 |
| be32 (host-native armips) | 53 | 22 | 42689688852 |
| wasm32 (host-native armips) | 56 | 32 | 42689688852 |

The slowest leg is macos-x64 (149 s job, 92 s armips). The plan 02-27 timeouts should start from that row and from the windows rows, not from the Linux ones. The `build` job's `timeout-minutes: 7` has 2.8 times the slowest job here.

## Accomplishments

- armips needed no change. `cmake/Armips.cmake` and `.github/workflows/ci.yml` built the pinned tarball unchanged on Linux x64 and arm64, macOS arm64 and x64, and MSVC x64 and arm64, and as a host-native build inside the `be32` and `wasm32` lanes. `fixtures.hello.header` passed on all eight, and `hash-equality` compared eight `hashes.txt` files with `-DCOUNT=8`.
- `asan` and both `nofp` jobs pass without building armips, as the presets select them out.
- Three failures that only hosted legs showed were fixed in the files that own them (below), and the same CI run is green on the plan's head commit with the documentation change.
- The armips text in PROVENANCE, CONFORMANCE section 2 and PX.50 and PX.58 is corrected after `gh api repos/Kingcom/armips/contents/ext?ref=62adab4ef30da765f5cf22a451eb08a59c54dc8b` showed `filesystem` as the submodule entry and `tinyformat` as a directory.

## Task Commits

1. **Task 1: tracer, push the branch, open the draft pull request, watch the first run** - no commit; the work is the push of the phase branch, pull request #7 and run 42687433421 (conclusion failure, see below)
2. **Task 2: fix what only the hosted legs show** - `03d13b5` (test, audio pair timeout tier), `070f42c` (test, diagnostic), `6f5686c` (test, file URL), `c84786f` (test, unreachable return)
3. **Task 3: the armips text and the per-leg cost** - `451c526` (docs)

**Plan metadata:** the commit that adds this file (docs: complete plan)

## Files Created/Modified

- `tests/core/tests.cmake` - `core.audio.pairs` registered on its own with `TIMEOUT 30`, the comment citing the two runs that set it
- `tests/vectors/tests.cmake` - the first-1000-bytes copy of the sample builds its file URL by prepending a slash only when the path has none
- `tests/vectors/vectors.c` - `main` ends on its `finish` call, with no unreachable `return`
- `PROVENANCE.md` - the armips row says only `ext/filesystem` is a submodule and `ext/tinyformat` is vendored
- `.planning/preparation/CONFORMANCE.md` - section 2 says the same and that the tarball is the only fetch
- `.planning/preparation/PSXDEV-EXTRACTS.md` - PX.50 and PX.58 say the same

## Decisions Made

- armips and the workflow needed no change, so the plan's `cmake/Armips.cmake` and `ci.yml` entries stay untouched; the plan listed them as possible owners, not as required edits.
- `core.audio.pairs` takes the 30 second tier. The test itself is unchanged: its 4,000 frames per region and every `CHECK` stand, and no test left `CI required`. The plan says nothing is "loosened"; I read that as assertions and gates, and a timeout is the one number the CI comments call provisional until measured. If the owner reads it as stricter, the alternative is to cut frames, which weakens the carried-remainder coverage, so I did not.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] core.audio.pairs timed out on the macOS Intel leg**
- **Found during:** Task 1 (first CI run, 42687433421, job 129397986045, log line `core.audio.pairs ***Timeout 10.06 sec`)
- **Issue:** the 10 second limit is too tight for 8,000 frames on that runner with 114 tests in parallel; the same leg passed in 6.42 s on main in run 40942957712 with 79 tests
- **Fix:** `TIMEOUT 30`, the tier 34 tests use, with the evidence in the comment
- **Files modified:** `tests/core/tests.cmake`
- **Verification:** local `cmake --workflow --preset ci` exit 0 (114 tests); macOS x64 job success in runs 42689148009, 42689688852 and 42691517611
- **Committed in:** `03d13b5`

**2. [Rule 1 - Bug] Both Windows legs failed at configure on the sample copy (curl error 37)**
- **Found during:** Task 2 (run 42687433421, jobs 129397986106 and 129397986245, `tests/vectors/tests.cmake:44`)
- **Issue:** the cause was not in the log. Commit `070f42c` printed the path, the file size and three URL forms; run 42688644088, job 129402263836 showed `url=file:///D/:/a/schwaystation/...` against `file:///D:/a/...` reading fine. `string(REGEX REPLACE "^([^/])" "/\\1")` restarts the caret after each match, so `D:/a` became `/D/:/a`. The line had never run on a path that starts without a slash.
- **Fix:** `set` the URL path and `string(PREPEND ...)` one slash when it does not match `^/`; the diagnostic is gone
- **Files modified:** `tests/vectors/tests.cmake`
- **Verification:** configure passes on both Windows legs in runs 42689148009 and later; local ci workflow exit 0
- **Committed in:** `070f42c` (the diagnostic), `6f5686c` (the fix)

**3. [Rule 1 - Bug] MSVC warning C4702 on the vector harness, an error under /WX**
- **Found during:** Task 2 (run 42689148009, jobs 129404212166 and 129404212019, `vectors.c(625)`), which showed once the configure error was gone
- **Issue:** MSVC inlines `finish`, which calls `exit`, so the `return 1;` after it is unreachable
- **Fix:** end `main` on the `finish` call; C allows reaching the end of `main`
- **Files modified:** `tests/vectors/vectors.c`
- **Verification:** local ci workflow exit 0 with no warning; both Windows legs green in runs 42689688852 and 42691517611
- **Committed in:** `c84786f`

---

**Total deviations:** 3 auto-fixed (3 Rule 1), each acting on a failing job, a log line and a run URL named in its commit body.
**Impact on plan:** all three were in phase-2 test files that no hosted run had built before; none touched armips. The probe found them before any plan consumes an assembled fixture, which is what the probe is for. Three pushes followed the first run, inside the plan's bounds.

## Issues Encountered

- The first run's Windows jobs could not reach the armips step because of fix 2, and the third run's Windows jobs stopped on fix 3 right after the armips configure step. armips itself configured and built under MSVC on both architectures in the green run.
- `gh run view --log-failed` was too noisy to read on a run with a large armips build; the useful route was `gh run view <id> --log --job <job id>` with the colour codes stripped, and `gh api --allow-escape-sequences repos/.../actions/jobs/<id>/logs` for a job that finished before its run did.
- The `rehearsal` job creates and `cleanup` deletes a prerelease named `rehearsal-<run id>-<attempt>` on each run; that is the existing pipeline, not this plan.

## Known Stubs

None.

## Threat Flags

None. No new network endpoint, authentication path, file access pattern or schema change.

## Authentication Gates

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Ready for plan 02-10 and the later plans that consume an assembled fixture: the toolchain is proven on every hosted lane.
- Pull request #7 is a draft and still open. It is closed unmerged before `/gsd-ship`, and it must never be merged or have auto-merge enabled.
- Plan 02-27 has the per-leg cost table above. Decision D-48's `workflow.test_gate_timeout` re-derivation is still due at phase 2 ship.

## Self-Check: PASSED

- `tests/core/tests.cmake`, `tests/vectors/tests.cmake`, `tests/vectors/vectors.c`, `PROVENANCE.md`, `.planning/preparation/CONFORMANCE.md` and `.planning/preparation/PSXDEV-EXTRACTS.md` exist and changed in `85d6b47..451c526`.
- Commits `03d13b5`, `070f42c`, `6f5686c`, `c84786f` and `451c526` are ancestors of HEAD.
- Acceptance: `test "$(gh pr view --json isDraft -q .isDraft)" = true` exit 0; `gh run list` finds a run for the head commit; the green run has `CI required` and `hash-equality` success (count 2); `cmake --workflow --preset ci` and `cmake --workflow --preset hygiene` exit 0.

---
*Phase: 02-the-cpu-matches-the-public-vectors*
*Completed: 2026-11-09*
