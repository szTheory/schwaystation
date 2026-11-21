# PBP Package and Link Evidence

Plan 10-10 validates the existing three component archives on macOS arm64 with
the optional CHD provider both disabled and enabled. All results below are
native package evidence; they do not establish a WASI module size or alter its
bound. The repository-check ceiling remains 3,901 lines; the final measured
total is 3,900.

## Task 1: CHD-OFF package closure

### Cheap preflight

The following checks ran before configuration, builds, or packaging; each
exited 0:

```sh
cmake -DCASE=check -DROOT=. -P tests/cmake/vendored.cmake
cmake -DROOT=. -DMANIFEST=tests/fixtures/manifest.txt -DNOTICES=THIRD-PARTY-NOTICES.md -P tests/cmake/notices.cmake
LC_ALL=C sort -cu tests/disc/inventory.txt
LC_ALL=C sort -cu tests/runner/inventory.txt
LC_ALL=C sort -cu tests/libretro/inventory.txt
cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake
```

The three selected upstream miniz source digests matched their pinned values:

| Source | SHA-256 |
| --- | --- |
| `miniz_tinfl.c` | `2296ebd21ef9af5ebbefd5b454d4bb67d8fb4af2d24945e7fb32114374f75a76` |
| `miniz_tinfl.h` | `da4850920fdf09f8877d9affabe1ebba1852ce4b1ebbd121f2ba7a7ea8e57e5d` |
| `miniz_common.h` | `f1ba29821c8caef83585b328196aa346c5da7c82890a70a3d10d77c6cec0ed39` |

They are the unmodified modular files at upstream commit
`77d0dce8627735138c51770d1799a1ef48f2117d` (v3.1.2). The applicable standalone
inflater notice is MIT; its exact wording is in `THIRD-PARTY-NOTICES.md` under
“Standalone miniz tinfl for PBP.” The notice source SHA-256 is
`239d21a89a87fb6d6322909808f27196b558eb02fb7836f06701b0710626b31d`.

### Configuration, tests, and archives

```sh
cmake --preset ci -DSCHWAYSTATION_CHD=OFF -DSCHWAYSTATION_BUILD_CHD_TOOL=OFF
ctest --preset ci --no-tests=error -R '^libretro[.](vendored|info[.]claims)([.]self_test)?$' --output-on-failure
cmake --build --preset ci --target schwaystation-run schwaystation_libretro
cmake --build --preset ci --target schwaystation-test-libretro-host schwaystation-libretro-card-test schwaystation-test-disc-pbp schwaystation-pbp-fixtures
ctest --preset ci --no-tests=error -R '^disc[.]image[.]pbp_parity$|^runner[.]disc[.]pbp$|^libretro[.]content[.]pbp$|^libretro[.]info[.]claims([.]self_test)?$' --output-on-failure
cpack --config build/ci/CPackConfig.cmake -C Release
ctest --preset ci-release --no-tests=error -R '^release[.](archive_contents|notices)([.]self_test)?$' --output-on-failure
```

Configure, build, CPack, and each final test command exited 0. Vendored and
metadata selectors passed 4/4. The focused PBP selector passed 7/7 (including
its fixture and compatibility preflights) in 6.07 seconds. The three produced
component archives passed all four existing archive and notice checks in 0.12
seconds. No package was omitted and no assertion or case was relaxed.

| Component archive | SHA-256 |
| --- | --- |
| `schwaystation-library-macos-arm64.zip` | `c603dc17b3b3cae5dbf27d964303d199b74418398916689616247663c5bbc8af` |
| `schwaystation-runner-macos-arm64.zip` | `b26d85e3a9893aed126569c920b9252f5f3e1cb4102da92a99f86a98027ae6fa` |
| `schwaystation-libretro-macos-arm64.zip` | `3ad2267fd9812ce7c6a04444a1cd6b5e4a3114af140852ea01ce925c09c6a38e` |

Each archive contains `THIRD-PARTY-NOTICES.md`; its extracted SHA-256 is the
same `239d21a8…6b31d` as the source. The existing `release.archive_contents`
check confirms packaged bytes match the installed source file, and
`release.notices` accepts the admitted closure. The generated libretro `.info`
claims `CC BY-SA 4.0, ISC, MIT`, lists
`exe|psexe|cue|iso|m3u|pbp`, retains `is_experimental = "true"`, and describes
only supported unencrypted single-data-track PSISO PBP images.

### Link inspection

`ninja -C build/ci -t commands schwaystation_libretro` shows the PBP inflater
as a separate `libschwaystation_pbp_miniz.a` input with
`MINIZ_NO_MALLOC`, `MINIZ_NO_STDIO`, and `MINIZ_NO_TIME`; the linker command
has no `-lz`. `nm -g build/ci/libschwaystation_pbp_miniz.a` reports only the
private `_schway__pbp_tinfl_*` and `_schway__pbp_inflate_raw` exports. No
unprefixed standalone `tinfl_*` export was found. `otool -L` on the built
libretro bundle lists only `/usr/lib/libSystem.B.dylib`, with no runtime zlib.
The actual CHD-OFF arm64 Mach-O bundle is 359,952 bytes.

The initial archive selector run used `ctest --preset ci` and exited 8 because
that preset excludes the `release` label. The existing `ci-release` preset was
then used and all four archive/notice cases passed; the test set was preserved.
The initial CHD-OFF focused host test also exposed a stale host/card helper
built under the previous CHD-ON configuration (`chd_enabled 0 != 1`). The
configuration-sensitive helpers and fixture targets were rebuilt for OFF;
the final 7/7 focused run passed.
