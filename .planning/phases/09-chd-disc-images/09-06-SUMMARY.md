---
phase: 09-chd-disc-images
plan: 06
subsystem: media
tags: [chd, libchdr, disc-image, sector-parity]
requires:
  - phase: 09-05
    provides: Validated CHD track metadata and logical-to-storage frame map
provides:
  - Optional CHD routing through the shared disc image provider
  - Bounded decoded-hunk reads with logical LBA mapping and CUE-compatible CDDA bytes
  - Whole-fixture CHD/CUE TOC, sector, subcode and content-identity parity
affects: [Phase 9 media ledger and host integration, later disc image providers]
actuals:
  tokens: 8770
  tasks: 2
  commits: 2
tech-stack:
  added: []
  patterns:
    - Keep one bounded decoded hunk in each image reader and map logical sectors through the validated track map.
    - Normalize decoded CDDA byte order before exposing raw sectors through the common provider.
key-files:
  created: []
  modified:
    - host/disc_chd.c
    - host/disc_chd.h
    - host/disc_image.c
    - host/disc_image.h
    - docs/discs.md
    - tests/disc/chd.c
    - tests/disc/media.c
    - tests/disc/tests.cmake
    - tests/disc/inventory.txt
key-decisions:
  - Reuse the existing content-based save-game identity policy so CHD hashes represent logical sector content, never compressed container bytes.
  - Keep CHD support optional; CHD-off builds reject the format without a fixture or libchdr dependency.
requirements-completed: [MEDIA-01, CHD-01]
coverage:
  - id: CHD-SHARED-OPEN
    description: The shared provider opens the generated CHD, preserves an active image after invalid replacement input, and rejects CHD when the feature is disabled.
    requirement: MEDIA-01
    verification:
      - kind: integration
        ref: "disc.image.chd_open passed with SCHWAYSTATION_CHD=ON and OFF in the focused eight-selector CHD/media slice."
        status: pass
    human_judgment: false
  - id: CHD-WHOLE-DISC-PARITY
    description: The generated 222-sector CHD and CUE expose matching TOCs, raw sector bytes, applicable subcode and content identity in forward and reverse read order.
    requirement: CHD-01
    verification:
      - kind: integration
        ref: "disc.image.chd_parity passed for 222 sectors in both read orders; identity sgi2-b4ddcc3b7feaee12b36d962e8ec7a569dd953e8421c7ec088887a456a37577e3."
        status: pass
    human_judgment: false
  - id: CHD-LBA-BOUNDARIES
    description: Stored and virtual pregaps, track and hunk transitions, leadout and out-of-range LBAs map or fail at the correct boundaries.
    requirement: MEDIA-01
    verification:
      - kind: integration
        ref: "disc.image.chd_boundaries passed in the focused eight-selector CHD/media slice with SCHWAYSTATION_CHD=ON and OFF."
        status: pass
    human_judgment: false
duration: 32min
completed: 2026-11-20
status: complete
---

# Phase 9 Plan 09-06 Summary

**The shared disc image provider now reads CHD sectors with CUE-compatible CDDA bytes and complete logical parity evidence.**

## Performance

- **Duration:** 32 minutes from the saved Plan 09-05 checkpoint to plan closeout.
- **Started:** 2026-11-20T09:22:03Z
- **Completed:** 2026-11-20T09:54:00Z
- **Tasks:** 2
- **Files modified:** 9 source, test and documentation files.
- **Runtime/model:** Codex/OpenGSD 1.20.0; exact primary model ID is not exposed.

## Accomplishments

- Routed `.chd` through the existing shared image API while preserving failure-atomic open behavior and the CHD-off unsupported result.
- Added a reader-owned, bounded hunk cache and exact logical LBA mapping for stored and virtual pregaps, track transitions and leadout.
- Compared all 222 fixture sectors, applicable subcode, output guards, TOC fields and content identity against the CUE in forward and reverse order.
- Corrected the discovered audio byte-order mismatch: decoded CHD CDDA words are pair-swapped relative to the CUE/BIN payload, so the provider restores the CUE byte order.

## Task Commits

1. **Route CHD through the shared disc provider** — `9758617` (`feat(09-06): route CHD through the shared disc provider`)
2. **Prove whole-disc CHD parity** — `277b592` (`test(09-06): prove whole-disc CHD parity`)

## Checks and Efficiency

- Before the focused slice, `git diff --check HEAD && LC_ALL=C sort -cu tests/disc/inventory.txt && ctest --preset ci -N -R '^disc[.](media[.]|image[.]chd_)' --no-tests=error` exited 0 and found exactly eight selectors.
- With CHD enabled, the targeted test build exited 0 and the eight-selector slice exited 0: 8/8 passed in 0.06 seconds. The fixture remained 5,552 bytes with SHA-256 `4952c3a05ff2e2e596deed7ed2d37358a646790b890d5ed38484feec5b797569`.
- With CHD disabled, the fresh `SCHWAYSTATION_CHD=OFF` configure and targeted build exited 0; the same eight selectors exited 0: 8/8 passed in 0.01 seconds, without requiring the generated CHD fixture.
- The first parity attempt failed immediately at LBA 214. Byte-level diagnostics and the existing CUE fixture isolated CDDA pair ordering; the fix was followed by a focused boundary/parity rerun (3/3 including fixture preflight, 0.03 seconds) and then the complete enabled and disabled slices.
- No `act`, hosted CI or full suite was run: this plan changed only the host provider, tests and user documentation. Local ON/OFF selectors directly exercised the changed behavior, and no hosted workflow changed.
- The standing efficiency guidance remains discoverable in the project instructions and `.planning/AUTONOMOUS.md`; no extra permanent preflight was needed.

## Deviations and Boundaries

The parity comparison exposed a required CDDA normalization step missing from the initial implementation. The reader now reverses each audio sample byte pair before returning the sector; exact whole-fixture parity confirms the correction. No requirement coverage was dropped and no unrelated implementation scope was added.

This plan proves the owned CHD fixture and shared provider. Phase-level MEDIA-01 and CHD-01 remain pending until their later plans complete routing, lane, packaging, fuzz and feature-ledger obligations.

## Next Plan Readiness

Plan 09-06 is complete. Continue with the next ready Phase 9 plan; the media reader and full-fixture oracle are now available to the remaining CHD routing and media-ledger work.

---
*Phase: 09-chd-disc-images*
*Completed: 2026-11-20*
