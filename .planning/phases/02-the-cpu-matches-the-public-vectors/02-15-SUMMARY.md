---
phase: 02-the-cpu-matches-the-public-vectors
plan: 15
subsystem: testing
tags: [vectors, nightly, pins, singlesteptests, ci, cmake-presets]

requires:
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: the vectors harness, the committed sample and the allow file, with every instruction family passing the sample (plan 02-12)
provides:
  - 55 r3000 rows in pins.txt, each a name, the upstream commit, a git blob SHA-1 and a size
  - --upstream DIR and --prefix-of SAMPLE in the vectors harness, which replays the 55 full upstream files and proves the committed sample is their first records
  - the nightly configure, build, test and workflow presets, registering only tests/nightly with its own inventory
  - vectors.r3000.full, guarded at configure time so a missing pinned file fails naming it and the lane never skips
  - the vectors-full job in nightly.yml, outside the CI required aggregate and feeding the rolling nightly issue
affects: [02-17, 02-19, 02-25]

actuals:
  tokens: 4500
  tasks: 3
  commits: 4

plan_head_before: 6c99cd4ba3081d3488fcfbaaac8486415184c933
plan_head_after: ad2fcd35e7bc6b33db8f5808960d64601c336eac
commits: 4

tech-stack:
  added: []
  patterns:
    - "one parser serves the committed sample and the full upstream files: --upstream swaps each sample slice for the whole file after a byte compare, then runs the same check_slice and run_slice"
    - "a lane that cannot skip: the nightly directory's tests.cmake errors at configure naming the first pinned file that SCHWAYSTATION_SST_DIR lacks, so a registered test always has its data"
    - "pin rows of two shapes share pins.txt: download rows (name version url sha256) read by fetch-pin.sh, and r3000- rows (name pin blob-sha1 size) read by the nightly job and the nightly configure guard"

key-files:
  created:
    - tests/nightly/tests.cmake
    - tests/nightly/inventory.txt
    - tests/inventory/nightly.txt
  modified:
    - tests/vectors/vectors.c
    - pins.txt
    - tests/tests.cmake
    - CMakePresets.json
    - .github/workflows/nightly.yml
    - docs/testing.md

key-decisions:
  - "The prefix check compares every byte of the sample slice after its count field with the upstream file, and the sample's patched count is the only difference allowed; a mismatch prints the upstream file and the first differing offset"
  - "tests/nightly/tests.cmake defines its own schwaystation-test-vectors target from tests/vectors/vectors.c rather than including tests/vectors/tests.cmake, because that file registers the sample's thirteen tests and the nightly inventory holds only vectors.r3000.full"
  - "The nightly build preset builds only schwaystation-test-vectors, so the lane does not compile the runner's tests or the libretro core it never runs"
  - "vectors.r3000.full is also given allow.txt, so an accepted upstream mismatch is one dated line whether the case sits in the sample or only in the full files"
  - "timeout-minutes on vectors-full stays 5 with the provisional comment the plan names; the dispatched run's job took 47 s, so a later ci: change can replace the comment with the run ID"
  - "Docs: docs/testing.md gets the nightly lane row and a short The full vectors section, since AGENTS rule 5 puts testing docs there and the fetch recipe has to live somewhere a maintainer will look"

patterns-established:
  - "A nightly lane is a preset plus a tests/<dir> inventory selected by tests/inventory/<preset>.txt, returned to early from tests/tests.cmake like the hygiene and nofp lanes"

requirements-completed: [CPU-01]

coverage:
  - id: D1
    description: "pins.txt holds one row per upstream r3000 file, r3000-NAME 51e227447d0b blob-sha1 size, 55 rows under a comment naming the gh api fetch and the git hash-object check, and every blob was fetched and hashed to its pin"
    requirement: "CPU-01"
    verification:
      - kind: other
        ref: "test $(grep -cE '^r3000-[A-Za-z]+ 51e227447d0b [0-9a-f]{40} [0-9]+$' pins.txt) -ge 55 (exit 0); git hash-object of each of build/sst/*.json.bin equals its row (no mismatch printed)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The harness reads the 55 upstream files with --upstream DIR (names from --prefix-of SAMPLE), fails naming the file and first differing offset unless each file starts with the committed sample, and runs all 55,000 cases with 0 failures"
    requirement: "CPU-01"
    verification:
      - kind: integration
        ref: "build/ci/tests/schwaystation-test-vectors --upstream build/sst --prefix-of tests/vectors/r3000/sst-sample.bin --expect 55000 --allow tests/vectors/r3000/allow.txt (exit 0, 55000 cases, 0 failures)"
        status: pass
      - kind: other
        ref: "the same command on a copy of build/sst with one byte changed inside ADD.json.bin exits 1 naming ADD.json.bin and offset 1000; with XOR.json.bin removed it exits 1 naming the unreadable file; with --upstream and no --prefix-of it exits 2"
        status: pass
    human_judgment: false
  - id: D3
    description: "The nightly preset registers only tests/nightly, whose vectors.r3000.full runs the 55,000 cases with the prefix check; configure fails naming the missing file when SCHWAYSTATION_SST_DIR lacks any pinned file, and the ci workflow still passes with 153 tests"
    requirement: "CPU-01"
    verification:
      - kind: integration
        ref: "cmake --workflow --preset nightly (exit 0) then cmake -DJUNIT=build/nightly/junit.xml -DINVENTORY=tests/inventory/nightly.txt -P tests/cmake/check_junit.cmake (exit 0, 1 test ran once)"
        status: pass
      - kind: other
        ref: "cmake --preset nightly -DSCHWAYSTATION_SST_DIR=build/sst-missing exits non-zero and prints is missing; cmake --workflow --preset ci then exits 0"
        status: pass
    human_judgment: false
  - id: D4
    description: "nightly.yml's vectors-full job fetches the pinned blobs, checks each with git hash-object, runs the nightly workflow preset and its JUnit check, is in the report job's needs and is outside ci.yml and CI required; it ran green once on the branch"
    requirement: "CPU-01"
    verification:
      - kind: e2e
        ref: "gh workflow run nightly.yml --ref phase/02-the-cpu-matches-the-public-vectors, run https://github.com/szTheory/schwaystation/actions/runs/42703494243 concluded success, job vectors-full success in 47 s"
        status: pass
      - kind: other
        ref: "cmake --workflow --preset hygiene (exit 0) and test $(grep -c vectors-full .github/workflows/nightly.yml) -ge 2 (exit 0)"
        status: pass
    human_judgment: false

duration: 15min
completed: 2026-11-09
status: complete
---

# Phase 2 Plan 15: The nightly full-vectors lane Summary

**All 55,000 public single-step vectors pass at their blob-SHA-1 pins, with the committed sample proven to be their first records, through a configure-guarded `nightly` preset and a `vectors-full` job that has run green on the branch.**

## Performance

- **Duration:** about 15 min of authoring and local runs, plus a 2 min hosted wait
- **Started:** 2026-11-09T21:20:00Z (approximate; the start time was not recorded)
- **Completed:** 2026-11-09T21:36:00Z
- **Tasks:** 3
- **Files modified:** 9

## Accomplishments

- 55 `r3000-<NAME>` rows in `pins.txt`, taken from `gh api repos/SingleStepTests/r3000/git/trees/51e227447d0b?recursive=1`, with a comment naming the row shape and the fetch and `git hash-object` check. All 55 blobs were fetched to `build/sst` and hashed to their pins and sizes.
- `--upstream DIR --prefix-of SAMPLE` in the harness. The full set passed first time: 55,000 cases, 0 failures (rule (a) handled 2,984 cases and rule (b) 4,000). A changed byte, a missing file and a half-given option are each refused, with the file and offset named.
- The `nightly` preset and `tests/nightly` with `vectors.r3000.full` (labels `portable vectors`, TIMEOUT 120) and its own inventory. The configure guard names the first missing file; `cmake --workflow --preset ci` still passes all 153 tests.
- The `vectors-full` job in `nightly.yml` and a `docs/testing.md` row and section. The dispatched run on the branch succeeded and the job took 47 s.

## Task Commits

1. **Task 1: Tracer - the harness runs the 55 fetched files and proves the sample is their prefix** - `ef03d94` (feat)
2. **Task 2: The nightly preset, inventory and configure-time guard** - `7e3dfd7` (feat)
3. **Task 3: The vectors-full job, dispatched once on the branch** - `36d115b` (ci), with `ad2fcd3` (ci) reading only the used fields of each pin row

**Plan metadata:** the docs commit that carries this summary, STATE.md and ROADMAP.md.

## Files Created/Modified

- `tests/vectors/vectors.c` - `check_slice` split out of the container parser, `load_upstream`, the two new options and their usage text
- `pins.txt` - the 55 `r3000-` rows and their comment block
- `tests/nightly/tests.cmake` - the configure guard, the harness target and `vectors.r3000.full`
- `tests/nightly/inventory.txt`, `tests/inventory/nightly.txt` - the nightly directory and its selection
- `tests/tests.cmake` - includes only the nightly directory and returns when the inventory is `nightly`
- `CMakePresets.json` - `nightly` configure, build, test and workflow presets
- `.github/workflows/nightly.yml` - the `vectors-full` job and the report job's `needs`
- `docs/testing.md` - the lane row and The full vectors section

## Decisions Made

Recorded under key-decisions above. The one that shapes later plans: the sample can no longer drift from the files the pins name, because the nightly run fails on any byte of difference in the first records, so a regenerated sample needs a pin check in the same change.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] vectors-full read pin and size fields it never used**
- **Found during:** Task 3 (actionlint's shellcheck pass on the new step, after the first commit)
- **Issue:** `read -r name pin sha size` left `pin` and `size` unused (SC2034).
- **Fix:** `read -r name _ sha _`.
- **Files modified:** `.github/workflows/nightly.yml`
- **Verification:** actionlint reports no shellcheck finding; the dispatched run, started after the fix, succeeded.
- **Committed in:** `ad2fcd3`

**2. [Rule 2 - Missing critical] Testing docs for the new lane**
- **Found during:** Task 2
- **Issue:** AGENTS rule 5 puts the lane's testing documentation in `docs/`, and the plan's file list names none. Without it the inventory list in `docs/testing.md` omitted `nightly.txt` and nothing said how to fetch the files.
- **Fix:** A table row, the inventory list entry and a short section.
- **Files modified:** `docs/testing.md`
- **Verification:** `cmake --workflow --preset hygiene` and `cmake --workflow --preset ci` pass.
- **Committed in:** `7e3dfd7`

---

**Total deviations:** 2 auto-fixed (1 bug, 1 missing critical)
**Impact on plan:** Both stay inside the plan's goal. No scope creep.

## Issues Encountered

- A local actionlint (older than the repository's runner labels) reports `ubuntu-26.04` as an unknown label on every job, including the phase-1 ones; that is the tool's table, not the workflow, and the hosted run accepted it.
- `cmake --preset nightly -DSCHWAYSTATION_SST_DIR=build/sst-missing` stores the override in the cache; rerunning the workflow preset resets it, because preset cache variables are applied on every configure.
- Each push to the branch also starts the `Nightly` workflow's pull-request event on draft PR 7, because `nightly.yml` is in its own path filter; those runs passed too.

## Known Stubs

None.

## Threat Flags

None. The job fetches public blobs with the workflow's own read-only token, checks each against a pinned hash, and commits, caches and uploads nothing, as the plan's prohibition requires.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- The nightly lane is live. A failing vector found by it is an allow line in `tests/vectors/r3000/allow.txt` (dated, with an upstream issue URL) or a fix in the core.
- `timeout-minutes: 5  # provisional` on `vectors-full` can become a measured comment in a later `ci:` change; the dispatched job took 47 s (run 42703494243).

## Self-Check: PASSED

- Created files exist: `tests/nightly/tests.cmake`, `tests/nightly/inventory.txt`, `tests/inventory/nightly.txt`.
- Commits `ef03d94`, `7e3dfd7`, `36d115b`, `ad2fcd3` are ancestors of HEAD; the ledger measured 4 commits since `6c99cd4`.
- Task 1, 2 and 3 acceptance criteria and verification commands were re-run and exit 0.

---
*Phase: 02-the-cpu-matches-the-public-vectors*
*Completed: 2026-11-09*
