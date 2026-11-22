# Disc images

The runner and libretro host use `schway_disc_image` to provide sector data to
the core. File access stays in the host. The provider accepts cooked `.iso`
images and `.cue` sheets whose data files contain MODE1/2048, MODE2/2352, or
AUDIO tracks. It also accepts the fixture-proven unencrypted PBP subset below.
With `SCHWAYSTATION_CHD=ON`, it opens the bounded CHD subset below through the
same raw-sector and TOC callbacks. `SCHWAYSTATION_CHD` is off by default;
`.ccd` remains unsupported.

## ECM images

The provider reads ECM v1.0 literal and modeled records without materializing
the decoded image. It validates the complete decoded-byte checksum and exposes
bounded random reads. A bare `.ecm` has no TOC, so it uses the existing single
MODE2/2352 fallback and must decode to an exact sequence of 2352-byte sectors.

A CUE may name an `.ecm` FILE child for MODE2/2352 or AUDIO tracks. Track
indexes and pregaps use the decoded extent, and reads match the equivalent
uncompressed raw CUE. ECM has no track table: the provider rejects cooked
2048-byte CUE sources and never invents the missing 16-byte sector header for
type 2 or type 3 records. Canonical child paths stay inside the CUE directory;
an open error leaves the caller's active disc unchanged.

## CHD metadata subset

`SCHWAYSTATION_CHD` is off by default. The bounded header reader recognizes
CHD v1 through v5 header layouts, but the PlayStation CD metadata path currently
accepts v3 through v5 with sequential `CHTR` or `CHT2` track records. CHD v1
and v2, images without track records, non-track metadata, mixed metadata tags,
and non-CD layouts are unsupported.

The accepted track types are `AUDIO` and raw `MODE1_RAW` or `MODE2_RAW` data.
Track and pregap subcode types must be `NONE`; subcode-bearing CHDs are
unsupported. `POSTGAP` must be zero. CHT2 pregaps with a `V`-prefixed `PGTYPE`
are stored in the track frames; an unprefixed pregap is virtual. Both require
the pregap type to match the track type. Stored pregap frames count in
`FRAMES`; virtual pregap frames do not. Track order and counts are checked
before narrowing to public TOC fields.

The CHD storage stream pads each track to a four-frame boundary. The reader
includes those frames when reconciling the CHD logical byte count, but excludes
them from public LBAs and leadouts. Virtual pregap sectors use the same blank
sector construction as the CUE provider; stored pregaps map to their source
frames. CHD track and pregap subcode metadata must say `NONE`; returned subcode
bytes come from the CHD sector stream, with the owned no-subcode fixture
returning 96 zero bytes per sector.

The media preflight checks the owned CUE/CHD inputs, manifest digests and track
geometry before guest runs. `disc.image.chd_parity` checks all 222 logical
sectors in forward and reverse order, sector guards, TOC equality, absent
subcode bytes and content-based save identity. `disc.image.chd_open` checks
shared routing and failed-open isolation, while `disc.image.chd_boundaries`
covers track/index/leadout and each eight-frame hunk transition.
`disc.image.chd_headers`, `disc.image.chd_metadata`, and `disc.image.chd_toc`
cover header bounds, metadata variants, and normalized TOC fields. The bounded
owned guest reuses its CUE expectations and completion limits through CHD in
`disc.fixture.chd.boot`, `.read`, `.audio`, and `.fmv`; the media parity and
feature-key preflights finish before these cases run. `runner.disc.chd` and
`libretro.content.chd` cover the two shipped hosts, and `release.archive_contents`
plus `release.notices` check the CHD-enabled and disabled package outputs.

Open errors retain the shared image status: malformed headers or geometry use
`BAD_IMAGE`, unsupported versions, parent images and metadata use
`UNSUPPORTED`, configured byte/hunk/track bounds use `LIMIT`, and open-time
file-read failures use `IO`. A later hunk decode failure makes the sector
callback return zero. A failed open leaves the image pointer null; its error
message identifies the rejected condition.

The host limits image data to 1 GiB, CHD hunks to 16 MiB, and CHD files to
65,536 hunks. A compressed v5 map is limited to 16 MiB; combined compressed
and decoded map storage is limited to 17 MiB. Header, map, metadata and hunk
extents are checked before libchdr decodes the image. CHD parent-image
references and CHD versions 1 and 2 are unsupported.

## PBP subset

The provider accepts the owned, unencrypted single-disc PSISO layout: one
MODE2 data track with A0, A1, and A2 lead-in records, followed by track 1. The
track record's absolute start is `00:02:00`; subtracting the 150-frame lead-in
maps it to the source CUE's INDEX 01 at LBA 0. A2 leadout is also absolute MSF
and includes those 150 frames, so the public leadout is the decoded frame
count. The index contains contiguous data extents in 16-sector blocks. Each
decoded block is at most `0x9300` bytes. Open validates every indexed stream
before publishing a playable handle, using one decoded-block buffer; PBP files
and logical media are limited to 1 GiB. PBP works with `SCHWAYSTATION_CHD=OFF`
and without system zlib.

PGD, multi-disc markers, audio or additional tracks, other control/mode
variants, unknown markers, and unknown TOC layouts return `UNSUPPORTED`.
Malformed fields, inconsistent tables or leadout, and truncated or overlapping
extents return `BAD_IMAGE`. The runner reports unsupported media with exit 77
and malformed media with exit 1, before guest execution. The owned runner test
compares probe output, a bounded frame hash, and save identity with the
equivalent single-track CUE. See [media testing](media-testing.md#pbp-provider)
for the focused fixture and provider checks.

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

The host disc API exposes the same allocation-free parser used by the identity
probe. It accepts at most 2,047 ASCII bytes and requires one
`BOOT = cdrom:\path;1` entry. Paths have at most 16 safe components. Each
component and the canonical boot name are bounded to 63 characters. Empty, `.` and `..`
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
