---
phase: 08-per-game-card-saves
plan: "02"
subsystem: storage
tags:
  - memory-card
  - wrapper-formats
  - atomic-files
  - per-game-paths
requires:
  - phase: 08-01
    provides: Existing host card path builder and card-file test inventory.
provides:
  - Validated raw, GME, VGS, and Magazine PSV card-file import and export.
  - Atomic publication of complete raw and wrapped card images.
  - Full-length versioned digest paths with the legacy literal-name limit preserved.
affects:
  - 08-03
  - 08-04
actuals:
  tokens: 10800.5
  tasks: 3
  commits: 6
commits: 6
plan_head_before: e0bcf081a5bad2e8a83fa78938256c626ae0f923
plan_head_after: 6f5b94cb247dc6634f4169ba1135b7c52ad42829
tech-stack:
  added: []
  patterns:
    - Shared atomic publisher for raw and explicit full-card wrapper exports.
key-files:
  created: []
  modified:
    - host/cards.c
    - host/cards.h
    - tests/card/files.c
key-decisions: []
patterns-established:
  - Shared sizing and path writing for legacy literal names and versioned digest names.
requirements-completed:
  - SAVE-01
  - SAVE-02
  - SAVE-03
  - SAVE-04
coverage:
  - id: D1
    description: Raw and documented full-card wrappers are accepted only with valid content and lengths; missing files are distinct from refused inputs.
    requirement: SAVE-02
    verification:
      - kind: unit
        ref: "ctest card.files.atomic_failures, card.files.wrapper_edges, and card.files.wrappers (Task 1 command below)"
        status: pass
    human_judgment: false
  - id: D2
    description: Raw, GME, VGS, and Magazine PSV exports preserve all 131072 card bytes and failed publication preserves the previous complete file.
    requirement: SAVE-02
    verification:
      - kind: unit
        ref: "ctest card.files.* (Task 2 command below)"
        status: pass
    human_judgment: false
  - id: D3
    description: Versioned sgi1 digests produce two complete distinct card paths while legacy literal names retain the 63-character boundary.
    requirement: SAVE-01
    verification:
      - kind: unit
        ref: "ctest card.files.wrappers (Task 3 command below)"
        status: pass
    human_judgment: false
metrics:
  duration: 33min
  completed: 2026-11-19
status: complete
---

# Phase 8 Plan 2: Validate and atomically export complete cards Summary

Validated raw and documented wrapper imports, atomic full-card export, and untruncated digest-based paths while preserving the established raw padding and literal-name behavior.

## Performance

- **Duration:** 33 minutes
- **Started:** 2026-11-19T13:07:19Z
- **Completed:** 2026-11-19T13:39:23Z
- **Tasks:** 3
- **Files modified:** 3
- **Runtime/model:** Codex/OpenGSD CLI 1.20.0; executor gpt-6-luna.

## Accomplishments

- Import distinguishes a missing path from malformed or unsupported content; validates the supported complete card wrappers and rejects recognized partial or single-save formats without changing output or lint state. Short raw input still pads with `0xFF`.
- Added explicit raw, GME, VGS, and PlayStation Magazine PSV exports. All produce the complete 131072-byte card payload, round-trip byte-for-byte, and use the existing temporary-write, flush, sync, close, and atomic-replace path. Injected flush and rename failures leave the existing complete raw or wrapped file unchanged and clean up temporary candidates.
- Added an internal path helper for exactly `sgi1-` plus 64 lowercase hexadecimal digits. It shares suffix and buffer sizing with the existing path code, preserves both outputs on validation/capacity errors, and leaves literal names capped at 63 characters.
- Verified wrapper fields against the public PSX-SPX source pinned at commit [`6d7d1bc106a7e0b616b0330fe58401ab1ba57f0f`](https://github.com/psx-spx/psx-spx.github.io/blob/6d7d1bc106a7e0b616b0330fe58401ab1ba57f0f/docs/controllersandmemorycards.md). The source documents the 0xF40-byte DexDrive header and fixed signature/version/directory bytes, the 64-byte `VgsM` header, and the 256-byte `PSV` PlayStation Magazine header. Export leaves nonrecoverable DexDrive descriptions blank and preserves the card body.

## Evidence

All commands ran from the repository root. No hosted run applies.

| Task | Command | Exit | Outcome |
|---|---|---:|---|
| 1 RED | `ctest --preset ci --no-tests=error -R '^card[.]files[.](atomic_failures|wrapper_edges|wrappers)$' --output-on-failure --output-junit /private/tmp/schwaystation-08-02-task1-red.xml` | 8 | New absent-file and wrapper-refusal assertions failed on prior behavior; RED evidence was accepted. |
| 1 RED | `cmake --build --preset ci --target schwaystation-test-card-files && ctest --preset ci --no-tests=error -R '^card[.]files[.]wrapper_edges$' --output-on-failure --output-junit /private/tmp/schwaystation-08-02-task1-marker-red.xml` | 8 | The bad fixed DexDrive marker was accepted before implementation. |
| 1 GREEN | `cmake --build --preset ci --target schwaystation-test-card-files && ctest --preset ci --no-tests=error -R '^card[.]files[.](atomic_failures|wrapper_edges|wrappers)$' --output-on-failure` | 0 | All 3 focused selectors passed. |
| 2 RED | `cmake --build --preset ci --target schwaystation-test-card-files && ctest --preset ci --no-tests=error -R '^card[.]files[.]' --output-on-failure --output-junit /private/tmp/schwaystation-08-02-task2-red.xml` | 8 | Export and wrapped-publication assertions failed before the exporter existed; RED evidence identified the intended tests. |
| 2 GREEN | `cmake --build --preset ci --target schwaystation-test-card-files && ctest --preset ci --no-tests=error -R '^card[.]files[.]' --output-on-failure` | 0 | All 4 card-file tests passed. |
| 3 RED | `cmake --build --preset ci --target schwaystation-test-card-files && ctest --preset ci --no-tests=error -R '^card[.]files[.]wrappers$' --output-on-failure --output-junit /private/tmp/schwaystation-08-02-task3-red.xml` | 8 | Full digest and capacity assertions failed against a temporary stub; `gsd_run check tdd-red-evidence` returned `RED_EVIDENCE_OK`. |
| 3 GREEN | `cmake --build --preset ci --target schwaystation-test-card-files && ctest --preset ci --no-tests=error -R '^card[.]files[.]wrappers$' --output-on-failure` | 0 | The wrapper and path test passed (1/1). |
| Source pin | `gh api repos/psx-spx/psx-spx.github.io/commits/6d7d1bc106a7e0b616b0330fe58401ab1ba57f0f` | 0 | Confirmed the documented primary source commit is public. |
| Preflight | Changed card fixture inventory and source vectors; exact selector registration; repository-check and E2E line budgets | 0 | Existing fixture sizes were checked, no fixture bytes were added; repository checks were 3898/3901 lines and E2E checks 300/300 lines. |

## Task Commits

1. **Task 1: Validate supported wrappers and distinguish missing files from rejections** — `dda525c` (test), `73cff15` (test), `817a3dc` (fix).
2. **Task 2: Export explicit raw, GME, VGS and PSV full-card images** — `69c2032` (test), `e64f69b` (feat).
3. **Task 3: Fit the complete digest in both card paths** — `6f5b94c` (feat).

## Files Created/Modified

- `host/cards.c` — wrapper validation, distinct missing-file status, shared atomic exporter, and digest path handling.
- `host/cards.h` — host export format and internal identity-path declarations.
- `tests/card/files.c` — source-derived wrapper vectors, full-payload round trips, publication failure checks, and path boundary tests.

## Decisions Made

None — implementation follows the settled Phase 8 D-01, D-04, and D-05 choices.

## Deviations from Plan

None. The implementation stayed within the three planned files and kept the existing public card API unchanged.

## Issues Encountered

An intermediate Task 2 build found a duplicate temporary export stub; it was removed before the Task 2 green run and commit. The final focused build and tests pass.

## User Setup Required

None.

## Next Phase Readiness

This plan provides validated card-file formats and atomic export for later runner and libretro persistence plans. No owner verification or hosted run is required here.

---
*Phase: 08-per-game-card-saves*
*Completed: 2026-11-19*

## Self-Check: PASSED

The summary exists, all six task commits are ancestors of the current branch head, and `git diff --check` passed for the summary.
