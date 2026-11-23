# libretro core

The libretro module is `schwaystation_libretro`. Install it in RetroArch's `cores/` directory and install the generated
`schwaystation_libretro.info` in `info/`.

## Content

The core accepts full-path `.exe` and `.psexe` homebrew, plus `.cue`, `.iso`, `.m3u`, `.ecm`, `.ccd`, and supported
`.pbp` disc content. `.chd` is available when the core is built with `SCHWAYSTATION_CHD=ON`. PBP support is limited
to unencrypted, single-data-track PSISO images; encrypted PGD and undocumented variants fail to load. ECM and
CloneCD support is limited to the documented subsets; CloneCD uses a same-stem `.img` and optional `.sub` file.
Archives are not extracted. Cue sheets resolve their track files relative to the cue; M3U entries resolve relative
to the playlist. A raw track file such as `.bin` is opened through its cue sheet. See [disc images](discs.md) for
format limits and unsupported layouts.

Disc content boots through a BIOS in RetroArch's system directory. With the BIOS option set to `none`, the core looks
for `openbios-fast.bin` there; the checked OpenBIOS artifact is used by CI and is not included in release packages.
The BIOS option can instead name `scph5500.bin`, `scph5501.bin`, or `scph5502.bin` in the same directory. PS-EXE
content uses the built-in stub and does not need a BIOS. With no content, `none` keeps the built-in test card.

## Disc changes

M3U playlists expose the libretro extended disk-control interface. Ejecting opens the emulated tray; selecting a
playlist member swaps its TOC while the tray is open; selecting an index at or above the image count removes the disc
and lets the frontend close the empty tray. The core reports each image's path and label and supports adding and
replacing playlist entries.

The frontend owns file access. Keep the cue, its tracks, and any M3U playlist together at stable paths while a game is
loaded.
