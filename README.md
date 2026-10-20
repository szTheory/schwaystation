# schwaystation

A Sony PlayStation emulator core in C: a small library, a headless runner (`schwaystation-run`) and a libretro
core (`schwaystation_libretro`) for RetroArch. One core covers every console revision and every region (NTSC-J,
NTSC-U/C and PAL).

**Status: early.** This release shows a built-in test card in RetroArch and in the runner; it does not run a
BIOS or a disc yet. Later releases add the CPU, the graphics and sound chips, the CD-ROM drive and the
controllers, and each one is published automatically.

## What you get

- **The library** (`libschwaystation`): the emulator core, for embedding in your own program.
- **The runner** (`schwaystation-run`): runs the core with no window and prints frame hashes, for scripts and
  tests.
- **The libretro core** (`schwaystation_libretro`): loads in RetroArch or another libretro frontend.

## Install in RetroArch

The one-line install for macOS on Apple Silicon is added with the first release.

## Quick start: runner

Download `schwaystation-runner-<os>-<arch>.zip` from the latest release, unzip it, and run 60 frames:

```sh
bin/schwaystation-run --frames 60 --hash
```

It prints the SHA-256 of the last frame's picture, the same on every platform:

```text
frame 60 sha256 be3538480b252f5bcfe7cb5b876a71c72d3795621b4278e405af61c425c06d71
```

Add `--region ntsc-j` or `--region pal` for the other regions' timing. The test card is a white border, a row
counter on the left, red, green, blue and grey ramps, a one-dot checkerboard and a frame counter that changes
every frame.

## BIOS and discs

You bring your own BIOS and disc images, dumped from hardware you own; schwaystation distributes neither
([ASSET_POLICY.md](ASSET_POLICY.md)).

## Documentation

- [docs/runner.md](docs/runner.md): `schwaystation-run` options, output, exit statuses and the test card.
- [docs/embedding.md](docs/embedding.md): using the C library from CMake or pkg-config.
- [docs/testing.md](docs/testing.md): building and running the tests.
- [docs/maintaining.md](docs/maintaining.md): releases and repository settings, for maintainers.

## Legal

Code from other projects enters only under permissive licences with their notices
([PROVENANCE.md](PROVENANCE.md)). The project is not affiliated with or endorsed by Sony Interactive
Entertainment.

## Licence

[MIT](LICENSE). Every release archive also carries [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md).
