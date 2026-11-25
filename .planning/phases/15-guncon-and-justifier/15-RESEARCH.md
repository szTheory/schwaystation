# Phase 15: GunCon and Justifier — Research

**Researched:** 2026-11-24  
**Domain:** C API compatibility, PS1 SIO peripherals, GPU beam/timer scheduling, libretro input  
**Confidence:** HIGH for repository seams and locked decisions; MEDIUM for T2 hardware protocol notes; LOW for optical behavior not measured by the project.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
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

### Deferred Ideas (OUT OF SCOPE)
- GunCon and Justifier support behind multitaps, shared IRQ10 interactions, and two-player Justifier wiring.
- Measured optical sensitivity/aperture and physical controller capture.
- A dedicated emulator-specific lightgun option or additional host dependency.
- Todo 052's BE32 movie shard redesign until the gun plan is stable and the required isolation/coverage proof
  can be completed without delaying Phase 15.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research support |
|---|---|---|
| SIO-05 | Official libretro identifiers, documented mapping and frame-boundary input latch | Vendored libretro definitions; existing adapter poll/queue path |
| GUN-01 | Deterministic GunCon/Justifier serial protocol, beam sample, documented threshold and IRQ timing | Existing SIO, GPU, timer and IRQ seams; PER.01 T2 protocol notes |
| GUN-02 | Lightgun/pointer controls and safe unsupported-input behavior | Vendored IDs and capability API; existing fake-host test seam |
</phase_requirements>

## Summary

**Primary recommendation:** Implement the locked append-only v10 gun extension and keep all gun behavior inside the existing instance-owned input, SIO, GPU, timer and IRQ paths. Add only direct-port support; do not claim optical calibration, multitap gun behavior or multi-Justifier wiring. [VERIFIED: `15-CONTEXT.md`, “Implementation Decisions”; `src/pad.c:466-498,714-810`; `src/gpu.c:190-205,253-348`]

The current API is size-tagged and copies normalized caller input into pending instance state. The existing timer/GPU scheduler already carries integer beam timing and processes timer synchronization around beam advancement. These are the implementation seams to extend, not replace. [VERIFIED: `src/frame.c:148-190`; `src/pad.h:45-51`; `src/gpu.c:190-205,290-348`]

The peripheral reference is explicitly tier T2. It documents GunCon X/Y units and Justifier IRQ10/timer use, but it does not provide an optical brightness threshold; the GunCon dark/offscreen reply interpretations overlap. Treat the threshold and ambiguous response selection as emulator-model decisions and label them accordingly. [CITED: `.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md` §2.7, PER.01 T2; `15-CONTEXT.md`, “Gun protocols and beam sensing”]

## Architectural Responsibility Map

| Capability | Primary tier | Secondary tier | Rationale |
|---|---|---|---|
| Public gun input and compatibility | C API / core boundary | Frame latch | Size-tagged normalization and pending input already live there. [`src/frame.c:148-190`; `src/pad.h:45-51`] |
| SIO gun ID and replies | Device protocol | SIO scheduler | Extend pad FSM and payload generation while retaining byte/ACK timing. [`src/pad.c:714-810`; `src/sio.c:125-142,363-376,659-675`] |
| Beam sensor and IRQ | GPU/timer/IRQ core | Scheduler | Beam timing is GPU-owned; Justifier sensor event must synchronize timers before raising IRQ10. [`src/gpu.c:190-205,208-250`; `src/timers.c:326-379,528-563`; `src/irq.c:25-32`] |
| Host controls and availability | Libretro adapter | C API snapshot | The adapter owns host callbacks, capability queries, controller descriptions and one pre-frame queue. [`libretro/libretro.c:618-627,664-699,1111-1283,1707-1726`] |
| Evidence and compatibility claims | Tests / docs | CI preflight | Reuse API, SIO, beam and host suites; ledger only predicates their vectors exercise. [`tests/abi/tests.cmake:150-157`; `tests/sio/tests.cmake:16-50`; `tests/gpu/tests.cmake:111-149`; `tests/libretro/tests.cmake:368-391`] |

## Standard Stack

| Component | Existing choice | Use in this phase |
|---|---|---|
| Language/build | C17, CMake; project declares `CMAKE_C_STANDARD 17`, required, extensions off. [VERIFIED: `CMakeLists.txt:16-18`] | Extend existing core and adapter; no new package or runtime dependency. |
| Test registration | Existing CMake/CTest inventories and selectors. [VERIFIED: `cmake/SchwaystationTests.cmake:1-10,38-70`; suites cited above] | Add vectors to existing API/SIO/GPU/libretro suites and keep sorted inventories. |
| Host mapping | Vendored `libretro/libretro.h` is the identifier source by locked decision. | Use the header’s LIGHTGUN / POINTER IDs and base-device polling rule, not private IDs. |

No dependency installation is required. This phase is an in-repository C/API/device implementation. [VERIFIED: `15-CONTEXT.md`, “Libretro mapping and scope”; `.planning/preparation/DECISIONS.md` DEC.75]

## Architecture Patterns

### Preserve the ABI prefix and latch once

The release header currently declares `#define SCHWAY_API_VERSION 9` (`include/schwaystation.h:14`). The ABI test pins these exact values: `"_Static_assert(sizeof(schway_controller_input) == 24u, \"API v9 endpoint input size\");"`, `"_Static_assert(offsetof(schway_current_input, multitap) == 28u, \"API v9 input.multitap\");"`, `"_Static_assert(offsetof(schway_current_input, controllers) == 36u, \"API v9 input.controllers\");"`, `"_Static_assert(sizeof(schway_current_input) == 228u, \"API v9 input size\");"`, `"_Static_assert(SCHWAY_API_VERSION == 9, \"API v9 version\");"` [VERIFIED: `tests/api/v9.c:36-41`]. The phase decision is to preserve that prefix and append the v10 extension; don’t put new fields in each nested endpoint.

`schway_set_input` reads the size tag, zero-initializes a current struct, copies the caller’s prefix, checks nonzero unknown trailing bytes and validates reserved/device values before assigning `pending_input`. [VERIFIED: `src/frame.c:148-190`] At the frame boundary, keep availability and coordinates in the same frame-latched snapshot and keep pending host state out of save states. [VERIFIED: `src/pad.h:45-51`; frame-latch implementation `src/pad.c:132-236`]

### Extend the current SIO FSM

The current `0x42` command returns the device ID and enters the ID-end phase; that phase returns `0x5A`; payload length is selected by `poll_size`, with per-byte reply/ACK behavior handled by the existing FSM. [VERIFIED: `src/pad.c:487-498,714-810`] SIO byte duration derives from baud and mode, and next-deadline/event functions already schedule transmission and ACK edges. [VERIFIED: `src/sio.c:125-142,363-376,659-675`] Add gun reports to this established path, with bounded fixed-width state and vectors for both report shapes.

The T2 peripheral note gives the protocol targets verbatim: `"GunCon (5A63h)"`, `"X in 8 MHz clocks since HSYNC, Y in scanlines since VSYNC"`, `"Konami Justifier / Hyperblaster (5A31h)"`, and `"The sensor pulls controller-port pin 8 (/IRQ10) when it sees the beam"`. [CITED: `.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md:96-100`, PER.01 T2] It also notes GunCon X=0001h with Y=0005h for light during VSYNC/offscreen-or-dark and Y=000Ah for no-light-or-busy; this ambiguity must remain visible in docs, not be presented as a uniquely established hardware response. [CITED: same §2.7 table]

### Schedule a bounded beam-time sample

GPU beam conversion carries an integer ratio remainder, and beam advancement calls `schway__timers_sync` before feeding dot progress. [VERIFIED: `src/gpu.c:112-133,190-205`] The GPU’s next deadline currently considers display/line edges and Timer 0 dot targets; lazy catch-up splits at those events. [VERIFIED: `src/gpu.c:253-348`] Add one bounded aimed-pixel deadline/event, convert host coordinates through active display geometry and region timing, then read the current RGB5 VRAM word at that point. Do not add per-output-pixel timestamp history. The current scanout path reads VRAM words in RGB15 mode and packs VRAM bytes in RGB24 mode, so define which displayed pixel representation feeds brightness before implementing the rule. [VERIFIED: `src/frame.c:301-337`; RGB555 and bit-15 semantics: `docs/hardware/gpu.md:51-75`]

The scheduler’s tie order is timers first and GPU second at equal `now` (`src/sched.c:56-79`). [VERIFIED] A Justifier sensor event must explicitly catch Timer 0/1 up to the shot beam point before raising IRQ10, as locked by CONTEXT; do not assume the ordinary top-level tie order alone provides this shot-position guarantee. The T2 reference says the IRQ handler reads Timer 0 dot and Timer 1 line. [CITED: PER.01 §2.7]

### Map libretro controls without inventing per-control capability

The vendored header states `"#define RETRO_DEVICE_LIGHTGUN     4"` and `"#define RETRO_DEVICE_POINTER      6"`; its Justifier example is `"#define RETRO_DEVICE_JUSTIFIER RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_LIGHTGUN, 2)"`. It also says `"Cores must use the base ID when polling for input"`. [VERIFIED: `libretro/libretro.h:120-129,238,305`] Relevant controls are `"#define RETRO_DEVICE_ID_LIGHTGUN_SCREEN_X        13 /*Absolute Position*/"`, `"#define RETRO_DEVICE_ID_LIGHTGUN_SCREEN_Y        14 /*Absolute Position*/"`, `"#define RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN    15 /*Status Check*/"`, `"#define RETRO_DEVICE_ID_LIGHTGUN_TRIGGER          2"`, `"#define RETRO_DEVICE_ID_LIGHTGUN_RELOAD          16 /*Forced off-screen shot*/"`, and pointer `"#define RETRO_DEVICE_ID_POINTER_X             0"`, `"#define RETRO_DEVICE_ID_POINTER_Y             1"`, `"#define RETRO_DEVICE_ID_POINTER_PRESSED       2"`, `"#define RETRO_DEVICE_ID_POINTER_IS_OFFSCREEN 15"`. [VERIFIED: `libretro/libretro.h:410-416,433-439`]

Capability query returns supported device types as a bitmask and is documented for `retro_run`; the header states `"Should only be called in \\c retro_run()."` [VERIFIED: `libretro/libretro.h:1150-1188`]. **Inference:** this type-level bitmask cannot establish whether each queried control is supported, and a control result of zero may be a neutral value. Therefore class/callback availability should gate a valid gun snapshot, while invalid/missing required aim must not schedule a hit. This is why type capability is not a substitute for explicit input validity. [INFERENCE from vendored type-mask contract plus locked CONTEXT]

The adapter already publishes controller descriptions and descriptors, queues a snapshot, and calls input polling before `schway_run_frame`. [VERIFIED: `libretro/libretro.c:618-699,1111-1283,1707-1726`] Extend description selection, device acceptance, control polling and snapshot queueing together; keep POINTER limited to its declared subset.

## Don't Hand-Roll

| Problem | Avoid | Reuse |
|---|---|---|
| Device byte/ACK timing | A second gun-specific timing loop | Existing cycle-scheduled SIO FSM and event deadlines. [`src/sio.c:125-142,363-376,659-675`] |
| GPU clock conversion and timer catch-up | Floating-point conversion or polling every dot/pixel | GPU integer beam ratio, existing scheduler and timer sync. [`src/gpu.c:112-133,190-205,253-348`] |
| Host input acquisition | Device or window code in the core | Libretro callback adapter; public C API only receives a latched snapshot. [`AGENTS.md:26-27,32-36`] |
| Capability semantics | Treating zero-valued controls as proof of unsupported or valid centered aim | Explicit validity plus device-class/callback checks; no-hit behavior on unsupported input. [`15-CONTEXT.md`, “Libretro mapping and scope”] |

## Common Pitfalls and Plan Risks

1. **Changing endpoint stride:** appending to the nested controller struct changes all eight endpoint strides. Preserve its 24-byte v9 size and append only beyond the full 228-byte prefix. [VERIFIED: `tests/api/v9.c:36-41`; decision: `15-CONTEXT.md`]
2. **Incorrect equal-tick ordering:** ordinary scheduler order runs timer event before GPU event; a beam shot needs Timer 0/1 synchronized to the exact sensor timestamp before IRQ10. Build a focused same-tick ordering vector. [VERIFIED: `src/sched.c:56-79`; requirement: `15-CONTEXT.md`]
3. **False hits from absent input or unsafe coordinates:** a missing callback, unavailable device class, signed-coordinate conversion, out-of-range aim or reload/offscreen state must not become a valid edge-clamped hit or VRAM index. Test no-hit and bounds before scheduling. [INFERENCE from current size-validation boundary and locked behavior: `src/frame.c:148-188`; `15-CONTEXT.md`]
4. **Calling the threshold “hardware accurate”:** no numeric sensitivity is established in PER.01. Pick a deterministic integer threshold after confirming the pixel encoding/event ordering, document it as a model, and test immediately below/at/above. [CITED: PER.01 §2.7; `docs/hardware/gpu.md:67-75`]
5. **Claiming physical or game compatibility from self-consistency:** synthetic vectors prove the implementation’s selected model and mapping only. Inbound lesson SW-ORACLE-001 says synthetic GPU assumptions were contradicted by pinned programs and recommends source-supported progress; CONTEXT explicitly disclaims physical calibration and retail-game compatibility. [VERIFIED: `.planning/LESSONS.md:200-207`; `15-CONTEXT.md`, Phase Boundary]
6. **Unproven topology expansion:** IRQ10’s tap wiring is mentioned in the T2 notes, but this phase explicitly defers tap guns and multi-Justifier interactions. Do not broaden the product or claims on that observation alone. [CITED: PER.01 §2.5; locked scope: `15-CONTEXT.md`]

## Verification Seams and Preflights

Use existing API v9 compatibility test and suites: `abi.api_v9` (`tests/abi/tests.cmake:150-157`), SIO selectors (`tests/sio/tests.cmake:16-50`), GPU beam selectors (`tests/gpu/tests.cmake:111-149`), and in-process libretro host selectors (`tests/libretro/tests.cmake:368-391`). Existing CMake registration compares expected and registered inventories; inventory checks enforce sorted/no-duplicate names. [VERIFIED: `cmake/SchwaystationTests.cmake:1-10,38-70`; `tests/cmake/inventory.cmake:1-33`]

Before building, follow the locked targeted preflight: frozen v9 offsets/controller stride, sorted changed inventories, selector registration, fixture sizes and budgets. Then run only API, gun SIO, GPU beam and libretro host selectors before affected local CI. Do not add another permanent check unless the implementation reproduces an omission. [VERIFIED: `15-CONTEXT.md`, “Verification and CI iteration”; existing compatibility ratchet: `tests/cmake/compat-ratchet.cmake:28-77`]

Minimum vector families: both serial IDs/replies/button words; API v9 prefix/frame latch; coordinate bounds and active display geometry; dark/bright/edge/threshold boundary and offscreen/reload distinction; Justifier enable and Timer 0/1/IRQ10 ordering; absent callback/capability no-hit path. Use the existing fake libretro environment callback in `tests/libretro/host.c`; it can validate callback and capability behavior without an emulator frontend. [VERIFIED: `15-CONTEXT.md`, “Verification and CI iteration”; `tests/libretro/host.c:523-559,711-782`]

**Evidence limit:** synthetic displays/host input establish only tested model predicates; they do not establish gun calibration, retail-game compatibility, per-control frontend capability, multitap wiring or two-player Justifier behavior. [VERIFIED: `15-CONTEXT.md`, Phase Boundary; capability limitation is an inference from the documented bitmask]

## Project Constraints (from AGENTS.md)

- Reuse permissive references only and never open the forbidden emulator sources; the shipped deliverables remain the library, runner and libretro core, with no window/input-device code. [`AGENTS.md:20-27`]
- Keep the core integer-only, deterministic, instance-owned and bounded; host callbacks stay in the adapter. [`AGENTS.md:32-36`]
- Land behavior changes with tests in the project workflow and document behavior outside the terse public header. Use deterministic automation and existing proportional gates; no new gate without a named fault. [`AGENTS.md:37-53`]
- Keep tests gating, avoid invented ceremony, follow phase plan shape (plan 1 CMake skeleton/inventories; parallelize independent later plans; split plans estimated above 30 minutes). [`AGENTS.md:47-59,80-82`]
- Run cheap consistency checks before expensive work; preserve all required CI cases and inspect exact-head run efficiency. [`AGENTS.md:98-123`]
- Use small C modules, fixed-width integers and hardware comments citing a source; public names start `schway_`, internal cross-file names `schway__<module>_`. [`AGENTS.md:133-142`]

## Assumptions Log and Open Questions

| # | Claim / unresolved decision | Risk if wrong |
|---|---|---|
| A1 | The exact RGB5 brightness threshold is not supplied by the T2 hardware source; choose the smallest deterministic rule supported by current pixel representation and document it as a model. [ASSUMED until implementation choice is made] | A different optical threshold may change which synthetic pixels trigger; do not claim hardware accuracy. |
| A2 | The implementation must choose one deterministic GunCon reply for ambiguous dark/offscreen cases. [ASSUMED model choice; T2 source leaves overlapping interpretations] | Software that distinguishes those conditions may expect another code. |
| A3 | Whether the beam-time sample should use the live RGB555 VRAM word or a separately reconstructed displayed RGB value needs to be resolved against scanout conversion, especially RGB24. [INFERENCE; inspect renderer mapping before code] | Host pixel and sensor sample may diverge at edges, display start offsets or RGB24 modes. |
| A4 | A frontend advertising a lightgun device type may still return neutral zero for an individual control; per-control availability cannot be inferred from this API’s type bitmask. [INFERENCE] | Incorrectly treating zero coordinates as valid can create false hits; retain explicit validity. |

No packages or external runtime services are introduced. Nyquist validation and security enforcement are explicitly disabled in `.planning/config.json:37-67`, so no generated Validation Architecture or Security Domain section applies. [VERIFIED: `.planning/config.json:37-67`]

## Sources and Confidence

- **Repository facts — HIGH:** `15-CONTEXT.md`; `15-DISCUSSION-LOG.md` synthesis; `include/schwaystation.h`; `tests/api/v9.c`; `src/frame.c`, `src/pad.c`, `src/pad.h`, `src/sio.c`, `src/gpu.c`, `src/gpu.h`, `src/timers.c`, `src/irq.c`, `src/sched.c`; `libretro/libretro.h`, `libretro/libretro.c`; suite inventories cited above.
- **Documented hardware — MEDIUM:** `.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md` §2.7 (PER.01, T2); `.planning/preparation/PSX-GPU-AND-GTE.md` §4; current GPU/timer documentation. In particular PER.01’s coordinate and IRQ behavior is not a first-party calibrated optical specification.
- **Project constraints and lessons — HIGH as repository instructions, not hardware evidence:** `AGENTS.md`; `.planning/LESSONS.md` SW-ORACLE-001.
- **Research boundary:** Broad external research was intentionally not repeated because `15-DISCUSSION-LOG.md` already preserves the domain/architecture/libretro/robustness/CI fan-out and adversarial synthesis. This note performs targeted repo checks for implementation planning only.

The discussion log already records the multi-lens and adversarial review: synthetic evidence limits, T2 threshold uncertainty, append-only ABI, bounded beam event versus pixel history, type-level capability limits, direct-port scope, and targeted CI. The recommendation here carries those choices forward without reopening alternatives. [VERIFIED: `.planning/phases/15-guncon-and-justifier/15-DISCUSSION-LOG.md:8-74`]

**Confidence breakdown:** API/SIO/GPU/libretro integration: HIGH; documented protocol facts: MEDIUM (T2); optical threshold/dark response and host-specific input exposure: LOW / unresolved.
