---
id: SEED-008
status: dormant
planted: 2026-10-19
planted_during: preparation, before milestone v1.0
trigger_when: milestone v1.0 has shipped (weigh it against v2.0 at that new-milestone step), or the owner asks for a web build sooner
scope: medium
---

# SEED-008: schwaystation in the browser: an efficient WebAssembly build

## Why This Matters

The owner ranks running efficiently in the browser through WebAssembly as very important. A web page reaches
players who will not install a frontend, and no permissive WASM PS1 core exists (EmulatorJS ships PCSX-ReARMed
under GPL). Efficiency counts twice here: a browser tab on a laptop or phone runs on battery, and the core's work
per frame sets both its speed and its power draw; WASM runs 45 to 55 % slower than native on average. The cheap
half, a wasm32-wasi hash lane under wasmtime, is in v1.0 phase 1.

## When to Surface

**Trigger:** v1.0 shipped, so there is something worth playing; weigh it against v2.0 at that new-milestone step.
Sooner if the owner asks.

## Scope Estimate

**Medium**: one or two phases: an Emscripten build of the core and a ~200-line JS harness (canvas 320 × 240
resized on geometry change, AudioWorklet at 44.1 kHz, keyboard and Gamepad API) on a static page; the player picks
the cue and its bins locally and the page serves sectors through the File API and the core's sector callback with a
prefetch ring, so a 700 MB image is never loaded; the BIOS is a user-picked 512 KiB file read once into WASM memory
and never uploaded, and the page ships no ROM or BIOS (ASSET_POLICY); the test card and the own fixtures as built-in
content; a 32 MB linear memory with no growth and no threads; frame and audio hashes equal to native (the WASI
lane); `size.wasm` and the `-Os` versus `-O2` choice measured in `quality.json`; frame-time and power checks in
Chrome, Firefox and Safari, including on a phone.

## Breadcrumbs

- `.planning/preparation/HOSTS-AND-INTEGRATION.md` section 6.
- `.planning/preparation/ENGINEERING.md` sections 8, 9 and 14: the ratchet, lanes, WASM and low power.
- `.planning/preparation/ARCHITECTURE.md` section 12: memory and 32-bit `size_t` rules.
- `.planning/preparation/PSX-ECOSYSTEM-AND-USERS.md` section 4: work per frame and device budgets.
- `.planning/seeds/SEED-005-everywhere.md`: the other ports and the cached interpreter.

## Notes

No threads means no SharedArrayBuffer, so no COOP/COEP headers, and any static host works. Emscripten is a nightly
compile-only lane and never replaces the WASI hash lane, because its JavaScript glue hides core behaviour.
