---
phase: 09-chd-disc-images
plan: 13
subsystem: testing
tags: [CHD, CMake, CPack, CTest, release]
requires:
  - phase: 09-09
    provides: Linked-provider metadata claims and exact CHD dependency notices
provides:
  - CHD ON and OFF package checks for archive contents, notices and provider-derived metadata
  - A negative control proving packaged metadata rejects a false license claim
affects: [Phase 9 Plan 09-11, release validation]
actuals:
  tokens: 1318
  tasks: 2
  commits: 3
tech-stack:
  added: []
  patterns:
    - Compare packaged libretro metadata against the configured build-tree claim.
    - Run CHD ON and OFF package checks sequentially when they share a build directory.
key-files:
  created:
    - .planning/phases/09-chd-disc-images/09-13-SUMMARY.md
  modified:
    - tests/release/tests.cmake
    - tests/release/inventory.txt
    - .planning/phases/09-chd-disc-images/09-13-PLAN.md
    - .planning/STATE.md
key-decisions:
  - Keep the repository-check budget at 3900/3901 lines.
  - Defer the generic source-wide SPDX scanner to its first tracked SPDX-bearing caller; todo 005 remains open for Phase 10.
patterns-established:
  - Release checks verify configuration-specific metadata from the actual package.
requirements-completed:
  - CHD-01
coverage:
  - id: CHD-ON-OFF-PACKAGES
    description: CHD-enabled and CHD-disabled packages preserve their component archive names and pass exact content, notice and provider-matched metadata checks.
    requirement: CHD-01
    verification:
      - kind: integration
        ref: "The ON and OFF configure, claim, notice, build, CPack and release-selector commands listed in this summary exited 0; the archive and metadata selectors passed 4/4 for each configuration."
        status: pass
    human_judgment: false
duration: "not measured"
completed: 2026-11-20
status: complete
---

# Phase 9 Plan 13: CHD package validation Summary

**CHD-enabled and disabled package archives now verify their contents, exact notices and configuration-specific libretro metadata.**

## Performance

- **Duration:** Not measured; the ON-to-OFF transition and active time were not timed.
- **Tasks:** 2
- **Files changed by the implementation task:** 2
- **Observed iteration cost:** The ON rebuild ran 229 Ninja actions; after switching to OFF, the incremental rebuild ran 27. The final release checks took 0.10 seconds for OFF. No broad test suite or hosted run was repeated.

## Accomplishments

- Added `release.notices` to extract packaged libretro metadata and compare its extensions and license with the configured build-tree claim.
- Added a synthetic false-license control and kept the exact archive contents and required notice-byte checks.
- Configured, rebuilt, packaged and checked CHD ON, then repeated the sequence with CHD OFF in the same build directory.
- CPack produced the same three local component archive names in both configurations: `schwaystation-library-macos-arm64.zip`, `schwaystation-libretro-macos-arm64.zip` and `schwaystation-runner-macos-arm64.zip`.
- Kept the complete repository-check total at 3900/3901 lines. Todo 005 remains open for the deferred source-wide SPDX scanner.

## Verification

| Configuration | Checks | Exit | Result |
|---|---|---:|---|
| ON | `cmake --preset ci -DSCHWAYSTATION_CHD=ON`; libretro claims under `ci`; policy notices under `hygiene`; build and CPack; `release.(archive_contents|notices)` with self-tests | 0 | Claims 2/2, notices 2/2, release selectors 4/4. |
| OFF | `cmake --preset ci -DSCHWAYSTATION_CHD=OFF`; same claims, notice, build, CPack and release selectors; both diff checks | 0 | Claims 2/2, notices 2/2, release selectors 4/4. The packaged metadata omits CHD and matches the OFF provider closure. |
| Final consistency | `LC_ALL=C sort -cu tests/release/inventory.txt`; line-budget check; `git diff --check origin/main...HEAD`; `git diff --check HEAD` | 0 | Inventory sorted; budget 3900/3901; no whitespace errors. |

The ON outputs were recorded in STATE before the OFF configure replaced them. CPack generated three local macOS arm64 component archives per configuration. No hosted URL or `act` run applies: this plan changed no workflow; Plan 09-11 retains the routing dry run and CHD producer critical-path measurement before hosted CI.

## Task Commits

1. **Task 1: Verify CHD ON archives** — `d275d99`.
2. **Task 2: Verify CHD OFF archives and close the plan** — package checks passed; summary and state closeout are committed with this artifact.

The ON checkpoint and selector correction were recorded in `5edcbd1` before the OFF leg.

## Deviations and Issues

The plan selectors were corrected after CTest listing showed that `policy.notices` is registered by the `hygiene` preset, while `libretro.info.claims` is registered by `ci`. The original combined preset selector would not have run the notice tests. No acceptance case or package was removed.

## Next Plan Readiness

Plan 09-13 is complete. Plan 09-11 can now add the distributed workflow/fuzz wiring, with `act --dryrun` for routing and measured CHD producer timing before an expensive hosted run. No owner action is needed.

---
*Phase: 09-chd-disc-images*
*Completed: 2026-11-20*
