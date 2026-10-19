# Provenance

How code, data and hardware facts enter schwaystation, which upstream files may seed it, and which notices that
creates. Written on 2026-10-19, before any emulator code exists, so it governs the whole history. People and AI
assistants follow it for every commit. [ASSET_POLICY.md](ASSET_POLICY.md) covers BIOS, disc, executable, card and
fixture bytes; `THIRD-PARTY-NOTICES.md` (created with the first vendored file) holds the notices this file
requires. `[PV.NN]` IDs resolve in section 10.

| Question | Section |
|---|---|
| May this code, table or fact enter? | 1 |
| Is this upstream file usable at its pin? | 2, 3 |
| What seeds each module, and how is the seed retired? | 4 |
| What must a derived file, the notices file and each release zip contain? | 5 |
| Which projects are never opened, and why? | 6 |
| psx-spx, Sony documents, databases, measured constants | 7 |
| Which files are vendored unmodified? | 8 |

## 1. Rules
- **Licence.** schwaystation is MIT ([LICENSE](LICENSE)). Inbound code or data must be MIT, BSD-2/3-Clause, ISC,
  Zlib, BSL-1.0, Unlicense, CC0-1.0 or public domain *at the exact commit cited*. Read the repository licence
  **and** the file's own header at that pin. Licences change and repositories mix: DuckStation went GPL-3 to
  PolyForm to CC BY-NC-ND within two weeks of 2024 [PV.08]; MAME is GPL-2.0 as a whole with BSD-3 device files
  and a GPL-2 PlayStation driver beside them [PV.03]; PCSX-Redux is GPL-2 while its `nugget` submodule, which
  holds OpenBIOS and the tests, is MIT [PV.02]; ps1-tests is MIT but its executables carry MPL-2.0 `libpsn00b`
  bytes [PV.05].
- **Facts** (addresses, timings, measured values, test expectations) enter in this project's own words, with a
  citation giving source, tier and pin. Facts are free to use; prose, code, tables and file structure are not.
- **Permissive expression enters only through section 4.** Code, tables, comments, names or structure from
  another project are copied or ported only as a section 4 seed: one module, with its notice (section 5), a
  replacement trigger and a parity proof. Other permissive sources are read to learn from; the behaviour is then
  written from documentation and shown by a test.
- **Copyleft, non-commercial and unlicensed source stays closed.** GPL/LGPL source, source under a
  non-commercial licence (DuckStation) and source with no stated licence (PeterLemon/PSX, simias psx-guide and
  psx-hardware-tests) is never read past its licence header, never pasted into an assistant's context and never
  kept in the workspace (no clones, no vendored copies, no "reference" folders). Its documentation, changelogs,
  issue trackers, commit metadata and released binaries may be used; the binaries run only as oracles.
- **Suspect constants.** A constant, table or name that the cited sources do not explain is suspect. It is
  re-derived from a cited source, or removed, before it merges. A table copied verbatim from an MIT, ISC or BSD
  file is expression and becomes a section 4 seed.
- **AI assistance is disclosed** in a co-author trailer or the PR description. An assistant writes from
  documentation, this repository and test results, and is never asked to port, translate or reproduce code
  outside section 4. Why: in 2026 an AI-assisted MIT emulator was found to contain GPL code ported from emulators
  kept in its workspace and had to relicense [PV.11].
- **Firmware and game bytes never enter** (section 6, ASSET_POLICY.md), whatever licence the file that carries
  them claims: no Sony BIOS, no PsyQ `licensea.dat`, no SCEx string, no logo sectors, no retail disc sectors.
- **Pins are public** (AGENTS rule 12): every pin below answered 200 from the GitHub commits API on 2026-10-19,
  except where section 3 says otherwise.

## 2. Upstream audit (LICENSE and file headers read at the pin, 2026-10-19)
| Repository @ pin | LICENSE at the pin | Per-file headers checked | What may be used |
|---|---|---|---|
| ares-emulator/ares `9408cb43d494` (v148) [PV.01] | root `LICENSE`: ISC, "ares team, Near et al", 2004 to 2025, followed by bundled libraries' notices (GitHub reports NOASSERTION for that reason) | source files carry no per-file header; `ares/ps1/*` and `ares/component/processor/*` listed | ISC: `ares/ps1/{cpu,gpu,spu,disc,dma,mdec,memory,interrupt,timer,peripheral,system}/**`; nothing under `thirdparty/` |
| pcsx-redux/nugget `9db6d88d23c4` [PV.02] | `LICENSE`: MIT, "PCSX-Redux authors", 2019 | `openbios/main/main.c`, `openbios/kernel/{events,alloc}.c`, `tests/*` headers: MIT 2019 to 2025 | MIT: `openbios/**` (the OpenBIOS kernel and shell), `tests/**` (the hardware-farm tests, libcester, support runtime), `common.mk`, `psyqo/**`; PCSX-Redux's own GPL-2 tree is not part of this submodule and is never opened |
| mamedev/mame `96df6a708bdd` [PV.03] | `COPYING`: GPL-2.0 for the whole | `src/devices/cpu/psx/{psx,gte,dma,irq,rcnt,sio,mdec}.cpp`: `license:BSD-3-Clause` (smf); `src/devices/video/psx.cpp`: BSD-3 (smf); `src/devices/sound/spu.cpp`: BSD-3 (pSXAuthor, R. Belmont); `src/devices/bus/psx/{ctlrport,analogue,memcard,multitap}.cpp`: BSD-3 (Carl et al.); `src/mame/sony/psxcd.cpp`: BSD-3 (smf, R. Belmont, pSXAuthor, Carl); **`src/mame/sony/psx.cpp`: `license:GPL-2.0+`** | BSD-3: exactly the files named, each header re-read before it is opened again; the driver `src/mame/sony/psx.cpp` is never opened (its ROM declarations were listed by name only) |
| JaCzekanski/ps1-tests `f727802fead1`; release `build-158` [PV.05] | `LICENSE`: MIT 2020 | `common/test.c` has no header; `gte/test-all/tests.c` is generated data | MIT: sources, `vram.png` references, `psx.log` hardware logs, the GTE case table; the built executables contain MPL-2.0 `libpsn00b` and libgcc bytes and are fetch-only (ASSET_POLICY 2) |
| SingleStepTests/r3000 `51e227447d0b` [PV.06] | `LICENSE`: MIT 2025 | `transcode_json.py`, README | MIT: the `v1/*.json.bin` vectors (data) and the documented binary layout; generated from an ISC ares fork, so they are T3 evidence |
| rtissera/libchdr `47d794151b2b` (master; v0.3.0 is `93d8c239ff0d`) [PV.07] | `LICENSE.txt`: BSD-3-Clause, Romain Tisserand | `deps/`: lzma-26.02 (public domain), miniz-3.1.2 (MIT in its header, (unverified) at the pin), zstd-1.5.7 (BSD-3 upstream, vendored copy (unverified)) | BSD-3: vendored unmodified behind `SCHWAYSTATION_CHD` (v2.0) with all four notices |
| RetroAchievements/rcheevos `dfdc64ea035c` (v12.5.0 is the latest tag) [PV.09] | `LICENSE`: MIT | `src/hash/rc_hash_disc.c`, `src/runtime/rc_consoleinfo.c`, `include/rc_consoles.h` | MIT: `rc_hash_disc.c` vendored into a CTest that checks `--probe`'s MD5; the memory map and hash rule as facts |
| libretro/libretro-common `a3194c18ba65` [PV.10] | per-file: `include/libretro.h` header licenses it MIT, "The RetroArch team" | the header itself | MIT: `libretro.h` vendored unmodified (section 8) |
| szTheory/cogmaster `abd950941e93`; szTheory/muhtari `d698364b180d` (v0.2.0) [PV.12] | MIT; every commit by the owner's account or its release bot | `PROVENANCE.md`, `THIRD-PARTY-NOTICES.md`, git authors | all of it except their third-party files (`libretro.h`, vectors, the a7800 palette) and their consoles' facts; muhtari's local checkout SHA `ecbe7f52aa37` is not public and is never a pin |
| Kingcom/armips `62adab4ef30d` [PV.13] | `LICENSE.txt`: MIT 2009 to 2020 | tarball SHA-256 `f7a65102febb4811a336a06e6d93c7e84cc86548182d54e1b1b4e2bffe562f56`; submodules `ext/filesystem` `3f1c185ab414`, `ext/tinyformat` `6175527d738c` | as a tool: its output is ours; `.psx` instruction set, `hi()`/`lo()`, `.sym` |
| Lameguy64/mkpsxiso v2.30: tag object `b6f9a25c8a96`, commit `54fb1644ed87` [PV.14] | `LICENSE.md`: GPL-2.0; bundles tinyxml2, miniaudio, FLAC | tag tarball SHA-256 `b44778180d239a586c13762322a1f4b9692c787db8772790ddd2fa255a75d9e0`; release zips pinned in PSXDEV-EXTRACTS 4 | as a tool only, run in CI, never vendored; the GPL covers the tool, the image is ours; the `<license file>` feature is never used |
| Lameguy64/PSn00bSDK `5d9aa2d3dfc7`, release v0.24 [PV.15] | `LICENSE.md`: core MPL-2.0; mkpsxiso GPL-2+; the bundled `mipsel-none-elf` GCC 12.3.0 is GPL-3 with the runtime exception | `doc/toolchain.md`, `LICENSE.md` | headers and facts; GCC as a tool for OpenBIOS and the nugget tests only; `libpsn00b` is never linked into an own fixture |
| libretro/libretro-database `12c21cf79f1e` [PV.16] | `LICENSE`: CC BY-SA 4.0 | `dat/System.dat` L498 to L519 (20 PS1 BIOS rows plus the PSP and PS3 images), `metadat/redump/Sony - PlayStation.dat` header | digests as facts (ASSET_POLICY 4); DAT titles for the ledger with the CC-BY-SA notice; no bulk redistribution |
| psx-spx/psx-spx.github.io `6d7d1bc106a7` (`stable`) [PV.17] | no licence; README: not clean-room, relies on fair use | n/a | facts in our own words with a citation; paraphrase only (section 7) |
| spicyjpeg/psxavenc `bdc24e897ed0` [PV.18] | Zlib | repository metadata | as a tool for fixture XA and STR streams; output is ours |

## 3. How the audit was done
Every row in section 2 was checked on 2026-10-19 with `gh api repos/<owner>/<repo>/contents/<path>?ref=<pin>`
(raw media type) for the LICENSE file and for at least one source file header per directory that may be opened,
and with `gh api repos/<owner>/<repo>/commits/<pin>` to prove the pin is public. Tarballs and release zips were
downloaded once and hashed with `shasum -a 256`; the digests are in PSXDEV-EXTRACTS 4 and `pins.txt`. The
preparation files that first read each repository record what they opened and what they did not (PSX-SYSTEM
"Licence checks at pin", PSX-SPU-CDROM-AND-MDEC 0, PSX-ECOSYSTEM-AND-USERS 1.1, PSXDEV-EXTRACTS 3 and 8).
Two findings corrected the owner's brief: Avocado (`d7fcf9060ac7`) and Rustation (`5e650b949d6e`) are GPL-2 at
their pins, not MIT, and the simias psx-guide has no licence file; all three are in section 6. One pin was
re-resolved: mkpsxiso's `b6f9a25c8a96` is the annotated v2.30 tag object, whose commit is `54fb1644ed87`. One
pin was replaced: muhtari's `ecbe7f52aa37` exists only in a local checkout, so the seed pin is the v0.2.0 release
commit `d698364b180d` (XFER-057). Re-check at every pin bump: a licence is true at a commit, not at a repository.

## 4. Strangler-fig seeds
A seed is the only route by which another project's expression reaches `src/`, `runner/`, `libretro/` or
`tests/`. An own implementation replaces a seed when the tests pass: retiring one needs a **zero-diff
differential**, the seed and its replacement run in one test binary over the row's corpus, and the frame,
VRAM, audio and state hashes must match on all eight lanes. The notice leaves in the same PR.

| Module | Seed @ pin | Licence | Must change | Attribution | Replace when | Parity corpus |
|---|---|---|---|---|---|---|
| Runner, libretro adapter, state visitor, CRC/SHA/PNG helpers, tests scaffolding, CI, hygiene, repo settings, publish | cogmaster @`abd950941e93` (which carries muhtari @`d698364b180d` and nesturbator) | MIT, same owner | names (`schway_`), the test card, PS1 frame sizes and rates, exit-status table, fixture manifest path | commit-body line (section 5a) | n/a (own code) | its own hash pins |
| R3000A CPU, COP0, bus, DMA, timers, interrupts | none: own code from the IDT R3000 manual, psx-spx and the r3000 vectors; ares `ps1/cpu`, `dma`, `timer`, `interrupt` and MAME `cpu/psx/{psx,dma,irq,rcnt}.cpp` are read for structure and cross-checks | | | lifting any table turns it into a seed row | | r3000 vectors every cycle; `exc_matrix`; nugget cpu, cop0, timers |
| GTE | own code from psx-spx formulas; the UNR table is generated from the documented formula and asserted against the printed 257 bytes; ares `interpreter-gte.cpp` and MAME `gte.cpp` read for flag placement | | | a copied flag or saturation helper becomes a seed row | | ps1-tests 1150 cases; amidog nightly |
| GPU rasteriser and display | own code from psx-spx and the ps1-tests images; ares `ps1/gpu` (top-left bias, dither and blend code) and MAME `video/psx.cpp` read; the derived cost column of GPU 4.4 is a fact table with its citation | | | an edge-function or stepper routine ported from ares becomes a seed row with notice | | ps1-tests GPU `vram.png` set; `gpu_motion` |
| SPU, CD-ROM controller, XA, MDEC | own code from psx-spx; ares `ps1/spu`, `ps1/disc`, `ps1/mdec` and MAME `sound/spu.cpp`, `cpu/psx/mdec.cpp`, `mame/sony/psxcd.cpp` read for rounding order and the seek curve | | | the seek curve constants are facts; a ported envelope table or FIR becomes a seed row | | `spu_tone`, `disc_boot`; ps1-tests spu, cdrom, mdec logs and PNGs |
| SIO0, pads, memory cards | own code from psx-spx; ares `ps1/peripheral` and MAME `bus/psx/*` read for ack delays and the card phase machine | | | | | `pad_counter`, `memcard_roundtrip`; nugget memcard, rumble |
| OpenBIOS (CI stand-in, never shipped) | pcsx-redux/nugget `openbios/**` @`9db6d88d23c4` | MIT | nothing: built unmodified from the pin with the pinned GCC; its digest is in `tests/built/expected.txt` | notices section naming the pin and the build flags | never; it is a stand-in, not core code | the `disc_boot` fixture and nugget `memcard-bios` |
| Stub kernel | none: own armips source `tests/fixtures/src/stub/` | MIT | | | n/a | every PS-EXE fixture |
| Disc image helper `schway_disc_image` | none: own parsers for cue/bin, iso, m3u (v1.0), ccd, pbp, ecm, sbi (v2.0); CHD through libchdr vendored unmodified @`47d794151b2b` (v2.0) | BSD-3 (libchdr) | nothing in libchdr; the wrapper is own code | notices sections for libchdr, LZMA SDK, miniz, zstd | never (a dependency, not a seed) | `fuzz.chd`; the local `library` preset |
| rcheevos hash check | `src/hash/rc_hash_disc.c` @`dfdc64ea035c` inside one CTest | MIT | nothing | notices section | n/a (test only) | `--probe` on the fixture discs |
| libretro API | `libretro.h` @`a3194c18ba65` | MIT | nothing | notices section | n/a | `libretro.host` |

Read, never copied: everything in ares `ares/ps1/*` and the MAME BSD-3 files not named as a seed above. ares is
also not an independent oracle for the r3000 vectors, which it generated (PSXDEV-EXTRACTS 3.1).

## 5. Notices
**(a) Source files.** Own files carry no header; the repository LICENSE covers them. Code taken from cogmaster,
muhtari or nesturbator gets no header either; the commit body says
`Seeded from cogmaster <sha> <path> (same author, MIT).` A file derived from any other seed starts with an SPDX
line naming both licences, the upstream path and 40-hex commit, the copyright holders as the upstream header
names them, and what changed; the full licence text lives in `THIRD-PARTY-NOTICES.md` under a section of the
same name.
- **Recommendation:** an SPDX line in derived files only. **Strongest alternative:** an SPDX line in every file
  (REUSE style). **Would change if:** a distributor asks for REUSE compliance.

**(b) `THIRD-PARTY-NOTICES.md`** has one section per upstream, each giving: the paths it applies to in this
repository; the upstream URL, path and 40-hex commit; the SHA-256 of any vendored unmodified file; what changed;
and the licence text verbatim, naming the holders as the upstream header does. The nugget section carries both
URLs (nugget and pcsx-redux) and states that PCSX-Redux's GPL-2 does not reach the submodule. The
libretro-database section carries the CC BY-SA 4.0 notice for ledger titles. Built and fetched programs that
never ship (OpenBIOS, nugget tests, ps1-tests executables) are listed with their pins so the build is
reproducible, even though running them creates no distribution obligation.

**(c) Release archives.** From the first release, the library, runner and libretro zips each hold `LICENSE` and
`THIRD-PARTY-NOTICES.md` at the root. `release.notices` (CMake `-P`) checks it in `cmake --workflow --preset ci`
and before attestation; its self-test builds a zip without them and must fail, naming the missing file.
`policy.notices` checks every SPDX expression is `MIT` or `MIT AND` one of the section 1 licences, that each
derived file has a notices section, and that the libretro `.info` `license` field names the same licences
(`MIT` in v1.0; `MIT AND BSD-3-Clause` once CHD is built in).

## 6. Never opened, and why
"Unknown" counts as excluded until the upstream adds a notice. Facts about these projects come only from their
documentation, changelogs, issue trackers, release notes and released binaries; the binaries are oracles.
- **Non-commercial, no derivatives:** DuckStation (CC BY-NC-ND 4.0 at `124a4b16feda`; PolyForm Strict and GPL-3
  earlier; README forbids modified forks and packaging) [PV.08]. Its `gamedb.yaml` is facts-only too.
- **GPL:** SwanStation (GPL-3 `cc94ebcd38a8`); Mednafen and Beetle PSX / Beetle PSX HW (GPL-2 `9bcc75d0a3e8`);
  PCSX-ReARMed (GPL-2 `c8816799b503`, notaz `8b6c163575e6`) and PCSX-Reloaded; PCSX-Redux outside the nugget
  submodule (GPL-2 `91285bf736af`); Avocado (GPL-2 `d7fcf9060ac7`); Rustation and rustation-ng (GPL-2+
  `5e650b949d6e`, `aa1fb446e03a`); MiSTer PSX_MiSTer RTL (GPL-2 `cd17b5a0ad26`); BizHawk's Nymashock and
  Octoshock (Mednafen-derived); MAME `src/mame/sony/psx.cpp` (GPL-2.0+); psxact forks (GPL-3); the 240p Test
  Suite (GPL-2, and it has no PlayStation port) [PV.03][PV.08].
- **Unlicensed:** simias/psx-guide (`a89043ea499a`, prose only, not cited); simias/psx-hardware-tests
  (`f9569e7c8aac`, rejected); PeterLemon/PSX (`6d20c132aba0`, fetch-only binary oracle, never committed); amidog's
  psxtest zips (no stated terms; the wiki is CC BY-NC-SA 3.0); Redump DATs (no terms text found) [PV.19].
- **Closed:** ePSXe, XEBRA, no$psx, pSX. Their documentation is T4; no$psx's author wrote psx-spx, which is cited
  through the edition.
- **Why the line is drawn here.** A permissive licence today does not clear a repository's history or the bytes
  it ships (XFER-072); DuckStation's two relicensings and the SwanStation hard fork are the concrete case, and
  the 2026 relicensing of an AI-assisted MIT emulator shows what one pasted function costs [PV.11].

## 7. Documentation, databases and measured constants
- **psx-spx.** Cited as T2 at the consoledev.net edition's `stable` pin `6d7d1bc106a7`; nocash's original
  `psx-spx.htm` only where the edition marks a passage as his. The edition states it holds no licence from
  nocash and that parts were paraphrased from confidential Sony material, so: paraphrase only, no passage longer
  than a short phrase, register names and byte strings quoted at most, never `master` (unreviewed AI edits). A
  table of constants printed there (the gaussian table, the UNR table, the XA zigzag filter, the reverb FIR, the
  dither matrix) is re-derived from its generating formula where one is documented and asserted against the
  printed values in a unit test; where no formula exists the values are restated as a cited fact table [PV.17].
- **Sony and MIPS documents** (the IDT R3000 family manual, official SDK and hardware documentation where
  available as archived scans) are T1 facts in our own words with a citation; no scan enters the repository.
- **Databases.** The libretro `System.dat` BIOS digests are facts (ASSET_POLICY 4). Ledger titles come from the
  CC-BY-SA-4.0 libretro-database Redump DAT with its notice; serials and the rcheevos MD5 are computed by our
  own probe. No DuckStation `gamedb.yaml` field is copied; no Redump DAT is redistributed. Behaviour never keys on
  a database row: a per-title hint is a profile default, never a hack.
- **Measured constants** (clocks, line lengths, GTE cycle counts, CD response means, SPU RAM schedule) are facts
  cited to the measurement and its method (PSXDEV-EXTRACTS 2). The nugget hardware-farm result files and the
  ps1-tests logs are T1; an emulator's constant is T3 and marked so.
- **Hash databases and hashes.** The only identities that leave the owner's machine are the Redump serial, the
  rcheevos MD5 and the Redump track SHA-1, all already public in RetroAchievements and Redump lists
  (CONFORMANCE 3.3).

## 8. Vendored files
Vendored files are never edited. The CTest `libretro.vendored` checks each SHA-256 in this table, and each notice
lives in `THIRD-PARTY-NOTICES.md`. The table is filled in v1.0 phase 1 with `libretro/libretro.h` and in v2.0
with libchdr and its three dependencies; `rc_hash_disc.c` joins when the probe CTest lands.

| File | Upstream | Upstream path | Commit | Licence | SHA-256 |
|---|---|---|---|---|---|
| `libretro/libretro.h` | https://github.com/libretro/libretro-common | `include/libretro.h` | `a3194c18ba65` (40-hex recorded at vendoring) | MIT | recorded at vendoring (phase 1) |
| `third_party/libchdr/**` (v2.0, `SCHWAYSTATION_CHD`) | https://github.com/rtissera/libchdr | whole tree minus tests | `47d794151b2b` | BSD-3-Clause; deps public domain, MIT, BSD-3 | recorded at vendoring |
| `tests/probe/rc_hash_disc.c` | https://github.com/RetroAchievements/rcheevos | `src/hash/rc_hash_disc.c` | `dfdc64ea035c` | MIT | recorded at vendoring |

## 9. What this file does not do
It keeps the project's own licence clean. It is not legal advice, and it is no defence against a rights holder's
claim; the asset policy handles that side by keeping firmware and game bytes out of the repository entirely.

## 10. Sources
| ID | Tier | Source (URL) | Pin or access date | Supports |
|---|---|---|---|---|
| PV.01 | T1 | github.com/ares-emulator/ares `LICENSE` (ISC text read raw (tested)); `ares/ps1/` tree; releases v148 | `9408cb43d494` | ISC scope |
| PV.02 | T1 | github.com/pcsx-redux/nugget `LICENSE`, `openbios/main/main.c`, `openbios/kernel/{events,alloc}.c`, `openbios/README.md`, `openbios/COMPATIBILITY.md`, `tests/run-emulator.sh`; github.com/grumpycoders/pcsx-redux `LICENSE` (GPL-2), `LICENSES.md`, `contents/src/mips` (submodule record) | `9db6d88d23c4`; `91285bf736af` | OpenBIOS and tests MIT; core GPL-2 |
| PV.03 | T1 | github.com/mamedev/mame `COPYING`; `license:` first lines of the fifteen files named in section 2, read raw (tested) | `96df6a708bdd` | per-file BSD-3 and the GPL-2 driver |
| PV.04 | T1 | psx-spx licence status as recorded in PSXDEV-EXTRACTS PX.03 | `6d7d1bc106a7` | paraphrase rule |
| PV.05 | T1 | github.com/JaCzekanski/ps1-tests `LICENSE`, README, `common.mk`, release `build-158` | `f727802fead1` | MIT sources; MPL bytes in builds |
| PV.06 | T1 | github.com/SingleStepTests/r3000 `LICENSE`, README, `transcode_json.py`; `ares_r3000_ssts` licence (ISC) | `51e227447d0b` | MIT vectors, T3 origin |
| PV.07 | T1 | github.com/rtissera/libchdr `LICENSE.txt`, `deps/` listing (lzma-26.02, miniz-3.1.2, zstd-1.5.7) (tested) | `47d794151b2b`; v0.3.0 `93d8c239ff0d` | BSD-3 and dependency licences |
| PV.08 | T1 | github.com/stenzek/duckstation `LICENSE` (CC BY-NC-ND 4.0, read raw (tested)), `commits?path=LICENSE` (2024-09-01 change), README "Contributing and Forks"; github.com/libretro/swanstation `LICENSE`; beetle-psx-libretro `COPYING`; pcsx_rearmed `COPYING`; Avocado `LICENSE`; rustation `LICENSE`; PSX_MiSTer `LICENSE` | `124a4b16feda`; `cc94ebcd38a8`; `9bcc75d0a3e8`; `c8816799b503`; `d7fcf9060ac7`; `5e650b949d6e`; `cd17b5a0ad26` | never-opened list |
| PV.09 | T1 | github.com/RetroAchievements/rcheevos `LICENSE` (MIT), `src/hash/rc_hash_disc.c`, `src/runtime/rc_consoleinfo.c` | `dfdc64ea035c`; v12.5.0 | hash rule, memory map |
| PV.10 | T1 | github.com/libretro/libretro-common `include/libretro.h` header (MIT) | `a3194c18ba65` | vendored header |
| PV.11 | T1 | nesturbator `.planning/preparation/DECISIONS.md` DEC.04 (relicensing precedent); cogmaster `PROVENANCE.md` PV.09 | `8ae2c6e32e7f`; `abd950941e93` | AI-assistance rule |
| PV.12 | T1 | github.com/szTheory/cogmaster `PROVENANCE.md`, `THIRD-PARTY-NOTICES.md`, git authors; github.com/szTheory/muhtari releases (v0.2.0 at `d698364b180d`), `PROVENANCE.md`; GitHub commits API: `ecbe7f52aa37` returned 422 (tested) | `abd950941e93`; `d698364b180d` | same-owner seeds; public-pin rule |
| PV.13 | T1 | github.com/Kingcom/armips `LICENSE.txt`, `contents/ext`; commit tarball hashed in PSXDEV-EXTRACTS 4 | `62adab4ef30d` | assembler licence and pin |
| PV.14 | T1 | github.com/Lameguy64/mkpsxiso `LICENSE.md`, `git/ref/tags/v2.30` (tag object `b6f9a25c8a96` → commit `54fb1644ed87`, tested) | v2.30 | tool licence and pin |
| PV.15 | T1 | github.com/Lameguy64/PSn00bSDK `LICENSE.md`, `doc/toolchain.md`, release v0.24 | `5d9aa2d3dfc7` | MPL core, GPL tools |
| PV.16 | T1 | github.com/libretro/libretro-database `LICENSE` (CC BY-SA 4.0), `dat/System.dat` L498 to L519 (tested) | `12c21cf79f1e` | digests, DAT terms |
| PV.17 | T2 | github.com/psx-spx/psx-spx.github.io README, `docs/` tree; problemkaputt.de/psx-spx.htm | `6d7d1bc106a7`; accessed 2026-10-19 | documentation rules |
| PV.18 | T1 | github.com/spicyjpeg/psxavenc licence field | `bdc24e897ed0` | Zlib tool |
| PV.19 | T1 | github.com/simias/psx-guide tree; simias/psx-hardware-tests tree; PeterLemon/PSX README; psx.amidog.se download pages; redump.org/downloads | `a89043ea499a`; `f9569e7c8aac`; `6d20c132aba0`; accessed 2026-10-19 | unlicensed list |
