---
phase: 14-multitap-and-special-controllers
reviewed: 2026-11-24T15:33:36Z
depth: standard
files_reviewed: 4
files_reviewed_list:
  - docs/runner.md
  - src/pad.c
  - src/sio.c
  - tests/core/test_state.c
findings:
  critical: 0
  warning: 0
  info: 0
  total: 0
  necessity: 0
status: clean
---

# Phase 14: Code Review Report

**Reviewed:** 2026-11-24T15:33:36Z
**Depth:** standard
**Files Reviewed:** 4 (the three-file incremental scope since `cae3772`, plus the GCC ABI gate follow-up in `src/pad.c`)
**Status:** clean

## Summary

The standard-depth review covered `docs/runner.md`, `src/sio.c`, and `tests/core/test_state.c`, the usable file list from the evaluation-scope resolver. The resolver returned `degraded` with reason `no-task-commits-since`; all three files were present and matched the requested scope. The short-reply state at index 3 is now accepted and resumes with the terminal slot ID, while index 4 is rejected. The per-mode transaction bounds still cap all indices used to derive long-reply socket shifts. The runner documentation accurately describes one selected basename across all eight socket paths. The follow-up review also covered `src/pad.c` after hosted GCC reported `d name_watchdogs` in the ABI globals gate. No issues remain in either scope. No tests were run during the static reviews; [14-REVIEW-FIX.md](14-REVIEW-FIX.md) records the targeted tests run for iteration 2.

## Narrative Findings (AI reviewer)

All reviewed files meet quality standards. No issues found.

### GCC ABI gate follow-up (2026-11-24T15:33:36Z)

Reviewed the `name_watchdogs` representation in `src/pad.c` after hosted GCC classified the pointer table as writable
data. The fixed-width constant character rows preserve all eight endpoint names, their NUL terminators and the existing
endpoint-indexed lookups. The array width derives from the `"pad.watchdog0"` literal, so each entry fits without
truncation. No correctness, security or maintainability issue found.

## Historical Review Record

The records below preserve earlier review scopes and findings. Fixed historical findings are not included in the current frontmatter counts.

### Initial standard-depth review

The initial review covered 45 source files and recorded two critical issues and two warnings. All four were fixed as recorded in [14-REVIEW-FIX.md](14-REVIEW-FIX.md):

- **CR-01:** Accepted multitap state could reach an out-of-range shift — fixed in `cc6eeb1`.
- **CR-02:** Runner input movies stopped controlling socket A when a tap was enabled — fixed in `b88bc11`.
- **WR-01:** SIO documentation described only two card slots — fixed in `ee834d8`.
- **WR-02:** The runner `--save-dir` summary promised two card files — fixed in `ea7ab0b`.

The initial review scope was:

- `docs/compatibility.md`
- `docs/controllers.md`
- `docs/embedding.md`
- `docs/hardware/sio.md`
- `docs/runner.md`
- `docs/saves.md`
- `docs/testing.md`
- `host/cards.c`
- `host/cards.h`
- `include/schwaystation.h`
- `libretro/libretro.c`
- `quality.json`
- `runner/main.c`
- `src/card.c`
- `src/card.h`
- `src/frame.c`
- `src/instance.c`
- `src/internal.h`
- `src/pad.c`
- `src/pad.h`
- `src/sio.c`
- `src/sio.h`
- `src/state.h`
- `tests/abi/inventory.txt`
- `tests/abi/tests.cmake`
- `tests/api/v9.c`
- `tests/card/card_tests.c`
- `tests/card/files.c`
- `tests/card/inventory.txt`
- `tests/card/tests.cmake`
- `tests/compat/features.csv`
- `tests/compat/tests.cmake`
- `tests/core/test_core.c`
- `tests/core/test_state.c`
- `tests/disc/tests.cmake`
- `tests/fixtures/tests.cmake`
- `tests/libretro/host.c`
- `tests/libretro/inventory.txt`
- `tests/libretro/tests.cmake`
- `tests/runner/inventory.txt`
- `tests/runner/tests.cmake`
- `tests/sio/inventory.txt`
- `tests/sio/pad_tests.c`
- `tests/sio/sio_tests.c`
- `tests/sio/tests.cmake`

### Prior incremental review (fixed in iteration 2)

The previous incremental review covered six files changed since `f033365`. Its two findings and fixes are preserved here and documented in iteration 2 of [14-REVIEW-FIX.md](14-REVIEW-FIX.md):

- **CR-03:** The short-reply state validator rejected reachable index 3, preventing a saved transfer from loading and resuming. Fixed in `0bcefc2` by allowing index 3 and rejecting index 4; a save/load/resume regression was added.
- **WR-03:** The `--memcard-names` row implied serial naming covered only two card files. Fixed in `d46d644` by stating it chooses the shared basename for all eight card socket paths.

The fix report records `git diff --check`, CI configure/build, and the selected state/SIO/runner tests passing after the test expectation correction. This re-review confirms both fixes against the current source and documentation.

---

_Reviewed: 2026-11-24T15:33:36Z_

_Reviewer: the agent (gsd-code-reviewer)_

_Depth: standard_
