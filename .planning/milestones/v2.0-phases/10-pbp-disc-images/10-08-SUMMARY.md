---
phase: 10-pbp-disc-images
plan: "08"
subsystem: host-media
tags: [c, pbp, runner, probe, ctest, docs]
requires:
  - phase: 10-pbp-disc-images
    provides:
      - validated PBP provider and deterministic single-track source fixture
      - provider parity and PBP preflight selectors
provides:
  - runner --disc and --probe support for the fixture-proven unencrypted PSISO layout
  - bounded runner output, save-identity parity, and pre-frame malformed/unsupported controls
  - user documentation for PBP scope, limits, errors, and focused checks
affects: [runner, host-media, media-testing, playlists]
actuals:
  tokens: 6653
  tasks: 2
  commits: 2
commits: 2
plan_head_before: e7667e5daa04c70827ab81237368736bb79ee9a2
plan_head_after: b5b00767965292409be5b5b7a5c43ccfda184f9f
tech-stack:
  added: []
  patterns:
    - Route runner probing through the shared host disc-image reader and preserve the existing core boundary.
    - Compare bounded guest output and save identity against the matching single-track source CUE.
    - Keep PBP out of M3U entries while enabling standalone PBP media.
key-files:
  created: []
  modified:
    - host/disc_image.c
    - host/disc_probe.c
    - tests/runner/tests.cmake
    - tests/runner/inventory.txt
    - docs/discs.md
    - docs/runner.md
    - docs/media-testing.md
    - .planning/WINDOWS.md
key-decisions:
  - Leave runner/main.c unchanged because its --disc path already uses the shared provider; remove the obsolete probe-only PBP rejection.
  - Use a fixed 1,200-frame hash comparison because the owned guest does not reach mailbox completion by that cap; PBP and source CUE both exit 0 with identical output.
  - Keep playlist members limited to the existing ISO/CUE contract when adding standalone PBP support.
requirements-completed: []
coverage:
  - id: runner_probe
    description: The runner probes and reads the fixture-proven PBP with the same identity and bounded guest output as its single-track CUE source.
    requirement: PBP-01
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^runner[.]disc[.]pbp$|^disc[.]image[.]pbp_parity$|^runner[.]disc[.]unsupported$|^docs[.]budgets$' --output-on-failure; exit 0; 5/5 including PBP preflight"
        status: pass
    human_judgment: false
  - id: input_rejection
    description: PGD and unknown PSAR layouts return unsupported, while a corrupt table and truncated stream return malformed-media errors before a guest frame.
    requirement: PBP-01
    verification:
      - kind: integration
        ref: "runner.disc.pbp checks --probe and --disc exits/messages plus absence of frame output for all four generated controls; exit 0"
        status: pass
    human_judgment: false
  - id: docs_and_bounds
    description: Documentation states only the tested PBP subset, links the focused checks, and the repository gate remains below its fixed ceiling.
    requirement: PBP-01
    verification:
      - kind: other
        ref: "docs.budgets; sorted runner/disc inventories; git diff --check origin/main...HEAD and git diff --check HEAD; line_budget.cmake reports 3900/3901; all exit 0"
        status: pass
    human_judgment: false
duration: 24m
completed: 2026-11-21
status: complete
---

# Phase 10 Plan 08: Runner PBP Support

**The runner now probes and reads the tested unencrypted PBP layout and matches its source CUE output and save identity.**

## Performance

- **Runtime/model:** Codex/OpenGSD 1.20.0; executor model `gpt-6-luna`; reasoning effort was not exposed.
- **Elapsed interval:** About 24 minutes from the saved STATE checkpoint at 13:16Z through closeout; active time was not separately measured.
- **Tasks:** 2. **Task commits:** 2. **Files changed:** 8, including the resolved broken-windows entry.
- **Observed bottleneck:** The final focused CTest run took 4.85 seconds, with `runner.disc.pbp` accounting for 4.83 seconds. PBP fixture preflight and full provider parity each took about 0.01 seconds. The bounded guest comparison dominates this small local path.
- An initial guest assertion used `--stop-on mailbox`; the fixture reached its 1,200-frame cap without a mailbox result. Direct fixed-frame runs of PBP and its single-track CUE both exited 0 and produced the same frame hash, so the test now asserts that bounded output and parity. No full suite or hosted CI was repeated; this plan changed no workflow.

## Accomplishments

- Removed the obsolete PBP suffix rejection from the host probe and used the existing shared image provider. `runner/main.c` already routed `--disc` through that provider and needed no change.
- Added `runner.disc.pbp`: probe output, a fixed-frame guest hash and memory-card identity match the owned single-track CUE; PGD, unknown markers, corrupt tables and truncated blocks fail with stable categories before guest execution.
- Preserved the documented ISO/CUE-only playlist contract with an explicit M3U PBP rejection. Updated the existing unsupported-media regression to test standalone PBP through its dedicated valid/malformed cases.
- Updated runner and disc documentation with the exact supported subset, 1 GiB limit, error categories and focused runner/provider command.
- The repository gate remains 3,900/3,901 lines.

## Task Commits

1. **Task 1: Accept supported PBP through runner load and probe paths** — `b8f43b1`.
2. **Task 2: Keep user media documentation aligned with the tested subset** — `b5b0076`.

## Commands and Results

- `LC_ALL=C sort -cu tests/runner/inventory.txt && LC_ALL=C sort -cu tests/disc/inventory.txt && git diff --check origin/main...HEAD && git diff --check HEAD && cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` — exit 0; inventories sorted and gate measured 3,900/3,901.
- `ctest --preset ci --no-tests=error -N -R '^runner[.]disc[.](pbp|unsupported)$|^disc[.]image[.]pbp_parity$|^docs[.]budgets$'` — exit 0; runner selectors, PBP parity, docs budget and required preflight were present.
- `cmake --preset ci -DSCHWAYSTATION_CHD=OFF -DSCHWAYSTATION_BUILD_CHD_TOOL=OFF` — exit 0; the initial plan configure.
- `cmake --build --preset ci --target schwaystation-run schwaystation-run-test schwaystation-pbp-fixtures` — exit 0.
- Initial `ctest --preset ci --no-tests=error -R '^runner[.]disc[.]pbp$' --output-on-failure` — CTest exit 8; `--stop-on mailbox` returned runner exit 8 at frame 1,200. The test was corrected after the matching fixed-frame PBP/CUE runs below showed identical output.
- `cmake --build --preset ci --target schwaystation-run` — exit 0; rebuilt the shared image provider and runner after adding the playlist compatibility guard.
- `ctest --preset ci --no-tests=error -R '^runner[.]disc[.]unsupported$' --output-on-failure` — exit 0; 1/1 in 0.04s.
- `cmake --preset ci -DSCHWAYSTATION_CHD=OFF -DSCHWAYSTATION_BUILD_CHD_TOOL=OFF` — exit 0; configure completed in about 1.1s.
- `ctest --preset ci --no-tests=error -R '^runner[.]disc[.]pbp$|^disc[.]image[.]pbp_parity$|^runner[.]disc[.]unsupported$|^docs[.]budgets$' --output-on-failure` — exit 0; 5/5 in 4.85s, including fixture preflight.
- Both of these exact guest commands exited 0 and reported the same frame-1200 hash, `83358471dc7807f9a7b070097835835ff27fa73a3db1f8ad468e3ef5208317b1`:

```sh
build/ci/schwaystation-run --bios build/ci/openbios/openbios-fast.bin --disc build/ci/fixtures/schwaystation-disc_boot.pbp --frames 1200 --hash
build/ci/schwaystation-run --bios build/ci/openbios/openbios-fast.bin --disc build/ci/fixtures/disc_boot_pbp.cue --frames 1200 --hash
```
- `gsd_run windows append --kind deviation --phase 10 --file host/disc_image.c ...` and `gsd_run windows fixed 15` — both completed; the compatibility correction is recorded as fixed, with no new open item.
- No `act` or hosted run applied because this plan did not modify workflow routing or CI configuration.

## Deviations from Plan

### Auto-fixed issue

**1. [Rule 1 - Compatibility] Kept standalone PBP support from broadening M3U membership.**
- **Found during:** Task 1
- **Issue:** The shared provider can open PBP directly, so M3U would otherwise gain a media member beyond its documented ISO/CUE set.
- **Fix:** Reject PBP playlist entries explicitly and update the old generic unsupported-media selector to cover PBP through the new dedicated runner cases.
- **Files modified:** `host/disc_image.c`, `tests/runner/tests.cmake`
- **Commit:** `b8f43b1`; broken-windows entry 15 is fixed.

## Self-Check: PASSED

- Summary file exists; both task commits are ancestors of the plan head.
- No generated fixture, BIOS, disc, executable, memory-card or state bytes were added to the repository.
- No new stubs, skipped tests or unrun planned verification remain.
