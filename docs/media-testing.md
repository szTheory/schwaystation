# Disc media testing

`disc.media.preflight` opens the owned CUE through the production host provider and checks its independently
authored two-track geometry, the touching track boundary, pregap/index values and exclusive leadout. It reads the
first and last sector at each track boundary through the same 2352-byte sector and 96-byte subcode callbacks used
by the runner and libretro core, with guard bytes around both buffers. It also rejects an empty path, an empty CUE,
and the exclusive leadout LBA. This is a cheap gate against broken metadata, out-of-range callback reads and
invalid media before the longer guest boot, audio and movie checks; CTest runs it first through
`FIXTURES_SETUP disc_media_preflight` and requires it for each `disc.fixture.*` guest test.

With `SCHWAYSTATION_CHD=ON`, the opt-in `SCHWAYSTATION_BUILD_CHD_TOOL=ON` target also creates a run-local CHD
from those same two tracks. Before compression, CMake checks the exact CUE layout, whole-sector source sizes,
track counts and the tool's codec/hunk/worker settings. It generates the CHD twice, requires byte-identical output
below 524,288 bytes, and writes source, tool and CHD hashes to `disc_boot.chd.manifest`. The preflight checks those
hashes, source counts, CHD size and header before guest work. `disc.media.fixture_controls` proves that changed
hashes, a missing track, wrong sector count, an oversized CHD, a damaged file and a malformed manifest fail. In
CHD-off builds it confirms the fixture and manifest are absent and `.chd` remains unsupported.

The PBP provider is a separate host-side source in `schwaystation_disc_image`. Its build-local producer copies
the owned first MODE2/2352 track into an equivalent single-track CUE, then generates the same unencrypted PSISO
PBP twice and requires byte identity. Raw-DEFLATE blocks are independently decoded with Python's standard
library. The PBP has a 1.5 MiB cap because the documented PSAR data base is at `PSAR + 0x100000`; the separate
CUE/BIN image cap remains 512 KiB. No generated PBP bytes are committed.

`disc.image.pbp_preflight` checks source CUE track count, raw sectors, PBP directory and PSISO metadata, TOC,
block extents, decoded sector totals, source/generator/output hashes and the PBP-only cap before provider or
guest consumers run. `disc.image.pbp_fixture_controls` proves wrong digest, missing input, wrong source/TOC count,
overlap, truncation and over-cap inputs fail for the intended reason. Provider and guest selectors require the
`disc_pbp_preflight` CTest fixture. `disc.image.pbp_headers` checks the production private boundary against
empty and truncated headers, bad magic, encrypted or unknown PSAR markers, and out-of-range or overflowing
section offsets. The shared image dispatcher accepts the fixture-proven unencrypted PSISO data track. On open,
the host checks every indexed block once before returning a reader. It accepts either a verbatim block stored at
exactly 0x9300 bytes or a smaller raw-DEFLATE stream, using one 0x9300-byte decoded block and compressed input
bounded to 0x9300 bytes; total decoded work cannot exceed the 1 GiB logical-media limit. The returned reader
retains only one decoded block as its cache. PBP sectors preserve the source's 2352 raw bytes and zero-fill
unavailable subcode.

`disc.image.pbp_open` checks dispatcher routing, normalized TOC parity and failure-atomic replacement.
`disc.image.pbp_boundaries` exercises the first/final sectors, each compressed-block transition, repeated reads,
exclusive leadout and beyond-leadout output preservation. `disc.image.pbp_parity` compares every source sector
in forward and reverse order, TOC, zero subcode and content identity against `disc_boot_pbp.cue`.
`disc.image.pbp_raw` constructs a one-block PBP with a verbatim block from the owned fixture and verifies sector
bytes through the provider. These selectors require `disc_pbp_preflight`, so fixture identity and extent checks
run before provider reads.

`disc.image.pbp_inflate` exercises the standalone raw-block wrapper with fixed stored and Huffman streams. It
requires terminal success, exact compressed-input consumption and exact output length; malformed, truncated,
trailing and size-mismatched streams leave the caller's output unchanged. The wrapper uses the pinned miniz
decoder with private symbols and adds no system zlib dependency. The selector is checked with CHD both disabled
and enabled.

For a local edit, check the exact inventory before configuring, then build only the affected provider/image tests
and deterministic disc fixtures. Run fixture preflight and the focused PBP selectors before any guest suite:

```sh
LC_ALL=C sort -cu tests/disc/inventory.txt
cmake --preset ci -DSCHWAYSTATION_CHD=ON -DSCHWAYSTATION_BUILD_CHD_TOOL=ON
cmake --build --preset ci --target schwaystation-test-disc-pbp schwaystation-test-disc-image schwaystation-pbp-fixtures
ctest --preset ci --no-tests=error -R '^disc[.]image[.]pbp_|^disc[.]playlist$' --output-on-failure
```

Use `-DSCHWAYSTATION_CHD=OFF` for the dependency-free negative path; configure removes any stale run-local CHD
and registers controls that assert the file is absent and the provider rejects `.chd`.

Keep the directory inventories tab-separated and bytewise sorted. Update `tests/fuzz/inventory.txt` or
`tests/compat/inventory.txt` only when the same change adds or removes a real test in that directory. Repository
checks remain CMake `-P` or POSIX sh and stay within the 3,901-line absolute ceiling; real-host RetroArch test code
stays below 300 lines. Run the existing budget check before adding a permanent repository gate, and retain a new
gate only when it prevents a recurring fault named by a requirement.

The CUE reader accepts a relative `FILE` component of up to 255 bytes. Its Windows boundary test confirms that a
255-byte component passes CUE validation and reaches the file-open step; the full identity/open positive uses the
same fixture on POSIX and macOS, where the build-tree prefix stays within the host path limit.
