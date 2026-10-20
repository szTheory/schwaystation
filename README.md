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

## Install in RetroArch (macOS, Apple Silicon)

Paste this line into Terminal. It downloads the core and the release's checksum list, checks the core against
it, and only then puts the core and its information file into RetroArch's folders:

```sh
mkdir -p ~/Library/Application\ Support/RetroArch && cd "$(mktemp -d)" && curl -fsSL -O https://github.com/szTheory/schwaystation/releases/latest/download/schwaystation-libretro-macos-arm64.zip -O https://github.com/szTheory/schwaystation/releases/latest/download/SHA256SUMS && shasum -a 256 -c --ignore-missing SHA256SUMS && tar -xf schwaystation-libretro-macos-arm64.zip -C ~/Library/Application\ Support/RetroArch cores info
```

If the download was damaged or changed, `shasum` reports `FAILED` and nothing is installed. RetroArch 1.22.2 (the
Metal build) then lists **Sony - PlayStation (schwaystation)** under Load Core; after an update, delete
`~/Library/Application Support/RetroArch/info/core_info.cache` so RetroArch rereads the core's information.

On other systems, download `schwaystation-libretro-<os>-<arch>.zip` from the
[latest release](https://github.com/szTheory/schwaystation/releases/latest) (`<os>` is `linux`, `macos` or
`windows`; `<arch>` is `x64` or `arm64`), check it against `SHA256SUMS` from the same release, and copy its
`cores` and `info` folders into the folders RetroArch lists under Settings → Directory → Cores and Core Info.

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
