# Phase 9 CHD fixture-generator audit

Status: **cleared for the build-only C/C++ route**. No generator was compiled or
run during this audit.

## Selected public pin

Use the vendored CHD source subset in [`danifunker/libchdman-rs` at
`7f71bdf3d4099dc38bded49c6745e453005d6e41`](https://github.com/danifunker/libchdman-rs/tree/7f71bdf3d4099dc38bded49c6745e453005d6e41).
The exact codeload archive SHA-256 is
`7346f16486f114af808c6b361228ce9c816156bfd910c8ca8f7727a21d2b6a89`.
Its `mame-chd-src/MAME_VERSION` identifies base MAME commit
[`f34f02505e32c1993c6a782b6814232cbfc74e36`](https://github.com/mamedev/mame/tree/f34f02505e32c1993c6a782b6814232cbfc74e36),
which the GitHub commits API confirms is public. The candidate's `deps/mame`
gitlink points to that same public MAME commit. Its vendored `mame-chd-src`
subset is present in the archive; `deps/mame` is an empty archive directory
used only to regenerate that subset and is not part of this build closure. No
submodules are fetched.

The candidate's `mame-chd-src/README.md` says the copied MAME files are
unmodified. That claim is not fully accurate: `src/lib/util/chd.cpp` has a
small downstream buffer-sizing patch for short hunk maps. Its file header
remains `BSD-3-Clause`; the exact patched bytes are protected by the candidate
archive digest above. Treat the archive commit and digest as the source pin,
and the MAME commit as the upstream base, rather than claiming byte identity to
the MAME tree. The patch adds space for the RLE-exported Huffman tree and does
not add code from another project.

The original MAME `chdman` executable was not selected: its aggregate `utils`,
`expat`, `7z`, and OSD targets would add a broader dependency closure. The
selected build uses the candidate's pinned CHD core and CD compressor shim,
called by a small project-owned C++ fixture tool. It does not compile the
candidate's Rust wrapper, Cargo dependencies, optional prebuilt archives,
examples or tests; it does not compile MAME's `chdman.cpp`; and it does not
read or build `src/mame/sony/psx.cpp` or any forbidden emulator source.

## Exact portable source set

The build-only tool compiles the following sources from the pinned archive.
The portable profile omits platform SIMD files; Windows additionally compiles
`mame-chd-src/3rdparty/flac/src/share/win_utf8_io/win_utf8_io.c`.

```text
sys/chd_shim.cpp
sys/cd_shim.cpp
sys/minimal_osd.cpp
mame-chd-src/src/lib/util/avhuff.cpp
mame-chd-src/src/lib/util/bitmap.cpp
mame-chd-src/src/lib/util/cdrom.cpp
mame-chd-src/src/lib/util/chd.cpp
mame-chd-src/src/lib/util/chdcodec.cpp
mame-chd-src/src/lib/util/corefile.cpp
mame-chd-src/src/lib/util/corestr.cpp
mame-chd-src/src/lib/util/flac.cpp
mame-chd-src/src/lib/util/hashing.cpp
mame-chd-src/src/lib/util/huffman.cpp
mame-chd-src/src/lib/util/ioprocs.cpp
mame-chd-src/src/lib/util/md5.cpp
mame-chd-src/src/lib/util/palette.cpp
mame-chd-src/src/lib/util/path.cpp
mame-chd-src/src/lib/util/strformat.cpp
mame-chd-src/src/lib/util/unicode.cpp
mame-chd-src/src/lib/util/vecstream.cpp
mame-chd-src/src/osd/strconv.cpp
mame-chd-src/3rdparty/lzma/C/7zCrc.c
mame-chd-src/3rdparty/lzma/C/7zCrcOpt.c
mame-chd-src/3rdparty/lzma/C/Alloc.c
mame-chd-src/3rdparty/lzma/C/CpuArch.c
mame-chd-src/3rdparty/lzma/C/LzFind.c
mame-chd-src/3rdparty/lzma/C/Lzma2Dec.c
mame-chd-src/3rdparty/lzma/C/Lzma2Enc.c
mame-chd-src/3rdparty/lzma/C/LzmaDec.c
mame-chd-src/3rdparty/lzma/C/LzmaEnc.c
mame-chd-src/3rdparty/zlib/adler32.c
mame-chd-src/3rdparty/zlib/compress.c
mame-chd-src/3rdparty/zlib/crc32.c
mame-chd-src/3rdparty/zlib/deflate.c
mame-chd-src/3rdparty/zlib/inffast.c
mame-chd-src/3rdparty/zlib/inflate.c
mame-chd-src/3rdparty/zlib/inftrees.c
mame-chd-src/3rdparty/zlib/trees.c
mame-chd-src/3rdparty/zlib/uncompr.c
mame-chd-src/3rdparty/zlib/zutil.c
mame-chd-src/3rdparty/utf8proc/utf8proc.c
mame-chd-src/3rdparty/zstd/lib/common/debug.c
mame-chd-src/3rdparty/zstd/lib/common/xxhash.c
mame-chd-src/3rdparty/zstd/lib/common/pool.c
mame-chd-src/3rdparty/zstd/lib/common/entropy_common.c
mame-chd-src/3rdparty/zstd/lib/common/error_private.c
mame-chd-src/3rdparty/zstd/lib/common/fse_decompress.c
mame-chd-src/3rdparty/zstd/lib/common/threading.c
mame-chd-src/3rdparty/zstd/lib/common/zstd_common.c
mame-chd-src/3rdparty/zstd/lib/decompress/zstd_decompress_block.c
mame-chd-src/3rdparty/zstd/lib/decompress/zstd_decompress.c
mame-chd-src/3rdparty/zstd/lib/decompress/zstd_ddict.c
mame-chd-src/3rdparty/zstd/lib/decompress/huf_decompress.c
mame-chd-src/3rdparty/zstd/lib/compress/zstd_lazy.c
mame-chd-src/3rdparty/zstd/lib/compress/zstd_compress_superblock.c
mame-chd-src/3rdparty/zstd/lib/compress/zstdmt_compress.c
mame-chd-src/3rdparty/zstd/lib/compress/fse_compress.c
mame-chd-src/3rdparty/zstd/lib/compress/zstd_compress_literals.c
mame-chd-src/3rdparty/zstd/lib/compress/zstd_compress_sequences.c
mame-chd-src/3rdparty/zstd/lib/compress/zstd_fast.c
mame-chd-src/3rdparty/zstd/lib/compress/zstd_compress.c
mame-chd-src/3rdparty/zstd/lib/compress/zstd_opt.c
mame-chd-src/3rdparty/zstd/lib/compress/zstd_double_fast.c
mame-chd-src/3rdparty/zstd/lib/compress/zstd_ldm.c
mame-chd-src/3rdparty/zstd/lib/compress/hist.c
mame-chd-src/3rdparty/zstd/lib/compress/huf_compress.c
mame-chd-src/3rdparty/flac/src/libFLAC/bitmath.c
mame-chd-src/3rdparty/flac/src/libFLAC/bitreader.c
mame-chd-src/3rdparty/flac/src/libFLAC/bitwriter.c
mame-chd-src/3rdparty/flac/src/libFLAC/cpu.c
mame-chd-src/3rdparty/flac/src/libFLAC/crc.c
mame-chd-src/3rdparty/flac/src/libFLAC/fixed.c
mame-chd-src/3rdparty/flac/src/libFLAC/float.c
mame-chd-src/3rdparty/flac/src/libFLAC/format.c
mame-chd-src/3rdparty/flac/src/libFLAC/lpc.c
mame-chd-src/3rdparty/flac/src/libFLAC/md5.c
mame-chd-src/3rdparty/flac/src/libFLAC/memory.c
mame-chd-src/3rdparty/flac/src/libFLAC/metadata_iterators.c
mame-chd-src/3rdparty/flac/src/libFLAC/metadata_object.c
mame-chd-src/3rdparty/flac/src/libFLAC/stream_decoder.c
mame-chd-src/3rdparty/flac/src/libFLAC/stream_encoder.c
mame-chd-src/3rdparty/flac/src/libFLAC/stream_encoder_framing.c
mame-chd-src/3rdparty/flac/src/libFLAC/window.c
```

There are 83 direct source files on Linux/macOS (84 on Windows). `utf8proc.c`
includes `utf8proc_data.c`; FLAC includes its four deduplication `.c` fragments
from the source tree. The transitive local include scan over all 93 source paths
enumerated by the pinned upstream build script reached 281 files (98 source
files and 183 headers), including platform-specific alternatives. No GPL
license marker or GPL-licensed source appeared in that reachable closure. The
149 headers without an inline license marker are third-party headers covered
by the bundled license for their source directory; MAME headers with explicit
markers are BSD-3-Clause except the public-domain MD5 implementation.

## License dispositions

| Compiled source family | Exact pinned notice | Selected terms |
|---|---|---|
| Candidate C++ shims in `sys/` | Candidate root `LICENSE` | BSD-3-Clause |
| MAME CHD/CD/util sources and OSD string conversion | Per-file headers; `mame-chd-src/docs/legal/BSD-3-Clause` | BSD-3-Clause; `md5.cpp` is public domain |
| LZMA SDK C sources | `mame-chd-src/3rdparty/lzma/DOC/lzma-sdk.txt` and source headers | Public domain |
| zlib C sources | `mame-chd-src/3rdparty/zlib/LICENSE` | zlib license |
| utf8proc C source | `mame-chd-src/3rdparty/utf8proc/LICENSE.md` | MIT |
| Embedded Unicode data | Same file's Unicode data notice | Unicode permissive data grant, preserve notice |
| zstd C sources | Per-file dual-license headers; `mame-chd-src/3rdparty/zstd/LICENSE` | BSD-3-Clause selected; no GPL option used |
| FLAC C sources | `mame-chd-src/3rdparty/flac/COPYING.Xiph` and source headers | Xiph permissive/BSD-style; embedded MD5 is public domain |

MAME's aggregate `COPYING` states the license for MAME as a whole, while these
selected files carry their own permissive grants. The exact source files and
their corresponding notices are included in the pinned archive. The build
recipe checks the source manifest and required notices before the compiler
starts. The archive and generated CHD stay under the build tree; the tool is
not installed or packaged.

## Deterministic build and output settings

- C++20, bundled LZMA/zlib/zstd/FLAC only; no host codec lookup, Cargo build,
  prebuilt static archive, source submodule, or upstream test suite.
- Portable FLAC path (`FLAC__NO_ASM`) and zstd C path (`ZSTD_DISABLE_ASM`).
- One OSD worker (`OSDWORKQUEUEMAXTHREADS=1`) for fixture creation.
- CHD codec slots match MAME's default CD set: `cdlz`, `cdzl`, `cdfl`, `0`;
  19,584-byte hunks and 2,448-byte CD units.
- Hashes and output-size limits are checked by the fixture preflight before a
  guest or long integration run.

## Evidence

- `gh api repos/danifunker/libchdman-rs/commits/7f71bdf3d4099dc38bded49c6745e453005d6e41 --jq .sha` — exit 0, exact public commit.
- `curl --fail --location https://github.com/danifunker/libchdman-rs/archive/7f71bdf3d4099dc38bded49c6745e453005d6e41.tar.gz` followed by `shasum -a 256` — exit 0, archive digest above; the archive has 439 entries.
- `gh api` for the candidate `.gitmodules` and recursive tree — exit 0; `deps/mame` is gitlink `f34f02505e32c1993c6a782b6814232cbfc74e36`, the archive contains no submodule source payload, and the build uses the included `mame-chd-src` subset only.
- `gh api repos/mamedev/mame/commits/f34f02505e32c1993c6a782b6814232cbfc74e36 --jq .sha` — exit 0, exact public MAME base commit.
- `gh api` for pinned `src/lib/util/chd.cpp` plus `git hash-object` of the bundled copy — exit 0; the different blob hashes expose the downstream buffer patch recorded above.
- Static include-closure and source-header scan over the pinned archive — exit 0; 281 reachable files, all 149 headers without inline license markers are in third-party directories with bundled notices, and no GPL marker occurs in the reachable source/header closure.
- No hosted CI or `act` run was relevant to this read-only license gate.
