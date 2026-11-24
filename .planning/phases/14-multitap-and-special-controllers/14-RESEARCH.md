## RESEARCH COMPLETE

# Phase 14: Multitap and special controllers - Research

## Planning recommendation

Implement this as a coordinated SIO, pad, card, public-input and host-mapping change. The tap is a router for
four controllers and four cards on each physical controller port; it is not merely a longer pad response.
Keep controller/card bytes and generation counters out of save states. Add protocol state for each endpoint,
and preserve the currently released caller and file contracts.

## Requirements and primary references

- **SIO-01:** Two SCPH-1070 taps can expose four controllers and four cards each. The pinned PER.01 source
  documents both controller poll methods (`01 42 01...` for all pads and `0n 42...` for one pad), card
  selection (`8n...`), the `5A80` tap ID, missing-slot `FFFFh` values, empty-first-slot abort, and the empty-tap
  short reply. The tap can be on physical port 1 or 2. Preserve the established SIO byte and card ACK timing,
  final-byte no-ACK and port-select/CS abort behavior.
- **SIO-02:** Mouse ID `5A12` reports active-low buttons and signed X/Y movement. Inputs are frame-latched;
  tests must pin saturation, reset and button behavior. The host reference uses `RETRO_DEVICE_MOUSE` relative
  axes/buttons.
- **SIO-03:** neGcon ID `5A23` carries buttons, twist and analog I/II/L; Jogcon ID `5AE3` adds dial data and
  direction flags. Keep integer behavior bounded to the modes established in the source index; reject
  unsupported modes explicitly. Do not infer undocumented Jogcon force feedback.
- **SIO-04:** Dual Analog flight mode uses ID `5A53` and the documented button/four-axis payload. Keep the
  existing `schway_input` 28-byte v8 prefix and old callers unchanged when adding the v9 input fields.

Required reads before implementation: `.planning/preparation/README.md`; `PSX-PERIPHERALS-AND-SAVES.md`
§§1–3; `HOSTS-AND-INTEGRATION.md` §1; `ARCHITECTURE.md` §§3–5; `DECISIONS.md` DEC.60, DEC.61 and DEC.67;
`.planning/LESSONS.md`; `AGENTS.md`; Phase 14 CONTEXT and project requirements. PER.01 is pinned psx-spx
revision `6d7d1bc106a7`; use its paraphrased protocol facts rather than copying source text. Libretro input
identifiers come from the pinned local `libretro/libretro.h` and the indexed official host reference.

## Existing implementation constraints

- `src/sio.c` dispatches one card and then one pad on the physical port selected by SIO CTRL. `src/pad.h` and
  `src/card.h` hold two entries; card dispatch in `src/card.c` indexes directly by physical port. Tap routing
  therefore has to track per-port tap state, virtual controller selection, card selection, byte ACK results,
  aborts and the scheduled reply without cross-slot leakage.
- `schway_set_input` currently checks against the current struct size and then copies the full struct. For an
  additive v9 `schway_input`, preserve the frozen 28-byte v8 minimum, zero a current-size value, copy only the
  caller-declared bytes, then validate. Add `tests/api/v9.c`; retain all existing frozen API tests.
- Public card APIs and memory IDs currently cover two cards. SIO-01 needs all eight sockets independently
  accessible and persistent. Add identifiers/accessors without changing legacy card 1/2 meaning. Existing
  policy maps `.srm` to card 1 and `.1.mcr` to card 2; document stable additional filenames for the other six.
- DEC.67 and the architecture require card bytes and generations to stay outside ordinary save states. Save
  state includes each endpoint's in-flight protocol state only. A new serialized tap/card FSM will require a
  state-format decision; update behavior revision when observable deterministic output changes.
- Libretro input is polled once before `schway_run_frame`. Its current setter accepts NONE, JOYPAD and ANALOG
  on two physical ports. Extend the existing controller descriptors/input snapshot path to special devices and
  player slots; leave existing digital/DualShock defaults intact and keep taps disabled by default.

## Test and CI guidance

Use short serial vectors and small in-process libretro assertions. Cover each physical port, both controller
poll paths, empty tap/slot behavior, four card identities, partial transfer/abort/state restore, all supported
special IDs and input saturation/latching. Do not extend long `libretro.input.movies` or guest-boot paths for
simple protocol checks. Record only modes the tests actually exercise in `tests/compat/features.csv`.

Before builds, sort changed inventories bytewise and run `cmake --preset ci`; this already catches sortedness,
missing registrations and exact inventory drift. Then build affected targets and select exact SIO, card, API-v9
and libretro tests. Run the broader local workflow once on the changed head, then the required hosted CI and
release gates. Phase 13 evidence places the BE32 CTest critical path near 4,711 seconds; avoid boot-heavy test
duplication and do not alter parallelism without a measurement. No workflow wiring changes are planned, so
`act` adds no coverage here; retain all hosted platform gates. No Docker resource is needed for the expected
local test path.

## Decisions still owned by planning

- Choose a compact v9 shape for the four virtual slots on both ports plus relative mouse and Jogcon controls;
  preserve the 28-byte prefix and verify every old caller size.
- Define stable per-port/per-socket card access and persistence names that coexist with existing card 1/2
  callers and save files.
- Set exact supported Jogcon serial/configuration modes only where the indexed primary reference provides
  enough detail for deterministic vectors. Treat missing evidence as an explicit unsupported mode.
- Partition work by shared-file ownership. Establish the CMake skeleton and test inventories first; sequence
  shared SIO/card/state changes before host mapping when they touch the same modules or inventories. Keep each
  plan under the 30-minute limit and review code before phase verification.

## Evidence limits

There is no dedicated owner hardware capture for these peripherals. The public primary source supports
deterministic protocol vectors, but a protocol pass alone does not prove a retail title boots or plays.
Jogcon's detailed feedback behavior is less complete in the indexed evidence than mouse, neGcon and tap
protocols; do not overstate it.
