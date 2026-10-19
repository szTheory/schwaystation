# Stack

**Project:** schwaystation · **Researched:** 2026-10-19 · Details and pins: `.planning/preparation/ENGINEERING.md`.

| Layer | Choice | Why |
|---|---|---|
| Language | C17, `CMAKE_C_EXTENSIONS OFF`; public header also valid C++; integer-only, no `float` or `double` anywhere | Embeds in any host, console SDK or WASM; the R3000A has no FPU and the GTE, GPU, SPU and MDEC are fixed-point |
| Build | CMake (≥ 3.25 for embedders; presets v11 locally and in CI), Ninja, CTest; one entrypoint `cmake --workflow --preset <lane>` | No script to keep in step with CI |
| Runtime dependencies | None; the core links only `memcpy`/`memmove`/`memset`/`memcmp`; libchdr (BSD-3) behind `SCHWAYSTATION_CHD` only from v2.0 | Portability and determinism; build pain is a top frontend complaint |
| Vendored | `libretro.h` (MIT, libretro-common `a3194c18ba65`) only | The libretro ABI |
| Test harness | Plain CTest executables, an in-repo check header and a ~60-line seeded splitmix64 property harness | No framework dependency |
| Fixture tools (pinned by SHA-256; the `ci` preset builds armips from phase 2 and mkpsxiso from the discs phase) | armips (MIT, commit `62adab4ef30d`) for own PS-EXE fixtures; mkpsxiso v2.30 (GPL-2, build tool only) for the own disc fixture; OpenBIOS built from pcsx-redux/nugget `9db6d88d23c4` (MIT) with the pinned mipsel GCC on the Linux leg as the BIOS stand-in; SingleStepTests/r3000 sample committed, full set fetched nightly at `51e227447d0b`; ps1-tests GTE table transcoded from `f727802fead1`, executables and `vram.png` references fetched nightly at `build-158`; `gcc-15-powerpc-linux-gnu` 15.2 + `qemu-user` 10.2.1 for the big-endian lane; wasi-sdk 34 + wasmtime 48.0.7 LTS for the wasm32 lane; Valgrind 3.26.0 for instructions per frame | Reproducible test inputs with no committed third-party, Sony or MPL bytes |
| Formatting and analysis | clang-format 21, clang-tidy 21 analyzer checks | One version locally and in CI |
| CI/CD | GitHub Actions (Ubuntu 26.04 and 26.04-arm, macOS 15 and 15-intel, Windows 2025 and 11-arm), release-please 17.6.0 via its v5.0.0 action, a GitHub App token, artifact attestations | Automatic, verifiable releases |
| Hosts | RetroArch 1.22.2 (Metal build, DMG pinned by SHA-256) on the owner's Mac; the runner for automation and Playstead | No GUI in this repository |
