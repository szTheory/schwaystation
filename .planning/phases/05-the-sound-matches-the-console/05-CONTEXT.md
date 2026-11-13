# Phase 5: The sound matches the console - Context

**Gathered:** 2026-11-13
**Status:** Ready for planning

<domain>
## Phase Boundary

Implement the PS1 SPU in the existing integer-only core, emit its native 44.1 kHz stereo PCM through the existing audio API, and prove hardware behavior, cross-platform determinism, and runner/libretro stream equality. The phase includes SPU registers, voices, BRR playback, envelopes, noise, reverb, capture, IRQ and DMA transfer behavior, audio hashing, and the `spu_tone` fixture. CD-ROM, XA, CD-DA decoding, disc support, and MDEC remain Phase 6. No window or audio device belongs in the deliverables.

</domain>

<decisions>
## Implementation Decisions

### SPU model and accuracy
- **D-01:** Carry forward DEC.36: one integer SPU tick per 768 CPU cycles; process voices 1–24 and reverb in the documented order, checking IRQA at each modeled SPU RAM access. Keep the sample-granular model. Do not introduce an eight-cycle RAM-slot scheduler unless a pinned test or hardware trace shows sample-granular timing is insufficient.
- **D-02:** Reconcile the frozen noise description with the newer primary psx-spx update before implementation. Use its clock-slot schedule and noise capture behavior, including that clearing SPU enable does not reset the LFSR. The project research/plan must cite the exact public commit and update any stale local summary; do not preserve two conflicting algorithms.
- **D-03:** Keep the documented integer BRR, Gaussian, ADSR, mixing, and reverb design as the baseline. High ADSR-rate and phase-negative sweep behavior remains uncertain without a direct hardware oracle. Do not add an unverified model/profile switch. Reconcile reverb against the new SCPH-1001 measurements: 16-bit intermediate saturation affects output, left/right processing alternates, and the combined 39-tap input/output delay is 38 samples. The exact per-stage rounding positions remain unknown, so tests must not claim unsupported bit-exactness.
- **D-04:** Keep all SPU state instance-owned, fixed-width, bounded, and serializable. Use explicit little-endian guest-memory helpers and integer fixed-point arithmetic so `be32` and `wasm32` produce the same PCM. Continue processing silent voices and timing-sensitive capture/IRQ work when output is muted; `core.no_busy_loop` must still bound idle work.

### PCM delivery and host timing
- **D-05:** Preserve DEC.37/38: emit signed 16-bit stereo at 44,100 Hz, one sample pair for every 768 CPU cycles, with the fractional frame remainder carried forward. Never pad frames or resample inside the core. Hash the final stream after main volume and saturation and before host resampling.
- **D-06:** Use the existing public `schway_get_audio` contract and bounded audio ring. Keep libretro batch delivery for frame-sized output and retain the single-sample callback fallback. `libretro.host` must compare the actual PCM from both paths against the runner's PCM, including samples produced by the SPU rather than only the current silent stream.
- **D-07:** Keep the current emulated AV rates in Phase 5 so the reported frame cadence and the carried PCM count remain coupled. Do not change only libretro's rate metadata to the interlaced field rate. If later primary evidence warrants a different interlaced report, change and validate the field cadence, audio accumulator, AV info, and host tests together.

### Conformance and evidence
- **D-08:** Keep SOUND-01/02's required gates and fixture behavior. Use the pinned `spu/memory-transfer` hardware log for its seven transfer pass lines; use the `spu_tone` fixture, cross-lane hashes, CPU-free block-table test, host equality test, and RetroArch smoke for their stated purposes.
- **D-09:** Treat the evidence types distinctly: cross-lane PCM hashes prove determinism, runner/libretro equality proves delivery, and the transfer log proves only its named hardware cases. No pinned hardware PCM waveform was found in the current ps1-tests SPU corpus. Self-authored tone and reverb digests are regression checks, not hardware captures. Add tests for the new primary noise and measured reverb constraints without inventing a complete silicon waveform oracle or asserting unknown rounding details.
- **D-10:** Add no new dependency for SPU math, sampling, or testing. Prefer small local C helpers and pinned, noticed reference data. Keep every behavior check in the CI inventory; avoid repository gates that prevent no named fault.

### The agent's discretion
- Choose private struct fields, helper boundaries, test partitioning, and plan waves while preserving the public API, source provenance, fixed-width state, and exact requirements above.
- Keep comments and hardware docs tied to sources. Record measured, documented, derived, and still-unknown behavior separately.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Scope, workflow, and constraints
- `AGENTS.md` — project rules for determinism, dependencies, tests, evidence, provenance, and plan shape.
- `.planning/AUTONOMOUS.md` — fan-out, source quality, adversarial review, and automation-first workflow.
- `.planning/PROJECT.md` — core guarantees, integer-only rule, three deliverables, and explicit exclusions.
- `.planning/REQUIREMENTS.md` — SOUND-01 and SOUND-02 acceptance details.
- `.planning/ROADMAP.md` — Phase 5 goal, success criteria, and Phase 6 boundary.
- `.planning/LESSONS.md` — especially SW-ORACLE-001 and the rule to run a real pinned oracle at its first runnable caller.

### Hardware, architecture, and host contract
- `.planning/preparation/README.md` — preparation index and which reference sections govern this phase.
- `.planning/preparation/DECISIONS.md` — DEC.09–11 and DEC.36–41; also the open interlaced-rate question.
- `.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md` §§1.1–1.9, 5–7 — SPU behavior, current oracles, determinism, and audio synchronization.
- `.planning/preparation/ARCHITECTURE.md` §§1–5, 7, 10–12 — scheduler, bus/module ownership, API, state, hashes, audio pipeline, build order, and performance.
- `.planning/preparation/CONFORMANCE.md` — `spu_tone`, `audio@600`, mailbox records, hardware logs, and the phase test matrix.
- `.planning/preparation/HOSTS-AND-INTEGRATION.md` §1 — libretro timing and batch/single audio delivery contract.
- `.planning/preparation/ENGINEERING.md` — C, boundedness, performance, and test rules.

### Source and asset policy
- `PROVENANCE.md` — exact source and notice rules.
- `ASSET_POLICY.md` — permitted fixture and firmware handling.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable assets
- `include/schwaystation.h` and `schway_get_audio` already define the public stereo PCM interface and sample-rate metadata.
- `src/frame.c` already owns the bounded audio ring and drain API; `src/internal.h` sizes it for four PAL frames. It currently queues silence once per 768-cycle boundary.
- `libretro/libretro.c` already prefers the batch callback and falls back to the single-sample callback. Its host-facing rate comes from `schway_get_info`.
- `runner/sha256.c` and `runner/main.c` provide the current hashing and frame-output paths for `--hash-audio`.
- Existing core and libretro host tests exercise audio ring bounds, reset/load clearing, sample counts, and adapter output; extend those instead of creating a second host path.

### Established patterns
- `src/sched.c` derives the next event from device deadlines and runs due events in a fixed switch. It currently schedules DMA, timers, GPU, and SIO.
- `src/bus.c` already routes the SPU register range and has a narrow ATTR read/write and DMA-control shim; other SPU accesses are recorded as unbuilt. `src/dma.c` has the existing channel-4 integration seams.
- `src/state.c` uses the field visitor and clears the host audio ring after a state load. Serialize SPU machine state and histories; keep the host drain queue outside the save state.
- CMake test inventories are exact and registered centrally in `tests/inventory/ci.txt`; add audio tests as gating inventory entries.

### Integration points
- Replace the silent sample producer in `src/frame.c` with samples from the scheduled SPU tick, preserving exact cycle and frame remainders.
- Add SPU state and timing to `src/internal.h`, `src/state.h`, `src/state.c`, `src/sched.c`, `src/bus.c`, and the DMA channel-4 path.
- Extend `runner/main.c` and runner tests for the audio hash, and extend `tests/libretro/host.c` so real SPU output is checked through both libretro callbacks against the runner stream.
- Add the SPU source and per-directory test inventory using the existing CMake structure. Do not add a host audio device or new dependency.

</code_context>

<specifics>
## Specific Ideas

Three focused reviews covered SPU timing/portability, PCM and libretro integration, and validation/oracle quality. They found no reason to reopen the settled sample-tick, PCM format, hash boundary, or required gate set. An adversarial pass found two newer primary-source updates than the preparation snapshot:

- [psx-spx noise-clock update](https://github.com/psx-spx/psx-spx.github.io/commit/3717718f642ce2ca9ff590b65918fb3a46279cc4) — more precise noise clock slots and the signal written to voice capture buffers. The commit resolves through the public GitHub API; use it as facts only and verify the exact source at planning time.
- [SCPH-1001 reverb measurements](https://github.com/psx-spx/psx-spx.github.io/commit/a37f2fec163e149f4e3e39599eb43c65c21181a8) — measured delay and saturation constraints from a digital audio tap; the exact arithmetic rounding remains open. Use the measurements to constrain tests, not to claim a full reference waveform.
- [libretro audio guidance](https://docs.libretro.com/development/cores/developing-cores/) recommends the batch callback for frame-sized output, consistent with the existing adapter.
- The current [pinned ps1-tests SPU tree](https://github.com/JaCzekanski/ps1-tests/tree/f727802fead1/spu) supplies the hardware transfer log but no hardware PCM capture. Do not treat emulator-recorded playback data as a silicon oracle.

These sources were read for facts only; no emulator implementation was copied and no dependency was added.

</specifics>

<deferred>
## Deferred Ideas

- CD-ROM input, XA decoding, CD-DA decoding, and disc-driven audio remain Phase 6. Keep the Phase 5 hash/output boundary compatible with those later mixer inputs.
- Exact high-rate ADSR quirks and the remaining reverb rounding gap stay evidence questions. Do not invent a profile toggle or a hardware capture requirement for the owner.
- The audio-rate question for interlaced fields remains open at the project level; Phase 5 keeps the existing emulated rate unless a complete cadence-and-audio change is supported by new evidence.

</deferred>

---

*Phase: 5-the-sound-matches-the-console*
*Context gathered: 2026-11-13*
