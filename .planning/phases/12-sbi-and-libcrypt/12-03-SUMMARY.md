---
phase: 12-sbi-and-libcrypt
plan: "03"
subsystem: disc
tags: [sbi, provider, m3u, bad-q]
requires:
  - phase: 12-02
    provides: bounded SBI format-1/2/3 parser
provides:
  - Same-stem SBI discovery and bounded provider-owned bad-Q callbacks
  - Per-child SBI ownership and TOC selection for M3U images
  - User-facing SBI pairing behavior and identity limitation
affects: [CD-ROM bad-Q integration, LibCrypt behavior]
actuals:
  tokens: 2800
  tasks: 3
  commits: 2
tech-stack:
  added: []
  patterns: [canonical-path sidecar discovery, provider-owned sorted LBA callback]
key-files:
  created: []
  modified: [host/disc_image.c, tests/disc/sbi.c, tests/disc/tests.cmake, docs/discs.md]
key-decisions:
  - "Load each same-stem sidecar into its concrete provider; an M3U parent owns no overlay and each selected child keeps its own callback and TOC."
  - "Treat a missing sidecar as ordinary behavior and reject malformed, oversized, unreadable or out-of-map sidecars before publishing the image."
  - "Document that SBI has no image digest and therefore cannot authenticate its pairing."
requirements-completed: [CRYPT-01]
coverage:
  - id: D1
    description: "ISO and CUE providers load same-stem overlays, leave the callback absent without a sidecar, and reject malformed or out-of-map sidecars atomically."
    requirement: CRYPT-01
    verification:
      - kind: unit
        ref: "disc.image.sbi"
        status: pass
    human_judgment: false
  - id: D2
    description: "Selecting M3U children with overlapping bad-Q LBAs returns each child's own callback and distinct TOC map without cross-leakage."
    requirement: CRYPT-01
    verification:
      - kind: unit
        ref: "disc.image.sbi"
        status: pass
    human_judgment: false
  - id: D3
    description: "Disc documentation names companion loading, absence and failure behavior, M3U child association and the lack of image authentication."
    requirement: CRYPT-01
    verification:
      - kind: other
        ref: "docs/discs.md"
        status: pass
    human_judgment: false
duration: 12min
completed: 2026-11-23
status: complete
---

# Phase 12 Plan 03: SBI provider binding Summary

**Each opened image now owns its validated SBI bad-Q overlay, and M3U selection returns the matching child callback and TOC.**

## Performance

- **Duration:** 16 minutes wall time
- **Started:** 2026-11-23T05:28:03Z
- **Completed:** 2026-11-23T05:43:46Z
- **Tasks:** 3
- **Files modified:** 5, including the plan amendment
- **Runtime/model:** Codex/GPT-6; primary model ID not exposed

## Accomplishments

- Added bounded same-stem sidecar loading using the canonical image path and provider TOC; absent sidecars preserve the existing no-callback behavior.
- Bound the sorted bad-Q LBA set to the concrete image object and proved overlapping M3U child LBAs do not cross providers, including distinct 16- and 17-sector TOCs.
- Documented companion naming, failure handling, per-child playlist association and the format's lack of image authentication.

## Task Commits

1. **Bind SBI overlays to image providers** - `ea949fd` (`feat(12): bind SBI overlays to image providers`)
2. **Cover SBI CUE provider binding** - `ab639dd` (`test(12-03): cover SBI CUE provider binding`)

## Test Evidence

- Cheap preflights passed: sorted disc inventory, exact CTest selector/argument wiring, `git diff --check`, and the 3,901-line repository-check budget.
- `cmake --build --preset ci --target schwaystation-test-disc-image schwaystation-test-disc-sbi` - exit 0.
- `ctest --preset ci --no-tests=error -R '^disc[.]image[.](sbi|.*open)$' --output-on-failure` - exit 0; 8/8 selected and fixture checks passed in 0.13 seconds after the final CUE vector.
- No hosted CI was started for this local plan slice; it does not change workflow routing. The affected selectors and open paths are covered locally before Phase 12's later changed-head CI.

## Deviations from Plan

- Provider integration lives in the existing `tests/disc/sbi.c` selector rather than adding a second image-test selector. Passing its build-owned directory keeps generated media isolated and exercises provider ownership in the parser's existing CTest.

## Issues Encountered

None. The implementation validates the SBI address against the provider's exclusive maximum TOC leadout; it does not claim that the sidecar cryptographically identifies the image.

## Next Phase Readiness

Plan 12-04 can now exercise bad-Q acceptance and previous-good GetlocP position through CD-ROM read and seek command sequences.

## Self-Check: PASSED

- All plan acceptance criteria and the plan-level open-path selector pass.
- `check evaluation-scope --plan 12-03 --commits-only --raw` resolves the CUE binding test commit on this branch.
- Every created artifact exists; `git diff --check` and the sorted inventory preflight pass.

---
*Phase: 12-sbi-and-libcrypt*
*Completed: 2026-11-23*
