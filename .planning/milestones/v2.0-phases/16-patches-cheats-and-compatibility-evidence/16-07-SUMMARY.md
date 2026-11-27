---
phase: 16-patches-cheats-and-compatibility-evidence
plan: "07"
subsystem: disc-patching
tags: [ppf1, raw-cue, sha256, sparse-overlay]

requires:
  - phase: 16-01
    provides: Disc-image provider, SHA-256 helper, and test infrastructure
provides:
  - Identity-bound PPF1 candidate loading and raw-sector overlay for one exact CUE mapping
  - PPF1 order, boundary, source-immutability, and failed-replacement regression coverage
  - PPF1 host compatibility contract and limits in docs/patches.md
affects: [phase-16-patch-and-cheat-consumers]

actuals:
  tokens: 14185
  tasks: 2
  commits: 3
  plan_head_before: 1e5a01c92911d39bfbf550b316ddc9789a3f767e
  plan_head_after: a526fb0feb13021cd2d11c3dff1ead62b202ee0c

tech-stack:
  added: []
  patterns:
    - Immutable patch candidate validated before attaching to an image
    - Ordered sparse sector overlay with bounded indexed reads

key-files:
  created:
    - docs/patches.md
  modified:
    - host/disc_patch.h
    - host/disc_patch.c
    - host/disc_image.h
    - host/disc_image.c
    - cmake/DiscTargets.cmake
    - tests/disc/patch.c

key-decisions:
  - Support only a one-file raw CUE with one MODE1/2352 or MODE2/2352 track at INDEX 01 and no pregap.
  - Bind candidates to the exact source instance, verify the complete source SHA-256, and apply validated changes without writing source media.
  - Keep PPF1 patch bytes and record count within the fixed 16 MiB and 262,144-record bounds.

requirements-completed: [PATCH-01]
coverage:
  - id: D1
    description: Apply identity-bound PPF1 patches to an exact raw-track mapping without modifying source media.
    requirement: PATCH-01
    verification:
      - kind: unit
        ref: "ctest --preset ci --no-tests=error -R '^(disc[.]patch[.]mapping|disc[.]patch[.]ppf1|disc[.]patch[.]atomic)$' --output-on-failure"
        status: pass
    human_judgment: false

duration: 34min
completed: 2026-11-26
status: complete
---

# Phase 16 Plan 07: Apply identity-bound PPF1 patches to an exact raw-track mapping Summary

**PPF1 candidates now validate and overlay an explicitly mapped raw CUE track, with source identity, digest, limits, and failed replacement isolation enforced.**

## Performance

- **Duration:** 34 minutes
- **Started:** 2026-11-26T00:49:40Z
- **Completed:** 2026-11-26T01:23:48Z
- **Tasks:** 2
- **Files modified:** 7
- **Executor model:** gpt-6-luna

## Accomplishments

- Added a frozen host patch API and PPF1 parser that checks the format header, whole-source SHA-256, record bounds, and patch limits before producing an immutable candidate.
- Mapped only a single-file raw CUE track at INDEX 01; candidate attachment requires the same image/source identity, and reads apply a bounded sparse overlay without source writes.
- Added generated-media tests for both raw sector modes, boundary and cross-sector records, ordered overlaps, repeat reads, invalid late records, wrong digests, and failed replacement; documented the supported contract.

## Task Commits

1. **Task 1 RED: raw mapping and PPF1 cases** - `6ad4214` (`test`)
2. **Task 1 GREEN: identity-bound PPF1 overlays** - `06561a5` (`feat`)
3. **Task 2: overlap order, replacement isolation, and docs** - `a526fb0` (`test`)

## Files Created/Modified

- `host/disc_patch.h` and `host/disc_patch.c` - Candidate API, PPF1 validation, record ordering, and sparse overlay.
- `host/disc_image.h` and `host/disc_image.c` - Exact raw CUE source mapping and identity-checked patch replacement.
- `cmake/DiscTargets.cmake` - Patch provider source and SHA-256 helper linkage.
- `tests/disc/patch.c` - Mapping, parser, overlay, immutability, and replacement regression tests.
- `docs/patches.md` - Supported format subset, limits, mapping, and digest requirements.

## Checks

- Task 1 RED: `ctest --preset ci --no-tests=error --output-on-failure --output-junit .gsd/tdd-evidence/16-07/task1-red.xml -R '^(disc[.]patch[.]mapping|disc[.]patch[.]ppf1)$'` — exit 8 as intended; both planned assertions failed before implementation. CTest-reported test time was 0.02 s. Evidence is retained under ignored `.gsd/tdd-evidence/16-07/`.
- Inventory and configure: `git status --short && git diff --check && LC_ALL=C sort -cu tests/disc/inventory.txt && cmake --preset ci` — exit 0, 0.86 s.
- Final plan verification: `set -o pipefail && git diff --check HEAD && cmake --preset ci && ctest --preset ci --show-only=json-v1 -R '^(disc[.]patch[.]mapping|disc[.]patch[.]ppf1|disc[.]patch[.]atomic)$' | python3 -c 'import json,sys; names=[t["name"] for t in json.load(sys.stdin)["tests"]]; required=set("disc.patch.mapping disc.patch.ppf1 disc.patch.atomic".split()); missing=required-set(names); print("missing selectors:",sorted(missing)); sys.exit(bool(missing) or len(names)!=len(set(names)))' && cmake --build --preset ci --target schwaystation-test-disc-patch && ctest --preset ci --no-tests=error -R '^(disc[.]patch[.]mapping|disc[.]patch[.]ppf1|disc[.]patch[.]atomic)$' --output-on-failure` — exit 0, 0.83 s; all three tests passed and all selectors were unique and present.
- Consumer build: `cmake --build --preset ci --target schwaystation-run schwaystation_libretro` — exit 0, 0.18 s.
- Disc-image regression: `cmake --build --preset ci --target schwaystation-test-disc-image && ctest --preset ci --no-tests=error -R '^disc[.]image[.]iso_sector$' --output-on-failure` — exit 0; CTest reported 0.02 s and passed.
- Task 2 TDD probe: `ctest --preset ci --no-tests=error --output-on-failure --output-junit .gsd/tdd-evidence/16-07/task2-red.xml -R '^disc[.]patch[.]atomic$'` — exit 0, CTest-reported test time 0.02 s. The classifier marked this `unexpected_green`; source inspection confirmed the Task 1 implementation already guarantees ordered overlap winners and rejects candidates bound to another image. The regression test was retained.

The phase orchestrator owns code review and the broad `cmake --workflow --preset ci` run on reviewed source.

## TDD Gate Compliance

Task 1 had valid RED evidence and then passed its mapping and PPF1 selectors after implementation, including the tracer feedback gate before Task 2. Task 2's pre-implementation test passed because its behavior was already delivered in Task 1; the machine RED classifier correctly rejected it as an `unexpected_green`. No production change was needed for Task 2; the test remains as regression coverage.

## Decisions Made

- Restrict raw-track mapping to one unambiguous raw CUE layout, rejecting ISO/2048, extra tracks, indexes, and pregaps.
- Require complete-source identity and SHA-256 validation before attachment, then replace candidates atomically so a failed replacement preserves the active reader and overlay.
- Store resolved ordered record winners as sparse sector segments to keep reads bounded independently of total patch-record count.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking issue] Removed a const-discarding image adapter cast**
- **Found during:** Task 1 implementation build.
- **Issue:** The focused build treated the adapter's const cast as a warning, blocking compilation.
- **Fix:** Changed the adapter to accept the mutable image instance required by the host callback contract.
- **Files modified:** `host/disc_image.h`, `host/disc_image.c`
- **Verification:** Focused target build and patch selectors passed.
- **Committed in:** `06561a5`.

**2. [Rule 1 - Test bug] Prevented a test double-close**
- **Found during:** Task 1 mapping test.
- **Issue:** The test closed the MODE1 image twice, causing a trap unrelated to patch behavior.
- **Fix:** Cleared the test's image handle after closing it.
- **Files modified:** `tests/disc/patch.c`
- **Verification:** Mapping and PPF1 selectors passed.
- **Committed in:** `06561a5`.

**3. Task 2's planned RED case was already green**
- **Found during:** Task 2 TDD probe.
- **Issue:** Task 1's ordered overlay and exact image identity checks already covered Task 2's behavior.
- **Resolution:** Kept the new atomicity regression test, made no redundant production edit, and recorded the unexpected-green evidence.
- **Files modified:** `tests/disc/patch.c`, `docs/patches.md`
- **Committed in:** `a526fb0`.

**Total deviations:** 2 auto-fixed issues and 1 task-overlap adjustment. No scope expansion.

## Issues Encountered

- An extra preflight of the existing fixture manifest sort check found its current entries out of order. `tests/fixtures/manifest.txt` is outside this plan's owned files and was left unchanged; the plan's disc inventory sort check passed.
- An early exploratory `disc.image` selector run reported `Not Run` for test executables that had not been built. Building the relevant target and running `disc.image.iso_sector` passed.

## User Setup Required

None.

## Next Phase Readiness

The PPF1 host interface, exact mapping, and regression tests are ready for downstream patch consumers. Review and broad workflow verification remain with the phase orchestrator on the reviewed head.

## Self-Check: PASSED

- The summary file exists at the required phase path.
- All three task commits (`6ad4214`, `06561a5`, `a526fb0`) are ancestors of the current worktree HEAD.
- `git diff --check` passed.

---
*Phase: 16-patches-cheats-and-compatibility-evidence*
*Plan: 07*
*Completed: 2026-11-26*
