---
id: SEED-001
status: dormant
planted: 2026-10-19
planted_during: preparation, before milestone v1.0
trigger_when: milestone v1.0 has shipped (PlayStation games play with sound in RetroArch) and the next milestone is about which discs, formats and peripherals work
scope: large
---

# SEED-001: The whole library plays: every format, every peripheral, saves and every region (v2.0)

## Why This Matters

After v1.0, cue/bin, iso and m3u discs run with a digital pad or DualShock, two memory cards and the NTSC-J,
NTSC-U/C and PAL profiles. What remains is what players and collectors notice next: CHD (the format most frontend
libraries hold), PBP, ECM and CCD, the LibCrypt titles that need `.sbi` bad-Q answers (about 100 PAL discs), the
multitap, GunCon and Justifier, mouse, neGcon and Jogcon, PocketStation card storage, per-game cards by serial, the
libretro save round trip, and the compatibility ledger over the whole libretro Redump DAT (13,407 entries, 3,573
multi-disc entries across 568 titles at `12c21cf79f1e`).

## When to Surface

**Trigger:** v1.0 shipped and the next milestone is about library breadth. Opens with SEED-007 (the tune-up),
which this time also builds the weekly triage digest, the CI-efficiency report and the `quality.json` ratchet.

## Scope Estimate

**Large**: a milestone. One phase per thing a player can see: the disc formats behind the one `schway_disc`
interface (libchdr BSD-3 behind `SCHWAYSTATION_CHD`, PBP, ECM, CCD, SBI with `fuzz.chd` and `fuzz.sbi`); saves
(per-game cards as a host policy, `.gme`/`.mem`/`.psx` import, the libretro `SAVE_RAM` round trip in-process and in
the second hosted RetroArch case, `libretro_saves "true"`); the peripherals at SIO0 protocol level (multitap with
cards on the tap, guns from the GPU-timestamped beam with a brightness test, mouse, neGcon, Jogcon, Dual Analog
flight mode, PocketStation storage commands); PAL timing, every board and GPU revision profile, the `fast` boot
option, GameShark strings and `--patch` (PPF1/2/3, BPS through the sector overlay) over the ledger, after which the
libretro `.info` drops `is_experimental`.

## Breadcrumbs

- `.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md` sections 2 to 6 and 8: pads, guns, PocketStation, card
  formats and naming, identity, PPF and cheats, the milestone table.
- `.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md` sections 2.6, 2.7 and 3: GetID, LibCrypt, disc formats.
- `.planning/preparation/HOSTS-AND-INTEGRATION.md` sections 1 and 4: extensions, subclasses, `.srm` and `.1.mcr`,
  the runner's save options, Playstead's save contract.
- `.planning/preparation/CONFORMANCE.md` section 3: the ledger, its ratchet and the local `library` preset.
- `.planning/preparation/ENGINEERING.md` section 7: the libchdr dependency rule and its licences.

## Notes

The light guns depend on the GPU phase recording the (line, dot) of each emitted pixel; schedule them after the
v1.0 display timing has held for a phase. CHD ships three more licences in every archive; a committed CHD fixture
under 512 KiB must prove hash equality on all eight lanes before the option defaults on.
