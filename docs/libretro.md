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

`Boot (reload content)` defaults to `full`, which follows the BIOS shell. `fast` skips the shell only when the selected
content is a supported disc with a valid `SYSTEM.CNF` boot executable and the selected BIOS file is available. A
missing BIOS, non-bootable disc, PS-EXE, or no-content load is refused while `fast` is selected. Reload content to
apply a boot-mode change; selecting `full` keeps the normal BIOS path and the BIOS-free PS-EXE stub route.

## Machine and region

`Machine (reload content)` selects one of the documented boards: `pu7`, `pu8-early`, `pu8-late`, `pu18`, `pu20`,
`pu22`, `pu23`, `pm41`, or `pm41-2`. It defaults to `pu18`, matching the settled v2 GPU and vC2 CD firmware
defaults. `Region (reload content)`
defaults to `auto`. Auto checks the disc serial first, then the disc licence area, then the boot executable's licence
marker; if none is known it uses NTSC-U/C. Conflicting hints are reported through the frontend log. Selecting NTSC-U/C,
NTSC-J, or PAL overrides the inferred region. Machine and region are independent choices, and both take effect when
content is loaded or reloaded. Region selection does not establish title compatibility.

## Controller input

Supported controllers, including GunCon and Justifier mappings, are documented in [controllers](controllers.md).
Lightguns require the frontend's `input_state` callback and either `LIGHTGUN` or `POINTER` device-type capability.
`LIGHTGUN` takes precedence when both are advertised; the POINTER fallback uses only X/Y, PRESSED and IS_OFFSCREEN,
so it cannot express forced reload. Libretro does not report support for individual controls, so a neutral zero
cannot distinguish a missing control from a released one. When neither host device type is available, the core
logs the transition when a log interface is available and supplies no valid gun aim.

## Disc changes

M3U playlists expose the libretro extended disk-control interface. Ejecting opens the emulated tray; selecting a
playlist member swaps its TOC while the tray is open; selecting an index at or above the image count removes the disc
and lets the frontend close the empty tray. The core reports each image's path and label and supports adding and
replacing playlist entries.

The frontend owns file access. Keep the cue, its tracks, and any M3U playlist together at stable paths while a game is
loaded.
