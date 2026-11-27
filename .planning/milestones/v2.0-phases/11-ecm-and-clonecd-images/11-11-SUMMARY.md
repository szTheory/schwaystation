---
phase: 11-ecm-and-clonecd-images
plan: "11"
subsystem: disc-image
tags: [ecm, clonecd, guest, compatibility]
requires:
  - phase: 11-07
    provides: deterministic ECM/CCD fixtures, source-count preflight, parity and reader comparisons
  - phase: 11-08
    provides: runner probe and bounded guest evidence for legacy media
  - phase: 11-09
    provides: libretro legacy content and extension evidence
  - phase: 11-10
    provides: bounded ECM/CCD replay targets and sanitizer campaign build evidence
provides:
  - bounded ECM and CCD guest selectors for fixture-supported media layouts
  - protected ECM and CCD compatibility rows with explicit local evidence boundaries
  - documentation for owned fixture coverage, source counts and accepted format subsets
affects: [phase-11-verification, disc-image, compatibility-ledger]
actuals:
  tokens: 2749.75
  tasks: 2
  commits: 3
plan_head_before: fb681fcefc510d874a71758ec58436532a58655f
plan_head_after: 7226071cd859d79338859da3a19600b446eb0d18
tech-stack:
  added: []
  patterns:
    - Reuse bounded guest predicates through the shared provider after independent source-count and parity fixtures.
    - Keep feature claims local and fixture-specific while hosted, package and fuzz evidence remains unknown.
key-files:
  created: []
  modified:
    - tests/disc/fixture.c
    - tests/disc/tests.cmake
    - tests/disc/inventory.txt
    - tests/compat/features.csv
    - tests/compat/tests.cmake
    - docs/media-testing.md
    - docs/compatibility.md
    - docs/discs.md
key-decisions:
  - The owned ECM fixture has one data track, so its guest predicates cover boot, read/replay and FMV, with no CD-DA claim.
  - Feature rows include only the measured local provider, runner, libretro and guest evidence; hosted CI, package and fuzz remain unknown until Plan 12.
requirements-completed: []
coverage:
  - id: D1
    description: Owned ECM and mixed-mode CloneCD media pass bounded guest boot, read, supported audio and FMV predicates after source preflight and parity.
    requirement: ECM-01
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^disc[.]image[.]legacy_|^disc[.]fixture[.](ecm|ccd)[.]' --output-on-failure; exit 0; 12/12"
        status: pass
    human_judgment: false
  - id: D2
    description: ECM and CCD feature rows are protected, sorted and limited to actual local evidence, with unavailable hosted, package and fuzz claims left unknown.
    requirement: CCD-01
    verification:
      - kind: other
        ref: "ctest --preset ci --no-tests=error -R '^compat[.]features[.]' --output-on-failure; exit 0; 2/2"
        status: pass
    human_judgment: false
duration: 23m27s
completed: 2026-11-22
status: complete
---

# Phase 11 Plan 11: ECM and CloneCD guest evidence Summary

**Fixture-backed ECM and CloneCD guests now have bounded route coverage and compatibility evidence that stops at verified local behavior.**

## Performance

- **Duration:** 23m27s through the final focused checks.
- **Started:** 2026-11-22T18:30:57Z.
- **Final verification completed:** 2026-11-22T18:54:24Z.
- **Tasks:** 2.
- **Files modified:** 8.
- **Measured diff:** 10,999 characters, or 2,749.75 estimate tokens; three task commits.
- **Runtime/model:** Codex with OpenGSD 1.20.0; executor configured as `gpt-6-luna` xhigh. The primary runtime model identifier was not exposed.
- **Bottleneck:** Two stale ignored test executables required focused rebuilds before the final guest selector; no product defect remained.

## Accomplishments

- Registered seven bounded guest aliases: ECM boot/read/FMV and mixed CCD boot/read/audio/FMV. Existing cycle, frame, status, output and replay assertions remain in use.
- Confirmed source counts before guest execution: ECM has 64 source sectors, requests and outputs; CCD has 64 data plus 158 audio sectors (222 total), 222 requests/outputs and 21,312 SUB bytes.
- Added protected ECM and CCD feature rows grounded in provider parity, runner, libretro and guest tests. Hosted CI, package and fuzz evidence remains explicitly unknown.
- Documented the supported format boundaries, bounded media work, optional SUB behavior, fixture applicability and exact focused guest command.

## Verification Evidence

- `LC_ALL=C sort -cu tests/disc/inventory.txt` — exit 0.
- `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` — exit 0 at 3,899/3,901 lines.
- `ctest --preset ci --no-tests=error -R '^disc[.]image[.]legacy_(preflight|parity)$' --output-on-failure -V` — exit 0, 2/2; ECM and CCD expected source counts were 64 and 222 sectors.
- `cmake --preset ci -DSCHWAYSTATION_CHD=OFF -DSCHWAYSTATION_BUILD_CHD_TOOL=OFF` — exit 0.
- Final guest selector `ctest --preset ci --no-tests=error -R '^disc[.]image[.]legacy_|^disc[.]fixture[.](ecm|ccd)[.]' --output-on-failure` — exit 0, 12/12 in 13.36 seconds, including all seven new guests, source preflight/parity, media preflight and existing FMV predicate setup.
- `cmake -DFEATURES=tests/compat/features.csv -DMODE=features-preflight -P tests/cmake/compat-ratchet.cmake` — exit 0.
- `sh scripts/compat-doc.sh tests/compat/ledger.csv docs/compatibility.md` — exit 0; compatibility docs regenerated from the ledgers.
- `cmake --preset ci -DSCHWAYSTATION_CHD=OFF -DSCHWAYSTATION_BUILD_CHD_TOOL=OFF` — exit 0; `ctest --preset ci --no-tests=error -R '^compat[.]features[.]' --output-on-failure` — exit 0, 2/2 in 0.53 seconds.
- `git diff --check HEAD` — exit 0. No hosted CI, broad suite, Docker or owner-hardware run was used for this plan.

## Task Commits

1. **Task 1 registration / RED:** `2193ce9` — register ECM and CCD guest selectors.
2. **Task 1 GREEN:** `53fc4e4` — use the ECM track count in guest predicates.
3. **Task 2:** `7226071` — protect ECM and CCD evidence.

**Plan metadata:** committed separately after summary and state updates.

## TDD Gate Compliance

- The first ECM guest attempt used an outdated ignored fixture executable and returned the old unsupported-format message. That was stale build output, not valid RED evidence; the fixture target was rebuilt before continuing.
- The refreshed `disc.fixture.ecm.read` test then failed on its intended assertion, `unexpected TOC track count`, because the shared predicate assumed two tracks. The focused report was accepted by `gsd_run check tdd-red-evidence` as `RED_EVIDENCE_OK`.
- The GREEN change derives a one-track expectation for the owned direct ECM fixture while preserving the existing PBP and two-track expectations. The focused ECM test passed 4/4 including its setup chain.
- The first combined guest selector found a stale ignored `schwaystation-test-disc-media` prerequisite. Rebuilding that target made its preflight pass; the final exact selector then passed 12/12. Task 2 changed evidence and docs only, so no artificial RED was created.

## Deviations from Plan

None. No guest case, assertion, inventory entry or feature claim was removed or weakened.

## Issues Encountered

Two inherited ignored build artifacts were stale. Each was diagnosed from its focused selector and rebuilt by target; the refreshed final guest selector and compatibility checks passed. No remaining implementation blocker was found.

## Next Plan Readiness

Plan 11-12 is next. This plan establishes local fixture, runner and libretro evidence only; hosted CI, package and fuzz evidence remains unknown until Plan 12, and phase-level code review and final verification remain pending. ECM-01 and CCD-01 are not marked complete here.

---
*Phase: 11-ecm-and-clonecd-images*
*Completed: 2026-11-22*

## Self-Check: PASSED

- Summary exists and records the measured three task commits; each is an ancestor of the plan head.
- Plan head matches `7226071cd859d79338859da3a19600b446eb0d18`; no personal filesystem paths or generated artifacts are included.
- Product and documentation changes passed their focused checks and whitespace validation before closeout.
