---
phase: 04-games-play-in-retroarch
reviewed: 2026-11-13T03:43:20Z
depth: standard
files_reviewed: 36
files_reviewed_list:
  - host/cards.c
  - host/cards.h
  - host/disc_probe.c
  - host/disc_probe.h
  - host/sha1.c
  - host/sha1.h
  - host/xrgb8888.c
  - host/xrgb8888.h
  - include/schwaystation.h
  - libretro/CMakeLists.txt
  - libretro/libretro.c
  - libretro/schwaystation_libretro.info.in
  - runner/CMakeLists.txt
  - runner/exit_status.h
  - runner/frame_hash.c
  - runner/frame_hash.h
  - runner/inputs.c
  - runner/inputs.h
  - runner/probe.c
  - runner/probe.h
  - runner/sha256.h
  - runner/tty.h
  - src/bios.c
  - src/bios.h
  - src/bios_table.inc
  - src/card.c
  - src/card.h
  - src/exe.c
  - src/instance.c
  - src/internal.h
  - src/pad.c
  - src/pad.h
  - src/sio.c
  - src/sio.h
  - src/state.c
  - src/state.h
findings:
  critical: 2
  warning: 0
  info: 0
  total: 2
  necessity: 0
status: issues_found
---

# Phase 04: Code Review Report

**Reviewed:** 2026-11-13T03:43:20Z
**Depth:** standard
**Files Reviewed:** 36
**Status:** issues_found

## Summary

Reviewed the 36 frozen stable-production source files (7,916 lines) listed in `/private/tmp/phase4-review-stable/files.txt`, from review baseline `8c3bdcb7aad20c4e8c5bb58d1974dc48b697f7d7` (phase diff base `abf58b394f0d1eac6416904f3b67c227d5e965f0`). The review used the byte-identical snapshot at `/private/tmp/phase4-review-stable/snapshot`; `manifest.json` records each file's Git blob and SHA-256. Two production defects were confirmed: changing the libretro region invalidates the loaded game's SAVE_RAM span before unload, and save states omit the pending card write-error status. BIOS identity is validated by the bus state walker at the frozen review baseline, so the earlier BIOS finding was retracted after checking that dependency at the correct baseline.

This report covers only the frozen stable-production stage. GPU/kernel/CPU/bus/DMA/runner-main late work and changed-file deltas after the freeze remain for the later review; this is not a phase-wide clean or completion report. No source files were changed and no tests, builds, or applications were run.

## Critical Issues

### CR-01: Changing region invalidates the live SAVE_RAM span before game unload [BLOCKER]

**File:** `libretro/libretro.c:918` (frozen Git blob `7e1f7e9e04c19da1aee397a5e5b4508d768342ca`)
**Issue:** The adapter immediately applies a changed `schwaystation_region` option during `retro_run` (`libretro/libretro.c:902-923`). It calls `lr_create`, which calls `lr_unload` (`libretro/libretro.c:350-386`) and destroys the active instance before the frontend calls `retro_unload_game`; `RETRO_MEMORY_SAVE_RAM` is that instance's card-1 span (`libretro/libretro.c:1103-1123`). The replacement instance has a newly initialized blank card, and the core clears its memory map and card-2 path during the internal unload. This violates the public promise that the SAVE_RAM pointer remains valid until unload or deinit (`docs/embedding.md:351-353`) and the settled load-to-unload lifetime policy. It also loses the loaded PS-EXE or no-content BIOS: `retro_load_game` consumes/frees those bytes (`libretro/libretro.c:1010-1078`), while the region path does not reload them, so the next frame starts the new instance's no-content path. A frontend can trigger this with `GET_VARIABLE_UPDATE` true and a new region value while a game is loaded.

**Fix:** Preserve the active content and both card images/path state across the region restart, and keep the existing SAVE_RAM span valid through actual game unload; or make the option explicitly take effect only on the next game load and do not destroy the current instance on a runtime update.

### CR-02: Save states do not restore a pending card write-error flag [BLOCKER]

**File:** `src/card.c:175-198` (frozen Git blob `8fe83415134fd770d426791bbb5f89e2c506836c`)
**Issue:** `slot_state_io` serializes command phase and pending transfer fields but omits `slot->flags`. The `0x04` write-error bit is transient protocol state: out-of-range writes and bad checksums set it (`src/card.c:346,364-366`), and the next supported command returns then clears it (`src/card.c:226-242`). A minimal sequence is: save state while the bit is clear; execute a bad-checksum or invalid-sector write; load the earlier state; issue a supported command. The error bit remains set because APPLY never restores it, so the guest receives `0x04` when the saved state says no error. D-14 and the state docs exclude persistent card bytes/generations while including pending SIO/card protocol (`docs/embedding.md:474-476`, `04-34-SUMMARY.md`), so excluding the card image does not account for this missing transient status. The separate `0x08` fresh-card bit is tied to card metadata and should remain with the receiver's persistent image when state is restored.

**Fix:** Serialize the pending `0x04` write-error status for each slot and apply it without replacing the receiver's `0x08` fresh-card bit, card bytes, or generation.

---

_Reviewed: 2026-11-13T03:43:20Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_

## Runtime validation and repair handoff

The retained card-error and region-loss findings have real caller failures. The card proof uses actual JOY transactions/public state calls and preserves receiver metadata controls; the region proof uses the actual module, generated content and empty temporary frontend directories. Both repair candidates have affected native RED/GREEN checks off-tree; they are not applied or accepted as final source yet. BIOS binding is retracted, with no new BIOS field needed. Exact frozen source is the behavior authority; phase start identifies the diff only.
