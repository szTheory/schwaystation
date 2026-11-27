---
phase: 10-pbp-disc-images
plan: "05"
subsystem: host-media
tags: [c, pbp, metadata, cmake, ctest]
requires:
  - phase: 10-pbp-disc-images
    provides:
      - deterministic owned PBP fixture and source parity preflight
      - bounded source and image limits
provides:
  - bounded PSISO metadata map for the fixture-proven single-track layout
  - checked PBP directory, TOC, block table and compressed extents
  - exact malformed and unsupported metadata controls
affects: [host-media, disc-testing]
actuals:
  tasks: 2
  commits: 2
plan_head_before: 4ef1a35
plan_head_after: 826cb13
tech-stack:
  added: []
  patterns:
    - fixed-size metadata reads with checked 64-bit extents before block-map allocation
    - subtract the 150-frame lead-in before mapping absolute MSF to zero-based disc LBAs
key-files:
  created:
    - host/disc_pbp_metadata.c
    - host/disc_pbp_metadata.h
  modified:
    - host/disc_pbp.c
    - cmake/DiscTargets.cmake
    - tests/disc/pbp.c
    - tests/disc/tests.cmake
    - tests/disc/inventory.txt
    - docs/discs.md
key-decisions:
  - Accept only the fixture-proven one-data-track PSISO metadata layout; valid metadata remains unsupported by public open until Plan 10-06 adds sector reading.
  - Keep 1 GiB input and decoded-media bounds; valid BCD MSF cannot encode a logical sector total above that cap, so cover the maximum representable TOC and the separate oversized-input guard.
requirements-completed: []
coverage:
  - id: M1
    description: The accepted PBP maps to the source MODE2 track, zero-based LBA range, 64 sectors and four compressed-byte extents.
    requirement: PBP-01
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^disc[.]image[.]pbp_(headers|inflate|preflight|fixture_controls|metadata|bounds)$' --output-on-failure; exit 0; 6/6 in 0.03 seconds"
        status: pass
    human_judgment: false
  - id: M2
    description: Invalid offsets, signatures, marker variants, BCD/MSF, track records, leadout, index extents and oversized advertised input fail with the expected status and a cleared output map.
    requirement: PBP-01
    verification:
      - kind: integration
        ref: "disc.image.pbp_bounds as part of the focused six-selector CTest command; exit 0"
        status: pass
    human_judgment: false
  - id: M3
    description: The largest block-aligned BCD-representable TOC is accepted with its bounded block map; a logical-over-1-GiB TOC is unrepresentable in valid BCD MSF.
    requirement: PBP-01
    verification:
      - kind: unit
        ref: "disc.image.pbp_bounds constructs and parses the maximum 449840-sector / 28115-block TOC; exit 0"
        status: pass
    human_judgment: false
  - id: M4
    description: The sorted disc inventory, whitespace check and repository-check ceiling remain valid.
    requirement: PBP-01
    verification:
      - kind: other
        ref: "LC_ALL=C sort -cu tests/disc/inventory.txt; git diff --check; cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake; exit 0; 3900/3901"
        status: pass
      - kind: other
        ref: "ctest --preset ci --no-tests=error -N -R '^disc[.]image[.]pbp_(metadata|bounds)$'; exit 0; both selectors present"
        status: pass
    human_judgment: false
duration: "Wall interval from the Plan 10-04 STATE checkpoint at 11:28:58Z through closeout at 12:02:59Z was about 34 minutes; active time was not separately measured."
completed: 2026-11-21
status: complete
---

# Phase 10 Plan 05: Bounded PSISO Metadata Summary

**The host now validates and maps the fixture-proven PSISO directory, TOC and block table before a reader can be published.**

## Performance

- **Runtime/model:** Codex/OpenGSD 1.20.0; executor `gpt-6-luna` xhigh; primary model ID is not exposed.
- **Elapsed interval:** About 34 minutes from the prior saved state checkpoint through closeout; active time was not separately measured.
- **Task commits:** `5db27a6`, `826cb13`.
- **Observed bottleneck:** The initial focused compile found one unused local, and the first controls exposed an outdated test envelope plus a wrong expectation for the maximum legal BCD leadout. Each was corrected at the focused boundary; the final six-selector run took 0.03s. No broad suite was needed.
- No workflow changed, so `act` and hosted CI were not applicable.

## Accomplishments

- Added a private metadata parser that checks PBP section offsets, PSISO markers and lengths, supported A0/A1/A2 and track records, BCD/MSF ranges, leadout, block count, cumulative compressed byte offsets, and each indexed extent before allocation or indexing.
- Mapped the fixture's absolute `00:02:00` track start and A2 leadout to the shared zero-based LBA contract by subtracting the 150-frame lead-in.
- Kept invalid metadata failure-atomic: the map is cleared on rejection, and the public open path publishes no reader. Until Plan 10-06 adds sector reads, valid metadata still returns `UNSUPPORTED` from public open.
- Added controls for outer offsets, bad signatures, PGD/multi-disc/unknown markers, malformed BCD/MSF, missing and duplicate records, leadout mismatch, extra tracks, overlapping/nonmonotonic/out-of-file extents, table truncation, maximum legal TOC, and oversized advertised input.
- A valid TOC cannot encode more than 1 GiB of logical sector data with its BCD MSF fields. The largest block-aligned valid TOC represents 449,840 sectors / 28,115 blocks; the separate file-size guard rejects input above the 1 GiB policy limit.
- The repository-check gate remains 3,900/3,901 lines.

## Verification

- `LC_ALL=C sort -cu tests/disc/inventory.txt` — exit 0.
- `git diff --check` — exit 0.
- `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` — exit 0; 3,900/3,901.
- `cmake --preset ci -DSCHWAYSTATION_CHD=OFF -DSCHWAYSTATION_BUILD_CHD_TOOL=OFF` — exit 0; 0.7s.
- `cmake --build --preset ci --target schwaystation-test-disc-pbp` — exit 0; `ctest --preset ci --no-tests=error -R '^disc[.]image[.]pbp_headers$' --output-on-failure` — exit 0 after correcting the test envelope version field.
- `ctest --preset ci --no-tests=error -R '^disc[.]image[.]pbp_(headers|inflate|preflight|fixture_controls|metadata|bounds)$' --output-on-failure` — exit 0; 6/6 in 0.03s.
- `ctest --preset ci --no-tests=error -N -R '^disc[.]image[.]pbp_(metadata|bounds)$'` — exit 0; both new selectors present.
- No broad suite, `act`, or hosted workflow was needed for metadata parsing alone. Phase-wide code review and verification remain later in Phase 10.

## Next Plan Readiness

Plan 10-06 can consume the validated block map to decode and expose sectors. PBP-01 remains phase-wide pending until sector parity, runner/libretro integration, package checks, guests, review and release pass.
