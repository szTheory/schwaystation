---
phase: 14-multitap-and-special-controllers
reviewed: 2026-11-24T13:33:15Z
depth: standard
files_reviewed: 6
files_reviewed_list:
  - docs/hardware/sio.md
  - docs/runner.md
  - runner/main.c
  - src/sio.c
  - tests/core/test_state.c
  - tests/runner/tests.cmake
findings:
  critical: 1
  warning: 1
  info: 0
  total: 2
  necessity: 0
status: issues_found
---

# Phase 14: Code Review Report

**Reviewed:** 2026-11-24T13:33:15Z
**Depth:** standard
**Files Reviewed:** 6 (incremental scope)
**Status:** issues_found

## Summary

This incremental review covered the six files returned by the phase-scope resolver for changes since `f033365`. The resolver returned `degraded` (`no-task-commits-since`), but its usable file list matched the requested scope and every file was present. The original standard-depth review covered 45 files and recorded two critical issues and two warnings. All four fixes are recorded in [14-REVIEW-FIX.md](14-REVIEW-FIX.md). The current changes fix the long-reply index bound, restore legacy movie input on tapped socket A, and update card-socket documentation. This re-review found one reachable short-reply state that the new bound rejects and one stale card-naming description. No tests were run for this static re-review.

## Initial Review Record (historical; fixed)

The initial 45-file review recorded these findings; the fix report documents each resolution and its commit:

- **CR-01:** Accepted multitap state could reach an out-of-range shift — fixed in `cc6eeb1`.
- **CR-02:** Runner input movies stopped controlling socket A when a tap was enabled — fixed in `b88bc11`.
- **WR-01:** SIO documentation described only two card slots — fixed in `ee834d8`.
- **WR-02:** The runner `--save-dir` summary promised two card files — fixed in `ea7ab0b`.

See [14-REVIEW-FIX.md](14-REVIEW-FIX.md) for the fix descriptions and recorded verification. Those four findings are historical and are not included in the current finding counts below.

The original review scope was:

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

## Narrative Findings (AI reviewer)

### Critical Issues

#### CR-03: Short-reply index bound rejects a reachable save state

**Severity:** BLOCKER  
**File:** `src/sio.c:40-41, 53-56, 575-588`; `tests/core/test_state.c:605-610`
**Issue:** After processing the short reply's second byte at index 2, `tap_byte` ACKs it and increments `tap_tx_index` to 3. Index 3 is the next live transaction position; processing that byte returns the slot ID and finishes the transaction. The new `TAP_SHORT_LAST_INDEX = 2` bound rejects index 3 during state validation, even though `schway_save_state` can serialize this reachable in-flight state. Loading that saved state therefore returns `SCHWAY_STATUS_ERR_STATE_FIELD` instead of resuming the transfer. The new malformed-state test also labels index 3 as past-terminal, cementing the off-by-one error.
**Fix:** Allow the terminal pending-byte position, index 3, for `TAP_SHORT`; reject index 4 instead. Add a save/load roundtrip at the reachable short-reply index 3, while retaining the checksummed invalid-state test at index 4.

### Warnings

#### WR-03: Runner naming option still promises two card files

**Severity:** WARNING  
**File:** `docs/runner.md:48-50, 192-197`
**Issue:** The `--save-dir` option now correctly says the runner loads and saves all eight sockets, and the detailed section says all eight files are imported. The adjacent `--memcard-names` row still describes naming “both card files,” so users can reasonably infer serial naming covers only the two legacy socket-A files even though it supplies the basename for all eight per-port/socket paths.
**Fix:** Change “both card files” to “all eight card socket files” (or equivalently say the naming mode selects the basename used for all eight socket paths).

---

_Reviewed: 2026-11-24T13:33:15Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
