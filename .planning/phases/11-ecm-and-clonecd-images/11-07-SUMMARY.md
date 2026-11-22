---
phase: 11
plan: "07"
subsystem: testing
tags: [disc-images, ECM, CloneCD, CMake, CTest, fixture-generation]
requires:
  - phase: 11 plans 05 and 06
    provides: bounded ECM/CCD readers and owned raw source fixtures
provides:
  - deterministic literal and modeled ECM fixtures with source-count manifests
  - deterministic CloneCD IMG, CCD, SUB, and image-backed CUE fixtures
  - source preflight, malformed-input controls, and full reader parity checks
affects: [Phase 11 plans 08-12]
actuals:
  tokens: 12457
  tasks: 2
  commits: 4
plan_head_before: a8a34c2c8dd9e515488244f81710e532b77d0c17
plan_head_after: 14bd092e345e17dd8fc89d450e28aa7c3da3aa99
tech-stack:
  added: []
  patterns:
    - deterministic standard-library fixture generators over owned raw sources
    - CTest setup fixtures gate later disc consumers on source preflight and parity
key-files:
  created:
    - tools/gen_ecm.py
    - tools/gen_ccd.py
    - cmake/LegacyDiscFixtures.cmake
    - tests/disc/legacy.c
  modified:
    - cmake/DiscFixtures.cmake
    - tests/disc/tests.cmake
    - tests/disc/inventory.txt
    - tests/fixtures/manifest.txt
key-decisions:
  - Generate ECM and CloneCD fixtures only from the repository-owned raw sources and record exact source/output digests and extents.
  - Compare save and content identity through the IMG-backed CUE view, while checking every CCD SUB byte independently.
requirements-completed: []
coverage:
  - id: D1
    description: Owned ECM fixtures are preflighted and their decoded source sectors, geometry, and identity are checked.
    requirement: ECM-01
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^disc[.]image[.]legacy_(preflight|parity)$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: Owned CloneCD fixtures are preflighted and their full sectors, TOC, SUB data, and identity are checked.
    requirement: CCD-01
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^disc[.]image[.]legacy_(preflight|parity)$' --output-on-failure"
        status: pass
    human_judgment: false
duration: 48m17s
completed: 2026-11-22
status: complete
---

# Phase 11 Plan 07: Deterministic ECM and CloneCD fixture parity

**Deterministic ECM and CloneCD fixtures now have source-bound preflight, malformed-input controls, and complete reader parity checks.**

## Performance

- **Duration:** 48m17s
- **Started:** 2026-11-22T15:07:34Z
- **Completed:** 2026-11-22T15:55:51Z
- **Tasks:** 2
- **Files modified:** 8
- **Runtime/model:** Codex/OpenGSD 1.20.0; executor model `gpt-6-luna`, xhigh effort. A more specific runtime model ID was not exposed.
- **Measured work:** 49,827 changed diff bytes (12,457 chars/4 tokens), 4 task commits. The investigation and correction of the save-identity comparison was the main time cost; implementation scope took longer than the 30-minute plan target.

## Accomplishments

- Added deterministic standard-library ECM and CloneCD producers from the existing owned raw sources. Generated media remains in the build tree and is not committed.
- Added source-bound manifests and clean-build output dependencies, including the CCD IMG-backed CUE used for identity comparison.
- Added five cheap negative controls for source extent, missing companion, manifest count, requested count, and generator digest.
- Added full-sector parity for literal/modelled ECM and CloneCD, including forward, reverse, repeated, and interleaved reads; checked TOC/index/pregap geometry and every decoded SUB byte.
- Gated other disc tests on the legacy preflight and parity setup fixtures.

## Verification Evidence

- `cmake --preset ci -DSCHWAYSTATION_CHD=OFF -DSCHWAYSTATION_BUILD_CHD_TOOL=OFF` — exit 0.
- `cmake --build --preset ci --target schwaystation-legacy-disc-fixtures` — exit 0.
- `cmake --build --preset ci --target schwaystation-test-disc-legacy` — exit 0.
- After the final generator cleanup, `cmake --build --preset ci --target schwaystation-test-disc-legacy schwaystation-legacy-disc-fixtures` — exit 0.
- `ctest --preset ci --no-tests=error -R '^disc[.]image[.]legacy_(preflight|parity)$' --output-on-failure` — exit 0, 2/2 passed; latest run took 0.14s.
- `LC_ALL=C sort -cu tests/disc/inventory.txt` — exit 0; line budget — 3,898/3,901; `git diff --check` — exit 0; generator SHA-256 matched the fixture policy manifest.
- The generated ECM source contains 64 sectors. The generated mixed-mode image contains 222 sectors (64 data plus 158 audio), and its SUB companion contains 21,312 bytes.
- No guest, broad suite, Docker, or hosted CI run was needed for this plan. These local checks do not complete the phase requirements.

## Task Commits

1. **Task 1: Generate both providers from owned uncompressed sources** — `64668c1` (`feat`).
2. **Task 2 test slice: Add legacy preflight and parity checks** — `1eb224d` (`test`).
3. **Task 2 producer slice: Emit the IMG identity CUE in the declared build graph** — `cbefda7` (`feat`).
4. **Review cleanup: Remove an unused CUE formatter value and refresh its source digest** — `14bd092` (`refactor`).

**Plan metadata commit:** created after this summary and state update.

## TDD Gate Compliance

- The earlier failing save-identity comparison was machine-classified as `RED_EVIDENCE_OK`, but semantic review rejected it: the source CUE had no SUB data while the CloneCD image intentionally had generated SUB bytes, which participate in save identity. That failure tested an invalid oracle, not the planned parity behavior.
- The comparison was corrected to use the generated IMG-backed CUE for save/content identity; `.ccd` parity still checks all sectors and SUB bytes independently. The corrected focused selector passed 2/2.
- No valid product RED was recorded before the producer update. The tests characterize readers already implemented by predecessor plans, and this summary does not claim canonical RED evidence. The test slice was committed before the CUE producer slice; no output deletion or history rewrite was used to simulate a failure.

## Files Created/Modified

- `tools/gen_ecm.py` and `tools/gen_ccd.py` create bounded deterministic fixture representations and manifests from owned source bytes.
- `cmake/LegacyDiscFixtures.cmake` and `cmake/DiscFixtures.cmake` declare the producer graph and outputs.
- `tests/disc/legacy.c`, `tests/disc/tests.cmake`, and `tests/disc/inventory.txt` implement and register source preflight, negative controls, and provider parity.
- `tests/fixtures/manifest.txt` records the first-party generator provenance and digest.

## Decisions Made

- Keep the existing raw sources as the only generation inputs so parity has a shared source of truth.
- Compare save/content identity using an IMG-backed CUE without synthetic SUB, and verify CloneCD SUB through an independent byte-level parity path.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Corrected the save-identity test oracle for generated SUB data**
- **Found during:** Task 2 (provider parity)
- **Issue:** Comparing source CUE identity directly to a CCD provider compared different SUB bases.
- **Fix:** Added a CUE view over the generated IMG for save/content identity checks; retained complete CCD sector and SUB comparisons.
- **Files modified:** `tools/gen_ccd.py`, `cmake/LegacyDiscFixtures.cmake`, `tests/disc/legacy.c`, `tests/fixtures/manifest.txt`
- **Verification:** Focused legacy preflight/parity selector passed 2/2.
- **Committed in:** `1eb224d`, `cbefda7`.

**2. [Rule 1 - Cleanup] Removed an unused CUE formatter local during final source review**
- **Found during:** Final source review
- **Issue:** The IMG-backed CUE formatter computed a leadout value it did not emit.
- **Fix:** Removed the unused local and refreshed the generator digest in the fixture policy manifest.
- **Files modified:** `tools/gen_ccd.py`, `tests/fixtures/manifest.txt`
- **Verification:** Rebuilt the generator and test target; focused selector passed 2/2.
- **Committed in:** `14bd092`.

**Total deviations:** 2 small corrections; no behavior cases were dropped.
**Impact on plan:** All planned local artifacts and checks are complete. The summary explicitly records the invalid semantic RED and does not claim formal RED/GREEN evidence.

## Issues Encountered

The first identity assertion compared a source image without SUB to a CloneCD image with generated SUB. The identity function includes SUB in its digest, so that assertion was not a valid parity oracle. The corrected identity path separates boot/content identity from full SUB parity. The plan took 48m17s, above its 30-minute target, with test-oracle diagnosis the main bottleneck; no remaining implementation work required a scope split.

## User Setup Required

None.

## Next Phase Readiness

Plan 11-08 is dependency-ready. Phase-level code review and final verification remain with the phase orchestrator; no hosted CI run is active.

## Self-Check: PASSED

- Summary file exists and contains no trailing whitespace.
- All four task commit hashes are ancestors of the current branch head.
