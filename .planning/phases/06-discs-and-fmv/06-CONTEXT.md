# Phase 6: Discs and FMV - Context

**Gathered:** 2026-11-15
**Status:** Ready for planning

<domain>
## Phase Boundary

Deliver DISC-01 through DISC-04: a cycle-driven CD-ROM controller; bounded host-side cue/bin, ISO and m3u support; XA and CD-DA audio through the SPU; integer MDEC decoding through the DMA and GPU display path; and the owned `disc_boot` fixture booting through OpenBIOS in the library, runner and libretro host flows. Keep the existing three deliverables and deterministic, bounded, integer-only core. CHD, PBP, ECM, CCD and full LibCrypt support remain outside v1.0 per DEC.43.

</domain>

<decisions>
## Implementation Decisions

### CD-ROM timing and command behavior
- Follow DEC.44's deterministic CPU-cycle model: 451,584 cycles per 1x sector, fixed documented response times, and the integer square-root seek model in 1/75-second ticks. Do not add random seek jitter, host-clock timing or speed-up behavior.
- Follow DEC.45 and DEC.47 for region/licence answers, lid-open INT5, command/interrupt sequencing, ADPCM-before-data delivery, and the bounded sector-buffer visibility rules.
- Keep uncertain silicon timing claims distinct from the selected baseline. The fixture and pinned logs test their named behavior; a later measured-accuracy pass can revisit the seek curve with new hardware evidence.

### Disc interface and supported images
- Keep file access and hostile image parsing outside the deterministic core. The host supplies sectors and TOC through `schway_disc` callbacks, including lid, swap and bad-Q behavior; only run calls may request sectors.
- Implement one shared host helper for cue/bin, ISO and m3u. Apply the hostile-input bounds and path containment rules in PER 7; refuse `.chd`, `.pbp`, `.ecm` and `.ccd` with the named unsupported-media status/message.
- Add only the public disc API needed by these consumers, following DEC.75's per-phase API growth and keeping API v1-v4 callers covered. Use the owned fixture's `bad_q` data to exercise the callback without broadening the v1 image parser into a general SBI/LibCrypt feature.
- For m3u in libretro, use `SET_DISK_CONTROL_EXT_INTERFACE`: eject opens the lid, insert swaps the selected image and TOC, and a closed lid rejects `schway_swap_disc`. Do not add a second legacy control surface without a named requirement.

### XA and CD-DA audio
- Feed sector-decoded XA and CD-DA through a bounded queue into the existing 44.1 kHz SPU output path; underflow produces silence. Preserve the fixed 768-cycle SPU sample cadence and the runner/libretro PCM equality contract.
- Use DEC.39's integer seven-phase, 29-tap XA resampler and reset its six-step phase to zero at power-on. Keep XA history across sectors, apply the ATV matrix and SPU AVOL at the documented point, and continue audio work when the SPU output is muted.
- Test XA and CD-DA independently and together. Resolve routine playback details such as double-speed CD-DA consumption in focused tests; do not reopen the selected resampler without a hardware capture that supports the alternative.

### MDEC, fixture and evidence
- Use DEC.33's integer two-pass IDCT, int16 Q14 scale table, Q16 color coefficients and 1,000-cycle block cost baseline. Hash raw MDEC output before DMA1 reordering; require exact 4-bit/8-bit reference blocks and the documented ±1 per-channel frame/movie tolerance.
- Wire the FMV path through CD DMA3 → RAM → MDEC DMA0 → RAM via DMA1 → GPU DMA2, then verify native 24-bit display output. Fetch and run each pinned real oracle as soon as its first caller works; keep direct block tests as fast arithmetic and malformed-input checks, not a replacement for hardware references.
- Build `disc_boot` in the build tree from owned sources with mkpsxiso. Keep it under the 512 KiB build cap, keep its generated image out of git and release archives, and include no BIOS, Sony licence material or commercial bytes.
- Keep every behavior test in the exact inventory and CI; reserve pinned ps1-tests reference comparisons for their intended nightly lane. The hosted RetroArch smoke runs in the background and compares its image with the runner's output.

### Folded Todos
- **Select the 40-clock CD-ROM DMA cost:** connect the device delay setting to DMA channel 3 and test the documented 24- and 40-cycle cases when the CD-ROM device endpoint is implemented.
- **Add the vendored-file digest check:** add `libretro.vendored` in the plan that vendors the pinned rcheevos `rc_hash_disc.c` used for the disc identity hash; retain its notice and a negative control.

### the agent's Discretion
- Choose private struct fields, module helper boundaries, test-file splits, plan waves and routine values not constrained above. Keep changes small and per-chip, with all mutable machine state in the instance.
- The MDEC status log has no cycle stamps, and the seek curve has low confidence; report those evidence limits accurately rather than treating deterministic regression hashes as hardware proof.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Scope, constraints and process
- `AGENTS.md` — project rules, deliverables, determinism, provenance, tests, plan shape and phase workflow.
- `.planning/AUTONOMOUS.md` — autonomous discussion, verification, efficiency and approval boundaries.
- `.planning/PROJECT.md` — core value, active milestone and out-of-scope features.
- `.planning/REQUIREMENTS.md` — DISC-01 through DISC-04 acceptance criteria and requirement mapping.
- `.planning/ROADMAP.md` §Phase 6 — phase goal, success criteria and canonical references.
- `.planning/LESSONS.md` SW-ORACLE-001 and SW-LEGAL-001 — run real oracles at their first caller and screen exact upstream files for licence.

### Hardware, architecture and host integration
- `.planning/preparation/README.md` — topic index and source conventions.
- `.planning/preparation/DECISIONS.md` DEC.42–48, DEC.75, DEC.91 and the Phase 6 open-question rows — settled disc, timing, fixture and public API policy.
- `.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md` §§2–7 — CD controller, image formats, XA/CD-DA, MDEC, tests, determinism and evidence confidence.
- `.planning/preparation/ARCHITECTURE.md` §§1–8 — scheduler, modules, API, I/O, state, DMA/device dependencies and hash definitions.
- `.planning/preparation/CONFORMANCE.md` §§2–5 — owned fixtures, mailbox records, oracle pins, nightly tiers and fuzz targets.
- `.planning/preparation/HOSTS-AND-INTEGRATION.md` §§1, 4, 7–8 — libretro disk control, runner behavior, API versioning and feature-to-host mapping.
- `.planning/preparation/ENGINEERING.md` — C rules, inventories, builds, CI, performance and code/documentation constraints.
- `docs/hardware/dma.md` §Costs — current CD-ROM delay-setting deferral and channel cost model.
- `docs/hardware/spu.md` §PCM output and evidence limits — current CD input boundary and native sample contract.

### Provenance and assets
- `PROVENANCE.md` §§1, 4, 6 and 8 — admissible code, exact excluded sources and vendored-file notices.
- `ASSET_POLICY.md` §§1–3 and 6 — permitted fixture sources, build-only disc images, size caps and enforcement.

### Existing integration seams
- `src/bus.c`, `src/dma.c`, `src/sched.c`, `src/internal.h`, `src/state.c` and `src/spu.c` — MMIO, DMA, events, instance state, serialization and audio integration.
- `host/disc_probe.c` and `host/disc_probe.h` — existing bounded host-only cue/ISO metadata parsing to inspect for reusable parsing patterns.
- `runner/main.c`, `libretro/libretro.c`, `libretro/schwaystation_libretro.info.in`, `tests/libretro/host.c`, `tests/fixtures/tests.cmake` and `tests/fixtures/manifest.txt` — host and fixture integration patterns.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `host/disc_probe.c` already has bounded host-only ISO and cue parsing for runner metadata; reuse its safe path and offset patterns where they fit the full sector provider.
- `src/spu.c` and `src/frame.c` provide the 768-cycle mixer and bounded PCM ring; `src/spu_reverb.c` and the existing tests provide established SPU structure.
- `src/gpu.c` already consumes DMA2 output and owns display scanout; the public video, audio and memory views are already available.
- Runner hashing, libretro audio delivery, test fixture assembly, reference fetching and CTest inventory registration already have per-subsystem patterns.

### Established Patterns
- One CMake OBJECT library and one exact inventory per chip/test area; tests link the object that ships.
- Device state is fixed-width and instance-owned; deadlines are derived and dispatched by `src/sched.c`; register handlers catch a device up to the current CPU tick.
- `src/dma.c` dispatches channel device endpoints through one switch and models bounded work as scheduled units. `src/state.c` serializes machine state through the field visitor.
- Host parsers stay outside the library core, use strict size/path bounds, and have both parser fuzz targets and committed regression seeds.

### Integration Points
- `src/bus.c` currently routes CD-ROM and MDEC MMIO to the unbuilt-device path; `src/sched.c` has no CD deadline.
- DMA channels 0, 1 and 3 currently use stub endpoints; channel 3's 24/40-cycle choice is deferred to the CD-ROM endpoint.
- `include/schwaystation.h` is API v4 and has no disc callbacks; add only the Phase 6 public surface and a new API consumer test while retaining prior API tests.
- The runner and libretro currently lack the shared disc provider; libretro metadata has disk control disabled. The existing libretro host test and hosted RetroArch job are the integration points for m3u swapping and boot evidence.
- The project has no committed MDEC or CD module yet; use the established source, header, test, documentation and inventory structure.

</code_context>

<specifics>
## Specific Ideas

Use the pinned `ps1-tests` CD timing/getloc, MDEC block/frame/movie and status-log cases at the earliest runnable caller. Compare direct output and full CD→DMA→MDEC→GPU behavior separately so a self-authored synthetic test cannot substitute for the external reference. `disc_boot` uses the own `SYSTEM.CNF`/PS-EXE, generated XA and CD-DA, short STR, cue and SBI callback data.

</specifics>

<deferred>
## Deferred Ideas

- CHD, PBP, ECM, CCD and full SBI/LibCrypt media support remain in v2.0 as settled by DEC.43.
- Refit CD seek timing, MDEC block cost, XA resampler rounding or other hardware-only unknowns in the measured-accuracy horizon only when new hardware evidence is available.
- Browser disc playback, save states, rewind, run-ahead, movies and unrelated peripherals stay in their later milestones.

</deferred>

---

*Phase: 6-discs-and-fmv*
*Context gathered: 2026-11-15*
