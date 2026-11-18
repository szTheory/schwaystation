# Phase 6: Discs and FMV - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in 06-CONTEXT.md; this log preserves the alternatives considered.

**Date:** 2026-11-15
**Phase:** 6-discs-and-fmv
**Areas discussed:** CD timing and command behavior, disc interface and supported images, XA/CD-DA audio, MDEC and conformance evidence

---

## CD timing and command behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Deterministic cycle model from DEC.44 | Fixed CPU-cycle response timings, exact 1x/2x sector periods and integer square-root seek ticks; stable across platforms. | ✓ |
| Randomized seek or speed-up model | Might mask a title-specific lockup but adds nondeterminism or documented compatibility failures. | |
| Wait for a new hardware seek curve before implementation | Could improve accuracy but blocks the named phase behavior on hardware unavailable by default. | |

**Autonomous resolution:** Follow the prepared deterministic model under the owner's standing authorization to accept the recommended choices. No new hardware measurement is required to implement the stated v1.0 baseline.
**Notes:** Keep DEC.45 region/licence behavior and DEC.47 delivery ordering. The `cdrom/timing` and `getloc` references constrain only their named timing/command behavior; deterministic hashes are not silicon evidence. The callback may stall host wall time, but adding asynchronous worker state would complicate ordering and is not required by the core contract.

## Disc interface and supported images

| Option | Description | Selected |
|--------|-------------|----------|
| Host callbacks plus one bounded shared parser | Core receives sectors/TOC/lid/swap/bad-Q; host helper handles cue/bin, ISO and m3u for runner and libretro. | ✓ |
| Parse files inside the core | Fewer host pieces, but brings file access and hostile text parsing into the deterministic library. | |
| Add CHD/PBP/ECM/CCD or general LibCrypt support now | Broader library compatibility, but conflicts with the settled v1.0 boundary and dependency/fixture policy. | |

**Autonomous resolution:** Follow DEC.42–43 and DISC-02; preserve the consumer-minimal API rule, use the libretro extended disk-control interface, and reject the deferred formats with the named unsupported-media result.
**Notes:** Cue `PREGAP` and `INDEX 00` are distinct; relative paths must remain contained. Exercise the bad-Q callback with owned fixture data without creating a general v1 SBI parser. Fold the channel-3 delay-cost todo into the plan that implements the CD DMA endpoint. Fold the vendored digest todo into the rcheevos disc-hash plan; keep the upstream notice and negative control.

## XA and CD-DA audio

| Option | Description | Selected |
|--------|-------------|----------|
| Seven-phase 29-tap integer XA filter, fixed initial phase | Uses DEC.39's documented deterministic baseline and the existing native PCM path. | ✓ |
| Two-stage interpolation filter | Could explain small documented rounding differences but lacks a confirming hardware capture and adds state/work. | |
| Linear 6:7 interpolation | Smaller implementation, but does not meet the selected filtered baseline. | |

**Autonomous resolution:** Use the seven-phase table, phase zero at power-on, bounded queue and fixed 44.1 kHz SPU consumption. Exercise double-speed CD-DA source advancement as a focused implementation check.
**Notes:** Reuse the existing mixer, capture, hash and libretro audio paths. Continue audio state progression while muted and preserve histories across sectors. Test XA and CD-DA independently plus the combined `disc_boot` output. A hash establishes repeatability, not a captured hardware waveform.

## MDEC and conformance evidence

| Option | Description | Selected |
|--------|-------------|----------|
| Real pinned guest references plus direct block tests | Checks block arithmetic, status behavior and full DMA/display integration at their first runnable callers. | ✓ |
| Synthetic tests and own hashes only | Fast and deterministic, but can preserve an incorrect arithmetic or ordering assumption. | |
| PCSX-Redux binary output as the hardware oracle | Useful as a divergence locator, but another emulator cannot establish silicon correctness. | |

**Autonomous resolution:** Follow DEC.33's integer model and the DISC-03 tolerances. Use direct tests for isolation, pinned ps1-tests data for reference checks, and a released emulator only for diagnostics.
**Notes:** The MDEC log lacks cycle stamps, so it cannot independently prove the 1,000-cycle cost. Keep that cost as the chosen baseline and describe the limit. Fetch MPL-bearing test executables only into build storage. Generated disc images are built from owned sources in CI, remain under 512 KiB and are never committed.

## Stakeholder and adversarial pass

- **Security and provenance:** Strict parser bounds, path containment, manifest-backed owned fixtures and exact source-file licence screening reduce untrusted-input and copyleft risks. Cue path normalization, sector-size overflow and GPL-derived helper files are the main footguns.
- **Product:** The planned v1 formats plus boot, XA, CD-DA, FMV and m3u swapping meet the phase goal. Supporting more compressed formats now would expand the settled library boundary.
- **Architecture and C:** CPU-cycle scheduling, fixed per-instance state, bounded queues, fixed-width arithmetic and host-supplied sectors preserve deterministic behavior. Synchronous host callbacks may cost wall time; they must not change emulated ticks.
- **Domain specialists:** Sector interrupt order, two-sector visibility, mode/filter behavior, XA history, MDEC FIFO states and the complete DMA chain are coupled. Unit-only success cannot establish disc boot or displayed FMV.
- **DevOps and CI:** Inventory every behavior test, run the pinned hardware comparisons in the nightly lane, keep fixture images build-only, and run the bounded RetroArch smoke in background. Preserve the total 300-line real-host test limit.
- **Adversarial countercheck:** A CD timing log may be only a band, MDEC PNG tolerance may hide a one-channel error class, and a matching hash can merely repeat a shared mistake. Keep evidence types separate and test each integration boundary with the specific source/reference named in the requirement.

## the agent's Discretion

Private state fields, helper names, parser/test partitioning and plan waves are implementation choices within the accepted scope. Resolve routine playback or scheduling details in code and focused tests; raise only a new consequence that changes scope, authority, risk, cost or a phase boundary.

## Deferred Ideas

- CHD, PBP, ECM, CCD and general SBI/LibCrypt support: v2.0.
- Hardware refits for seek curve, block timing or XA rounding: measured-accuracy horizon, when owner-hardware evidence exists.
- Browser disc UI, state/movie features and unrelated peripherals: later roadmap phases.
