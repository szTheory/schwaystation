# Asset policy

Which bytes may be in this repository, its history, its CI artifacts and its release archives: BIOS images, disc
images, PlayStation executables, memory cards, saves, states, screenshots and test fixtures. Code provenance is in
[PROVENANCE.md](PROVENANCE.md); this file covers data. Written on 2026-10-19, before any such file exists.
`[AP.NN]` IDs resolve in section 9.

| Question | Section |
|---|---|
| Can this file be committed? How is it listed? | 1, 2 |
| What is built or fetched in CI instead of committed? | 3 |
| How do I test with my own BIOS and discs? | 4 |
| Which BIOS digests does the loader accept? | 5 |
| What enforces this? | 6 |
| Size caps | 7 |

## 1. Never
- Bytes of a commercial disc in any amount or form: raw or cooked sectors, cue sheets naming a commercial dump,
  CHD or PBP containers, ECM files, the boot executable, extracted textures, audio, video, text or save data.
- Firmware: every retail PlayStation BIOS (SCPH-1000 through SCPH-102, the DTL and Net Yaroze units, the PSP and
  PS3 `psxonpsp660.bin` and `ps1_rom.bin` images), the CD-ROM sub-CPU firmware, and any bytes retyped into source
  code or hex. This includes copies inside permissively licensed repositories.
- Sony licence material: PsyQ's `licensea.dat`, `licensee.dat`, `licensej.dat`; the "Licensed by Sony Computer
  Entertainment" system-area text; the PlayStation logo data of sectors 5 to 11; the `SCEx` wobble strings as
  data. The `header.inc` region marker string at offset 4Ch of a PS-EXE is the documented loader text and is
  allowed; the fixture disc carries no licence file and tests GetID's unlicensed path.
- Memory cards, save states, movies, screenshots, recordings, traces, TTY logs or RAM dumps made while running a
  commercial disc or a retail BIOS.
- Bulk tables from databases that state no licence or a share-alike one, beyond what section 2 names: no Redump
  DAT, no DuckStation `gamedb.yaml` rows, no No-Intro BIOS DAT. The libretro `System.dat` digests (section 5) are
  facts; ledger titles from the CC-BY-SA-4.0 libretro-database DAT enter one row at a time with the notice.
- Sony logos, boot animations and artwork; for example a frame dump of the BIOS splash or the "PS" logo.
- Test programs or data whose authors state no licence, or a copyleft or non-commercial one (section 3): amidog's
  psxtest zips, PeterLemon/PSX, simias/psx-hardware-tests, ps1-tests executables (MPL-2.0 `libpsn00b` bytes).

Owning the console or the disc changes nothing on this list. A player's own dumps stay on the player's machine.

## 2. Allowed, when listed in `tests/fixtures/manifest.txt`
- Our own builds: PS-EXE fixtures assembled from this repository's MIT armips sources under
  `tests/fixtures/src/`, which link no runtime, so every byte in the image is ours (a sibling learned that an SDK
  runtime, a font or a compiler library can carry its own licence into a built image [AP.07]); the own stub
  kernel; the own mkpsxiso disc images built from a committed XML with generated audio (the images themselves are
  built in CI, section 3, and only their digests are committed); fuzz seeds and generated regressions; own 8x8
  glyphs.
- The SingleStepTests/r3000 sample: the first 100 cases of each of the 55 files, transcoded into
  `tests/vectors/r3000/sst-sample.bin` with the MIT notice [AP.04].
- The ps1-tests GTE table transcoded from MIT C source into `tests/vectors/gte/ps1tests-1150.bin` with the MIT
  notice [AP.05].
- Hardware reference images and logs from ps1-tests under MIT: the `vram.png` captures, `psx.log` hardware
  outputs and the `mdec/*.png` references, listed with their notice [AP.05].
- nugget `RESULTS.md` numbers and ps1-tests log numbers restated in `tests/diag/expected.csv` with source and
  citation columns (facts; the files themselves are fetched, not copied).
- Single facts about a disc or title (serial, region, a hash, a track SHA-1 already public in Redump), and images
  or recordings made from committed fixtures or from the core's own test card.

The manifest has one line per file, with tab-separated fields; `#` starts a comment. It uses nesturbator's format
plus a `notice` field [AP.06]: `path source pin licence sha256 notice`. The fields are: path from the repository
root; source = upstream URL or `this-repo`; pin = 40-hex commit, or release tag plus asset name; licence = SPDX
expression (`NOASSERTION` when unstated, which keeps the row out of section 2); sha256 = 64 lowercase hex; notice
= the `THIRD-PARTY-NOTICES.md` section name or `-`. The directory is `tests/fixtures/` everywhere: CONFORMANCE 2
and 7, the committed `scripts/hygiene.sh` and `.gitignore` all name it, and the manifest is `tests/fixtures/manifest.txt` [AP.08].
- **Recommendation:** the `notice` field. **Strongest alternative:** nesturbator's five fields. **Would change
  if:** no committed fixture ever needs a notice.

## 3. Built or fetched in CI, digests committed, never committed, never an artifact
A file whose licence is unstated, mixed or restricts redistribution, or whose size exceeds the caps, is built or
fetched in CI at pinned commits. Only its digests are committed: `tests/built/expected.txt` holds the name, tool
and source pins, image SHA-256, and frame, audio and mailbox hashes. Tools are pinned by the SHA-256 of their
archives, and every lane must produce byte-identical files (`hash-equality`). Built and fetched files stay in the
build directory; they never reach a release archive or an Actions artifact (the Actions cache carries OpenBIOS
between legs of one run only).

| Fixture | Built or fetched from (pin) | Why not committed |
|---|---|---|
| OpenBIOS `openbios.bin` | built from pcsx-redux/nugget `9db6d88d23c4` with the pinned mipsel GCC on the Linux leg [AP.03] | 512 KiB binary (section 6 treats every 524,288-byte binary as a BIOS); third-party code is built, not stored |
| nugget tests (about 60 PS-EXEs) | built from the same pin | MIT plus libcester and libgcc bytes; dozens of binaries; graded by their own success lines |
| ps1-tests executables `tests.zip` | fetched at release `build-158` (6,613,816 bytes), digest recorded at first fetch [AP.05] | MPL-2.0 `libpsn00b` bytes; running needs no notice, redistributing would |
| SingleStepTests/r3000 full set | fetched nightly at `51e227447d0b` (23,946,132 bytes) [AP.04] | size; the committed sample covers the per-PR tier |
| Own mkpsxiso disc images (`disc_boot.bin/.cue` and the perf disc) | built from the committed XML, PS-EXE and generated audio with mkpsxiso v2.30 (tag `b6f9a25c8a96`, commit `54fb1644ed87`) | a disc image is hundreds of KB of fill sectors; rebuilt and hash-compared on every lane |
| amidog psxtest_cpu, psxtest_cpx, psxtest_gte | fetched nightly at most, binary oracle; results read from VRAM text | no stated terms; CC BY-NC-SA wiki |
| PeterLemon/PSX | fetched locally, binary oracle against its own hardware PNGs | no licence file |
| PCSX-Redux dev AppImage (oracle) | fetched by build id and SHA-256 in the nightly oracle lane | GPL-2 binary used only as a released program; its source is never opened |

- **Recommendation:** fetch at pins, commit digests. **Strongest alternative:** commit permissively licensed
  binaries outright. **Would change if:** an author grants a permissive licence on request and the file fits the
  caps; then it moves to section 2 with the grant cited.

## 4. Local only: your own BIOS and discs
- `.env.local` (git-ignored; template `.env.example`) sets `SCHWAYSTATION_BIOS_DIR` (your BIOS dumps, recognised
  by digest) and `SCHWAYSTATION_DISC_DIR` (your disc images). Only the local presets' CMake drivers read it; the
  core and runner read no environment. The local `library` preset is never registered in the `ci` inventory and
  configure fails if a directory is missing (no self-skip) [AP.09].
- What leaves the machine from a `library` run: the Redump serial, the rcheevos MD5, the Redump track SHA-1, the
  DAT title, the predicate result and the date. What never leaves: file names, paths, any hash of an unlisted
  file, frames, RAM, saves, TTY, BIOS digests other than the public ones in section 5. The BIOS directory is never
  listed in a transcript, summary or issue.
- Oracles that need a retail BIOS (DuckStation regtest, Mednafen, ares, MAME) run only in this preset.
- Issue forms say: never attach disc images, BIOS files or memory-card dumps of commercial saves.

## 5. BIOS digest table (the loader accepts any listed digest, whatever the file name)
Firmware is identified by digest. The digests are facts and may be committed; the bytes may not. Names, sizes,
MD5 and SHA-1 are the libretro `System.dat` rows at `12c21cf79f1e`, read raw on 2026-10-19 (tested) [AP.02];
every image is 524,288 bytes. The libretro `.info` lists `scph5500.bin`, `scph5501.bin` and `scph5502.bin` as
required and `psxonpsp660.bin` and `ps1_rom.bin` as optional; the loader first tries the region's canonical
name, then any 512 KiB file in the directory whose digest is below, logs the match, and refuses an unknown image
by name instead of guessing. The runner's `--bios` prints the digest it matched.

| libretro name | Version, model (psx-spx) | Region | MD5 | SHA-1 |
|---|---|---|---|---|
| `scph1000.bin` | 1.0j SCPH-1000, DTL-H1000 | J | 239665b1a3dade1b5a52c06338011044 | 343883a7b555646da8cee54aadd2795b6e7dd070 |
| `scph3000.bin` | 1.1j SCPH-3000 | J | 849515939161e62f6b866f6853006780 | b06f4a861f74270be819aa2a07db8d0563a7cc4e |
| `scph1002.bin` | 2.0e SCPH-1002 | E | 54847e693405ffeb0359c6287434cbef | 20b98f3d80f11cbf5a7bfd0779b0e63760ecc62c |
| `scph3500.bin` | 2.1j SCPH-3500 | J | cba733ceeff5aef5c32254f1d617fa62 | e38466a4ba8005fba7e9e3c7b9efeba7205bee3f |
| `scph5000.bin` | 2.2j SCPH-5000 (libretro's dump, CRC 8C93A399; psx-spx lists 24FC7E17, so this row is a second variant until a dump is hashed) | J | eb201d2d98251a598af467d4347bb62f | e340db2696274dda5fdc25e434a914db71e8b02b |
| `scph1001.bin` | 2.2a SCPH-1001 | U | 924e392ed05558ffdb115408c263dccf | 10155d8d6e6e832d6ea66db9bc098321fb5e8ebf |
| `scph5500.bin` | 3.0j SCPH-5500 | J | 8dd7d5296a650fac7319bce665a6a53c | b05def971d8ec59f346f2d9ac21fb742e3eb6917 |
| `scph5501.bin`, `scph7003.bin` | 3.0a SCPH-5501, SCPH-7003 (same image) | U | 490f666e1afb15b7362b406ed1cea246 | 0555c6fae8906f3f09baf5988f00e55f88e9f30b |
| `scph5502.bin`, `scph5552.bin` | 3.0e SCPH-5502, SCPH-5552 (same image) | E | 32736f17079d0b2b7024407c39bd3050 | f6bc2d1f5eb6593de7d089c425ac681d6fffd3f0 |
| `scph7001.bin` | 4.1a SCPH-7001, 7501, 7503, 9001 | U | 1e68c231d0896b7eadcad1d7d8e76129 | 14df4f6c1e367ce097c11deae21566b4fe5647a9 |
| `scph7002.bin`, `scph7502.bin`, `scph9002(7502).bin` | 4.1e SCPH-7002, 7502, 9002 (same image) | E | b9d9a0286c33dc6b7237bb13cd46fdee | 8d5de56a79954f29e9006929ba3fed9b6a418c1d |
| `scph100.bin` | 4.3j SCPH-100 | J | 8abc1b549a4a80954addc48ef02c4521 | 339a48f4fcf63e10b5b867b8c93cfd40945faf6c |
| `scph101.bin` | 4.5a SCPH-101 | U | 6e3735ff4c7dc899ee98981385f6f3d0 | dcffe16bd90a723499ad46c641424981338d8378 |
| `scph102A.bin` | 4.4e SCPH-102 | E | b10f5e0e3d9eb60e5159690680b1e774 | beb0ac693c0dc26daf5665b3314db81480fa5c7c |
| `scph102B.bin`, `scph102C.bin` | 4.5e SCPH-102 (same image) | E | de93caec13d1a141a40a79f5c86168d6 | dbc7339e5d85827c095764fc077b41f78fd2ecae |
| `psxonpsp660.bin` | PSP firmware 6.60 PS1 image, region-free | any | c53ca5908936d412331790f4426c6c33 | 96880d1ca92a016ff054be5159bb06fe03cb4e14 |
| `ps1_rom.bin` | PS3 PS1 image, region-free | any | 81bbe60ba7a3d1cea1d48c14cbcc647b | c40146361eb8cf670b19fdc9759190257803cab7 |

Not in `System.dat`, so not accepted by the loader until a public digest exists: 2.0a DTL-H1001, 2.1a/2.1e,
2.2e SCPH-1002 and the DTL units, 2.2v SCPH-5903 (1 MB), 2.2d DTL-H1100, 4.0j SCPH-7000/9000, 4.1w SCPH-7000W,
4.4a SCPH-101, and the Net Yaroze DTL-H3000 family (psx-spx CRC32 values in PSX-SYSTEM 7). No SHA-256 is
recorded upstream; the first maintainer dump that matches both MD5 and SHA-1 records one here. The canonical
name per region is `scph5500.bin` (J), `scph5501.bin` (U), `scph5502.bin` (E). OpenBIOS is not in this table:
it is third-party MIT code identified by the digest in `tests/built/expected.txt`, never a Sony image.

## 6. How this is checked
- `scripts/hygiene.sh` runs at pre-commit (`--staged`), at pre-push over all outgoing history (`--history`), and
  in CI (`--tree`). Unless `tests/fixtures/manifest.txt` lists the file, it rejects: disc, executable, BIOS,
  card and save extensions by name (`.bin .cue .iso .img .chd .pbp .ecm .mcr .mcd .gme .exe .psexe .psx .sbi .m3u
  .ccd .sub .mds .mdf .sav .state`, as in `.gitignore`); the 8-byte `PS-X EXE` signature at offset 0; the 5-byte
  `CD001` identifier at 0x8001 (2048-byte sectors) or 0x9319 (2352-byte raw sectors); a 131,072-byte file
  starting with the 2-byte `MC` frame header; any binary of exactly 524,288 bytes (a BIOS by size, with no byte
  of a BIOS compared, not even as hex); and any file containing a NUL byte. Every signature is compared as hex so
  the script never carries the signature itself (XFER-076) [AP.08]. It also rejects home-directory paths, e-mail
  addresses, GPL text in code, a non-interactive GSD config and commits on `main` once a remote exists.
- `policy.rom_hash_coverage` ties every manifest row to a tracked file and every tracked binary to a row;
  `policy.fixture_budget` enforces section 7; `policy.no_skip` keeps the fixture lanes from skipping themselves.
- `release.firmware_scan` (v1.0 phase 1) hashes every member of every release zip and every tracked binary (MD5,
  SHA-1, SHA-256) and fails on any section 5 digest, any 524,288-byte member, any `PS-X EXE` or `CD001` signature,
  and any 32-byte window of the Sony licence string compared as hex. A synthetic PS-EXE is its negative control.
- `hygiene.no_bios_bytes` scans every tracked file for the BIOS signatures as hex; `local.firmware_window_scan`
  (local only, planned) fails if any 32-byte window of a dump in `SCHWAYSTATION_BIOS_DIR`, with at least 8
  distinct byte values, appears in a built core, runner or libretro module.
- Fetched and built programs are admitted by digest only: the first fetch records the SHA-256 of the archive and
  of each file inside; a later fetch that differs is an `infrastructure_error`, and a pin bump is a PR
  (CONFORMANCE 7).

## 7. Size caps
| What | Cap | Check |
|---|---|---|
| One committed PS-EXE fixture source (`.asm`, `.inc`, `.xml`, glyph data) | 64 KiB | `policy.fixture_budget` |
| One built PS-EXE | 64 KiB of authored content (plus the 2 KiB header) | `policy.fixture_budget` |
| One built disc image | 512 KiB, build directory only, never committed | `policy.fixture_budget` |
| `tests/fixtures/**` in all | 4 MiB | `policy.fixture_budget` |
| One committed vector file (`tests/vectors/**`) | 4 MB (the r3000 sample is about 2.3 MB, the GTE table about 594 KB, both (derived)) | `policy.fixture_budget` |
| `tests/fuzz/**` (minimised regressions only) | 1 MiB; each seed at most 4 KB | `policy.fixture_budget` |
| CHD | never a committed fixture; libchdr only | `policy.fixture_budget` |
| Release archive members | no built or fetched program, no fixture image | `release.archive_contents`, `release.firmware_scan` |

## 8. Names, and if something gets in
"Sony", "PlayStation", "PS1", "PSX", "PS one", "DualShock" and the SCPH model numbers are used only to say what
the core emulates. The project is not affiliated with or endorsed by Sony Interactive Entertainment.
If a forbidden file gets in: remove it and rewrite the branch before pushing; the push hook scans every commit
about to leave. If it was already pushed, rewrite the history, force-push, and ask GitHub support to drop cached
copies. A fixture whose licence turns out wrong moves to section 3 in the same PR, with its digest.

## 9. Sources
| ID | Tier | Source (URL) | Pin or access date | Supports |
|---|---|---|---|---|
| AP.01 | T1 | github.com/szTheory/cogmaster `ASSET_POLICY.md`; github.com/szTheory/muhtari `ASSET_POLICY.md` | `abd950941e93`; `d698364b180d` | template and section structure |
| AP.02 | T1 | github.com/libretro/libretro-database `dat/System.dat` L498 to L519, fetched raw with `gh api` and read here (tested); `LICENSE` CC BY-SA 4.0 (digests are facts) | `12c21cf79f1e` | section 5 names, sizes, MD5, SHA-1 |
| AP.03 | T1 | github.com/pcsx-redux/nugget `LICENSE` (MIT), `openbios/README.md`, `openbios/Makefile`, `.github/workflows/build.yml` | `9db6d88d23c4` | OpenBIOS build-in-CI disposition |
| AP.04 | T1 | github.com/SingleStepTests/r3000 `LICENSE` (MIT), README, `transcode_json.py`, `v1/` tree sizes | `51e227447d0b` | sample and full-set dispositions, sizes |
| AP.05 | T1 | github.com/JaCzekanski/ps1-tests `LICENSE` (MIT), README, `common.mk` (libpsn00b link), `gte/test-all/tests.c`, release `build-158` asset | `f727802fead1`; `build-158` | reference images and logs, GTE table, MPL bytes in executables |
| AP.06 | T1 | github.com/szTheory/nesturbator `ASSET_POLICY.md`, `tests/roms/manifest.txt`, `scripts/hygiene.sh` | `9f9386c661f6` | manifest format |
| AP.07 | T1 | `.planning/LESSONS.md` XFER-019 (an unpublished Genesis sibling's phase 1) | this repository | runtime bytes carry licences |
| AP.08 | T1 | `scripts/hygiene.sh`, `.gitignore`, `.env.example` in this repository (read 2026-10-19) | this repository | content rules, extension list, manifest path, hex comparison |
| AP.09 | T1 | `.planning/preparation/CONFORMANCE.md` 3.3 and 7; `.planning/preparation/ENGINEERING.md` 3, 13 | 2026-10-19 edition | `library` preset, admission rules, caps |
| AP.10 | T2 | psx-spx Kernel (BIOS) page, version and CRC table; CDROM File Formats page (PS-EXE header, region marker, logo sectors) | `6d7d1bc106a7` | version-to-model mapping, forbidden Sony material |
