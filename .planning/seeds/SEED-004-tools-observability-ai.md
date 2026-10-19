---
id: SEED-004
status: dormant
planted: 2026-10-19
planted_during: preparation, before milestone v1.0
trigger_when: states and movies exist (SEED-002) and a homebrew developer, ROM hacker, TAS author or AI user asks for hooks, a GDB stub, a debugger view, Python bindings or a Gymnasium environment
scope: medium
---

# SEED-004: Tools: hooks, GDB stub, debugger views, ROM-hacker aids, Python and Gymnasium, Lua on demand (v5.0)

## Why This Matters

The PS1 homebrew scene (PSn00bSDK, nugget and psyqo, Psy-Q) builds against PCSX-Redux's debugger and GDB server
and no$psx's TTY, profiler and breakpoints; libretro cores offer none of it. ROM hackers and translators want
symbol-aware traces, a VRAM viewer and a memory API. AI researchers want a vectorised, deterministic environment
with RAM peeks; stable-retro lists no PS1 core. All of it is cheap on a core whose state is one struct, whose loop
is compiled twice (traced and untraced), and whose `schway_run` already returns stop reasons.

## When to Surface

**Trigger:** SEED-002 shipped and a user ask. The inspection API (`schway_peek`, register views, counters, TTY,
first-fault trace) is already in v1.0; this seed adds the rest in YAGNI order.

## Scope Estimate

**Medium**: one or two phases: hooks (instruction, bus, DMA, IRQ, line, frame, TTY) at zero cost when off with
the Cachegrind proof; `--gdb PORT` (RSP over TCP, MIPS32 register layout, software breakpoints, memory read and
write); `--symbols FILE`, `--vram-dump N`, `--trace full`; a memory-card editor in the runner; Python `ctypes`
bindings and a Gymnasium environment (native, rgb and ram observations; 14 buttons and 4 axes; frame skip and
sticky actions in the wrapper); Lua 5.4 in the runner only when asked.

## Breadcrumbs

- `.planning/preparation/HOSTS-AND-INTEGRATION.md` section 5: the scripting ladder.
- `.planning/preparation/ARCHITECTURE.md` section 6: counters, trace format, the hooks ABI.
- `.planning/preparation/PSX-ECOSYSTEM-AND-USERS.md` sections 3.4 and 3.5: AI agents, homebrew developers.
- `.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md` section 6.3: what the library offers hackers.
