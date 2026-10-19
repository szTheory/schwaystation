---
id: SEED-006
status: dormant
planted: 2026-10-19
planted_during: preparation, before milestone v1.0
trigger_when: v1.0 has shipped with the native frame hashed on every lane, and players ask for internal resolution, PGXP-style precision, widescreen, texture filtering or true colour
scope: large
---

# SEED-006: Enhancements: internal resolution, PGXP, widescreen, texture filtering, true colour, CRT host helpers

## Why This Matters

Players of DuckStation and Beetle PSX HW expect upscaling, PGXP geometry precision and widescreen, and also report
that PGXP breaks cutscenes and that overclock desyncs audio. schwaystation's product claim is the native frame,
identical on every platform, so every enhancement lives in a host renderer over the core's primitive record stream
and never in the hashed path; `policy.no_enhancement_in_core` rejects their option names in `src/`. v1 keeps the
seams so none of this needs a redesign.

## When to Surface

**Trigger:** v1.0 shipped and a player ask; weighed against the library and states milestones at each boundary.

## Scope Estimate

**Large**: a host renderer (software first, then the libretro hardware-render path) consuming `schway__gpu_prim`
records through a hook: internal resolution scaling with VRAM 1024 × 512 × 16 as the texture and readback source;
PGXP-style precision from the GTE sideband (pre-truncation MAC0/IR and SZ per vertex keyed by the SXY FIFO) and
the `sub_x`, `sub_y`, `w` vertex slots; perspective-correct UV; a widescreen hack over the one OFX/H projection
function; texture filtering through the one fractional-UV texel fetch; true colour through the one 8-bit
truncation point with the dither table as a flag; deinterlacers and CRT helpers host-side over the field bit, parity
and native PAR in the metadata; each option flagged in the replay identity and off by default.

## Breadcrumbs

- `.planning/preparation/PSX-GPU-AND-GTE.md` section 8: the enhancement seeds and the v1 frame format.
- `.planning/preparation/ARCHITECTURE.md` section 9: the rasteriser and the seams it keeps.
- `.planning/preparation/HOSTS-AND-INTEGRATION.md` section 8: feature ownership between host and core.
- `.planning/preparation/PSX-ECOSYSTEM-AND-USERS.md` section 2 (C.02, C.03): the complaints and the not-a-bug list.
- `.planning/preparation/DECISIONS.md` DEC.08 and DEC.32.
