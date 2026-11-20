---
phase: 09-chd-disc-images
plan: 07
subsystem: media
tags:
  - chd
  - disc-image
  - fuzz
  - limits
requires:
  - phase: 09-06
    provides: Shared disc-image provider with CHD sector reads and CUE parity oracle
provides:
  - Shared checked media arithmetic and documented allocation/count limits for CUE and CHD
  - Production-parser CHD replay with deterministic outcomes and failure-atomic replacement checks
  - A bounded ASan/UBSan CHD campaign with reproducible build-local corpus and crash paths
affects:
  - Phase 9 media safety and fuzz lanes
  - Phase 9 Plan 11 hosted campaign wiring
actuals:
  tokens: 15307
  tasks: 3
  commits: 4
plan_head_before: 2298b4adc8ed3eed49be330ff568160c8727f4bc
plan_head_after: a6271c201fa096cef3aa9ef106bcfdeb849de9c9
commits: 4
tech-stack:
  added: []
  patterns:
    - Keep checked media arithmetic in one private header shared by format readers.
    - Exercise the real provider with generated hostile inputs in a fixed build-owned directory.
key-files:
  created:
    - host/media_limits.h
    - tests/fuzz/chd.c
  modified:
    - host/disc_chd.c
    - host/disc_chd_metadata.c
    - host/disc_image.c
    - docs/discs.md
    - tests/disc/chd.c
    - tests/disc/image.c
    - tests/fuzz/seeds.h
    - tests/fuzz/tests.cmake
    - tests/fuzz/inventory.txt
    - CMakePresets.json
    - .planning/phases/09-chd-disc-images/09-07-PLAN.md
key-decisions:
  - Keep the existing 1 GiB image limit and use checked arithmetic before decode, allocation, or offset narrowing.
  - Treat a stable lazy hunk decode failure as a bounded read failure for an accepted CHD; successful repeated reads must still match.
  - Keep fixture, corpus, smoke copy, and libFuzzer artifacts inside the build tree and pass one corpus directory to each invocation.
requirements-completed:
  - MEDIA-02
  - MEDIA-01
  - CHD-01
coverage:
  - id: MEDIA-CHECKED-LIMITS
    description: Shared CUE and CHD limits reject malformed counts, extents, offsets, and allocation totals before use.
    requirement: MEDIA-02
    verification:
      kind: integration
      ref: "CHD-on disc.image selectors passed 16/16, including CUE limits/confinement, CHD boundaries/parity, and disc.media.preflight; exit 0."
      status: pass
    human_judgment: false
  - id: CHD-FAILURE-ATOMIC-REPLAY
    description: Malformed CHD seeds run through the production provider, preserve retained media after rejected opens, and keep TOC/read results bounded and deterministic.
    requirement: MEDIA-01
    verification:
      kind: integration
      ref: "ctest --preset ci --no-tests=error -R '^fuzz[.]regress[.]chd$' passed 2/2 with CHD on and 1/1 with CHD off; exit 0 in both configurations."
      status: pass
    human_judgment: false
  - id: CHD-SANITIZER-CAMPAIGN
    description: A bounded sanitizer campaign exercises the production CHD parser against textual seeds and the owned generated fixture.
    requirement: CHD-01
    verification:
      kind: integration
      ref: "The copied-corpus -runs=1 smoke passed 13 inputs; the 30-second ASan/UBSan campaign exited 0 after 31 seconds and 47,207 runs, with 701 edges and 1,304 features."
      status: pass
    human_judgment: false
duration: 2h52m
completed: 2026-11-20
status: complete
---

# Phase 9 Plan 09-07 Summary

**CUE and CHD now share checked media bounds, and the production CHD parser has portable replay plus a bounded sanitizer campaign.**

## Performance

- **Duration:** 2 hours 52 minutes of wall time from the preceding saved checkpoint at 09:54:00Z to closeout at 12:46:09Z; active time was not measured.
- **Runtime/model:** Codex/OpenGSD 1.20.0; typed executor `gpt-6-luna` at xhigh. The exact primary model ID is not exposed.
- **Tasks:** 3. **Plan commits measured from the recorded baseline:** 4. **Files changed:** 13.
- The baseline is `2298b4a`, after three plan-scope edits were already committed: `860e0be`, `3d08a98`, and `2298b4a`. The measured commits after that baseline are `b6205e7`, `5e5915b`, `57d4579`, and `a6271c2`.

## Accomplishments

- Extracted shared checked addition, multiplication, count, offset, and image-size limits into `host/media_limits.h`; applied them to CUE and CHD parsing before decoding, allocation, or narrowing offsets. Companion-path confinement and symlink checks remain in the host reader.
- Added `tests/fuzz/chd.c`, which drives the production parser from a fixed build-owned directory. It checks stable TOCs, deterministic successful sector reads, bounded leadout behavior, failure-atomic rejected opens, and CHD-off unsupported results.
- Added malformed seeds for truncated headers, parent references, exact and over-limit hunk/map/image sizes, bad metadata offsets, and arithmetic overflow. The campaign also produced an `unreadable_hunk` seed by changing one byte in the generated fixture; no disc bytes were committed.
- Made the sanitizer smoke copy and campaign corpus independent, reset their build-local directories, and kept crash artifacts under `build/fuzz/crashes/`.

## Verification and Efficiency

- Cheap preflights passed before the campaign: `git diff --check`, `CMakePresets.json` parsing, sorted fuzz inventory validation, and the generated fixture existence/hash check. The owned fixture is 5,552 bytes with SHA-256 `4952c3a05ff2e2e596deed7ed2d37358a646790b890d5ed38484feec5b797569`.
- CHD-on replay verification passed: configure with `cmake --preset ci -DSCHWAYSTATION_CHD=ON -DSCHWAYSTATION_BUILD_CHD_TOOL=ON`, build the fixture/media/replay targets, then run `ctest --preset ci --no-tests=error -R '^fuzz[.]regress[.]chd$' --output-on-failure`; 2/2 passed, including `disc.media.preflight`.
- CHD-off replay verification passed after configuring with `-DSCHWAYSTATION_CHD=OFF -DSCHWAYSTATION_BUILD_CHD_TOOL=OFF` and building the media/replay targets; the same CTest selector passed 1/1.
- The focused CHD-on image slice passed 16/16 in 0.03 seconds. It covered the CUE and ISO limit/confinement selectors, CHD header/metadata/TOC/open/boundary/parity checks, and the media preflight. The corrected plan command now explicitly configures CHD on and builds `schwaystation-test-disc-media` plus `schwaystation-chd-fixtures` before running selectors. That avoids stale mode-specific test binaries and missing generated fixtures when a build directory is fresh or toggled.
- Homebrew macOS ASan hung before `main` in a minimal probe. The campaign therefore ran in native ARM64 Ubuntu 26.04 Docker with Clang 21. Its packaged CMake was 4.2.3 while the repository preset requires 4.3 and schema 11, so the sanitizer cache was configured directly with the preset's flags and CHD settings; `CMakeUserPresets.json` was untouched. A one-iteration smoke run on a copy of the 13-input corpus passed before the 30-second campaign. The campaign exited 0 after 31 seconds and 47,207 runs, reaching 701 edges and 1,304 features at about 383 MB peak RSS.
- The first campaign exposed a harness assumption: a one-byte-mutated CHD can be accepted with a stable TOC while a compressed hunk fails on repeated lazy reads. The harness now allows stable read failure, compares repeated output when reads succeed, validates the TOC, and rejects leadout reads. The regression seed replays in both enabled and disabled builds.
- No GitHub Actions run or `act` run was needed: this plan did not modify a hosted workflow. Local selectors covered the changed parser paths, and the sanitizer campaign was local. Local runs have no hosted URL.
- No generated Docker, build, corpus, crash, or disc data was added to the repository. The implementation and plan commits left the code tree clean before this summary/state closeout.

## Task Commits

1. **Share checked media limits** — `b6205e7` (`fix(09-07): share checked media limits`)
2. **Replay hostile CHD inputs** — `5e5915b` (`test(09-07): replay hostile CHD inputs`)
3. **Record bounded campaign setup** — `57d4579` (`docs(09-07): record bounded CHD fuzz setup`)
4. **Make parser preflight reproducible** — `a6271c2` (`docs(09-07): make parser preflight reproducible`)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Harness assumption] Allowed stable lazy hunk read failures for malformed accepted CHDs**
- **Found during:** Task 3 sanitizer campaign.
- **Issue:** The generated one-byte mutation passed CHD parsing and exposed a stable TOC, but its first compressed hunk could not decode. Treating every accepted file as guaranteed to decode its first sector caused a harness false positive.
- **Fix:** Validate stable TOC/read behavior; permit repeated bounded read failure and compare sector/subcode bytes when a read succeeds. Add `unreadable_hunk` as a deterministic textual regression seed.
- **Files modified:** `tests/fuzz/chd.c`.
- **Commit:** `5e5915b`.

**2. [Rule 3 - Verification setup] Made the image selector command build its generated fixture and mode-specific preflight binary**
- **Found during:** Plan closeout verification.
- **Issue:** The first selector attempt used an old CHD-off media test binary and lacked the generated CHD fixture; its preflight failed before four dependent tests could run. CUE/ISO selectors passed.
- **Fix:** Built the existing fixture and media test targets, then reran the focused suite successfully. Updated the plan command so clean or toggled build directories create the correct prerequisites before CTest.
- **Files modified:** `.planning/phases/09-chd-disc-images/09-07-PLAN.md`.
- **Commit:** `a6271c2`.

**3. [Rule 3 - Local toolchain] Ran the sanitizer campaign in Linux Docker**
- **Found during:** Task 3.
- **Issue:** Homebrew macOS ASan stalled before `main`, and the available Ubuntu CMake was below the preset's minimum version.
- **Fix:** Used native ARM64 Ubuntu 26.04 with Clang 21 and directly configured the fuzz build with the preset's sanitizer flags and CHD-on settings. Built only the CHD replay/campaign dependency closure; the production parser remained under ASan/UBSan.
- **Files modified:** `.planning/phases/09-chd-disc-images/09-07-PLAN.md`.
- **Commit:** `57d4579`.

## Requirements and Phase Boundary

This plan completes its MEDIA-01, MEDIA-02, and CHD-01 obligations. The phase-wide requirements remain pending until the remaining Phase 9 plans complete feature-ledger, packaging, lane, and release obligations.

## Next Plan Readiness

Plan 09-07 is complete. Continue with the next ready Phase 9 plan. Plan 11 can wire the recurring hosted campaign using the proven CHD replay and bounded campaign targets.

## Self-Check: PASSED

- `09-07-SUMMARY.md` exists.
- All four measured plan commits are ancestors of the current phase branch.
