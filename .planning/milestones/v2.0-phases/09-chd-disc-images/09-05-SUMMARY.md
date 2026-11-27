---
phase: 09-chd-disc-images
plan: 05
subsystem: media
tags: [chd, metadata, toc, bounds]
requires:
  - Reproducible build-local CHD fixture from Plan 09-04
provides:
  - Bounded CHTR/CHT2 metadata parser and logical-to-storage track map
  - Normalized CHD TOC matching the owned CUE fixture
affects: [Phase 9 CHD sector parity, later disc providers]
actuals:
  tokens: 7739
  tasks: 2
  commits: 5
tech-stack:
  added: []
  patterns:
    - Parse metadata into a fixed-size map before publishing a reader
    - Keep logical pregap/index/leadout LBAs distinct from four-frame CHD storage padding
key-files:
  created:
    - host/disc_chd_metadata.c
    - host/disc_chd_metadata.h
  modified:
    - cmake/DiscTargets.cmake
    - host/disc_chd.c
    - tests/disc/chd.c
    - tests/disc/tests.cmake
    - tests/disc/inventory.txt
    - docs/discs.md
key-decisions:
  - Accept only sequential CHTR/CHT2 CD records with raw AUDIO, MODE1_RAW or MODE2_RAW sectors, no subcode and no postgap semantics.
  - Reject malformed metadata before returning a reader; keep the public router and sector parity for later plans.
patterns-established:
  - Check sorted inventories and diffs before fixture generation or focused builds.
  - Exercise both CHD-enabled and dependency-free CHD-disabled configurations without repeating the full suite.
requirements-completed: []
coverage:
  - id: CHD-TOC
    description: The generated CHD returns the same meaningful track types, indexes, pregap and exclusive leadouts as the authored CUE.
    requirement: MEDIA-01
    verification:
      - kind: integration
        ref: "cmake --preset ci -DSCHWAYSTATION_CHD=ON -DSCHWAYSTATION_BUILD_CHD_TOOL=ON; build targets schwaystation-test-disc-chd and schwaystation-chd-fixtures; focused CHD selectors (exit 0, 4/4 including fixture preflight, 0.06s)."
        status: pass
    human_judgment: false
  - id: CHD-METADATA
    description: Track counts, order, metadata lengths, supported modes, pregaps, postgaps, hunk/logical sizes and arithmetic limits are checked before the map is returned.
    requirement: MEDIA-02
    verification:
      - kind: unit
        ref: "disc.image.chd_metadata and disc.image.chd_headers passed in CHD-on and CHD-off configurations; exact 99-track boundary and one-over control are included."
        status: pass
    human_judgment: false
  - id: CHD-SUBSET
    description: User documentation names the bounded v3-v5 CD metadata subset and unsupported layout semantics without claiming public routing or sector parity yet.
    requirement: CHD-01
    verification:
      - kind: other
        ref: "docs/discs.md CHD metadata subset; generated v5 CHT2 plus in-memory v3 CHTR and v4 CHT2 controls passed. Phase-wide CHD parity remains assigned to later plans."
        status: pass
    human_judgment: false
duration: 198min
completed: 2026-11-20
status: complete
---

# Phase 9 Plan 09-05 Summary

**Bounded CHD track metadata now projects the owned image into the exact logical TOC without exposing per-track storage padding.**

## Performance

- **Duration:** 3h 19m wall interval from the prior saved STATE checkpoint; active time was not measured.
- **Started:** 2026-11-20T06:03:28Z (previous saved STATE checkpoint)
- **Completed:** 2026-11-20T09:22:03Z
- **Tasks:** 2
- **Changed paths:** 8 implementation/test/documentation files, plus plan and closeout records.
- **Runtime/model:** Codex/OpenGSD 1.20.0; exact primary GPT-6 model ID is not exposed.

## Accomplishments

- Added a bounded private parser for sequential CHTR/CHT2 records, explicit stored and virtual pregaps, and a per-track map from public LBAs to padded CHD frames.
- The real generated two-track CHD returns the same meaningful TOC fields as its CUE oracle: track 1 leadout 64; track 2 index 0 at 64, index 1 at 214, 150-sector pregap and exclusive leadout 222.
- Added boundary and rejection controls for zero/over-limit track records, order, mixed/unknown metadata, unsupported modes and subcode, postgaps, truncated strings, oversized logical data, hunk alignment and integer overflow.
- Documented the exact private metadata subset and kept public image routing and sector parity explicitly for later plans.

## Task Commits

1. **Return normalized CHD metadata and exercise its bounds** — `725c464` (`feat(09-05): normalize CHD metadata TOC`)
2. **Document the tested CHD metadata subset** — `6a4d468` (`docs(09-05): define supported CHD metadata subset`)

Planning corrections: `ba579e5`, `7af3a5f` and `04d3558` enable the fixture generator in acceptance commands, isolate the parser module in plan scope, and include the target wiring and metadata selector in the plan's file/test inventory.

## Checks and Efficiency

- Cheap preflight: `LC_ALL=C sort -cu tests/disc/inventory.txt && git diff --check HEAD` exited 0 before configure/build work.
- CHD-on command: `cmake --preset ci -DSCHWAYSTATION_CHD=ON -DSCHWAYSTATION_BUILD_CHD_TOOL=ON && cmake --build --preset ci --target schwaystation-test-disc-chd schwaystation-chd-fixtures && ctest --preset ci --no-tests=error -R '^disc[.]image[.]chd_(headers|metadata|toc)$' --output-on-failure` exited 0; 4/4 passed in 0.06s including fixture preflight. The generator produced the expected 5,552-byte fixture with SHA-256 `4952c3a05ff2e2e596deed7ed2d37358a646790b890d5ed38484feec5b797569`.
- CHD-off command: sorted-inventory and diff preflight, fresh `SCHWAYSTATION_CHD=OFF` configure/build, and the same three CHD selectors exited 0; 3/3 passed in 0.03s.
- No full suite, `act`, or hosted CI run applied: this plan changed no workflow, and the focused ON/OFF selectors directly covered the changed parser and generated fixture path. A broad run would have added wait and cost without useful new coverage.
- The standing CI/release efficiency policy remains discoverable in `AGENTS.md` and `.planning/AUTONOMOUS.md`; no additional permanent preflight was needed.

## Deviations and Boundaries

The plan inventory initially omitted `cmake/DiscTargets.cmake` and its acceptance regex omitted the in-memory metadata selector. The plan was corrected before the implementation commits. A reader-buffer write on the over-limit path was also removed. These changes preserve the intended behavior and bounds.

The generated CHD's normalized TOC is available through the private reader. Public routing, sector/subcode parity, fuzz coverage and phase-wide requirements remain open for their planned successors; this summary does not mark MEDIA-01, MEDIA-02 or CHD-01 complete at phase level.

## Next Plan Readiness

Plan 09-05 is complete. Plan 09-06 is next on the dependency path and can build on the exact CHD TOC. Plans 09-08 (feature ledger preflight) and 09-10 (measured CI reuse) are also independently ready.

## Self-Check

Both plan tasks, the CHD-on and CHD-off focused selectors, the sorted inventory, the diff checks and the summary metadata were checked; all required local commands exited 0.

---
*Phase: 09-chd-disc-images*
*Completed: 2026-11-20*
