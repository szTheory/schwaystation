---
phase: 08-per-game-card-saves
reviewed: 2026-11-19T22:38:51Z
depth: standard
files_reviewed: 16
files_reviewed_list:
  - .github/workflows/ci.yml
  - CMakePresets.json
  - docs/testing.md
  - host/cards.c
  - libretro/libretro.c
  - libretro/schwaystation_libretro.info.in
  - quality.json
  - src/card.c
  - tests/card/card_tests.c
  - tests/cmake/info_claims_self_test.cmake
  - tests/cmake/workflows.cmake
  - tests/fixtures/manifest.txt
  - tests/fixtures/src/pocketstation_storage.asm
  - tests/libretro/host.c
  - tests/libretro/tests.cmake
  - tests/runner/tests.cmake
findings:
  critical: 0
  warning: 0
  info: 0
  total: 0
  necessity: 0
status: clean
---

# Phase 08: Code Review Report

**Reviewed:** 2026-11-19T22:38:51Z
**Depth:** standard
**Files Reviewed:** 16
**Status:** clean

## Summary

The original review at 16:20 covered the Phase 8 plan 08-05 and 08-06 changes, including the libretro card 2 lifecycle, bounded PocketStation address mapping, write staging, state validation, guest transcript, and release-only symbol wiring. It found one test assertion gap; that was fixed and verified below. The current Plan 08-10 review is recorded separately at the end of this report.

## Resolved warning

### WR-01: Partial-write checks can pass after destination corruption

**Severity:** WARNING
**File:** `tests/card/card_tests.c:1107-1116`
**Issue:** The abort and malformed-final-byte cases compare the destination against the complete requested payload with `!= 0`. A bug that publishes only a prefix still passes these checks, so this test does not establish the stated all-or-nothing guarantee.
**Resolution:** `tests/card/card_tests.c` now snapshots the entire destination before each write, compares all bytes after abort and after the invalid final byte, and retains separate generation assertions.

## Resolution verification (2026-11-19)

- `git diff --check` — exit 0.
- `cmake --build --preset ci --target schwaystation-test-card` — exit 0.
- `ctest --preset ci --no-tests=error -R '^card[.]protocol[.]pocketstation_metadata$' --output-on-failure` — exit 0, 1/1 passed.

---

_Reviewed: 2026-11-19T16:20:37Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_

## Plan 08-10 claim update review

**Reviewed:** 2026-11-19T21:37:45Z
**Commits:** `5789c16`, `d4463e7`
**Files reviewed:** `tests/cmake/info_claims_self_test.cmake`, `libretro/schwaystation_libretro.info.in`

The self-test now derives the existing save-claim line from the generated metadata, so its malformed-value case and its grounded and missing-inventory cases continue to run when the shipped claim is true. The production checker maps a true `libretro_saves` value to both registered persistence and control tests, and the test inventory contains both rows. The template change is limited to the claim enabled after the fresh-process evidence. No confirmed issues found in this scope.

## Plan 08-10 findings

None.

---

_Reviewed: 2026-11-19T21:37:45Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_

## Plan 08-10 CI workflow and test-portability review

**Reviewed:** 2026-11-19T22:37:03Z
**Base:** `9bbac37` through `f52ef90`
**Files reviewed:** `.github/workflows/ci.yml`, `CMakePresets.json`, `host/cards.c`, `libretro/libretro.c`, `quality.json`, `tests/cmake/workflows.cmake`, `tests/runner/tests.cmake`, `docs/testing.md`

The early state-size and WASI persistence preflights preserve the full test inventory; the manual perf skip is limited to report-only dispatches without a comparison ref; the Windows file-opening change preserves the existing not-found versus I/O result; and the runner test commands now consistently use the configured cross-compiling emulator. No other confirmed issues found in these changes.

## Resolved warning

### WR-02: Maintainer guide reports the obsolete serialized-state size

**Severity:** WARNING
**File:** `docs/testing.md:603`
**Issue:** The guide reports 3,755,020 serialized bytes, but `quality.json` now sets `state.bytes` to 3,755,328 and records that as the current producer measurement. This presents a stale baseline to maintainers investigating the metric gate.
**Resolution:** The guide now reports 3,755,328 bytes, matching the current producer measurement in `quality.json`.

## Resolution verification (2026-11-19)

- Compared the parsed state byte count from `docs/testing.md` with both `quality.json` bounds; exit 0 (`3755328` equals both values).

---

_Reviewed: 2026-11-19T22:38:51Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
