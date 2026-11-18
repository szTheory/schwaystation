# Technology Stack

**Project:** schwaystation v2.0, “The whole library”  
**Researched:** 2026-11-18  
**Scope:** v2 additions only. The v1 C17 core, CMake/CTest build, libretro ABI, host-supplied sector boundary, and cross-platform determinism contract remain as recorded in the v1 stack.

## Recommendation

Keep new disc and peripheral behavior in small C modules behind the existing disc and device boundaries. Add one optional upstream library, libchdr, for CHD decoding, built only when `SCHWAYSTATION_CHD=ON`. PBP needs DEFLATE, so compile a small MIT-licensed miniz inflater module independently of that option; this is vendored C source, not a system library or package-manager dependency. ECM, CCD, SBI/LibCrypt, card wrappers, patches, cheats, and controller protocols need no additional library.

The core itself still receives sectors and inputs from its host. Put file and CHD callbacks in the host-side disc-image helper. Keep the CHD option off by default so library consumers who do not need it retain the v1 footprint and link surface. Advertise `.chd` from the libretro `.info` only in CHD-enabled builds.

## Recommended v2 Stack

### Optional media and compatibility code

| Technology | Version / license | Purpose | Recommendation and trade-off |
|---|---|---|---|
| libchdr | v0.3.0; BSD-3-Clause; project-approved source pin `47d794151b2b` | CHD v1–v5 headers, metadata, hunk decoding and CD codecs | Build as a static host-side backend only when `SCHWAYSTATION_CHD=ON`. The official CMake defaults keep the backend C-only, bundle LZMA/miniz/zstd, use the dr_flac C backend, and leave system zlib/zstd off. Keep raw-sector and subcode output plus block CRC checking enabled. `CHDR_LOWRAM_TARGET` is the upstream option to measure for WASM/handheld memory pressure. Do not use the optional C++ micro-flac backend, which fetches another project at configure time. |
| miniz inflater | 3.1.2; MIT | PBP PSISO DEFLATE blocks; also a transitive libchdr codec | Build the needed inflater code from the checked-in miniz source independently of CHD, then share one CMake target when CHD is enabled. This preserves PBP support in CHD-off builds without depending on the host's zlib package. Retain the MIT notice. The source is vendored runtime code, but it adds no separate dynamic library, system package, or ABI. |
| In-tree C parsers | Project MIT | ECM sector reconstruction; CCD/IMG/SUB; SBI bad-Q list; PBP TOC and PSISO records; `.gme`/`.mem`/`.psx` card headers | Use bounded parsers from the format descriptions and test each with its own fuzz/replay target. ECM has no sector index, so load-time scanning or normalization is required for random sector access. Scope PBP to supported unencrypted PSISO content; PGD-encrypted payloads and undocumented audio or multi-disc variants need separate evidence. |
| In-tree device and patch code | Project MIT | Multitap, guns, mouse, neGcon, Jogcon, PocketStation storage, PPF/BPS, GameShark | Implement protocols and byte transformations at SIO0, runner overlay, and libretro cheat boundaries. Do not add an input-device, patching, or memory-card library. PPF offsets and patch validation belong in the runner overlay; GameShark strings belong in the libretro adapter. |

### CHD license and build boundary

At the approved libchdr pin, `CMakeLists.txt` declares a C-only v0.3.0 build. Its default dependencies are LZMA SDK 26.02 (public domain), miniz 3.1.2 (MIT), zstd 1.5.7 (dual BSD-3/GPL-2; select the BSD-3 terms), and dr_flac (public domain or MIT-0; select the public-domain grant). `WITH_SYSTEM_ZLIB` and `WITH_SYSTEM_ZSTD` default off. `chd.h` contains a separate MAME-derived BSD-3 notice in addition to libchdr's root BSD-3 license. The callback API lets the host supply file-size/read/seek/close operations, so the core need not open a path or own a file handle.

For CHD-enabled release archives, carry the libchdr BSD-3 text, the separate `chd.h` BSD-3 attribution, miniz's MIT text, and Zstandard's BSD-3 text. The bundled Zstandard source is dual-licensed and its own header records both choices; the vendored directory at the approved pin has no license file, so copy the BSD-3 text from the pinned upstream Zstandard release into the notices. Preserve the LZMA and dr_flac license texts with their source; their chosen public-domain grants do not add a binary notice requirement. Do not select the GPL-2 Zstandard option or fetch micro-flac.

If miniz is compiled for PBP in all builds, include its MIT notice in every archive, including CHD-off archives; CHD-on must not duplicate the same notice. This resolves the old stack note that treated the bundled miniz/Zstandard notices as unverified. Update `PROVENANCE.md` and `THIRD-PARTY-NOTICES.md` when the exact vendored files are admitted, not by assuming the root libchdr license covers its dependencies.

The CHD wrapper must enforce the existing hostile-input limits before accepting metadata or allocating buffers, check every libchdr result, and return sectors through the host callback. Cross-platform sector/hash equality is the gate for enabling the option in a build. Keep `SCHWAYSTATION_CHD` default OFF so CHD stays an explicit build choice.

### Parser and module choices

| Feature | Stack choice | Why no additional library |
|---|---|---|
| PBP | In-tree PSISO directory/TOC parser plus the shared miniz inflater | The documented PSISO payload uses DEFLATE blocks. A system zlib would add platform-dependent link requirements; a tiny vendored inflater keeps the feature available when CHD is disabled. PGD-encrypted payloads remain unsupported. |
| ECM | Bounded in-tree decoder in the disc-image helper | The format reconstructs sector sync, EDC and ECC and has no random-access index. A small scan/index path is more suitable than a second archive package. |
| CCD/IMG/SUB | Bounded text parser and sector/subchannel reader in the disc-image helper | CCD is an INI-like descriptor; IMG and SUB carry sectors and subchannel bytes. No codec package is required. |
| SBI/LibCrypt | In-tree SBI parser and bad-Q callback | SBI records are bounded per-sector data consumed by the existing CD path; the emulator need not embed a separate protection database. |
| Card import/export | In-tree wrapper detection and raw 128 KiB card span | `.gme`, `.mem`, and `.psx` add headers around the existing raw card bytes. Keep import/export outside file I/O in the core. |
| Peripherals | Existing SIO0/device model, with host inputs expressed through the API | These are emulated byte protocols and storage behavior. The product still contains no host input-device code. |
| PPF/BPS and GameShark | In-tree runner parser/sector overlay and libretro cheat parser | Their inputs are bounded records or strings; no patch library is needed. Keep patch application outside the deterministic core. |

## Native Build, Test, and CI Tools

These are development, fixture-generation, or CI tools. They are not linked into the public library and are not shipped in release archives. Keep the existing `cmake --workflow --preset <lane>` entry point and do not add a language runtime or package manager for SEED-007.

| Tool | Current upstream snapshot (2026-11-18) | Project use and recommendation |
|---|---|---|
| CMake / CTest | Existing project baseline CMake 3.25+ | Use `cmake -P` plus built-in `string(JSON)` for `quality.json`; `string(JSON)` predates the project's 3.25 minimum. Keep inventory and ratchet checks as CMake scripts, with parser/tooling faults treated as test bugs. |
| POSIX shell and GitHub CLI | Existing toolchain; `gh api` supports `--jq` | Use shell for the weekly digest and CI-efficiency report, with GitHub Actions API data and `gh api --jq`. Do not add Python, Node, a standalone jq dependency, database, hosted service, or AI summarizer. |
| LLVM clang-format / clang-tidy | LLVM 23.1.2 is the latest official release; existing project pin is 21 | Tool-only, Apache-2.0 with LLVM exceptions. Keep the exact project pin during the tune-up unless the report finds a concrete maintenance issue; a formatter major bump can cause broad, low-value churn. |
| wasi-sdk | `wasi-sdk-34` is the latest official release and matches the current project pin | WASM compile/test tool only; upstream SDK is Apache-2.0 and carries its toolchain component notices. No SDK code is linked into the deliverables. |
| Wasmtime | v50.0.0 is the latest stable official release; the existing project pin is 48.0.7 | WASI execution/hash tool only, Apache-2.0. Keep the pin stable through the tune-up unless a controlled comparison passes and `quality.json` is remeasured; instruction-count readings are tool-version-sensitive. |
| Valgrind / Cachegrind | 3.28.0 is the current upstream release; existing project pin is 3.26.0 | Performance-measurement tool only. Valgrind is GPL-3 since 3.26; this is not a source or runtime dependency and must not be included in deliverables. Keep the measured pin until an upgrade is shown to preserve the Ir/frame signal. |
| armips | v0.11.0 is the latest release; MIT | Build-time fixture assembler only. Re-audit its exact source pin in the tune-up; do not make assembler binaries or libraries runtime dependencies. |
| mkpsxiso | v2.30 is the latest release; GPL-2 | Existing fixture-generation tool only under the settled tool-only exception. Do not vendor, link, package, or add another GPL tool. Generated owned fixtures do not inherit the tool's license. |

SEED-007 should use the weekly digest, measured CI report, and `quality.json` rather than adding a new CI service. Measure caching against compile share and critical path first; do not add ccache or a cache action as an assumption. The `policy.lessons_unique` check belongs in the existing CMake/POSIX repository-check budget and should name its prevented fault.

## Alternatives Considered

| Category | Recommended | Alternative | Why not |
|---|---|---|---|
| CHD | Optional static libchdr, default OFF | Reimplement CHD codecs or use the system's zlib/zstd/FLAC/LZMA packages | CHD v5 supports several codec families and metadata; a custom decoder is a large correctness/security burden. System codec discovery makes builds and archive behavior platform-dependent. |
| PBP compression | Vendored miniz inflater source, built independently | Depend on system zlib or make PBP require CHD | System zlib adds a separate install/link dependency. Gating PBP behind CHD would make unrelated format support depend on an optional feature. |
| Other formats and patches | Small in-tree C parsers and transforms | Add media/patch packages | The supported subset is bounded and already described by the preparation references; an extra package adds more licenses and portability work than it removes. |
| Tune-up reporting | CMake `string(JSON)`, POSIX shell, GitHub CLI | Python/Node report framework, external dashboard, cache service | The project already has the required parsers, runner API and CI data. New dependencies do not improve the measurements SEED-007 requires. |
| Tool upgrades | Keep exact pins until a measured comparison supports a bump | Float latest tool versions or mass-upgrade every major | Floating versions make hashes, Cachegrind figures, and output formats harder to reproduce; a tune-up should update a pin only with evidence and refreshed baselines. |

## Installation and Build Guidance

No new package-manager install command is required. Build the default format set with the existing CMake workflow. When the CHD option is enabled, compile the pinned libchdr source and its bundled C dependencies into the host-side media helper; keep the core API unchanged. Build the PBP inflater separately so PBP remains available when CHD is off. Link only the C memory functions into the core and retain all applicable upstream notices in the generated archives.

## Sources

- [libchdr v0.3.0 latest release](https://github.com/rtissera/libchdr/releases/tag/v0.3.0); [project-approved source pin and tree](https://github.com/rtissera/libchdr/tree/47d794151b2b64f6aba60304e6480f4f5a2bc24b); [CMake options and dependency wiring at that pin](https://github.com/rtissera/libchdr/blob/47d794151b2b64f6aba60304e6480f4f5a2bc24b/CMakeLists.txt); [libchdr BSD-3 license](https://github.com/rtissera/libchdr/blob/47d794151b2b64f6aba60304e6480f4f5a2bc24b/LICENSE.txt); [MAME-derived `chd.h` BSD-3 header](https://github.com/rtissera/libchdr/blob/47d794151b2b64f6aba60304e6480f4f5a2bc24b/include/libchdr/chd.h); [LZMA SDK license](https://github.com/rtissera/libchdr/blob/47d794151b2b64f6aba60304e6480f4f5a2bc24b/deps/lzma-26.02/LICENSE); [miniz source/license header](https://github.com/rtissera/libchdr/blob/47d794151b2b64f6aba60304e6480f4f5a2bc24b/deps/miniz-3.1.2/miniz.c); [Zstandard source license header](https://github.com/rtissera/libchdr/blob/47d794151b2b64f6aba60304e6480f4f5a2bc24b/deps/zstd-1.5.7/zstddeclib.c); [Zstandard 1.5.7 BSD-3 license](https://github.com/facebook/zstd/blob/v1.5.7/LICENSE); [dr_flac license alternatives](https://github.com/rtissera/libchdr/blob/47d794151b2b64f6aba60304e6480f4f5a2bc24b/include/dr_libs/dr_flac.h).
- [psx-spx CD-ROM file-format reference](https://github.com/psx-spx/psx-spx.github.io/blob/6d7d1bc106a7/docs/cdromfileformats.md); [CD-ROM formats and LibCrypt reference](https://github.com/psx-spx/psx-spx.github.io/blob/6d7d1bc106a7/docs/cdromformat.md); [controller and memory-card reference](https://github.com/psx-spx/psx-spx.github.io/blob/6d7d1bc106a7/docs/controllersandmemorycards.md); [PPF3 format description](https://github.com/meunierd/ppf/blob/4f24f8c784fa/ppfdev/PPF3.txt). Format links are used as facts only; no code is copied from them.
- [CMake `string(JSON)` documentation](https://cmake.org/cmake/help/v3.25/command/string.html#json); [GitHub CLI API command](https://cli.github.com/manual/gh_api); [LLVM releases](https://releases.llvm.org/) and [LLVM 23.1.2 license](https://github.com/llvm/llvm-project/blob/llvmorg-23.1.2/LICENSE.TXT); [wasi-sdk releases](https://github.com/WebAssembly/wasi-sdk/releases) and [wasi-sdk 34 license](https://github.com/WebAssembly/wasi-sdk/blob/wasi-sdk-34/LICENSE); [Wasmtime v50.0.0 release](https://github.com/bytecodealliance/wasmtime/releases/tag/v50.0.0) and [Wasmtime license](https://github.com/bytecodealliance/wasmtime/blob/v50.0.0/LICENSE); [Valgrind current releases](https://valgrind.org/downloads/) and [upstream release notes](https://valgrind.org/docs/manual/dist.news.html).
- [armips v0.11.0 release](https://github.com/Kingcom/armips/releases/tag/v0.11.0); [armips MIT license](https://github.com/Kingcom/armips/blob/v0.11.0/LICENSE.txt); [mkpsxiso v2.30 release](https://github.com/Lameguy64/mkpsxiso/releases/tag/v2.30); [mkpsxiso GPL-2 license](https://github.com/Lameguy64/mkpsxiso/blob/v2.30/LICENSE.md).
- Project decisions and boundaries: `.planning/preparation/DECISIONS.md` DEC.43, DEC.55, DEC.86 and DEC.94; `.planning/preparation/ENGINEERING.md` sections 4, 7, 8, 11 and 12; `.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md` sections 2.7 and 3; `.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md` sections 3, 6 and 7; `.planning/seeds/SEED-001-whole-library.md`; `.planning/seeds/SEED-007-milestone-tune-up.md`.
