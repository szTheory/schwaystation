# Disc patch support

Patch parsing stays in the host disc provider. A patch is prepared as a candidate and becomes visible to sector reads only after its source identity, complete record stream, and geometry have passed validation. The original track file is read-only and is never rewritten.

## Runner CLI

Attach one patch to a disc run with `--patch FILE`. PPF1, PPF2 and PPF3 also require the
SHA-256 of the complete raw source file through `--patch-source-sha256 HEX`; BPS may omit
this extra identity check because its source, target and patch CRCs are always validated.
A supplied BPS source SHA is checked in addition to those CRCs. The runner reports the
format, source identity when supplied, and the checks it performed without printing the
input paths.

Patches require `--disc` and cannot be combined with `--bench` or `--probe`. Repeated
patch options, malformed SHA text, a SHA option without a patch, and a missing PPF SHA
are usage errors (exit 2). A bad patch, failed identity check or patch-file I/O error
exits 1; an unsupported patch type or disc mapping exits 77. The runner refuses a patch
before creating the emulator instance, running frames or writing a requested PNG.
`--hash-state` remains unavailable with any `--disc` run.

For example, a PPF run binds the patch to the entire raw source image before applying its overlay:

```sh
schwaystation-run --bios ./openbios.bin --disc ./game.cue --patch ./game.ppf \
  --patch-source-sha256 0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef \
  --frames 900
```

BPS can rely on its native CRC checks, or add the same source SHA option when the patch
must be bound to a specific source image. The patching path changes only the bytes returned
by the host sector provider; it does not add a patch operation to the emulated core.

## PPF1 subset

This implementation accepts PPF1 (`PPF10`, method 0), PPF2 (`PPF20`, method 1), and the BIN subset of PPF3 (`PPF30`, method 2). Format facts follow the [PPF developer format reference](https://raw.githubusercontent.com/meunierd/ppf/4f24f8c784faf27b54081caee3835fa59a8a009e/ppfdev/PPF3.txt); the test fixtures in this repository are self-authored from that layout.

Every PPF version requires the caller-supplied SHA-256 of the complete raw source track. The report lists that identity check and each native check that passed. PPF1 has no native size or block check. PPF2 also requires its 32-bit original image size to equal the source size and its 1,024-byte header block to equal source bytes beginning at `0x9320`. Both native PPF2 checks must pass. PPF3 has no source-size field; its header selects either a 60-byte form without block validation or a 1,084-byte form with a 1,024-byte source block at `0x9320`. PPF3 block validation is optional. These size and block checks are format checks, not cryptographic identity checks; the full-source SHA-256 supplies the external identity binding.

PPF2 and PPF3 records use nonzero byte counts and little-endian raw-file offsets. PPF2 uses a 32-bit offset; PPF3 uses a 64-bit offset. Each PPF3 record carries the forward replacement bytes and, when the header enables undo data, an equally sized undo payload. Undo bytes are bounds-checked and consumed but are not applied. Only BIN PPF3 image type is supported; GI and unknown image types, invalid flags, nonzero reserved bytes, malformed records, and truncated or mismatched FileID trailers are rejected. A PPF3 FileID trailer is optional and must use the exact `@BEGIN_FILE_ID.DIZ` and `@END_FILE_ID.DIZ` markers, a matching little-endian 16-bit description length, and at most 3,072 description bytes. PPF2 FileID trailers are not admitted by this subset.

Every version rejects empty records, out-of-source ranges, malformed tails, and any invalid later record before creating a candidate. Record order is retained, so later overlapping records win. The PPF3 header offsets, record offsets, undo layout and FileID trailer follow the pinned developer text; PPF2 method and size fields follow that method-1 layout and the repository's self-authored byte fixtures.

The limits are 16 MiB for patch bytes and 262,144 records. Records are resolved in file order while the candidate is built, so a later record wins for every byte it overlaps. Reads use the resulting sparse overlay; they do not scan the input record stream.

## BPS1

BPS1 supports SourceRead, TargetRead, SourceCopy and TargetCopy, including overlapping TargetCopy from bytes already produced. The complete target is staged and checked before it can replace the current overlay. Source, target and patch CRC-32 values cover the complete source, complete target and all patch bytes before the trailing patch-CRC field, respectively. CRCs detect accidental corruption but do not provide cryptographic identity. A caller may also supply the complete source SHA-256 when stronger source binding is needed; BPS's native checks remain required. Format details follow the [BPS File Format Specification](https://floating.muncher.se/byuu/bps/bps_spec.html).

BPS source and target images are limited to 1 GiB, patch bytes to 64 MiB and metadata to 1 MiB. The target byte count must equal the source byte count, preserving raw-sector geometry. Targets up to 1 MiB use memory; larger targets use a private temporary file. If staging or I/O fails, the candidate is rejected and its temporary storage is closed; the source and any active overlay remain unchanged.

## Supported raw track mapping

The patch source must be exactly one CUE `FILE` with one data `TRACK`, using `MODE1/2352` or `MODE2/2352`. `INDEX 01` must begin at file offset zero. The provider rejects `INDEX 00`, any physical or virtual pregap, additional indexes or tracks, playlists, cooked 2048-byte sectors, and compressed or provider-specific images.

PPF offsets address the raw file directly: `LBA = offset / 2352`, and `byte within sector = offset % 2352`. The same 2352 bytes are returned to the core, including raw MODE1 headers. No cooked-sector reconstruction is applied to an accepted raw track.

## Validation and replacement

Candidate loading reads and hashes the complete source, validates the PPF1 header and every record, enforces source bounds with subtraction-based checks, then builds a bounded sorted overlay. Loading does not change the current image reader. Attaching a candidate requires the same source length, raw-sector geometry, read callback, and image instance used to validate it.

The image takes ownership only after that source binding succeeds. A malformed patch, wrong digest, unsupported geometry, or candidate for another image leaves the prior reader and overlay in place. Applying an overlay changes only the sector buffer returned by the provider; opening, hashing, and reading a patch never writes the source file.
