# Phase 15: GunCon and Justifier - Context

**Gathered:** 2026-11-24
**Status:** Ready for planning

<domain>
## Phase Boundary

Deliver GUN-01, GUN-02 and SIO-05 for direct physical controller ports: deterministic GunCon and Justifier
serial behavior, beam-timed light detection, documented libretro controls, and frame-latched C and libretro
input. Preserve the released API v9 prefix, integer-only deterministic core, bounded scheduler, host-only
device access, three deliverables, and all existing controller classes. Tests use synthetic displays and
host input. They establish the selected model and mappings, not physical-gun calibration or retail-game
compatibility. Lightguns behind multitaps and multi-Justifier wiring are not compatibility claims in this
phase.

</domain>

<decisions>
## Implementation Decisions

### Public input and compatibility
- Append gun input after the complete 228-byte API v9 `schway_input` prefix and advance the API version to
  v10. Keep `schway_controller_input` at 24 bytes so the eight existing endpoint strides do not move.
  Normalize from the caller's size tag and validate before applying input. The published struct layout and
  caller compatibility make this one-way; it follows the Phase 14 append-only input contract and DEC.75.
- Represent input availability separately from coordinates. Missing, unsupported or out-of-bounds host input
  cannot become a centered aim point or a beam hit. Keep aim, trigger, reload/offscreen and selected device
  state in the same frame-latched snapshot.

### Gun protocols and beam sensing
- Preserve the existing cycle-scheduled SIO byte/ACK path. GunCon (5A63h) reports its documented X/Y units in
  the 42h reply; Justifier (5A31h) reports its buttons and raises IRQ10 when its enabled sensor sees the beam.
  Synchronize Timer 0/1 to the beam before IRQ10 so the interrupt handler observes the shot position.
- Reuse the GPU's integer beam clock and scheduler. Schedule a bounded event for the aimed display pixel and
  sample its current VRAM value at that beam time; do not store a timestamp for every output pixel. Map host
  screen coordinates through the active display geometry and region timing before any VRAM lookup.
- Use one documented integer brightness rule over the pixel's 5-bit RGB channels. Choose and record its
  threshold from the implemented pixel representation; label it as a deterministic emulator model, not a
  calibrated physical-gun measurement. Test dark, bright, edge, and below/at/above-threshold values. Keep
  offscreen reload distinct from clamped edge coordinates. The prepared GunCon dark/offscreen reply codes are
  ambiguous, so document the chosen deterministic response without claiming a physical measurement.
- Keep pending host input outside serialized state. If in-flight protocol or scheduled sensor state needs
  serialization, use the existing fixed-width visitor and update format/behavior revisions only for fields
  that are actually serialized.

### Libretro mapping and scope
- Map GunCon to `RETRO_DEVICE_LIGHTGUN`; select Justifier with the official
  `RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_LIGHTGUN, 2)` identifier and poll through the base device as required
  by the vendored header. Keep controller descriptions and public mapping docs aligned with every delivered
  class.
- Use LIGHTGUN controls as the complete path for screen X/Y, trigger, offscreen and forced reload. Accept
  POINTER as a fallback for coordinates, pressed and offscreen status, and document that it has no separate
  forced-reload control. Latch the resulting snapshot once before each frame.
- Query device capabilities from `retro_run`. If the input callback or required device capability is absent,
  expose an unsupported result, submit no valid gun input and produce no sensor hit. Zero returned for an
  unsupported individual control must not silently become a valid shot.
- Deliver direct-port support. The existing eight-slot multitap route remains available for other controller
  classes, but tap gun and shared IRQ10 behavior are deferred until their routing is independently tested.
  Do not claim two-player Justifier wiring.

### Verification and CI iteration
- Reuse the existing API, SIO, GPU beam, and in-process libretro host suites. Add vectors for both IDs,
  replies and button words; frame latch and input-size compatibility; coordinate bounds and display geometry;
  bright/dark/edge/offscreen outcomes; Justifier enable, timer reads and IRQ10 timing; and absent host
  capability/callback. Record only exercised gun predicates in the compatibility ledger.
- Before a build, check the frozen v9 offsets and controller stride, sorted changed inventories, exact selector
  registrations, fixture sizes and relevant budget totals. Then build and run only the API, gun SIO, beam, and
  libretro selectors before the affected local CI workflow. Reuse existing preflight gates; add a permanent
  checker only if this phase reproduces a recurring omission.
- If gun state changes a `quality.json` metric, run that metric's producer locally and compare it with the
  bound before broad hosted CI. No workflow change means `act` adds no coverage; if routing changes, run
  `act --dryrun` and the reproducible jobs while keeping hosted-only checks required. Inspect every hosted
  run during and after execution for critical-path time, duplicate work, iteration delay and runner use; retain
  every required job and assertion. Reconsider todo 052 only after the controller plan is stable and without
  displacing this phase. Any Docker resources must use project/task names and be removed individually.

### Decided autonomously
- [Public input] Q: "Where should gun aim and availability fields live?" → Selected: "Append a dedicated gun extension after the complete v9 prefix; do not enlarge the nested endpoint struct." — it preserves the frozen v9 bytes and endpoint stride, and the extension shape can be revised before release.
- [Host input] Q: "How should absent input be represented?" → Selected: "Use explicit validity and distinct offscreen/reload state; invalid input cannot create a hit." — this is local validation logic and cheap to change before release.
- [Beam sensing] Q: "How much pixel history should the GPU retain?" → Selected: "Schedule the aimed-pixel beam event and sample at that time." — it reuses the integer scheduler and can be replaced if a measured aperture requirement appears.
- [Brightness] Q: "What evidence supports a sensor threshold?" → Selected: "Use a documented integer model and test its boundary; make no hardware-calibration claim." — the available reference supplies no numeric threshold, so the model can be tuned later against new evidence.
- [Topology] Q: "What gun wiring is supported by this phase?" → Selected: "Support direct physical ports and defer tap guns and shared multi-Justifier behavior." — it matches the tested contract and avoids claiming unverified tap semantics.
- [Host mapping] Q: "Which available libretro input path is primary?" → Selected: "Use LIGHTGUN for the complete control set and POINTER only for the subset its contract provides." — it uses the existing official IDs and adds no dependency.
- [Verification] Q: "Should unchanged workflow wiring run under act?" → Selected: "Use targeted local tests; use act only when a workflow change adds reproducible coverage." — an equivalent route check adds no new evidence.

### the agent's Discretion
- Select the concrete integer threshold only after inspecting current pixel encoding and scheduling order; pin
  that value in hardware documentation and below/at/above tests without presenting it as measured hardware.
- Choose exact public names and test split consistent with the additive API and existing module boundaries.
- Keep one-shot board/region mapping deterministic and ensure missing controls, coordinate overflow and
  reload/offscreen inputs are validated before indexing or scheduling.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/pad.c` owns the bounded device IDs, reply lengths and SIO command FSM; `src/sio.c` owns cycle-derived
  byte and ACK scheduling.
- `src/gpu.c` and `src/gpu.h` track integer beam timing, timer catch-up and display geometry. `src/irq.c`
  already handles IRQ10, while GPU timer paths can synchronize Timer 0/1.
- `src/frame.c` normalizes size-tagged input and queues one snapshot; `src/pad.h` keeps pending host input
  outside save states.
- `libretro/libretro.c` polls once before running a frame and already publishes controller descriptions and
  accepted device IDs. The vendored `libretro/libretro.h` defines LIGHTGUN, POINTER and Justifier IDs.
- Existing test seams are `tests/api/`, `tests/sio/`, `tests/gpu/beam.c`, and `tests/libretro/host.c`.

### Established Patterns
- Public structs are fixed-width, size-tagged and append-only; absent trailing fields are zero-filled and
  validated before state changes. The v9 `schway_input` size is 228 bytes and each nested endpoint is 24.
- Core behavior uses integer clocks and bounded instance-owned state. Host callbacks stay in the libretro
  adapter. Exact sorted inventories register tests at configure time.
- Synthetic tests prove deterministic behavior and dataflow, not physical hardware accuracy or game play.
  Ledger rows describe only predicates actually exercised.

### Integration Points
- `include/schwaystation.h` and `src/frame.c` define the input boundary; `src/pad.c` dispatches serial device
  IDs; `src/gpu.c` schedules scanout and timer progress; `src/irq.c` raises source 10.
- `libretro/libretro.c` must update device selection, poll IDs and capability handling together, then queue one
  snapshot. `docs/embedding.md` and `docs/hardware/gpu.md` carry the public mapping and chosen model.
- Compatibility evidence flows through the existing feature ledger and its count/key preflight.

</code_context>

<specifics>
## Specific Ideas

- Read `.planning/preparation/README.md` first, then `.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md` §2.7,
  `PSX-GPU-AND-GTE.md` §4, `ARCHITECTURE.md` §9 and `HOSTS-AND-INTEGRATION.md` §1 before changing code.
- PER.01 is a T2 community source: it gives GunCon coordinates and Justifier IRQ10/timer behavior but no
  calibrated brightness threshold. Keep this confidence limit visible in code comments and user-facing
  hardware docs. The local vendored libretro header is the mapping authority.
- Use a self-authored synthetic fixture or existing host harness; do not require an owner hardware capture.
  A protocol vector is not a retail-game playability result.

</specifics>

<deferred>
## Deferred Ideas

- GunCon and Justifier support behind multitaps, shared IRQ10 interactions, and two-player Justifier wiring.
- Measured optical sensitivity/aperture and physical controller capture.
- A dedicated emulator-specific lightgun option or additional host dependency.
- Todo 052's BE32 movie shard redesign until the gun plan is stable and the required isolation/coverage proof
  can be completed without delaying Phase 15.

</deferred>
