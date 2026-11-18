---
phase: 04-games-play-in-retroarch
plan: "35"
subsystem: card
tags: [memory-card, wrapper-import, save-naming, host-helper]

requires:
  - phase: 04-17
    provides: raw card import and atomic host save helpers
provides:
  - Bounded imports for GME, VGS, and PSX card wrappers
  - Validated content-basename and optional serial-based save paths
  - Gating wrapper and refusal-edge coverage with user-facing documentation
affects: [runner-card-persistence, libretro-card-persistence]

actuals:
  tokens: 7862
  tasks: 2
  commits: 3
plan_head_before: 8b64b132e5a159f845024cad23e6c931c5cfd4f7
plan_head_after: 88bd06c53913e732eac8eac47e0dd51b6f6dc115

tech-stack:
  added: []
  patterns:
    - Bounded host-only wrapper parsing before raw card lint and publish
    - Validate complete output path capacity before mutating caller buffers

key-files:
  created: []
  modified:
    - host/cards.c
    - host/cards.h
    - tests/card/files.c
    - tests/card/tests.cmake
    - tests/card/inventory.txt
    - docs/runner.md

key-decisions:
  - "Card files remain host-owned; core code does not read files or resolve save paths."
  - "Content basename selects canonical .srm and .1.mcr names by default; validated serial naming is an optional policy."
  - "Only GME, VGS, and PSX wrapper forms are accepted, with exact bounded headers; PSP/Vita and MCX remain unsupported."

patterns-established:
  - "Rejected imports preserve output buffers, lint results, and source files."
  - "Card wrapper and naming behavior is exercised through registered tests and byte-sorted inventory rows."

requirements-completed: []
coverage:
  - id: D1
    description: "Import supported GME, VGS, and PSX card wrappers through the host helper."
    requirement: GAME-04
    verification:
      - kind: unit
        ref: "tests/card/files.c#card.files.wrappers"
        status: pass
    human_judgment: false
  - id: D2
    description: "Resolve canonical content-basename save names and optional validated serial-based names."
    requirement: GAME-04
    verification:
      - kind: unit
        ref: "tests/card/files.c#card.files.wrappers"
        status: pass
    human_judgment: false
  - id: D3
    description: "Reject malformed, unsupported, oversized, truncated, and unsafe inputs without mutating prior results or source files."
    requirement: GAME-04
    verification:
      - kind: unit
        ref: "tests/card/files.c#card.files.wrapper_edges"
        status: pass
    human_judgment: false

duration: 29min
completed: 2026-11-11
status: complete
---

# Phase 4 Plan 35: Legacy Card Wrappers and Canonical Names Summary

**The host card helper now imports three approved legacy card formats and resolves canonical save names without changing core state or performing core file I/O.**

## Performance

- **Duration:** 29 minutes
- **Started:** 2026-11-11T10:18:01Z
- **Completed:** 2026-11-11T10:47:00Z
- **Tasks:** 2
- **Files modified:** 6

## Accomplishments

- Added bounded GME, VGS, and PSX wrapper imports while keeping raw card imports available.
- Added validated path construction for default content basenames and optional game serials, producing `.srm` and `.1.mcr` names.
- Added passing gating coverage for valid imports, malformed-input refusals, read-only lint, source preservation, and duplicate basenames in separate directories.
- Documented supported wrappers, naming, and refusal behavior in the runner guide.

## Task Commits

1. **Task 1: Import legacy wrappers and enforce canonical save names** — `efd79c6` (test), `e999e07` (feat)
2. **Task 2: Expand and document the proven path** — `88bd06c` (fix)

The plan contains three measured task commits. The final metadata commit is recorded separately after bookkeeping.

## Files Created/Modified

- `host/cards.c` and `host/cards.h` — bounded wrapper import and validated save-path helpers.
- `tests/card/files.c` — valid wrapper, path, malformed-input, and preservation cases.
- `tests/card/tests.cmake` and `tests/card/inventory.txt` — registered gating tests and exact sorted inventory entries.
- `docs/runner.md` — documented import and canonical naming behavior.

## Decisions Made

- Card file access and save-path resolution stay in the host helper; the core remains filesystem-independent.
- Content basenames are the default save-name source. Hosts may select a validated serial name explicitly.
- The importer accepts GME, VGS, and PSX wrappers only; other formats remain rejected.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Support content basenames as the default save path source**
- **Found during:** Task 2 (Expand and document the proven path)
- **Issue:** The first implementation exposed serial-based paths but did not provide the plan’s default content-basename paths.
- **Fix:** Generalized the path helper to accept the selected safe basename, supporting content names by default and validated serial names when requested.
- **Files modified:** `host/cards.c`, `host/cards.h`, `tests/card/files.c`
- **Verification:** `card.files.wrappers`, `card.files.wrapper_edges`, and both final workflow checks passed.
- **Committed in:** `88bd06c`

**Total deviations:** 1 auto-fixed (Rule 1)
**Impact on plan:** This corrected the naming contract within scope; no additional feature or dependency was added.

## Evidence

- TDD RED evidence: the registered `card.files.wrappers` test failed against the initial stub implementation; `gsd-tools check tdd-red-evidence` accepted the matching JUnit report and semantic assertion.
- Focused wrapper test: `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-card-files && ctest --test-dir build/ci -R '^card\.files\.wrappers$' --no-tests=error --output-on-failure` — exit 0.
- Focused edge and final card tests — `card.files.wrappers` and `card.files.wrapper_edges` selected together — exit 0.
- Full CI workflow: `cmake --workflow --preset ci` — exit 0; all 450 CI tests and both release archive tests passed.
- Full hygiene workflow: `cmake --workflow --preset hygiene` — exit 0; all 23 checks passed.

## Threat Flags

| Flag | File | Description |
|------|------|-------------|
| threat_flag: file-access | `host/cards.c` | Adds bounded host-side reads of card-image paths before validating supported wrapper headers. |

## Issues Encountered

The initial path helper only covered serial names. The plan required content basenames by default, so the helper and tests were corrected before completion.

## User Setup Required

None.

## Next Phase Readiness

The host wrapper and path helpers are ready for their declared runner and libretro consumers. Their persistence wiring remains in the dependent plans; this plan does not complete GAME-04 by itself.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-11*

## Self-Check: PASSED

- Summary file exists and its required coverage entries pass `uat classify-coverage`.
- All three task commits are ancestors of `HEAD`.
- Full CI and hygiene workflows passed after the final implementation and test changes.
- Stub scan found no TODO/FIXME/placeholder markers or hardcoded empty values flowing to rendering.
