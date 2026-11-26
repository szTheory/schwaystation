# Disc patch support

Patch parsing stays in the host disc provider. A patch is prepared as a candidate and becomes visible to sector reads only after its source identity, complete record stream, and geometry have passed validation. The original track file is read-only and is never rewritten.

## PPF1 subset

This implementation accepts PPF1 (`PPF10`, method 0). The 56-byte header contains the five-byte signature, method byte, and 50-byte description. Each following record contains a 32-bit little-endian byte offset, a one-byte nonzero byte count, and that many replacement bytes. Empty, truncated, out-of-range, or trailing partial records are rejected. The format facts follow the [PPF developer format reference](https://raw.githubusercontent.com/meunierd/ppf/4f24f8c784faf27b54081caee3835fa59a8a009e/ppfdev/PPF3.txt); the test fixtures in this repository are generated independently.

The whole source track must match the caller-supplied SHA-256. PPF1 has no native source identity check, so the report records the full SHA-256 check as the identity evidence. PPF2, PPF3, and BPS are selected by the shared patch API contract but are unsupported until their corresponding validators are implemented.

The limits are 16 MiB for patch bytes and 262,144 records. Records are resolved in file order while the candidate is built, so a later record wins for every byte it overlaps. Reads use the resulting sparse overlay; they do not scan the input record stream.

## Supported raw track mapping

The patch source must be exactly one CUE `FILE` with one data `TRACK`, using `MODE1/2352` or `MODE2/2352`. `INDEX 01` must begin at file offset zero. The provider rejects `INDEX 00`, any physical or virtual pregap, additional indexes or tracks, playlists, cooked 2048-byte sectors, and compressed or provider-specific images.

PPF offsets address the raw file directly: `LBA = offset / 2352`, and `byte within sector = offset % 2352`. The same 2352 bytes are returned to the core, including raw MODE1 headers. No cooked-sector reconstruction is applied to an accepted raw track.

## Validation and replacement

Candidate loading reads and hashes the complete source, validates the PPF1 header and every record, enforces source bounds with subtraction-based checks, then builds a bounded sorted overlay. Loading does not change the current image reader. Attaching a candidate requires the same source length, raw-sector geometry, read callback, and image instance used to validate it.

The image takes ownership only after that source binding succeeds. A malformed patch, wrong digest, unsupported geometry, or candidate for another image leaves the prior reader and overlay in place. Applying an overlay changes only the sector buffer returned by the provider; opening, hashing, and reading a patch never writes the source file.
