# Phase 8: Per-game card saves - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-11-19
**Phase:** 8-per-game-card-saves
**Areas discussed:** save identity and naming, wrapper compatibility, host persistence, PocketStation storage protocol

---

## Save identity and naming

| Option | Description | Selected |
|--------|-------------|----------|
| Serial-only | Use a serial when available and fail when it is missing or invalid. | |
| Serial with content fallback | Keep serial naming opt-in for the runner, freeze an M3U set identity, and use a versioned full-content digest when identity is missing. | ✓ Recommended |
| Filename-derived | Use the content filename or playlist text to choose the card name. | |

**Auto selection:** Serial with content fallback (recommended default).
**Notes:** Preserve DEC.67: libretro uses frontend content basenames. Keep explicit `--serial` literal for compatibility. For an M3U, use the first member's normalized serial and freeze it across swaps; if unavailable, hash every declared member's canonical media content. The adversarial pass rejected filename/path hashing, boot-EXE-only hashing, or a short disc prefix because those can change on moves or collide for different games. A valid serial can still be reused by distinct revisions; this follows the settled serial policy rather than inventing title grouping.

## Card wrapper compatibility

| Option | Description | Selected |
|--------|-------------|----------|
| Raw images only | Keep the current raw-card format and avoid wrapper logic. | |
| Raw plus validated `.gme`, `.mem`, and `.psx` full-card wrappers | Preserve short raw compatibility, validate wrapper magic/size/bounds, and export each supported wrapper explicitly. | ✓ Recommended |
| Extension-only recognition | Trust the suffix and accept a wrapper without checking its contents. | |

**Auto selection:** Raw plus validated full-card wrappers (recommended default).
**Notes:** Preserve all canonical card bytes. Reject malformed recognized formats and make sure failed imports cannot be flushed over the existing file. `.psx` is ambiguous, so only a confirmed full-card format is supported. The domain researcher must confirm exact header fields from an independent primary source or pinned provenance-cleared data before vectors are finalized. Test full-size equality and preserve a sentinel destination on failure.

## Host persistence and recovery

| Option | Description | Selected |
|--------|-------------|----------|
| One shared path policy | Force serial names across runner and libretro slots. | |
| Split by host contract | Runner uses opt-in serial naming; libretro keeps frontend-managed card 1 SAVE_RAM and a separate card-2 save-directory file. | ✓ Recommended |
| In-process checks only | Prove adapter behavior without restarting RetroArch. | |

**Auto selection:** Split by host contract with a fresh-process RetroArch round trip (recommended default).
**Notes:** This follows DEC.62 and DEC.67. Set libretro card-2 paths for executable, disc, and M3U content. A failed import must not arm persistence. Keep the isolated save directory across two RetroArch processes; verify card 1 reinjection and card 2 reload separately. Set `libretro_saves` to `"true"` only after both in-process and hosted checks pass.

## PocketStation storage protocol

| Option | Description | Selected |
|--------|-------------|----------|
| Plain card only | Keep PocketStation commands unsupported, as older preparation text suggested. | |
| Bounded storage protocol | Implement deterministic `58h`–`5Fh` command handling with validated storage transfer only and no handheld peripherals or code execution. | ✓ Recommended |
| Full PocketStation | Add the ARM CPU, BIOS, display, clock, audio, and link behavior. | |

**Auto selection:** Bounded storage protocol (recommended default; scope locked by SAVE-05).
**Notes:** The newer v2.0 requirement supersedes the older placement of the command family as a later seed only for storage protocol work. Preserve ordinary memory-card transactions. Data transfers are limited to documented flash/file storage and accepted bounds; CPU-dependent or peripheral operations return deterministic unsupported results. Exact command bytes, ACK timing, write-commit timing, and valid ranges must be checked against the primary protocol source before implementation. No wall-clock reads are allowed.

## Cheap preflight and adversarial pass

The security lens prioritizes bounded parsing, path-safe names, staging before mutation, and preserving existing saves on every failure. The product lens prioritizes recognizable serial names, stable M3U behavior, and no lost progress. The architecture lens keeps file work in `host/`, card protocol in the instance-owned core, and libretro card 1 under frontend control. The devops lens keeps the existing inventories and two-launch test wired, and checks the 300-line harness limit before a hosted run. The PS1 bus lens checks command phases, ACKs, transfer limits, and commit only on completed writes. The C/build lens favors current CMake inventories and host helpers, with no new dependency.

The adversarial pass found these failure paths to test early: two discs sharing a boot prefix but differing later, a changed or reordered M3U, invalid and truncated wrapper signatures, a valid save next to a rejected import, failed flush/rename, card 1 and card 2 values that could alias, and interrupted PocketStation writes. It also noted that full-media identity hashing has a startup cost; measure it once and keep the full-content identity guarantee. A cheap consistency preflight must catch inventory, selector, fixture, workflow, metadata, and budget mismatches before broad CTest/CI or hosted RetroArch.

### the agent's Discretion

- Private state layout, host helper boundaries, test fixture choices, and dependency-aware plan waves.
- Whether to extend or copy `memcard_roundtrip.asm`, provided both cards and fresh-process persistence are proved.
- The exact deterministic unsupported response for non-storage PocketStation operations, after checking the primary command reference.

## Deferred Ideas

- No CPU/BIOS/LCD/RTC/audio/IrDA emulation.
- No libretro serial naming or guessed title grouping.
- Main/Release CI artifact-reuse research stays bound to Phase 9. The broad todo matcher results for unrelated CI, CPU, GTE, CD, and build tasks were reviewed and not folded into this fixed phase.

---

*Phase: 8-per-game-card-saves*
*Discussion mode: --auto; all four gray areas selected and the recommended default chosen for each.*
