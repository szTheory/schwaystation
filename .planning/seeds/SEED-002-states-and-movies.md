---
id: SEED-002
status: dormant
planted: 2026-10-19
planted_during: preparation, before milestone v1.0
trigger_when: milestone v2.0 has shipped and players, speedrunners or Playstead ask for save states, rewind, run-ahead or input movies
scope: large
---

# SEED-002: States, rewind, run-ahead and movies (v3.0)

## Why This Matters

Save states are the first thing a player asks a frontend for; speedrunners and TAS authors need frame-exact
movies, a lag counter, emulated-time output and frame advance; RetroArch's rewind, run-ahead and netplay all need a
fixed-size, deterministic state; Playstead's continuation protocol needs a state that survives a fresh process.
BizHawk is the only TASVideos-accepted PlayStation platform today, so an own format with BK2 import is the route
to verified runs.

## When to Surface

**Trigger:** v2.0 shipped. Sooner if Playstead's `psx` integration lands first and needs continuation.

## Scope Estimate

**Large**: a milestone: the four-pass state visitor over every device (CPU with delay records and I-cache, COP0,
GTE, bus and memory control, DMA, timers, GPU with VRAM and line state, SPU with RAM and voices, CD-ROM with its
FIFOs, INT queue, sector buffer and XA history, MDEC, SIO, pads and cards) frozen into the 64-byte-header format
with CRC-32, disc identity and behaviour revision, validated in full before it is applied, about 3.72 MB and under
the 4,194,304-byte ceiling; cards outside the state by default with `WITH_CARDS` for TAS; fork tests at every
frame; libretro `serialize` with the `deterministic` claim staged behind its tests; an own text movie format with
lag frames (no SIO0 pad poll in the frame) plus BK2 then PJM import; emulated-time print and input display in the
runner; the Playstead continuation adapter (`qualify`, `initial-run`, `continue`); rewind and run-ahead e2e in
RetroArch within the e2e line budget.

## Breadcrumbs

- `.planning/preparation/ARCHITECTURE.md` sections 5 and 6: state layout, header, sizes, observability.
- `.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md` section 4: state constraints that apply from phase 1.
- `.planning/preparation/HOSTS-AND-INTEGRATION.md` sections 1, 4 and 5: `.info` claims, continuation protocol,
  movie formats and imports.
- `.planning/preparation/PSX-ECOSYSTEM-AND-USERS.md` sections 3.2 and 3.3: speedrun and TAS rules.
- `.planning/LESSONS.md` XFER-070: hidden-state checkpoints and omitted-field controls.

## Notes

Keep persistent saves, snapshots and movies as three formats with separate version rules (sibling lesson). A
state with another disc identity loads only with `ERR_STATE_DISC` overridden by a flag.
