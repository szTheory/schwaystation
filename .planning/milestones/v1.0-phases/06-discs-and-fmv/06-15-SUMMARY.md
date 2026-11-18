---
phase: 06-discs-and-fmv
plan: "15"
subsystem: disc-boot-fixture
tags: [disc, openbios, readn, toc]
actuals:
  tasks: 2
requirements-completed: []
coverage:
  - id: DISC-01-OPENBIOS-DISC-BOOT
    description: "The fast OpenBIOS image boots SYSTEM.CNF and starts the owned guest within the frame bound."
    requirement: DISC-01
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^disc\\.fixture\\.(boot|read)$' --no-tests=error --output-on-failure (exit 0, 2/2)."
      status: pass
    human_judgment: false
  - id: DISC-01-READN-TOC-AND-LICENSE
    description: "The guest queries GetTN/GetTD/GetID, reads sixteen sectors through CD-ROM DMA3, and matches independently derived sector bytes and CRC."
    requirement: DISC-01
    verification:
      kind: integration
      ref: "Focused disc.fixture.boot/read CTests (exit 0, 2/2); read case also checks SBI callbacks, sealed unique mailbox records, save/load replay and press@300 input."
      status: pass
    human_judgment: false
  - id: DISC-02-ALTERNATE-TOC-ASSET
    description: "The build produces a second owned image whose audio track index-1 differs by 75 sectors for the later playlist test."
    requirement: DISC-02
    verification:
      kind: integration
      ref: "cmake --preset ci (exit 0); cmake --build --preset ci --target schwaystation-test-disc-fixture schwaystation-disc-fixtures -j2 (exit 0); both --probe commands exit 0; CUE delta preflight exits 0."
      status: pass
    human_judgment: false
  - id: DISC-03-DISC-READ-REPLAY
    description: "Generated sector data, mailbox records and serialized state replay identically across the split run and input variation."
    requirement: DISC-03
    verification:
      kind: integration
      ref: "disc.fixture.read within the focused CTest command (exit 0); all sixteen DMA sectors, guest CRC, state replay and input comparison pass."
      status: pass
    human_judgment: false
metrics:
  duration: "The prior execution crossed a 22-hour pause; active runtime was not tracked reliably."
  runtime: "Codex/OpenGSD 1.20.0; primary GPT-6 session (exact model ID unavailable)."
completed: 2026-11-16
status: complete
---

# Phase 6 Plan 15: OpenBIOS disc boot and sector read

The owned disc now boots through the pinned fast OpenBIOS image and a generated guest exercises the real CD-ROM controller and DMA3 path. The fixture derives the expected sector data from the generated image, checks eight independently specified mailbox records, and verifies save/load replay. The host TOC lookup no longer adds the 150-frame lead-in a second time, which had classified boot LBA 16 as audio and prevented OpenBIOS from reading the disc.

## Work completed

- Corrected sector-to-track lookup to use the zero-based host LBA and added a cheap Mode 2 classification check before boot emulation.
- Added the guest's GetTN/GetTD/GetID and sixteen-sector ReadN path, with the CSV oracle, unique record IDs, independent sector CRC, SBI callback observations, and deterministic split-run/input checks.
- Kept the existing shell BIOS digest intact and added a separate digest for the pinned fast-boot OpenBIOS image used by the disc test.
- Added a second playlist-seed image with a three-second audio pregap. The first targeted build showed that mkpsxiso retained its default two-second gap; the splitter now reads the authored XML duration and pads only the missing silent sectors. A fast CUE check confirms the 75-sector TOC change.
- Recorded a public SCPH-5501 CD-ROM capture covering GetID, GetTN and GetTD in the preparation notes. The capture informs command behavior but does not replace the generated-disc DMA oracle.

## Verification

- `cmake --preset ci` — exit 0.
- `cmake --build --preset ci --target schwaystation-test-disc-fixture schwaystation-disc-fixtures -j2` — exit 0; builds both disc images and the fixture test.
- `ctest --test-dir build/ci -R '^disc\\.fixture\\.(boot|read)$' --no-tests=error --output-on-failure` — exit 0, 2/2.
- `cmake -DOPENBIOS_INPUT=build/openbios/nugget/openbios/openbios.bin -DEXPECTED_FILE=tests/built/fast-expected.txt -P cmake/OpenBIOS.cmake` — exit 0; 524,288-byte image matches its pinned digest.
- `build/ci/schwaystation-run --probe build/ci/fixtures/disc_boot.cue` and the matching `disc_boot_playlist.cue` command — both exit 0.
- CSV/XML/hash, inventory-sort and diff-whitespace preflights — exit 0. No hosted run URL applies; all recorded commands ran locally.

## Remaining acceptance

The plan-specific boot/read acceptance passes. Phase-wide DISC-01 through DISC-04 remain open while Plans 06-16 through 06-20 and 06-22 through 06-24 add the remaining media, FMV, runner and host integration slices. No PlayStation capture is needed for this plan.

## Next

Continue with Plan 06-16, then follow its remaining phase dependencies. Run the phase code review before final verification.
