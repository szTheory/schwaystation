---
title: "Engineering: C rules, tests, CI, release, repo settings and quality loops"
summary: "Build, test, CI, release, repository, triage, performance, portability and documentation decisions for schwaystation, a PlayStation core five to ten times the size of its 8-bit siblings, with mutable facts re-verified on 2026-10-19 and the sibling lessons folded in as rules with checks."
read_when: "Before creating or changing CMake files, presets, tests, workflows, repository settings, the release pipeline, quality gates or engineering docs."
updated: 2026-10-19
---

# Engineering: C rules, tests, CI, release, repo settings and quality loops

Mutable facts were re-read on 2026-10-19 [ENG.01–ENG.16]; sibling practice is cited by repo and commit [ENG.17–ENG.22]; sibling lessons by ID in `.planning/LESSONS.md` [ENG.20][ENG.25]; (tested) marks runs made here [ENG.24]. Choices read **Rec** (recommendation), **Alt** (strongest alternative), **Flip** (the observation that would change it). cogmaster's ENGINEERING.md is the direct ancestor [ENG.19] and muhtari's live tree the implementation it describes [ENG.17]; what differs is PlayStation specific (scale, fixtures, disc images, no BIOS, the interpreter's fast-path constraints) or a folded lesson.

## Where to look
| Question | Section |
|---|---|
| Runner labels and vCPUs, action SHAs, tool versions, release-please semantics, GitHub token rules, fixture tools | 1 |
| Flags, presets, integer-only / no-globals / no-UB checks, analyzers, coverage | 2 |
| Test tiers, exact inventory, no-skip, negative controls, property tests, fuzz targets, fixture caps | 3 |
| Workflows, jobs, aggregator, triggers, timeouts, caching, minutes per PR | 4 |
| release-please config, App token, 18 archives, attestations, rehearsal, withdrawal | 5 |
| Repo settings script, ruleset, branch policy; issue forms, labels, digest, Dependabot, YAGNI | 6, 7 |
| Ir/frame gate and its three fixtures, wall-clock tails, frame budgets, device budgets, `quality.json` ratchet | 8 |
| Big-endian, wasm32 and armhf lanes; style contract; interpreter fast-path constraints | 9, 10 |
| Docs budgets, evidence format; milestone tune-up; local paths and secrets; WASM and low-power | 11, 12, 13, 14 |
| Which sibling lesson became which rule and check | 15 |

## 1. Re-verified facts (2026-10-19)
**Runners** (standard, public repo, free) [ENG.01][ENG.02][ENG.03]:
| OS × arch | Label | vCPU / RAM | On the image (image version) | Notes |
|---|---|---|---|---|
| Linux x64 | `ubuntu-26.04` | 4 / 16 GB | GCC 13.4, 14.3, 15.2; Clang, clang-format, clang-tidy 20.1.8, 21.1.8, 22.1.2; CMake 4.4.4; Ninja 1.13.2; jq 1.8.1 (20261012.163.1) | GA 2026-09-17 (#14747, open); `ubuntu-latest` = `ubuntu-24.04` until November 2026 (#14748, open) |
| Linux arm64 | `ubuntu-26.04-arm` | 4 / 16 GB | same toolchains (20261012.150.1) | |
| macOS arm64 | `macos-15` | 3 (M1) / 7 GB | Apple clang 17.0.0; Homebrew `llvm@18` 18.1.8; CMake 4.4.3 (20260907.0337.1) | `macos-latest` = `macos-26` |
| macOS x64 | `macos-15-intel` | 4 / 14 GB | Apple clang 17.0.0; CMake 4.4.2 (20260824.0482.1) | x86_64 offered until August 2027 (#13045, closed); `macos-26-intel` listed |
| Windows x64 | `windows-2025` | 4 / 16 GB | VS 2026 18.10; LLVM 20.1.8; CMake 4.4.4 (`Windows2025-VS2026`, 20261007.264.1) | `windows-latest` = `windows-2025` = `windows-2025-vs2026`; the plain `Windows2025` readme still lists CMake 3.31.6, which is not the image the label resolves to |
| Windows arm64 | `windows-11-arm` | 4 / 16 GB | VS 2026 18.10; LLVM 22.1.8; CMake 4.4.4 (20261013.183.4) | resolves to `Windows11-VS2026-Arm64` since September 2026 (#14602, open) |

Change since cogmaster's 2026-10-18 read: none on the six images (tested) [ENG.02]. Elsewhere: wasmtime 49.0.4 and 48.0.7 (LTS) and Emscripten 6.0.13 were released today; LLVM 23.1.3 (2026-10-06) is upstream's latest but no image carries it [ENG.23][ENG.26][ENG.29]. **Rec** these six, Ubuntu 26.04 for every Linux job. **Alt** Ubuntu 24.04. **Flip** a 26.04 defect costs a red day, or 26.04-arm queue p95 > 2 min.

Apt-installable on 26.04 ("resolute"), not preinstalled, (tested) [ENG.13]: Valgrind 3.26.0, cppcheck 2.19.0, `qemu-user` 10.2.1, `gcc-15-{powerpc,arm}-linux-gnu(eabihf)` 15.2.0, `wasi-libc` git20250726.

**Actions**: pin the full SHA with a `# vX.Y.Z` comment. muhtari's live pins [ENG.17] were compared with `releases/latest` and `git/ref/tags` on 2026-10-19 (tested) [ENG.04]; no newer release exists for any of them:
| Action | Pin | Latest release | Commit SHA |
|---|---|---|---|
| actions/checkout | v7.0.1 | v7.0.1 (2026-07-20) | `3d3c42e5aac5ba805825da76410c181273ba90b1` |
| actions/upload-artifact | v7.0.2 | v7.0.2 (2026-10-07) | `cf430e030ddbb5b0abf93d22962f4752f3646cd9` |
| actions/download-artifact | v8.0.2 | v8.0.2 (2026-10-07) | `9000827ccba6bdab643e8b6fd33ac0654aef8333` |
| actions/attest | v4.2.2 | v4.2.2 (2026-08-04) | `1e69f48acb82d1966a394da916b4c1698aa569d6` |
| actions/create-github-app-token | v3.3.0 | v3.3.0 (2026-10-12) | `64ab729a6294000f5aa4a4bc0b73d1f32bc45510` |
| googleapis/release-please-action | v5.0.0 | v5.0.0 (2026-04-22) | `45996ed1f6d02564a971a2fa1b5860e934307cf7` |
| dependabot/fetch-metadata | v3.1.0 | v3.1.0 (2026-04-20) | `25dd0e34f4fe68f24cc83900b1fe3fe149efef98` |

`create-github-app-token` takes `client-id` and `permission-<name>: write` inputs that narrow the token, as muhtari's `release.yml` uses them [ENG.11][ENG.17]; `actions/attest` with no predicate makes SLSA build provenance from `subject-checksums` [ENG.12].

**release-please**: the action's `package.json` at v5.0.0 depends on `release-please` `^17.6.0` and its lock resolves 17.6.0 (re-read 2026-10-19, tested); the library is at 17.11.2 (2026-08-24), and the changelog between them does not touch `simple`, the pre-major bump options, `initial-version`, `draft` or `force-tag-creation` [ENG.05][ENG.06]:
| Option | Behaviour |
|---|---|
| `release-type: simple` | edits `version.txt` (must exist) and `CHANGELOG.md`; the manifest releaser edits `.release-please-manifest.json` |
| `bump-minor-pre-major: true`; `bump-patch-for-minor-pre-major: false` | below 1.0.0 a breaking change bumps minor; `feat` bumps minor |
| `initial-version: "0.1.0"` | used only when no release exists; manifest `"0.0.0"` counts as none |
| `draft: true`, `force-tag-creation: true` | draft release whose tag exists at once |
| `Release-As: x.y.z` footer | newest such footer since the last release wins; config key `release-as` is deprecated and sticks |

**GitHub behaviour** [ENG.07][ENG.08][ENG.09][ENG.10][ENG.22]:
- An event caused by `GITHUB_TOKEN` starts no workflow run (PR `opened`/`synchronize`/`reopened` runs wait for approval); an App token avoids both.
- Dependabot-triggered runs get a read-only `GITHUB_TOKEN` and only Dependabot secrets; whether `vars` resolve there is (unverified).
- Immutable releases: tag and assets lock at publish; a deleted immutable release's tag name is never reusable; title, notes, pre-release and latest flags stay editable; publishing also creates a release attestation; attach every asset to the draft first (github/docs at `9af3df3aa46b`, file present, accessed 2026-10-19).
- `PUT /repos/{o}/{r}/actions/permissions` takes `sha_pinning_required`; `…/permissions/workflow` takes `default_workflow_permissions` and `can_approve_pull_request_reviews` (docs page re-read 2026-10-19, tested) [ENG.10].
- A head commit message with `[skip ci]`, `[ci skip]`, `[no ci]`, `[skip actions]`, `[actions skip]` or `skip-checks:true` starts no `push`/`pull_request` run. GSD `/gsd-ship` titles the PR `Phase N: Name` and pushes a `[ci skip]` ship note [ENG.22].

**Fixture tools** [ENG.14][ENG.15]: both are build tools, never linked and never seeded, fetched by `scripts/fetch-pin.sh` from a `pins.txt` row (`name version url sha256`), verified before extraction, built or unpacked under `build/pins/`; `tests/fixtures/manifest.txt` records every fixture's SHA-1 as built; a digest mismatch fails configure (sibling lesson XFER-056; signature chains stay off the critical path).
| Tool | Licence and version | Role | Negative control |
|---|---|---|---|
| armips (Kingcom) | MIT (`LICENSE.txt`); last release v0.11.0 (2020-05-10), `master` `62adab4ef30d` (2026-08-01); the exact pin is PSXDEV-EXTRACTS 4 | assembles `tests/fixtures/src/*.s` (pure MIPS, no SDK) into PS-EXE fixtures | `fixtures.armips_pin.self_test`: a wrong digest fails with `armips digest mismatch` |
| mkpsxiso (Lameguy64) | GPL-2.0, v2.30 (2026-07-06), prebuilt zips for Linux, macOS, win64 | builds the cue/bin disc fixtures from XML; GPL reaches neither the image nor the core | `fixtures.mkpsxiso_pin.self_test` |
| BIOS stand-in | no Sony BIOS in CI; a stub kernel assembled with armips or a permissive stand-in, PSXDEV-EXTRACTS 5 (MIT at `9db6d88d23c4`, README and `main/main.c` headers checked; PX.60) | boots PS-EXE fixtures with a fixed exception vector and the kernel calls the fixtures use | `hygiene.no_bios_bytes` scans every tracked file for the SCPH BIOS signatures compared as hex (sibling lesson XFER-076) |

## 2. C rules, flags and presets
C17 with `CMAKE_C_EXTENSIONS OFF`; `cmake_minimum_required(VERSION 3.25)` for embedders; `CMakePresets.json` version 11 (CMake ≥ 4.3, on every chosen image) so test presets can say `"jobs": ""` [ENG.16]; one entrypoint `cmake --workflow --preset <lane>` locally and in CI [ENG.17].
| Rule | Check |
|---|---|
| Unsigned fixed-width guest arithmetic, cast to `uint32_t` before `<<` or `*`; signed guest values (GTE, CPU `SLT`, SPU volumes) through explicit two's-complement helpers, never signed shifts; no VLAs, recursion, bit-fields, packed structs, punning unions, FP | warnings; UBSan in `asan`; property tests |
| **No floating point anywhere in the core**: the R3000A has no FPU and the GTE, GPU, SPU and MDEC are fixed-point | `hygiene.no_float` scans `src/` and `include/` for `float`, `double`, `<math.h>`, `<float.h>` and float literals (muhtari `tests/cmake/no_float.cmake`); preset `nofp` builds every core object with GCC `-mgeneral-regs-only` (x64, arm64); `abi.undefined` runs `nm -P` on the Release archive against per-OS allowlists (`memcpy memmove memset memcmp`, Darwin `bzero`, x86-64 ELF `_GLOBAL_OFFSET_TABLE_`, MSVC `__ImageBase`, ppc32 `__udivdi3 __umoddi3 __ashldi3 __lshrdi3`; also the 64-bit multiply the GTE's `MAC` needs on ppc32 if GCC emits a libgcc call, measured in phase 1) and rejects any `__wasm_*` or `__aeabi_*` float helper (sibling lesson XFER-060); a float fixture must fail `hygiene.no_float` and `nofp`; `abi.undefined`'s negative control is a libc fixture (a call to `abort`) |
| No mutable globals or statics | `abi.globals`: no `B b D d C` symbol in the Release static library; `llvm-nm` reads COFF. File-scope `const` tables hold integers only: under PIC a `const` pointer table lands in `.data.rel.ro` (`d`), so opcode and GPU-command dispatch use `switch` or pointers stored in the instance at load (sibling lesson XFER-071) |
| One symbol namespace | `abi.symbols`: each defined external symbol is `schway_*` declared in `include/schwaystation.h`, or internal `schway__<module>_*`; libretro option keys, file names and CMake names keep `schwaystation` |
| Header is C and C++ | `abi.header.c17`, `abi.header.cxx17` (`-Wall -Wextra -Wpedantic -Werror`; MSVC `/TC`, `/TP`), `install.consumer`, `embed.subdirectory` |
| No host paths in binaries | `-ffile-prefix-map=<source dir>=.`; `release.no_build_paths` scans each archive |
| Hash and inventory files are LF | `file(CONFIGURE … NEWLINE_STYLE UNIX)`, never `file(WRITE)`; `hygiene.lf_only` rejects CRLF in `tests/**/*.txt` (sibling lesson XFER-061) |
| Git revision embedded at configure | `build.revision_matches_head`; the CI preset configures after checkout; `docs/testing.md` says to reconfigure after every commit (sibling lesson GB-GSD-020) |
| The core never reads files | disc sectors, memory cards and the BIOS stand-in arrive through caller callbacks and buffers; `abi.undefined` has no `fopen`, `read` or `clock_gettime` in any allowlist; `core.no_host_clock` proves two runs with injected RTC values differ only where the RTC is read |

Warnings (GCC, Clang): `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wstrict-prototypes -Wmissing-prototypes -Wvla -Wundef -Wcast-qual -Wimplicit-fallthrough -Wmissing-variable-declarations` [ENG.28]; GCC adds `-Wduplicated-cond -Wlogical-op`; MSVC `/W4 /utf-8`. Applied only when `PROJECT_IS_TOP_LEVEL`, fatal only through presets (`CMAKE_COMPILE_WARNING_AS_ERROR`). `-Wswitch-default`, `-Wdeclaration-after-statement` and `-Wpadded` stay out (cogmaster's 2026-10-15 run [ENG.19]).

| Preset | Compiler and flags | Where | Tests |
|---|---|---|---|
| `dev` | Debug, host compiler | local | inventory `ci` |
| `ci`, `ci-msvc` | Release, warnings fatal; MSVC environment from vswhere + vcvarsall (no third-party action) | six legs | inventory `ci`, then package |
| `asan` | clang-21 `-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer`; `SCHWAYSTATION_SHORT_RUNS` shortens long runs, never omits a test; TIMEOUT × 3 | ubuntu x64 | inventory `ci` minus labels `abi`, `release` |
| `nofp` | GCC 15 + `-mgeneral-regs-only` | ubuntu x64, arm64 | label `abi` |
| `hygiene` | clang-format-21 (configure fails on another major), clang-tidy-21, jq, git, sh, dash; `CMAKE_OSX_SYSROOT` set on macOS (sibling lesson XFER-060) | ubuntu x64 (local: Mac) | label `hygiene` |
| `be32` | `powerpc-linux-gnu-gcc` 15 + qemu-ppc as `CMAKE_CROSSCOMPILING_EMULATOR`; TIMEOUT × 11 (muhtari measured 10.7×) | ubuntu x64 | label `portable` (section 9) |
| `wasm32` | wasi-sdk clang + wasmtime as `CMAKE_CROSSCOMPILING_EMULATOR`; TIMEOUT × 3 (muhtari measured 2.5×) | ubuntu x64 | label `portable` (section 14) |
| `fuzz`, `coverage`, `vectors-full`, `oracle`, `bench` | clang-21 libFuzzer + ASan/UBSan; clang-21 source coverage; the full CPU vector set if a permissive R3000 set exists (CONFORMANCE); reference binaries; Release | nightly | sections 3, 8 |
| `library` | Release; private paths from `.env.local` | owner's Mac | the owner's disc dumps and real BIOS |

- **Analyzers. Rec** `clang-tidy-21 -checks='-*,clang-analyzer-*'` over `src/` in `hygiene`, zero findings; at PS1 scale this takes minutes, so `hygiene` runs it only on files changed against `main` per PR and over all of `src/` nightly; GCC `-fanalyzer` and cppcheck at the tune-up. **Alt** no analyzer. **Flip** three `NOLINT`s needed → tune-up only; a nightly run finds a real bug → per PR on every file.
- **clang-format. Rec** 21 (preinstalled on 26.04, equals Homebrew `llvm@21`); LLVM base, indent 4, 100 columns, Linux braces [ENG.17]. **Alt** 18. **Flip** Linux jobs stay on 24.04.
- **Sanitizers. Rec** one ASan + UBSan lane per PR; no MSan/TSan. **Alt** MSan nightly. **Flip** a hash mismatch traces to uninitialised memory. **Coverage. Rec** nightly llvm-cov: line % reported; function coverage of `src/` held at 100 %. **Alt** per-PR line ratchet. **Flip** a shipped regression sat in uncovered lines twice.

## 3. Tests
| Tier | Trigger | Content |
|---|---|---|
| PR, required | every PR; heavy jobs skip on `.planning/**`-only changes | units, committed CPU vector sample (CONFORMANCE names the set), armips PS-EXE and mkpsxiso disc fixtures' frame/audio/state hashes, `fuzz.regress.*`, `policy.*`, `hygiene.*`, `abi.*`, `install.consumer`, `embed.subdirectory`, `libretro.host`, `rehearsal` when release inputs change (section 5) |
| Nightly | schedule, `workflow_dispatch`, PRs touching the nightly's inputs | full vectors at their pin, libFuzzer over ten targets, `suite-flake` (`--repeat until-fail:3 --schedule-random`), coverage, bench, `armhf`, oracles; red → one rolling `nightly` issue |
| Local | owner | `library` preset (dumps, real BIOS), one RetroArch session per milestone |

- **Exact inventory** (gabbaboy `cmake/ExpectedTests.cmake` [ENG.19]; muhtari `cmake/MuhtariTests.cmake` [ENG.17]): `tests/inventory/<preset>.txt` holds sorted `name<TAB>labels` lines; a `cmake_language(DEFER)` check compares the directory `TESTS` property with it as sorted sets and names every missing and unexpected entry. Subsystem files are pulled in with `include()`, never `add_subdirectory()`. One `ci.txt` serves all six legs; `check_junit.cmake` filters it by `LABELS` and `EXCLUDE` for the lanes that run a subset.
- After the test step `tests/cmake/check_junit.cmake` requires each listed name exactly once with `status="run"` and no `failure`, `error` or `skipped`. It never matches CTest's summary text; every acceptance check asserts on exit status and a tolerant regex, and `policy.no_exact_output` rejects literal tool summary strings in `tests/` (sibling lesson XFER-058).
- **No self-skip:** `policy.no_skip` rejects `SKIP_RETURN_CODE`, `SKIP_REGULAR_EXPRESSION` and `DISABLED`; a missing tool fails configure through `find_program(... REQUIRED)`. **Negative controls:** every gate test `x` has `x.self_test` through `tests/cmake/expect_fail.cmake -DCMD=… -DCODE=<n> -DREGEX=<rule>`, which passes only on exactly that exit code and output (muhtari's script); bare `WILL_FAIL` is banned. A failing control exits with its own code under ASan too: the fixture frees what it allocated before failing, and `expect_fail.cmake` runs it with `ASAN_OPTIONS=detect_leaks=0` (sibling lesson GB-CGB-002); `asan` runs every `*.self_test`.
- **Registration:** `schwaystation_add_test(NAME … LABELS … TIMEOUT … COST …)` multiplies TIMEOUT by `SCHWAYSTATION_TIMEOUT_SCALE` (1; 3 under sanitizers and wasmtime; 11 under qemu-ppc, re-measured in phase 1 on the PS1 fixtures) in one place, so no helper can forget it; `policy.timeout_scaled` checks that no test under `asan` has the unscaled value (sibling lessons GB-CI-004/005). One file per subsystem, `tests/{cpu,cop0,gte,bus,dma,timers,gpu,spu,cdrom,mdec,sio,memcard,exe,disc,state,runner,libretro,policy,fuzz,perf,release}/tests.cmake`, each ≤ 400 lines. Names are `<subsystem>.<behaviour>`, never phase, plan or requirement IDs.
- **Property tests. Rec** in-repo `tests/prop.h` (~60 lines): splitmix64 seeded per test; failure prints seed, case and repro; 100 cases per PR, 10,000 nightly; size-halving shrink (sibling lesson XFER-073). Properties: `run(N)` then `run(M)` equals `run(N+M)` in cycles; save → load → continue equals uninterrupted; state round trip byte-identical; a rejected load or state leaves the instance unchanged; `probe` is pure; two instances never interact; input applies only at the next poll the SIO issues; NTSC-U and NTSC-J profiles of the same PS-EXE differ only where the profile says; the GTE's results equal a straight C reference for random inputs (overflow and saturation flags included); a DMA transfer and the same words written by the CPU leave identical VRAM or SPU RAM. **Alt** `theft` v0.4.5 (needs `fork`). **Flip** hand-minimising a failure takes > 30 min twice.
- **Fixture caps:** a PS-EXE fixture ≤ 64 KiB; a disc image fixture ≤ 512 KiB (a few hundred KB in practice: one data track with the PS-EXE and a short XA or CD-DA track built by mkpsxiso); `tests/fixtures/**` ≤ 4 MiB in all; CHD only through libchdr (section 7's dependency rule) and never as a committed fixture; `policy.fixture_budget` enforces the caps.
- **Fuzz corpus. Rec** seeds generated at build time by `tests/fuzz/seeds.c`; only minimised regressions committed; `tests/fuzz/**` ≤ 1 MiB. **Alt** commit evolved corpora. **Flip** nightly coverage plateaus → nightly artifact, still uncommitted. Fuzz targets, each an `LLVMFuzzerTestOneInput` plus a replay `main` registered as `fuzz.regress.<target>` on all six legs; nightly 60 s per target, clang-21 + ASan/UBSan, `-timeout=2`, `-max_len` per target; report-only until green for 7 consecutive nightly runs, then a crash fails the nightly:
| Target | Entry | `-max_len` | Arrives with |
|---|---|---|---|
| `fuzz.cue` | cue sheet text → track table | 64 KiB | disc loader |
| `fuzz.exe` | PS-EXE header and body → `schway_load_exe` | 2 MiB + 2 KiB | exe loader |
| `fuzz.cnf` | `SYSTEM.CNF` text → boot file, TCB, EVENT, STACK | 4 KiB | disc boot |
| `fuzz.memcard` | 128 KiB image → `schway_load_memcard` | 128 KiB + 64 | memory cards |
| `fuzz.state` | `schway_load_state` after a fixed load (validate, then apply) | 4 MiB + 4 KiB | save states |
| `fuzz.gpu` | word stream into GP0/GP1 for 2 frames | 64 KiB | GPU |
| `fuzz.cdrom` | command and parameter stream into the CD-ROM controller for 2 frames | 16 KiB | CD-ROM |
| `fuzz.spu` | register write stream for 2 frames | 64 KiB | SPU |
| `fuzz.mdec` | MDEC bitstream and command words | 256 KiB | MDEC |
| `fuzz.sio` | random bus bytes into the pad and card devices | 16 KiB | SIO0 (the tenth target, ARCHITECTURE 13) |

## 4. CI design
Workflows: `ci.yml` (required), `nightly.yml`, `release.yml`, `digest.yml` (weekly, section 7), `dependabot-automerge.yml`; the job graph is muhtari's [ENG.17].
| Job (`ci.yml`) | Runner | Content | Skipped when |
|---|---|---|---|
| `changes` | ubuntu x64 | `heavy=false` only if every changed path, old and new name, is under `.planning/`; empty or unknown → heavy; `ci.classify` CTest covers mixed, renamed and empty lists; `release_inputs=true` when section 5's rehearsal inputs changed | never |
| `hygiene`, `title` | ubuntu x64 | preset `hygiene`; live PR title and body via `gh pr view` | `title`: not a PR |
| `build` × 6 | section 1 | `ci`/`ci-msvc`; archive check; `hashes.txt` + `commit.txt`; README install line on macOS arm64 | light |
| `asan`, `nofp` × 2, `be32`, `wasm32`, `hash-equality` | ubuntu | presets of the same names; `hash-equality` compares eight byte-identical `hashes.txt` (six legs + `be32` + `wasm32`); `wasm32` joins `CI required` once it has a record on `main` | light |
| `retroarch` | macos-15 | RetroArch from the pinned DMG runs the built core with the test card; screenshot equals the runner's frame pixel for pixel, with its negative control; ≤ 300 lines in total (HOSTS-AND-INTEGRATION) | light |
| `perf`, `quality`, `rehearsal` | ubuntu x64 | sections 8 and 5 | light; `perf` also when not a PR or when called with `ref`; `rehearsal` when `release_inputs=false` |
| `CI required` | ubuntu x64 | `if: always()`; jq over `toJSON(needs)`: every result `success`, except heavy jobs `skipped` when `heavy=false`, `title` and `perf` outside PRs, `rehearsal` when its inputs did not change; `ci.required` CTest runs the expression on synthetic `needs` | never |

- **Aggregator. Rec** one required check, `CI required`. **Alt** each job required. **Flip** the aggregator lets one real failure through. **Matrix. Rec** all six legs on every PR: identical output on six platforms is a product claim. **Alt** three per PR. **Flip** measured queueing dominates the critical path.
- **`.planning` skip. Rec** job-level skip via `changes`, aggregator still reporting; no trigger-level `paths` (a skipped workflow leaves the required check pending). **Alt** run everything (a `.planning`-only PR cost 15 jobs and 3,089 job-seconds on nesturbator, MU-CI-002 [ENG.25]). **Flip** the classifier lets a code change through.
- **Title. Rec** a regex job inside `ci.yml`: `^(feat|fix|perf|docs|test|build|ci|chore|refactor|style|revert)(\([a-z][a-z0-9._-]*\))?!?: [a-z]`; a second pattern rejects `feat`/`fix`/`perf` scoped `ci`, `release`, `build`, `test` or `docs`; the job scans the live title and body for every skip token of section 1, because the squash body becomes `main`'s commit message (sibling lesson XFER-063); `ci.title` CTest holds title and body fixtures. **Alt** `amannn/action-semantic-pull-request`. **Flip** title rules outgrow one regex.
- **Shell. Rec** every script in `scripts/` and `tests/` is POSIX sh, run by `hygiene.dash` under `dash` as well as the default shell; a script that reads a pipe consumes all of stdin before any early exit (sibling lesson XFER-062), checked by `policy.sh_stdin`; no stateful shell harness wraps RetroArch (sibling lesson XFER-067). **Alt** bash with `set -euo pipefail`. **Flip** a POSIX-only script exceeds 150 lines (then a CMake `-P` script).
- **Triggers:** `pull_request` (opened, synchronize, reopened, edited), `push` to `main`, `workflow_dispatch`, `workflow_call` with `ref`. **Concurrency** `ci-${{ github.workflow }}-${{ github.event.pull_request.number || github.sha }}`, `cancel-in-progress` only for PRs; `release.yml` has its own group with `queue: max` and no `cancel-in-progress` [ENG.27]. **Permissions:** workflow `contents: read`; `title` and `changes` add `pull-requests: read`; every checkout `persist-credentials: false`. **Pins:** full SHA everywhere; `sha_pinning_required: true` in repo settings, watched by `scripts/apply-repo-settings.sh --check` (MU-CI-003).
- **Timeouts:** twice the slowest successful run among the last 40, rounded up to minutes, with run ID and sample count in a comment; 5-minute floor for network-bound and roll-up jobs; `provisional` until 10 samples; `hygiene.timeout_comments` enforces it [ENG.17]. **Cores:** Ninja default parallelism; test presets `"jobs": ""` [ENG.16]; long tests carry `COST`; local `ctest` in the foreground.
- **Caching. Rec** none in phase 1; fetched suites never cached. **Alt** ccache keyed by preset + label + compiler version. **Flip** the tune-up measures compile share ≥ 40 % of a critical-path leg; at 30–50 k lines of C this is likelier than on the 8-bit siblings (nesturbator measured 4–22 % [ENG.19]), so the phase-2 tune-up measures it first.
- **Minutes per PR** (muhtari phase 1 and 2 measurements [ENG.18]): a phase-1 run took 89 s wall, 15 jobs of 3–60 s, about 300 job-seconds; phase 2 added `perf` (41 s) and `retroarch` (18–22 s) and `be32` grew to 32–45 s with 257 vector tests. Budgets here, scaled for a core five to ten times larger: phase 1 ≤ 3 min wall and ≤ 10 runner-minutes per run; v1 close critical path ≤ 15 min and ≤ 150 runner-minutes per run (nesturbator closed v1 at 23 min and 93 runner-minutes [ENG.19]); `ci.critical_path_s` is a `quality.json` ceiling. First lever: the Intel leg runs label `portable` only; second: `be32` runs the CPU vectors on a sample.
- **Post-phase-1 tail. Rec** one `ci:` PR is budgeted right after the first merge for what only the first `push` run shows (ruleset, required check name, App merge, release PR, arming `perf`, promoting `wasm32`) (sibling lesson XFER-065). **Alt** fold it into phase 2. **Flip** the first merge is green end to end.
- Seed gates (SEED-007), not phase-1 work unless a requirement names them: `policy.timeout_scaled`, `policy.lessons_unique`, `policy.no_enhancement_in_core`, the JUnit timing script, the `changes` job, the `quality` job.

## 5. Release
```json
{"packages": {".": {"release-type": "simple", "initial-version": "0.1.0", "bump-minor-pre-major": true,
  "bump-patch-for-minor-pre-major": false, "draft": true, "force-tag-creation": true, "include-v-in-tag": true}}}
```
Manifest `{".": "0.0.0"}` and `version.txt` `0.0.0` until the first release, which is then 0.1.0 whatever the commit types [ENG.05].
1. A squash merge to `main` runs `release.yml`: App token (`client-id: ${{ vars.SCHWAYSTATION_APP_CLIENT_ID }}`, `private-key: ${{ secrets.SCHWAYSTATION_APP_PRIVATE_KEY }}`, `permission-contents`, `-pull-requests`, `-issues: write`), release-please, then `gh pr merge --auto --squash` on the release PR.
2. The App-made release PR runs CI without approval; GitHub merges it as the App when `CI required` passes; that push starts `release.yml` again.
3. On `release_created`, `uses: ./.github/workflows/ci.yml` runs CI with release-please's `sha` output as `ref`, never `github.sha`; `publish` fails unless every leg's `commit.txt` records the tag's commit.
4. `publish`: exactly 18 zips `schwaystation-{library,runner,libretro}-{linux,macos,windows}-{x64,arm64}.zip` → `SHA256SUMS` → `actions/attest` with `subject-checksums` → upload to the draft → `gh attestation verify` each → a copy with its last byte flipped must fail → publish as latest → `gh release verify vX` → re-read until 19 non-empty assets → download each from `releases/latest/download/`, check against `SHA256SUMS`, `gh attestation verify --signer-workflow <repo>/.github/workflows/release.yml --source-ref refs/heads/main`.
- **Re-runs:** `publish` is safe to re-run. The draft is addressed by release ID (release-please's `id` output, or `gh api --paginate` filtered on `draft and tag_name`), because `GET releases/tags/{tag}` omits drafts (sibling lesson XFER-039); `release.draft_by_id` is `publish.sh`'s self-test on a fixture list of 35 releases. A version already published with 19 verified assets exits 0. **Pre-1.0 policy:** `feat` → minor, breaking → minor, `fix`/`perf` → patch; `docs chore ci test build refactor style` cut no release; 1.0.0 only through a `Release-As: 1.0.0` final line in the PR body.
- **Version source. Rec** release-please edits only `version.txt`, the manifest and `CHANGELOG.md`; CMake `configure_file` generates the version header, `schwaystation_libretro.info` (`display_version`) and the runner's `--version`; no `extra-files`; `release.single_version_source` fails when the version string appears in any other tracked file; asset names carry no version (MU-REL-003). **Alt** `extra-files` markers. **Flip** a consumer needs the version in the file name.
- **Archives:** `LICENSE` and `THIRD-PARTY-NOTICES.md` in every zip (the MIT `libretro.h` notice; the BSD-3 libchdr, public-domain LZMA SDK, miniz and zstd notices when CHD is built in); no top-level folder; the libretro zip holds `cores/` and `info/`; `release.archive_contents` checks names, members and build paths on every leg. The libretro `.info` lists `supported_extensions` `cue|bin|exe|psexe|img|iso` plus `chd` only when built with CHD, checked by `release.info_claims`.
- **Rehearsal** of every publish step before the first real tag, as job `rehearsal` inside `ci.yml` and therefore inside `CI required`, on each same-repository PR whose `changes` output `release_inputs` is true (`release.yml`, `scripts/publish.sh`, `release-please-config.json`, `cmake/packaging.cmake`, `ci.yml` itself); it re-runs on every push to such a PR (sibling lesson XFER-064): CI on the head commit, then `publish.sh` against a prerelease `rehearsal-<run_id>-<run_attempt>` with `make_latest=false` and `--source-ref refs/pull/N/merge`, and a `cleanup` job that deletes the release and its tag whatever failed. muhtari's rehearsal took 399 s at its slowest in 11 runs [ENG.17]. Precedent: gabbaboy merged 18 release-fix PRs on its first tag [ENG.19].
- **Phase close:** no open `autorelease: pending` PR; `releases/latest` is the expected version with 19 assets. **Withdrawal** (`docs/maintaining.md`): `gh release edit vGOOD --latest`; edit the bad notes to "Withdrawn: reason; use vX"; fix forward with a `fix:` PR; delete only content that must not be distributed (its tag name is then gone for good) [ENG.09].
- **Bot credential. Rec** GitHub App token, minted only on `push` to `main` and in the Dependabot job. **Alt** a fine-grained PAT. **Flip** the owner declines to create an App. **Release-PR merge. Rec** native auto-merge. **Alt** a `workflow_run` merger. **Flip** auto-merge merges a head whose `CI required` is not green.

## 6. Repository settings as code and branch policy
`scripts/apply-repo-settings.sh` (POSIX sh + `gh api`, idempotent; `--check` prints all drift before exiting 1) [ENG.10][ENG.17]:
| Setting | Call |
|---|---|
| squash only; `PR_TITLE` / `PR_BODY`; auto-merge; delete branch on merge; no wiki/projects | `PATCH /repos/{o}/{r}` |
| secret scanning + push protection | same `PATCH`, `security_and_analysis` |
| private vulnerability reporting; Dependabot alerts + security updates | `PUT …/private-vulnerability-reporting`, `…/vulnerability-alerts`, `…/automated-security-fixes` |
| SHA-pinned actions; read-only default token; Actions cannot approve PRs | `PUT …/actions/permissions` `sha_pinning_required=true`; `…/actions/permissions/workflow` `default_workflow_permissions=read can_approve_pull_request_reviews=false` |
| immutable releases; the eight labels; the ruleset | `PUT …/immutable-releases`; `gh label create --force`; `.github/rulesets/main.json` |

`PR_BODY` matters because, with `COMMIT_MESSAGES`, GSD's `[ci skip]` ship note would land in `main`'s squash commit (MU-GSD-002). The ruleset: `~DEFAULT_BRANCH`; `deletion`, `non_fast_forward`, `required_linear_history`; `pull_request` with 0 approvals and `allowed_merge_methods: ["squash"]`; required check `CI required` with `integration_id` 15368; no bypass actors [ENG.17]. Apply it in phase 1 once a run has reported `CI required`.
- **Rec** script + committed ruleset, drift checked at the tune-up; `--check` covers every setting the script applies from phase 1 (lattice_stripe's unchecked `allow_auto_merge: false` broke every Dependabot merge, XFER-031). **Alt** a scheduled drift workflow. **Flip** a setting drifts unnoticed between two tune-ups.

Branch policy: `.githooks/pre-commit` runs `hygiene.sh --staged` and refuses a commit on `main` once `origin` exists; `.githooks/pre-push` runs `hygiene.sh --history`; one branch and one squash-merged PR per phase; right after `/gsd-ship` the agent retitles the PR (`gh pr edit N --title "feat: …"`). Code review runs before verification and never after it, at most two fix rounds and one gap round per phase (sibling lesson XFER-064). No attempt quotas, slots, admission or publication ceremonies anywhere in plans, requirements or gates (sibling lesson XFER-045); review checks REQUIREMENTS.md and PLAN.md for them at each discuss step.

## 7. Triage automation (minimal) and dependencies
- **Issue forms** (`.github/ISSUE_TEMPLATE/`): `bug.yml` (schwaystation version, OS/arch, frontend + version, machine profile, steps, expected/actual); `compatibility.yml` with required disc identity as `schwaystation-run --probe` prints it (serial from `SYSTEM.CNF`, track count, data-track SHA-1 of the first 16 sectors, and the executable's SHA-1), region, frontend + version, schwaystation version, symptom checkboxes (boot, graphics, GTE geometry, sound, CD-DA/XA, FMV, input, memory card, speed, crash); `feature.yml`; `config.yml` turns off blank issues and links private vulnerability reporting. Every form says: never attach disc images, BIOS files or memory card dumps of commercial saves. **Labels (8):** `bug`, `compatibility`, `enhancement`, `docs`, `performance`, `needs-info`, `nightly`, `digest` (plus Dependabot's `dependencies` and release-please's `autorelease: *`).
- **`digest.yml`**, Mondays 06:00 UTC, sh + gh + jq: rewrites the one open `digest` issue with unlabelled issues, PRs idle > 7 days, the open `nightly` issue and its age, Dependabot PRs > 7 days, a release PR open > 24 h or red, runner-image deprecations, `.planning` size; first Monday of a month adds the CI-efficiency section (section 12). **Rec** this one issue. **Alt** notifications. **Flip** the agent skips the digest for two phases. **Rolling nightly issue:** opened or updated on failure, closed on success [ENG.17]. Every scheduled reporter decides from `needs.*.result` or a step's `outcome`, never a piped `$?`: the default shell is `bash -e` without `pipefail` [ENG.27]. A `workflow_dispatch` input `force_failure` is run once in phase 1.
- **Dependabot:** `github-actions` only, weekly, one group for minor and patch (`update-types: [minor, patch]`), majors as their own PRs; `commit-message.prefix: ci`; `policy.dependabot` checks the file. `dependabot-automerge.yml` (~50 lines in muhtari): `fetch-metadata`, App token with `permission-contents`, `-pull-requests` (and `-workflows: write`, unverified need), `gh pr merge --auto --squash` for patch and minor; the private key is also a Dependabot secret [ENG.08].
- **Dependencies. Rec** none at runtime. Vendored: `libretro.h` (MIT, libretro-common `a3194c18ba65`, 2026-10-11 [ENG.21]) and, behind `SCHWAYSTATION_CHD` (default OFF), libchdr (BSD-3-Clause `LICENSE.txt`; v0.3.0 `93d8c239ff0d`, `master` `47d794151b2b` 2026-10-11) with its own vendored LZMA SDK 26.02 (public domain), miniz 3.1.2 (MIT in its header, unverified) and zstd 1.5.7 (BSD-3 upstream; the vendored copy's licence file is unverified at the pin): libchdr needs no system zlib (`WITH_SYSTEM_ZLIB` and `WITH_SYSTEM_ZSTD` default OFF), has `CHDR_LOWRAM_TARGET` for handhelds and ships its own libFuzzer target [ENG.21]. **v1 ships without CHD**: the option exists, `fuzz.cue` and the disc loader take sectors through one callback so a CHD reader is a second provider behind the same interface, and a seed under `.planning/seeds/` holds the CHD plan with the licence table above; shipping it means three more licences in every archive, a C++-free but 400 KB larger core, and a decompression path the hash lanes cannot cover with a committed fixture under the size cap. **Alt** ship CHD in v1 (most frontends' PS1 libraries are CHD). **Flip** the first ten `compatibility` issues name CHD, or a committed CHD fixture ≤ 512 KiB proves hash equality on all eight lanes.
- **YAGNI:** stale bot; CLA/DCO bots; auto-labellers; CodeQL per PR; Codecov; merge queue; Renovate; commitlint; release-drafter; AI triage; PR template; CODEOWNERS; Discussions; Pages; SBOM files; code signing and notarisation; a cheat database; an internal HLE BIOS beyond the stand-in.

## 8. Performance engineering and the ratchet
| Measure | Where | Rule |
|---|---|---|
| Cachegrind Ir/frame = (Ir at 360 frames − Ir at 60) / 300 per fixture (`--cache-sim=no`, `env -i`, equal-length install paths); head = PR merge commit, base = first parent measured twice (unequal → UNSTABLE); integer arithmetic in `tests/cmake/perf_compare.cmake` | `perf`, ubuntu x64, GCC 15, three fixtures: the built-in test card (phase 1), `gpu_motion` (joins the gate in the games phase) and `disc_boot` (joins in the discs phase) | report-only until two `workflow_dispatch` control runs (one commit measured three times) show zero spread, armed in phase 1 or 2 at the latest (sibling lesson XFER-051); then fail above +1.00 % (100 basis points) per fixture; exceptions are dated lines in `tests/perf/allow.txt`; a fixture joins the gate the phase its subsystem lands |
| Tracing costs nothing when off | `perf` | `schwaystation-run` with the dual-compiled loop and no hooks against `SCHWAYSTATION_TRACE_LOOP=OFF`: Ir/frame differs by ≤ 64 (derived); size delta goes to `quality.json` |
| Zero allocations after load | `core.no_alloc_after_load`, six legs | counting allocator through the config hook; after `schway_load`, 600 frames plus a state save and load into caller buffers and one disc swap make 0 calls; the frame loop never calls an allocator |
| Nearest-rank p50, p90, p99, p99.9, p99.99, p99.999 and max µs per frame; fps; max RSS; deadline misses per budget profile | nightly `bench`, ubuntu x64 + arm64 + macos-15, 300,000 headless frames of the test card and 60,000 each of the two heavy fixtures | trend data, never a gate; budgets below |

Frame budgets, `tests/perf/budgets.txt` (cycles per frame and rates from ARCHITECTURE 1: 263 × 3413 and 314 × 3406 GPU clocks through the exact ratio; the µs values follow from those rates (derived)):
| Profile | Guest work per frame | µs per frame | fps |
|---|---|---|---|
| `ntsc` | 566,203.775 CPU cycles (405,350,378,496/715,909) at 33.8688 MHz; 263 lines | 16,718 | 59.8173 |
| `pal` | 680,823.456 CPU cycles (482,961,862,656/709,379); 314 lines | 20,102 | 49.7468 |
| `lowpower` | NTSC ÷ 10 | 1,672 | hypothesis for a target 10× slower than the x64 runner (section 14) |

- **Rec** Ir/frame is the only per-PR speed gate; wall-clock tails are nightly trend data. **Alt** a wall-clock PR gate. **Flip** a quiet dedicated benchmark host appears. A zero-miss 95 % bound at p = 10⁻⁵ needs n ≥ 299,572 independent frames (glueyneo's calculation [ENG.19]), 83 min of NTSC play for the test card; the heavy fixtures get the p99.99 claim only. Ir is deterministic on the siblings (+0.00 % spread on nesturbator [ENG.19]); the PS1's CD-ROM seek model and SPU reverb must be timing-free of the host for that to hold, which `core.no_host_clock` proves.

Initial Ir/frame ceilings (derived hypotheses, replaced by measured +1 % at the first tune-up): test card ≤ 40 M; `gpu_motion` (joins the gate in the games phase) ≤ 150 M; `disc_boot` (joins in the discs phase) ≤ 120 M. Device budgets feed the ratchet as `budget = device_mips × frame_µs ÷ 2` (headroom for the host); the device MIPS are (unverified) order-of-magnitude figures until a device run calibrates them (section 14 ratchet input):
| Target | Assumed sustained MIPS | NTSC budget Ir/frame (÷ 2) | Meaning |
|---|---|---|---|
| Apple Silicon M1 class | 12,000 | 100 M | every fixture fits the interpreter |
| Raspberry Pi 4 (A72 1.5 GHz) | 2,250 | 19 M | the GTE/GPU fixture needs the cached interpreter |
| RG35XX-H class (A53 1.5 GHz) | 1,500 | 12.5 M | needs the cached interpreter and the GPU fast paths; the `lowpower` profile |
| wasm32 in Chrome | 0.5 × host | half the host row | the hash lane measures Ir under wasmtime, not Chrome |

**Ratchet contract:** one committed `quality.json` with floors (`tests.<preset>`, `compat.public`, `fuzz.targets`, `coverage.functions_pct`), ceilings (`warnings`, `analyzer_findings`, `fuzz.open_crashes`, `ir_per_frame.<fixture>`, `size.<zip>`, `size.wasm`, `ci.critical_path_s`, `state.bytes`) and `reasons[]`. Job `quality` runs `cmake -P tests/cmake/quality.cmake` with `string(JSON)`: a floor may not fall and a ceiling may not rise unless the same PR loosens the bound with a reason; tightening is free at each tune-up. `compat.local` (owner dumps) is recorded by the `library` preset and checked at the tune-up. Enhancements (upscaling, PGXP, texture filtering, 32-bit colour) are seeds and never enter the hashed native path: `policy.no_enhancement_in_core` rejects their option names in `src/`. **Rec** one file, one check. **Alt** per-metric scoreboards. **Flip** one metric's updates dominate PR churn.

## 9. Portability lanes
| Lane | Mechanism | Tier | Reason |
|---|---|---|---|
| `be32` | `gcc-powerpc-linux-gnu` 15.2 + `qemu-user` 10.2.1 via `CMAKE_CROSSCOMPILING_EMULATOR`; scripts that start the runner receive the same emulator | per PR from phase 1: label `portable` (core units, PS-EXE and cue parse, state round trip, hash list) and `hashes.txt` into `hash-equality`; full `ci` nightly | one lane covers big-endian, ILP32 and unsigned `char`; the PS1 is little-endian throughout (RAM, VRAM halfwords, SPU RAM, PS-EXE and disc sectors), so every guest access goes through `rd16le`/`rd32le` helpers and endian bugs surface here |
| `wasm32` | wasi-sdk + wasmtime (section 14) | per PR: label `portable`; hash lane eight | 32-bit, no FP, no libc surprises, browser ports |
| `armhf` | `gcc-arm-linux-gnueabihf` + `qemu-arm` | nightly | 32-bit ARM handhelds and Raspberry Pi |
| s390x, ppc64, i686, Win32; console compile-only | not used; devkitPro and pspdev containers pinned by digest | nightly, later milestone | covered by `be32`, `wasm32` and `armhf` (derived); core library with each SDK's toolchain file, no tests |

- **Rec** `be32` and `wasm32` per PR. **Alt** nightly only. **Flip** either job exceeds 5 min wall once measured in phase 1 (muhtari: 45 s with 257 vector tests under qemu-ppc [ENG.18]; a PS1 fixture frame costs 20–50× more, so the `portable` label stays to a few hundred frames). If Intel macOS runners end in August 2027, build a universal macOS binary on arm64 and test the x86_64 slice under Rosetta (unverified on runners).

## 10. Style contract (joy-to-read C) and the interpreter's fast-path constraints
| Topic | Rule |
|---|---|
| Names | public `schway_<noun>_<verb>()`, types `schway_<noun>`, macros and enum constants `SCHWAYSTATION_`; cross-file internals `schway__<module>_<verb>` (the module prefix inside the namespace so static libretro builds cannot collide; libretro option keys and file names keep `schwaystation`), module = its file (cpu, cop0, gte, bus, dma, timers, gpu, spu, cdrom, mdec, sio, memcard, exe, disc, state); file-local `static` names unprefixed; units in names (`cycles`, `lines`, `_bytes`, `_num`/`_den`); hardware registers keep the names of the Sony and psx-spx documents (`GP0`, `GP1`, `GPUSTAT`, `I_STAT`, `I_MASK`, `D2_CHCR`, `SR`, `CAUSE`, `EPC`, `BIU_CONFIG`) |
| Size | one module per chip; a file stays under 1,200 lines (the GPU rasteriser and the CPU decoder are the exceptions that must be split by concern, not by size; generated tables exempt); a function fits one screen (60 lines) unless it is a dispatch switch of one-line cases |
| Comments | say what the hardware does and why; every hardware fact cites its primary source and the `docs/hardware/<chip>.md` section; no planning IDs in code, tests or user docs; no other emulator's identifiers |
| Ladder | ponytail.dev: remove the need → reuse → standard library → platform → minimum new code; reused permissive code keeps its licence header and gets a THIRD-PARTY-NOTICES row |
| Errors | every fallible public call returns `schway_status`; an error changes no state (validate, then apply); untrusted bytes (cue, PS-EXE, `SYSTEM.CNF`, memory card, state, sectors) take checked paths; `assert` only states programmer invariants; the core never aborts, exits, prints, allocates after load, or reads files, clocks or the environment |
| Macros | constants and include guards only, plus the single dual-compiled traced loop (`run_loop.inc` included twice under `SCHWAYSTATION_TRACE` 0 and 1); no X-macros, no function-like macros |
| Format | clang-format 21 and the committed `.clang-format`; no hand-aligned columns |
| Tests | a test name states the behaviour (`gte.rtps.saturates_sz3_on_overflow`); each failure prints expected and actual |

Interpreter constraints that let a cached interpreter (v1.x) and a JIT seed land without redesign (ARCHITECTURE owns the design; these are the rules review enforces from phase 2):
| Constraint | Why |
|---|---|
| Decode is a pure function of the 32-bit word and the physical address; the interpreter's `exec` takes a decoded record, never re-reads RAM | a block cache stores the same records |
| Block boundaries are fixed: a block ends after the delay slot of any branch, jump, `SYSCALL`, `BREAK`, `RFE`, a COP0 write, at a 4 KiB page edge, or at 128 instructions | the cached interpreter and JIT share one `block_end()` predicate and one test for it |
| Every store to RAM goes through one `bus_write` that marks a 1 KiB dirty-page bitmap; code pages in the bitmap invalidate their blocks before the next fetch; the I-cache model (4 KiB, isolate-cache bit) is applied in the fetch path, not inside blocks | self-modifying code and `ICACHE` flush behaviour stay correct and hashed identically in every execution mode |
| The scratchpad (1 KiB at `0x1F800000`) is data only; a fetch from it takes the interpreter's exception path per PSX-SYSTEM | the block cache never maps it |
| Cycle accounting is per block with the event deadline checked at block ends and at every load from I/O; exceptions exit mid-block with exact `EPC`, `BD` and the delay-slot target | `run(N)` then `run(M)` equals `run(N+M)` under every mode |
| `core.exec_mode_equivalence` runs every fixture under interpreter and cached interpreter and compares hashes; the hashed native path is the interpreter until that test is green on all eight lanes | the hash claim never silently moves to a faster, different core |

- **Rec** enforce names, macros and format by `abi.symbols`, hygiene and clang-format; size, comments and the constraints above by review. **Alt** clang-tidy `readability-function-size` and naming checks. **Flip** size or naming comes up in two consecutive reviews.

## 11. Docs standards
| Item | Standard | Check |
|---|---|---|
| `README.md` | for users: status → install one-liners from `releases/latest/download/` → RetroArch and runner quick starts → BIOS and disc format notes → compatibility summary → docs ladder → licence; ≤ 16 KB | `docs.budgets`; install line run against the built zip on macOS arm64 |
| `docs/` | `embedding.md`, `runner.md`, `testing.md`, `maintaining.md` (App setup, repo settings, release, rehearsal, withdrawal, tune-up), `hardware/<chip>.md` (`cpu`, `cop0`, `gte`, `gpu`, `spu`, `cdrom`, `mdec`, `dma`, `timers`, `sio`, `memcard`) graduated from `.planning/preparation/PSX-*.md` when the subsystem lands, `compatibility.md` generated from `tests/compat/ledger.csv`; each ≤ 60 KB (the GPU and GTE files carry command tables) | `docs.budgets`; `docs.compatibility_fresh` |
| Public header | terse, one line per function and field, no hardware prose; ≤ 40 KB (discs, swaps, memory cards, controllers, multitap, save states, the trace hooks) | `docs.budgets` |
| Evidence; `.planning` | command + exit status + run URL in SUMMARY and VERIFICATION files, written by the command from the repository root, never pasted logs; `.planning` ≤ 4 MB per milestone, archived at close | review; digest reports size |
| Gate code | policy and hygiene code (`scripts/*.sh`, `tests/cmake/*.cmake`, `.githooks/*`) ≤ the absolute line count REQUIREMENTS names for the phase, ratcheted down at ship (`policy.gate_budget`) (muhtari carried 1,340 gate lines against 315 of `src/` at phase 1 [ENG.18]); CMake `-P` or POSIX sh only; real-host e2e code (RetroArch) ≤ 300 lines in total (sibling lesson XFER-044) | `policy.gate_budget`, `policy.e2e_budget` CTests |

**Rec** byte and line budgets as CTests. **Alt** review-only. **Flip** a budget blocks a correct change twice (raise it with a reason in `quality.json`).

## 12. Recurring loops
At each milestone close (six phases in v1.0), one tune-up phase fed by the monthly digest section:
| Loop | Data | Output |
|---|---|---|
| CI efficiency audit | per-job p50/p95 duration and queue time (`started_at − created_at`) over 30 days; flake rate (`run_attempt` > 1 that later passed, plus `suite-flake` failures); runner-minutes per merged PR against section 4's budgets; critical-path job; cores used = Σ JUnit durations ÷ `ctest` wall; compile share for the caching rule; lowest-value tests (never failed in 90 days and duplicated by a stronger signal) | timeouts re-derived; caching decision; tests removed only with the redundant signal named |
| Determinism | `hash-equality` and nightly results; every intentional hash change has a behaviour-revision bump and reason; `core.exec_mode_equivalence` once the cached interpreter exists | issue or fix |
| Weakest-quality-dimension review | score 1–5 with links for performance, CI/CD, ease of use, architecture, docs/README, upgrade path/semver, compatibility breadth | lowest becomes the next milestone's tune-up seed |
| Perf baseline | Ir/frame per fixture, sizes and the section 8 device table on `main` | `quality.json` ceilings tightened to measured |
| Docs freshness, drift | docs whose covered code changed since their last edit; budgets; install line; `apply-repo-settings.sh --check`; Dependabot majors; runner-image announcements; armips, mkpsxiso, wasi-sdk, wasmtime and libchdr pins | doc, settings or upgrade PRs |

Results go into a seed under `.planning/seeds/` plus `quality.json`; the release PR is green at every phase close. **Rec** per milestone. **Alt** per phase. **Flip** a phase-level regression goes unaddressed for a whole milestone.

## 13. Configuration and secrets (12-factor)
- Local-only paths live in gitignored `.env.local` (`SCHWAYSTATION_DISC_DIR`, `SCHWAYSTATION_BIOS_DIR`); committed `.env.example` lists the names with empty values. Only the local presets' CMake drivers parse it as `KEY=VALUE` lines; the core and runner read no environment. **Rec** this. **Alt** a gitignored `CMakeUserPresets.json`. **Flip** a second machine needs the same lanes.
- The secret-read guard refuses any agent command that names `.env`, `.env.<suffix>` (except `.example`, `.sample`, `.template`, `.dist`) or `.secrets` files (MU-GSD-008). Agents read `.env.example` only. The BIOS directory is never listed in a transcript or summary. CI holds one variable and one secret: `SCHWAYSTATION_APP_CLIENT_ID` (Actions variable) and `SCHWAYSTATION_APP_PRIVATE_KEY` (Actions secret and Dependabot secret), set by the owner through GitHub's App manifest flow (MU-REL-004; GSD-HANDOFF says how and how to rotate); agents never handle the key. App permissions: Contents, Pull requests, Issues and Workflows read/write, Metadata read. Attestations use OIDC (`id-token: write`).

## 14. WASM and low-power
- **Lane eight. Rec** `wasm32` preset: wasi-sdk 34 clang with `--target=wasm32-wasi`, wasmtime 48 LTS (48.0.7 since today) as `CMAKE_CROSSCOMPILING_EMULATOR`, both from GitHub releases and pinned by SHA-256 in `pins.txt` exactly as armips is (section 1); label `portable` per PR, its `hashes.txt` the eighth input to `hash-equality` once it has a record on `main` [ENG.17][ENG.23]. **Alt** Emscripten first. **Flip** wasi-sdk cannot build the runner's sector-callback shim (then the lane builds the library only and a 40-line WASI host runs the hash list).
- **Hash-equality claim:** the same fixtures give byte-identical `hashes.txt` on the six legs, `be32` and `wasm32`; the browser page is a seed and makes no claim of its own.
- **Memory:** instance ≈ 3.5–4 MB (2 MB RAM, 1 MB VRAM, 512 KB SPU RAM, 1 KB scratchpad, 4 KB I-cache, registers and queues) plus one 4 MB state buffer and a 16-sector read-ahead ring; a 32 MB linear memory with no growth and no threads holds it; the page streams sectors from a `File` through the callback, so a 700 MB image is never loaded; `state.bytes` is a `quality.json` ceiling (4,194,304 at v1).
- **Emscripten** (6.0.13 today [ENG.29]) comes later, for the browser page seed, as a nightly compile-only lane; it never replaces the WASI hash lane because its JavaScript glue hides core behaviour. **`-Os` versus `-O2`** is measured, not guessed: the nightly `bench` builds the `wasm32` library both ways and records size and Ir/frame (wasmtime's instruction count) in `quality.json`; the release flag is chosen at the tune-up.
- **Low power:** no busy loops anywhere (the CD-ROM seek, SPU and MDEC wait on the event scheduler, never on a spin); the host drives pacing: `schway_run_frame` and `schway_run(instance, budget, &stop)` return one stop reason from the single enum `FRAME`, `BUDGET`, `AUDIO_FULL`, `HOOK` (breakpoint, mailbox, tty line, exit port), `FAULT`, `LID` (ARCHITECTURE 1; HOSTS 7 uses the same names) and never sleep; audio leaves in batches into a caller buffer (one NTSC frame 527,799,972/715,909 = 737.244 stereo pairs on average at 44,100 Hz, PAL 886.489, derived) so a handheld or the page can run the core once per audio period; `core.no_busy_loop` proves a frame with the CD motor spinning and the SPU idle costs no more Ir than one with them stopped plus a fixed bound.
- **No floating point** and **zero allocations in the frame loop** are the section 2 and 8 rules; on wasm32 `abi.undefined` rejects any `__wasm_*` float helper import, and `core.no_alloc_after_load` runs under wasmtime. **Device budgets** (section 8) are ratchet inputs: `ir_per_frame.<fixture>` must stay under the slowest listed device's budget divided by two, or the gap is a named line in the roadmap (the cached interpreter milestone), never a silent exception.

## 15. Sibling lessons folded in
| Lesson | Rule here | Check |
|---|---|---|
| XFER-058 | exit status and tolerant regexes, never exact tool text | `policy.no_exact_output`; `check_junit.cmake` |
| XFER-060 | `CMAKE_OSX_SYSROOT` set in `hygiene`; per-OS `nm -u` allowlist | `hygiene` configure; `abi.undefined` |
| XFER-061 | `file(CONFIGURE … NEWLINE_STYLE UNIX)` for hash and inventory files | `hygiene.lf_only` |
| XFER-062 | scripts consume stdin before early exit | `policy.sh_stdin` |
| XFER-039 | drafts by release ID, `--paginate` | `release.draft_by_id` |
| XFER-063 | title job scans title and body for skip tokens | `ci.title` |
| XFER-064 | review before verification; rehearsal re-runs on input change, inside `CI required` | `ci.classify`; `ci.required` |
| XFER-065 | one `ci:` PR budgeted after the first merge | ROADMAP phase 1 exit |
| XFER-044 | e2e code ≤ 300 lines | `policy.e2e_budget` |
| XFER-067 | no stateful harness around RetroArch; every script also runs under dash | `hygiene.dash` |
| XFER-045 | no quotas, slots or ceremonies | discuss-step review of REQUIREMENTS and PLAN |
| XFER-051 | arm the Ir/frame gate in phase 1–2 after two zero-spread controls | `perf` job mode flag |
| XFER-056 | tarballs pinned by SHA-256; signature chain off the critical path | `fixtures.armips_pin.self_test`, `fixtures.mkpsxiso_pin.self_test` |
| XFER-071 | `switch` dispatch, no `const` pointer tables | `abi.globals` |
| XFER-073 | splitmix64 harness, not `theft` | `tests/prop.h` |
| XFER-076 | signature scans compare hex, never the literal | `hygiene.no_bios_bytes` |
| GB-CI-004/005 | timeouts scaled in one pass after registration | `policy.timeout_scaled` |
| GB-GSD-020 | reconfigure after committing | `build.revision_matches_head` |
| GB-CGB-002 | negative controls do not leak; `detect_leaks=0` in `expect_fail.cmake` | `*.self_test` under `asan` |
| MU-CI-002, MU-CI-003 | `.planning`-only PRs skip heavy jobs via `changes`; settings script with `--check` | `ci.classify`; tune-up loop |

## Open questions
Do `vars` resolve in Dependabot-triggered runs, and does an App merge of a workflow-file PR need `workflows: write`? · the armips pin (PSXDEV-EXTRACTS 4) and whether `master` or v0.11.0 assembles the GTE mnemonics the fixtures need · the BIOS stand-in's licence per file (PSXDEV-EXTRACTS 5) · `be32` and `wasm32` per-PR cost on PS1 fixtures (phase 1) · whether ppc32 GCC emits a libgcc call for the GTE's 64-bit multiply-accumulate (allowlist row) · whether wasmtime's instruction count is stable enough to be a ceiling · the device MIPS figures in section 8 until a device run.

## Sources
| ID | Tier | Source (URL) | Pin or access date | Supports |
|---|---|---|---|---|
| ENG.01 | T1 | github/docs `data/reusables/actions/supported-github-runners.md` (https://github.com/github/docs) | 9af3df3aa46b, accessed 2026-10-19 (tested) | labels, `ubuntu-latest` = 24.04, `macos-latest` = 26, `windows-latest` = 2025; vCPU/RAM as cogmaster read them [ENG.19] |
| ENG.02 | T1 | actions/runner-images README and `images/{ubuntu,macos,windows}/*-Readme.md` (https://github.com/actions/runner-images) | 82d1b86ad393, accessed 2026-10-19 (tested) | preinstalled versions, image versions, label-to-readme map |
| ENG.03 | T1 | actions/runner-images announcements #13045 (closed), #14602, #14747, #14748 (open) | accessed 2026-10-19 (tested) | Intel end, 26.04 GA, label moves |
| ENG.04 | T1 | GitHub REST `releases/latest` and `git/ref/tags` for the seven actions (https://api.github.com) | accessed 2026-10-19 (tested) | tags, SHAs, no newer releases |
| ENG.05 | T1 | googleapis/release-please `releases/latest` (v17.11.2); option semantics as read at v17.6.0 by muhtari (MU-REL-001) and cogmaster | accessed 2026-10-19; v17.6.0 (712fcf01effd, unverified today) | option semantics |
| ENG.06 | T1 | googleapis/release-please-action `package.json`, `package-lock.json` | v5.0.0 (45996ed1f6d0), accessed 2026-10-19 (tested) | bundles release-please 17.6.0 (`^17.6.0`) |
| ENG.07 | T1 | github/docs `data/reusables/actions/actions-do-not-trigger-workflows.md` | 9af3df3aa46b, file present 2026-10-19; content as cogmaster read it at dae83f7d28f3 | token trigger rules |
| ENG.08 | T1 | github/docs `content/code-security/reference/supply-chain-security/dependabot-on-actions.md` | 9af3df3aa46b, file present 2026-10-19; content as cogmaster read it | Dependabot restrictions |
| ENG.09 | T1 | github/docs `content/code-security/concepts/supply-chain-security/immutable-releases.md` | 9af3df3aa46b, file present 2026-10-19; content as cogmaster read it | tag lock, no reuse, editable notes, release attestation |
| ENG.10 | T1 | GitHub REST docs "Actions permissions" (https://docs.github.com/en/rest/actions/permissions) | accessed 2026-10-19 (tested: the three field names are on the page) | `sha_pinning_required`, workflow permission fields |
| ENG.11, ENG.12 | T1 | actions/create-github-app-token `action.yml` (v3.3.0, 64ab729a6294); actions/attest `action.yml` and README (v4.2.2, 1e69f48acb82) | inputs as used in muhtari `release.yml` [ENG.17], run 38942043999 green; files (unverified today) | token inputs; `subject-checksums` |
| ENG.13 | T1 | Launchpad published binaries, resolute amd64 (https://api.launchpad.net/devel/ubuntu) | accessed 2026-10-19 (tested) | Valgrind, cppcheck, qemu, cross GCC, wasi-libc |
| ENG.14 | T1 | Kingcom/armips `LICENSE.txt`, releases, `master` (https://github.com/Kingcom/armips) | v0.11.0; 62adab4ef30d; accessed 2026-10-19 (tested) | MIT, versions |
| ENG.15 | T1 | Lameguy64/mkpsxiso `LICENSE.md`, releases (https://github.com/Lameguy64/mkpsxiso) | v2.30 (2026-07-06); accessed 2026-10-19 (tested) | GPL-2.0, prebuilt assets |
| ENG.16 | T1 | Kitware/CMake releases; CMake manuals `cmake-presets(7)`, `ctest(1)` (https://cmake.org/cmake/help/latest/) | v4.4.4 (2026-10-02), accessed 2026-10-19 | presets v11 `jobs` |
| ENG.17 | T1 | muhtari `.github/workflows/{ci,release,nightly,dependabot-automerge}.yml`, `CMakePresets.json`, `cmake/MuhtariTests.cmake`, `tests/cmake/{expect_fail,check_junit,abi,no_float,perf_compare}.cmake`, `tests/abi/tests.cmake`, `tests/tests.cmake`, `pins.txt`, `scripts/fetch-pin.sh` (https://github.com/szTheory/muhtari) | ecbe7f52aa37 | implementation of every mechanism in sections 2–9 |
| ENG.18 | T1 | muhtari `.planning/phases/01-*/01-14-SUMMARY.md`, `01-16-SUMMARY.md`, `01-VERIFICATION.md`, `02-*/02-01-SUMMARY.md`, `02-02-SUMMARY.md`, `02-06-SUMMARY.md`, `02-VERIFICATION.md` | ecbe7f52aa37 | per-job wall times, 89 s run, be32 32–45 s, gate-code tally, hosted-only faults, rehearsal staleness |
| ENG.19 | T1 | cogmaster `.planning/preparation/ENGINEERING.md` (https://github.com/szTheory/cogmaster) | abd950941e93 | ancestor text; nesturbator, gabbaboy, glueyneo, lattice_stripe and exifcleaner measurements it cites (not re-fetched) |
| ENG.20 | T1 | cogmaster `.planning/LESSONS.md` | abd950941e93 | XFER-001…XFER-079 |
| ENG.21 | T1 | rtissera/libchdr `LICENSE.txt`, `README.md`, `CMakeLists.txt`, `deps/` (https://github.com/rtissera/libchdr); libretro/libretro-common `include/libretro.h` | v0.3.0 (93d8c239ff0d), `master` 47d794151b2b; libretro-common a3194c18ba65; accessed 2026-10-19 (tested) | BSD-3, options, vendored deps; MIT header |
| ENG.22 | T1 | OpenGSD core `workflows/ship.md` | 1.17.0 (via cogmaster [ENG.19]) | ship title, `[ci skip]` |
| ENG.23 | T1 | WebAssembly/wasi-sdk and bytecodealliance/wasmtime releases | wasi-sdk-34 (2026-08-25); wasmtime v49.0.4, v48.0.7 (2026-10-19); accessed 2026-10-19 (tested) | lane versions |
| ENG.24 | T1 | local runs: `gh api`, Launchpad and docs queries on 2026-10-19 | 2026-10-19 | (tested) results |
| ENG.25 | T1 | muhtari `.planning/LESSONS.md` | ecbe7f52aa37 | MU-CI-002/003, MU-REL-001/003/004, MU-GSD-002/008, MU-ARCH-001, MU-TEST-001, XFER-031 |
| ENG.26 | T1 | llvm/llvm-project releases | llvmorg-23.1.3 (2026-10-06), accessed 2026-10-19 | upstream LLVM ahead of the images |
| ENG.27 | T1 | GitHub Docs: workflow concurrency; workflow syntax `defaults.run.shell` (https://docs.github.com/en/actions) | as cogmaster read them 2026-10-15; (unverified today) | pending runs cancelled; `bash -e` without `pipefail` |
| ENG.28 | T1 | GCC Warning Options (https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html) | as cogmaster read them 2026-10-15; (unverified today) | warning flags |
| ENG.29 | T1 | emscripten-core/emscripten releases; grumpycoders/pcsx-redux repository licence via the GitHub licence API; pcsx-redux/nugget LICENSE | 6.0.13 (2026-10-19); pcsx-redux GPL-2.0 at `91285bf736af` (corrected 2026-10-19), nugget MIT at `9db6d88d23c4` | Emscripten version; BIOS stand-in licence lead |
