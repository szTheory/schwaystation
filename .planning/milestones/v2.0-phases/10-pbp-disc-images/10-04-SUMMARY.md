---
phase: 10-pbp-disc-images
plan: "04"
subsystem: host-media
tags: [c, python, pbp, fixtures, cmake, ctest]
requires:
  - phase: 10-pbp-disc-images
    provides:
      - bounded raw inflater wrapper
      - PBP format research and approved fixture cap
provides:
  - deterministic build-local single-track PBP fixture from owned source sectors
  - source, generator and output hashes with exact sector and block counts
  - PBP geometry, extent, identity and cap preflight before provider or guest use
affects: [host-media, disc-testing]
actuals:
  tasks: 2
  commits: 2
plan_head_before: 7628a9b
plan_head_after: 54679e1
commits: 2
tech-stack:
  added: []
  patterns:
    - repeated deterministic generation plus independent standard-library raw-DEFLATE decode
    - CTest setup fixture validates media identity and geometry before later consumers
key-files:
  created:
    - tools/gen_pbp.py
    - cmake/PBPFixtures.cmake
  modified:
    - cmake/DiscFixtures.cmake
    - tests/disc/pbp.c
    - tests/disc/tests.cmake
    - tests/disc/inventory.txt
    - ASSET_POLICY.md
    - docs/media-testing.md
key-decisions:
  - Keep the generated PBP build-local with a 1.5 MiB cap; the existing CUE/BIN cap remains 512 KiB.
requirements-completed: []
coverage:
  - id: P1
    description: The owned 64-sector data track generates byte-identical PBP output with a matching single-track CUE and source, generator and output identities.
    requirement: PBP-01
    verification:
      - kind: integration
        ref: "cmake --preset ci -DSCHWAYSTATION_CHD=OFF -DSCHWAYSTATION_BUILD_CHD_TOOL=OFF; exit 0; 0.7 seconds"
        status: pass
      - kind: integration
        ref: "cmake --build --preset ci --target schwaystation-test-disc-pbp schwaystation-pbp-fixtures; exit 0; generated 1124404 bytes, 64 sectors, 4 blocks"
        status: pass
    human_judgment: false
  - id: P2
    description: The fixture preflight accepts valid media and checks source geometry, PBP metadata, all decoded sectors, exact extents, hashes and the PBP-only size cap.
    requirement: PBP-01
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^disc[.]image[.]pbp_(preflight|fixture_controls)$' --output-on-failure; exit 0; 2/2 in 0.04 seconds"
        status: pass
    human_judgment: false
  - id: P3
    description: Wrong digest, missing input, source and TOC counts, overlapping and truncated extents, wrong generator identity, and over-cap output fail at their intended check.
    requirement: PBP-01
    verification:
      - kind: integration
        ref: "disc.image.pbp_fixture_controls as part of the focused CTest command; exit 0"
        status: pass
    human_judgment: false
  - id: P4
    description: The exact disc inventory and repository-check ceiling remain valid, with no whitespace errors.
    requirement: PBP-01
    verification:
      - kind: other
        ref: "LC_ALL=C sort -cu tests/disc/inventory.txt; git diff --check; cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake; Python source compile check; exit 0; 3900/3901"
        status: pass
      - kind: other
        ref: "git diff --check origin/main...HEAD; exit 0"
        status: pass
    human_judgment: false
duration: "Wall interval from the saved 09:49:43Z STATE checkpoint to closeout at 11:28:58Z was about 1h39m; it includes inherited idle time. Active work in this resumed segment was not separately timed."
completed: 2026-11-21
status: complete
---

# Phase 10 Plan 04: Deterministic PBP Fixture Summary

**The owned single-track source now produces a bounded deterministic PBP, and preflight rejects fixture identity or geometry mistakes before later consumers.**

## Performance

- **Runtime/model:** Codex session; primary model ID is not exposed.
- **Elapsed interval:** About 1h39m from the saved state checkpoint through closeout, including inherited idle time; active time was not separately measured.
- **Task commits:** `68f8173`, `54679e1`.
- **Observed bottleneck:** This plan's configure, focused build and preflight were fast: 0.7s, under 1s, and 0.04s respectively. The prior wait was not attributable to these checks. No process was interrupted.
- The focused suite was sufficient for this producer-only slice. No workflow changed, so `act` and hosted CI were not applicable.

## Accomplishments

- Added a build-local deterministic PBP generator. It copies the owned first MODE2/2352 track into an equivalent one-track CUE, emits raw or fixed-strategy raw-DEFLATE blocks, independently decodes compressed output, and regenerates the image to assert byte identity.
- Added an explicit 1.5 MiB PBP-only cap while preserving the 512 KiB CUE/BIN asset cap. The generated 64-sector fixture is 1,124,404 bytes with four blocks.
- Added C preflight checks for CUE track geometry, source sector completeness, PBP directory and PSISO metadata, TOC, block table and extents, exact decoded sectors, and source/generator/output hashes.
- Registered negative controls for wrong digest, missing media, wrong source and TOC counts, overlap, truncation, wrong generator identity, and over-cap output. Later provider and guest tests can depend on `disc_pbp_preflight`.
- The repository-check gate remains 3,900/3,901 lines.

## Verification

- `LC_ALL=C sort -cu tests/disc/inventory.txt` — exit 0.
- `git diff --check` — exit 0.
- `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` — exit 0; 3,900/3,901.
- `python3 -I -c "compile(open('tools/gen_pbp.py', encoding='utf-8').read(), 'tools/gen_pbp.py', 'exec')"` — exit 0.
- `cmake --preset ci -DSCHWAYSTATION_CHD=OFF -DSCHWAYSTATION_BUILD_CHD_TOOL=OFF` — exit 0; configured in 0.7s.
- `cmake --build --preset ci --target schwaystation-test-disc-pbp schwaystation-pbp-fixtures` — exit 0; built the test and generated the PBP.
- `ctest --preset ci --no-tests=error -R '^disc[.]image[.]pbp_(preflight|fixture_controls)$' --output-on-failure` — exit 0; 2/2 in 0.04s.
- `git diff --check origin/main...HEAD` — exit 0.
- No broad suite, `act`, or hosted workflow was needed for this isolated fixture producer. Phase-wide review and verification remain later in Phase 10.

## Next Plan Readiness

Plan 10-05 can parse the fixture-proven PSISO directory, TOC and block table into the existing host media contract. PBP-01 remains phase-wide pending until the provider, parity, integration and release checks pass.
