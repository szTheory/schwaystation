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

The PBP provider is a separate host-side source in `schwaystation_disc_image`. `disc.image.pbp_headers` checks
the production private boundary against empty and truncated headers, bad magic, encrypted or unknown PSAR markers,
and out-of-range or overflowing section offsets. This slice does not yet accept PSISO sectors, so runner and
libretro metadata do not advertise PBP.

For a local edit, check the exact inventory before configuring, then build only the provider test and deterministic
disc fixtures and run the preflight selector:

```sh
LC_ALL=C sort -cu tests/disc/inventory.txt
cmake --preset ci -DSCHWAYSTATION_CHD=ON -DSCHWAYSTATION_BUILD_CHD_TOOL=ON
cmake --build --preset ci --target schwaystation-test-disc-media schwaystation-chd-fixtures
ctest --preset ci --no-tests=error -R '^disc[.]media[.](preflight|fixture_controls)$' --output-on-failure
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
