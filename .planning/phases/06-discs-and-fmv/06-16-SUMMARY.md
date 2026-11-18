---
phase: 06-discs-and-fmv
plan: "16"
subsystem: disc-fixtures-audio-fmv
tags: [disc, xa, cdda, str, mdec, gpu]
actuals:
  tasks: 2
requirements-completed: []
coverage:
  - id: DISC-02-XA-CDDA-NATIVE-PCM
    description: "The guest reads authored XA and CD-DA sectors; both decode queues contain nonzero samples and the public native PCM stream is nonzero."
    requirement: DISC-02
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^disc\\.fixture\\.audio$' --no-tests=error --output-on-failure (exit 0); no hosted run URL applies."
      status: pass
    human_judgment: false
  - id: DISC-03-STR-MDEC-GPU-RGB24
    description: "The guest reads the authored STR sectors through DMA3, demuxes the MDEC stream, decodes RGB24 through DMA0/1 and uploads it through DMA2; displayed pixels match MDEC output RAM and retain the channel pattern."
    requirement: DISC-03
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^disc\\.fixture\\.fmv$' --no-tests=error --output-on-failure (exit 0); no hosted run URL applies."
      status: pass
    human_judgment: false
  - id: DISC-02-03-AV-REGRESSION-PINS
    description: "None and press@300 runs pin PCM and RGB24 hashes and reproduce the same audio, frame, mailbox and final state across a save/load split."
    requirement: DISC-02
    verification:
      kind: integration
      ref: "disc.fixture.audio and disc.fixture.fmv in the final four-test fixture command (exit 0, 4/4); record-mode capture command exited 0 and produced the matching local hashes. No hosted run URL applies."
      status: pass
    human_judgment: false
  - id: DISC-02-OWNED-IMAGE-CAP
    description: "Generated XA, CD-DA and STR coverage is checked before emulation, and the primary two-track build is rejected above 512 KiB."
    requirement: DISC-02
    verification:
      kind: integration
      ref: "python3 tools/gen_disc.py --check-media build/ci/fixtures/media (exit 0: XA=7056, CD-DA=4704, STR=1); fixture build passed the 524288-byte cap at 522144 bytes total. No hosted run URL applies."
      status: pass
    human_judgment: false
metrics:
  duration: "The owner reported more than 22 hours elapsed during this autonomous run; active runtime was not measured reliably."
  runtime: "Codex/OpenGSD 1.20.0; primary GPT-6 session (exact model ID unavailable); inline code review."
completed: 2026-11-17
status: complete
---

# Phase 6 Plan 16: XA, CD-DA and STR playback

The owned disc guest now routes authored XA and CD-DA through the CD-ROM and SPU paths, then reads one authored STR frame through CD-ROM DMA3, MDEC DMA0/1 and GPU DMA2 into native RGB24 video. The fixture checks source queues and native output independently, pins PCM and frame hashes for no-input and press@300 runs, and verifies deterministic split-state replay. These are generated-media regressions, not a physical PlayStation capture.

## Work completed

- Added a cheap generated-media preflight for XA sector headers and decoded coverage, CD-DA WAV geometry, and complete STR chunk/frame metadata. Normal CMake configure checks supplied media after verifying its hashes.
- Kept the complete primary two-track fixture below the 512 KiB cap. Reducing XA to three complete sectors saves one data-track sector; the guest and predicates follow the resulting XA, CD-DA INDEX 1, and STR LBAs. The splitter checks exact final artifact bytes before writing the output.
- Added guest XA filtering/playback, CD-DA playback, queued-sector drainage, STR chunk DMA, one-block MDEC RGB24 decode and GPU upload.
- Added source-sector, filter/subheader, nonzero XA/CD-DA queue, public PCM, DMA-output/display equality, per-channel image and state-replay predicates.
- Registered `disc.fixture.audio` and `disc.fixture.fmv`; added deterministic PCM and RGB24 hashes for both input variants. Existing OpenBIOS and ReadN predicates remain intact.
- The repository email-address hygiene check matched the original symbol-based internal pin labels; renamed them with hyphens and reran the affected audio/FMV tests.
- Inline code review found and fixed the initial image-cap overage and a sector-address shift caused by reducing XA coverage. Final review has no outstanding findings.

## Verification

- `python3 tools/gen_disc.py --check-media build/ci/fixtures/media` — exit 0; XA=7056 decoded frames, CD-DA=4704 frames, STR=1 complete frame.
- `cmake --preset ci` — exit 0.
- `cmake --build --preset ci --target schwaystation-test-disc-fixture schwaystation-disc-fixtures -j2` — exit 0; both primary and alternate-TOC images built.
- `wc -c build/ci/fixtures/disc_boot.bin build/ci/fixtures/disc_boot_track02.bin` — exit 0; 522,144 bytes total, below 524,288.
- `build/ci/tests/schwaystation-test-disc-fixture record build/ci/openbios/openbios-fast.bin tests/built/fast-expected.txt build/ci/fixtures/disc_boot.cue tests/fixtures/src/disc_boot.sbi tests/diag/expected.csv` — exit 0; PCM hash `0bafcc0656c54eeca8e4012915ef6f875f219cf7d6c3b8a19e930706a5701d79`, frame hash `12f05a7a951a57be6b3348b7ce469049bde78325f952f771928cc9ff35bf0850` for both variants.
- `ctest --test-dir build/ci -R '^disc\\.fixture' --no-tests=error --output-on-failure` — exit 0, 4/4; total 22.47 seconds.
- After the hygiene-safe pin-label edit, `cmake --build --preset ci --target schwaystation-test-disc-fixture` and `ctest --test-dir build/ci -R '^disc\\.fixture\\.(audio|fmv)$' --no-tests=error --output-on-failure` — both exit 0; 2/2 affected fixtures.
- `git diff --check` — exit 0. No hosted run URL applies; all commands ran locally.

## Remaining acceptance

The plan's authored-media playback and native output acceptance passes. The broader DISC-02 and DISC-03 requirements remain open for runner, libretro, oracle, fuzz and hosted per-platform coverage in later plans. No owner hardware capture is needed for this plan, and none is claimed.

## Next

Continue with Plan 06-17: runner disc loading and deterministic lid/swap scripts.
