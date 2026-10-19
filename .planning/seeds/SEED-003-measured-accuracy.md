---
id: SEED-003
status: dormant
planted: 2026-10-19
planted_during: preparation, before milestone v1.0
trigger_when: the compatibility ledger has a stable failing set that documentation alone does not explain, or the owner wants hardware-measured timing
scope: large
---

# SEED-003: Measured accuracy: hardware fixtures, oracles and the fitted constants settled (v4.0)

## Why This Matters

Several PlayStation timings (GPU clocks per line, the CD seek curve and INT1 period, per-primitive GPU cost, MDEC
block cost, ADSR edge rates, the XA resampler rounding, reverb rounding, interrupt latency, memory-control and DMA
per-word costs) are documented from emulator inference, not from measurement. The owner has real consoles and
discs. The own fixtures already print numbers (no verdicts) into a RAM mailbox and on screen with a CRC, so the same
PS-EXEs run on hardware settle each fitted constant; released binaries of PCSX-Redux, DuckStation, Mednafen, ares
and MAME locate divergences; nobody publishes hardware-verified PS1 timing today.

## When to Surface

**Trigger:** a stable failing set in the ledger, or the owner's hardware session. Not before v2.0.

## Scope Estimate

**Large**: a milestone: the hardware-run protocol (loader, capture path, `session.csv`, the CRC check against the
photographed mailbox); hardware runs of `timers_lines`, `exc_matrix`, `gte_vectors`, `spu_tone` and `disc_boot`
plus a seek-curve fixture at three distances; the nightly oracle lanes (PCSX-Redux with OpenBIOS under Xvfb in CI;
DuckStation regtest, Mednafen, ares and MAME in the local preset) with per-frame RAM, VRAM and register comparison
and the alignment search; amidog and PeterLemon as local binary oracles; the constants replaced with measured values
and the `(unverified)` markers retired; per-line hooks for the homebrew debugger audience.

## Breadcrumbs

- `.planning/preparation/CONFORMANCE.md` sections 1, 3.1, 3.4 and 4: oracles, comparison fields, the hardware-run
  protocol.
- `.planning/preparation/PSX-SYSTEM.md` section 10, `PSX-GPU-AND-GTE.md` sections 7 and 10,
  `PSX-SPU-CDROM-AND-MDEC.md` section 7: accuracy claims and confidence.
- `.planning/preparation/DECISIONS.md`: the open questions at its end.
