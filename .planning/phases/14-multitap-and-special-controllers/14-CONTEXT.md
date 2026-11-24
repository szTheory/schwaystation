# Phase 14: Multitap and special controllers - Context

**Gathered:** 2026-11-23
**Status:** Ready for planning

<domain>
## Phase Boundary

Deliver SIO-01 through SIO-04: independently selectable multitap controllers and cards on either physical
SIO0 port; deterministic frame-latched mouse, neGcon, Jogcon and Dual Analog flight input; and matching
size-tagged library and libretro host mappings. Each tap exposes four controller sockets and four card
sockets, so two taps expose up to eight of each. Keep the existing digital-pad and DualShock callers
compatible. Preserve the integer-only, deterministic core, host-only file access and the three deliverables.
Light guns belong to Phase 15; do not add SIO1 link devices, PS2 peripherals or PocketStation execution.

</domain>

<decisions>
## Implementation Decisions

### Public input compatibility
- Append the new controller and peripheral fields in API v9. Keep the released 28-byte v8 `schway_input`
  prefix and its meaning unchanged; accept older sizes and default absent appended fields to zero. Update the
  setter to copy only the bytes covered by the size tag before validating the resulting full-size value.
  Add frozen API-v9 coverage for a v8-sized caller and a partially populated extension. — **Reversibility:**
  one-way — the published C struct, API version and downstream caller compatibility become public contracts.
- Carry all eight tap endpoints, mouse relative movement/buttons, neGcon controls, Jogcon integer controls and
  Dual Analog flight inputs through the size-tagged boundary. Keep values as integers and latch them only at
  frame start.

### Multitap cards and saves
- Model four controller and four card endpoints on each physical port, with the tap disabled by default and
  independently configurable per port. Preserve both documented controller polling methods and card address
  selection. Preserve the empty-tap/empty-first-slot behavior, FFFFh padding for later missing controllers,
  ACK/IRQ timing and final-byte no-ACK behavior.
- Expose every card behind either tap through additive, stable per-port/per-socket API identifiers and calls.
  Keep the existing card 1 and card 2 calls mapped to physical port 1 and port 2 respectively; do not
  renumber them when a tap is enabled. Persist all eight cards independently through runner and libretro host
  files, while keeping card bytes and generations outside save states as DEC.67 requires. — **Reversibility:**
  costly — changing the new public socket mapping or host filenames later would invalidate integrations and
  existing saves.
- Save and restore each endpoint's in-flight protocol state, but never roll persistent card contents back
  when loading a state. Bump state and behavior versions when the new serialized device state requires it.

### Controller protocols and host mapping
- Implement the documented device IDs and bounded serial modes. Pass raw integer controls through without
  adding host-side response curves or deadzones to the core. Implement only supported, documented Jogcon
  behavior; reject unsupported modes explicitly and do not claim force feedback without a cited protocol.
- Use the pinned libretro device IDs and documented public mappings. Keep libretro's one-poll-per-frame input
  latch, expose the eight player descriptors when the corresponding tap option is enabled, and preserve the
  current default mappings and tap-disabled behavior.
- Record only controller modes actually exercised by tests in the existing compatibility ledger; a protocol
  vector alone must not imply that a full game boots or plays.

### Verification and iteration
- Add short serial and libretro mapping vectors in the existing SIO/card/libretro suites. Keep these checks
  separate from long movie and guest-boot scenarios. Before building, sort changed inventories and configure
  so missing registrations or inventory drift fail cheaply; run focused selectors before the required full
  local and hosted gates.
- Do not alter workflow routing, matrix coverage, test cases, assertions or bounds without evidence. `act`
  adds no coverage for unchanged workflow wiring; use it only if a relevant workflow change makes a local job
  reproducible. If Docker becomes useful, name every created resource for this project and task, then remove
  only those resources.

### Decided autonomously
- [Protocol coverage] Q: "Which tap access paths and empty-slot cases are in scope?" → Selected: "Both controller poll methods, card addressing, both physical ports, empty tap/slot behavior, later-slot padding, ACK timing and no-ACK final bytes." — these are already specified by the pinned hardware reference and are cheap to verify with short vectors.
- [Jogcon boundary] Q: "How much undocumented Jogcon behavior should be modeled?" → Selected: "Limit behavior to documented integer controls and explicitly refuse unsupported modes; do not infer force feedback." — the source gap makes a narrower implementation cheaper to extend than to unwind an unsupported compatibility claim.
- [Test placement] Q: "Where should controller checks run?" → Selected: "Extend the existing SIO/card and libretro inventories with direct protocol/mapping tests." — this reuses the established seams and is local to the affected modules.

### the agent's Discretion
- Choose the smallest internal structs and switch-based routing that preserve the current module direction and bounded instance storage.
- Choose exact public enum names, card-sidecar suffixes, host mappings for special analog controls, and task boundaries consistent with the decisions above and the pinned sources.
- Keep new plans within the 30-minute planning rule and sequence work that shares SIO, card, state or test-inventory ownership.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/sio.c` already schedules each serial byte and ACK; retain its baud and device-specific timing model.
- `src/pad.c` already implements DualShock configuration, rumble, frame latching and a bounded command FSM.
- `src/card.c` already owns card command phases, generation counters and state visitation; card bytes remain outside state files.
- `tests/sio/`, `tests/card/` and `tests/libretro/` already provide focused vectors and exact sorted inventories.

### Established Patterns
- Public structs are append-only and size-tagged; missing fields default to zero and every header addition increments `SCHWAY_API_VERSION`.
- Tests are registered and inventory-checked at configure time. Keep exact inventory rows bytewise sorted.
- State is a fixed-width integer visitor, validated before apply. Routing uses plain switch statements and does not add dependencies.

### Integration Points
- SIO dispatch is currently one card then one pad per selected physical port (`src/sio.c`); pad and card state arrays currently have two entries (`src/pad.h`, `src/card.h`).
- `schway_set_input` in `src/frame.c` validates and copies the entire current struct, so it must become size-bounded when the struct grows.
- Card memory IDs, card load/generation APIs and current runner/libretro persistence expose two cards (`include/schwaystation.h`, `src/instance.c`, `runner/`, `libretro/libretro.c`).
- Libretro accepts NONE, JOYPAD and ANALOG on two physical ports and polls before each frame (`libretro/libretro.c`); controller descriptors and host assertions live in the existing libretro test suite.
</code_context>

<specifics>
## Specific Ideas

- Read `.planning/preparation/README.md` and the linked `PSX-PERIPHERALS-AND-SAVES.md` §§1–3, `HOSTS-AND-INTEGRATION.md` §1 and `ARCHITECTURE.md` §§3–5 before design. Keep `DEC.60`, `DEC.61`, `DEC.67` and the API compatibility rules in force.
- The core hardware reference is pinned psx-spx PER.01 revision `6d7d1bc106a7`; libretro identifiers come from the pinned header and host documentation. The live hardware page confirms four controllers and four cards per tap and the port-2 title cases: https://psx-spx.consoledev.net/controllersandmemorycards/.
- No owner hardware capture is available or needed for this phase. Use documented serial vectors and deterministic host mapping checks, and state any evidence limits plainly.
</specifics>

<deferred>
## Deferred Ideas

- GunCon and Justifier beam/timing behavior remains in Phase 15.
- SIO1 link cable, PS2 devices, PocketStation program execution and undocumented Jogcon force feedback remain out of scope.
- Workflow routing or CI matrix redesign is not supported by this phase's evidence; keep any larger efficiency work as a todo bound to the phase that first needs it.
</deferred>
