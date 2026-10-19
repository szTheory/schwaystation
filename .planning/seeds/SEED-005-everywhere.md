---
id: SEED-005
status: dormant
planted: 2026-10-19
planted_during: preparation, before milestone v1.0
trigger_when: a port target is named (a handheld, a console homebrew SDK, the libretro buildbot) or the instructions-per-frame ratchet shows a low-power budget is met or missed
scope: large
---

# SEED-005: Everywhere: the cached interpreter and JIT, handhelds, console ports, the libretro buildbot, Playstead (v6.0)

## Why This Matters

The owner wants the core on low-power devices (RG35XX-class A53 handhelds, Raspberry Pi 4, Switch homebrew) with
good battery life, in the libretro buildbot so RetroArch users get it without a manual install, and in Playstead.
A plain interpreter is expected at 25 to 42 M Ir per NTSC frame, which fits an Apple Silicon Mac and is marginal on
an A72 and short on an A53 (device budgets 100 M, 19 M and 12.5 M Ir per frame); the v1 interpreter is written to
the fast-path constraints (pure decode, fixed block ends, one `bus_write` with a dirty-page bitmap, per-block cycle
accounting) so a cached interpreter and a JIT add a block cache and a code generator without touching semantics.

## When to Surface

**Trigger:** a named target, or the ratchet shows a device budget met or missed (ENGINEERING 8 and
PSX-ECOSYSTEM-AND-USERS 4 give the per-device figures).

## Scope Estimate

**Large**: the cached interpreter (block cache keyed by physical PC, invalidated through the dirty bitmap) behind
`core.exec_mode_equivalence` on all eight lanes before it becomes the hashed path; a JIT seed for x64 and arm64
that keeps the integer-only, no-mmap posture off by default; compile-only lanes first (`armhf` nightly, devkitPro
and pspdev containers pinned by digest), then one playable port the owner can try; the buildbot recipe
(`-DLIBRETRO_STATIC=ON`, prefixed globals, the libretro-core-info `.info` PR, then asking RetroAchievements to list
the core); the Playstead `psx` adapter pin with the two-artifact RetroArch launch.

## Breadcrumbs

- `.planning/preparation/PSX-ECOSYSTEM-AND-USERS.md` section 4: work per frame and device budgets.
- `.planning/preparation/ENGINEERING.md` sections 8, 9, 10 and 14: the ratchet, portability lanes, the fast-path
  constraints, WASM and low power.
- `.planning/preparation/ARCHITECTURE.md` sections 8 and 12: the execution engine and performance posture.
- `.planning/preparation/HOSTS-AND-INTEGRATION.md` sections 1 (buildbot) and 4 (Playstead).
