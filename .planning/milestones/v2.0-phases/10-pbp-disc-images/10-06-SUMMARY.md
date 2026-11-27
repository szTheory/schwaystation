---
phase: 10-pbp-disc-images
plan: "06"
subsystem: host-media
tags: [c, pbp, disc-image, ctest]
requires:
  - phase: 10-pbp-disc-images
    provides:
      - deterministic owned PBP fixture and source-sector preflight
      - validated PSISO TOC and block metadata
provides:
  - validated bounded PBP block reader through the shared disc-image callbacks
  - raw and compressed block acceptance with full source-sector parity
  - failure-atomic PBP dispatch, TOC, boundaries and content identity checks
affects: [host-media, runner, libretro, disc-testing]
actuals:
  tasks: 2
  commits: 1
plan_head_before: 2aa79bd
plan_head_after: 83d7bf0
tech-stack:
  added: []
  patterns:
    - validate each indexed block before publishing a reader, then retain one decoded block as the cache
    - use one fixed 0x9300-byte decoded buffer and bounded raw-DEFLATE input
key-files:
  created: []
  modified:
    - host/disc_image.c
    - host/disc_pbp.c
    - tests/disc/image.c
    - tests/disc/pbp.c
    - tests/disc/tests.cmake
    - tests/disc/inventory.txt
    - docs/media-testing.md
key-decisions:
  - Accept a block stored verbatim at exactly 0x9300 bytes or a smaller exact raw-DEFLATE stream; reject malformed blocks before returning a reader.
  - Keep PBP inside the existing image dispatcher and save/content identity path, with no public ABI or system-zlib dependency.
requirements-completed: []
coverage:
  - id: M1
    description: The PBP provider validates its complete indexed block stream before exposing a bounded sector reader and normalized TOC.
    requirement: PBP-01
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^disc[.]image[.]pbp_(raw|metadata|bounds|parity)$' --output-on-failure; exit 0; 5/5 including preflight"
        status: pass
    human_judgment: false
  - id: M2
    description: Raw/compressed sectors, source TOC, block boundaries, leadout behavior and content identity match the owned CUE source.
    requirement: PBP-01
    verification:
      - kind: integration
        ref: "Plan 10-06 focused open, boundary and parity selectors; exit 0; forward/reverse 64-sector parity and identity checks passed"
        status: pass
    human_judgment: false
  - id: M3
    description: Sorted inventory, whitespace checks and the absolute repository-check ceiling remain valid.
    requirement: PBP-01
    verification:
      - kind: other
        ref: "LC_ALL=C sort -cu tests/disc/inventory.txt; git diff --check; line_budget.cmake with LIMIT=3901; exit 0; 3900/3901"
        status: pass
      - kind: other
        ref: "ctest --preset ci --no-tests=error -N -R '^disc[.]image[.]pbp_raw$'; exit 0; raw selector and fixture preflight present"
        status: pass
    human_judgment: false
duration: "Handoff interval from the 12:02:59Z STATE checkpoint through root closeout at 12:27:39Z was 24m40s; active executor time was not separately measured."
completed: 2026-11-21
status: complete
---

# Phase 10 Plan 06: Validated PBP Sector Reader

**The shared image dispatcher now reads fixture-proven PBP sectors after validating every indexed block.**

## Performance

- **Runtime/model:** Codex/OpenGSD 1.20.0; executor `gpt-6-luna` xhigh; orchestration model ID and effort were not exposed.
- **Elapsed interval:** 24m40s from the prior saved STATE checkpoint through root closeout; active executor time was not separately measured.
- **Task commit:** `83d7bf0`.
- **Observed bottleneck:** Root diff review caught that the first implementation did not handle the fixture-supported verbatim 0x9300-byte block form. The executor added the raw-copy path and a dedicated selector before reporting completion. Root review then aligned the synthetic fixture's PSAR length metadata with its larger raw payload. No broad suite was needed.
- The measured fixture open/scan process time was 0.002783s, including load, parse, scan and endpoint reads. The reader uses one 0x9300-byte decoded buffer plus a 0x9300-byte input buffer and one cached block; the open scan is bounded by the 1 GiB logical-media limit.
- No workflow changed, so `act` and hosted CI were not applicable.

## Accomplishments

- Implemented PBP open, TOC and sector reads through the shared disc-image dispatcher while keeping `.pbp` invalid as a CUE child.
- Validated every compressed block before publishing a reader, using exact raw-DEFLATE completion and fixed-size storage. A block exactly 0x9300 bytes is copied verbatim. Malformed and unsupported layouts retain their distinct failure statuses.
- Added first/final sector, compressed-block transition, repeated-cache, leadout, beyond-leadout, failed-replacement, zero-subcode and complete forward/reverse sector parity checks against the source CUE.
- Confirmed save/content identity is the same for the PBP and source CUE. The synthetic raw-block case checks PSAR section lengths and verifies raw sector bytes at LBAs 0, 7 and 15.
- Updated maintainer media-testing instructions for the provider, focused preflight, raw/compressed cases and bounds.
- The repository-check gate remains 3,900/3,901 lines.

## Commands and Results

- `LC_ALL=C sort -cu tests/disc/inventory.txt && git diff --check && cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake && ctest --preset ci --no-tests=error -N -R '^disc[.]image[.]pbp_raw$'` — exit 0; inventory and diff clean, 3,900/3,901, both raw selector and preflight registered.
- `cmake --build --preset ci --target schwaystation-test-disc-pbp schwaystation-test-disc-image schwaystation-pbp-fixtures && ctest --preset ci --no-tests=error -R '^disc[.]image[.]pbp_raw$' --output-on-failure` — exit 0; 2/2 including fixture preflight.
- `ctest --preset ci --no-tests=error -R '^disc[.]image[.]pbp_(raw|metadata|bounds|parity)$' --output-on-failure` — executor reported exit 0; 5/5 including fixture preflight in 0.02s.
- Focused open/boundary/parity and playlist checks — executor reported exit 0; 10/10. The affected CUE child rejection check passed 1/1.
- No workflow changed, so no `act` or hosted run was required. Phase-level code review and verification remain in Plan 10-13.

## Next Plan Readiness

Plan 10-07 can drive malformed and accepted PBP variants through the production parser and add one bounded Nightly sanitizer campaign. PBP-01 remains phase-wide pending runner/libretro integration, package checks, guest evidence, review and release.

## Self-Check: PASSED
