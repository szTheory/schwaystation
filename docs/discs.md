# Disc images

The runner and libretro host use `schway_disc_image` to provide sector data to
the core. File access stays in the host. The provider currently accepts cooked
`.iso` images and `.cue` sheets whose data files contain MODE1/2048,
MODE2/2352, or AUDIO tracks. It rejects `.chd`, `.pbp`, `.ecm`, and `.ccd` as
unsupported formats; those formats are not sniffed or partially opened.

## Playlists

An `.m3u` file is ASCII text containing up to 32 relative paths, one path per
line. Blank lines and lines whose first non-space character is `#` are
ignored. CRLF and LF line endings are accepted. Each path is at most 255 bytes;
the playlist is at most 64 KiB. Entries may name `.iso` or `.cue` files and may
use subdirectories, but absolute paths, drive prefixes, parent traversal,
recursive playlists, and paths that resolve through a symlink outside the
playlist directory are rejected. A CUE entry's own data-file paths are checked
against that CUE file's directory as well. Unsupported image extensions are
reported as unsupported whether opened directly, listed in a playlist, or
used as a CUE data file.

`schway_disc_image_count()` returns the number of entries (one for a
single-image provider). `schway_disc_image_select()` returns a pointer to the
selected entry's `schway_disc` descriptor without changing the core's current
disc. The host decides when to pass that descriptor to the core's disc-swap
operation. The provider owns every opened file and descriptor until
`schway_disc_image_close()`; selected descriptors and the strings returned by
`schway_disc_image_path()` and `schway_disc_image_label()` remain valid only
until that close. Paths are canonical paths; labels are the bounded entry
basenames.

Open failures leave the output image pointer null. Playlist errors use
`SCHWAY_DISC_IMAGE_BAD_M3U` or `SCHWAY_DISC_IMAGE_LIMIT`; the error includes
the source line where available. Unsupported formats use
`SCHWAY_DISC_IMAGE_UNSUPPORTED` and a named format message. The provider
closes all children already opened when a later entry fails.

## SYSTEM.CNF metadata

The image provider and identity probe use the same allocation-free parser. It
accepts at most 2,047 ASCII bytes and requires one `BOOT = cdrom:\path;1`
entry. Paths can have up to 16 safe components; each component and the
canonical boot name are bounded to 63 characters. Empty, `.` and `..`
components are rejected. Optional `TCB`, `EVENT`, and `STACK` fields are
validated as integers: TCB is at most `0x40`, EVENT at most `0x100`, and STACK
must address the 2 MiB RAM range through KUSEG, KSEG0, or KSEG1. Duplicate
recognized fields, malformed numbers, control bytes, and non-ASCII bytes are
rejected with `SCHWAY_DISC_IMAGE_BAD_SYSCNF`. Parsing leaves its output
unchanged on failure.

When the identity probe finds no `SYSTEM.CNF`, it looks for `PSX.EXE` in the
ISO root. If neither exists, it returns `SCHWAY_HOST_DISC_PROBE_NO_BOOT_EXE`.
An invalid or unreadable `SYSTEM.CNF` is an error and does not silently fall
back. The probe transfers its allocated executable buffer only on success;
the caller releases it with `schway__disc_probe_dispose()`.

The CUE and SYSTEM.CNF parsers do not parse SBI subchannel patch files.
