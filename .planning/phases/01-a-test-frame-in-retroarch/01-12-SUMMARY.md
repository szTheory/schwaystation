---
phase: 01-a-test-frame-in-retroarch
plan: 12
subsystem: ci, e2e, hygiene, docs
tags: [github-actions, retroarch, e2e, screenshot-compare, action-pins, timeouts, cachegrind, hash-equality]
status: complete

requires:
  - phase: 01-06
    provides: "line_budget.cmake, expect_fail.cmake, the hygiene preset and its inventory"
  - phase: 01-07
    provides: "perf_compare.cmake (SCHWAYSTATION_PERF_MODE), hashes.cmake compare mode, the perf preset"
  - phase: 01-11
    provides: "the ci and ci-msvc workflow presets with package and release-test steps, install_line.cmake"
provides:
  - ".github/workflows/ci.yml: changes, hygiene, title, build (6 legs), asan, nofp (2 legs), be32, hash-equality (COUNT=7), wasm32 (report-only), retroarch (frame 5, 320x240), perf (report mode), CI required"
  - "pins.txt: three wasmtime 48.0.7 rows and the RetroArch 1.22.2 Metal DMG row"
  - "tests/retroarch: run_retroarch.cmake, test.cfg.in, bmp.c, compare.c, flip.cmake (293 lines)"
  - "tests/cmake/workflows.cmake with MODE=timeouts|pins and DIR"
  - "CTests policy.e2e_budget, hygiene.timeout_comments, policy.action_pins and their self tests"
  - "The CI jobs section of docs/testing.md"
affects: [01-13, 01-14, 01-15]

actuals:
  tokens: 12500
  tasks: 3
  commits: 3
plan_head_before: 52fb54995a2211e30a356955f0f146a1c71631be
plan_head_after: 3e97b7bb285b0efe3e86646b3227f0332ff13aaa

tech-stack:
  added: []
  patterns:
    - "One CMake -P script with a MODE switch replaces two near-identical workflow scans"
    - "Frame number and size live in the workflow YAML; the e2e files under tests/retroarch stay value-free and byte-identical to the seed"

key-files:
  created:
    - .github/workflows/ci.yml
    - pins.txt
    - tests/retroarch/run_retroarch.cmake
    - tests/retroarch/test.cfg.in
    - tests/retroarch/bmp.c
    - tests/retroarch/compare.c
    - tests/retroarch/flip.cmake
    - tests/cmake/workflows.cmake
  modified:
    - tests/hygiene/tests.cmake
    - tests/hygiene/inventory.txt
    - docs/testing.md

key-decisions:
  - "The five tests/retroarch files carry no PS1 value (the seed had none either): FRAMES=5, --frames 5 --png 5:runner.png and the 320x240 regex are in ci.yml, so the e2e count stays 293, net zero against the seed"
  - "Every timeout keeps the seed's bound with a comment ending provisional and no run ID; plan 01-15 sets measured values"
  - "CI required's jq program drops the rehearsal and cleanup clauses and the REHEARSAL_TAG env until plan 01-13 adds those jobs"
  - "perf passes -DFIXTURE=testcard and no -DMODE: this repository's perf_compare.cmake reads its mode only from SCHWAYSTATION_PERF_MODE"
  - "A failed retroarch job uploads build/frames/*.png as retroarch-frames so the frame counter can be read"

requirements-completed: [FRAME-01, FRAME-02, FRAME-05, FRAME-07, FRAME-09]

coverage:
  - id: D1
    description: "tests/retroarch holds the five e2e files, 293 lines in total, and policy.e2e_budget holds them at or under 300"
    requirement: FRAME-07
    verification:
      - kind: integration
        ref: "tests/hygiene/tests.cmake#policy.e2e_budget"
        status: pass
    human_judgment: false
  - id: D2
    description: "The e2e budget self test fails a 301-line fixture with its named message"
    requirement: FRAME-07
    verification:
      - kind: integration
        ref: "tests/hygiene/tests.cmake#policy.e2e_budget.self_test"
        status: pass
    human_judgment: false
  - id: D3
    description: "The compare chain the hosted job runs works on this host's images: runner PNG at frame 5, sips to BMP, compare prints 320x240, every pixel equal, and flip.cmake makes it exit 1 naming 7,3"
    requirement: FRAME-07
    verification:
      - kind: e2e
        ref: "build/ci/schwaystation-run --frames 5 --png 5:build/ci/a.png exit 0; sips --deleteColorManagementProperties and sips -s format bmp exit 0; compare a.bmp b.bmp exit 0 printing 320x240, every pixel equal; cmake -P tests/retroarch/flip.cmake exit 0 (compare exited 1 naming 7,3)"
        status: pass
    human_judgment: false
  - id: D4
    description: "ci.yml holds every lane but the rehearsal and is statically valid, with 13 jobs, COUNT=7 and FRAMES=5"
    requirement: FRAME-01
    verification:
      - kind: other
        ref: "actionlint -shellcheck= -pyflakes= -ignore 'label \"ubuntu-26\\.04(-arm)?\" is unknown' .github/workflows/ci.yml exit 0; yq '.jobs | keys' exit 0"
        status: pass
    human_judgment: false
  - id: D5
    description: "The six build legs, asan, nofp, be32 and CI required run green on hosted runners"
    requirement: FRAME-01
    verification:
      - kind: e2e
        ref: "hosted CI run of .github/workflows/ci.yml, run URL supplied by plan 01-14"
        status: pending
    human_judgment: false
  - id: D6
    description: "hash-equality compares the six legs' and be32's hashes.txt and wasm32 prints its comparison with Linux x64 report-only"
    requirement: FRAME-02
    verification:
      - kind: e2e
        ref: "hosted CI jobs hash-equality and wasm32, run URL supplied by plan 01-14"
        status: pending
    human_judgment: false
  - id: D7
    description: "hygiene.timeout_comments and policy.action_pins pass on .github/workflows"
    requirement: FRAME-05
    verification:
      - kind: integration
        ref: "tests/hygiene/tests.cmake#hygiene.timeout_comments"
        status: pass
      - kind: integration
        ref: "tests/hygiene/tests.cmake#policy.action_pins"
        status: pass
    human_judgment: false
  - id: D8
    description: "Their self tests fail a bare and a guessed timeout and an action used by tag, each with its named message"
    requirement: FRAME-05
    verification:
      - kind: integration
        ref: "tests/hygiene/tests.cmake#hygiene.timeout_comments.self_test"
        status: pass
      - kind: integration
        ref: "tests/hygiene/tests.cmake#policy.action_pins.self_test"
        status: pass
    human_judgment: false
  - id: D9
    description: "The retroarch job runs RetroArch 1.22.2 for 5 frames and its screenshot equals the runner's frame 5"
    requirement: FRAME-07
    verification:
      - kind: e2e
        ref: "hosted CI job retroarch, run URL supplied by plan 01-14"
        status: pending
    human_judgment: false
  - id: D10
    description: "The perf job measures Cachegrind Ir per frame at head and merge base and reports through perf_compare.cmake"
    requirement: FRAME-09
    verification:
      - kind: e2e
        ref: "hosted CI job perf on a pull request, run URL supplied by plan 01-14"
        status: pending
    human_judgment: false

duration: 6min
completed: 2026-10-20
---

# Phase 1 Plan 12: The Hosted CI Workflow, RetroArch Compare and Workflow Gates Summary

`ci.yml` now holds every lane a pull request needs: six build legs, the sanitizer, no-FP and big-endian lanes, seven-file hash equality, report-only wasm32, the RetroArch 1.22.2 screenshot compare at frame 5, and the Cachegrind perf report, all under one `CI required` check. Hygiene tests gate the workflow's timeouts and action pins, and the 293-line e2e code stays under its 300-line budget.

## Performance

- **Duration:** about 6 min wall time (2026-10-20T15:58:03Z to 16:04:08Z, before the SUMMARY)
- **Started:** 2026-10-20T15:58:03Z
- **Completed:** 2026-10-20
- **Tasks:** 3
- **Files modified:** 11 (8 created, 3 modified)

## Accomplishments

- `tests/retroarch/` is ported unchanged from the seed: the driver, config template, BMP reader, compare and flipped-pixel control. The PS1 values (5 frames, 320x240) live in the YAML, so the count is 293 lines, net zero against the seed.
- `policy.e2e_budget` counts `tests/retroarch/*` with `line_budget.cmake` against 300. Its self test fails a 301-line fixture.
- `.github/workflows/ci.yml` has 13 jobs. Every `uses:` is a full SHA with a `# v` comment (checkout v7.0.1, upload-artifact v7.0.2, download-artifact v8.0.2; each commit answered `gh api` 200 and matches its tag). Every checkout sets `persist-credentials: false`, and every timeout is `provisional`.
- `pins.txt` has the three wasmtime 48.0.7 rows, whose digests match the release API's `digest` field, and the RetroArch DMG row with `81b79121…db434`.
- `tests/cmake/workflows.cmake` (48 lines) merges the seed's timeout and action-pin scans into one script with `MODE=timeouts|pins`. It backs `hygiene.timeout_comments` and `policy.action_pins` and their self tests.
- `docs/testing.md` now has the CI jobs table, wasm32's report-only status, the pins, and how to read a red retroarch job's uploaded PNGs.

## Task Commits

1. **Task 1: e2e compare chain inside the 300-line budget** - `b34b76e` (test)
2. **Task 2: ci.yml and pins.txt** - `7d3927e` (ci)
3. **Task 3: workflow gates and the CI section** - `3e97b7b` (test)

## FRAME-02 reading for the verifier

`hash-equality` gates on the seven lanes that gate: the six build legs plus `be32`, with `hashes.cmake -DCOUNT=7 -DMODE=compare`. The eighth `hashes.txt`, from `wasm32`, is compared with Linux x64's and printed to the step summary as report-only. It sits outside `CI required` until the phase-1 `ci:` tail promotes it. `be32` gates from day one.

## Evidence (commands from the repository root, exit status)

- `test "$(cat tests/retroarch/* | wc -l)" -le 300 && cmake --workflow --preset hygiene && ctest --test-dir build/hygiene -R '^policy\.e2e_budget' --output-on-failure`: exit 0. `cat tests/retroarch/* | wc -l` printed 293. This was also the tracer gate re-run.
- One-off compare chain, with scratch binaries outside the tree:
  - `cc -std=c99 -O2 -Wall -o <scratch>/compare tests/retroarch/bmp.c tests/retroarch/compare.c`: exit 0
  - `build/ci/schwaystation-run --frames 5 --png 5:build/ci/a.png`: exit 0
  - `sips --deleteColorManagementProperties` then `sips -s format bmp` on two copies: exit 0 each
  - `compare a.bmp b.bmp`: exit 0, printed `320x240, every pixel equal`
  - `cmake -DCOMPARE=… -DEXPECTED=a.bmp -DBMP=b.bmp -DFLIPPED=flipped.bmp -P tests/retroarch/flip.cmake`: exit 0. The compare exited 1 with `first difference at 7,3: expected FFFFFF, actual FFFF00`.
- `actionlint -shellcheck= -pyflakes= -ignore 'label "ubuntu-26\.04(-arm)?" is unknown' .github/workflows/ci.yml`: exit 0. See Deviations for why the plain command exits 1.
- `yq '.jobs | keys' .github/workflows/ci.yml`: exit 0 (13 jobs). The rest of Task 2's verify (`length -ge 12`, `COUNT=7`, `FRAMES=5`): exit 0.
- `grep -c -i cogmaster .github/workflows/ci.yml pins.txt`: 0 and 0. `grep -E -c 'runs? [0-9]{9,}' .github/workflows/ci.yml`: 0. `persist-credentials: false` count 11, equal to `actions/checkout@` count 11.
- `cmake --workflow --preset hygiene`: exit 0 (21 tests). `cmake -DJUNIT=build/hygiene/junit.xml -DINVENTORY=tests/inventory/hygiene.txt -P tests/cmake/check_junit.cmake`: exit 0 (21 ran once each).
- `wc -l tests/cmake/workflows.cmake`: 48, within the 50 ceiling. The gate-line total `wc -l scripts/*.sh tests/cmake/*.cmake .githooks/* | tail -1` printed 1392, within 1500.
- `ls tests/cmake/timeouts.cmake tests/cmake/action_pins.cmake 2>/dev/null | wc -l`: 0. The inventory grep for the three gate families printed 6.
- Cruft grep (the plan's two `git grep --untracked` commands): a=1, b=1, exit 0.
- Planning-ID scan over `tests/retroarch tests/hygiene tests/cmake/workflows.cmake .github/workflows/ci.yml pins.txt`: exit 1, so no matches. The same scan over `docs/testing.md`: exit 1.
- `cmake --workflow --preset ci`: exit 0 (77 tests, then 2 release tests). Both JUnit checks (`-DEXCLUDE=release`, `-DLABELS=release`): exit 0.
- Hosted facts (the green run of each job, the RetroArch compare, hash equality, perf report): pending until plan 01-14 supplies the run URL.

## Decisions Made

See `key-decisions` in the frontmatter.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] actionlint's built-in runner list does not know ubuntu-26.04**
- **Found during:** Task 2
- **Issue:** The local actionlint 1.7.12 reports `label "ubuntu-26.04" is unknown` (and `ubuntu-26.04-arm`) on every Linux job. The plan and the seed's successful hosted runs both use these labels, and it was the only finding.
- **Fix:** Ran actionlint with `-ignore 'label "ubuntu-26\.04(-arm)?" is unknown'`, which suppresses only that message. I added no actionlint config file, since a self-hosted-label config would misdescribe the runners and is outside files_modified. With the filter, exit 0.
- **Files modified:** none
- **Commit:** n/a

### Plan interpretation

- The seed's `tests/retroarch` files contain no frame count, size or emulator name. Porting them meant copying them unchanged; the PS1 values went into ci.yml as the plan lists.
- The perf step passes `-DFIXTURE=testcard` and no `-DMODE`, to match this repository's `perf_compare.cmake` interface from plan 01-07. The seed passed `-DROM` and `-DMODE`.
- The workflow-level `REHEARSAL_TAG` env and the jq clause for `rehearsal`/`cleanup` are left out with those jobs. Plan 01-13 adds them back.

**Total deviations:** 1 auto-fixed (blocking tooling). **Impact:** none on the workflow; only the one-off lint command differs.

## Issues Encountered

None. The known `dev`-preset `abi.undefined` failure on macOS Debug is still in deferred-items.md. This plan did not touch it.

## Known Stubs

None.

## Next Phase Readiness

Plan 01-13 adds the rehearsal and cleanup jobs and their `CI required` clauses. Plan 01-14 runs the workflow on hosted runners and records the run URL for D5, D6, D9 and D10. If that run shows a fixed one-frame offset, the frame counter in `retroarch-frames` is read and the runner frame pinned with a comment. FRAME-01, 02, 05, 07 and 09 stay open in REQUIREMENTS.md, because plans 01-13 through 01-15 still declare them.

## Self-Check: PASSED

- FOUND: .github/workflows/ci.yml, pins.txt, tests/retroarch/{run_retroarch.cmake,test.cfg.in,bmp.c,compare.c,flip.cmake}, tests/cmake/workflows.cmake
- FOUND: b34b76e, 7d3927e, 3e97b7b (ancestors of HEAD)
