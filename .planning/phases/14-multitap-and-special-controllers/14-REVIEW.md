---
phase: 14-multitap-and-special-controllers
reviewed: 2026-11-24T13:12:58Z
depth: standard
files_reviewed: 45
files_reviewed_list:
  - docs/compatibility.md
  - docs/controllers.md
  - docs/embedding.md
  - docs/hardware/sio.md
  - docs/runner.md
  - docs/saves.md
  - docs/testing.md
  - host/cards.c
  - host/cards.h
  - include/schwaystation.h
  - libretro/libretro.c
  - quality.json
  - runner/main.c
  - src/card.c
  - src/card.h
  - src/frame.c
  - src/instance.c
  - src/internal.h
  - src/pad.c
  - src/pad.h
  - src/sio.c
  - src/sio.h
  - src/state.h
  - tests/abi/inventory.txt
  - tests/abi/tests.cmake
  - tests/api/v9.c
  - tests/card/card_tests.c
  - tests/card/files.c
  - tests/card/inventory.txt
  - tests/card/tests.cmake
  - tests/compat/features.csv
  - tests/compat/tests.cmake
  - tests/core/test_core.c
  - tests/core/test_state.c
  - tests/disc/tests.cmake
  - tests/fixtures/tests.cmake
  - tests/libretro/host.c
  - tests/libretro/inventory.txt
  - tests/libretro/tests.cmake
  - tests/runner/inventory.txt
  - tests/runner/tests.cmake
  - tests/sio/inventory.txt
  - tests/sio/pad_tests.c
  - tests/sio/sio_tests.c
  - tests/sio/tests.cmake
findings:
  critical: 2
  warning: 2
  info: 0
  total: 4
  necessity: 0
status: issues_found
---

# Phase 14: Code Review Report

**Reviewed:** 2026-11-24T13:12:58Z  
**Depth:** standard  
**Files Reviewed:** 45  
**Status:** issues_found

## Summary

The review covered the resolver's 33 `files` entries and all 12 `outsideUnion` entries, for 45 distinct paths; all were present on disk. The resolver marked `14-11-SUMMARY.md` unparsed. That summary exists and was read directly, so its source changes were still accounted for. The main concerns are a malformed multitap state that can reach undefined behavior and runner input movies losing socket A controls when a tap is enabled. Two documentation pages also retain outdated card counts. No tests were run as part of this read-only review.

## Critical Issues

### CR-01: Accepted multitap state can reach an out-of-range shift

**Severity:** BLOCKER  
**File:** `src/sio.c:244, 301-321, 595-618`; `src/state.c:247-268`  
**Issue:** The state reader accepts `sio.tap_tx_index` through 35, while a Method 1 long reply completes at index 34. The cross-field checks require a non-idle mode, nonzero index, enabled tap, same physical port and Slot A in the mask, but impose no mode-specific index bound. A checksummed state with long mode and index 35 therefore passes validation and is applied. Further transmitted bytes never finish that transfer; when the 8-bit index wraps, `index - 3u` underflows and the handler evaluates `1u << socket` with a shift count greater than the width of `unsigned`, which is undefined behavior.
**Fix:** Reject impossible mode/index combinations during state validation. At minimum, cap the long-reply index at 34; add mode-specific bounds so other transaction modes cannot resume beyond their terminal byte.

### CR-02: Runner input movies stop controlling socket A when a tap is enabled

**Severity:** BLOCKER  
**File:** `runner/main.c:1873-1875`; `runner/inputs.c:129-169`; `src/pad.c:148-166`  
**Issue:** The runner movie parser writes the existing `p1` and `p2` grammar into the legacy `device`, `buttons`, `axes` and `analog_button` fields. The runner then sets `input_snapshot.multitap`, but leaves `controllers[port][0]` zeroed. When a tap is enabled, `schway__pad_frame_start` takes socket A's device and controls only from that zeroed endpoint entry and ignores the legacy fields. The corresponding movie-controlled player is therefore disconnected for each tapped port.
**Fix:** Preserve the current two-port movie contract by copying each legacy port snapshot into `controllers[port][0]` whenever that port's tap is enabled, before calling `schway_set_input`. Keep the other sockets disconnected until the runner has an input format for them.

## Warnings

### WR-01: SIO documentation still describes only two card slots

**Severity:** WARNING  
**File:** `docs/hardware/sio.md:3-5, 108-111`  
**Issue:** The page says there is one card per port and later says “Both slots” own separate card spans, but Phase 14 adds four card sockets per port and routes addresses `81h`–`84h` when a tap is enabled. Readers of this protocol page are not told that the additional card endpoints exist.
**Fix:** Describe all eight physical port/socket card endpoints and clarify that the existing direct card address selects socket A when taps are disabled.

### WR-02: Runner save-directory option still promises two card files

**Severity:** WARNING  
**File:** `docs/runner.md:48`  
**Issue:** The option table says `--save-dir` loads and saves “the two memory cards,” while the same page's card section documents importing all eight socket files. The option summary gives users an outdated description of the runner's persistence behavior.
**Fix:** Change the option summary to say it loads and saves all eight card sockets under the directory.

---

_Reviewed: 2026-11-24T13:12:58Z_  
_Reviewer: the agent (gsd-code-reviewer)_  
_Depth: standard_
