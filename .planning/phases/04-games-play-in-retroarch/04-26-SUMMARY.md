---
phase: 04-games-play-in-retroarch
plan: "26"
subsystem: compatibility
tags: [local-library, disc-identity, metadata, privacy]
requires:
  - phase: "04-42"
    provides: [bounded CUE/ISO identity probe, DAT and ledger join]
  - phase: "04-01"
    provides: [registered CTest and per-directory inventory]
provides:
  - Local-only library workflow and atomic compatibility report
  - Exact production metadata lookup shared with the host runner
  - Temporary-fixture controls for privacy, invalid inputs and report preservation
affects: [04-28, 04-32, phase-04-verification, GAME-06]
actuals:
  tokens: 15062.25
  tasks: 2
  commits: 1
commits: 1
plan_head_before: e3cd2875ffeb8f66a3ad9c37e1059f95da5dc8df
plan_head_after: bbaac44b9a6c3081d6c0d9a06910540930044d7d
tech-stack:
  added: []
  patterns:
    - Use the existing bounded host probe and exact DAT/ledger identity join for ordinary library scans
    - Keep owner-content scans outside CI and exercise the same runner with temporary metadata fixtures
key-files:
  created:
    - scripts/library.sh
    - tests/compat/library_tests.c
  modified:
    - .gitignore
    - CMakePresets.json
    - docs/runner.md
    - runner/main.c
    - runner/probe.c
    - runner/probe.h
    - tests/cmake/compat-controls.cmake
    - tests/compat/inventory.txt
    - tests/compat/tests.cmake
key-decisions:
  - "Use paired metadata-ledger and metadata-dat inputs through the production host runner so tests and local scans share the same identity decision."
  - "Expose identity only for an exact passing ledger and DAT match; keep boot and play unknown until a disc backend measures them."
  - "Keep the owner library workflow outside ci* and run registered library cases only against isolated temporary fixtures."
patterns-established:
  - "Parse the opt-in local configuration as data without sourcing or evaluating shell text."
  - "Write the report atomically to build/library/results.csv and keep unmatched identity fields redacted."
requirements-completed: []
coverage:
  - id: local-library-report
    description: "The local library workflow scans supported CUE, ISO and standalone raw BIN inputs and writes only its private CSV report."
    requirement: GAME-06
    verification:
      - kind: integration
        ref: "compat.library.synthetic_result"
        status: pass
      - kind: integration
        ref: "compat.library.controls ordinary-mode temporary mirror"
        status: pass
      - kind: other
        ref: "cmake --workflow --preset library"
        status: pass
    human_judgment: false
  - id: exact-metadata-join
    description: "Matched identity rows require the exact serial, boot MD5, track SHA-1, DAT title and passing ledger row; boot and play remain unknown."
    requirement: GAME-06
    verification:
      - kind: integration
        ref: "compat.library.synthetic_result"
        status: pass
      - kind: integration
        ref: "compat.library.controls"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci"
        status: pass
    human_judgment: false
  - id: private-failure-safe-output
    description: "Malformed configuration and supported inputs are refused without leaking private paths or replacing the prior report; empty and unlisted inputs produce no identity rows."
    requirement: GAME-06
    verification:
      - kind: integration
        ref: "compat.library.controls"
        status: pass
      - kind: other
        ref: "cmake --workflow --preset hygiene"
        status: pass
    human_judgment: false
duration: 86min
completed: 2026-11-12
status: complete
---

# Phase 4 Plan 26: Private Local Compatibility Library Summary

**A local-only library workflow now reports exact disc metadata through the production runner, with private unmatched results and failure-safe CSV output.**

## Performance

- **Duration:** 86 minutes of active execution across the original run and approved continuation; the resource-approval wait is excluded.
- **Started:** 2026-11-12T07:10:48Z
- **Completed:** 2026-11-12T13:42:30Z
- **Tasks:** 2
- **Files modified:** 11 implementation and test files
- **Runtime/model:** Codex OpenGSD 1.20.0, gpt-6-luna xhigh

## Accomplishments

- Added an opt-in `library` preset and `scripts/library.sh`. Ordinary mode reads the ignored `.env.local` as data, checks both configured directories, runs the local workflow, and writes its ignored report atomically.
- The script checks the pinned libretro-database DAT SHA-256 before using a downloaded copy. It probes CUE and ISO images plus standalone raw BINs, skipping BIN companions named by CUE sheets.
- Extended the production runner with a paired metadata-ledger/DAT lookup path. A disc's serial, rcheevos MD5, physical track SHA-1 and title appear only after the exact identity row passes; boot and play remain explicitly unavailable.
- Registered `compat.library.synthetic_result` and `compat.library.controls` with the compatibility inventory. Temporary fixtures exercise exact identity, empty input, duplicate CUE/BIN representations, malformed configuration and images, bad joins, privacy, and preservation of the previous report.
- Reused the existing compatibility documentation controls, reducing that helper by 10 lines. The resulting Phase 4 repository-check total is 2,448 lines, within the owner-approved 2,700-line ceiling; no gate logic was moved or minified.

## Task Commits

The recovered candidate contained both plan tasks together, so the verified implementation and required controls landed in one source commit:

1. **Tasks 1–2: local library implementation and controls** — `bbaac44` (`feat`)

The final metadata commit records the summary, checkpoint resolution and state/roadmap updates.

## Files Created/Modified

- `scripts/library.sh` — opt-in local scan, exact ledger joins, redacted unmatched rows and atomic report replacement.
- `runner/main.c`, `runner/probe.c`, `runner/probe.h` — paired production metadata inputs through the bounded host probe.
- `tests/compat/tests.cmake`, `tests/compat/library_tests.c`, `tests/compat/inventory.txt` — registered synthetic identity and refusal controls.
- `CMakePresets.json`, `.gitignore`, `tests/cmake/compat-controls.cmake`, `docs/runner.md` — local-only workflow, shared helper reuse, ignored output and setup documentation.

## Decisions Made

- Reuse the production runner's bounded host lookup for ordinary scans and isolated tests, avoiding a second metadata implementation.
- Keep raw disc identity private when the DAT or passing ledger join is incomplete. Unsupported media is counted without being assigned a disc identity.
- Keep boot and play status unknown until a real disc backend provides those measurements.

## Deviations from Plan

**1. Approved budget update:** The original plan text named an 1,850-line ceiling. The owner approved 2,700 lines for Phase 4 after reviewing complete gate accounting. This plan's measured 2,448-line total preserves all checks within that approved limit.

**2. Resumed commit sequence:** The verified candidate arrived as a complete off-tree patch containing both tasks. The prior RED evidence was genuine but uncommitted. The implementation and expansion controls were committed together after the focused tests passed; no synthetic RED commit or history rewrite was created.

## TDD Gate Compliance

- The original `compat.library.synthetic_result` RED ran through CTest and exited 8 because the planned report behavior was absent. The target test executed and failed on its assertion. The captured evidence passed `gsd check tdd-red-evidence` with `RED_EVIDENCE_OK`.
- The RED report remained uncommitted in ignored build output. The complete candidate was then verified in isolation and the focused tests passed again after application.
- No separate `test(04-26)` commit precedes the feature commit. The only source commit is `bbaac44`, which records both tasks together, matching the recovered history.

## Evidence

- Original RED: `ctest --test-dir build/ci -R '^compat\.library\.synthetic_result$'` — exit 8; one target assertion failed because the planned library command was absent. The evidence classifier returned `RED_EVIDENCE_OK`.
- Focused applied-patch verification: `cmake --preset ci && cmake --build --preset ci --target schwaystation-run schwaystation-test-library && ctest --test-dir build/ci -R '^compat\.library\.(synthetic_result|controls)$' --no-tests=error --output-on-failure` — exit 0, 2/2 selected cases passed.
- Full CI: `cmake --workflow --preset ci` — exit 0, 497/497 CI tests and 5/5 release checks passed.
- Hygiene: `cmake --workflow --preset hygiene` — exit 0, 23/23 checks passed, including the repository-check budget.
- Additional checks: shell syntax, preset JSON parsing, byte-sorted compatibility inventory, compatibility-doc freshness and `git diff --check` — exit 0.
- The controls use temporary fixture trees and the real production runner. The ordinary-mode mirror substitutes only the CMake workflow command to avoid rebuilding there; it verifies the actual scan and lookup path and that only its report changes.
- The separate `library` workflow was also run against temporary fixtures and passed. No owner `.env.local`, BIOS or disc content was read.

## Threat Flags

| Flag | File | Description |
|------|------|-------------|
| threat_flag: network_fetch | scripts/library.sh | When the pinned DAT is absent, ordinary mode downloads it from the pinned upstream revision and verifies its SHA-256 before use. |
| threat_flag: explicit_metadata_file_read | runner/probe.c | The host runner accepts two caller-supplied bounded lookup files only as a pair; identity is exposed only after the exact DAT and passing-ledger match. |

## User Setup Required

None for CI. Ordinary local scans are opt-in and their configuration is documented in `docs/runner.md`.

## Self-Check: PASSED

- Implementation and test files listed above exist.
- Source commit `bbaac44` is present and is an ancestor of `plan_head_after`.
- Full CI, hygiene and focused library checks passed.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-12*
