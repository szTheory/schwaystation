---
phase: 10-pbp-disc-images
plan: "07"
subsystem: host-media
tags: [c, pbp, libfuzzer, sanitizer, nightly-ci]
requires:
  - phase: 10-pbp-disc-images
    provides:
      - validated PBP sector reader and deterministic build-local fixture
provides:
  - Production-parser replay and sanitizer coverage for hostile PBP metadata and streams
  - A bounded PBP Nightly campaign with a classifier-limited routing path
affects: [host-media, disc-testing, fuzzing, nightly-ci]
actuals:
  tokens: 7368
  tasks: 2
  commits: 2
plan_head_before: 483d9f886528b00607961ac67181b98a6d33dc2b
plan_head_after: e2115d1f1a202ec918eb1d43a5c53492524c4075
tech-stack:
  added: []
  patterns:
    - Generate compact mutation scripts against build-owned raw and compressed media baselines.
    - Restore the selected baseline after each bounded parser input.
key-files:
  created:
    - tests/fuzz/pbp.c
  modified:
    - tests/fuzz/tests.cmake
    - tests/fuzz/inventory.txt
    - tests/fuzz/seeds.h
    - CMakePresets.json
    - tests/disc/pbp.c
    - .github/workflows/nightly.yml
    - tests/cmake/workflows.cmake
    - docs/media-testing.md
key-decisions:
  - Keep the committed corpus as 14 short scripts with at most 51 byte edits and a 256-byte input cap; generate valid raw and compressed PBP baselines at runtime.
  - Keep all existing Nightly campaigns and route only proven PBP-only pull requests around the CHD generator/baseline and non-PBP fuzz targets.
patterns-established:
  - Drive the production image reader through both checked replay seeds and libFuzzer.
  - Preserve existing campaign bounds and assert exact corpus counts before fuzzing.
requirements-completed: []
coverage:
  - id: D1
    description: PBP replay seeds exercise accepted raw/compressed media and malformed metadata through the production image reader.
    requirement: PBP-01
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^fuzz[.]regress[.]pbp$' --output-on-failure; exit 0; fixture preflight and replay passed"
        status: pass
    human_judgment: false
  - id: D2
    description: The sanitizer campaign consumed the exact 14-seed corpus and completed within the existing time and per-input bounds.
    requirement: PBP-01
    verification:
      - kind: integration
        ref: "Ubuntu 26.04 ARM64 Docker: schwaystation-fuzz-pbp -max_total_time=60 -timeout=20 -max_len=256; exit 0; 54,946 runs in 61 seconds, 337 edges, 845 features, 381 MB peak RSS"
        status: pass
    human_judgment: false
  - id: D3
    description: Nightly retains every prior fuzz campaign and routes isolated PBP changes through the PBP-only campaign.
    requirement: PBP-01
    verification:
      - kind: other
        ref: "disc_registration workflow gate, pin/timeout gates, 3,900/3,901 line budget, act push dry-run and classifier probes; all exit 0"
        status: pass
    human_judgment: false
duration: "44 min"
completed: 2026-11-21
status: complete
---

# Phase 10 Plan 07: PBP Replay and Fuzz Campaign Summary

**The production PBP reader now has deterministic malformed-input replay and a bounded sanitizer campaign without committed disc bytes.**

## Performance

- **Runtime/model:** Codex/OpenGSD 1.21.0; executor `gpt-6-luna` xhigh. Sanitizer build and campaign ran on native ARM64 Ubuntu 26.04 Docker; local macOS has no clang-21.
- **Started:** 2026-11-21T12:32:26Z.
- **Completed:** 2026-11-21T13:16:00Z; 44 minutes wall time.
- **Tasks:** 2.
- **Files modified:** 9.
- **Sanitizer workload:** 54,946 inputs in 61 seconds, about 900 inputs/second, 337 edges, 845 features, and 381 MB peak RSS. No crash artifacts were produced.
- **Iteration cost:** sorted inventories, replay counts, route checks, CMake wiring, and the repository line budget ran before the sanitizer campaign. The campaign was the only long check; no unrelated campaign or hosted run was repeated.

## Accomplishments

- Added 14 runtime-generated replay scripts covering accepted raw/compressed images, unsupported markers, TOC and section faults, and short, long, corrupted, truncated, and overlong compressed extents.
- Registered the same harness as a portable replay test and an ASan/UBSan libFuzzer target; accepted reads check TOC stability, repeatability, zero subcode, leadout bounds, and failure-atomic outputs.
- Added exact seed-count checks and a 60-second PBP campaign with the existing 20-second per-input bound. Full Nightly still runs every prior campaign and adds PBP as the seventh media campaign.
- Added a classifier-proven PBP-only route that skips CHD tooling/baseline construction and non-PBP fuzz targets while preserving PBP's shared disc-fixture dependencies.

## Task Commits

1. **Task 1: Replay malformed PBP corpus through the shared provider** — `735abbb`.
2. **Task 2: Add one bounded PBP sanitizer campaign** — `e2115d1`.

## Commands and Results

- The sorted fuzz and disc inventories passed (exit 0): LC_ALL=C sort -cu tests/fuzz/inventory.txt; LC_ALL=C sort -cu tests/disc/inventory.txt.
- `cmake --preset ci -DSCHWAYSTATION_CHD=OFF -DSCHWAYSTATION_BUILD_CHD_TOOL=OFF` — exit 0; refreshed the portable replay selector.
- `cmake --build --preset ci --target schwaystation-fuzz-pbp-replay schwaystation-pbp-fixtures` — exit 0.
- `ctest --preset ci --no-tests=error -R '^fuzz[.]regress[.]pbp$' --output-on-failure` — exit 0; replay and fixture preflight passed.
- `ctest --preset ci --no-tests=error -R '^disc[.]image[.]pbp_bounds$' --output-on-failure` — exit 0; bounds selector and fixture preflight passed.
- Ubuntu 26.04 ARM64 Docker direct configure used the preset's ASan/UBSan/libFuzzer flags with CHD disabled; PBP fuzzer and replay targets built successfully. The generated corpus had exactly 14 files; the sanitizer replay processed 14 in-memory and 28 total file/in-memory inputs; the one-run smoke completed 15 runs; each exited 0.
- `schwaystation-fuzz-pbp -max_total_time=60 -timeout=20 -max_len=256` — exit 0 after 54,946 runs in 61 seconds. The input cap is 256 bytes and the largest observed input was 167 bytes.
- Workflow registration, pins, timeout comments, JSON syntax, and both branch/working-tree whitespace checks (git diff --check origin/main...HEAD; git diff --check HEAD) — all exit 0. Repository checks total 3,900 of the 3,901-line ceiling.
- `act pull_request ... --dryrun` parsed the `changes` dependency and skipped `fuzz` because dry-run does not produce job outputs. `act push ... --dryrun` parsed the complete full-fuzz route and exited 0. Direct classifier probes returned `pbp_only=true` for `tests/fuzz/pbp.c` alone and `nightly_full=true` when the Nightly workflow was also changed. Act did not execute jobs or establish GitHub API permissions, artifacts, or hosted-runner behavior.
- This plan does not require hosted CI, so no hosted run was dispatched. The exact branch diff includes the workflow itself and therefore classifies as a full Nightly change; hosted critical-path time and Actions cost remain for the next required exact-head CI run.

## Files Created/Modified

- `tests/fuzz/pbp.c` — compact runtime-generated PBP mutation harness and 14 replay seeds.
- `tests/fuzz/tests.cmake`, `tests/fuzz/inventory.txt`, `tests/fuzz/seeds.h`, `CMakePresets.json` — portable replay registration, exact selector, seed cap, and sanitizer target wiring.
- `tests/disc/pbp.c` — short/long compressed-stream and corrupt final-block cases.
- `.github/workflows/nightly.yml`, `tests/cmake/workflows.cmake` — bounded campaign, PBP-only route, and ordering-aware route assertions.
- `docs/media-testing.md` — corpus, sanitizer, and routing behavior.

## Decisions Made

- Keep the corpus compact and mutate build-owned baseline files so no disc bytes or large inputs enter source control.
- Preserve the full Nightly coverage. Skip unrelated work only when the classifier proves a pull request changes PBP-only source/docs paths.
- Keep the existing 60-second campaign and 20-second per-input bounds; use a 256-byte script cap for this PBP harness.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Added the required libFuzzer entry-point prototype.**
- **Found during:** Task 1.
- **Issue:** Strict `-Wmissing-prototypes -Werror` rejected the external libFuzzer entry point.
- **Fix:** Declared `LLVMFuzzerTestOneInput` before its definition.
- **Files modified:** `tests/fuzz/pbp.c`.
- **Verification:** Portable replay and sanitizer targets built successfully.
- **Committed in:** `735abbb`.

**2. [Rule 3 - Blocking] Scoped the replay-only little-endian reader to replay builds.**
- **Found during:** Task 2 sanitizer build.
- **Issue:** The libFuzzer build rejected `get_le16` as unused because only replay seed generation needs it.
- **Fix:** Compile the helper only with `SCHWAY_FUZZ_REPLAY`.
- **Files modified:** `tests/fuzz/pbp.c`.
- **Verification:** Sanitizer targets built, seed replay and campaign passed.
- **Committed in:** `735abbb`.

**3. [Rule 1 - Test bug] Matched the Nightly guard to the new workflow order.**
- **Found during:** Task 2 preflight.
- **Issue:** The combined assertion expected the PBP-only configure after the full-campaign arrays, but the workflow configures PBP before the campaign shell branch.
- **Fix:** Reordered the guard pattern while retaining checks for all prior targets, counts, bounds, and the PBP-only branch.
- **Files modified:** `tests/cmake/workflows.cmake`.
- **Verification:** `disc_registration` passed and the counted helper total is 3,900 lines.
- **Committed in:** `e2115d1`.

**Total deviations:** 3 auto-fixed. All were build or check failures directly caused by this plan; no product scope changed.

## Issues Encountered

- Ubuntu 26.04 ships CMake 4.2.3, below the preset schema version. I configured the Docker fuzz tree directly with the exact compiler, sanitizer, inventory, fixture, and CHD flags from the preset.
- The minimal Ubuntu image lacked Git, and the mounted fuzz tree contained a nested `psxavenc` CMake cache created under the macOS path, so the initial configure/fixture attempt failed. Installing Git fixed configure; I verified the existing generated PBP fixture's SHA-256 (`e59b202ab5dadc3e9431519c02453a61031aff9dfec464bfd810e95003126f32`), copied those same bytes into the Docker fuzz tree, and built only the PBP fuzzer/replay targets. No fixture contents or build instructions were changed.
- The first `act` dry-run hit the default image-selection prompt; setting `-P ubuntu-26.04=ubuntu:26.04` made it noninteractive. A subsequent write to the default cache path was blocked by the sandbox; moving the cache to `/tmp/schwaystation-act-cache` resolved it. The final dry-run passed.

## User Setup Required

None.

## Next Phase Readiness

Plan 10-07 is complete. PBP parser replay, the PBP sanitizer campaign, Nightly routing assertions, and the repository line budget pass. PBP-01 remains phase-wide pending integration/package evidence and final Phase 10 verification. Plan 10-08 is next; no hosted CI was required by this plan.

## Self-Check: PASSED

- Summary exists at the required path and its frontmatter passes the GSD summary verifier.
- Both task commits are ancestors of `e2115d1`; the task commit count measured from `plan_head_before` is 2.
- No generated media bytes, sanitizer logs, or Docker artifacts were committed.

---
*Phase: 10-pbp-disc-images*
*Completed: 2026-11-21*
