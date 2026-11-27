# Phase 15: GunCon and Justifier - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md; this log preserves the alternatives considered.

**Date:** 2026-11-24
**Phase:** 15-GunCon and Justifier
**Areas discussed:** public input contract, beam sensor model, libretro mapping, topology, verification and CI

## Synthesis and adversarial review

Role fan-out covered domain behavior, C/GPU/SIO architecture, libretro integration, input safety, and CI cost.
The comparison below records the useful trade-offs and the adversarial pass that selected the recommendation.

| Lens | Evidence and benefit | Risk or counterargument | Resolution |
|---|---|---|---|
| Product and compatibility | GunCon uses coordinates; Justifier uses a beam interrupt. Testing both keeps delivered classes meaningful. | Synthetic tests can look like proof of retail playability. | Ledger only records exercised protocol/input predicates; no retail title is called playable from synthetic evidence. |
| Hardware and domain | PER.01 records GunCon X/Y units and Justifier IRQ10 plus Timer 0/1 reads. | The source is T2 and gives no calibrated brightness value; dark and offscreen GunCon replies are ambiguous. | Use a deterministic documented model and boundary tests; leave hardware accuracy unclaimed. |
| C/API and architecture | The size-tagged v9 prefix is 228 bytes; each nested endpoint is 24 bytes. GPU, scheduler, SIO and IRQ paths already exist. | Enlarging the nested endpoint would change every endpoint stride; storing every pixel timestamp adds memory and work. | Append after the full v9 prefix; schedule an aimed-pixel event and sample at beam time. |
| Robustness | Explicit validity, signed coordinate checks and offscreen state prevent missing input from selecting a default center point or indexing VRAM out of range. | Libretro's type capability does not prove individual queries are available. | Treat missing callback/type capability as unsupported, keep query-zero controls neutral, and test the no-hit path. |
| Frontend integration | The vendored header names LIGHTGUN, its controls, POINTER, and Justifier subclass 2; polling uses the base class. | POINTER has no separate forced reload; input drivers may not expose a gun class. | LIGHTGUN is complete path; POINTER is a documented subset; report unavailable capability and submit invalid input. |
| Topology | SIO can route eight controller endpoints and the reference says IRQ10 is wired through the tap. | The phase predecessor explicitly deferred guns-in-tap; simultaneous sensors and shared IRQ semantics lack a tested title. | Support direct physical ports and defer tap gun claims until a concrete protocol/test need exists. |
| Performance and CI | Targeted event avoids a per-pixel history; existing selectors and preflights cheaply establish wiring before longer runs. | A timer/event ordering defect may only show in the full integration suite. | Keep the full local and hosted quality gates; run focused cases first, then inspect exact-head CI time/cost. |

## Public input and compatibility

| Option | Description | Selected |
|---|---|---|
| Grow `schway_controller_input` | Put gun coordinates and controls in each nested endpoint; changes the existing endpoint stride. | |
| Append a gun extension | Preserve all 228 v9 bytes and append direct-port gun data, advancing the API version to 10. | ✓ |
| Separate setter | Keep `schway_input` fixed and add a second input call with ordering/latching rules. | |

**Selection:** Follow the additive v10 recommendation, consistent with Phase 14's accepted v9 contract and the owner's standing instruction to follow the synthesized recommendation. Existing callers retain the exact v9 prefix and old endpoint stride.

## Beam sensor model

| Option | Description | Selected |
|---|---|---|
| Geometric aim only | Convert cursor to GunCon coordinates or timer values without checking the emitted pixel. | |
| Beam-time pixel sample | Schedule the aimed point on the integer GPU beam, sample its displayed VRAM value at that time, and apply a documented integer threshold. | ✓ |
| Full pixel timestamp buffer | Retain line/dot and brightness for every output pixel for later sensor queries. | |

**Selection:** The beam-time sample best matches the documented sensor behavior while keeping state and per-frame work bounded. The threshold will be an emulator model and tested at its boundary because the cited reference supplies no measurement.

## Host and controller mapping

| Option | Description | Selected |
|---|---|---|
| LIGHTGUN only | Full screen, trigger, offscreen and reload mapping using the base class and Justifier subclass. | |
| POINTER only | Screen position, pressed and offscreen mapping; no distinct forced reload. | |
| LIGHTGUN plus POINTER fallback | Use LIGHTGUN as the complete path and POINTER for the controls its contract exposes. | ✓ |

**Selection:** Prefer the official lightgun path and retain a bounded pointer fallback. Poll the base ID, choose Justifier with subclass 2, and latch before the frame.

## Input availability and topology

| Option | Description | Selected |
|---|---|---|
| Default coordinates | Treat absent control values as a centered or neutral aim point. | |
| Explicit validity | Track whether the selected gun input exists; invalid/unsupported input cannot produce a sensor hit. | ✓ |
| Direct ports only | Support the two physical ports and defer multitap gun interactions and multi-Justifier behavior. | ✓ |
| All tap sockets | Advertise gun devices at every existing endpoint and model shared IRQ10 behavior. | |

**Selection:** Use explicit validity and direct ports. Architecture's tap wiring observation is real, but the domain review found no established title behavior and the prior phase left this interaction out; adversarially, adding eight-player claims would widen the phase beyond demonstrated need.

## Verification and CI

| Option | Description | Selected |
|---|---|---|
| Add broad permanent preflights | Add new maintained gates for each anticipated omission. | |
| Reuse preflight, then focused tests | Check frozen ABI offsets, inventory order, selectors and budgets, then build/run API, SIO, beam and host selectors before full required gates. | ✓ |
| Run Act for unchanged workflows | Recreate existing job wiring even when local tests already cover the same steps. | |

**Selection:** Existing checks are sufficient for unchanged workflows. Use `act` only if a workflow edit adds reproducible coverage; preserve hosted-only gates and all assertions. Reconsider todo 052 only after Phase 15 planning stabilizes.

## the agent's Discretion

- Choose the exact integer threshold after checking current RGB5 representation and GPU scheduler order; document the number as a model, not hardware calibration.
- Choose exact names and bounded test slices consistent with the API and existing test seams.

## Deferred Ideas

- Tap gun support, shared IRQ10 behavior with multiple lightguns, and two-player Justifier wiring.
- Physical threshold calibration and owner-hardware captures.
- The separate todo 052 movie-shard workflow redesign until it can be isolated from Phase 15 work.

