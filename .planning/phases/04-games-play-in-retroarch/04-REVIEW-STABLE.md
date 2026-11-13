---
phase: 04-games-play-in-retroarch
reviewed: 2026-11-13T03:27:57Z
depth: standard
review_stage: stable-production
phase_review_complete: false
disposition: pending_validation
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
  critical: 3
  warning: 0
  info: 0
  total: 3
  necessity: 0
status: issues_found
---

# Phase 04: Code Review Report

**Reviewed:** 2026-11-13T03:27:57Z  
**Depth:** standard  
**Files Reviewed:** 36  
**Status:** issues_found

## Summary

Reviewed the 36 frozen stable-production source files (7,916 lines) listed in `/private/tmp/phase4-review-stable/files.txt`, from phase base `abf58b394f0d1eac6416904f3b67c227d5e965f0`. The review used the byte-identical snapshot at `/private/tmp/phase4-review-stable/snapshot`; `manifest.json` records each file's Git blob and SHA-256. Three production defects were confirmed: BIOS identity is not checked when loading a state, changing the libretro region silently discards the loaded content and slot-1 card, and save states omit observable card status flags.

This report covers only the frozen stable-production stage. GPU/kernel/CPU/bus/DMA/runner-main late work and changed-file deltas after the freeze remain for the later review; this is not a phase-wide clean or completion report. No source files were changed and no tests, builds, or applications were run.

## Critical Issues

### CR-01: BIOS identity is not validated when loading a state [BLOCKER]

**File:** `src/state.c:107-108`
**Issue:** State serialization records `exe_loaded` and then visits bus state, but the baseline bus state walker explicitly excludes the host BIOS; `bus.bios_sha256` and `bus.bios_loaded` are never visited. This contradicts the public contract in `docs/embedding.md:417-418`, which says a state records BIOS presence and SHA-256 and rejects a state from a different BIOS before mutation. Consequently, a same-region state made with BIOS A is accepted by an instance using BIOS B, which continues to read BIOS B and can diverge as soon as guest execution reaches firmware. `schway__state_match_u8_array` in `src/state.h:266-289` already supports validating a fixed identity without replacing the receiving instance, but the state body does not use it.

**Fix:** Add BIOS-loaded identity and digest to the state visitor and size/write/validate/apply passes. Validate both against the receiving instance before any mutation, and advance over them without copying the identity during apply. Keep the approved state format/version policy aligned with the added fields.

### CR-02: Changing the libretro region drops the loaded content and slot-1 card [BLOCKER]

**File:** `libretro/libretro.c:918`
**Issue:** When the user changes the advertised Region option, `lr_check_region` calls `lr_create(region)`. `lr_create` first calls `lr_unload` (`libretro/libretro.c:381-386`), which destroys the active instance and clears the card-2 path. The new instance is only profiled; the loader does not retain or reload the current PS-EXE or BIOS, so the next frame runs the no-content test-card path. The slot-1 card is the instance-owned `RETRO_MEMORY_SAVE_RAM` span, so destroying the instance also replaces the frontend's live card image with a blank one. This occurs while a game is loaded and turns the documented “restarts the console” option into an unload plus persistent-card loss.

**Fix:** Preserve the current content source and both card images/path state across the region recreation, load them into the new instance before re-registering host memory, and restore the content before the next frame. Alternatively, reject/defer a region change while content is loaded unless the adapter can perform that restart without losing it.

### CR-03: Save states omit the card protocol status flags [BLOCKER]

**File:** `src/card.c:175-198`
**Issue:** `slot_state_io` serializes the command phase and pending transfer fields but not `slot->flags`. Those flags are guest-visible: `begin_command` returns them for read/ID/write commands (`src/card.c:226-242`), and invalid-sector or bad-checksum writes set the error bit (`src/card.c:346-366`), which is cleared only when reported. Loading a state captured before such an error leaves the receiver's newer flags in place, so the guest receives a response that does not match the saved protocol state. The public state contract says pending SIO/card protocol is included (`docs/embedding.md:474-476`), and the preparation's state inventory explicitly includes the card flag.

**Fix:** Serialize and validate each slot's protocol flags with the card state, preserving the existing exclusion of card bytes and generations. Limit accepted flag bits to those implemented by this card model.

---

_Reviewed: 2026-11-13T03:27:57Z_  
_Reviewer: the agent (gsd-code-reviewer)_  
_Depth: standard_
